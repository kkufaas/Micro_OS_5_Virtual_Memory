#include <stdnoreturn.h>

#include <syslib/compiler_compat.h>
#include <util/util.h>

#include "hardware/cpu_x86.h"
#include "cpu.h"
#include "kernel.h"
#include "lib/assertk.h"
#include "lib/todo.h"
#include "scheduler.h"

/* Ready queue and pointer to currently running process */
pcb_t *current_running;

/*
 * Choose and dispatch next task, do maintenance on ready queue
 *
 * The scheduler picks the next job to run, and removes blocked and exited
 * processes from the ready queue. Then it calls dispatch to start the
 * chosen task.
 */
void scheduler(void)
{
    todo_abort();
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
    todo_noop();
}

void block(pcb_t **q)
{
    todo_use(q);
    todo_noop();
}

void unblock(pcb_t **q)
{
    todo_use(q);
    todo_noop();
}

/*
 * Remove the current_running process from the linked list so it will
 * not be scheduled in the future
 */
noreturn void exit(void)
{
    todo_noop();
    assertf(0, "control returned to exited thread");
}

