/*
 * Kernel logging facilities
 *
 * Inspired by the Linux kernel's printk system.
 */
#ifndef PRINTK_H
#define PRINTK_H

#include <limits.h>
#include <stdarg.h>

#include <ansi_term/termbuf.h>
#include <syslib/compiler_compat.h>

enum log_level {
    LOG_ERROR = 0,
    LOG_DUMP,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_SKIP, // Print with this log level to never print
};

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_DEBUG
#endif

extern const char *level_strs[];

ATTR_PRINTFLIKE(2, 3) int kprintf(enum log_level lvl, const char *fmt, ...);
int vkprintf(enum log_level lvl, const char *fmt, va_list args);

/*
 * Use this macro to prepend a subsystem name for a file
 *
 * Example:
 *      #define pr_fmt(fmt) "sync: " fmt
 */
#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define printk(lvl, fmt, ...) \
    do { \
        if (lvl <= LOG_LEVEL) \
            kprintf(lvl, "%-5s: " pr_fmt(fmt), level_strs[lvl], \
                    ##__VA_ARGS__); \
    } while (0)

#define pr_dump(fmt, ...)  printk(LOG_DUMP, fmt, ##__VA_ARGS__)
#define pr_error(fmt, ...) printk(LOG_ERROR, fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)  printk(LOG_WARN, fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...)  printk(LOG_INFO, fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...) printk(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define pr_never(fmt, ...) printk(INT_MAX, fmt, ##__VA_ARGS__)

#endif /* PRINTK_H */
