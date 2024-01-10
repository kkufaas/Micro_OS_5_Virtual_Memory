#include "ctype.h"

#include "stdio.h"

#include <unittest/unittest.h>

int test_isdigit()
{
    tassert(!isdigit('a'));
    tassert(isdigit('0'));
    tassert(isdigit('1'));
    tassert(isdigit('2'));
    tassert(isdigit('3'));
    tassert(isdigit('4'));
    tassert(isdigit('5'));
    tassert(isdigit('6'));
    tassert(isdigit('7'));
    tassert(isdigit('8'));
    tassert(isdigit('9'));
    return TEST_PASS;
}

int test_isspace()
{
    tassert(isspace(' '));
    tassert(isspace('\t'));
    tassert(isspace('\n'));
    tassert(isspace('\r'));
    tassert(isspace('\f'));
    tassert(isspace('\v'));
    tassert(!isspace('h'));
    tassert(!isspace('\0'));
    return TEST_PASS;
}

int test_isprint()
{
    tassertf(
            !isprint(EOF),
            "EOF character %d (%#x) should not be considered printable, "
            "but it was\n",
            EOF, EOF
    );
    for (int ch = 0; ch < 0x20; ch++) {
        tassertf(
                !isprint(ch),
                "character %d (%#x) should not be considered printable, "
                "but it was\n",
                ch, ch
        );
    }
    for (int ch = 0x20; ch <= 0x7e; ch++) {
        tassertf(
                isprint(ch),
                "character %d (%#x, '%c') should be considered printable, "
                "but it was not\n",
                ch, ch, ch
        );
    }
    for (int ch = 0x7f; ch < 255; ch++) {
        tassertf(
                !isprint(ch),
                "character %d (%#x) should not be considered printable, "
                "but it was\n",
                ch, ch
        );
    }
    return TEST_PASS;
}

#define upper_lower_case(ch, UP, LO) \
    do { \
        int up = isupper(ch); \
        int lo = islower(ch); \
        tassertf( \
                (bool) up == (bool) UP && (bool) lo == (bool) LO, \
                "char '%c' (%d, %#x) upper/lower should be %d/%d. Got " \
                "%d/%d.\n", \
                ch, ch, ch, UP, LO, up, lo \
        ); \
    } while (0)

int test_isupper_islower()
{
    upper_lower_case(EOF, false, false);
    for (int ch = 0; ch < 'A'; ch++) upper_lower_case(ch, false, false);
    for (int ch = 'A'; ch <= 'Z'; ch++) upper_lower_case(ch, true, false);
    for (int ch = 'Z' + 1; ch < 'a'; ch++) upper_lower_case(ch, false, false);
    for (int ch = 'a'; ch <= 'z'; ch++) upper_lower_case(ch, false, true);
    for (int ch = 'z' + 1; ch < 256; ch++) upper_lower_case(ch, false, false);

    return TEST_PASS;
}

