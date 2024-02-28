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
#include "keyboard.h"
#include "mbox.h"
#include "memory.h"
#include "pcb.h"
#include "scheduler.h"
#include "sync.h"
#include "syscall.h"
#include "time.h"
#include "usb/scsi.h"
#include "usb/usb.h"

/* Kernel threads */
#include "th1.h"
#include "th2.h"
#include "barrier_test.h"
#include "philosophers.h"

/* Kernel threads to start automatically */
static uintptr_t start_thrds[] = {
        (uintptr_t) loader_thread, /* Loads shell */
        (uintptr_t) clock_thread,  /* Running indefinitely */
        (uintptr_t) usb_thread,    /* Scans USB hub port */
        (uintptr_t) lock_thread0,  /* Test thread */
        (uintptr_t) lock_thread1,  /* Test thread */

        (uintptr_t) phl_thread0,   (uintptr_t) phl_thread1,
        (uintptr_t) phl_thread2,

        (uintptr_t) barrier1,      (uintptr_t) barrier2,
        (uintptr_t) barrier3,
};

static const int numthrds = sizeof(start_thrds) / sizeof(uintptr_t);

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

    /* Initialize various "subsystems" */
    time_init();
    init_memory();
    mbox_init();
    keyboard_init();
    scsi_static_init();
    usb_static_init();

    /* Create the threads */
    for (int i = 0; i < numthrds; i++) {
        create_thread(start_thrds[i]);
    }

    /*
     * Select the page directory of the first thread in the
     * ready queue. Then enable paging before dispatching
     */
    set_page_directory(current_running->page_directory);
    enable_paging();

    /* Start the first thread */
    pr_info("Beginning task dispatch...\n");
    dispatch();

    /* Should never be reached */
    pr_error("Ran out of tasks and returned to kernel main.\n");
    abortk();
}

