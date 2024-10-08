#include "printk.h"

#include <stdio.h>

#include <ansi_term/termbuf.h>
#include <syslib/compiler_compat.h>
#include <syslib/screenpos.h>

#include "../hardware/serial.h"

#define PRINTK_BUF_SZ 256

const char *level_strs[] = {
        [LOG_ERROR] = "error", [LOG_DUMP] = "dump",   [LOG_WARN] = "warn",
        [LOG_INFO] = "info",   [LOG_DEBUG] = "debug", [LOG_LOG] = "log"
};

static struct term printkterm = PRINTK_TERM_INIT;


// Modified to avoid printing everything to the screen but still
// keep logs. Suggestion from Odin Bjerke.
int vkprintf(enum log_level lvl, const char *fmt, va_list args)
{
    /* On dump, set the screen cursor to grow mode. */
    if (lvl == LOG_DUMP) printkterm.overflow = TERM_OF_GROW;

    char buf[PRINTK_BUF_SZ];
    int  ret = vsnprintf(buf, PRINTK_BUF_SZ, fmt, args);
    if (ret < 0) return ret;

    // provide option to only write to file without using the screen
    if (lvl != LOG_LOG)
        term_puts(&printkterm, buf);
    serial_puts(PORT_COM1, buf);
    return ret;
}


ATTR_PRINTFLIKE(2, 3) int kprintf(enum log_level lvl, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vkprintf(lvl, fmt, args);
    va_end(args);
    return ret;
}

