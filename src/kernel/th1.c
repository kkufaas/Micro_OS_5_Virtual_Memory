#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "pcb.h"
#include "scheduler.h"
#include "th1.h"
#include "time.h"

static struct term clockterm = CLOCK_TERM_INIT;

/*
 * This thread runs indefinitely, which means that the scheduler
 * should never run out of processes.
 */
void clock_thread(void)
{
    /* 64-bit division is tricky, so we shift the values down to 32-bit ints.
     */
    /* Shifting down by 20 bits approximates division by 1 million. */
    const unsigned int tick_shift = 20;

    uint64_t start_ticks = read_cpu_ticks();

    while (1) {
        uint64_t     ticks_now     = read_cpu_ticks();
        uint64_t     elapsed       = ticks_now - start_ticks;
        unsigned int elapsed_shift = (int) (elapsed >> tick_shift);
        unsigned int mhz = cpuspeed();
        unsigned int time = elapsed_shift / mhz;

        tprintf(&clockterm, "%2d ", current_running->pid);
        tprintf(&clockterm, "Clock: %d seconds", time);
        tprintf(&clockterm, ANSIF_EL "\r",
                ANSI_EFWD); // Clear right, then return

        print_pcb_table();

        yield();
    }
}

