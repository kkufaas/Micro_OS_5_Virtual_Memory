/*
 * Simple process which does some calculations and exits.
 */

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/common.h>
#include <syslib/screenpos.h>
#include <syslib/syslib.h>
#include <util/util.h>

#define DELAY_MS 700

/* calculate 1 + ... + n */
static int rec(int n)
{
    if (n % 37 == 0) {
        yield();
    }

    if (n == 0) {
        return 0;
    } else {
        return n + rec(n - 1);
    }
}

int main(void)
{
    static struct term term = SUM_TERM_INIT;

    int i, res;

    for (i = 0; i <= 50; i++) {
        res = rec(i);

        tprintf(&term, "%2d ", getpid());
        tprintf(&term, "Did you know: 1 + ... + %d = %d ", i, res);
        tprintf(&term, ANSIF_EL "\r", ANSI_EFWD); // Clear right, then return

        ms_delay(DELAY_MS);
        yield();
    }

    return 0;
}
