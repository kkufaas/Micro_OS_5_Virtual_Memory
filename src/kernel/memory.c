/*
 * Note:
 * There is no separate swap area. When a data page is swapped out,
 * it is stored in the location it was loaded from in the process'
 * image. This means it's impossible to start two processes from the
 * same image without screwing up the running. It also means the
 * disk image is read once. And that we cannot use the program disk.
 */

#define pr_fmt(fmt) "vmem: " fmt
#include "memory.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <syslib/addrs.h>
#include <syslib/common.h>
#include <syslib/compiler_compat.h>
#include <util/util.h>
#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"
#include "scheduler.h"
#include "sync.h"
#include "interrupt.h"
#include "usb/scsi.h"

#define UNUSED(x) ((void) x)
//#define KERNEL_SIZE 0x400000 // 4 MB in hexadecimal
#define KERNEL_SIZE 0x300000 // 3 MB in hexadecimal
static unsigned long next = 1; // Used in random function
#define MEMDEBUG 1
#define DEBUG_PAGEFAULT 1
#define MEM_DEBUG_LOADPROC 1
#define USER_STACK_SIZE 0x1000 

static pcb_t dummy_kernel_pcb[1]; 

enum {
    PE_INFO_USER_MODE              = 1 << 0,     /* user */
    PE_INFO_KERNEL_DUMMY           = 1 << 1,     /* kernel ''dummy'' */
    PE_INFO_PINNED                 = 1 << 2,     /* pinned */
};

/* === Simple memory allocation === */

static uintptr_t  next_free_mem;
static spinlock_t next_free_mem_lock = SPINLOCK_INIT;

static spinlock_t page_frame_info_lock = SPINLOCK_INIT;


inline uint32_t get_table_index(uint32_t vaddr);
uint32_t* get_page_table(uint32_t vaddr, uint32_t* page_directory);
bool is_page_dirty(uint32_t vaddr, uint32_t *page_directory);
uint32_t* try_evict_page();
 
///////////////////////////////////////////////////////
// similar (but static) datastructure as in INF1101
// google-clone exam to link physical and virtual adresses
//
// python size calculation for 4GB mem system:
// >>> GB = 1024**3
// >>> page_size = 4*1024
// >>> page_frame_info_size = 4*4 + 1
// >>> ((4) - (4*GB - (4*GB/page_size)*2*page_frame_info_size)/(GB))*(1024)
// 34.0 
//
// i.e, the current info will use ~34 MB of memory
// if everything is pageable and one assumes all memory
// is shareable --> considerable optimization still possible
///////////////////////////////////////////////////////

//
// idea from hashmaps: collisions only for physical
// pages shared between different processes
//

/* === Info structure to keep track on pages condition === */

struct page_frame_info {
    pcb_t *owner;

    // the next info frame like this sharing the same physical address
    struct page_frame_info *next_shared_info; 

    // if not taken - points to the next free page
    // if taken - is null.
    // free a page? index by physical address and insert into
    // into the linked list
    struct page_frame_info *next_free_page;

    uintptr_t *paddr;
    uintptr_t *vaddr;
    uint32_t info_mode;
};
typedef struct page_frame_info page_frame_info_t;

page_frame_info_t page_frame_info[PAGEABLE_PAGES];
page_frame_info_t page_frame_info_shared[PAGEABLE_PAGES];

page_frame_info_t *page_free_head;

#define set_frame_info(frame, ownerv, next_shared_infov, paddrv, vaddrv, info_modev) {\
    frame.owner = ownerv;\
    frame.next_shared_info = next_shared_infov;\
    frame.paddr = paddrv;\
    frame.info_mode = info_modev;\
}
void initialize_page_frame_infos(void) {
    uint32_t paddr = PAGING_AREA_MIN_PADDR;

    page_free_head = &page_frame_info[0];
    set_frame_info(page_frame_info[0], NULL, NULL, (uintptr_t *) paddr, NULL, 0);
    set_frame_info(page_frame_info_shared[0], NULL, NULL, NULL, NULL, 0);

    for (int i = 1; i < PAGEABLE_PAGES - 1; i++) {
        paddr += PAGE_SIZE;
        set_frame_info(page_frame_info[i], NULL, NULL, (uintptr_t *) (paddr), NULL, 0);
        set_frame_info(page_frame_info_shared[i], NULL, NULL, NULL, NULL, 0);

        page_frame_info[i - 1].next_free_page = &page_frame_info[i];
    }
}

/*
 * It is the responisibility of the caller to wrap this function between
 * spinlock_acquire(&page_frame_info_lock);
 * and
 * spinlock_release(&page_frame_info_lock);
 */
void add_page_frame_to_free_list_info(uintptr_t *paddress) {
    // Convert 'address' to its corresponding index in the free_pages array
    int index = ((uint32_t) paddress - PAGING_AREA_MIN_PADDR) / PAGE_SIZE;
    page_frame_info_t* page_frame = &page_frame_info[index];
    // Prepend this page_frame to the start of the free list
    page_frame_info->next_free_page = page_free_head;
    page_free_head = page_frame;
}

