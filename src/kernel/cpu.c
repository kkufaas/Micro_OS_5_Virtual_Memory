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
};

struct pseudo_descriptor gdt_desc = {
        .base_addr = gdt,
        .limit     = sizeof(gdt),
};

void init_cpu(void)
{
}

