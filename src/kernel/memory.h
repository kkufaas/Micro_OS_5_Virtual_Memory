#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

#include <syslib/addrs.h>
#include <syslib/common.h>

#include "hardware/cpu_x86.h"
#include "pcb.h"

enum {
    /* physical page facts */
    PAGE_N_ENTRIES   = (PAGE_SIZE / sizeof(uint32_t)),
    SECTORS_PER_PAGE = (PAGE_SIZE / SECTOR_SIZE),

    PTABLE_SPAN = (PAGE_SIZE * PAGE_N_ENTRIES),

    /* page directory/table entry bits (PMSA p.235 and p.240) */
    PE_P              = 1 << 0,     /* present */
    PE_RW             = 1 << 1,     /* read/write */
    PE_US             = 1 << 2,     /* user/supervisor */
    PE_PWT            = 1 << 3,     /* page write-through */
    PE_PCD            = 1 << 4,     /* page cache disable */
    PE_A              = 1 << 5,     /* accessed */
    PE_D              = 1 << 6,     /* dirty */
    PE_BASE_ADDR_BITS = 12,         /* position of base address */
    PE_BASE_ADDR_MASK = 0xfffff000, /* extracts the base address */

    /* Useful sizes, bit-sizes and masks */
    PAGE_DIRECTORY_BITS = 22,         /* position of page dir index */
    PAGE_TABLE_BITS     = 12,         /* position of page table index */
    PAGE_DIRECTORY_MASK = 0xffc00000, /* page directory mask */
    PAGE_TABLE_MASK     = 0x003ff000, /* page table mask */
    PAGE_MASK           = 0x00000fff, /* page offset mask */
    /* used to extract the 10 lsb of a page directory entry */
    MODE_MASK = 0x000003ff,

    PAGE_TABLE_SIZE = (1024 * 4096 - 1), /* size of a page table in bytes */
};

/* Initialize the memory system, called from kernel.c: _start() */
void init_memory(void);

/* Set up a page directory and page table for the process. */
void setup_process_vmem(pcb_t *p);

/* Allocate contiguous bytes of memory aligned to a page boundary */
uintptr_t alloc_memory(size_t bytes);

#endif /* MEMORY_H */
