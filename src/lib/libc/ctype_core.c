#include "ctype.h"

int isdigit(int ch)
{
    switch (ch) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': return 1;
    default: return 0;
    }
}

int isspace(int ch)
{
    switch (ch) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\f':
    case '\v': return 1;
    default: return 0;
    }
}

int isprint(int ch)
{
    /* In 7-bit ASCII (chars 0--127), the printable characters span from
     * 0x20 (' ') to 0x7e ('~'). */
    return 0x20 <= ch && ch <= 0x7e;

    /* Not yet implemented: locales */
}

int isupper(int ch)
{
    if ('A' <= ch && ch <= 'Z') return 1;
    return 0;
}

int islower(int ch)
{
    if ('a' <= ch && ch <= 'z') return 1;
    return 0;
}

