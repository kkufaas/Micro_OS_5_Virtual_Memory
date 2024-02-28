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

/* Size of a sector on the USB stick */
#define SECTOR_SIZE 512

/* === System call numbers === */

/* Unique integers for each system call */
enum {
    SYSCALL_YIELD,
    SYSCALL_EXIT,
    SYSCALL_GETPID,
    SYSCALL_GETPRIORITY,
    SYSCALL_SETPRIORITY,
    SYSCALL_CPUSPEED,
    SYSCALL_MBOX_OPEN,
    SYSCALL_MBOX_CLOSE,
    SYSCALL_MBOX_STAT,
    SYSCALL_MBOX_RECV,
    SYSCALL_MBOX_SEND,
    SYSCALL_GETCHAR,
    SYSCALL_READDIR,
    SYSCALL_LOADPROC,
    SYSCALL_COUNT
};

/* === IPC msg type === */

/*
 * Note that this struct only allocates space for the size element.
 *
 * To use a message with a body of 50 bytes we must first allocate space for
 * the message:
 *
 *    char space[sizeof(int) + 50];
 *
 * Then we declares a msg_t * variable that points to the allocated space:
 *
 *    msg_t *m = (msg_t *) space;
 *
 * We can now access the size variable:
 *
 *    m->size (at memory location &(space[0]) )
 *
 * And the 15th character in the message:
 *
 *    m->body[14]
 *      (at memory location &(space[0]) + sizeof(int) + 14 * sizeof(char))
 */
struct ATTR_PACKED msg {
    int  size;    /* Size of message contents in bytes */
    char body[0]; /* Pointer to start of message contents */
};

typedef struct msg msg_t;

/* Return size of header */
#define MSG_T_HEADER_SIZE (sizeof(int))
/* Return size of message including header */
#define MSG_SIZE(m) (MSG_T_HEADER_SIZE + m->size)

/* === Structure of the process directory on the disk image === */

/*
 * Structure used for interpreting the process directory in the
 * "filesystem" on the USB stick.
 */
struct directory_t {
    int location; /* Sector number */
    int size;     /* Size in number of sectors */
};

#endif /* !COMMON_H */
