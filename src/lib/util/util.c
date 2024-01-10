/*
 * Various utility functions that can be linked with both the kernel
 * and user code.
 *
 * K&R = "The C Programming Language", Kernighan & Ritchie
 */

#include "util.h"

#include <string.h>

#include <syslib/addrs.h>

static short *screen = (short *) VGA_TEXT_PADDR;

/* Simple delay loop to slow things down */
void delay(uint32_t ticks)
{
    uint64_t wait_until = get_timer() + ticks;

    while (get_timer() < wait_until)
        ;
}

/* Read the pentium time stamp counter */
uint64_t get_timer(void)
{
    uint64_t x = 0LL;

    /* Load the time stamp counter into edx:eax (x) */
    asm volatile("rdtsc" : "=A"(x));

    return x;
}

/*
 * Print to screen
 */

/* return true if string s1 and string s2 are equal */
int same_string(char *s1, char *s2)
{
    while ((*s1 != 0) && (*s2 != 0)) {
        if (*s1 != *s2) return false;
        s1++;
        s2++;
    }
    return (*s1 == *s2);
}

/* === From bsd/string.h === */

/*
 * strlcpy:
 * Copies at most size-1 characters from src to dest, and '\0'
 * terminates the result. One must include a byte for the '\0' in
 * dest. Returns the length of src (so that truncation can be
 * detected).
 */
int strlcpy(char *dest, const char *src, int size)
{
    int n, m;

    n = m = 0;
    while (--size > 0 && (*dest++ = *src++)) ++n;
    *dest = '\0';
    while (*src != '\0') ++src, ++m;
    return n + m;
}

