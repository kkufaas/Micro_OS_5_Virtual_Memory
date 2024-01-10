#include "unittest.h"

#include <stdarg.h>
#include <stdio.h>

__attribute__((weak)) int tvprintf(const char *fmt, va_list args)
{
    return vfprintf(stderr, fmt, args);
}

