#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdnoreturn.h>

#include <syslib/common.h>

#include "kernel.h"

enum thread_status {
    STATUS_FIRST_TIME = 0,
    STATUS_READY,
    STATUS_BLOCKED,
    STATUS_EXITED,
};

/* The currently running process, and also a pointer to the ready queue */
extern pcb_t *current_running;

/* Low-level dispatch to next task. Defined in assembly. */
void dispatch(void);

/* Calls scheduler to run the 'next' process */
void yield(void);

/* Remove current running from ready queue and insert it into 'q'. */
void block(pcb_t **q);

/* Move first process in 'q' into the ready queue */
void unblock(pcb_t **q);

/*
 * Set status = STATUS_EXITED and call scheduler_entry() which will remove the
 * job from the ready queue and pick next process to run.
 */
noreturn void exit(void);

#endif /* !SCHEDULER_H */
