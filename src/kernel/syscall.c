#include "syscall.h"

#include <syslib/addrs.h>
#include <syslib/common.h>

#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"

/* Includes necessary for syscall function prototypes. */

#include "scheduler.h"

/* === Syscall Entry === */

void syscall_entry(int fn); // Defined in assembly in entry.S

static void init_syscall_entry_fixedpoint(void)
{
    /* Declare the entry function type. */
    typedef typeof(syscall_entry) entry_fn_t;

    /* The fixed address is a pointer to the entry function. */
    /* entry_point is a pointer to the fixed address. */
    entry_fn_t **entry_point = (entry_fn_t **) SYSCALL_ENTRY_FN_PTR_PADDR;

    /* Load the address of syscall_entry into the fixed address. */
    *entry_point = syscall_entry;

    pr_info("Inititialized syscall entry fixed point.\n");
}

/* === Syscall Dispatch === */

/*
 * Helper function for syscall_entry, in entry.S. Does the actual work
 * of executing the specified syscall.
 */
void syscall_dispatch(int fn)
{
    /* TODO: Dispatch system call */
    todo_use(fn);
    todo_noop();
}

/* === Syscall Initialization === */

void init_syscalls(void)
{
    init_syscall_entry_fixedpoint();
}

