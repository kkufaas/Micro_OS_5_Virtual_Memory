/*
 * Simple programs to check locks and condition variables
 */

#include "th2.h"

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/common.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "lib/assertk.h"
#include "scheduler.h"
#include "sync.h"

/* lock thread status */
enum lock_thread_status {
    RUNNING,
    OK,
    FAILED,
};

static const char *status_str[] = {
        [RUNNING] = "Running",
        [OK]      = "OK",
        [FAILED]  = "Failed",
};

static void
print_trace_term(int threadnum, int value, enum lock_thread_status status);

static int          shared_var = 0;
static int          exit_count = 0; /* number of threads that have exited */
static volatile int init       = false;     /* is lock l initialized */
static lock_t       l          = LOCK_INIT; /* Lock for the monitor */

static const int n2       = 200;
static const int n3       = 100;
static const int ntotal = n2 + n3;
static const int delay_ms = 300;

static const int   divby = 3;
static condition_t divisible;   /* thread3 waits for this one */
static condition_t indivisible; /* thread2 waits for this one */

static const int switch_to_broadcast = 20;

void lock_thread0(void)
{
    int tmp, i;

    l = (lock_t) LOCK_INIT;
    divisible   = (condition_t) CONDITION_INIT;
    indivisible = (condition_t) CONDITION_INIT;
    init = true;

    for (i = 0; i < n2; i++) {
        lock_acquire(&l);

        while (shared_var % divby == 0) {
            condition_wait(&l, &indivisible);
        }
        assertk(shared_var % divby != 0);

        tmp = shared_var;

        if (i % 13 == 0) {
            yield();
        }

        shared_var = tmp + 1;

        if (shared_var % divby == 0) {
            /* Signal or broadcast condition for thread3 */
            if (shared_var < switch_to_broadcast) condition_signal(&divisible);
            else condition_broadcast(&divisible);
        }

        print_trace_term(2, shared_var, RUNNING);
        ms_delay(delay_ms);
        lock_release(&l);

    }

    lock_acquire(&l);
    exit_count++;
    lock_release(&l);

    for (;;) {
        bool both_exited = false;
        lock_acquire(&l);
        both_exited = exit_count == 2;
        lock_release(&l);
        if (both_exited) break;
        yield();
    }

    lock_acquire(&l);
    /* when both threads have finished we print the result */
    if (exit_count == 2) {
        enum lock_thread_status status = (shared_var == ntotal) ? OK : FAILED;
        print_trace_term(2, shared_var, status);
    }
    /* leave monitor */
    lock_release(&l);
    exit();
}

void lock_thread1(void)
{
    int tmp, i;

    while (!init) {
        yield();
    }

    for (i = 0; i < n3; i++) {
        lock_acquire(&l);

        while (shared_var % divby != 0) {
            condition_wait(&l, &divisible);
        }
        assertk(shared_var % divby == 0);

        tmp = shared_var;

        if (i % 17 == 0) {
            yield();
        }

        shared_var = tmp + 1;

        if (shared_var % divby != 0) {
            if (shared_var < switch_to_broadcast)
                condition_signal(&indivisible);
            else condition_broadcast(&indivisible);
        }

        print_trace_term(3, shared_var, RUNNING);
        ms_delay(delay_ms);
        lock_release(&l);

    }

    lock_acquire(&l);
    exit_count++;
    lock_release(&l);

    for (;;) {
        bool both_exited = false;
        lock_acquire(&l);
        both_exited = exit_count == 2;
        lock_release(&l);
        if (both_exited) break;
        yield();
    }

    lock_acquire(&l);
    /* when both threads have finished  print the result */
    if (exit_count == 2) {
        enum lock_thread_status status = (shared_var == ntotal) ? OK : FAILED;
        print_trace_term(3, shared_var, status);
    }
    /* leave monitor */
    lock_release(&l);
    exit();
}

static struct term lockterm = LOCKS_TERM_INIT;

static void
print_trace_term(int threadnum, int value, enum lock_thread_status status)
{
    int  threadzero = 2;
    int  line       = 1 + threadnum - threadzero;
    char label      = 'A' + threadnum - threadzero;
    tprintf(&lockterm, ANSIF_CUP, line, 1);
    tprintf(&lockterm, "%2d ", current_running->pid);
    tprintf(&lockterm, "Lock test %c: %3d %s", label, value,
            status_str[status]);
    tprintf(&lockterm, ANSIF_EL, ANSI_EFWD); // Clear right
}

