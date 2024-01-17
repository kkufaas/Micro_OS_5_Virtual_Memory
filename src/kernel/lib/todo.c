#include "todo.h"

#include <stdbool.h>

#include "printk.h"

void _todo_print_once(
        volatile int *printed,
        const char   *file,
        unsigned int  line,
        const char   *func
)
{
    if (!*printed) {
        pr_warn("%s:%d: TODO: implement %s\n", file, line, func);
        *printed = true;
    }
}

