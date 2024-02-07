/*
 * Driver for output to a region of the screen
 */
#ifndef TERMBUF_H
#define TERMBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "ansi_parse.h"

/*
 * The default VGA text screen is 25 rows by 80 columns of 16-bit cells.
 *
 * For each cell:
 *  - low byte: ASCII character
 *  - high byte: color
 *      - low nibble: foreground color
 *      - high nibble: background color
 *
 * Each color is an RGB value with one bit per color. The fourth bit is for
 * intensity. If that bit is set, the color is brighter.
 */

typedef uint8_t  color_t;     // Terminal color attribute
typedef uint16_t colorchar_t; // Color + character pair

#define fgbg(fg, bg) ((uint8_t) ((bg << 4) + fg))
#define colorchar(color, ch) \
    ((uint16_t) (((uint8_t) color << 8) + (uint8_t) ch))

enum color {
    /*                       iRGB */
    COLOR_BLACK          = 0b0000,
    COLOR_RED            = 0b0100,
    COLOR_GREEN          = 0b0010,
    COLOR_BLUE           = 0b0001,
    COLOR_CYAN           = 0b0011,
    COLOR_MAGENTA        = 0b0101,
    COLOR_BROWN          = 0b0110, // Technically non-bright yellow
    COLOR_LIGHT_GREY     = 0b0111, // Technically non-bright white
    COLOR_DARK_GREY      = 0b1000, // Technically bright black
    COLOR_BRIGHT_RED     = 0b1100,
    COLOR_BRIGHT_GREEN   = 0b1010,
    COLOR_BRIGHT_BLUE    = 0b1001,
    COLOR_BRIGHT_CYAN    = 0b1011,
    COLOR_BRIGHT_MAGENTA = 0b1101,
    COLOR_YELLOW         = 0b1110, // Technically bright yellow
    COLOR_WHITE          = 0b1111,

    COLOR_GREY_ON_BLACK = fgbg(COLOR_LIGHT_GREY, COLOR_BLACK),
};

struct termbuf {
    colorchar_t *base;
    size_t       rows;
    size_t       cols;
};

struct termwin {
    int start_row;
    int height;
    int start_col;
    int width;
};

enum termwin_overflow {
    TERM_OF_CLIP,
    TERM_OF_SCROLL,
    TERM_OF_GROW,
};

struct term {
    struct termbuf buf;
    struct termwin win;

    enum termwin_overflow overflow;

    /* Cursor state */
    int     row;   // Row number in window
    int     col;   // Column number in window
    color_t color; // Color to write

    bool        show_cursor;    // Should we display the cursor?
    bool        cursor_visible; // Cursor is currently on screen
    colorchar_t cursor_color;   // Color and character of cursor

    struct ansi_parse parser;
};

#define TERMWIN_INIT(SROW, HEIGHT, SCOL, WIDTH) \
    { \
        .start_row = SROW, .height = HEIGHT, .start_col = SCOL, \
        .width = WIDTH \
    }

#define TERM_INIT(BASE, BROWS, BCOLS, RSTART, HEIGHT, CSTART, WIDTH) \
    { \
        .buf = {.base = (colorchar_t *) BASE, .rows = BROWS, .cols = BCOLS}, \
        .win = {.start_row = RSTART, \
                .height    = HEIGHT, \
                .start_col = CSTART, \
                .width     = WIDTH}, \
        .overflow = TERM_OF_SCROLL, .row = 0, .col = 0, \
        .color        = COLOR_GREY_ON_BLACK, \
        .cursor_color = fgbg(COLOR_BLACK, COLOR_LIGHT_GREY), \
        .parser       = ANSI_PARSE_INIT, \
    }

#define VGA_BASE 0xb8000
#define VGA_ROWS 25
#define VGA_COLS 80

#define TERM_INIT_VGA_WIN(RSTART, HEIGHT, CSTART, WIDTH) \
    TERM_INIT(VGA_BASE, VGA_ROWS, VGA_COLS, RSTART, HEIGHT, CSTART, WIDTH)

#define TERM_INIT_VGA_FULL \
    TERM_INIT(VGA_BASE, VGA_ROWS, VGA_COLS, 0, VGA_ROWS, 0, VGA_COLS)

#define TERM_INIT_VGA(WIN)

void term_putc(struct term *t, int ch);
void term_puts(struct term *t, const char *str);
void term_write(struct term *t, const char *buf, size_t nbyte);

struct termwin term_getwin(struct term *t);

#endif /* TERMBUF_H */
