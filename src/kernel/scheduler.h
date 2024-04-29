#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdnoreturn.h>

#include <syslib/common.h>

#include "hardware/cpu_x86.h"
#include "pcb.h"
#include "sync.h"

enum thread_status {
    STATUS_FIRST_TIME = 0,
    STATUS_READY,
    STATUS_SLEEPING,
    STATUS_BLOCKED,
    STATUS_EXITED,
};

/*
 * CPU flags to use when starting a new thread/process
 *
 * - IOPL 0: only kernel can use I/O ports
 * - IF: interrupts enabled
 */
static const ureg_t INIT_EFLAGS = ((PL0 << EFLAGS_IOPL_SHIFT) | EFLAGS_IF);

/* The currently running process, and also a pointer to the ready queue */
extern pcb_t *current_running;

extern uint32_t running_processes;

/* Low-level dispatch to next task. Defined in assembly. */
void dispatch(void);

/* Calls scheduler to run the 'next' process */
void yield(void);

/* Preempt the current task */
void preempt(void);

/* Remove current running from ready queue and insert it into 'q'. */
void block(pcb_t **q);

/* Move first process in 'q' into the ready queue */
void unblock(pcb_t **q);

/*
 * Set status = STATUS_EXITED and call scheduler_entry() which will remove the
 * job from the ready queue and pick next process to run.
 */
noreturn void exit(void);

/* === Get and set process priority === */

int  getpriority(void);
void setpriority(int);

#endif /* !SCHEDULER_H */
