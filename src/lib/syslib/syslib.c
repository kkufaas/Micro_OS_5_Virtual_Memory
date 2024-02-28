/*
 * Implementation of the system call library for the user processes.
 */

#include "syslib.h"

#include <syslib/addrs.h>

#include "common.h"

enum syscall_impl {
    SYSCALL_IMPL_INTERRUPT,
};

static const enum syscall_impl SYSCALL_IMPL = SYSCALL_IMPL_INTERRUPT;

/*
 * Invoke system call via interrupt
 *
 * Syscalls are implemented using an interrupt. The interrupt enters the
 * kernel, and the kernel interrupt handler will dispatch the system call.
 *
 * The interrupt vector used is controlled by the IVEC_SYSCALL constant
 * (currently interrupt number 48, which is after both the CPU-reserved vectors
 * and the mapped hardware IRQs).
 *
 * We use registers to pass the parameters:
 *
 * - EAX: syscall number
 * - EBX: arg1
 * - ECX: arg2
 * - EDX: arg3
 *
 * The kernel will pass the syscall return value (if any) in EAX.
 */
static inline int
invoke_syscall_interrupt(int syscall_id, int arg1, int arg2, int arg3)
{
    int ret;
    asm volatile("int	%[syscall_ivec]"
                 : "=a"(ret)
                 : [syscall_ivec] "n"(IVEC_SYSCALL), "a"(syscall_id),
                   "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

/*
 * Invoke syscall via selected implementation
 */
static inline int invoke_syscall(int syscall_id, int arg1, int arg2, int arg3)
{
    switch (SYSCALL_IMPL) {
    case SYSCALL_IMPL_INTERRUPT:
        return invoke_syscall_interrupt(syscall_id, arg1, arg2, arg3);
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

int getpid(void) { return invoke_syscall0(SYSCALL_GETPID); }

int getpriority(void) { return invoke_syscall0(SYSCALL_GETPRIORITY); }

void setpriority(int p) { invoke_syscall1(SYSCALL_SETPRIORITY, p); }

int cpuspeed(void) { return invoke_syscall0(SYSCALL_CPUSPEED); }

int mbox_open(int key) { return invoke_syscall1(SYSCALL_MBOX_OPEN, key); }

int mbox_close(int q) { return invoke_syscall1(SYSCALL_MBOX_CLOSE, q); }

int mbox_stat(int q, int *count, int *space)
{
    return invoke_syscall3(SYSCALL_MBOX_STAT, q, count, space);
}

int mbox_recv(int q, msg_t *m)
{
    return invoke_syscall2(SYSCALL_MBOX_RECV, q, m);
}

int mbox_send(int q, msg_t *m)
{
    return invoke_syscall2(SYSCALL_MBOX_SEND, q, m);
}

int getchar(int *c) { return invoke_syscall1(SYSCALL_GETCHAR, c); }

int readdir(unsigned char *buf)
{
    return invoke_syscall1(SYSCALL_READDIR, buf);
}

int loadproc(int location, int size)
{
    return invoke_syscall2(SYSCALL_LOADPROC, location, size);
}

