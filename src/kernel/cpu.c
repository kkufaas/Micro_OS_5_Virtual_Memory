#include "cpu.h"

#include "hardware/cpu_x86.h"

/*
 * Expands to a code/data segment descriptor initialization
 *
 * - full address space: base = 0, limit = max, granularity bit (G) set
 * - present: P bit set
 * - 32-bit: D bit set
 */
#define data_sd(dpl, flags) \
    sd_init(0x00000, 0xfffff, dpl, SD_G | SD_P | SD_DB | flags)

/*
 * Global Descriptor Table (GDT)
 *
 * We initialize the GDT at compile time so that it will be available right
 * away with no processing needed. The _start code will install this GDT and
 * set initialize segment registers before transferring control to kernel_main.
 */
static struct descriptor gdt[] = {
        [GI_NULL_SEG]    = {}, // Required null selector
        [GI_KERNEL_CODE] = data_sd(PL_KERNEL, SD_CODE | CS_R),
        [GI_KERNEL_DATA] = data_sd(PL_KERNEL, SD_DATA | DS_W),
        [GI_USER_CODE] = data_sd(PL_USER, SD_CODE | CS_R),
        [GI_USER_DATA] = data_sd(PL_USER, SD_DATA | DS_W),
        [GI_TSS]       = {}, // To be initialized dynamically
};

/*
 * Pseudo-descriptor used to load the GDT address into the GDT register
 *
 * Like the GDT itself, we initialize this at compile time so that startup
 * assembly code can use it right away.
 */
struct pseudo_descriptor gdt_desc = {
        .base_addr = gdt,
        .limit     = sizeof(gdt),
};

/* === Task-State Segment (TSS) === */

/*
 * Task-State Segment (TSS)
 */
struct tss tss;

/*
 * Initialize task state segment. This OS only uses one task state
 * segment, so the backlink points back at itself. We don't use the
 * processors multitasking mechanism.
 *
 * We use the tss to switch from the user stack to the kernel stack.
 * This happens only when a process running in privilege level 3  is
 * interrupted and enters the kernel (which has privilege level 0).
 * See the description init_idt().
 *
 * The pointer to the kernel stack (tss.ss0:tss.esp0) is set every time a
 * process is dispatched.
 */
void init_tss(void)
{
    tss = (struct tss){
            // .backlink   = KERNEL_TSS,
            // .iomap_base = sizeof(tss),
    };

    gdt[GI_TSS] = sd_init(&tss, sizeof(tss), PL_KERNEL, SD_P | SD_TSS);
    ltr(KERNEL_TSS);
}

/*
 * Choose the kernel-level stack to switch to on interrupt
 *
 * This is what the TSS is actually used for: choosing which stack to switch to
 * on interrupt. When an interrupt causes a privilege level change (from user
 * to kernel), the CPU loads stack information from the TSS and automatically
 * switches to that stack.
 *
 * This function sets that stack pointer ahead of time.
 */
void cpu_set_interrupt_stack(uintptr_t esp0)
{
    tss.ss0  = KERNEL_DS;
    tss.esp0 = esp0;
}

void init_cpu(void)
{
    /* The static GDT in this file has already been installed by the
     * startup code in kernel_start.S, so no further initialization is
     * needed for that. */

    /* We still need to initialize the Task-State Segment. */
    init_tss();
}

