/*
 * Note:
 * There is no separate swap area. When a data page is swapped out,
 * it is stored in the location it was loaded from in the process'
 * image. This means it's impossible to start two processes from the
 * same image without screwing up the running. It also means the
 * disk image is read once. And that we cannot use the program disk.
 */
/*
 * This code currently has nothing to do with the process of paging to
 * disk. It only contains code to set up a page directory and page
 * table for a new process and for the kernel and kernel threads. The paging
 * mechanism is used for protection.
 *
 * Note:
 * This code assumes that the kernel can be mapped fully within the first
 * page table, and that any process only spans a single page table (4 MB).
 */

#define pr_fmt(fmt) "vmem: " fmt

#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <syslib/addrs.h>
#include <syslib/common.h>
#include <syslib/compiler_compat.h>
#include <util/util.h>

#include "lib/assertk.h"
#include "lib/printk.h"
#include "scheduler.h"
#include "sync.h"

#define UNUSED(x) ((void) x)

/* === Simple memory allocation === */

static uintptr_t  next_free_mem;
static spinlock_t next_free_mem_lock = SPINLOCK_INIT;

/*
 * Allocate contiguous bytes of memory aligned to a page boundary
 *
 * Note: if size < 4096 bytes, then 4096 bytes are used, beacuse the
 * memory blocks must be aligned to a page boundary.
 */
uintptr_t alloc_memory(size_t bytes)
{
    uint32_t ptr;

    spinlock_acquire(&next_free_mem_lock);
    ptr = next_free_mem;
    next_free_mem += bytes;

    if ((next_free_mem & 0xfff) != 0) {
        /* align next_free_mem to page boundary */
        next_free_mem = (next_free_mem & 0xfffff000) + 0x1000;
    }

    uintptr_t next_free_copy = next_free_mem;
    spinlock_release(&next_free_mem_lock);

    pr_debug(
            "Allocated %08x, %5d bytes. Next %08x. Max %08x\n", ptr, bytes,
            next_free_copy, PAGING_AREA_MAX_PADDR
    );
    assertf(next_free_copy < PAGING_AREA_MAX_PADDR, "Memory exhausted!");
    return ptr;
}

void free_memory(uint32_t ptr)
{
    spinlock_acquire(&next_free_mem_lock);

    /* do nothing now, we are not reclaiming */
    UNUSED(ptr);

    spinlock_release(&next_free_mem_lock);
}

/* === Page tables === */

/* Use virtual address to get index in page directory.  */
inline uint32_t get_directory_index(uint32_t vaddr)
{
    return (vaddr & PAGE_DIRECTORY_MASK) >> PAGE_DIRECTORY_BITS;
}

/*
 * Use virtual address to get index in a page table.  The bits are
 * masked, so we essentially get a modulo 1024 index.  The selection
 * of which page table to index into is done with
 * get_directory_index().
 */
inline uint32_t get_table_index(uint32_t vaddr)
{
    return (vaddr & PAGE_TABLE_MASK) >> PAGE_TABLE_BITS;
}

/*
 * Maps a page as present in the page table.
 *
 * 'vaddr' is the virtual address which is mapped to the physical
 * address 'paddr'.
 *
 * 'mode' sets bit [12..0] in the page table entry.
 *
 * If user is nonzero, the page is mapped as accessible from a user
 * application.
 */
static inline void
table_map_page(uint32_t *table, uint32_t vaddr, uint32_t paddr, uint32_t mode)
{
    int index    = get_table_index(vaddr);
    table[index] = (paddr & PE_BASE_ADDR_MASK) | (mode & ~PE_BASE_ADDR_MASK);
}

/*
 * Insert a page table entry into the page directory.
 *
 * 'vaddr' is the virtual address which is mapped to the physical
 * address 'paddr'. 'mode' sets bit [12..0] in the page table entry.
 */
inline void
dir_ins_table(uint32_t *directory, uint32_t vaddr, void *table, uint32_t mode)
{
    uint32_t access = mode & MODE_MASK;
    uint32_t taddr  = (uint32_t) table;
    int      index  = get_directory_index(vaddr);

    directory[index] = (taddr & PE_BASE_ADDR_MASK) | access;
}

/* === Page allocation tracking === */

static lock_t page_map_lock = LOCK_INIT;

/* === Page allocation === */

/*
 * Returns a pointer to a freshly allocated page in physical
 * memory. The address is aligned on a page boundary.  The page is
 * zeroed out before the pointer is returned.
 */
static uint32_t *allocate_page(void)
{
    uint32_t *page = (uint32_t *) alloc_memory(PAGE_SIZE);
    int       i;

    for (i = 0; i < 1024; page[i++] = 0)
        ;

    return page;
}

/* === Kernel and process setup === */

/* address of the kernel page directory (shared by all kernel threads) */
static uint32_t *kernel_pdir;

