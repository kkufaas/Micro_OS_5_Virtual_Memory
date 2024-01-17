#include "termbuf.h"

#include <string.h>

#include <unittest/unittest.h>

#include "ansi_esc.h"

typedef unsigned char bufarray[25][80][2];
typedef char          linebuf[81];

static void bufstr(bufarray buf, int row, int col, char *str)
{
    for (int c = col; c < 80; c++, str++) {
        *str = buf[row][c][0];
    }
    str[80] = 0;
}

static void fillbuf(bufarray buf, int ch)
{
    for (int r = 0; r < 25; r++) {
        for (int c = 0; c < 80; c++) {
            buf[r][c][0] = ch;
        }
    }
}

static void term_setpos(struct term *t, int row, int col)
{
    t->row = row;
    t->col = col;
}

int test_termbuf()
{
    bufarray buf;
    linebuf  line;
    fillbuf(buf, 0);

    struct term term = TERM_INIT(buf, 25, 80, 0, 25, 0, 80);

    term_puts(&term, "hello!");
    bufstr(buf, 0, 0, line);
    tassert_streq("hello!", line);
    tassert_eq(COLOR_GREY_ON_BLACK, buf[0][0][1]);

    /* Newline should move to next line */
    term_puts(&term, "\nworld!");
    bufstr(buf, 1, 0, line);
    tassert_streq("world!", line);

    /* Carriage return should move to left without going to next line */
    term_puts(&term, "\rnew world!");
    bufstr(buf, 1, 0, line);
    tassert_streq("new world!", line);

    return TEST_PASS;
}

int test_termbuf_window()
{
    bufarray buf;
    linebuf  line;
    fillbuf(buf, 0);

    /* Tiny window */
    struct term term = TERM_INIT(buf, 25, 80, 2, 3, 10, 5);

    /* Text should wrap */
    {
        term_puts(&term, "abcdefghijkl");
        bufstr(buf, 1, 10, line);
        tassert_streq("", line);

        bufstr(buf, 2, 10, line);
        tassert_streq("abcde", line);

        bufstr(buf, 3, 10, line);
        tassert_streq("fghij", line);

        bufstr(buf, 4, 10, line);
        tassert_streq("kl", line);

        bufstr(buf, 5, 10, line);
        tassert_streq("", line);
    }

    /* Should be no writing outside of window */
    {
        tassert_eq(0, buf[1][10][0]);
        tassert_eq(0, buf[1][14][0]);
        tassert_eq(0, buf[2][9][0]);
        tassert_eq(0, buf[2][15][0]);
    }

    /* Add a newline which should put cursor below the bottom of the screen */
    term_puts(&term, "\n");

    /* Screen should not scroll yet */
    {
        bufstr(buf, 4, 10, line);
        tassert_streq("kl", line);

        bufstr(buf, 5, 10, line);
        tassert_streq("", line);
    }

    /* Write with cursor at bottom of screen */
    term_putc(&term, 'm');

    /* Now it should scroll */
    {
        bufstr(buf, 1, 10, line);
        tassert_streq("", line);

        bufstr(buf, 2, 10, line);
        tassert_streq("fghij", line);

        bufstr(buf, 3, 10, line);
        tassert_streq("kl", line);

        bufstr(buf, 4, 10, line);
        tassert_streq("m", line);

        bufstr(buf, 5, 10, line);
        tassert_streq("", line);
    }

    return TEST_PASS;
}

int test_termbuf_ansi_cursor_pos()
{
    bufarray buf;
    linebuf  line;
    fillbuf(buf, 0);

    struct term term = TERM_INIT(buf, 25, 80, 0, 25, 0, 80);

    term_puts(&term, "\033[4;10Hhello!");

    bufstr(buf, 0, 0, line);
    tassert_streq("", line);

    /* Remember that ANSI escape positions are 1-indexed,
     * so moving to 4;10 puts our 0-indexed cursor at [3][9] */
    bufstr(buf, 3, 9, line);
    tassert_streq("hello!", line);

    /* Parameters should default to 1 */
    term_puts(&term, "\033[Hworld!");

    bufstr(buf, 0, 0, line);
    tassert_streq("world!", line);

    return TEST_PASS;
}

int test_termbuf_ansi_erase_line()
{
    bufarray buf;
    linebuf  line;
    fillbuf(buf, 0);

    struct term term = TERM_INIT(buf, 25, 80, 0, 25, 0, 80);

    /* Default erase: forward */
    {
        term_setpos(&term, 10, 0);
        term_puts(&term, "hello!!");

        term_setpos(&term, 10, 3);
        term_puts(&term, "\033[K");

        bufstr(buf, 10, 0, line);
        tassert_streq("hel", line);
    }

    /* Erase backward */
    {
        term_setpos(&term, 10, 0);
        term_puts(&term, "hello!!");

        term_setpos(&term, 10, 3);
        term_puts(&term, "\033[1K");

        tassert_eq(0, buf[10][0][0]);
        tassert_eq(0, buf[10][1][0]);
        tassert_eq(0, buf[10][2][0]);

        bufstr(buf, 10, 3, line);
        tassert_streq("lo!!", line);
    }

    /* Erase full */
    {
        term_setpos(&term, 10, 0);
        term_puts(&term, "hello!!");

        term_setpos(&term, 10, 3);
        term_puts(&term, "\033[2K");

        tassert_eq(0, buf[10][0][0]);
        tassert_eq(0, buf[10][1][0]);
        tassert_eq(0, buf[10][2][0]);
        tassert_eq(0, buf[10][3][0]);
        tassert_eq(0, buf[10][4][0]);
        tassert_eq(0, buf[10][5][0]);
    }

    return TEST_PASS;
}

