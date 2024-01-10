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
#define KERNEL_PADDR 0x1000

/* Working stack for the bootblock and for kernel initialization */
#define STACK_PADDR 0x80000

#endif /* ADDRS_H */
