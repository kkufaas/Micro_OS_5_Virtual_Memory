#include "syscall.h"

#include <syslib/addrs.h>
#include <syslib/common.h>

#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"

#include "sync.h"

/* Includes necessary for syscall function prototypes. */

#include "pcb.h"
#include "scheduler.h"
#include "time.h"

/* === Syscall Jump Table === */

/* Defining a function pointer is easier when we have a type. */
/* Syscalls return an int. Don't specify arguments! */
typedef int (*syscall_t)();

/* Global table with pointers to the kernel functions implementing the
 * system calls. System call number 0 has index 0 here. */
static syscall_t syscall_table[SYSCALL_COUNT];

static void add_to_table(int i, syscall_t call)
{
    assertk((i >= 0) && (i < SYSCALL_COUNT));
    assertk(call != NULL);
    syscall_table[i] = call;
}

static void init_syscall_jumptable(void)
{
/* Disables intentional warning about e.g.
 * setpriority() being different function-type than (func_t) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"

    add_to_table(SYSCALL_YIELD, (syscall_t) yield);
    add_to_table(SYSCALL_EXIT, (syscall_t) exit);
    add_to_table(SYSCALL_GETPID, (syscall_t) getpid);
    add_to_table(SYSCALL_GETPRIORITY, (syscall_t) getpriority);
    add_to_table(SYSCALL_SETPRIORITY, (syscall_t) setpriority);
    add_to_table(SYSCALL_CPUSPEED, (syscall_t) cpuspeed);

#pragma GCC diagnostic pop

    pr_info("Inititialized syscall jump table.\n");
}

/* === Syscall Dispatch === */

/*
 * Dispatch syscall via jump table
 *
 * This function handles executing a given syscall, and returns the result
 * to the syscall entry function, from where it is returned to the calling
 * process. Before we get here, the syscall entry function will have stored
 * the context of the process making the syscall, and entered a critical
 * section (through nointerrupt_enter()).
 *
 * Note:
 *
 * The use of nointerrupt_leave() causes the interrupts to be turned on
 * again after nointerrupt_leave. (nointerrupt_count goes from 1 to 0 again.)
 *
 * This makes sense if we want system calls or other interrupt
 * handlers to be interruptable (for instance allowing a timer interrupt
 * to preempt a process while it's inside the kernel in a system call).
 *
 * It does, however, also mean that we can get interrupts while we are
 * inside another interrupt handler (the same thing is done in
 * the other interrupt handlers).
 *
 * In syslib.c we put systemcall number in eax, arg1 in ebx, arg2 in ecx
 * and arg3 in edx. The return value is returned in eax.
 *
 * Before entering the processor has switched to the kernel stack
 * (PMSA p. 209, Privilege level switch whitout error code)
 */
int syscall_dispatch(int fn, int arg1, int arg2, int arg3)
{
    int ret_val = 0;

    assertf(current_running->nested_count == 0,
            "A process/thread that was running inside the kernel made a "
            "syscall.");
    current_running->nested_count++;
    nointerrupt_leave();

    /* Call function and return result as usual (ie, "return ret_val"); */
    if (fn >= SYSCALL_COUNT || fn < 0) {
        /* Illegal system call number, call exit instead */
        fn = SYSCALL_EXIT;
    }
    /*
     * In C's calling convention, caller is responsible for cleaning up the
     * stack. Therefore we don't really need to distinguish between different
     * argument numbers. Just pass all 3 arguments and it will work
     */
    ret_val = syscall_table[fn](arg1, arg2, arg3);

    nointerrupt_enter();
    current_running->nested_count--;
    assertf(current_running->nested_count == 0, "bad nest count: %d",
            current_running->nested_count);
    return ret_val;
}

/* === Syscall Initialization === */

void init_syscalls(void)
{
    init_syscall_jumptable();
}

