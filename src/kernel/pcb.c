#include <stddef.h>

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/addrs.h>
#include <syslib/screenpos.h>

#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"

#include "cpu.h"
#include "hardware/intctl_8259.h"
#include "interrupt.h"
#include "memory.h"
#include "pcb.h"
#include "scheduler.h"
#include "sync.h"

/* Required for dynamic loading */

#include <string.h>

#include "usb/scsi.h"
#include "usb/usb.h"

#include "sleep.h"

#include "stdlib.h"


// four pinned for stack, page directory and two page tables for kernel and user space
// three on average required. This last number could be modeled better with. eg
// a percentage of the size of the images loaded to memory. The idea is to
// keep competition for page frames at tolerable level to avoid thrashing.
#define AVERAGE_PAGES_PER_PROCESS 7
#define NEW_PROCESS_WAIT_TIME_FOR_PAGES 1000 // millisecs


lock_t load_process_lock_debug = LOCK_INIT;

/* === Get PCB info === */

/* Get process PID (exported as syscall) */
int getpid(void) { return current_running->pid; }

/* === Threads as a doubly-linked ring queue === */

/*
 * Insert thread p into queue q
 */
void queue_insert(struct pcb **q, struct pcb *p)
{
    nointerrupt_enter();
    assertf(q, "pointer to queue must be non-null");
    assertf(p->next == NULL, "thread is already in another queue");

    if (!*q) {
        /* Queue is empty: make a queue of one. */
        p->next     = p;
        p->previous = p;
        *q          = p;
    } else {
        /* Insert into queue. */
        p->previous = (*q)->previous; // End of list.
        p->next     = (*q);           // Wrap aroud to beginning.

        p->previous->next = p;
        p->next->previous = p;
    }
    nointerrupt_leave();
}

/*
 * Pull the next thread from queue q
 *
 * Returns a pointer to the removed thread.
 * If the queue is empty, returns null.
 */
struct pcb *queue_shift(struct pcb **q)
{
    nointerrupt_enter();
    struct pcb *p;
    assertf(q, "pointer to queue must be non-null");
    if (!*q) {
        p = NULL;
    } else {
        p = *q;
        if (p->next == p && p->previous == p) {
            /* This was a queue of one. Set queue to empty. */
            *q = NULL;
        } else {
            /* Remove the thread from the queue. */
            p->previous->next = p->next;
            p->next->previous = p->previous;
            *q                = p->next;
        }
        p->previous = NULL;
        p->next     = NULL;
    }
    nointerrupt_leave();
    return p;
}

/*
 * Find position of thread p in queue q
 *
 * Returns -1 if the thread is not in the queue.
 */
static int queue_pos(struct pcb **q, struct pcb *p)
{
    static const int LIMIT = PCB_TABLE_SIZE;

    nointerrupt_enter();
    assertf(q, "pointer to queue must be non-null");

    int pos;
    if (*q == NULL) {
        /* Queue is empty. */
        pos = -1;
        goto end;
    }

    struct pcb *start = *q;
    struct pcb *cur   = *q;
    int         i     = 0;

    for (;;) {
        if (cur == p) {
            /* Found it. */
            pos = i;
            goto end;
        }

        cur = cur->next;
        i++;

        assertf(cur != NULL, "malformed queue does not wrap");
        assertf(i <= LIMIT, "malformed queue goes too far");

        if (cur == start) {
            /* We have wrapped back around to the beginning. */
            pos = -1;
            goto end;
        }
    }

end:
    nointerrupt_leave();
    return pos;
}

/*
 * Remove thread p from queue q
 */
void queue_remove(struct pcb **q, struct pcb *p)
{
    nointerrupt_enter();
    assertf(q, "pointer to queue must be non-null");
    assertf(queue_pos(q, p) >= 0, "thread must be in queue to remove");

    if (*q == p && p->next == p) {
        /* p was the only thread in the queue. Mark it as empty. */
        *q = NULL;
    } else if (*q == p) {
        /* p was the head of the queue. Advance the head. */
        *q = p->next;
    }

    /* Stitch p out of the linked list. */
    p->previous->next = p->next;
    p->next->previous = p->previous;

    p->next     = NULL;
    p->previous = NULL;

    nointerrupt_leave();
}

/* === PCB Allocation === */

/* Statically allocate some storage for the pcb's */
pcb_t pcb[PCB_TABLE_SIZE];

/* Used for allocation of pids, kernel stack, and pcbs */
static pcb_t    *freelist   = NULL;
static int       next_pid   = 0;
static uintptr_t next_stack = T_KSTACK_AREA_MIN_PADDR;

