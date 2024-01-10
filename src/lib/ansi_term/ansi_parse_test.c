#include "ansi_parse.h"

#include <unittest/unittest.h>

#include "ansi_esc.h"

static int ansi_parse_puts(struct ansi_parse *parser, char *str)
{
    int ret = 0;
    for (; str && *str; str++) ret = ansi_parse_putc(parser, *str);
    return ret;
}

int test_ansi_parse()
{
    struct ansi_parse parser = ANSI_PARSE_INIT;

    int ret;

    ret = ansi_parse_puts(&parser, "hello");
    tassert_eq('o', ret);

    ret = ansi_parse_puts(&parser, "\033[22;40H");
    tassert_eq(ANSI_CUP, parser.cmd);
    tassert_eq(22, parser.csi_param[0]);
    tassert_eq(40, parser.csi_param[1]);
    tassert_eq(-APS_FINAL, ret);

    ret = ansi_parse_puts(&parser, "\033[J");
    tassert_eq(ANSI_ED, parser.cmd);
    tassert_eq(0, parser.csi_param[0]);
    tassert_eq(-APS_FINAL, ret);
    return TEST_PASS;
}

