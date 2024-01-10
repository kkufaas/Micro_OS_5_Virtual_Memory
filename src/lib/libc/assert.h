#ifndef CUSTOM_LIBC_ASSERT_H
#define CUSTOM_LIBC_ASSERT_H

/* === Core: no syscalls required === */

#ifdef OSLIBC_CORE_ONLY
/* --- Hack: assume we are in the kernel and delegate to assertk --- */

#include <kernel/lib/assertk.h>

#ifdef NDEBUG
#define assert(expr)
#else
#define assert assertk
#endif /* !NDEBUG */

#endif /* OSLIBC_CORE_ONLY --- Hack */

/* === Non-core: syscalls required === */
#ifndef OSLIBC_CORE_ONLY
#endif /* !OSLIBC_CORE_ONLY */
#endif /* CUSTOM_LIBC_ASSERT_H */
