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
#define HASH_TABLE_SIZE 1024
//#define KERNEL_SIZE 0x400000 // 4 MB in hexadecimal
#define KERNEL_SIZE 0x300000 // 4 MB in hexadecimal
uint32_t free_pages_bitmap[BITMAP_SIZE];
static unsigned long next = 1;


#define MEMDEBUG 1
#define DEBUG_PAGEFAULT 1
#define MEM_DEBUG_LOADPROC 1

#define STACK_SIZE 0x1000 // two pages - same as kernel level stacks

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

// void *memset(void *s, int c, size_t n)
// {
//     for (unsigned char *cur = s; n > 0; n--, cur++) *cur = (unsigned char) c;
//     return s;
// }

/*
 * === Pinned Page Tracking ===
 * Functions for managing pinned pages, which should not be swapped out.
 */

typedef struct PinnedPageEntry {
    uint32_t vpn; // Virtual Page Number
    bool pinned;  // Whether the page is pinned
    struct PinnedPageEntry* next; // For handling collisions via chaining
} PinnedPageEntry;

PinnedPageEntry* hashTable[HASH_TABLE_SIZE];
PinnedPageEntry* entryBlock = NULL; // Pointer to a block of entries
size_t entriesAllocated = 0;        // Number of entries allocated from the block
size_t entriesPerBlock = 4096 / sizeof(PinnedPageEntry); 

unsigned int hashFunction(uint32_t vpn) {
    return vpn % HASH_TABLE_SIZE;
}

PinnedPageEntry* allocateEntry() {
    if (entryBlock == NULL || entriesAllocated >= entriesPerBlock) {
        // Allocate a new block if needed
        entryBlock = (PinnedPageEntry*)alloc_memory(4096); // Allocates a full page
        entriesAllocated = 0;
    }
    return &entryBlock[entriesAllocated++];
}

bool isPagePinned(uint32_t vpn) {
    unsigned int index = hashFunction(vpn);
    PinnedPageEntry* entry = hashTable[index];
    while (entry) {
        if (entry->vpn == vpn) {
            return entry->pinned;
        }
        entry = entry->next;
    }
    return false;
}

void insertPinnedPage(uint32_t vpn, bool pinned) {
    PinnedPageEntry* newEntry = allocateEntry();
    if (newEntry == NULL) {
        return;
    }
    newEntry->vpn = vpn;
    newEntry->pinned = pinned;
    unsigned int index = hashFunction(vpn);

    // Collision resolution by chaining
    newEntry->next = hashTable[index];
    hashTable[index] = newEntry;

}


