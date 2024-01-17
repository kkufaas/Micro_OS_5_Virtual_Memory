/*
 * Macros to mark code that is to-be-implemented
 */
#ifndef TODO_H
#define TODO_H

#define todo_use(x) ((void) x)

void _todo_print_once(
        volatile int *printed,
        const char   *file,
        unsigned int  line,
        const char   *func
);

/*
 * Print a todo message and then continue
 */
#define todo_noop() \
    do { \
        static volatile int msg_printed = 0; \
        _todo_print_once(&msg_printed, __FILE__, __LINE__, __func__); \
    } while (0)

/*
 * Print a todo message and then halt the kernel
 */
#define todo_abort(fmt, ...) \
    do { \
        todo_noop(); \
        abortk(); \
    } while (0)

#endif /* TODO_H */
