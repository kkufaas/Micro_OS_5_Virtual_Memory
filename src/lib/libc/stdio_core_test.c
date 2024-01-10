#include "stdio.h"

#include <stdint.h>
#include "ctype.h"
#include "errno.h"
#include "string.h"

#include <unittest/unittest.h>

#include "math.h"

#define TEST_BUFSZ 256

#define sprintf_case(desc, expected, fmt, ...) \
    { \
        char buf[TEST_BUFSZ]; \
        int  count          = snprintf(buf, TEST_BUFSZ, fmt, ##__VA_ARGS__); \
        int  expected_count = strlen(expected); \
        tassertf( \
                strcmp(expected, buf) == 0 && expected_count == count, \
                "snprintf case failed:\n" \
                "case:      %s (bufsz: %3d)\n" \
                "input:     %s, " #__VA_ARGS__ \
                "\n" \
                "expected:  %s (len: %3d)\n" \
                "got:       %s (len: %3d, returned count %3d)\n", \
                desc, TEST_BUFSZ, fmt, expected, expected_count, buf, \
                strlen(buf), count \
        ); \
    }

#define sprintf_err_case(desc, errno, fmt, ...) \
    { \
        char buf[TEST_BUFSZ]; \
        int  count          = snprintf(buf, TEST_BUFSZ, fmt, ##__VA_ARGS__); \
        int  expected_count = errno; \
        tassertf( \
                expected_count == count, \
                "snprintf case failed:\n" \
                "case:      %s (bufsz: %3d)\n" \
                "input:     %s, " #__VA_ARGS__ \
                "\n" \
                "expected:  returned count %3d (-%s)\n" \
                "got:       returned count %3d (-%s)\n", \
                desc, TEST_BUFSZ, fmt, expected_count, \
                strerror(-expected_count), count, strerror(-count) \
        ); \
    }

int test_printf_strings()
{
    sprintf_case("no args", "hello", "hello");
    sprintf_case("simple string", "hello world!", "hello %s!", "world");
    /* clang-format off */
    sprintf_case(
            "string with width",
            "hello      world!",
            "hello %10s!", "world"
    );
    sprintf_case(
            "string with precision",
            "hello wor!",
            "hello %.3s!", "world"
    );
    sprintf_case(
            "string with both width and precision",
            "hello        wor!",
            "hello %10.3s!", "world"
    );
    sprintf_case(
            "string with both width and precision, left-aligned",
            "hello wor       !",
            "hello %-10.3s!", "world"
    );
    sprintf_case(
            "string with star width",
            "hello      world!",
            "hello %*s!", 10, "world"
    );
    sprintf_case(
            "string with star precision",
            "hello wo!",
            "hello %.*s!", 2, "world"
    );
    sprintf_case(
            "string with star for both width and precision",
            "hello    wo!",
            "hello %*.*s!", 5, 2, "world"
    );

    /* "A negative field width argument is taken as a '-' flag followed by
     * a positive field width." */
    sprintf_case(
            "negative star width should be taken as left flag",
            "hello wo   !",
            "hello %*.*s!", -5, 2, "world"
    );

    /* "A negative precision argument is taken as if the precision were
     * omitted." */
    sprintf_case(
            "negative star precision should be taken as if omitted",
            "hello world!",
            "hello %*.*s!", 5, -2, "world"
    );

    /* "if only the period is specified, the precision is taken as zero." */
    sprintf_case(
            "precision of only period should count as zero",
            "hello      !",
            "hello %5.s!", "world"
    );

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_signed_ints()
{
    /* clang-format off */

    sprintf_case(
            "simple int with %d",
            "my number is 12345!",
            "my number is %d!", 12345
    );

    sprintf_case(
            "simple int with %i",
            "my number is 54321!",
            "my number is %i!", 54321
    );

    sprintf_case(
            "negative value",
            "my number is -12345!",
            "my number is %d!", -12345
    );

    sprintf_case(
            "plus flag",
            "my number is +12345!",
            "my number is %+d!", 12345
    );

    sprintf_case(
            "space flag",
            "my number is  12345!",
            "my number is % d!", 12345
    );

    sprintf_case(
            "width",
            "my number is      12345!",
            "my number is %10d!", 12345
    );

    sprintf_case(
            "precision",
            "my number is 0012345!",
            "my number is %.7d!", 12345
    );

    sprintf_case(
            "width and precision",
            "my number is    0012345!",
            "my number is %10.7d!", 12345
    );

    sprintf_case(
            "width with zero flag",
            "my number is 0000012345!",
            "my number is %010d!", 12345
    );

    sprintf_case(
            "width, precision, and sign, without zero flag",
            "my number is   +0012345!",
            "my number is %+10.7d!", 12345
    );

    sprintf_case(
            "value of 0 with 0 precision: no chars",
            "my number is !",
            "my number is %.0d!", 0
    );

    /* Tests with unusual formats that the compiler would warn about */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"

    sprintf_case(
            "space and plus flags (plus should take precedence)",
            "my number is +12345!",
            "my number is % +d!", 12345
    );

    sprintf_case(
            "width and precision with zero flag",
            "my number is 0000012345!",
            "my number is %010.7d!", 12345
    );

    sprintf_case(
            "width, precision, and sign, with zero flag",
            "my number is +000012345!",
            "my number is %+010.7d!", 12345
    );

#pragma GCC diagnostic pop

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_unsigned_ints()
{
    /* clang-format off */

    sprintf_case(
            "simple unsigned int with %u",
            "my number is 12345!",
            "my number is %u!", 12345
    );

    sprintf_case(
            "octal",
            "my number is 644 0644!",
            "my number is %o %#o!", 0644, 0644
    );

    sprintf_case(
            "hex",
            "my number is af AF 0xaf 0XAF!",
            "my number is %x %X %#x %#X!", 0xaf, 0xaf, 0xaf, 0xaf
    );

    sprintf_case(
            "unsigned with precision",
            "my number is 00420 00644 0x001a4!",
            "my number is %.5u %#.5o %#.5x!", 0644, 0644, 0644
    );

    sprintf_case(
            "unsigned with width and precision",
            "my number is    00420    00644  0x001a4!",
            "my number is %8.5u %#8.5o %#8.5x!", 0644, 0644, 0644
    );

    sprintf_case(
            "value of 0 with 0 precision: no chars",
            "my number is //!",
            "my number is %.0u/%.0o/%.0x!", 0, 0, 0
    );

    /* Tests with unusual formats that the compiler would warn about */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

    /* Binary specifiers are coming in C23. We implement them, and new glibc
     * implements them, but GCC still warns if you use them.. */
    sprintf_case(
            "binary",
            "my number is 10101111 10101111 0b10101111 0B10101111!",
            "my number is %b %B %#b %#B!", 0xaf, 0xaf, 0xaf, 0xaf
    );

#pragma GCC diagnostic pop

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_length_mod_signed()
{
    /* clang-format off */

    sprintf_case(
            "signed char",
            "my number is 99, followed by 0x123!",
            "my number is %hhd, followed by %#x!", (signed char) 'c', 0x0123
    );

    sprintf_case(
            "signed short",
            "my number is -1, followed by 0x123!",
            "my number is %hd, followed by %#x!", (signed short) -1, 0x0123
    );

    sprintf_case(
            "signed long",
            "my number is -1, followed by 0x123!",
            "my number is %ld, followed by %#x!", -1L, 0x0123
    );

    sprintf_case(
            "signed long long",
            "my number is -1, followed by 0x123!",
            "my number is %lld, followed by %#x!", -1LL, 0x0123
    );

    sprintf_case(
            "signed intmax_t",
            "my number is -1, followed by 0x123!",
            "my number is %jd, followed by %#x!", (intmax_t) -1, 0x0123
    );

    sprintf_case(
            "signed size_t",
            "my number is -1, followed by 0x123!",
            "my number is %zd, followed by %#x!", (size_t) -1, 0x0123
    );

    sprintf_case(
            "signed ptrdiff_t",
            "my number is -1, followed by 0x123!",
            "my number is %td, followed by %#x!", (ptrdiff_t) -1, 0x0123
    );

    /* Tests with unusual formats that the compiler would warn about */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

    /* Fixed width legnth modifiers are coming in C23. */
    sprintf_case(
            "signed int64_t",
            "my number is 9223372036854775807, followed by 0x123!",
            "my number is %w64d, followed by %#x!",
            (int64_t) INT64_MAX, 0x0123
    );

    sprintf_case(
            "signed int_fast64_t",
            "my number is 9223372036854775807, followed by 0x123!",
            "my number is %wf64d, followed by %#x!",
            (int_fast64_t) INT64_MAX, 0x0123
    );

#pragma GCC diagnostic pop

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_length_mod_unsigned()
{
    /* clang-format off */

    sprintf_case(
            "unsigned char",
            "my number is 0xff, followed by 0x123!",
            "my number is %#hhx, followed by %#x!",
            (unsigned char) 0xff, 0x0123
    );

    sprintf_case(
            "unsigned short",
            "my number is 0xffff, followed by 0x123!",
            "my number is %#hx, followed by %#x!",
            (unsigned short) 0xffff, 0x0123
    );

    sprintf_case(
            "unsigned long",
            "my number is 0xffffffff, followed by 0x123!",
            "my number is %#lx, followed by %#x!", 0xffffffffUL, 0x0123
    );

    sprintf_case(
            "unsigned long long",
            "my number is 0xffffffffffffffff, followed by 0x123!",
            "my number is %#llx, followed by %#x!",
            0xffffffffffffffffULL, 0x0123
    );

    sprintf_case(
            "unsigned intmax_t",
            "my number is 0xffffffffffffffff, followed by 0x123!",
            "my number is %#jx, followed by %#x!",
            (uintmax_t) 0xffffffffffffffff, 0x0123
    );

    sprintf_case(
            "unsigned size_t",
            "my number is 0xffffffff, followed by 0x123!",
            "my number is %#zx, followed by %#x!", (size_t) 0xffffffff, 0x0123
    );

    sprintf_case(
            "unsigned ptrdiff_t",
            "my number is 0xffffffff, followed by 0x123!",
            "my number is %#tx, followed by %#x!", (ptrdiff_t) 0xffffffff, 0x0123
    );

    /* Tests with unusual formats that the compiler would warn about */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

    /* Fixed width legnth modifiers are coming in C23. */
    sprintf_case(
            "unsigned int64_t",
            "my number is 0xffffffffffffffff, followed by 0x123!",
            "my number is %#w64x, followed by %#x!",
            (uint64_t) 0xffffffffffffffff, 0x0123
    );

    sprintf_case(
            "unsigned int_fast64_t",
            "my number is 0xffffffffffffffff, followed by 0x123!",
            "my number is %#wf64x, followed by %#x!",
            (uint_fast64_t) 0xffffffffffffffff, 0x0123
    );

#pragma GCC diagnostic pop

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_pointer()
{
    switch (sizeof(void *)) {
    case 4:
        /* clang-format off */
        sprintf_case(
                "4-byte pointer",
                "my pointer is 0x0000ffff!",
                "my pointer is %p!", (void *) 0xffff
        );
        /* clang-format on */
        break;
    case 8:
        /* clang-format off */
        sprintf_case(
                "8-byte pointer",
                "my pointer is 0x00000000ffffffff!",
                "my pointer is %p!", (void *) 0x00000000ffffffff
        );
        /* clang-format on */
        break;
    default: tfail("no test case for %zu-byte pointers\n", sizeof(void *));
    }

    return TEST_PASS;
}

int test_printf_char()
{
    /* clang-format off */

    sprintf_case(
            "simple character",
            "my char is f!",
            "my char is %c!", 'f'
    );

    sprintf_case(
            "char with width",
            "my char is    f!",
            "my char is %4c!", 'f'
    );

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_escape()
{
    /* clang-format off */

    sprintf_case(
            "character escape",
            "escaped %, followed by 0x123",
            "escaped %%, followed by %#x", 0x123
    );

    /* Tests with unusual formats that the compiler would warn about */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

    sprintf_err_case(
            "character escape",
            -EINVAL,
            "malformed %3%, followed by %#x", 0x123
    );

#pragma GCC diagnostic pop

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_float()
{
    /* clang-format off */

    sprintf_case(
            "float, with precision",
            "my number is 12345.543!",
            "my number is %.3f!", 12345.54321
    );

    sprintf_case(
            "float, default to 6 digit precision",
            "my number is 123456789.987654!",
            "my number is %f!", 123456789.987654321
    );

    sprintf_case(
            "float, round number",
            "my number is 1.000!",
            "my number is %.3f!", 1.0
    );

    sprintf_case(
            "float, zero precision, no point",
            "my number is 20!",
            "my number is %.0f!", 20.0
    );

    sprintf_case(
            "float, zero precision, alt always gives point",
            "my number is 20.!",
            "my number is %#.0f!", 20.0
    );

    sprintf_case(
            "float, with width",
            "my number is   345.54!",
            "my number is %8.2f!", 345.54321
    );

    sprintf_case(
            "float, with width and zero padding",
            "my number is 0234.432!",
            "my number is %08.3f!", 234.4321
    );

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_exponent()
{
    /* clang-format off */

    sprintf_case(
            "exponent, large number",
            "my number is 1.234554e+04!",
            "my number is %e!", 12345.54321
    );

    sprintf_case(
            "exponent, small number",
            "my number is 1.234554e-04!",
            "my number is %e!", 0.0001234554321
    );

    sprintf_case(
            "exponent, normalized number",
            "my number is 1.234554e+00!",
            "my number is %e!", 1.234554321
    );

    sprintf_case(
            "exponent, capital E",
            "my number is 1.234554E+00!",
            "my number is %E!", 1.234554321
    );

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_exponent_hex()
{
    /* clang-format off */

    sprintf_case(
            "exponent hex, large number",
            "my number is 0xa.f09cd1p+03!",
            "my number is %a!", 0xAF09.CD12p00
    );

    sprintf_case(
            "exponent hex, small number",
            "my number is 0xa.f09cd1p-03!",
            "my number is %a!", 0x0.00AF09CD12p00
    );

    sprintf_case(
            "exponent hex, normalized number",
            "my number is 0xa.f09cd1p+00!",
            "my number is %a!", 0xA.F09CD12p00
    );

    sprintf_case(
            "exponent hex, capital A",
            "my number is 0Xa.f09cd1P+00!",
            "my number is %A!", 0xA.F09CD12p00
    );

    /* clang-format on */
    return TEST_PASS;
}

int test_printf_float_edgecases()
{
    /* clang-format off */

    sprintf_case(
            "floating point: nan with %f",
            "my number is   nan!",
            "my number is %5f!", NAN
    );

    sprintf_case(
            "floating point: nan with %F",
            "my number is   NAN!",
            "my number is %5F!", NAN
    );

    sprintf_case(
            "floating point: inf with %f",
            "my number is   inf!",
            "my number is %5f!", INFINITY
    );

    sprintf_case(
            "floating point: inf with %F",
            "my number is   INF!",
            "my number is %5F!", INFINITY
    );

    sprintf_case(
            "floating point: negative zero",
            "my number is -0.0!",
            "my number is %.1f!", -0.0
    );

    sprintf_case(
            "floating point: positive zero",
            "my number is 0.0!",
            "my number is %.1f!", +0.0
    );

    /* A naive exponent implementation might loop forever. */
    sprintf_case(
            "floating point: exponent with zero",
            "my number is 0.000000e+00!",
            "my number is %e!", 0.0
    );

    sprintf_case(
            "floating point: round up",
            "my number is down 1.23, up 2.35!",
            "my number is down %.2f, up %.2f!", 1.23456, 2.34567
    );

    sprintf_case(
            "floating point hex: round up",
            "my number is down 0xa.56p+00, up 0xa.68p+00!",
            "my number is down %.2a, up %.2a!", 0xa.5678p00, 0xa.6789p00
    );

    /* clang-format on */
    return TEST_PASS;
}

int replace_nonprint(char *dst, const char *src, int len)
{
    char *start = dst;
    for (; len > 0; len--, src++) {
        if (isprint(*src)) *(dst++) = *src;
        else *(dst++) = '-';
    }
    *dst = '\0';
    return dst - start;
}

int test_snprintf_cutoff()
{
    char buf[] = "abcdefghijklmnop";
    int  count = snprintf(buf, 3, "hello");
    tassert_streq("he", buf); // 2 letters + null byte = length 3
    tassert_eq(5, count);     // should return count as if buf was long enough

    char expected[] = "he\0defghijklmnop";
    char expected_str[64];
    char buf_str[64];
    replace_nonprint(expected_str, expected, 16);
    replace_nonprint(buf_str, buf, 16);

    int cmp = memcmp(expected, buf, 16);

    tassertf(
            cmp == 0,
            "byte strings differ:\n"
            "expected:  %s\n"
            "got:       %s\n"
            "memcmp:    %d\n",
            expected_str, buf_str, cmp
    );
    return TEST_PASS;
}

