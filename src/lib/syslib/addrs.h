/*
 * Central location for important memory addresses
 */
#ifndef ADDRS_H
#define ADDRS_H

#define TODO_ADDR(tmp) tmp

/* === System-defined physical addresses === */

#define BOOTBLOCK_ORIG_PADDR 0x7c00 // BIOS loads bootblock here

#define VGA_TEXT_PADDR 0xb8000 // Memory map of the VGA text-mode display
#define VGA_TEXT_ROWS  25
#define VGA_TEXT_COLS  80

/* === OS-defined physical addresses === */

/* Where to load the kernel */
#define KERNEL_PADDR 0x8000

/* Working stack for the bootblock and for kernel initialization */
#define STACK_PADDR 0x80000

/* Physical area reserved for allocatinge kernel-level stacks for threads */
#define T_KSTACK_AREA_MIN_PADDR 0x40000
#define T_KSTACK_AREA_MAX_PADDR 0x80000
#define T_KSTACK_SIZE_EACH      0x2000
#define T_KSTACK_START_OFFSET   0x1ffc

/*
 * Physical memory area used for allocating virtual memory pages
 *
 * From this simple operating system's perspective, modern PCs basically have
 * unlimited memory, so we have to artificially limit the amount of memory
 * available if we want to see swapping in action.
 */
#define PAGE_SIZE 0x1000 // aka 4096 aka 4 KiB
#define PAGEABLE_PAGES 33
//#define PAGEABLE_PAGES 34
// #define PAGEABLE_PAGES 44
// #define PAGEABLE_PAGES        500
#define PAGING_AREA_MIN_PADDR 0x100000 /* 1MB */
#define PAGING_AREA_MAX_PADDR \
    (PAGING_AREA_MIN_PADDR + PAGE_SIZE * PAGEABLE_PAGES)

/* === OS-defined virtual addresses === */

#define PROCESS_VADDR       0x1000000
#define PROCESS_STACK_VADDR 0xeffffff0

/* === Interrupt vectors === */

/* Vectors 0--31 are reserved for the CPU itself. */

/*
 * CPU interrupt vector for hardware IRQ 0
 *
 * External hardware interrupt requests (IRQs) are numbered in relation to the
 * lines coming in to the interrupt controller. These IRQ numbers have to be
 * mapped to CPU interrupt vector numbers.
 *
 * This constant controls that mapping.
 */
#define IVEC_IRQ_0 32 /* Vectors 32--47 will be IRQs 0--15 */

/* CPU interrupt vector for system calls */
#define IVEC_SYSCALL 48

/* Size of Interrupt Desscriptor Table (end of used interrupt vectors) */
#define IDT_SIZE 49

#endif /* ADDRS_H */
