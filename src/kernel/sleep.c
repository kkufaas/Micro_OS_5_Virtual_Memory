/*
 * Contains functions to allow processes to sleep for some number of
 * milliseconds.
 */

#include "sleep.h"

#include <util/util.h>

#include "interrupt.h"
#include "scheduler.h"
#include "time.h"

void msleep(uint32_t msecs)
{
    uint64_t time;

    time                         = read_cpu_ticks();
    current_running->wakeup_time = time + (msecs * cpu_mhz * 1000);
    current_running->status      = STATUS_SLEEPING;
    yield();
    /*
     * When we return here, we will have waited atleast <msecs>
     * milliseconds.
     */
}
