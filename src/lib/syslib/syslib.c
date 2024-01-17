/*
 * Implementation of the system call library for the user processes.
 */

#include "syslib.h"

#include <syslib/addrs.h>

#include "common.h"

enum syscall_impl {
    SYSCALL_IMPL_FIXEDPOINT,
};

static const enum syscall_impl SYSCALL_IMPL = SYSCALL_IMPL_FIXEDPOINT;

/*
 * Invoke a syscall via fixed-address pointer
 */
static inline int
invoke_syscall_fixedpoint(int syscall_id, int arg1, int arg2, int arg3)
{
    /* Declare the entry function type. */
    typedef int entry_fn_t(int syscall_id, int arg1, int arg2, int arg3);

    /* The fixed address is a pointer to the entry function. */
    /* entry_point is a pointer to the fixed address. */
    entry_fn_t **entry_point = (entry_fn_t **) SYSCALL_ENTRY_FN_PTR_PADDR;

    /* Call the function via the pointer-to-pointer. */
    return (*entry_point)(syscall_id, arg1, arg2, arg3);
}

/*
 * Invoke syscall via selected implementation
 */
static inline int invoke_syscall(int syscall_id, int arg1, int arg2, int arg3)
{
    switch (SYSCALL_IMPL) {
    case SYSCALL_IMPL_FIXEDPOINT:
        return invoke_syscall_fixedpoint(syscall_id, arg1, arg2, arg3);
        break;
    }
}

#define invoke_syscall(i, arg1, arg2, arg3) \
    invoke_syscall(i, (int) arg1, (int) arg2, (int) arg3)

/* Macros for calls that have fewer arguments */

#define IGNORE 0

#define invoke_syscall0(i)             invoke_syscall(i, IGNORE, IGNORE, IGNORE)
#define invoke_syscall1(i, arg1)       invoke_syscall(i, arg1, IGNORE, IGNORE)
#define invoke_syscall2(i, arg1, arg2) invoke_syscall(i, arg1, arg2, IGNORE)
#define invoke_syscall3(i, arg1, arg2, arg3) \
    invoke_syscall(i, arg1, arg2, arg3)

#define unreachable() __builtin_unreachable()

void yield(void) { invoke_syscall0(SYSCALL_YIELD); }

noreturn void exit(void)
{
    invoke_syscall0(SYSCALL_EXIT);
    unreachable(); // Exit will never return.
}

