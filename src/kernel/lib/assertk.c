#include "assertk.h"

#include <syslib/compiler_compat.h>

#include "../hardware/cpu_x86.h"
#include "printk.h"

noreturn void abortk()
{
    kprintf(LOG_ERROR, "halting\n");
    interrupts_disable();
    while (1) cpu_halt();
}

ATTR_PRINTFLIKE(3, 4)
noreturn extern void
_assertk_fail(const char *file, unsigned int line, const char *fmt, ...)
{
    kprintf(LOG_ERROR, "error: %s:%d: ", file, line);

    va_list args;
    va_start(args, fmt);
    vkprintf(LOG_ERROR, fmt, args);
    va_end(args);

    abortk();
}

