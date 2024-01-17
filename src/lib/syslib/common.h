#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <syslib/compiler_compat.h>

/*
 * === Definitions that match POSIX sys/types.h ===
 *
 * See:
 * <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_types.h.html>
 */

#ifndef __ssize_t_defined
typedef long int ssize_t; // Used for a count of bytes or error indication.
#define __ssize_t_defined
#endif
#ifndef __off_t_defined
typedef long int off_t; // Used for file sizes.
#define __off_t_defined
#endif

/* === Other definitions === */

/* === System call numbers === */

/* Unique integers for each system call */
enum {
    SYSCALL_YIELD,
    SYSCALL_EXIT,
    SYSCALL_COUNT
};

#endif /* !COMMON_H */
