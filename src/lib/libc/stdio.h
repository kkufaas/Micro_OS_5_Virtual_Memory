#ifndef CUSTOM_LIBC_STDIO_H
#define CUSTOM_LIBC_STDIO_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

/* === Core: no syscalls required === */

#define EOF (-1)

#define _IOFBF 0 /* full buffering */
#define _IOLBF 1 /* line buffering */
#define _IONBF 2 /* no buffering */

typedef struct _file_impl FILE;

/* A table of function pointers for core file operations.
 * This is how we will implement polymorphism for files. */
struct _file_vtable {
    int (*fflush)(FILE *stream);
};

struct _file_impl {
    unsigned char *write_base; // Start of write buffer
    unsigned char *write_ptr;  // Current write position (write here, then inc)
    unsigned char *write_end;  // End of buffer (write_base + buffer size)
    int            buffer_mode; // Buffering mode: _IOFBF, _IOLBF, or _IONBF

    /* Built-in 1-char buffer for unbuffered output
     * (will need to flush on every write) */
    unsigned char tiny_buffer[1];

    /* Number of bytes written, not counting what's in the buffer */
    size_t written_flushed_ct;

    int error; // Error number (positive)

    const struct _file_vtable *vtable;
};

int setvbuf(FILE *stream, char *buffer, int mode, size_t size);
int fflush(FILE *stream);
int ferror(FILE *stream);

#define putc(ch, stream) fputc(ch, stream)

int fputc(int ch, FILE *file);
int fputs(const char *str, FILE *file);

__attribute__((format(printf, 2, 3))) int
    fprintf(FILE *file, const char *format, ...);
int vfprintf(FILE *file, const char *format, va_list args);

__attribute__((format(printf, 3, 4))) int
    snprintf(char *buffer, size_t bufsz, const char *format, ...);
int vsnprintf(char *buffer, size_t bufsz, const char *format, va_list args);

__attribute__((format(printf, 2, 3))) int
    sprintf(char *buffer, const char *format, ...);
int vsprintf(char *buffer, const char *format, va_list args);

/* === Non-core: syscalls required === */
#ifndef OSLIBC_CORE_ONLY

#endif /* !OSLIBC_CORE_ONLY */
#endif /* CUSTOM_LIBC_STDIO_H */
