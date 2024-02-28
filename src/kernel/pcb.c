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
    assertf(freelist, "no free pcb structs");
    p = queue_shift(&freelist);
    return p;
}

/* put the pcb back into the free list */
void free_pcb(pcb_t *p)
{
    queue_insert(&freelist, p);
}

static void create_pcb_common(struct pcb *p)
{
    p->pid = next_pid++;

    /* allocate kernel stack */
    assertf(next_stack < T_KSTACK_AREA_MAX_PADDR, "Out of stack space");
    p->kernel_stack = next_stack + T_KSTACK_START_OFFSET;
    p->base_kernel_stack = p->kernel_stack;
    next_stack += T_KSTACK_SIZE_EACH;

    p->priority = 10;
    p->status = STATUS_FIRST_TIME;

    p->preempt_count = 0;
    p->yield_count   = 0;

    p->int_controller_mask = ~IRQS_TO_ENABLE;
}

/*
 * Allocate and set up the pcb for a new thread, allocate resources
 * for it and insert it into the ready queue.
 */
int create_thread(uintptr_t start_addr)
{

    pcb_t *p = alloc_pcb();
    create_pcb_common(p);

    p->is_thread = true;

    p->nested_count = 1;

    p->user_stack = 0; /* threads don't have a user stack */

    p->start_pc = start_addr;

    /* Kernel code segment selector, RPL = 0 (kernel mode) */
    p->cs = KERNEL_CS;
    p->ds = KERNEL_DS;

    p->base  = 0; /* only set for processes */
    p->limit = 0; /* only set for processes */
    /* Sets p->page_directory = &(created page directory) */
    setup_process_vmem(p);

    queue_insert(&current_running, p);
    return 0;
}

/*
 * Allocate and set up the pcb for a new process, allocate resources
 * for it and insert it into the ready queue.
 */
int create_process(uint32_t base, uint32_t size)
{
    nointerrupt_enter();

    pcb_t *p = alloc_pcb();
    create_pcb_common(p);

    p->is_thread = false;

    p->nested_count = 0;

    /* setup user stack */
    p->user_stack =
            PROCESS_VADDR + size - T_KSTACK_SIZE_EACH + T_KSTACK_START_OFFSET;

    /* Each processes has its own address space */
    p->start_pc = PROCESS_VADDR;

    /* process code segment selector, with RPL = 3 (user mode) */
    p->cs = PROCESS_CS | 3;
    p->ds = PROCESS_DS | 3;

    p->base  = base;
    p->limit = size;
    setup_process_vmem(p);

    queue_insert(&current_running, p);

    nointerrupt_leave();
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
    todo_use(buf);
    todo_noop();
    return -1;
}

/*
 * Load a process from disk.
 *
 * Location is sector number on disk, size is number of sectors.
 */
int loadproc(int location, int size)
{
    todo_use(location);
    todo_use(size);
    todo_noop();
    return -1;
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
    static const int W_KSTACK  = 5;

    tprintf(&procterm, "%*s", W_PID, "Pid");
    tprintf(&procterm, " %*s", W_TYPE, "Type");
    tprintf(&procterm, " %*s", W_STATUS, "St");
    if (W_PREEMPT) tprintf(&procterm, " %*s", W_PREEMPT, "Pmpt");
    if (W_YIELD) tprintf(&procterm, " %*s", W_YIELD, "Yld");
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
        if (W_KSTACK) tprintf(&procterm, " %*x", W_KSTACK, p->kernel_stack);
        tprintf(&procterm, ANSIF_EL "\n",
                ANSI_EFWD); // Clear right, then newline
    }

    /* Clear rest of window below cursor. */
    tprintf(&procterm, ANSIF_ED, ANSI_EFWD);

    nointerrupt_leave();

}

