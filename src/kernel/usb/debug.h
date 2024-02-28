#ifndef DEBUG_H
#define DEBUG_H

#include <util/util.h>

#include "../lib/printk.h"

// #define USB_LOG_LEVEL LOG_SKIP
#define USB_LOG_LEVEL LOG_DEBUG

#define DEBUG_NAME(x)   static const char debug_name[] = x;
#define DEBUG_STATUS(x) (((x) >= 0) ? "OK" : "ERR")
#define DEBUG(fmt, ...) \
    printk(USB_LOG_LEVEL, "%s: " fmt "\n", debug_name, ##__VA_ARGS__)

#endif /* DEBUG_H */
