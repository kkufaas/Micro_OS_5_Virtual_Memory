#include "tprintf.h"

#include <stdarg.h>

#include <syslib/compiler_compat.h>

#include "termbuf.h"

#define TPRINTF_BUFSZ 256

int vtprintf(struct term *t, const char *fmt, va_list args)
{
    char buf[TPRINTF_BUFSZ];
    int  ret = vsnprintf(buf, TPRINTF_BUFSZ, fmt, args);
    if (ret < 0) return ret;

    term_puts(t, buf);
    return ret;
}

ATTR_PRINTFLIKE(2, 3) int tprintf(struct term *t, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vtprintf(t, fmt, args);
    va_end(args);
    return ret;
}

