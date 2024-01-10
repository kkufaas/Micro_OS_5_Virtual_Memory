#ifndef CPU_H
#define CPU_H

#include "hardware/cpu_x86.h"

enum privilege_level {
    /* All code runs at kernel level for now. */
    PL_KERNEL = PL0,
};

enum gdt_index {
    GI_NULL_SEG = 0, // x86 arch requires GDT to start with a null descriptor
    GI_KERNEL_CODE,
    GI_KERNEL_DATA,
};

enum gdt_selector {
    KERNEL_CS = segment_selector(GI_KERNEL_CODE, TI_GDT, PL_KERNEL),
    KERNEL_DS = segment_selector(GI_KERNEL_DATA, TI_GDT, PL_KERNEL),
};

void init_cpu(void);

#endif /* CPU_H */
