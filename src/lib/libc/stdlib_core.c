#include "stdlib.h"

#include <stdbool.h>
#include "ctype.h"

/* --- Numeric conversion functions --- */

int atoi(const char *s)
{
    if (!s) return 0;
    while (*s && isspace(*s)) s++; // skip whitespace

    bool isnegative = *s == '-';     // check for negative sign
    if (*s == '+' || *s == '-') s++; // consume negative or positive sign

    int n = 0;
    for (; *s && isdigit(*s); s++) {
        n *= 10;
        n += *s - '0';
    }

    if (isnegative) n = -n;

    return n;
}

/* --- Pseudo-random sequence generation functions --- */

static unsigned int rand_val;

/* Return a pseudo random number */
int rand(void)
{
    rand_val = rand_val * 1103515245 + 12345;
    return (rand_val / 65536) % 32768;
}

void srand(unsigned int seed) { rand_val = seed; }

