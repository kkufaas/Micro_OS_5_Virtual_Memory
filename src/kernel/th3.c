/*
 * Monte Carlo Pi calcuation
 *
 * These threads excercise the lock implementation and the saving of
 * floating point context across process switches.
 *
 * The Monte Carlo method of estimating pi uses a unitary circle,
 * inscribed inside a square. The application repeatedly picks a random
 * point within the square, the fraction of the points that are inside
 * the unitary circle then represents pi/4.
 *
 * Implementation trades Pi accuracy and performance for determenism.
 *
 * Excpected behavior:
 * - Accuracy of printed results increase.
 * - Final result is always the same.
 *
 * Note! Threads have not been debugged. The file may be changed.
 */

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "scheduler.h"
#include "sync.h"

static const int WORKERS = 4;
static const int DARTS = 40000;

/* Global stats */
static volatile int gdarts;
static volatile int ghits;
static lock_t       glock;

/* Number of workers that are finished */
static volatile int done_ct;
static lock_t       donelock;

/* Terminal to print on */
static struct term term = MONTE_TERM_INIT;
static lock_t      termlock;

/* are mutexes and global variables initialized */
static volatile int init = false;

void do_init(void)
{
    glock    = (lock_t) LOCK_INIT;
    donelock = (lock_t) LOCK_INIT;
    termlock = (lock_t) LOCK_INIT;
    ghits    = 0.0;
    done_ct  = 0;
    init     = true;
}

void wait_for_init(void)
{
    while (init == false) {
        yield();
    }
}

void print_mcpi_status(int line, char label, int hits, int darts)
{
    double estpi = 4.0 * hits / darts;
    lock_acquire(&termlock);
    tprintf(&term, ANSIF_CUP, line + 1, 0);
    tprintf(&term, "%2d ", current_running->pid);
    tprintf(&term, "Pi %c: %5d/%5d->%f", label, hits, darts, estpi);
    lock_release(&termlock);
}

double my_rand(double x)
{
    x = x * 1048576;
    x = 1.0 * (((1629 * (int) x) + 1) % 1048576);
    return x / 1048576.0;
}

int is_hit(double x, double y)
{
    /* The mid-computation yield calls here cause trouble if the task switch
     * code does not properly save and restore floating-point registers. */
    double x2 = x * x;
    yield();
    double y2 = y * y;
    yield();
    double distance = x2 + y2;
    yield();

    return distance <= 1;
}

void mcpi(int worker_index, int cnt)
{
    int darts = 0;
    int hits  = 0;

    double x = 0.1 + 0.15 * worker_index;
    double y = 1.0 - 0.33 * worker_index;

    while (darts < cnt) {
        x            = my_rand(x);
        y            = my_rand(y);
        bool got_hit = is_hit(x, y);
        darts++;
        if (got_hit) hits++;

        lock_acquire(&glock);
        if (got_hit) ghits++;
        gdarts++;
        lock_release(&glock);

        /* Computation does not contribute to results, but is done to
         * reduce accuracy of result if context switch mechnaism is not
         * working correctly
         */
        is_hit(2.0, 2.0); // Never a hit

        const char label = 'A' + worker_index;
        print_mcpi_status(worker_index, label, hits, darts);

        if (worker_index == WORKERS - 1) {
            lock_acquire(&glock);
            int lghits  = ghits;
            int lgdarts = gdarts;
            lock_release(&glock);

            print_mcpi_status(WORKERS, 'T', lghits, lgdarts);
        }
    }
}

void mcpi_worker(int worker_index)
{
    if (worker_index == 0) do_init();
    else wait_for_init();

    int worker_darts = DARTS / WORKERS;
    if (worker_index == 0) worker_darts += DARTS % WORKERS;

    mcpi(worker_index, worker_darts);

    lock_acquire(&donelock);
    done_ct++;
    lock_release(&donelock);

    exit();
}

void mcpi_thread0(void) { mcpi_worker(0); }
void mcpi_thread1(void) { mcpi_worker(1); }
void mcpi_thread2(void) { mcpi_worker(2); }
void mcpi_thread3(void) { mcpi_worker(3); }
