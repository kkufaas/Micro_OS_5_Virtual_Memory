#include "termbuf.h"

#include <stdarg.h>
#include <string.h>

#include <syslib/addrs.h>

#include "ansi_esc.h"
#include "ansi_parse.h"

static colorchar_t *term_ptr(const struct term *t, int row, int col)
{
    return t->buf.base + (t->win.start_row + row) * t->buf.cols
           + t->win.start_col + col;
}

static bool is_in_bounds(const struct term *t, int row, int col)
{
    return 0 <= row && row < t->win.height && 0 <= col && col < t->win.width;
}

static void
setcc(const struct term *t, int row, int col, colorchar_t colorchar)
{
    if (is_in_bounds(t, row, col)) {
        colorchar_t *ptr = term_ptr(t, row, col);
        *ptr             = colorchar;
    }
}

static void setc(const struct term *t, int row, int col, color_t color, int ch)
{
    setcc(t, row, col, colorchar(color, ch));
}

static void setcolor(const struct term *t, int row, int col, color_t color)
{
    if (is_in_bounds(t, row, col)) {
        colorchar_t *ptr = term_ptr(t, row, col);

        char ch = *ptr & 0xff;
        *ptr    = colorchar(color, ch);
    }
}

static void erase_rows(const struct term *t, int start_row, int end_row)
{
    for (int r = start_row; r < end_row; r++) {
        for (int c = 0; c < t->win.width; c++) {
            setc(t, r, c, COLOR_GREY_ON_BLACK, 0);
        }
    }
}

static void erase_line(struct term *t, int mode)
{
    int start, end;
    switch (mode) {
    case ANSI_EFWD:
        start = t->col;
        end   = t->win.width;
        break;
    case ANSI_EBACK:
        start = 0;
        end   = t->col;
        break;
    case ANSI_EFULL:
        start = 0;
        end   = t->win.width;
        break;
    default: return;
    }

    for (int c = start; c < end; c++)
        setc(t, t->row, c, COLOR_GREY_ON_BLACK, 0);
}

static void erase_display(struct term *t, int mode)
{
    int start, end;
    switch (mode) {
    case ANSI_EFWD:
        start = t->row + 1;
        end   = t->win.height;
        break;
    case ANSI_EBACK:
        start = 0;
        end   = t->row;
        break;
    case ANSI_EFULL:
        start = 0;
        end   = t->win.height;
        break;
    default: return;
    }

    /* Erase before or after in current line according to mode. */
    erase_line(t, mode);

    /* Erase lines above or below according to mode. */
    erase_rows(t, start, end);
}

static void term_setpos(struct term *t, int row, int col)
{
    t->row = row;
    t->col = col;
}

static void scroll(struct term *t, int row_ct)
{
    int r;

    /* Copy rows */
    for (r = 0; r + row_ct < t->win.height; r++) {
        colorchar_t *dest = term_ptr(t, r, 0);
        colorchar_t *src  = term_ptr(t, r + row_ct, 0);
        memcpy(dest, src, t->win.width * sizeof(colorchar_t));
    }

    /* Blank bottom row(s) */
    erase_rows(t, r, t->win.height);

    /* Move position up to match scroll */
    term_setpos(t, t->row - row_ct, t->col);
}

static void overflow(struct term *t)
{
    int row_ct = t->row - t->win.height + 1;
    if (row_ct <= 0) return;

    switch (t->overflow) {
    case TERM_OF_CLIP: break;
    case TERM_OF_SCROLL: scroll(t, row_ct); break;
    case TERM_OF_GROW:
        while (row_ct && t->win.start_row) {
            t->win.start_row--;
            t->win.height++;
            t->row++;
            scroll(t, 1);
            row_ct--;
        }
        while (row_ct && t->win.height < (int) t->buf.rows) {
            t->win.height++;
            row_ct--;
        }
        if (row_ct) scroll(t, row_ct);
        break;
    };
}

static void move_cursor_into_bounds(struct term *t)
{
    if (t->overflow == TERM_OF_CLIP) return;

    /* Wrap at end of line. */
    if (t->col >= t->win.width) term_setpos(t, t->row + 1, 0);

    /* Handle overflow */
    if (t->row >= t->win.height) overflow(t);
}

static void backspace(struct term *t)
{
    if (t->row == 0 && t->col == 0) return; // Already at beginning

    t->col--;
    if (t->col < 0) {
        t->col = t->win.width - 1;
        t->row--;
    }
    setc(t, t->row, t->col, t->color, '\0');
}

static int ansi_pos_param(struct term *t, int i)
{
    int param = t->parser.csi_param[i];
    if (!param) param = 1; // Default to 1 (ANSI positions are 1-indexed)
    return param - 1;      // Adjust 1-indexed parameter to 0-indexed
}

static void execute_ansi_escape(struct term *t)
{
    switch (t->parser.cmd) {
    case ANSI_CUP: { // Cursor Position
        int n = ansi_pos_param(t, 0);
        int m = ansi_pos_param(t, 1);
        term_setpos(t, n, m);
        break;
    }
    case ANSI_EL: { // Erase in Line
        int n = t->parser.csi_param[0];
        erase_line(t, n);
        break;
    }
    case ANSI_ED: { // Erase in Display
        int n = t->parser.csi_param[0];
        erase_display(t, n);
        break;
    }
    }
}

static void clear_cursor(struct term *t)
{
    if (t->cursor_visible) {
        setcolor(t, t->row, t->col, t->color);
        t->cursor_visible = false;
    }
}

static void write_cursor(struct term *t)
{
    if (t->show_cursor) {
        move_cursor_into_bounds(t);
        setcolor(t, t->row, t->col, t->cursor_color);
        t->cursor_visible = true;
    }
}

void term_putc(struct term *t, int ch)
{
    clear_cursor(t);

    ch = ansi_parse_putc(&t->parser, ch);

    switch (ch) {
    case -APS_ESCAPE:
    case -APS_INVALID:
    case -APS_UNIMPLEMENTED: return;
    case -APS_FINAL: execute_ansi_escape(t); return;
    default: break;
    }

    move_cursor_into_bounds(t);

    /* Handle character */
    switch (ch) {
    case '\n': term_setpos(t, t->row + 1, 0); break;
    case '\r': term_setpos(t, t->row, 0); break;
    case '\b': backspace(t); break;
    default:
        setc(t, t->row, t->col, t->color, ch);
        t->col++;
        break;
    }

    write_cursor(t);
}

void term_puts(struct term *t, const char *str)
{
    if (!str) return;

    for (; *str; str++) {
        term_putc(t, *str);
    }
}

void term_write(struct term *t, const char *buf, size_t nbyte)
{
    if (!buf) return;

    for (; nbyte; nbyte--, buf++) {
        term_putc(t, *buf);
    }
}

struct termwin term_getwin(struct term *t) { return t->win; }

