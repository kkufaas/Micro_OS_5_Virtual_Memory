#include "stdlib.h"

#include <stddef.h>

#include <unittest/unittest.h>

int test_atoi()
{
    tassert_eq(12345, atoi("12345"));
    tassert_eq(-12345, atoi("-12345"));
    tassert_eq(12345, atoi("   12345junk"));
    tassert_eq(-12345, atoi("   -12345junk"));
    tassert_eq(0, atoi(NULL));
    tassert_eq(0, atoi(""));
    tassert_eq(0, atoi("-"));
    tassert_eq(0, atoi("+"));
    tassert_eq(0, atoi("not a number"));

    /* Cases from cpprefence examples
     * <https://en.cppreference.com/w/c/string/byte/atoi> */
    tassert_eq(-123, atoi(" -123junk"));
    tassert_eq(321, atoi(" +321dust"));
    tassert_eq(0, atoi("0"));
    tassert_eq(42, atoi("0042")); // decimal number with leading zeros
    tassert_eq(0, atoi("0x2A")); // leading zero is converted, discarding "x2A"
    tassert_eq(0, atoi("junk")); // no conversion can be performed
    return TEST_PASS;
}

