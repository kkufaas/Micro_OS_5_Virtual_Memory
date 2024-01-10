#ifndef CUSTOM_LIBC_STDLIB_H
#define CUSTOM_LIBC_STDLIB_H

#include <stdnoreturn.h>

/* === Core: no syscalls required === */

int atoi(const char *s);

int  rand(void);
void srand(unsigned int seed);

/* === Non-core: syscalls required === */
#ifndef OSLIBC_CORE_ONLY
#endif /* !OSLIBC_CORE_ONLY */

#endif /* CUSTOM_LIBC_STDLIB_H */
