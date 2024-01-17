#include "unittest.h"

#include <stdarg.h>
#include <stdio.h>

#include <syslib/compiler_compat.h>

ATTR_WEAK int tvprintf(const char *fmt, va_list args)
{
    return vfprintf(stderr, fmt, args);
}

