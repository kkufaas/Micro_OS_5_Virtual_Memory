#ifndef CPU_H
#define CPU_H

#include "hardware/cpu_x86.h"

enum privilege_level {
    PL_KERNEL = PL0,
    PL_USER   = PL3,
};

enum gdt_index {
    GI_NULL_SEG = 0, // x86 arch requires GDT to start with a null descriptor
    GI_KERNEL_CODE,
    GI_KERNEL_DATA,
    GI_USER_CODE,
    GI_USER_DATA,
    GI_TSS,
};

enum gdt_selector {
    KERNEL_CS = segment_selector(GI_KERNEL_CODE, TI_GDT, PL_KERNEL),
    KERNEL_DS = segment_selector(GI_KERNEL_DATA, TI_GDT, PL_KERNEL),
    PROCESS_CS = segment_selector(GI_USER_CODE, TI_GDT, PL_KERNEL),
    PROCESS_DS = segment_selector(GI_USER_DATA, TI_GDT, PL_KERNEL),
    KERNEL_TSS = segment_selector(GI_TSS, TI_GDT, PL_KERNEL),
};

void cpu_set_interrupt_stack(uintptr_t esp0);

void init_cpu(void);

#endif /* CPU_H */
