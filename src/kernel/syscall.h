#ifndef SYSCALL_H
#define SYSCALL_H
#include <syslib/common.h>

void init_syscalls(void);

/* === Syscall Entry === */

void syscall_entry_interrupt(void); // Defined in assembly

#endif /* SYSCALL_H */
