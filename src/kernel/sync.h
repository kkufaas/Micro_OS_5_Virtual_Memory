/*
 * Implementation of sync primitives
 */
#ifndef _SYNC_H
#define _SYNC_H

#include <stdbool.h>

#include <syslib/common.h>
#include <syslib/compiler_compat.h>

/* Forward declare the pcb type to avoid circular includes. */
struct pcb;
typedef struct pcb pcb_t;

/* === Blocking lock with attached wait-queue === */

struct _lock {
    /* TODO: Design your lock struct.
     * You'll need at the very least a flag and a wait queue. */
};

typedef struct _lock lock_t;

#define LOCK_INIT \
    { \
    }

void lock_acquire(lock_t *);
void lock_release(lock_t *);

#endif /* !_SYNC_H */
