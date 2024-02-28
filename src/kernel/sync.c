/*
 * Implementation of sync primitives
 */

#include "sync.h"

#include <stdatomic.h>

#include <syslib/common.h>

#include "hardware/cpu_x86.h"
#include "lib/assertk.h"
#include "lib/todo.h"
#include "scheduler.h"
#include "interrupt.h"

#define unimplemented(msg) abortk(msg)

/* === Choose major sync primitive implementation strategy to use ===*/

enum sync_impl {
    SYNC_IMPL_COOP,
    SYNC_IMPL_NOINTERRUPT,
    SYNC_IMPL_ATOMIC,
};

static const enum sync_impl SYNC_IMPL = SYNC_IMPL_ATOMIC;

/* === Critical Sections === */

unsigned int nointerrupt_count_val = 1;

ATTR_EASY_ASM_CALL void nointerrupt_enter(void)
{
    interrupts_disable();
    nointerrupt_count_val++;
}

ATTR_EASY_ASM_CALL void nointerrupt_leave(void)
{
    nointerrupt_count_val--;
    if (nointerrupt_count_val == 0) {
        interrupts_enable();
    }
}

ATTR_EASY_ASM_CALL void nointerrupt_leave_delayed(void)
{
    nointerrupt_count_val--;
    assertf(nointerrupt_count_val == 0, "bad critical count: %d\n",
            nointerrupt_count_val);
}

ATTR_EASY_ASM_CALL unsigned int nointerrupt_count(void)
{
    return nointerrupt_count_val;
}

/* === Spinlock with busy waiting === */

/* --- Spinlock implementation: cooperative (no need for atomicity) --- */

static void spinlock_acquire_coop(spinlock_t *s)
{
    while (s->locked) yield();
    s->locked = true;
}

static void spinlock_release_coop(spinlock_t *s)
{
    s->locked = false;
}

/* --- Spinlock implementation: nointerrupt --- */

static void spinlock_acquire_nointerrupt(spinlock_t *s)
{
    nointerrupt_enter();
    while (s->locked) {
        nointerrupt_leave();
        yield();
        nointerrupt_enter();
    }
    s->locked = true;
    nointerrupt_leave();
}

static void spinlock_release_nointerrupt(spinlock_t *s)
{
    nointerrupt_enter();
    s->locked = 0;
    nointerrupt_leave();
}

/* --- Spinlock implementation: atomic --- */

static void spinlock_acquire_atomic(spinlock_t *s)
{
    todo_use(s);
    todo_noop();
}

static void spinlock_release_atomic(spinlock_t *s)
{
    todo_use(s);
    todo_noop();
}

/* --- Spinlock API --- */

void spinlock_acquire(spinlock_t *s)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: spinlock_acquire_coop(s); break;
    case SYNC_IMPL_NOINTERRUPT: spinlock_acquire_nointerrupt(s); break;
    case SYNC_IMPL_ATOMIC: spinlock_acquire_atomic(s); break;
    }
}

void spinlock_release(spinlock_t *s)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: spinlock_release_coop(s); break;
    case SYNC_IMPL_NOINTERRUPT: spinlock_release_nointerrupt(s); break;
    case SYNC_IMPL_ATOMIC: spinlock_release_atomic(s); break;
    }
}

/* === Blocking lock with attached wait-queue === */

/* --- Lock implementation: cooperative (no need for atomicity) --- */

static void lock_acquire_coop(lock_t *l)
{
    while (l->locked) {
        block(&l->wait_queue);
    }
    l->locked = true;
}

static void lock_release_coop(lock_t *l)
{
    l->locked = false;
    if (l->wait_queue) unblock(&l->wait_queue);
}

/* --- Lock implementation: nointerrupt --- */

static void lock_acquire_nointerrupt(lock_t *l)
{
    nointerrupt_enter();
    while (l->locked) {
        nointerrupt_leave();
        block(&l->wait_queue);
        nointerrupt_enter();
    }
    l->locked = true;
    nointerrupt_leave();
}

static void lock_release_nointerrupt(lock_t *l)
{
    nointerrupt_enter();
    l->locked = 0;
    if (l->wait_queue) unblock(&l->wait_queue);
    nointerrupt_leave();
}

/* --- Lock implementation: atomic --- */

static void lock_acquire_atomic(lock_t *l)
{
    todo_use(l);
    todo_noop();
}

static void lock_release_atomic(lock_t *l)
{
    todo_use(l);
    todo_noop();
}

/* --- Lock API --- */

void lock_acquire(lock_t *l)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: lock_acquire_coop(l); break;
    case SYNC_IMPL_NOINTERRUPT: lock_acquire_nointerrupt(l); break;
    case SYNC_IMPL_ATOMIC: lock_acquire_atomic(l); break;
    }
}

void lock_release(lock_t *l)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: lock_release_coop(l); break;
    case SYNC_IMPL_NOINTERRUPT: lock_release_nointerrupt(l); break;
    case SYNC_IMPL_ATOMIC: lock_release_atomic(l); break;
    }
}

/* === Condition Variables === */

/* --- Condvars implementation: cooperative (no atomicity needed) --- */

