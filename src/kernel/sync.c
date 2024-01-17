/*
 * Implementation of sync primitives
 */

#include "sync.h"

#include <syslib/common.h>

#include "lib/assertk.h"
#include "lib/todo.h"
#include "scheduler.h"

#define unimplemented(msg) abortk(msg)

/* === Choose major sync primitive implementation strategy to use ===*/

enum sync_impl {
    SYNC_IMPL_COOP,
};

static const enum sync_impl SYNC_IMPL = SYNC_IMPL_COOP;

/* === Blocking lock with attached wait-queue === */

/* --- Lock implementation: cooperative (no need for atomicity) --- */

static void lock_acquire_coop(lock_t *l)
{
    todo_use(l);
    todo_noop();
}

static void lock_release_coop(lock_t *l)
{
    todo_use(l);
    todo_noop();
}

/* --- Lock API --- */

void lock_acquire(lock_t *l)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: lock_acquire_coop(l); break;
    }
}

void lock_release(lock_t *l)
{
    switch (SYNC_IMPL) {
    case SYNC_IMPL_COOP: lock_release_coop(l); break;
    }
}

