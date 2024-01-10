/*
 * Various utility functions that can be linked with both the kernel
 * and user code.
 */

#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#include <syslib/common.h>

void delay(uint32_t val);
uint64_t get_timer(void);

int same_string(char *s1, char *s2);

/* === From bsd/string.h === */

int strlcpy(char *dest, const char *src, int size);

#endif /* !UTIL_H */