/*
 * This sets up mapping for memory that should be shared between the
 * kernel and the user process. We need this since an interrupt or
 * exception doesn't change to another page directory, and we need to
 * have the kernel mapped in to handle the interrupts. So essentially
 * the kernel needs to be mapped into the user process address space.
 *
 * The user process can't access the kernel internals though, since
 * the processor checks the privilege level set on the pages and we
 * only set USER privileges on the pages the user process should be
 * allowed to access.
 *
 * Note:
 * - we identity map the pages, so that physical address is
 *   the same as the virtual address.
 *
 * - The user processes need access video memory directly, so we set
 *   the USER bit for the video page if we make this map in a user
 *   directory.
 */
static void make_common_map(uint32_t *page_directory, int user)
{
    uint32_t *page_table, addr;

    unsigned int kernel_mode = PE_P | PE_RW;
    unsigned int user_mode   = PE_P | PE_RW | (user ? PE_US : 0);

    /* Allocate memory for the page table  */
    page_table = allocate_page();

    /* Identity map the first 640KB of base memory */
    for (addr = 0; addr < 640 * 1024; addr += PAGE_SIZE)
        table_map_page(page_table, addr, addr, kernel_mode);

    /* Identity map the video memory, from 0xb8000-0xb8fff. */
    table_map_page(
            page_table, (uint32_t) VGA_TEXT_PADDR, (uint32_t) VGA_TEXT_PADDR,
            user_mode
    );

    /*
     * Identity map in the rest of the physical memory so the
     * kernel can access everything in memory directly.
     */
    for (addr = PAGING_AREA_MIN_PADDR; addr < PAGING_AREA_MAX_PADDR;
         addr += PAGE_SIZE)
        table_map_page(page_table, addr, addr, kernel_mode);

    /*
     * Insert in page_directory an entry for virtual address 0
     * that points to physical address of page_table.
     */
    dir_ins_table(page_directory, 0, page_table, user_mode);
}

/*
 * Set up the page directory for the kernel and kernel threads.  Create kernel
 * page table, and identity map the first 0-640KB, Video RAM and
 * PAGING_AREA_MIN_PADDR-PAGING_AREA_MAX_PADDR, with kernel access privileges.
 */
static void setup_kernel_vmem(void)
{
    /*
     * Allocate memory for the page directory. A page directory
     * is exactly the size of one page.
     */
    kernel_pdir = allocate_page();

    /* This takes care of all the mapping that the kernel needs  */
    make_common_map(kernel_pdir, 0);
}

/*
 * Allocates and sets up the page directory and any necessary page
 * tables for a given process or thread.
 *
 * Note: We assume that the user code and data for the process fits
 * within a single page table (though not the same as the one the
 * kernel is mapped into). This works for project 4.
 *
 */
void setup_process_vmem(pcb_t *p)
{
    /*
     * Two page tables are created,
     * one for kernel memory (v.addr: 0..PAGING_AREA_MAX_PADDR)
     * and one for process memory (v.addr: PROCESS_VADDR..).
     *
     * - 0..640KB, and PAGING_AREA_MIN_PADDR..PAGING_AREA_MAX_PADDR is identity
     *   mapped with kernel access privileges. Video RAM is identity mapped
     * with user aceess privileges.
     *
     * - PROCESS_VADDR..(PROCESS_VADDR + process size) is mapped into the
     *   allocated base..(base + limit), with user privileges.
     *
     * - Finally p->page_directory points to the newly created page directory.
     */

    uint32_t *page_directory, *page_table, offset, paddr, vaddr;

    lock_acquire(&page_map_lock);

    if (p->is_thread) {
        /*
         * Threads use the kernels page directory, so just set
         * a pointer to that one and return.
         */
        p->page_directory = kernel_pdir;
        lock_release(&page_map_lock);
        return;
    }
    /*
     * User process. Allocate memory for page directory map in the
     * address space we share with the kernel.
     */
    page_directory = allocate_page();
    make_common_map(page_directory, 1);

    unsigned int user_mode = PE_P | PE_RW | PE_US;

    /* Now we need to set up for the user processes address space. */
    page_table = allocate_page();
    dir_ins_table(page_directory, PROCESS_VADDR, page_table, user_mode);

    /*
     * Map in the image that we loaded in from disk. limit is the
     * size of the process image + stack in bytes
     */
    for (offset = 0; offset < p->limit; offset += PAGE_SIZE) {
        paddr = p->base + offset;
        vaddr = PROCESS_VADDR + offset;
        table_map_page(page_table, vaddr, paddr, user_mode);
    }
    p->page_directory = page_directory;

    lock_release(&page_map_lock);
}

/*
 * Called by _start() Must be called before we can allocate memory,
 * and before we call any "paging" function.
 */
void init_memory(void)
{
    next_free_mem = PAGING_AREA_MIN_PADDR;
    setup_kernel_vmem();
}

