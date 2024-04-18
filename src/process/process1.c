/*
 * Just "flies a plane" over the screen.
 *
 * The plane will fire a bullet when it detects a message in its mailbox.
 * (Type "fire" in the shell.)
 */

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/common.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "syslib.h"

#define PLANE_ROWS    4
#define PLANE_COLUMNS 18

/* clang-format off */
static const char plane_art[PLANE_ROWS][PLANE_COLUMNS + 1] =
{
    "     ___       _  ",
    " | __\\_\\_o____/_| ",
    " <[___\\_\\_-----<  ",
    " |  o'            "
};
/* clang-format on */

#define PLANE_LOC_X_MAX (PLANE_COL_MAX)

#define DELAY_MS 250

#define COMMAND_MBOX 1

static struct term term = PLANE_TERM_INIT;

static void draw_plane(int x, int y)
{
    struct termwin win = term_getwin(&term);

    for (int i = 0; i < PLANE_ROWS; i++) {
        for (int j = 0; j < PLANE_COLUMNS; j++) {
            int row = y + i;
            int col = x + j;
            if (0 <= row && row < win.height && 0 <= col && col < win.width) {
                tprintf(&term, ANSIF_CUP, row + 1, col + 1);
                tprintf(&term, "%c", plane_art[i][j]);
            }
        }
    }
}

/* === Bullet Logic === */

struct bullet_state {
    int q;        // Mailbox to read for "fire" message
    int fired;    // A bullet has been fired and is active
    int bullet_x; // x position on screen
    int bullet_y; // y position on screen
};

/* Read the character stored at location (x,y) on the screen */
int peek_screen(int x, int y)
{
    colorchar_t *screen = (colorchar_t *) VGA_BASE;
    return screen[y * VGA_COLS + x] & 0xff;
}

void poke_screen(int x, int y, colorchar_t ch)
{
    colorchar_t *screen      = (colorchar_t *) VGA_BASE;
    screen[y * VGA_COLS + x] = ch;
}

static void draw_bullet(int loc_x, int loc_y)
{
    color_t color = fgbg(COLOR_BRIGHT_RED, COLOR_BLACK);
    poke_screen(loc_x, loc_y, colorchar(color, '<'));
    poke_screen(loc_x + 1, loc_y, colorchar(color, '='));
}

static void erase_bullet(int loc_x, int loc_y)
{
    color_t color = fgbg(COLOR_LIGHT_GREY, COLOR_BLACK);
    poke_screen(loc_x, loc_y, colorchar(color, ' '));
    poke_screen(loc_x + 1, loc_y, colorchar(color, ' '));
}

static void bullet_logic(struct bullet_state *bullet, int plane_x, int plane_y)
{
    /* mbox opened and bullet not fired? */
    if ((bullet->q >= 0) && (!bullet->fired)) {

        int msg_count, msg_space;
        mbox_stat(bullet->q, &msg_count, &msg_space);

        if (msg_count > 0) {
            /* if messages in mbox, recive first command */
            msg_t m;
            mbox_recv(bullet->q, &m);
            if (m.size != 0) {
                exit();
            }

            struct termwin win   = term_getwin(&term);
            int            loc_y = win.start_row + plane_y;
            int            loc_x = win.start_col + plane_x;

            /* fire bullet */
            bullet->fired    = true;
            bullet->bullet_y = loc_y + 2;
            bullet->bullet_x = loc_x - 2;
            if (bullet->bullet_x < 0) bullet->bullet_x = VGA_COLS - 1;
        }
    }

    if (bullet->fired) {
        /* erase bullet */
        erase_bullet(bullet->bullet_x, bullet->bullet_y);

        int c;

        /* if bullet hit a character at screen[X-1, Y] */
        if ((bullet->bullet_x - 1 >= 0)
            && ((c = peek_screen(bullet->bullet_x - 1, bullet->bullet_y)) != 0)
            && (c != ' ')) {
            /*
             * erase bullet and character at
             * screen[X-1, Y]
             */
            erase_bullet(bullet->bullet_x - 1, bullet->bullet_y);
            bullet->fired = false;
        }
        /* if bullet hit a character at screen[X-2, Y] */
        else if ((bullet->bullet_x - 2 >= 0) && ((c = peek_screen(bullet->bullet_x - 2, bullet->bullet_y)) != 0) && (c != ' ')) {
            /*
             * erase bullet and character at
             * screen[X-2, Y]
             */
            erase_bullet(bullet->bullet_x - 2, bullet->bullet_y);
            bullet->fired = false;
        }
        /* bullet did not hit a character */
        else {
            bullet->bullet_x -= 2;
            if (bullet->bullet_x < 0) bullet->fired = false;
            else draw_bullet(bullet->bullet_x, bullet->bullet_y);
        }
    }
}

int main(void)
{

    struct bullet_state bullet = {
            .q        = mbox_open(COMMAND_MBOX),
            .fired    = false,
            .bullet_x = -1,
            .bullet_y = -1,
    };

    struct termwin win = term_getwin(&term);

    int       plane_y = 0;
    int       plane_x = win.width;
    const int wrap_x  = 0 - PLANE_COLUMNS;

    while (1) {
        /* adjust plane position */
        if (--plane_x < wrap_x) plane_x = win.width;

        /* draw */
        draw_plane(plane_x, plane_y);

        bullet_logic(&bullet, plane_x, plane_y);
        ms_delay(DELAY_MS);
    }

    if (bullet.q >= 0) { /* should not be reached */
        mbox_close(bullet.q);
    }

    return 0;
}
