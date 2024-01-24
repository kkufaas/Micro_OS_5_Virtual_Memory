#include "syscall.h"

#include <syslib/addrs.h>
#include <syslib/common.h>

#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"

/* Includes necessary for syscall function prototypes. */

#include "scheduler.h"

/* === Syscall Entry === */

void syscall_entry(int fn); // Defined in assembly

#define stringify(x) #x

static void init_syscall_entry_fixedpoint(void)
{
    /* Load the address of the entry function into the fixed address
     *
     * In assembly this boils down to one instruction:
     *
     *      00001a86 <init_syscall_entry_fixedpoint>:
     *          1a86:       c7 05 00 0f 00 00 41    movl   $0x1a41,0xf00
     *          1a8d:       1a 00 00
     *
     *  "load the address of the entry function into memory location 0xf00."
     *
     * But to get there in C we have to do some type trickery:
     *
     * 1. Cast the address constant to a double pointer:
     *  "the constant 0xf00 is a pointer to address 0xf00 which will be
     *   a pointer to an entry function."
     *
     * 2. Dereference once to get an assignable function pointer:
     *  "address 0xf00 is a pointer to an entry function"
     *
     * 3. Finally assign the function pointer:
     *  "load the address of the entry function into memory location 0xf00."
     */
    *(typeof(syscall_entry) **) SYSCALL_ENTRY_FN_PTR_PADDR =
            syscall_entry;

    pr_info("Set syscall fixed point %#x -> %p -> %s\n",
            SYSCALL_ENTRY_FN_PTR_PADDR, syscall_entry,
            stringify(syscall_entry));
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