/*
 * It is the responisibility of the caller to wrap this function between
 * spinlock_acquire(&page_frame_info_lock);
 * and
 * spinlock_release(&page_frame_info_lock);
 */
page_frame_info_t* remove_page_frame_from_free_list_info() {
    if (page_free_head == NULL) {
        // No free page frames available
        return NULL;
    }
    // Take the first page frame from the free list
    page_frame_info_t* allocated_page_frame = page_free_head;

    // Move head to the next frame
    page_free_head = allocated_page_frame->next_free_page; 

    allocated_page_frame->next_free_page = NULL; 
    return allocated_page_frame;
}


//Not in use
// int is_pinned(page_frame_info_t *frame_info)
// {
//     return (frame_info -> info_mode) & PE_INFO_PINNED;
// }


/*
 * "Hashing" function carrying physical page addresses into the info struct array 
 */
uint32_t calculate_info_index(uintptr_t *paddr)
{
    return ((uint32_t)paddr - PAGING_AREA_MIN_PADDR)/PAGE_SIZE;
}


/*
 * ..
*/
page_frame_info_t*
insert_page_frame_info(uintptr_t *paddr, uintptr_t *vaddr, 
                        pcb_t *owner_pcb, uint32_t info_mode)
{
    uint32_t index = calculate_info_index(paddr);
    if (page_frame_info[index].owner) {

        pr_debug("shared page at physical address %u, already occupided by %u", 
                        (uint32_t) paddr, page_frame_info[index].owner -> pid);
        page_frame_info_t *next_info = &page_frame_info[index];
        
        page_frame_info_t *final_info = next_info;

        // find the last infostruct in the chain
        while (next_info) {
            if ( (next_info = next_info -> next_shared_info) ) {
                final_info = next_info;
            }
        }
        // try same index in the shared infostruct array 
        if (!page_frame_info_shared[index].owner) {
            page_frame_info_t *new_info = &page_frame_info_shared[index];
            final_info -> next_shared_info = new_info;
            new_info->owner = owner_pcb;
            new_info->paddr = paddr;
            new_info->vaddr = vaddr;
            new_info->info_mode = info_mode;
            return new_info;
        } else {
            int j = 0;
            for (int i = index; j < PAGEABLE_PAGES; i++, j++) {
                if (!page_frame_info_shared[i].owner) {
                    page_frame_info_t *new_info = &page_frame_info_shared[i];
                    final_info -> next_shared_info  = new_info;
                    new_info->owner = owner_pcb;
                    new_info->paddr = paddr;
                    new_info->vaddr = vaddr;
                    new_info->info_mode = info_mode;
                    return new_info;
                }
            }
            // no suitable slot for a page found
            return NULL;
        }
    } else {
        // page not occupied / page not shared
        page_frame_info_t *new_info = &page_frame_info[index];
        new_info->owner = owner_pcb;
        new_info->next_shared_info = NULL;
        new_info->vaddr = vaddr;
        new_info->info_mode = info_mode;
        return new_info;
    }
}




// Not in use
/*
 *  lookup all page dirs and page tables that
 *  contain references to the physical address paddr
 *  and set the mode
 */
// int traverse_pages(uintptr_t *paddr, uint32_t mode)
// {
//     uint32_t info_index = calculate_info_index(paddr);
//     page_frame_info_t *info = &page_frame_info[info_index];
//     // process vaddr of info
//     
// 
//     while (info -> next_shared_info) {
//         // process next_shared_info
//     }
//     //page_set_mode
// }


/*
 * Processes all page tables with a reference to the physical address paddr.
 * Checks if the pages are dirty
 * Returns -1 if paddr must not be evicted
 * Returns 0 when not dirty
 * Returns 1 when dirty
 */
int traverse_evict(uintptr_t *paddr) {
    int dirty = 0;
    uint32_t info_index = calculate_info_index(paddr);
    page_frame_info_t *info = &page_frame_info[info_index];
    uint32_t *current_pdir= info -> owner -> page_directory;
    uint32_t current_vaddr = (uint32_t) info -> vaddr;
    while (info -> next_shared_info) {
        // process next_shared_info
        // if (info -> owner == current_running) {
        // }
        // process vaddr of info
        // check if dirty
        if (! (info -> info_mode & PE_INFO_KERNEL_DUMMY) ) {
            dirty += is_page_dirty(current_vaddr, current_pdir);
        } else {
            // trying to evict kernel page dir
            // frame should be pinned and this should never happen!
            pr_error("Trying to evict kernel page directory \n");
            return -1;
        }
    }
    //page_set_mode
    return dirty;
}

