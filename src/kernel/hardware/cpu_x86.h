/*
 * Abstractions for x86 CPUs
 *
 * This file provides simple C abstractions for x86 CPUs and their in-memory
 * data structures, such as segment descriptors and interrupt gates.
 *
 * References:
 *
 *  - "Intel" refers to:
 *
 *      _Intel® 64 and IA-32 Architecture Software Developer's Manuals_
 *      <https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html>
 *
 *      This file was written against the December 2022 version.
 *      Section numbers may have changed since then.
 *
 *  - "AMD" refers to:
 *
 *      _AMD64 Architecture Programmer's Manual_
 *
 *      Manuals can be downloaded through AMD's "documentation hub" by
 *      searching for "AMD64" and filtering results by document type
 *      "programmer references" and product type "processors"
 *      <https://www.amd.com/en/search/documentation/hub.html>
 *
 *      This file was written against the Januare 2023 version, Rev. 3.40,
 *      version id 24593. Section numbers may have changed since then.
 */

#ifndef _CPU_X86_H
#define _CPU_X86_H

#include <stdbool.h>
#include <stdint.h>

/* An unsigned integer type with the size of a general-purpose register. */
typedef uint32_t ureg_t;

enum x86_privilege_level {
    PL0 = 0, // System privilege level, intended for kernel code
    PL1 = 1, // Intermediate level 1, intended for drivers, but rarely used
    PL2 = 2, // Intermediate level 2, intended for drivers, but rarely used
    PL3 = 3, // Least privileged level, intended for user/program code
};

/* === Segment/Gate Descriptors === */

/*
 * Segment/Gate Descriptor
 *
 * See:
 *  - AMD, Vol 2, §4.7, Legacy Segment Descriptors
 *  - Intel, Vol 3, §3.4.5, Segment Descriptors
 *  - Intel, Vol 3, §5.8.3, Call Gates
 *  - Intel, Vol 3, §6.11, IDT Descriptors
 *
 * AMD's manual has documentation for all descriptors grouped together in one
 * section, which may be more convenient for comparing.
 */
struct __attribute__((packed, aligned(8))) descriptor {
    uint16_t low_w;   // Segment: limit[15:0], Gate: offset[15:0]
    uint16_t high_w;  // Segment: base[15:0], Gate: segment selector
    uint32_t high_dw; // Segment: base + limit + flags, Gate: offset + flags
};

/* Bit masks used in the high doubleword of a segment/gate descriptor */
enum descriptor_high_dw_bits {
    /* Fields for Segment Descriptors */
    //                 10987654321098765432109876543210
    SD_BASE_31_24  = 0b11111111000000000000000000000000,
    SD_FLAGS       = 0b00000000111100001001111100000000,
    SD_DPL         = 0b00000000000000000110000000000000,
    SD_LIMIT_19_16 = 0b00000000000011110000000000000000,
    SD_BASE_23_16  = 0b00000000000000000000000011111111,

    /* Fields for Gate Descriptors */
    //                  10987654321098765432109876543210
    GD_OFFSET_31_16 = 0b11111111111111110000000000000000,
    GD_FLAGS        = 0b00000000000000001000000000000000,
    GD_DPL          = 0b00000000000000000110000000000000,
    GD_PARAM_COUNT  = 0b00000000000000000000000000011111,

    /* Flags */
    SD_G   = 1 << 23, // Granularity of limit (0: bytes, 1: 4 KiB pages)
    SD_DB  = 1 << 22, // Default operand size (0: 16-bit, 1: 32-bit)
    SD_L   = 1 << 21, // 64-bit code segment
    SD_AVL = 1 << 20, // Available to the OS
    SD_P   = 1 << 15, // Present
    SD_S   = 1 << 12, // System (0: system segment, 1: code/data segment)

    /* Type fields for code-segment descriptors */
    SD_CODE = 1 << 11 | SD_S, // Code SDs are non-system, with bit 11 set
    CS_C    = 1 << 10,        // Conforming
    CS_R    = 1 << 9,         // Readable
    CS_A    = 1 << 8,         // Accessed

    /* Type fields for code-segment descriptors */
    SD_DATA = 0 << 11 | SD_S, // Data SDs are non-system, with bit 11 clear
    DS_E    = 1 << 10,        // Expand-down
    DS_W    = 1 << 9,         // Writeable
    DS_A    = 1 << 8,         // Accessed

    /* Type fields for task-state-segment descriptors */
    SD_TSS = 0b1001 << 8, // TSS SDs are system, with type field of 0b10B1
    TSS_B  = 1 << 9,      // Busy flag

    /* Type fields for gate descriptors */
    GD_TASK    = 0b0101 << 8,
    GD_CALL_32 = 0b1100 << 8,
    GD_INT_32  = 0b1110 << 8,
    GD_TRAP_32 = 0b1111 << 8,
};