static void condition_wait_coop(lock_t *m, condition_t *c)
{
    lock_release(m);
    block(&c->wait_queue);
    lock_acquire(m);
}

static void condition_signal_coop(condition_t *c)
{
    if (c->wait_queue != NULL) {
        unblock(&c->wait_queue);
    }
}

static void condition_broadcast_coop(condition_t *c)
{
    while (c->wait_queue != NULL) {
        unblock(&c->wait_queue);
    }
}

/* --- Condvars implementation: nointerrupt --- */

static void condition_wait_nointerrupt(lock_t *m, condition_t *c)
{
    lock_release(m);
    block(&c->wait_queue);
    lock_acquire(m);
}

static void condition_signal_nointerrupt(condition_t *c)
{
    nointerrupt_enter();
    if (c->wait_queue != NULL) {
        unblock(&c->wait_queue);
    }
    nointerrupt_leave();
}

static void condition_broadcast_nointerrupt(condition_t *c)
{
    nointerrupt_enter();
    while (c->wait_queue != NULL) {
        unblock(&c->wait_queue);
    }
    nointerrupt_leave();
}

/* --- Condvars implementation: atomic --- */

static void condition_wait_atomic(lock_t *m, condition_t *c)
{
    todo_use(m);
    todo_use(c);
    todo_noop();
}

static void condition_signal_atomic(condition_t *c)
{
    todo_use(c);
    todo_noop();
}

static void condition_broadcast_atomic(condition_t *c)
{
    todo_use(c);
    todo_noop();
}

/* --- Condvars API --- */

/*
 * unlock m and block the thread (enqued on c), when unblocked acquire
 * lock m
 */
void condition_wait(lock_t *m, condition_t *c)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: condition_wait_coop(m, c); break;
    case SYNC_IMPL_NOINTERRUPT: condition_wait_nointerrupt(m, c); break;
    case SYNC_IMPL_ATOMIC: condition_wait_atomic(m, c); break;
    }
}

/* unblock first thread enqued on c */
void condition_signal(condition_t *c)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: condition_signal_coop(c); break;
    case SYNC_IMPL_NOINTERRUPT: condition_signal_nointerrupt(c); break;
    case SYNC_IMPL_ATOMIC: condition_signal_atomic(c); break;
    }
}

/* unblock all threads enqued on c */
void condition_broadcast(condition_t *c)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: condition_broadcast_coop(c); break;
    case SYNC_IMPL_NOINTERRUPT: condition_broadcast_nointerrupt(c); break;
    case SYNC_IMPL_ATOMIC: condition_broadcast_atomic(c); break;
    }
}

/* === Semaphore === */

/* --- Semaphore impl: nointerrupt --- */

static void semaphore_up_nointerrupt(semaphore_t *s)
{
    nointerrupt_enter();
    s->value++;
    if (s->value <= 0 && s->wait_queue) {
        unblock(&s->wait_queue);
    }
    nointerrupt_leave();
}

static void semaphore_down_nointerrupt(semaphore_t *s)
{
    nointerrupt_enter();
    s->value--;
    if (s->value < 0) {
        nointerrupt_leave();
        block(&s->wait_queue);
        nointerrupt_enter();
    }
    nointerrupt_leave();
}

/* --- Semaphore impl: atomic --- */

static void semaphore_up_atomic(semaphore_t *s)
{
    todo_use(s);
    todo_noop();
}

static void semaphore_down_atomic(semaphore_t *s)
{
    todo_use(s);
    todo_noop();
}

/* --- Semaphore API --- */

void semaphore_up(semaphore_t *s)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: unimplemented("coop semaphores"); break;
    case SYNC_IMPL_NOINTERRUPT: semaphore_up_nointerrupt(s); break;
    case SYNC_IMPL_ATOMIC: semaphore_up_atomic(s); break;
    }
}

void semaphore_down(semaphore_t *s)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: unimplemented("coop semaphores"); break;
    case SYNC_IMPL_NOINTERRUPT: semaphore_down_nointerrupt(s); break;
    case SYNC_IMPL_ATOMIC: semaphore_down_atomic(s); break;
    }
}

/* === Barrier === */

/* Wait at barrier until all n threads reach it */
void barrier_wait(barrier_t *b)
{
    lock_acquire(&b->mutex);

    if (b->count_up) {
        b->count1++;
        while (b->count1 < b->threads)
            condition_wait(&b->mutex, &b->all_arrived);
        /*
         * Only the last thread that reachs the barrier
         * executes this code
         */
        if (b->count_up) {
            /* Wakeup other threds */
            condition_broadcast(&b->all_arrived);
            b->count2   = b->threads;
            b->count_up = 0;
        }
    } else {
        b->count2--;
        while (b->count2 > 0) condition_wait(&b->mutex, &b->all_arrived);
        /*
         * Only the last thread that reachs the barrier
         * executes this code
         */
        if (b->count_up == 0) {
            /* Wakeup other threds */
            condition_broadcast(&b->all_arrived);
            b->count1   = 0;
            b->count_up = 1;
        }
    }
    lock_release(&b->mutex);
}

