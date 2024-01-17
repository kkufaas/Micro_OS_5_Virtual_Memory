#include "kernel.h"

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
#include "cpu.h"
#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"
#include "pcb.h"
#include "scheduler.h"
#include "syscall.h"

/* Kernel threads */
#include "th1.h"
#include "th2.h"
#include "th3.h"

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

    init_pcb_table();

    /* TODO: Initialize your PCB table with tasks to execute and then
     * begin running them. */
    todo_abort();
}

