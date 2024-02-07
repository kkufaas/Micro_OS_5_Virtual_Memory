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

/* === Uninterruptable Section: turn off interrupts === */

ATTR_EASY_ASM_CALL void         nointerrupt_enter(void);
ATTR_EASY_ASM_CALL void         nointerrupt_leave(void);
ATTR_EASY_ASM_CALL void         nointerrupt_leave_delayed(void);
ATTR_EASY_ASM_CALL unsigned int nointerrupt_count(void);

/* === Spinlock with busy waiting === */

struct _spinlock {
    /* TODO: design your spinlock struct. */
};

typedef struct _spinlock spinlock_t;

#define SPINLOCK_INIT \
    { \
    }

void spinlock_acquire(spinlock_t *s);
void spinlock_release(spinlock_t *s);

/* === Blocking lock with attached wait-queue === */

struct _lock {
    bool         locked;
    pcb_t       *wait_queue;
};

typedef struct _lock lock_t;

#define LOCK_INIT \
    { \
    }

void lock_acquire(lock_t *);
void lock_release(lock_t *);

/* === Condition variable === */

struct _condvar {
    /* TODO: Design your condition variable struct */
};

typedef struct _condvar condition_t;

#define CONDITION_INIT \
    { \
    }

void condition_wait(lock_t *m, condition_t *c);
void condition_signal(condition_t *c);
void condition_broadcast(condition_t *c);

/* === Semaphore === */

struct _semaphore {
    /* TODO: Design your semaphore struct */
};

typedef struct _semaphore semaphore_t;

#define SEMAPHORE_INIT(VAL) \
    { \
    }

void semaphore_up(semaphore_t *s);
void semaphore_down(semaphore_t *s);

/* === Barrier === */

/* Barrier struct, for a simple shared variable barrier */
struct _barrier {
    /* TODO: Design your barrier struct */
};

typedef struct _barrier barrier_t;

#define BARRIER_INIT(N) \
    { \
    }

/*
 * Make calling process wait at barrier until all N processes have
 * called barrier_wait()
 */
void barrier_wait(barrier_t *b);

#endif /* !_SYNC_H */
