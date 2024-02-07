/*
 * Just "flies a plane" over the screen.
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

#define PRIORITY_FRAME_COUNT 100
#define PRIORITY_MIN         10
#define PRIORITY_MAX         64

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

int main(void)
{
    /* priority state */
    int          frame_count = 0;
    unsigned int priority    = getpriority();

    struct termwin win = term_getwin(&term);

    int       plane_y = 0;
    int       plane_x = win.width;
    const int wrap_x  = 0 - PLANE_COLUMNS;

    while (1) {
        /* adjust plane position */
        if (--plane_x < wrap_x) plane_x = win.width;

        /* draw */
        draw_plane(plane_x, plane_y);

        /* Adjust priority */
        frame_count++;
        if (frame_count % PRIORITY_FRAME_COUNT == 0) {

            priority++;
            if (priority > PRIORITY_MAX) {
                priority = PRIORITY_MIN;
            }
            setpriority(priority);
        }

        ms_delay(DELAY_MS);
    }

    return 0;
}
