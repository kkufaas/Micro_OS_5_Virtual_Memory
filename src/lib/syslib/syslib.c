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

    /* Call the entry function via fixed address
     *
     * In assembly this is very simple. Take the yield function:
     *
     *      00007bc2 <yield>:
     *          7bc2:       6a 00                   push   $0x0
     *          7bc4:       6a 00                   push   $0x0
     *          7bc6:       6a 00                   push   $0x0
     *          7bc8:       6a 00                   push   $0x0
     *          7bca:       ff 15 00 0f 00 00       call   *0xf00
     *          7bd0:       83 c4 10                add    $0x10,%esp
     *          7bd3:       c3                      ret
     *
     *  "dereference 0xf00 to get a function address and call it."
     *
     * But to get there in C we have to do some type trickery:
     *
     * 1. Cast the address constant to a double pointer:
     *  "the constant 0xf00 is a pointer to address 0xf00 which is
     *   a pointer to the entry function."
     *
     * 2. Dereference once to get a callable function pointer:
     *  "address 0xf00 is a pointer to the entry function"
     *
     * 3. Finally call via the function pointer:
     *  "dereference 0xf00 to get a function address and call it."
     */
    return (*(entry_fn_t **) SYSCALL_ENTRY_FN_PTR_PADDR
    )(syscall_id, arg1, arg2, arg3);
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