/*
 * Allocate contiguous bytes of memory aligned to a page boundary
 *
 * Note: if size < 4096 bytes, then 4096 bytes are used, beacuse the
 * memory blocks must be aligned to a page boundary.
 * 
 * TODO: update so that if next_free_mem is exhausted it will look for
 * a page to evict.
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

/* 
 *  Allocates a page of memory by removing a page_info_frame from the list
 *  of free pages and returning a pointer of it to the user.
 */
page_frame_info_t* page_alloc()
{
    spinlock_acquire(&page_frame_info_lock);
    page_frame_info_t* page_info_frame = remove_page_frame_from_free_list_info();
    spinlock_release(&page_frame_info_lock);

    // zero out the page
    if (page_info_frame) {
        uintptr_t *paddr = page_info_frame -> paddr;
        for (int i = 0; i < PAGE_SIZE; i++) {
            *(paddr + i) = 0;
        }
    }
    return page_info_frame;
}

void page_free(uintptr_t *paddr) {
    page_frame_info_t *info_frame, *next_shared_info;

    spinlock_acquire(&page_frame_info_lock);
    add_page_frame_to_free_list_info(paddr);
    spinlock_release(&page_frame_info_lock);

    uint32_t info_index = ((uint32_t) paddr - PAGING_AREA_MIN_PADDR) / PAGE_SIZE;
    info_frame = &page_frame_info[info_index];

    // reset the frame to initial state
    do {
        info_frame -> owner = NULL;
        info_frame -> vaddr = NULL;
        info_frame -> info_mode = 0;

        // transfer to next shared info frame
        next_shared_info = info_frame -> next_shared_info;
        info_frame -> next_shared_info = NULL;
        info_frame = next_shared_info;
    }
    while (info_frame);
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
    invalidate_page((uint32_t *) vaddr);
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

/* Set 12 least significant bytes in a page table entry to 'mode' */
static inline void page_set_mode(uint32_t *pdir, uint32_t vaddr, uint32_t mode)
{
    uint32_t dir_index = get_directory_index((uint32_t) vaddr),
             index     = get_table_index((uint32_t) vaddr), dir_entry, *table,
             entry;
    dir_entry = pdir[dir_index];
    assertk(dir_entry & PE_P); /* dir entry present */
    table = (uint32_t *) (dir_entry & PE_BASE_ADDR_MASK);
    /* clear table[index] bits 11..0 */
    entry = table[index] & PE_BASE_ADDR_MASK;
    /* set table[index] bits 11..0 */
    entry |= mode & ~PE_BASE_ADDR_MASK;
    table[index] = entry;
    /* Flush TLB */
    invalidate_page((uint32_t *) vaddr);
}

/* Debug-function.
 * Write all memory addresses and values by with 4 byte increment to
 * output-file. Output-file name is specified in bochsrc-file by line: com1:
 * enabled=1, mode=file, dev=serial.out where 'dev=' is output-file.
 * Output-file can be changed, but will by default be in 'serial.out'.
 *
 * Arguments
 * title:              prefix for memory-dump
 * start:              memory address
 * end:                        memory address
 * inclzero:   binary; skip address and values where values are zero
 */
ATTR_UNUSED
static void
dump_memory(char *title, uint32_t start, uint32_t end, uint32_t inclzero)
{
    uint32_t numpage, paddr;
    pr_debug("%s\n", title);
    numpage = 0;
    for (paddr = start; paddr < end; paddr += sizeof(uint32_t)) {
        /* Print header if address is page-aligned. */
        if (paddr % PAGE_SIZE == 0) {
            pr_debug("=== PAGE NUMBER %02d ===\n", numpage);
            numpage++;
        }
        /* Avoid printing address entries with no value. */
        if (!inclzero && *(uint32_t *) paddr == 0x0) {
            continue;
        }
        /* Print:
         * Entry-number from current page.
         * Physical main memory address.
         * Value at address.
         */
        pr_debug(
                "%04d - Memory Loc: 0x%08x ~~~~~ Mem Val: 0x%08x\n",
                ((paddr - start) / sizeof(uint32_t)) % PAGE_N_ENTRIES, paddr,
                *(uint32_t *) paddr
        );
    }
}

/* === Page allocation tracking === */

static lock_t page_map_lock = LOCK_INIT;

/*
 * === Page allocation ===
 */

uint32_t* allocate_page(void) {
    uint32_t* page = (uint32_t*) alloc_memory(4096);
    for (int i = 0; i < 1024; i++) page[i] = 0; // Zero out the page
    return page;
}


/* === Kernel and process setup === */

/* address of the kernel page directory (shared by all kernel threads) */
static uint32_t *kernel_pdir;


/*
 * Sets up 
*/
static void setup_kernel_vmem_common(uint32_t *pdir, int is_user) {
    uint32_t user_mode = PE_P | PE_RW | PE_US; // Define access mode for user pages.
    uint32_t kernel_mode = PE_P | PE_RW; // Define access mode for kernel pages.

    page_frame_info_t *kernel_info_page = page_alloc();
    uint32_t *kernel_ptable = kernel_info_page -> paddr;
    //uint32_t *kernel_ptable = allocate_page();
    // Allocate and "pin" the page table to ensure it's not swapped out.
    // uint32_t kernel_ptable_number = ((uintptr_t)kernel_ptable) >> PAGE_TABLE_BITS;
    insert_page_frame_info(kernel_ptable, kernel_ptable, dummy_kernel_pcb, PE_INFO_PINNED);
    /* Set the appropriate access flag (user or kernel) for the video buffer page at 0xb8000.
     * Identity maps the video memory from 0xb8000-0xb8fff for direct access.
     */
    int mode = (is_user ? user_mode : kernel_mode);
    table_map_page(
            kernel_ptable, (uint32_t) VGA_TEXT_PADDR, (uint32_t) VGA_TEXT_PADDR, mode
    );
    // Map the kernel memory using identity mapping for direct physical to virtual address mapping.
    for (uint32_t paddr = 0; paddr < KERNEL_SIZE; paddr += PAGE_SIZE) {
        table_map_page(kernel_ptable, paddr, paddr, kernel_mode);
    }
    /*
     * Identity map the rest of the physical memory so the kernel can directly access
     * all physical memory.
     */
    for (uint32_t paddr = PAGING_AREA_MIN_PADDR; paddr < PAGING_AREA_MAX_PADDR; paddr += PAGE_SIZE) {
        table_map_page(kernel_ptable, paddr, paddr, kernel_mode);
    }
    // Insert the kernel page table into the page directory at the first entry,
    // setting up the mapping for kernel space.
    dir_ins_table(pdir, 0, kernel_ptable, kernel_mode);
}


static void setup_kernel_vmem(void) {
    dummy_kernel_pcb -> is_thread = 1;
    uint32_t info_mode = PE_INFO_PINNED | PE_INFO_KERNEL_DUMMY;
    lock_acquire(&page_map_lock);
    //kernel_pdir = allocate_page();
    page_frame_info_t *kernel_pdir_info = page_alloc();
    kernel_pdir = kernel_pdir_info -> paddr;
    insert_page_frame_info(kernel_pdir, kernel_pdir, dummy_kernel_pcb, info_mode);
    setup_kernel_vmem_common(kernel_pdir, 0);
    lock_release(&page_map_lock);
}


 /* Sets up a page directory and page table for a new process or thread.
 */
void setup_process_vmem(pcb_t *p) {
    // Allocate a new page directory for the process
    page_frame_info_t *proc_pdir_info = page_alloc();
    uint32_t *proc_pdir = proc_pdir_info -> paddr;
    //uint32_t *proc_pdir = allocate_page();
    uint32_t user_mode = PE_RW | PE_US;

    // basically same as P4-precode
    lock_acquire(&page_map_lock);
    if (p->is_thread) {
        p -> page_directory = kernel_pdir;
        pr_debug("setup thread vmem done\n");
        lock_release(&page_map_lock);
        return;
    } else {
        // Ensure the kernel's virtual address space is consistently mapped into 
        // the virtual address space of every process. The kernel needs to be accessible
        // at all times to handle interrupts since the CPU only sees virtual addresses.
        // It is important that flags make the addresses not accessible by the user
        setup_kernel_vmem_common(proc_pdir, 1);
    }

    // Allocate and setup page table for the process's specific memory (code and data segments)
    // uint32_t *proc_ptable = allocate_page();
    page_frame_info_t *proc_ptable_info = page_alloc();
    uint32_t *proc_ptable = proc_ptable_info -> paddr;
    insert_page_frame_info(proc_ptable, proc_ptable, p, PE_INFO_PINNED);
    insert_page_frame_info(proc_pdir, proc_pdir,p, PE_INFO_PINNED);

    // Map it into the second entry of the page directory
    // set as present since it is a page table.
    dir_ins_table(proc_pdir, PROCESS_VADDR, proc_ptable, user_mode | PE_P);

    /*
     * Map in the process image.
     * Modified from P4-precode
     * 
     * NOTE: either swap_size is text + data only (excludes the stack), or the
     * stack size must be subtracted from p->swap_size in the loop conditional
     */
    uint32_t paddr, vaddr, offset;
    for (offset = 0; offset < p->swap_size; offset += PAGE_SIZE) {
        paddr = p->swap_loc + offset; // page not present - anything works
        vaddr = PROCESS_VADDR + offset;
        table_map_page(proc_ptable, vaddr, paddr, user_mode);
    }

    // Allocate pages of stack area
    // assuming a fixed size stack area for each process
    uint32_t *stack_page, stack_vaddr;
    uint32_t stack_lim = PROCESS_STACK_VADDR - USER_STACK_SIZE;
    uint32_t stack_base = PROCESS_STACK_VADDR;
    for (stack_vaddr = stack_base; stack_vaddr > stack_lim; stack_vaddr -= PAGE_SIZE) {
        //stack_page = allocate_page();
        page_frame_info_t *stack_page_info = page_alloc();
        stack_page = stack_page_info -> paddr;
        insert_page_frame_info(stack_page, (uintptr_t *) stack_vaddr, p, PE_INFO_PINNED | PE_INFO_USER_MODE);
        // map the stack virtual address to the proc_ptable
        table_map_page(proc_ptable, stack_vaddr,  (uint32_t) stack_page, user_mode | PE_P);
    }

    // Update the process control block
    p->page_directory = proc_pdir;
    pr_debug("setup process vmem done\n");
    lock_release(&page_map_lock);
}



/*
 * init_memory()
 *
 * called once by _start() in kernel.c
 * You need to set up the virtual memory map for the kernel here.
 */
void init_memory(void)
{
    // Initialize circular linked list to track free pages
    //initialize_free_pages();
    initialize_page_frame_infos();

    next_free_mem = PAGING_AREA_MIN_PADDR;
    setup_kernel_vmem();

}

/* === Exception handlers === */


/* === Page Fault Handler Helper Functions === */

// Generates a pseudo-random number using a linear congruential generator.
// paddr of a page is then result*PAGE_SIZE + PAGING_AREA_MIN_PADDR
unsigned long generate_random_number() {
    next = next * 1103515245 + 12345;
    return (unsigned long)(next/65536) % 32768;
}

/*
 * === Page Eviction and State Management ===
 * Functions to handle the eviction of pages, check if pages 
 * are dirty or free, and mark pages as used/free.
 */
 
// Random eviction algorithm
uint32_t select_page_for_eviction() {
    uint32_t random_page_number = generate_random_number() % PAGEABLE_PAGES;
    return random_page_number*PAGE_SIZE + PAGING_AREA_MIN_PADDR;
    //return random_page_number;
}

// Checks if the given page is dirty by looking up its page table entry
bool is_page_dirty(uint32_t vaddr, uint32_t *page_directory)
{
    uint32_t dir_index = get_directory_index(vaddr);
    uint32_t dir_entry = page_directory[dir_index];
    if (!(dir_entry & PE_P)) {
        // Page directory entry is not present.
        return false;
    }
    uint32_t* page_table = (uint32_t*)(dir_entry & PE_BASE_ADDR_MASK);
    uint32_t table_index = get_table_index(vaddr);
    uint32_t page_entry = page_table[table_index];
    return page_entry & PE_D;
}

void unmap_physical_page(uint32_t vaddr) {
    // Eindride 8/04-22: 
    // Changed kernel_pdir to current_running->page_directory
    // Given mode 0 clears the PE_P bit
    page_set_mode(current_running->page_directory, vaddr, 0); 
}


/*
 * Given a virtual address and a pointer to the page directory, it
 * returns a pointer to the relevant page table.
 */
uint32_t* get_page_table(uint32_t vaddr, uint32_t* page_directory) 
{
    uint32_t dir_index = get_directory_index(vaddr);
    uint32_t dir_entry = page_directory[dir_index];
    if (!(dir_entry & PE_P)) {
        pr_debug("get_page_table: no page table found in page dir %p with virtual address %u\n", page_directory, vaddr);
        return NULL;
    }
    uint32_t* page_table = (uint32_t*)(dir_entry & PE_BASE_ADDR_MASK);
    return page_table;
}

// Not in use
// uint32_t *vaddr_to_paddr(uint32_t vaddr, uint32_t *page_directory)
// {
//     /*
//      * perform a lookup in page directory
//      * to extract physical address
//      */
//     uint32_t *page_table, *page, index;
//     index = get_table_index(vaddr);
//     if(! (page_table = get_page_table(vaddr, page_directory)) ) {
//         goto error_notable;
//     }
//     page = &page_table[index];

//     if (*page & PE_P)
//         return page + (vaddr & PAGE_MASK);
//     else 
//         goto error_page_not_present;

//     error_notable:
//         pr_error("no page table in page_directory %p with suiting virtual address %u", page_directory, vaddr);
//         return NULL;
    
//     error_page_not_present:
//         pr_error("no page in page table %p in page directory %p suiting virtual address %u", page_table, page_directory, vaddr);
//         return NULL;
// }

// Not in use
// uint32_t *vaddr_to_frameref(uint32_t vaddr, uint32_t *page_directory)
// {
//     /*
//      * Locates the (physical) page frame of the cirtual address
//      * with respect to the provided page_directory
//      */
//     uint32_t *page_table;
//     if( (page_table = get_page_table(vaddr, page_directory)) )
//         return &page_table[get_table_index(vaddr)];
//     else
//         return NULL;
// }

// Not in use
// uint32_t *paddr_to_frameref(uint32_t paddr)
// {
//     /*
//      * Locates the (physical) page frame related to
//      * a physical address,
//      * Useful for checking page frame flags when evicting etc.
//      */

//     // integer division: (paddr / PAGE_SIZE) * PAGE_SIZE == PAGE_SIZE
//     // is true only when paddr is a multiple of PAGE_SIZE
//     // uint32_t *frameref = (uint32_t *) ((paddr / PAGE_SIZE)*PAGE_SIZE);
//     // if (*frameref & PE_P) return frameref;
//     // else return NULL;

//     //  
//     //  memory in kernel is identity mapped, so we can treat
//     //  the physical addres paddr as a virtual address inside
//     //  the kernel
//     return vaddr_to_frameref(paddr, kernel_pdir);
// }

// Not in use
// Calculates the disk offset for a given physical address.
// Uses PAGING_AREA_MIN_PADDR as the base address for disk offset calculations.
// uint32_t calculate_disk_offset(uint32_t paddr) {
//     // Use the defined base address where the swap space or process images start.
//     const uint32_t DISK_BASE_ADDR = PAGING_AREA_MIN_PADDR;
//     // The offset is the difference from this base.
//     return paddr - DISK_BASE_ADDR;
// }

// Not in use
// void clear_page_dirty_bit(uint32_t vaddr, uint32_t* page_directory) {
//     page_set_mode(page_directory, vaddr, PE_P | PE_RW); 
// }

// Not in use
// uint32_t virtual_to_physical_address(uint32_t vaddr, uint32_t* page_directory) {
//     uint32_t dir_index = get_directory_index(vaddr);
//     uint32_t dir_entry = page_directory[dir_index];

//     if (!(dir_entry & PE_P)) {
//         return 0;
//     }
//     uint32_t* page_table = (uint32_t*)(dir_entry & PE_BASE_ADDR_MASK);
//     uint32_t table_index = get_table_index(vaddr);
//     uint32_t page_entry = page_table[table_index];

//     if (!(page_entry & PE_P)) {
//         return 0;
//     }
//     // Extract the physical address from the page table entry and the offset from the virtual address.
//     uint32_t page_offset = vaddr & PAGE_MASK;
//     uint32_t physical_page_start = page_entry & PE_BASE_ADDR_MASK;
//     return physical_page_start + page_offset;
// }

uint32_t vaddr_to_disk_addr(uint32_t vaddr, pcb_t *pcb, int *error)
{
    *error = 0;
    uint32_t disk_addr = (vaddr - PAGING_AREA_MIN_PADDR) + pcb -> swap_loc; // in bytes
    if (disk_addr < pcb->swap_loc + pcb->swap_size) {
        //error
        *error = 1;
        pr_debug("disk address too large, outside swap area of process %u\n", pcb -> pid);
    }
    return disk_addr;
}

int write_page_back_to_disk(uint32_t vaddr, pcb_t *pcb){
    // Calculate the disk offset based on the physical address.
    int success = -1;
    uint32_t disk_addr = (vaddr - PAGING_AREA_MIN_PADDR) + pcb -> swap_loc; // in bytes
    if (disk_addr < pcb->swap_loc + pcb->swap_size) {
        pr_debug("disk address too large, outside swap area of process %u\n", pcb -> pid);
        return success;
    }
    uint32_t disk_loc = disk_addr/SECTOR_SIZE;

    uint32_t *frameref_table;
    if (! (frameref_table = get_page_table(vaddr, pcb->page_directory)) ) {
        // allocate new table
        page_frame_info_t *frameref_table_info = page_alloc();
        frameref_table = frameref_table_info -> paddr;

        int info_mode;
        int mode; 
        if (pcb -> is_thread) {
            info_mode = PE_INFO_PINNED;
            mode = PE_P | PE_RW;
        } else {
            info_mode = PE_INFO_USER_MODE;
            mode = PE_P | PE_RW | PE_US;
        }
        insert_page_frame_info(frameref_table, (uintptr_t *) vaddr, pcb, info_mode);
        dir_ins_table(pcb -> page_directory, vaddr, frameref_table, mode);
    } 


    uint32_t *frameref = &frameref_table[get_table_index(vaddr)];
    success = scsi_write(disk_loc, PAGE_SIZE/SECTOR_SIZE, (void *) frameref);
    // Clear the dirty bit for this page in the page table to simulate writing it back to disk.
    pr_debug("Page at virtual address 0x%08x written back to disk at address 0x%08x\n", vaddr, disk_addr);
    return success; //success
}

int load_page_from_disk(uint32_t vaddr, pcb_t *pcb) {
    uint32_t user_mode = PE_US | PE_RW;
    int success = -1;
    uint32_t disk_offset = (vaddr - PAGING_AREA_MIN_PADDR) + pcb->swap_loc; // in bytes
    if ( disk_offset > (pcb->swap_loc + pcb->swap_size) ) {
        pr_debug("Disk address is outside the swap area of process %u\n", pcb->pid);
        return success; 
    }

    uint32_t disk_sector = disk_offset / SECTOR_SIZE;
    uint32_t *frameref_table = get_page_table(vaddr, pcb->page_directory);
    if (frameref_table == NULL) {
         //pr_debug("Failed to allocate frame for virtual address 0x%08x\n", vaddr);
         //return success;
         // no table exists in page dir with the virtual address
         // allocate new page table and insert into page directory
         page_frame_info_t *frameref_table_info = page_alloc();
         frameref_table = frameref_table_info -> paddr;

        int info_mode;
        int mode; 
        if (pcb -> is_thread) {
            info_mode = PE_INFO_PINNED;
            mode = PE_P | PE_RW;
        } else {
            info_mode = PE_INFO_USER_MODE;
            mode = PE_P | PE_RW | PE_US;
        }

        insert_page_frame_info(frameref_table, (uintptr_t *) vaddr, pcb, info_mode);
        dir_ins_table(pcb->page_directory, vaddr, frameref_table, mode);
    }

    // tries to allocate a page if available and evicts a page if not
    page_frame_info_t *frameref_info = page_alloc();
    if (frameref_info == NULL) {
        uint32_t *evicted_page = try_evict_page();
        frameref_info = &page_frame_info[calculate_info_index(evicted_page)];
    }

    uint32_t *frameref = frameref_info -> paddr;
    insert_page_frame_info(frameref, (uintptr_t *) vaddr, pcb, PE_INFO_USER_MODE);

    success = scsi_read(disk_sector, PAGE_SIZE / SECTOR_SIZE, (void *) frameref);
    if (success != 0) {
        pr_debug("Failed to read from disk sector %u\n", disk_sector);
        return success;
    }
    // check that frameref is not present before overwriting
    if (*frameref & PE_P) {
       // ???
    }
    table_map_page(frameref_table, vaddr, (uint32_t) frameref, user_mode);
    pr_debug("Loaded page at virtual address 0x%08x from disk into physical address 0x%08x\n", vaddr, (uint32_t) frameref);
    return success; 
}


// Attempts to find and evict a page
// Returns virtual address of the page that was evicted, or NULL if no suitable page was found.
uint32_t* try_evict_page()
{
    uintptr_t *frameref;
    uint32_t info_index;
    page_frame_info_t *frame_info;
    //uint32_t *base = (uint32_t *) PAGING_AREA_MIN_PADDR;

    for (int attempts = 0; attempts < PAGEABLE_PAGES; ++attempts) {
        frameref = (uintptr_t *) select_page_for_eviction();
        info_index = calculate_info_index(frameref);
        frame_info = &page_frame_info[info_index];
        if (!(frame_info -> info_mode & PE_INFO_PINNED)) {
            // Check if the page is dirty before deciding to evict
            int dirty = traverse_evict(frame_info->paddr);
            if (dirty > 0) {
                // The page is dirty, write it back to disk
                write_page_back_to_disk((uint32_t)frame_info->vaddr, frame_info->owner);
                // Proceed to unmap the page
                unmap_physical_page((uint32_t)frame_info->vaddr);

                spinlock_acquire(&page_frame_info_lock);
                add_page_frame_to_free_list_info(frame_info->paddr);
                spinlock_release(&page_frame_info_lock);

                return frame_info->vaddr; 
            } else if (dirty == 0) {
                // The page is not dirty, proceed with eviction directly
                unmap_physical_page((uint32_t)frame_info->vaddr);
                add_page_frame_to_free_list_info(frame_info->paddr);
                return frame_info->vaddr;
            } else {
            // page must not be evicted, continue
            continue;
            }
        }
    }
    // Couldn't find a suitable page to evict.
    return NULL;
}


/*
 * Comment author: Eindride Kjersheim
 * Error code bits, from Intel i386 manual page 170.
 * https://css.csail.mit.edu/6.858/2014/readings/i386.pdf
 * 
 *                                 2       1       0
 *  ____________________________ ______________________
 * |            29 bits         |  U/S    W/R     V/P  | 
 * |___________undefined________|______________________|
 * 
 *  Error code is the last 3 bits masked as error_code & 0x7, recall 0x7 = 0b111
 *  
 * U/S bit: 1 when processor was executing in user mode, 0 when in supervisor mode
 * W/R bit: 1 for write, 0 for read
 * P/V bit: 1 for page-level protection violation, 0 for page not present
 */ 


#define ec_privilige_level(error_code) (error_code) & 0x4 >> 2
#define ec_write(error_code) (error_code) & 0x2 >> 1
#define ec_page_not_present(error_code) !((error_code) & 0x1)
#define ec_privilige_violation(error_code) (error_code) & 0x1

// uint32_t total_page_fault_counter = 0;
// uint32_t total_page_fault_page_not_present_counter = 0;

/*
 * Handle page fault
 */
void page_fault_handler(struct interrupt_frame *stack_frame, ureg_t error_code)
{
    nointerrupt_leave();
    //total_page_fault_counter++;
    if (MEMDEBUG) {
        print_pcb_table();
        pr_debug("error code: %u \n", error_code & 0x7);
    }
    // bool error_code_privilige = (error_code & 0x4) >> 2;
    // bool error_code_write = (error_code & 0x2) >> 1;
    // bool error_code_page_not_present = (~error_code) & 0x1;
    // bool error_code_privilige_violation = !error_code_page_not_present;

    if (ec_privilige_violation(error_code)) {
        // abort - access violation
        pr_error("page fault: access violation");
        pr_debug("error code: %0x \n", error_code);
        abortk();
    } else {
        //total_page_fault_page_not_present_counter++;
        // page not present

        if (DEBUG_PAGEFAULT) {
            pr_debug("page fault: page not present \n");
            pr_debug("page fault: privlige level (U = 1; S = 0) %u\n", ec_privilige_level(error_code));
            pr_debug("page fault: read/write operation (W=1, R=0) %u\n", ec_write(error_code));
            //pr_debug("total page faults: %u \n", total_page_fault_counter);
            //pr_debug("total page not present page faults: %u \n", total_page_fault_page_not_present_counter);
        }

        int error;
        uint32_t fault_address = load_page_fault_addr();
        uint32_t disk_address = vaddr_to_disk_addr(fault_address, current_running, &error);

        if (error) {
            pr_error("error occured; disk address %u out of bounds of paging area\n", disk_address);
        }

        page_frame_info_t *new_page_info = page_alloc();
        uintptr_t *new_page = new_page_info -> paddr;
        insert_page_frame_info(new_page, (uintptr_t *) fault_address, current_running, PE_INFO_USER_MODE);

        if ( load_page_from_disk(fault_address, current_running) >= 0) {
            set_page_directory(current_running -> page_directory);
            nointerrupt_enter();
            return;
        }

    }

    todo_use(stack_frame);
    todo_use(error_code);
    todo_use(page_map_lock);
    todo_abort();

    nointerrupt_enter();

/*
    page_fault_handler, pseudocode:
    ===============================
    // Step 1: Retrieve the faulting address from CR2 register
    faulting_address = call load_page_fault_addr() -- precode, cpu_x86.h

    // Step 2: Log or print the fault details for debugging
    log_page_fault(faulting_address, error_code) -- check presence/need to implement

    // Step 3: Decode the error code to understand the nature of the fault
    if error_code indicates protection violation:
        // This is an access violation, not just a page not being present.
        // It could mean an illegal access by a program, such as writing to a read-only page,
        // accessing kernel memory from user mode, etc.
        handle_protection_violation(faulting_address, error_code) -- check presence/need to implement
        return

    if error_code indicates the page is not present:
        // The page the program wants to access is not in memory.
        
        // Step 4: Check if the address is within a valid range for the current process
        if not valid_address_for_process(faulting_address): -- check presence/need to implement
            handle_invalid_access(faulting_address) -- check presence/need to implement
            return
        
        // Step 5: Determine if the page needs to be loaded from disk or just allocated
        if page_needs_to_be_loaded_from_disk(faulting_address): -- check presence/need to implement

            // Step 6: Check if there's a free page available
            if not is_free_page_available(): -- check presence/need to implement

                // Step 7: Evict a page if no free pages are available
                DONE evicted_page = select_page_for_eviction()
                TO FINISH! try_to_evict_page(evicted_page)
            
            // Step 8: Load the page into a free page frame
            load_page_from_disk(faulting_address) -- check presence/need to implement
        else:
            // The page is not present because it hasn't been allocated yet (e.g., stack growth)
            allocate_new_page(faulting_address) -- check presence/need to implement
        
        // Step 9: Update the page table to reflect the changes
        update_page_table(faulting_address) -- check presence/need to implement
        return


Function to find in precode or implement:
=========================================

function log_page_fault(faulting_address, error_code):
    // Log the faulting address and error code details for debugging purposes

function handle_protection_violation(faulting_address, error_code):
    // Handle protection violations, such as illegal accesses

function valid_address_for_process(faulting_address):
    // Check if the faulting address is within the valid address space of the current process

function page_needs_to_be_loaded_from_disk(faulting_address):
    // Determine if the faulting address corresponds to data that needs to be loaded from disk

function is_free_page_available():
    // Check if there is a free page available in physical memory

DONE function select_page_for_eviction():
    // Select a page to evict based on the eviction policy (e.g., LRU, random?)

TO FINISH!function try_to_evict_page(page):
    // Evict the specified page, writing it back to disk if necessary

function load_page_from_disk(faulting_address):
    // Load the required data from disk into the corresponding physical page

function allocate_new_page(faulting_address):
    // Allocate a new page in physical memory for the faulting address

function update_page_table(faulting_address):
    // Update the page table entry for the faulting address to reflect the new page frame
*/
}
