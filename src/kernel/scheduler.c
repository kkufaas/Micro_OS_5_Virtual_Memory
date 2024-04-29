#include <stdnoreturn.h>

#include <syslib/compiler_compat.h>
#include <util/util.h>

#include "hardware/cpu_x86.h"
#include "hardware/intctl_8259.h"
#include "cpu.h"
#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"
#include "scheduler.h"
#include "sync.h"
#include "time.h"


// shaky - can become circular include
#include "memory.h"

/* Ready queue and pointer to currently running process */
pcb_t *current_running;
uint32_t running_processes;

/* Helper function for dispatch() */
void setup_current_running(void)
{
    /* Restore harware interrupt mask */
    pic_set_mask(current_running->int_controller_mask);

    /* Load pointer to the page directory of current_running into CR3 */
    set_page_directory(current_running->page_directory);

    if (!current_running->is_thread) { /* process */
        cpu_set_interrupt_stack(current_running->base_kernel_stack);
    }
}

/*
 * Choose and dispatch next task, do maintenance on ready queue
 *
 * The scheduler picks the next job to run, and removes blocked and exited
 * processes from the ready queue. Then it calls dispatch to start the
 * chosen task.
 */
void scheduler(void)
{
    nointerrupt_enter();
    unsigned long long t;

    /*
     * Save hardware interrupt mask in the pcb struct. The mask
     * will be restored in setup_current_running()
     */
    current_running->int_controller_mask = pic_get_mask();

    do {
        switch (current_running->status) {

        case STATUS_SLEEPING:
            t = read_cpu_ticks();
            if (current_running->wakeup_time < t)
                current_running->status = STATUS_READY;

            FALLTHROUGH;

        case STATUS_FIRST_TIME:
        case STATUS_READY:
            /* pick the next job to run */
            current_running = current_running->next;
            break;

        case STATUS_BLOCKED:
            queue_shift(&current_running);
            assertf(current_running != NULL, "no more jobs");
            break;

        case STATUS_EXITED: {
            struct pcb *outgoing = queue_shift(&current_running);
            assertf(current_running != NULL, "no more jobs");
            free_pcb(outgoing);
            break;
        }

        default: assertf(0, "Invalid job status."); break;
        }
    } while (current_running->status != STATUS_READY
             && current_running->status != STATUS_FIRST_TIME);

    /* .. and run it */
    dispatch();
    nointerrupt_leave();
}

/*
 * Save task state and enter the scheduler (defined in assembly)
 *
 * This low-level function saves the context of the 'current_running' task
 * and then calls 'scheduler()' to pick the next task. On the return path,
 * it restores the state of the new 'current_running' and returns to where
 * it left off.
 *
 * Because this function has to do low-level manipulation of stack and
 * registers, it must be written in assembly. The implementation is in
 * scheduler_asm.S
 */
extern void scheduler_entry(void);

/* Call scheduler to run the 'next' process */
void yield(void)
{
    nointerrupt_enter();
    current_running->yield_count++;
    scheduler_entry();
    nointerrupt_leave();
}

void preempt(void)
{
    nointerrupt_enter();
    current_running->preempt_count++;
    scheduler_entry();
    nointerrupt_leave();
}

void block(pcb_t **q)
{
    nointerrupt_enter();
    assertk(q != NULL);

    current_running->status = STATUS_BLOCKED;

    /* Put current task into give blocked queue */
    if (*q == NULL) {
        *q = current_running;
    } else {
        pcb_t *p = *q;
        while (p->next != NULL) { // Navigate to end of queue
            p = p->next;
        }
        p->next = current_running; // Put current task at the end
    }

    /* remove job from ready queue, pick next job to run and dispatch it */
    scheduler_entry();

    nointerrupt_leave();
}

void unblock(pcb_t **q)
{
    nointerrupt_enter();
    assertk(q != NULL);
    assertk(*q != NULL);

    /* Pull a task from the blocked queue */
    pcb_t *job;
    job = queue_shift(q);

    /* Put it back into the ready queue */
    job->status = STATUS_READY;
    queue_insert(&current_running, job);

    nointerrupt_leave();
}

/*
 * Remove the current_running process from the linked list so it will
 * not be scheduled in the future
 */
noreturn void exit(void)
{
    nointerrupt_enter();
    current_running->status = STATUS_EXITED;

    if ( !(current_running -> is_thread) ) {
        pr_debug("process exited \n");
        running_processes -= 1;
        pr_debug("current number of running processes %u \n", running_processes);
    }
    //     free_done_process_memory(current_running);
    // }

    /* Removes job from ready queue, and dispatches next job to run */
    scheduler_entry();
    /* No need to leave the critical section. This code is unreachable. */
    assertf(0, "control returned to exited thread");
}

/* === Get and set process priority === */

/* Get task priority (exported as syscall) */
int getpriority(void) { return current_running->priority; }

/* Set task priority (exported as syscall) */
void setpriority(int p) { current_running->priority = p; }

