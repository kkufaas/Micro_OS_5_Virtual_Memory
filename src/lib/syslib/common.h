#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

#endif /* !COMMON_H */
