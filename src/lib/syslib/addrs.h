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

/* The bootblock should relocate itself to this address,
 * in case 0x7c00 gets clobbbered. */
#define BOOTBLOCK_RELO_PADDR 0x0e00 // After Real-Mode interrupt table.

/* Where to load the kernel */
#define KERNEL_PADDR 0x1000

/* Where to load processes */
#define PROC1_PADDR 0x10000
#define PROC2_PADDR 0x14000

/* Working stack for the bootblock and for kernel initialization */
#define STACK_PADDR 0x80000

/* Physical area reserved for allocatinge kernel-level stacks for threads */
#define T_KSTACK_AREA_MIN_PADDR 0x20000
#define T_KSTACK_AREA_MAX_PADDR 0x80000
#define T_KSTACK_SIZE_EACH      0x1000
#define T_KSTACK_START_OFFSET   0x0ffc

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