/* === Initializes the bitmap to mark all pages as free === */
void initialize_free_pages_bitmap() {
    // Using custom memset function from string_core.c file
    // Multiply by sizeof(uint32_t) because the bitmap is an array of uint32_t, not bytes
    memset(free_pages_bitmap, 0xFF, sizeof(free_pages_bitmap[0]) * BITMAP_SIZE);
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
 * === Page Allocation and Management ===
 * Functions to allocate physical pages, map virtual addresses to physical pages, and convert page numbers to virtual addresses.
 */

uint32_t* allocate_page(void) {
    uint32_t* page = (uint32_t*) alloc_memory(4096);
    for (int i = 0; i < 1024; i++) page[i] = 0; // Zero out the page
    return page;
}

void identity_map_page(uint32_t* table, uint32_t vaddr, uint32_t mode) {
    uint32_t index = (vaddr >> 12) & 0x3FF; // Extract the page table index
    table[index] = (vaddr & ~0xFFF) | mode | PE_P; // Map the page with the provided mode
}

uint32_t page_number_to_virtual_address(uint32_t page_number) {
    return page_number * PAGE_SIZE;
}


/* === Kernel and process setup === */

/* address of the kernel page directory (shared by all kernel threads) */
static uint32_t *kernel_pdir;


static void setup_kernel_vmem_common(uint32_t *pdir, int is_user) {
    uint32_t user_mode = PE_P | PE_RW | PE_US;
    uint32_t kernel_mode = PE_P | PE_RW;
    uint32_t *kernel_ptable = allocate_page();

    // Mark the kernel's page table as pinned
    uint32_t kernel_ptable_number = ((uintptr_t)kernel_ptable) >> PAGE_TABLE_BITS;
    insertPinnedPage(kernel_ptable_number, true);

    /* Identity map the video memory, from 0xb8000-0xb8fff.
     * From P4-precode
     * */
    int mode = (is_user ? user_mode : kernel_mode);
    table_map_page(
            kernel_ptable, (uint32_t) VGA_TEXT_PADDR, (uint32_t) VGA_TEXT_PADDR, mode
    );

    // Mapping kernel memory with specific physical addresses
    for (uint32_t paddr = 0; paddr < KERNEL_SIZE; paddr += PAGE_SIZE) {
        table_map_page(kernel_ptable, paddr, paddr, kernel_mode);
    }

    /*
     * Identity map in the rest of the physical memory so the
     * kernel can access everything in memory directly.
     * From P4-precode
     */
    for (uint32_t paddr = PAGING_AREA_MIN_PADDR; paddr < PAGING_AREA_MAX_PADDR; paddr += PAGE_SIZE) {
        table_map_page(kernel_ptable, paddr, paddr, kernel_mode);
    }
    dir_ins_table(pdir, 0, kernel_ptable, kernel_mode);
}


static void setup_kernel_vmem(void) {
    lock_acquire(&page_map_lock);
    kernel_pdir = allocate_page();
    setup_kernel_vmem_common(kernel_pdir, 0);
    lock_release(&page_map_lock);
}


// static void load_disk_segment(location, size) {
// }


/*
 * Sets up a page directory and page table for a new process or thread.
 */
void setup_process_vmem(pcb_t *p) {
    // Allocate a new page directory for the process
    uint32_t *proc_pdir = allocate_page();
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
    uint32_t *proc_ptable = allocate_page();

    // Calculate the (physical) page number of the page table and page dirs and pin them
    uint32_t proc_ptable_number = ((uintptr_t)proc_ptable) >> PAGE_TABLE_BITS; 
    uint32_t proc_pdir_number = ((uintptr_t)proc_pdir) >> PAGE_TABLE_BITS; 
    insertPinnedPage(proc_ptable_number, true);
    insertPinnedPage(proc_pdir_number, true);

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
        paddr = p->swap_loc + offset;
        vaddr = PROCESS_VADDR + offset;
        table_map_page(proc_ptable, vaddr, paddr, user_mode);
    }

    // Allocate pages of stack area
    // assuming a fixed size stack area for each process
    uint32_t *stack_page, stack_vaddr;
    uint32_t stack_lim = PROCESS_STACK_VADDR - STACK_SIZE;
    uint32_t stack_base = PROCESS_STACK_VADDR;
    for (stack_vaddr = stack_base; stack_vaddr > stack_lim; stack_vaddr -= PAGE_SIZE) {
        stack_page = allocate_page();
        // should another table be used?
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
    // Initialize the bitmap to track free pages
    initialize_free_pages_bitmap();

    next_free_mem = PAGING_AREA_MIN_PADDR;
    setup_kernel_vmem();

}

/* === Exception handlers === */

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

/* === Page Fault Handler Helper Functions === */

/*
 * === Random Number Generation for Page Eviction ===
 * These functions are involved in generating random numbers for the random page eviction strategy.
 * They implement a simple pseudo-random number generator.
 */


// Generates a pseudo-random number using a linear congruential generator.
unsigned long generate_random_number() {
    next = next * 1103515245 + 12345;
    return (unsigned long)(next/65536) % 32768;
}

// Seed the generator. In a real system, we might use a more unpredictable seed.
// void srand(unsigned long seed) {
//     next = seed;
// }

/*
 * === Page Eviction and State Management ===
 * Functions to handle the eviction of pages, check if pages are dirty or free, and mark pages as used/free.
 */
 
// Random eviction algorithm
uint32_t select_page_for_eviction() {
    uint32_t random_page_number = generate_random_number() % PAGEABLE_PAGES;
    return random_page_number;
}

// Checks if the given page is dirty by looking up its page table entry
bool is_page_dirty(uint32_t vaddr) {
    uint32_t dir_index = get_directory_index(vaddr);
    uint32_t dir_entry = kernel_pdir[dir_index];
    if (!(dir_entry & PE_P)) {
        // Page directory entry is not present.
        return false;
    }
    uint32_t* page_table = (uint32_t*)(dir_entry & PE_BASE_ADDR_MASK);
    uint32_t table_index = get_table_index(vaddr);
    uint32_t page_entry = page_table[table_index];
    return (page_entry & PE_D) != 0;
}

// bool is_page_dirty_user(uint32_t vaddr, uint32_t* page_directory) {
//     // We might need this version of the function that would take an additional parameter for the page directory,
//     // allowing it to be used with any process's address space, not just the kernel's...
// }

// Checks if a page is free by consulting the free pages bitmap.
bool is_page_free(uint32_t page_number) {
    if (page_number >= PAGEABLE_PAGES) {
        return false; // Out of bounds
    }
    uint32_t index = page_number / BITS_PER_ENTRY;
    uint32_t bit = page_number % BITS_PER_ENTRY;
    return (free_pages_bitmap[index] & (1 << bit)) != 0;
}

void unmap_physical_page(uint32_t vaddr) {
    page_set_mode(kernel_pdir, vaddr, 0); // Given mode 0 clears the PE_P bit
}

void mark_page_as_free(uint32_t page_number) {
    if (page_number < PAGEABLE_PAGES) {
        uint32_t index = page_number / BITS_PER_ENTRY;
        uint32_t bit = page_number % BITS_PER_ENTRY;
        free_pages_bitmap[index] |= (1 << bit);
    }
}

void mark_page_as_used(uint32_t page_number) {
    if (page_number < PAGEABLE_PAGES) {
        uint32_t index = page_number / BITS_PER_ENTRY;
        uint32_t bit = page_number % BITS_PER_ENTRY;
        free_pages_bitmap[index] &= ~(1 << bit);
    }
}

// Attempts to find and evict a page
// Returns the physical address of the page that was evicted, or NULL if no suitable page was found.
uint32_t* try_evict_page() {
    for (int attempts = 0; attempts < PAGEABLE_PAGES; ++attempts) {
        uint32_t page_number = select_page_for_eviction();
        if (!isPagePinned(page_number)) {
            uint32_t vaddr = page_number_to_virtual_address(page_number); // Convert page number to virtual address.
            if (is_page_dirty(page_number)) {
                pr_debug("Write the page back to disk if it's dirty\n");
                // Write the page back to disk if it's dirty
                //TODO! update_page_table(page_number);
            }
            // Unmap the page from the address space, marking it as not present
            unmap_physical_page(page_number);
            // Mark the page as free for future use
            mark_page_as_free(page_number);
            // Optionally, return the virtual or physical address of the evicted page
            return (uint32_t*)vaddr; // Returning the virtual address so far, can be adjusted
        }
    }
    // Couldn't find a suitable page to evict.
    return NULL;
}


/*
 * Handle page fault
 */
void page_fault_handler(struct interrupt_frame *stack_frame, ureg_t error_code)
{
    nointerrupt_leave();
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
        abortk();
    } else {
        // page not present

        if (DEBUG_PAGEFAULT) {
            pr_debug("page fault: page not present \n");
            pr_debug("page fault: privlige level (U = 1; S = 0) %u\n", ec_privilige_level(error_code));
            pr_debug("page fault: read/write operation (W=1, R=0) %u\n", ec_write(error_code));
        }

        // different handling of read/writes?

        // what to do with privilige levels - simply info for what mode to set on pages?
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

            // DONE Step 6: Check if there's a free page available
            if not is_free_page_available(): -- check presence/need to implement

                // Step 7: Evict a page if no free pages are available
                DONE evicted_page = select_page_for_eviction() -- need to implement
                TO FINISH! evict_page(evicted_page) -- need to implement
            
            // Step 8: Load the page into a free page frame
            load_page_from_disk(faulting_address) -- check presence/need to implement
        else:
            // The page is not present because it hasn't been allocated yet (e.g., stack growth)
            allocate_new_page(faulting_address) -- check presence/need to implement
        
        // Step 9: Update the page table to reflect the changes
        TODO update_page_table(faulting_address) -- check presence/need to implement
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

DONE function is_free_page_available():
    // Check if there is a free page available in physical memory

DONE function select_page_for_eviction():
    // Select a page to evict based on the eviction policy (e.g., LRU, random?)

TO FINISH!function evict_page(page):
    // Evict the specified page, writing it back to disk if necessary

function load_page_from_disk(faulting_address):
    // Load the required data from disk into the corresponding physical page

function allocate_new_page(faulting_address):
    // Allocate a new page in physical memory for the faulting address

TODO! function update_page_table(faulting_address):
    // Update the page table entry for the faulting address to reflect the new page frame
*/
}