/*
 * Expands to a struct initialization for a segment descriptor
 *
 * This macro takes its inputs as normal values and shifts their bits around
 * to populate a segment decriptor struct.
 *
 * If all inputs are constants, this expansion is also a constant expression
 * which can be evaluated at compile time.
 */
#define sd_init(base, limit, dpl, flags) \
    (struct descriptor) \
    { \
        .low_w   = (uint16_t) (limit) &0xffff, \
        .high_w  = (uint16_t) ((uint32_t) base) & 0xffff, \
        .high_dw = (uint32_t) (((uint32_t) base) & SD_BASE_31_24) \
                   | ((flags) &SD_FLAGS) | ((limit) &SD_LIMIT_19_16) \
                   | ((dpl) << 13 & SD_DPL) \
                   | (((uint32_t) base) >> 16 & SD_BASE_23_16), \
    }

/* === Segment Descriptor Table Registers === */

/*
 * Pseudo-descriptor used to load/store descriptor table registers
 *
 * See: Intel, Vol 3, §3.5.1, Segment Descriptor Tables
 */
struct __attribute__((packed)) pseudo_descriptor {
    uint16_t           limit;
    struct descriptor *base_addr;
};

static inline void lgdt(struct pseudo_descriptor *desc)
{
    asm volatile("lgdt %0" ::"m"(*desc));
}

static inline void lidt(struct pseudo_descriptor *desc)
{
    asm volatile("lidt %0" ::"m"(*desc));
}

/* === Segment Selectors === */

enum table_indicator {
    TI_GDT = 0,
    TI_LDT = 1,
};

/*
 * Segment Selector
 *
 * See: Intel, Vol 3, § 3.4.2, Segment Selectors
 */
typedef uint16_t segment_selector_t;

/* Expands into an integer expression for a segment selector */
#define segment_selector(index, table_indicator, requested_priv_level) \
    ((index << 3) | (table_indicator << 2) | requested_priv_level)

/* Expands to a function that loads a specific data segment register */
#define decl_load_seg_fn(NAME, REG) \
    static inline void NAME(const segment_selector_t s) \
    { \
        asm inline volatile("mov %0, %%" #REG ::"r"(s)); \
    }

decl_load_seg_fn(load_ds, ds);
decl_load_seg_fn(load_es, es);
decl_load_seg_fn(load_fs, fs);
decl_load_seg_fn(load_gs, gs);
decl_load_seg_fn(load_ss, ss);

#undef decl_load_seg_fn

static inline void load_cs(const segment_selector_t s)
{
    /* Set CS by doing a long jump to the next instruction.
     *
     * The "%=" format string outputs a number that we can use as a unique
     * local label. When compiled, this asm will look something like this:
     *
     *  	ljmp $8, $(120f)	# Set CS via ljmp to next instruction
     *  120:				# ljmp target
     *
     * See:
     * https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#Special-format-strings
     */
    asm inline volatile(
            "ljmp %0, $(%=f)	# Set CS via ljmp to next instruction\n"
            "%=:				# ljmp target\n"
            :
            : "n"(s)
    );
}

/* === Interrupts === */

#define interrupts_disable() asm inline volatile("cli")
#define interrupts_enable()  asm inline volatile("sti")

/*
 * LTR: Load Task Register
 */
static inline void ltr(segment_selector_t selector)
{
    asm volatile("ltr %0" ::"mr"(selector));
}

/* === I/O === */

typedef uint16_t ioport_t;

#define decl_in_fn(NAME, INT_TYPE) \
    static inline INT_TYPE NAME(ioport_t port) \
    { \
        INT_TYPE ret; \
        asm inline volatile( \
                "in	%[port],	%[val]\n" \
                : [val] "=a"(ret)   /* A register */ \
                : [port] "Nd"(port) /* D register or immediate byte */ \
        ); \
        return ret; \
    }

#define decl_out_fn(NAME, INT_TYPE) \
    static inline void NAME(ioport_t port, INT_TYPE data) \
    { \
        asm inline volatile( \
                "out	%[val],	%[port]\n" \
                : \
                : [val] "a"(data),  /* A register */ \
                  [port] "Nd"(port) /* D register or immediate byte */ \
        ); \
    }

decl_in_fn(inb, uint8_t);
decl_in_fn(inw, uint16_t);
decl_in_fn(inl, uint32_t);

decl_out_fn(outb, uint8_t);
decl_out_fn(outw, uint16_t);
decl_out_fn(outl, uint32_t);

#undef decl_in_fn
#undef decl_out_fn

/* === Misc === */

static inline void cpu_halt() { asm inline volatile("hlt"); }

#endif /* _CPU_X86_H */
