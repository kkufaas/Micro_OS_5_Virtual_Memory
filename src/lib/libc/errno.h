#ifndef CUSTOM_LIBC_ERRNO_H
#define CUSTOM_LIBC_ERRNO_H

/*
 * Errors required by the C standard
 *
 * <https://en.cppreference.com/w/c/error/errno_macros>
 */

#define EDOM   1 /* Mathematics argument out of domain of function */
#define EILSEQ 2 /* Illegal byte sequence */
#define ERANGE 3 /* Result too large */

/*
 * Errors required by POSIX
 *
 * <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html>
 */

#define EINVAL 4 /* Invalid argument */
#define EIO    5 /* I/O error */
#define EBADF  6 /* Bad file descriptor */

/* Not implemented: additional POSIX error numbers */

/*
 * Errors specific to this libc implementation
 */

#define EUNIMPLEMENTED 2201 /* Operation not implemented in our libc */

#endif /* CUSTOM_LIBC_ERRNO_H */
