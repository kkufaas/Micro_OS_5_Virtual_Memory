#include "string.h"

#include <stdint.h>
#include "errno.h"

/* === Core: no syscalls required === */

/* --- Copying functions --- */

void *memcpy(void *restrict dest, const void *restrict src, size_t count)
{
    for (unsigned char *cur = dest; count > 0; count--, cur++, src++)
        *cur = *(unsigned char *) src;

    return dest;
}

/* Not yet implemented: memccpy */
/* Not yet implemented: memmove */

/*
 * copy byte sequence (safe if arrays overlap)
 *
 * This function is a BSD extension that is now deprecated in favor of memcpy
 * and memmove. <https://linux.die.net/man/3/bcopy>
 */
void bcopy(const char *source, char *destin, int size)
{
    int i;

    if (size == 0) return;

    if (source < destin) {
        for (i = size - 1; i >= 0; i--) destin[i] = source[i];
    } else {
        for (i = 0; i < size; i++) destin[i] = source[i];
    }
}

char *strcpy(char *restrict dest, const char *restrict src)
{
    char *destorig = dest;
    for (;; dest++, src++) {
        char copiedchar = *dest = *src;
        if (!copiedchar) break;
    }
    return destorig;
}

char *strncpy(char *restrict dest, const char *restrict src, size_t count)
{
    char *destorig = dest;
    for (; count; count--, dest++, src++) {
        char copiedchar = *dest = *src;
        if (!copiedchar) break;
    }

    /* Note that we do not add a terminator if src is too long.
     *
     * C Standard: "If there is no null character in the first n characters of
     * the array pointed to by [src], the result will not be null-terminated."
     */

    /* Write nulls until the count is finished.
     *
     * C Standard: "If the array pointed to by [src] is a string that is
     * shorter than n characters, null characters are appended to the copy in
     * the array pointed to by [dest], until n characters in all have been
     * written."
     */
    for (; count; count--, dest++) {
        *dest = '\0';
    }
    return destorig;
}

/* Not yet implemented: strdup */
/* Not yet implemented: strndup */

/* --- Concatenation functions --- */

char *strcat(char *restrict dest, const char *restrict src)
{
    return strncat(dest, src, SIZE_MAX);
}

char *strncat(char *restrict dest, const char *restrict src, size_t count)
{
    char *destorig = dest;
    while (*dest) dest++; // Find end of dest string.
    for (; count; count--, dest++, src++) {
        char copiedchar = *dest = *src;
        if (!copiedchar) break;
    }

    /* C Standard: "A terminating null character is always appended." */
    if (*dest != 0) *++dest = '\0';

    return destorig;
}

/* --- Comparison functions --- */

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *b1 = (const unsigned char *) s1;
    const unsigned char *b2 = (const unsigned char *) s2;

    for (; n > 0; n--, b1++, b2++) {
        if (*b1 < *b2) return -1;
        if (*b1 > *b2) return 1;
    }
    return 0;
}

int strcmp(const char *s, const char *t) { return strncmp(s, t, SIZE_MAX); }

/*
 * strncmp:
 * Compares at most n bytes from the strings s and t.
 * Returns >0 if s > t, <0 if s < t and 0 if s == t.
 */
int strncmp(const char *s, const char *t, size_t n)
{
    for (; n > 0; n--, s++, t++) {
        if (*s && *t && *s == *t) continue;
        if (*s < *t) return -1;
        if (*s == *t) return 0;
        if (*s > *t) return 1;
    }
    return 0;
}

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

void *memset(void *s, int c, size_t n)
{
    for (unsigned char *cur = s; n > 0; n--, cur++) *cur = (unsigned char) c;
    return s;
}

/* Not yet implemented: memset_explicit */

char *strerror(int errnum)
{
    switch (errnum) {
#define errcase(ERRCONST) \
    case ERRCONST: return #ERRCONST

        errcase(EDOM);
        errcase(EILSEQ);
        errcase(ERANGE);

        errcase(EINVAL);

        errcase(EUNIMPLEMENTED);

#undef errcase

    default: return "[unknown err]";
    }
}

/* Get the length of a null-terminated string */
int strlen(const char *s)
{
    if (!s) return 0;
    int n;
    for (n = 0; *s != '\0'; s++) n++;
    return n;
}

/*
 * Zero out size bytes starting at area
 *
 * This function is a BSD extension.
 * <https://man.freebsd.org/cgi/man.cgi?query=bzero&sektion=3&manpath=FreeBSD+13.2-RELEASE+and+Ports>
 */
void bzero(char *area, int size)
{
    int i;

    for (i = 0; i < size; i++) area[i] = 0;
}

