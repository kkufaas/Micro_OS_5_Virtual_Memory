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
#include "lib/assertk.h"
#include "lib/printk.h"
#include "cpu.h"

/* === Kernel main === */

void kernel_main(void)
{
    /* Ensure interrupts are disabled for now. */
    interrupts_disable();

    struct term whole_screen = TERM_INIT_VGA_FULL;
    tprintf(&whole_screen, ANSIF_ED, ANSI_EFULL);

    pr_info("Kernel starting...\n");
    init_cpu();

    pr_info("Kernel started. Nothing to do.\n");
    abortk();
}

