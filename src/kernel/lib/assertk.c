#include "assertk.h"

#include <string.h>

#include "../hardware/cpu_x86.h"
#include "printk.h"

noreturn void abortk()
{
    interrupts_disable();
    while (1) cpu_halt();
}

__attribute__((format(printf, 3, 4))) noreturn extern void
_assertk_fail(const char *file, unsigned int line, const char *fmt, ...)
{
    /* Remove obvious part of kernel file path. */
    static const char   file_prefix[] = "../kernel/";
    static const size_t prefix_len    = sizeof(file_prefix) - 1;
    if (0 == strncmp(file, file_prefix, prefix_len)) file += prefix_len;

    kprintf(LOG_ERROR, "error: %s:%d: ", file, line);

    va_list args;
    va_start(args, fmt);
    vkprintf(LOG_ERROR, fmt, args);
    va_end(args);

    abortk();
}

