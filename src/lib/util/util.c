/*
 * Various utility functions that can be linked with both the kernel
 * and user code.
 *
 * K&R = "The C Programming Language", Kernighan & Ritchie
 */

#include "util.h"

#include <string.h>

#include <syslib/addrs.h>

/* Simple delay loop to slow things down */
void delay(uint32_t ticks)
{
    uint64_t wait_until = read_cpu_ticks() + ticks;

    while (read_cpu_ticks() < wait_until)
        ;
}

/* Read the x86 time stamp counter */
uint64_t read_cpu_ticks(void)
{
    uint64_t tsc;

    /* RDTSC reads a 64-bit timestamp counter into EDX:EAX (high bits in EDX,
     * low bits in EAX).
     *
     * The "A" operand constraint is specifically for this register
     * configuration on x86-32.
     *
     * Note that if/when we move to x86-64, this will need to be changed.
     * See <https://gcc.gnu.org/onlinedocs/gcc/Machine-Constraints.html> */
    asm volatile("rdtsc" : "=A"(tsc));

    return tsc;
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