int test_termbuf_ansi_erase_display()
{
    bufarray buf;
    fillbuf(buf, 0);

    struct term term = TERM_INIT(buf, 25, 80, 1, 23, 1, 78);

    /* Control: filled buffer */
    {
        fillbuf(buf, '*');

        tassert_eq('*', buf[0][0][0]);   // Out of window
        tassert_eq('*', buf[1][1][0]);   // Upper left of window
        tassert_eq('*', buf[11][3][0]);  // Just before cursor
        tassert_eq('*', buf[11][4][0]);  // At cursor
        tassert_eq('*', buf[23][78][0]); // Lower right of window
        tassert_eq('*', buf[24][79][0]); // Out of window
    }

    /* Default erase: forward */
    {
        fillbuf(buf, '*');
        term_setpos(&term, 10, 3);
        term_puts(&term, "\033[J");

        tassert_eq('*', buf[0][0][0]);   // Out of window
        tassert_eq('*', buf[1][1][0]);   // Upper left of window
        tassert_eq('*', buf[11][3][0]);  // Just before cursor
        tassert_eq(0, buf[11][4][0]);    // At cursor
        tassert_eq(0, buf[23][78][0]);   // Lower right of window
        tassert_eq('*', buf[24][79][0]); // Out of window
    }

    /* Erase backward */
    {
        fillbuf(buf, '*');
        term_setpos(&term, 10, 3);
        term_puts(&term, "\033[1J");

        tassert_eq('*', buf[0][0][0]);   // Out of window
        tassert_eq(0, buf[1][1][0]);     // Upper left of window
        tassert_eq(0, buf[11][3][0]);    // Just before cursor
        tassert_eq('*', buf[11][4][0]);  // At cursor
        tassert_eq('*', buf[23][78][0]); // Lower right of window
        tassert_eq('*', buf[24][79][0]); // Out of window
    }

    /* Erase full */
    {
        fillbuf(buf, '*');
        term_setpos(&term, 10, 3);
        term_puts(&term, "\033[2J");

        tassert_eq('*', buf[0][0][0]);   // Out of window
        tassert_eq(0, buf[1][1][0]);     // Upper left of window
        tassert_eq(0, buf[11][3][0]);    // Just before cursor
        tassert_eq(0, buf[11][4][0]);    // At cursor
        tassert_eq(0, buf[23][78][0]);   // Lower right of window
        tassert_eq('*', buf[24][79][0]); // Out of window
    }

    return TEST_PASS;
}

int test_termbuf_backspace()
{
    bufarray buf;
    linebuf  line;
    fillbuf(buf, 0);

    /* Tiny window */
    struct term term = TERM_INIT(buf, 25, 80, 2, 10, 10, 5);

    /* Simple backspace */
    {
        term_puts(&term, "hello!\b");
        bufstr(buf, 2, 10, line);
        tassert_streq("hello", line);
    }

    /* Backspace at line beginning */
    {
        term_setpos(&term, 0, 0);
        term_puts(&term, "123456"); // Setup
        bufstr(buf, 2, 10, line);
        tassert_streq("12345", line); // Part of line
        bufstr(buf, 3, 10, line);
        tassert_streq("6", line); // Line has wrapped

        term_puts(&term, "\b\b"); // Backspace
        bufstr(buf, 3, 10, line);
        tassert_streq("", line); // First BS should erase char
        bufstr(buf, 2, 10, line);
        tassert_streq("1234", line); // Second BS should un-wrap
    }

    /* Backspace at top of screen */
    {
        fillbuf(buf, 0); // Clear
        term_setpos(&term, 0, 0);
        term_puts(&term, "h\b\bello"); // Backspace
        bufstr(buf, 2, 10, line);
        tassert_streq("ello", line); // No not un-wrap past top.
    }

    return TEST_PASS;
}

int test_termbuf_cursor()
{
    bufarray buf;
    fillbuf(buf, 0);

    struct term term = TERM_INIT(buf, 25, 80, 0, 25, 0, 80);

    term.show_cursor = true;
    term_puts(&term, "h"); // Backspace

    tassert_eq('h', buf[0][0][0]);
    tassert_eq(fgbg(COLOR_LIGHT_GREY, COLOR_BLACK), buf[0][0][1]);

    tassert_eq('\0', buf[0][1][0]);
    tassert_eq(fgbg(COLOR_BLACK, COLOR_LIGHT_GREY), buf[0][1][1]);

    return TEST_PASS;
}

