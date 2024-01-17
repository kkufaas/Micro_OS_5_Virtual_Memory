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

/* Syscalls via agreed-upon fixed memory location.
 * This location will hold a pointer to the kernel's syscall entry function. */
#define SYSCALL_ENTRY_FN_PTR_PADDR 0xf00

/* The bootblock should relocate itself to this address,
 * in case 0x7c00 gets clobbbered. */
#define BOOTBLOCK_RELO_PADDR 0x0e00 // After Real-Mode interrupt table.

/* Where to load the kernel */
#define KERNEL_PADDR 0x1000

/* Where to load processes */
#define PROC1_PADDR 0x6000
#define PROC2_PADDR 0xa000

/* Working stack for the bootblock and for kernel initialization */
#define STACK_PADDR 0x80000

/* Physical area reserved for allocatinge kernel-level stacks for threads */
#define T_KSTACK_AREA_MIN_PADDR 0x20000
#define T_KSTACK_AREA_MAX_PADDR 0x80000
#define T_KSTACK_SIZE_EACH      0x1000
#define T_KSTACK_START_OFFSET   0x0ffc

#endif /* ADDRS_H */
