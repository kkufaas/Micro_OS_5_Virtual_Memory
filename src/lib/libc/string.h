#ifndef CUSTOM_LIBC_STRING_H
#define CUSTOM_LIBC_STRING_H

#include <stddef.h>

/* === Core: no syscalls required === */

/* --- Copying functions --- */

void *memcpy(void *restrict dest, const void *restrict src, size_t count);
/* Not yet implemented: memccpy */
/* Not yet implemented: memmove */

void bcopy(const char *source, char *destin, int size); // BSD extension

char *strcpy(char *restrict dest, const char *restrict src);
char *strncpy(char *restrict dest, const char *restrict src, size_t count);

/* Not yet implemented: strdup */
/* Not yet implemented: strndup */

/* --- Concatenation functions --- */

char *strcat(char *restrict dest, const char *restrict src);
char *strncat(char *restrict dest, const char *restrict src, size_t count);

/* --- Comparison functions --- */

int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s, const char *t);
int strncmp(const char *s, const char *t, size_t n);

/* Not yet implemented: strcoll */
/* Not yet implemented: strxfrm */

/* --- Search functions --- */

/* Not yet implemented: memchr */
/* Not yet implemented: strchr */
/* Not yet implemented: strcspn */
/* Not yet implemented: strpbrk */
/* Not yet implemented: strrchr */
/* Not yet implemented: strspn */
/* Not yet implemented: strstr */
/* Not yet implemented: strtok */

/* --- Miscellaneous functions --- */

void *memset(void *s, int c, size_t n);
/* Not yet implemented: memset_explicit */

char *strerror(int errnum);

int strlen(const char *s);

void bzero(char *a, int size); // BSD extension

/* === Non-core: syscalls required === */
#ifndef OSLIBC_CORE_ONLY
#endif /* !OSLIBC_CORE_ONLY */

#endif /* CUSTOM_LIBC_STRING_H */
