/*
 * Assertion macros specifically for the kernel
 */
#ifndef ASSERTK_H
#define ASSERTK_H

#include <stdnoreturn.h>

#include <syslib/compiler_compat.h>

ATTR_CALLED_FROM_ISR
noreturn void abortk();

ATTR_PRINTFLIKE(3, 4)
noreturn extern void
_assertk_fail(const char *file, unsigned int line, const char *fmt, ...);

#define assertf(assertion, fmt, ...) \
    do { \
        if (assertion) { \
        } else { \
            _assertk_fail(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define assertk(assertion) \
    assertf(assertion, "assertion `%s` failed\n", #assertion)

#endif /* ASSERTK_H */
