/*
 * A printf variant that writes to a terminal buffer
 */
#ifndef TPRINTF_H
#define TPRINTF_H

#include <stdarg.h>

#include <syslib/compiler_compat.h>

#include "termbuf.h"

ATTR_PRINTFLIKE(2, 3) int tprintf(struct term *t, const char *fmt, ...);
int vtprintf(struct term *t, const char *fmt, va_list args);

#endif /* TPRINTF_H */
