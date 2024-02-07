/*
 * Barrier test program
 */

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "lib/assertk.h"
#include "scheduler.h"
#include "sync.h"

#define BTHREAD_CT 3
#define ITERATIONS 20000
#define DELAY_MS   20

static barrier_t barrier = BARRIER_INIT(BTHREAD_CT);

static volatile int bvar[BTHREAD_CT] = {};

static struct term term = BAR_TERM_INIT;
static lock_t      term_lock;

static void barrier_print_status(int bi)
{
    lock_acquire(&term_lock);

    tprintf(&term, ANSIF_CUP, bi + 1, 1);
    tprintf(&term, "%2d ", current_running->pid);

    char label = 'A' + bi;
    tprintf(&term, "barrier test %c: %5d", label, bvar[bi]);
    tprintf(&term, ANSIF_EL "\n", ANSI_EFWD); // Clear rest of line
    lock_release(&term_lock);
}

static void barrier_test(int bi)
{
    for (int i = 0; i < ITERATIONS; i++) {
        /* Phase 1: Update variable. */
        bvar[bi]++;
        barrier_print_status(bi);

        /* Checkpoint: All threads have updated their variable. */
        barrier_wait(&barrier);

        /* Delay and yield to give more chances for desync if barriers are not
         * properly enforced. */
        ms_delay(DELAY_MS);
        yield();
        ms_delay(DELAY_MS);

        /* Phase 2: Check that all variables are still in sync. */
        for (int bj = 0; bj < BTHREAD_CT; bj++)
            assertf(bvar[bi] == bvar[bj],
                    "barrier %d: desync: I have %d, but %d has %d\n", bi,
                    bvar[bi], bj, bvar[bj]);

        /* Checkpoint: All threads have checked their variables. */
        barrier_wait(&barrier);
    }

    /* End: Check result. */
    assertf(bvar[bi] == ITERATIONS,
            "barrier %d: bad result: exited loop with %d, should be %d\n", bi,
            bvar[bi], ITERATIONS);

    exit();
}

void barrier1(void) { barrier_test(0); }
void barrier2(void) { barrier_test(1); }
void barrier3(void) { barrier_test(2); }

