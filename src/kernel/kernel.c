#include <stdalign.h>
#include <string.h>

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/addrs.h>
#include <syslib/common.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "hardware/cpu_x86.h"
#include "hardware/intctl_8259.h"
#include "cpu.h"
#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"
#include "interrupt.h"
#include "pcb.h"
#include "scheduler.h"
#include "sync.h"
#include "syscall.h"
#include "time.h"

/* Kernel threads */
#include "th1.h"
#include "th2.h"
#include "th3.h"
#include "barrier_test.h"
#include "philosophers.h"

/* Kernel threads to start automatically */
static uintptr_t start_thrds[] = {
        (uintptr_t) clock_thread,  /* Running indefinitely */
        (uintptr_t) lock_thread0,  /* Test thread */
        (uintptr_t) lock_thread1,  /* Test thread */
        (uintptr_t) mcpi_thread0,  (uintptr_t) mcpi_thread1,
        (uintptr_t) mcpi_thread2,  (uintptr_t) mcpi_thread3,

        (uintptr_t) phl_thread0,   (uintptr_t) phl_thread1,
        (uintptr_t) phl_thread2,

        (uintptr_t) barrier1,      (uintptr_t) barrier2,
        (uintptr_t) barrier3,
};

/* User processes to start automatically */
static uintptr_t start_procs[] = {
        (uintptr_t) PROC1_PADDR, /* given in Makefile */
        (uintptr_t) PROC2_PADDR,
};

static const int numthrds = sizeof(start_thrds) / sizeof(uintptr_t);
static const int numprocs = sizeof(start_procs) / sizeof(uintptr_t);

/* === Kernel main === */

void kernel_main(void)
{
    /* Ensure interrupts are disabled for now. */
    interrupts_disable();

    struct term whole_screen = TERM_INIT_VGA_FULL;
    tprintf(&whole_screen, ANSIF_ED, ANSI_EFULL);

    pr_info("Kernel starting...\n");
    init_cpu();

    init_syscalls();

    init_idt();
    init_int_controller();
    init_pit();

    init_pcb_table();

    time_init();

    /* Create the threads */
    for (int i = 0; i < numthrds; i++) {
        create_thread(start_thrds[i]);
    }
    /* Create the processes */
    for (int i = 0; i < numprocs; i++) {
        create_process(start_procs[i]);
    }

    /* Start the first thread */
    pr_info("Beginning task dispatch...\n");
    dispatch();

    /* Should never be reached */
    pr_error("Ran out of tasks and returned to kernel main.\n");
    abortk();
}