/* Initialize pcb table before allocating pcbs */
void init_pcb_table(void)
{
    /* Put entire PCB table into the freelist. */
    freelist = NULL;
    for (struct pcb *p = pcb; p < pcb + PCB_TABLE_SIZE; p++) {
        *p = (struct pcb){
                .next     = NULL,
                .previous = NULL,
        };
        queue_insert(&freelist, p);
    }

    /* current_running also serves as the ready queue pointer */
    current_running             = NULL;
}

/* Get a free pcb */
static pcb_t *alloc_pcb()
{
    pcb_t *p;
    nointerrupt_enter();
    assertf(freelist, "no free pcb structs");
    p = queue_shift(&freelist);
    nointerrupt_leave();
    return p;
}

/* put the pcb back into the free list */
void free_pcb(pcb_t *p)
{
    nointerrupt_enter();
    queue_insert(&freelist, p);
    nointerrupt_leave();
}

static void create_pcb_common(struct pcb *p)
{
    nointerrupt_enter();
    p->pid = next_pid++;
    nointerrupt_leave();

    /* allocate kernel stack */
    assertf(next_stack < T_KSTACK_AREA_MAX_PADDR, "Out of stack space");
    p->kernel_stack = next_stack + T_KSTACK_START_OFFSET;
    p->base_kernel_stack = p->kernel_stack;
    next_stack += T_KSTACK_SIZE_EACH;

    p->priority = 10;
    p->status = STATUS_FIRST_TIME;

    p->preempt_count = 0;
    p->yield_count   = 0;
    p->page_fault_count = 0;

    p->int_controller_mask = ~IRQS_TO_ENABLE;
}

/*
 * Allocate and set up the pcb for a new thread, allocate resources
 * for it and insert it into the ready queue.
 */
int create_thread(uintptr_t start_addr)
{
    nointerrupt_enter();

    pcb_t *p = alloc_pcb();
    create_pcb_common(p);

    p->is_thread = true;

    nointerrupt_leave();

    p->nested_count = 1;

    p->user_stack = 0; /* threads don't have a user stack */

    p->start_pc = start_addr;

    /* Kernel code segment selector, RPL = 0 (kernel mode) */
    p->cs = KERNEL_CS;
    p->ds = KERNEL_DS;

    p->swap_loc  = 0;
    p->swap_size = 0;
    /* Sets p->page_directory = &(created page directory) */
    setup_process_vmem(p);

    queue_insert(&current_running, p);
    return 0;
}

static uint32_t first_process = 1;
static uint32_t wait_load = 0;
/*
 * Allocate and set up the pcb for a new process, allocate resources
 * for it and insert it into the ready queue.
 */

int create_process(uint32_t location, uint32_t size)
{

    lock_acquire(&load_process_lock_debug);
    if (first_process) {
        // initialize running_processes
        running_processes = 0;
        first_process = 0;
    }


    // rough estimate - can be improved upon by using eg. p->swap_size for each
    // process to estimate the number of pages used

    // uint32_t page_space_available = PAGEABLE_PAGES - running_processes*AVERAGE_PAGES_PER_PROCESS;
    // while(page_space_available < AVERAGE_PAGES_PER_PROCESS + 1) {
    //     pr_debug("create_process: too much competition for pages: sleeping while others finish\n");
    //     pr_debug("create_process: too much competition for pages: currently %u processes running\n", running_processes);
    //     page_space_available = PAGEABLE_PAGES - running_processes*AVERAGE_PAGES_PER_PROCESS;
    //     wait_load = 1;
    //     msleep(NEW_PROCESS_WAIT_TIME_FOR_PAGES);
    // }
    // // wait a random time to load a process that was put to sleep
    // if (wait_load) {
    //     msleep(245 + (rand() % 420));
    // }

    running_processes += 1;
    nointerrupt_enter();
    pr_debug("loading process at location %u with size %u\n", location, size);
    nointerrupt_leave();

    nointerrupt_enter();
    pcb_t *p = alloc_pcb();
    create_pcb_common(p);

    p->is_thread = false;

    p->nested_count = 0;

    /* setup user stack */
    p->user_stack = PROCESS_STACK_VADDR;

    /* Each processes has its own address space */
    p->start_pc = PROCESS_VADDR;

    /* process code segment selector, with RPL = 3 (user mode) */
    p->cs = PROCESS_CS | 3;
    p->ds = PROCESS_DS | 3;

    p->swap_loc  = location;
    p->swap_size = size;

    nointerrupt_leave();

    setup_process_vmem(p);

    nointerrupt_enter();
    queue_insert(&current_running, p);
    nointerrupt_leave();

    lock_release(&load_process_lock_debug);

    return 0;
}

/* === Dynamic Process Loading === */

/*
 * Read the directory from the USB and copy it to the user provided buf.
 *
 * NOTE that block size equals sector size.
 */
int readdir(unsigned char *buf)
{
    /*
     * NOTE avoid passing 'buf'-argument directly to scsi_read(),
     * use an intermidary buffer on the stack.
     * This is not a problem when loader_thread() spawns the shell
     * since that argument address is in kernel-space.
     * The problem comes when the virtual-address argument pointer
     * refers to something in process-space. Which is not a problem
     * in kernel space because addresses are one-to-one with
     * physical-address.
     */
    char      internal_buf[SECTOR_SIZE];
    int       rc;

    uint16_t  os_size;         /* in sectors */
    const int OS_SIZE_LOC = 2; /* Position within bootblock */

    /* read the boot block */
    pr_info("reading bootblock\n");
    /* bootblock is in block 0 */
    rc = scsi_read(0, 1, internal_buf);

    if (rc < 0) return -1;

    os_size = *((uint16_t *) (internal_buf + OS_SIZE_LOC));

    /* now skip the kernel, and read the directory */
    pr_info("reading process directory\n");
    rc = scsi_read(os_size + 1, 1, internal_buf);

    if (rc < 0) return -1;

    /* we are done! */
    bcopy(internal_buf, (char *) buf, SECTOR_SIZE);

    return 0;
}


/*
 * Load a process from disk.
 *
 * Location is sector number on disk, size is number of sectors.
 */
int loadproc(int location, int size)
{
    /*
     * With swap enabled, we no longer have to pre-load the process binary
     * from disk. We merely set up the mapping between memory and disk,
     * and let the page-fault handler swap pages in on demand.
     */
    return create_process(location, size);
}

/* === Print Process Status Table === */

static struct term procterm = PROCTAB_TERM_INIT;

/*
 * Used for debugging. The clock thread will call this function every
 * so often to update the status info.
 */
void print_pcb_table(void)
{
    procterm.overflow = TERM_OF_CLIP;

    /* Reset to top of window and print header. */
    tprintf(&procterm, ANSIF_CUP, 1, 1);

    tprintf(&procterm, "spurious IRQ:%5d", spurious_irq_ct);
    tprintf(&procterm, ANSIF_EL "\n", ANSI_EFWD); // Clear right, then newline

    tprintf(&procterm, "== Process status ==");
    tprintf(&procterm, ANSIF_EL "\n", ANSI_EFWD); // Clear right, then newline

    static const char *status[] = {
        [STATUS_FIRST_TIME] = "1st",
        [STATUS_READY]      = "Run",
        [STATUS_BLOCKED]    = "Blk",
        [STATUS_SLEEPING]   = "Slp",
        [STATUS_EXITED]     = "Exd"
    };

    /* Column widths. Use negative for left-align, 0 to skip. */
    static const int W_PID     = 3;
    static const int W_TYPE    = -4;
    static const int W_STATUS  = -3;
    static const int W_PREEMPT = 6;
    static const int W_YIELD   = 6;
    static const int W_PGFLT   = 5;
    static const int W_KSTACK  = 5;

    tprintf(&procterm, "%*s", W_PID, "Pid");
    tprintf(&procterm, " %*s", W_TYPE, "Type");
    tprintf(&procterm, " %*s", W_STATUS, "St");
    if (W_PREEMPT) tprintf(&procterm, " %*s", W_PREEMPT, "Pmpt");
    if (W_YIELD) tprintf(&procterm, " %*s", W_YIELD, "Yld");
    if (W_PGFLT) tprintf(&procterm, " %*s", W_PGFLT, "PgFlt");
    if (W_KSTACK) tprintf(&procterm, " %*s", W_KSTACK, "KStck");
    tprintf(&procterm, ANSIF_EL "\n", ANSI_EFWD); // Clear right, then newline

    /* Print the process table. */
    nointerrupt_enter();
    for (struct pcb *p = pcb; p < pcb + PCB_TABLE_SIZE; p++) {
        /* Skip unused and existed threads. */
        if (p->pid == 0 || p->status == STATUS_EXITED) continue;

        tprintf(&procterm, "%*d", W_PID, p->pid);
        tprintf(&procterm, " %*s", W_TYPE, p->is_thread ? "Thrd" : "Proc");
        tprintf(&procterm, " %*s", W_STATUS, status[p->status]);
        if (W_PREEMPT) tprintf(&procterm, " %*d", W_PREEMPT, p->preempt_count);
        if (W_YIELD) tprintf(&procterm, " %*d", W_YIELD, p->yield_count);
        if (W_PGFLT) tprintf(&procterm, " %*d", W_PGFLT, p->page_fault_count);
        if (W_KSTACK) tprintf(&procterm, " %*x", W_KSTACK, p->kernel_stack);
        tprintf(&procterm, ANSIF_EL "\n",
                ANSI_EFWD); // Clear right, then newline
    }

    /* Clear rest of window below cursor. */
    tprintf(&procterm, ANSIF_ED, ANSI_EFWD);

    nointerrupt_leave();

}

