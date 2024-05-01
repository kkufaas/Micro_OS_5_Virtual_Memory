/*
 * Note:
 * There is no separate swap area. When a data page is swapped out,
 * it is stored in the location it was loaded from in the process'
 * image. This means it's impossible to start two processes from the
 * same image without screwing up the running. It also means the
 * disk image is read once. And that we cannot use the program disk.
 */

#include <kernel/hardware/cpu_x86.h>
#define pr_fmt(fmt) "vmem: " fmt
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

#include "interrupt.h"
#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"
#include "memory.h"
#include "scheduler.h"
#include "sync.h"
#include "usb/scsi.h"

#include "config.h"

#define UNUSED(x)   ((void) x)
//#define KERNEL_SIZE 0x400000 // 4 MB in hexadecimal
#define KERNEL_SIZE        0x400000 // 3 MB in hexadecimal
//#define KERNEL_SIZE        0x200000 // 2 MB in hexadecimal
//#define KERNEL_SIZE PAGING_AREA_MIN_PADDR + 1

// #define KERNEL_SIZE 640 * 1024

#define MEMDEBUG           1
#define DEBUG_PAGEFAULT    0
#define MEM_DEBUG_LOADPROC 0
#define USER_STACK_SIZE    0x1000

#define MIN(x, y) (x < y ? x : y)

// #define PIN_SHELL 0
// enum {
//     EVICTION_STRATEGY_FIFO = 1,
//     EVICTION_STRATEGY_RANDOM = 2,
// };
// 
// #define EVICTION_STRATEGY EVICTION_STRATEGY_FIFO
// #define EVICTION_STRATEGY EVICTION_STRATEGY_RANDOM
// #define USERS_SHARE_KERNEL_PAGE_TABLE 1

static uint32_t first_process = 1;
static uint32_t first_process_pid;

static pcb_t dummy_kernel_pcb[1];

static uint32_t  number_of_pinned_page_frames = 0;
static uint32_t *kernel_pdir;

enum {
    PE_INFO_USER_MODE    = 1 << 0, /* user */
    PE_INFO_KERNEL_DUMMY = 1 << 1, /* kernel ''dummy'' */
    PE_INFO_PINNED       = 1 << 2, /* pinned */
    PE_INFO_STACK        = 1 << 3,
};

/* === Simple memory allocation === */

static uintptr_t  next_free_mem;
static spinlock_t next_free_mem_lock   = SPINLOCK_INIT;

/* === Page allocation tracking === */
static lock_t page_map_lock = LOCK_INIT;
static spinlock_t page_frame_info_lock = SPINLOCK_INIT;

inline uint32_t get_table_index(uint32_t vaddr);
uint32_t       *get_page_table(uint32_t vaddr, uint32_t *page_directory);
bool            is_page_dirty(uint32_t vaddr, uint32_t *page_directory);
uint32_t       *try_evict_page();
uint32_t        calculate_info_index(uintptr_t *paddr);
inline uint32_t get_directory_index(uint32_t vaddr);
void unmap_physical_page(uint32_t *process_directory, uint32_t vaddr);
void print_page_table_info(void);
void print_fifo_queue();

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


/* === Info structure to keep track on pages condition === */

struct page_frame_info {
    pcb_t *owner;

    // the next info frame like this sharing the same physical address
    struct page_frame_info *next_shared_info;

    // points to the next free page
    // Note: If taken - is null.
    // free a page index by physical address and insert
    // into the linked list
    struct page_frame_info *next_free_page;

    uintptr_t *paddr;
    uintptr_t *vaddr;
    uint32_t   info_mode;
};
typedef struct page_frame_info page_frame_info_t;

page_frame_info_t  page_frame_info[PAGEABLE_PAGES];
page_frame_info_t  page_frame_info_shared[PAGEABLE_PAGES];
page_frame_info_t *page_free_head;

#define set_frame_info(frame, ownerv, next_shared_infov, paddrv, info_modev) \
    { \
        frame.owner            = ownerv; \
        frame.next_shared_info = next_shared_infov; \
        frame.paddr            = paddrv; \
        frame.info_mode        = info_modev; \
    }

/*
 Initializes the array of page frame info structures representing the physical
 pages within a predefined memory region. This function sets up each page frame
 info structure with a sequential physical address, starting from a defined
 base, and links each to its subsequent frame to form a single-linked list of
 free page frames. It also initializes a parallel array for shared
 page frames, albeit without any owner or physical address, to be used for
 tracking shared page frame information.
*/
void initialize_page_frame_infos(void)
{
    uint32_t paddr = PAGING_AREA_MIN_PADDR;

    page_free_head = &page_frame_info[0];
    set_frame_info(page_frame_info[0], NULL, NULL, (uintptr_t *) paddr, 0);
    set_frame_info(page_frame_info_shared[0], NULL, NULL, NULL, 0);

    for (int i = 1; i < PAGEABLE_PAGES; i++) {
        paddr += PAGE_SIZE;
        set_frame_info(
                page_frame_info[i], NULL, NULL, (uintptr_t *) (paddr), 0
        );
        set_frame_info(page_frame_info_shared[i], NULL, NULL, NULL, 0);

        page_frame_info[i - 1].next_free_page = &page_frame_info[i];
    }
    page_frame_info[PAGEABLE_PAGES - 1].next_free_page = NULL;
}

/*
Adds a page frame, identified by its physical address, to the head of the
global free page frame list and calculates the index of the page
frame based on its physical address and then links it to the current head of
the free list, making it the new head. It is the responsibility of the caller
to ensure that access to this function is synchronized using the provided
spinlock mechanisms to avoid race conditions.
 */
void add_page_frame_to_free_list_info(uintptr_t *paddress)
{
    // Convert 'address' to its corresponding index in the free_pages array
    int index = calculate_info_index(paddress);

    page_frame_info_t *page_frame = &page_frame_info[index];
    // Prepend this page_frame to the start of the free list
    page_frame->next_free_page = page_free_head;
    page_free_head = page_frame;
}

/*
Removes and returns the first page frame from the global free list of page
frames. When a page frame is removed from the free list, the function updates the head of the list to the
next page frame in the list and clears the 'next' pointer of the removed frame.
 */
page_frame_info_t *remove_page_frame_from_free_list_info()
{
    if (page_free_head == NULL) {
        // No free page frames available
        return NULL;
    }
    // Take the first page frame from the free list
    page_frame_info_t *allocated_page_frame = page_free_head;
    // Move head to the next frame
    page_free_head = allocated_page_frame->next_free_page;
    allocated_page_frame->next_free_page = NULL;
    return allocated_page_frame;
}

/*
 * "Hashing" function carrying physical page addresses into the info struct
 * array
 */
uint32_t calculate_info_index(uintptr_t *paddr)
{
    return ((uint32_t) paddr - PAGING_AREA_MIN_PADDR) / PAGE_SIZE;
}

/*
 * Insert or update page frame information in the info arrays.
 */
page_frame_info_t *insert_page_frame_info(
        uintptr_t *paddr,
        uintptr_t *vaddr,
        pcb_t     *owner_pcb,
        uint32_t   info_mode
)
{
    uint32_t           index = calculate_info_index(paddr);
    page_frame_info_t *info  = &page_frame_info[index];

    // align vaddr to lower (virtual) page boundary
    vaddr = (uint32_t *) (((uint32_t) vaddr) & 0xfffff000);

    // If the page frame is already occupied, manage the shared info structure.
    if (info->owner) {
        if (MEMDEBUG) {
                pr_log(
                    "Shared page at physical address %u, already occupied by PID "
                    "%u",
                    (uint32_t) paddr, info->owner->pid
            );
        }

        if ( !(info->owner -> is_thread) ) {
            assertk(PAGING_AREA_MIN_PADDR <= (uintptr_t) vaddr && (uintptr_t) vaddr < PAGING_AREA_MAX_PADDR);
        }

        // Traverse to the last info struct in the chain.
        page_frame_info_t *last_info = info;
        while (last_info->next_shared_info) {
            last_info = last_info->next_shared_info;
        }

        // Try to find a free slot in the shared info array.
        for (int i = 0; i < PAGEABLE_PAGES; ++i) {
            page_frame_info_t *shared_info = &page_frame_info_shared[i];
            if (!shared_info->owner) {
                last_info->next_shared_info = shared_info;
                shared_info->owner          = owner_pcb;
                shared_info->paddr          = paddr;
                shared_info->vaddr          = vaddr;
                shared_info->info_mode      = info_mode;
                return shared_info;
            }
        }
        // No suitable slot found for the new page info.
        nointerrupt_enter();
        pr_error(
                "No available shared info slots for physical address %u",
                (uint32_t) paddr
        );
        nointerrupt_leave();
        return NULL;
    } else {
        // If the page is not occupied, initialize the main info struct.
        info->owner            = owner_pcb;
        info->next_shared_info = NULL;
        info->vaddr            = vaddr;
        info->info_mode        = info_mode;
        return info;
    }
}

static spinlock_t pinned_pages_counter_lock = SPINLOCK_INIT;

void inc_pinned_pages(int increment)
{
    spinlock_acquire(&pinned_pages_counter_lock);
    number_of_pinned_page_frames += increment;
    spinlock_release(&pinned_pages_counter_lock);
}

/* === Info structure to keep track on fifo queue === */
typedef struct {
    uint32_t next_in, next_out;
    uint32_t queue[PAGEABLE_PAGES];
    uint32_t items;
} FIFOQueue;

static FIFOQueue fifo_queue;

void fifo_init()
{
    fifo_queue.next_in  = 0;
    fifo_queue.next_out = 0;
    fifo_queue.items    = 0;
    for (int i = 0; i < PAGEABLE_PAGES; i++) {
        fifo_queue.queue[i] = 0;
    }
}

bool fifo_is_empty()
{
    return fifo_queue.items == 0;
}

bool fifo_is_full()
{
    return (fifo_queue.next_in + 1) % PAGEABLE_PAGES == fifo_queue.next_out;
    // bool is_full;
    // spinlock_acquire(&pinned_pages_counter_lock);
    // is_full =
    //         fifo_queue.items == PAGEABLE_PAGES - number_of_pinned_page_frames;
    // spinlock_release(&pinned_pages_counter_lock);
    // return is_full;
}

bool fifo_enqueue(uint32_t item)
{
    if (fifo_is_full()) {
        return false;
    }
    fifo_queue.queue[fifo_queue.next_in] = item;
    fifo_queue.next_in = (fifo_queue.next_in + 1) % PAGEABLE_PAGES;
    fifo_queue.items++;
    return true;
}

uint32_t fifo_dequeue(int *error)
{
    if (MEMDEBUG) pr_log("fifo_dequeue: fifo_queue.items = %u\n", fifo_queue.items);
    if (fifo_is_empty()) {
        *error = 1; // Indicates queue is empty
    }
    uint32_t item       = fifo_queue.queue[fifo_queue.next_out];
    fifo_queue.next_out = (fifo_queue.next_out + 1) % PAGEABLE_PAGES;
    fifo_queue.items--;
    if (MEMDEBUG) pr_log("fifo_dequeue next.out index = %u\n", fifo_queue.next_out);

    return item;
}

void fifo_enqueue_info(uint32_t *paddr)
{
    
    uint32_t info_index = calculate_info_index(paddr);
    int      pinned = page_frame_info[info_index].info_mode & PE_INFO_PINNED;
    if (!pinned) {
        fifo_enqueue(info_index);
    }
}

uint32_t *fifo_dequeue_info(int *error)
{
    uint32_t fifo_index = fifo_dequeue(error);
    if (error) {
        return NULL;
    }

    uint32_t *paddr = page_frame_info[fifo_index].paddr;
    return paddr;
}



/*
 *  Allocates a page of memory by removing a page_info_frame from the list
 *  of free pages and returning a pointer of it to the user.
 */
uint32_t *allocate_page_internal()
{
    uint32_t *paddr = NULL;
    page_frame_info_t *page_info_frame =
            remove_page_frame_from_free_list_info();

    // zero out the page
    if (page_info_frame) {
        paddr = page_info_frame->paddr;
        for (int i = 0; i < PAGE_SIZE; i++) {
            *(paddr + i) = 0;
        }
    }
    return paddr;
}

uint32_t *allocate_page()
{
    uint32_t *paddr = allocate_page_internal();
    if (paddr == NULL) {
        // page table full, evict
        nointerrupt_enter();
        if (MEMDEBUG) pr_log("allocate_page: page table full, evicting page\n");
        paddr = try_evict_page();
        if (MEMDEBUG) pr_log("allocate_page: evicted page frame %p\n", paddr);
        nointerrupt_leave();
    }
    if (!paddr) {
        nointerrupt_enter();
        pr_error("allocate_page: couldn't allocate a page!... aborting!\n");
        nointerrupt_leave();
        abortk();
    } else {
        nointerrupt_enter();
        if (MEMDEBUG) pr_log("allocate_page: allocated page frame %p \n", paddr);
        nointerrupt_leave();
    }
    return paddr;
}

void page_clear_info(uintptr_t *paddr)
{
    page_frame_info_t *info_frame, *next_shared_info;
    uint32_t           info_index = calculate_info_index(paddr);
    info_frame                    = &page_frame_info[info_index];

    // reset the frame to initial state
    do {
        info_frame->owner     = NULL;
        info_frame->vaddr     = NULL;
        info_frame->info_mode = 0;

        // transfer to next shared info frame
        next_shared_info             = info_frame->next_shared_info;
        info_frame->next_shared_info = NULL;
        info_frame                   = next_shared_info;
    } while (info_frame);
}

void page_free(uintptr_t *paddr, int evict)
{
    page_frame_info_t *info_frame, *next_shared_info;

    uint32_t info_index = calculate_info_index(paddr);
    info_frame          = &page_frame_info[info_index];

    // reset the frame to initial state
    uint32_t vaddr;
    do {
        if (info_frame->owner == dummy_kernel_pcb
            || info_frame->owner->page_directory == kernel_pdir) {
            nointerrupt_enter();
            pr_error("tried to free a kernel page, aborting!!");
            nointerrupt_leave();
            abortk();
        }
        vaddr = (uint32_t) info_frame->vaddr;

        nointerrupt_enter();
        if (evict) {
            // unmaps the page and invalidates it
            unmap_physical_page(info_frame->owner->page_directory, vaddr);
        } else {
            //spinlock_acquire(&page_frame_info_lock);
            add_page_frame_to_free_list_info(paddr);
            //spinlock_release(&page_frame_info_lock);
        }
        nointerrupt_leave();

        info_frame->owner     = NULL;
        info_frame->vaddr     = NULL;
        info_frame->info_mode = 0;

        // transfer to next shared info frame
        next_shared_info             = info_frame->next_shared_info;
        info_frame->next_shared_info = NULL;
        info_frame                   = next_shared_info;
    } while (info_frame);
}

// current bug: locks cause process to skip scheduler entry or return from it
// solution is probably to use a cleaner thread/daemon like suggested in Tanenbaum MOS.
// void free_done_process_memory(pcb_t *p) 
// {
//     // uint32_t table_index, dir_index, vaddr, *table;
//     // uint32_t dir_entry, table_entry, old_mode;
//     // // the quick and dirty way - linear search through entire pageable area,
//     // // mark all pages belonging to this process as not pinned.
//     // lock_acquire(&page_map_lock);
//     // for (int i=0; i < PAGEABLE_PAGES; i++) {
//     //     if (page_frame_info[i].owner == p) {
// 
//     //         
//     //         // slight modification of page_set_mode to instead just clear dirty  bit,
//     //         // because we don't want to write pages of finished processes to disc.
//     //         // #################################################################
//     //         // vaddr = (uint32_t) page_frame_info[i].vaddr;
//     //         // dir_index = get_directory_index(vaddr);
//     //         // table_index = get_table_index((uint32_t) vaddr);
// 
//     //         // // table_index = get_table_index(vaddr);
//     //         // // table = p -> page_directory[dir_index] & PE_BASE_ADDR_MASK;
// 
//     //         // dir_entry = p->page_directory[dir_index];
//     //         // assertk(dir_entry & PE_P); /* dir entry present */
//     //         // table = (uint32_t *) (dir_entry & PE_BASE_ADDR_MASK);
//     //         // table_entry = table[table_index];
// 
//     //         // /* set table[index] bits 11..0 */
//     //         // old_mode = table_entry & PE_BASE_ADDR_MASK;
//     //         // table_entry |= (old_mode & ~(PE_D)) & ~PE_BASE_ADDR_MASK;
//     //         // table[table_index] = table_entry;
//     //         // // #################################################################
// 
// 
//     //         // put pinned pages to the fifo-queue so that they become available
//     //         // for paging/swapping.
//     //         if (page_frame_info[i].info_mode & PE_INFO_PINNED) {
//     //             fifo_enqueue(i);
//     //             page_frame_info[i].info_mode &=~(PE_INFO_PINNED);
//     //         }
//     //         // unpin the page frame, page fault handler can now swap it when added to the fifo queue
//     //         //page_frame_info[i].info_mode &=~(PE_INFO_PINNED | PE_INFO_KERNEL_DUMMY | PE_INFO_USER_MODE);
//     //     }
//     // }
//     // lock_release(&page_map_lock);
// }


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
    //if (pdir == current_running -> page_directory) invalidate_page((uint32_t *) vaddr);
}

/* Debug-function.
 * Write all memory addresses and values by with 4 byte increment to
 * output-file. Output-file name is specified in bochsrc-file by line:
 * com1: enabled=1, mode=file, dev=serial.out where 'dev=' is output-file.
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

/* Helper function to decode info_mode flags into a readable string. */
const char *decode_info_mode(uint32_t info_mode)
{
    static char buffer[128];
    buffer[0] = '\0'; // Initialize buffer to an empty string

    if (info_mode & PE_INFO_USER_MODE) strcat(buffer, "USER ");
    if (info_mode & PE_INFO_KERNEL_DUMMY) strcat(buffer, "KERNEL ");
    if (info_mode & PE_INFO_PINNED) strcat(buffer, "PINNED ");
    if (info_mode & PE_INFO_STACK) strcat(buffer, "STACK");

    if (buffer[0] == '\0') return "NONE";
    return buffer;
}

/* Prints a table of virtual and physical addresses for managed pages. */
void print_page_table_info(void)
{
    /* clang-format off */
    pr_log("Page Table Info:\n");
    pr_log(
            "%-5s | %-12s | %-15s | %-15s | %-20s\n", 
            "Index", "Owner PID", "Virtual Addr", "Physical Addr", "Info Mode"
    );
    pr_log("------------------------------------------------------------------------------------\n");

    for (int i = 0; i < PAGEABLE_PAGES; i++) {
        page_frame_info_t *info = &page_frame_info[i];

        // Check if the page frame is used
        if (info->owner != NULL) {
            uint32_t owner_pid = info->owner->pid; 
            pr_log(
                    "%-5d | %-12u | 0x%013x | 0x%013x | %-20s\n", 
                    i, owner_pid, (uint32_t) info->vaddr, (uint32_t) info->paddr,
                    decode_info_mode(info->info_mode)
            );
        } else if (info->next_free_page != NULL) {
            // If the page is free, indicate as such
            pr_log(
                    "%-5d | %-12s | %-15s | 0x%013x | %-20s\n", 
                    i, "FREE NEXT", "", (uint32_t) info->paddr, decode_info_mode(info->info_mode)
            );
        } else {
            // The page is not used and not part of the free list
            pr_log(
                    "%-5d | %-12s | %-15s | 0x%013x | %-20s\n",
                    i, "UNUSED", "", (uint32_t) info->paddr, decode_info_mode(info->info_mode)
            );
        }
    }
    /* clang-format on*/
}

/* Prints the contents of the FIFO queue, showing physical addresses in
 * extended format and marking the positions of next_in and next_out. */
void print_fifo_queue()
{
    if (fifo_is_empty()) {
        pr_log("print_fifo_queue: FIFO Queue is empty.\n");
        return;
    }

    uint32_t current = fifo_queue.next_out;
    pr_log("print_fifo_queue: FIFO Queue contents:\n");
    pr_log("Index | Physical Address      | Position\n");
    pr_log("----------------------------------------\n");

    do {
        // Convert page index back to physical address for clarity, if needed
        uint32_t paddr_index = fifo_queue.queue[current];
        uintptr_t *current_paddr = page_frame_info[paddr_index].paddr;
        const char *position = "";
        if (current == fifo_queue.next_out) {
            position = "next_out";
        } 
        if (current == fifo_queue.next_in) {
            position = (strcmp(position, "next_out") == 0) ? "next_out, next_in" : "next_in";
        }

        pr_log("%5d | 0x%016x | %s\n", current, (uint32_t)current_paddr, position);

        if (current == fifo_queue.next_in) {
            break; // If current has reached rear, break the loop
        }
        current = (current + 1) % PAGEABLE_PAGES; // Cycle through the queue
    } while (current != fifo_queue.next_out);

    pr_log("----------------------------------------\n\n");
}

uint32_t* user_kernel_ptable;

static void user_setup_kernel_vmem(uint32_t *pdir, int first_time)
{
    uint32_t user_mode = PE_P | PE_RW | PE_US; // Access mode for user pages.
    uint32_t kernel_mode = PE_P | PE_RW; // Access mode for kernel pages.

    // Identity map the entire kernel region.
    for (uint32_t paddr = 0; paddr < KERNEL_SIZE; paddr += PAGE_SIZE) {
        if (first_time) table_map_page(user_kernel_ptable, paddr, paddr, kernel_mode);
        dir_ins_table(pdir, paddr, user_kernel_ptable, kernel_mode);
    }

    // Extend the identity mapping to the entire physical memory range managed by the kernel.
    for (uint32_t paddr = PAGING_AREA_MIN_PADDR; paddr < PAGING_AREA_MAX_PADDR; paddr += PAGE_SIZE) {
        if (first_time) table_map_page(user_kernel_ptable, paddr, paddr, kernel_mode);
        dir_ins_table(pdir, paddr, user_kernel_ptable, kernel_mode);
    }

    dir_ins_table(pdir, 0, user_kernel_ptable, kernel_mode);

    // Map the video memory from 0xB8000 to 0xB8FFF.
    int mode = user_mode;
    mode |= PE_PCD; 
    if (first_time) table_map_page(user_kernel_ptable, (uint32_t)VGA_TEXT_PADDR, (uint32_t)VGA_TEXT_PADDR, mode);
    dir_ins_table(pdir, VGA_TEXT_PADDR, user_kernel_ptable, mode);
}




/* === Kernel and process setup === */

static void setup_kernel_vmem_common(pcb_t *pcb, uint32_t *pdir, int is_user)
{
    uint32_t user_mode = PE_P | PE_RW | PE_US; // Access mode for user pages.
    uint32_t kernel_mode = PE_P | PE_RW; // Access mode for kernel pages.

    uint32_t *kernel_ptable = allocate_page();


    // Conditionally apply the PE_INFO_PINNED flag depending on the pid of the process
    uint32_t kernel_page_info_flags = PE_INFO_PINNED;
    insert_page_frame_info(kernel_ptable, kernel_ptable, pcb, kernel_page_info_flags);

    inc_pinned_pages(1); 

    // Identity map the entire kernel region.
    for (uint32_t paddr = 0; paddr < KERNEL_SIZE; paddr += PAGE_SIZE) {
        table_map_page(kernel_ptable, paddr, paddr, kernel_mode);
        dir_ins_table(pdir, paddr, kernel_ptable, kernel_mode);
    }

    // Extend the identity mapping to the entire physical memory range managed by the kernel.
    for (uint32_t paddr = PAGING_AREA_MIN_PADDR; paddr < PAGING_AREA_MAX_PADDR; paddr += PAGE_SIZE) {
        table_map_page(kernel_ptable, paddr, paddr, kernel_mode);
        dir_ins_table(pdir, paddr, kernel_ptable, kernel_mode);
    }

    dir_ins_table(pdir, 0, kernel_ptable, kernel_mode);

    // Map the video memory from 0xB8000 to 0xB8FFF.
    int mode = (is_user ? user_mode : kernel_mode);
    mode |= PE_PCD; // dont cache video memory
    pr_log("user mode ?? %d\n", mode & PE_US);
    table_map_page(kernel_ptable, (uint32_t)VGA_TEXT_PADDR, (uint32_t)VGA_TEXT_PADDR, mode);
    dir_ins_table(pdir, VGA_TEXT_PADDR, kernel_ptable, mode);
}

static void setup_kernel_vmem(void)
{
    lock_acquire(&page_map_lock);
    dummy_kernel_pcb->is_thread = 1;
    uint32_t info_mode          = PE_INFO_PINNED | PE_INFO_KERNEL_DUMMY;
    kernel_pdir = allocate_page();
    insert_page_frame_info(
            kernel_pdir, kernel_pdir, dummy_kernel_pcb, info_mode
    );
    inc_pinned_pages(1);
    setup_kernel_vmem_common(dummy_kernel_pcb, kernel_pdir, 0);

    if (PROCESSES_SHARE_KERNEL_PAGE_TABLE) {
        user_kernel_ptable = allocate_page();
        uint32_t kernel_page_info_flags = PE_INFO_PINNED | PE_INFO_KERNEL_DUMMY;
        insert_page_frame_info(user_kernel_ptable, user_kernel_ptable, dummy_kernel_pcb, kernel_page_info_flags);
        inc_pinned_pages(1); 
    }

    lock_release(&page_map_lock);
}

void setup_process_vmem(pcb_t *p)
{
    lock_acquire(&page_map_lock);
    pr_log("setup_process_vmem: setting up new process memory with pid: %u\n", p->pid);

    if (p->is_thread) {
        p->page_directory = kernel_pdir;
        pr_debug("setup_process_vmem: Done. Set up a new kernel thread with pid %u\n", p->pid);
        lock_release(&page_map_lock);
        return;
    }

    uint32_t base_page_info_flags = PE_INFO_USER_MODE | PE_INFO_PINNED;
    uint32_t *proc_pdir = allocate_page();
    insert_page_frame_info(proc_pdir, proc_pdir, p, base_page_info_flags); // Conditionally pin the page directory
    uint32_t user_mode = PE_RW | PE_US;

    if (first_process) {
        nointerrupt_enter();
        first_process_pid = p -> pid;
        first_process = 0;
        nointerrupt_leave();
        // Ensure the kernel's virtual address space is mapped into the process's space
        if (PROCESSES_SHARE_KERNEL_PAGE_TABLE) user_setup_kernel_vmem(proc_pdir, 1);
    } else {
        if (PROCESSES_SHARE_KERNEL_PAGE_TABLE) user_setup_kernel_vmem(proc_pdir, 0);
    }
    if (!PROCESSES_SHARE_KERNEL_PAGE_TABLE) setup_kernel_vmem_common(p, proc_pdir, 1);


    uint32_t *proc_ptable = allocate_page();
    dir_ins_table(proc_pdir, PROCESS_VADDR, proc_ptable, user_mode | PE_P);
    insert_page_frame_info(proc_ptable, proc_ptable, p, base_page_info_flags); // pin the page table
    inc_pinned_pages(2); 

    uint32_t paddr, vaddr, offset;
    uint32_t upper_lim = ((p->swap_size) * SECTOR_SIZE);
    pr_debug("setup vmem: upper_lim = %u bytes = %u pages \n", upper_lim, upper_lim / PAGE_SIZE);
    for (offset = 0; offset < upper_lim; offset += PAGE_SIZE) {
        paddr = p->swap_loc * SECTOR_SIZE + offset;
        vaddr = PROCESS_VADDR + offset;
        nointerrupt_enter();
        pr_debug("setup vmem: mapped vaddr 0x%08x to to user space for process pid %u\n", vaddr, p->pid);
        nointerrupt_leave();
        table_map_page(proc_ptable, vaddr, paddr, user_mode);
        //dir_ins_table(proc_pdir, vaddr, proc_ptable, user_mode | PE_P);
    }

    // Allocate pages for the stack area assuming a fixed size stack area for each process
    uint32_t *stack_page, stack_vaddr;
    uint32_t stack_lim  = PROCESS_STACK_VADDR - USER_STACK_SIZE;
    uint32_t stack_base = PROCESS_STACK_VADDR;
    dir_ins_table(proc_pdir, stack_base, proc_ptable, user_mode | PE_P);

    //uint32_t *stack_page_table = allocate_page();
    //insert_page_frame_info(stack_page_table, stack_page_table, p, base_page_info_flags | PE_INFO_STACK);
    //inc_pinned_pages(1);
    for (stack_vaddr = stack_base; stack_vaddr > stack_lim; stack_vaddr -= PAGE_SIZE) {
        stack_page = allocate_page();
        insert_page_frame_info(
            stack_page, (uintptr_t *) stack_vaddr, p,
            base_page_info_flags | PE_INFO_STACK  // Apply the pinning flags consistently for stack pages
        );
        inc_pinned_pages(1);     // Allocate pages for the stack area assuming a fixed size stack area for each process

        pr_debug(
            "allocated a stack frame with vaddr=%x at paddr %x, proceeding to map \n",
            (uint32_t) stack_vaddr, (uint32_t) stack_page
        );
        table_map_page(proc_ptable, stack_vaddr, (uint32_t) stack_page, user_mode | PE_P);
        //dir_ins_table(proc_pdir, stack_vaddr, proc_ptable, user_mode | PE_P);

        //table_map_page(stack_page_table, stack_vaddr, (uint32_t) stack_page, user_mode | PE_P);
        //dir_ins_table(proc_pdir, stack_vaddr, stack_page_table, user_mode | PE_P);
        
    } 

    // table_map_page(proc_ptable, stack_vaddr, (uint32_t) stack_page, user_mode | PE_P);
    // dir_ins_table(proc_pdir, stack_vaddr, proc_ptable, user_mode | PE_P);
    // table_map_page(stack_page_table, stack_vaddr, (uint32_t) stack_page, user_mode | PE_P);
    // dir_ins_table(proc_pdir, stack_vaddr, stack_page_table, user_mode | PE_P);

    p->page_directory = proc_pdir;
    pr_debug("setup_process_vmem: done setup for process pid %u\n", p->pid);
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
    initialize_page_frame_infos();
    fifo_init();
    next_free_mem = PAGING_AREA_MIN_PADDR;
    setup_kernel_vmem();
}



uint32_t *select_page_for_eviction_v2()
{
    int      error_empty_queue = 0;
    uint32_t *paddr = fifo_dequeue_info(&error_empty_queue);
    if (error_empty_queue) {
        nointerrupt_enter();
        pr_error(
                "select_page_for_eviction: FIFO queue is empty, no page "
                "available for eviction\n"
        );
        nointerrupt_leave();
        abortk();
    }
    if (paddr == NULL) {
        pr_error(
                "invalid address, physical page is NULL!\n"
        );

    }
    if (paddr < (uint32_t *) PAGING_AREA_MIN_PADDR) {
        pr_error(
                "invalid address, physical page frame address in kernel area!\n"
        );

    }
    return paddr;
}


/*
FIFO eviction algorithm
*/
uint32_t *select_page_for_eviction_v1()
{
    int      error_empty_queue = 0;
    uint32_t index             = fifo_dequeue(&error_empty_queue);
    if (error_empty_queue) {
        nointerrupt_enter();
        pr_error(
                "select_page_for_eviction: FIFO queue is empty, no page "
                "available for eviction\n"
        );
        nointerrupt_leave();
        abortk();
    }

    spinlock_acquire(&page_frame_info_lock);
    uint32_t *paddr = page_frame_info[index].paddr;
    spinlock_release(&page_frame_info_lock);

    if (paddr == NULL) {
        pr_error(
                "invalid address, physical page is NULL!\n"
        );

    }
    if (paddr < (uint32_t *) PAGING_AREA_MIN_PADDR) {
        pr_error(
                "invalid address, physical page frame address in kernel area!\n"
        );

    }
    return paddr;
}


static uint32_t random_index_val = 45;
/* modfied from precode rand*/
int random_index_generator(void)
{
    random_index_val = random_index_val * 1103515245 + 12345 + (read_cpu_ticks() % 2397);
    return (random_index_val / 65536) % PAGEABLE_PAGES;
}


uint32_t *evict_random_page()
{
    uint32_t index = random_index_generator();
    while (page_frame_info[index].info_mode & PE_INFO_PINNED) {
        index = random_index_generator();
    }
    return page_frame_info[index].paddr;
}



uint32_t *select_page_for_eviction()
{
    uint32_t* evicted_page;
    if (EVICTION_STRATEGY == EVICTION_STRATEGY_RANDOM) {
        evicted_page = evict_random_page();
    } else if (EVICTION_STRATEGY == EVICTION_STRATEGY_FIFO) {
        evicted_page = select_page_for_eviction_v1();
        //return select_page_for_eviction_v2();
    }
    if (page_frame_info[calculate_info_index(evicted_page)].owner) {
        invalidate_page(page_frame_info[calculate_info_index(evicted_page)].vaddr);
    }
    return evicted_page;
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
    uint32_t *page_table  = (uint32_t *) (dir_entry & PE_BASE_ADDR_MASK);
    uint32_t  table_index = get_table_index(vaddr);
    uint32_t  page_entry  = page_table[table_index];
    return page_entry & PE_D;
}

void unmap_physical_page(uint32_t *process_directory, uint32_t vaddr)
{
    page_set_mode(process_directory, vaddr, 0);
}
/*
 * Given a virtual address and a pointer to the page directory, it
 * returns a pointer to the relevant page table.
 */
uint32_t *get_page_table(uint32_t vaddr, uint32_t *page_directory)
{
    uint32_t dir_index = get_directory_index(vaddr);
    uint32_t dir_entry = page_directory[dir_index];
    if (!(dir_entry & PE_P)) {
        pr_debug(
                "get_page_table: no page table found in page dir %p with "
                "virtual address %u\n",
                page_directory, vaddr
        );
        return NULL;
    }
    uint32_t *page_table = (uint32_t *) (dir_entry & PE_BASE_ADDR_MASK);
    return page_table;
}

char write_buffer[PAGE_SIZE];
lock_t write_buffer_lock = LOCK_INIT;
int write_page_back_to_disk(uint32_t vaddr, pcb_t *pcb, uint32_t* paddr)
{

    /*
     * pcb-> swap_loc is sector number on disk, pcb -> swap_size is number
     * of sectors.
     */

    // Calculate the disk offset based on the physical address.
    int      success     = -1;
    uint32_t disk_offset = (vaddr - PROCESS_VADDR) / PAGE_SIZE;
    disk_offset *= (PAGE_SIZE / SECTOR_SIZE);
    uint32_t write_block_count = MIN(PAGE_SIZE / SECTOR_SIZE, pcb->swap_size - disk_offset);
    uint32_t disk_loc = pcb->swap_loc + disk_offset;

    if (disk_offset > pcb->swap_size) {
        nointerrupt_enter();
        pr_error(
                "too large virtual address for the swap image for pid "
                "%u\n",
                pcb->pid
        );
        nointerrupt_leave();
        return success;
    }

    uint32_t *frameref_table;
    if (!(frameref_table = get_page_table(vaddr, pcb->page_directory))) {
         // nothing to do, no page with this virtual address precent in pcb
         // page dir
         pr_error(
                 "write_page_back_to_disk: no vaddr found in page "
                 "directory\n"
         );
         return -1;
    }
    //uint32_t *frameref = (uint32_t *) (frameref_table[get_table_index(vaddr)] & PE_BASE_ADDR_MASK);
    char *frameref = (char*) (frameref_table[get_table_index(vaddr)] & PE_BASE_ADDR_MASK);
    //for (int i=0; i < PAGE_SIZE; i++) write_buffer[i] = frameref[i];
    //lock_acquire(&write_buffer_lock);
    //nointerrupt_enter();
    //bcopy((char *) frameref, write_buffer, PAGE_SIZE);
    //nointerrupt_leave();
    //success = scsi_write(disk_loc, write_block_count, write_buffer);
    //success = scsi_write(disk_loc, write_block_count, frameref);
    //lock_release(&write_buffer_lock);

    //success = scsi_write(disk_loc, write_block_count, (char *) frameref);

    // for(int i=0; i < PAGE_SIZE; i++) write_buffer[i] = paddr[i];
    // nointerrupt_enter();
    // bcopy((char *) paddr, write_buffer, PAGE_SIZE);
    // nointerrupt_leave();
    // success = scsi_write(disk_loc, write_block_count, write_buffer);
    
    
    lock_acquire(&write_buffer_lock);
    success = scsi_write(disk_loc, write_block_count, (char *) paddr);
    //if (pcb == current_running) invalidate_page((uintptr_t *)vaddr);
    invalidate_page((uintptr_t *)vaddr);
    lock_release(&write_buffer_lock);
    
    /* clang-format off */
    if (MEMDEBUG) {
        pr_log( "write_page_back_to_disk: Page at virtual address 0x%08x referencing frame %0x08x written back to disk at offset 0x%08x = %u for pid %u\n", 
                                                                    vaddr, (uint32_t) frameref, disk_offset, disk_offset,  pcb->pid);
    }
    /* clang-format on */
    return success;
}

#define load_page_pr_log(pid, vaddr, disk_offset, swap_size) \
    { \
        /* clang-format off */ \
    pr_log("load_page_from_disk: Start loading from disk for PID %u, VADDR %p\n", pid, vaddr);\
    pr_log("load_page_from_disk: Calculated page number %u from VADDR %p\n", disk_offset, vaddr);\
    pr_log("load_page_from_disk: Calculated initial disk offset %u sectors\n", disk_offset);\
    pr_log("load_page_from_disk: Calculated swap size %u \n", swap_size); \
        /* clang-format on */ \
    }

char read_buffer[PAGE_SIZE];
lock_t read_buffer_lock = LOCK_INIT;

int disk_loader(int disk_loc, int block_count, uint32_t *frameref) {

    lock_acquire(&read_buffer_lock);

    // clear the read buffer
    for (int i = 0; i < PAGE_SIZE; i++) read_buffer[i] = 0;

    int success = scsi_read(disk_loc, block_count, (void *) read_buffer);
    if (success >= 0) {
        nointerrupt_enter();
        bcopy(read_buffer, (char *) frameref, PAGE_SIZE);
        nointerrupt_leave();
    } else {
        pr_log("failed to read from disk\n");
    }
    lock_release(&read_buffer_lock);
    return success;
}


int load_page_from_disk(uint32_t vaddr, pcb_t *pcb)
{
    /*
     * pcb-> swap_loc is sector number on disk, pcb -> swap_size is number
     * of sectors.
     */

    uint32_t *fault_dir, *frameref_table, *frameref, disk_offset, block_count;
    uint32_t info_mode, mode, disk_loc;

    fault_dir   = pcb->page_directory;
    disk_offset = (vaddr - PROCESS_VADDR) / PAGE_SIZE;
    disk_offset *= (PAGE_SIZE / SECTOR_SIZE);
    //assertk(disk_offset < pcb->swap_size);
    if (disk_offset > pcb->swap_size) {
        pr_debug("allocating extra stack space? vaddr = 0x%08x \n", vaddr);
    }

    // Compute the actual disk location by adding the swap location
    disk_loc = pcb->swap_loc + disk_offset;

    // Determine the number of sectors to read, ensuring not to exceed file boundaries
    block_count = MIN(PAGE_SIZE / SECTOR_SIZE, pcb->swap_size - disk_offset);

    if (MEMDEBUG) {
        nointerrupt_enter();
        load_page_pr_log(pcb->pid, (void *) vaddr, disk_offset, pcb->swap_size);
        nointerrupt_leave();
    }

    mode = PE_P | PE_RW | PE_US;
    if (pcb->is_thread) {
        //info_mode = PE_INFO_PINNED; 
        pr_error("thread pid %u is paging \n", pcb->pid);
        abortk();
    } else {
        info_mode = PE_INFO_USER_MODE;
        mode |= PE_US;
    }

    // // If the process PID is the first one assume it's the
    // shell and pin if PIN_SHELL is set to 1
    if (pcb->pid == first_process_pid && PIN_SHELL) {
        info_mode |= PE_INFO_PINNED; // Add the pinned flag for processes with pid 11
    }

    int success = -1;
    //lock_acquire(&page_map_lock);
    frameref = allocate_page();
    //lock_release(&page_map_lock);

    if (!frameref) {
        nointerrupt_enter();
        pr_error("load_page_from_disk: could not allocate new page... aborting\n");
        nointerrupt_leave();
        abortk();
    } else {
        invalidate_page((uintptr_t *) vaddr);
        success = disk_loader(disk_loc, block_count, frameref);
        invalidate_page((uintptr_t *) vaddr);
    }

    //lock_acquire(&page_map_lock);
    frameref_table = get_page_table(vaddr, fault_dir);
    if (frameref_table == NULL) {
        frameref_table = allocate_page();
        insert_page_frame_info(frameref_table, (uintptr_t *) vaddr, pcb, info_mode);
        inc_pinned_pages(1);
        dir_ins_table(fault_dir, vaddr, frameref_table, mode);
    }

    if (success < 0) {
        pr_error("load_page_from_disk: Failed to read from disk sector %u\n", disk_loc);
        lock_release(&page_map_lock);
        return success;
    }

    if ( !(info_mode & PE_INFO_PINNED) && EVICTION_STRATEGY == EVICTION_STRATEGY_FIFO) {
        fifo_enqueue_info(frameref);
    }
    insert_page_frame_info(frameref, (uintptr_t *) vaddr, pcb, info_mode); 
    table_map_page(frameref_table, vaddr, (uint32_t) frameref, mode);
    dir_ins_table(fault_dir, vaddr, frameref_table, mode);

    invalidate_page((uintptr_t *) vaddr);
    set_page_directory(pcb->page_directory);
    //lock_release(&page_map_lock);

    nointerrupt_enter();
    pr_log("load_page_from_disk: Loaded page at virtual address 0x%08x with disk offset 0x%08x = %u from disk into physical address 0x%08x for pid = %u\n",
           vaddr, disk_offset, disk_offset, (uint32_t) frameref, pcb -> pid);
    nointerrupt_leave();

    return success;
}


/*
 * Processes all page tables with a reference to the physical address
 * paddr. Checks if the pages are dirty Returns -1 if paddr must not be
 * evicted Returns 0 when not dirty Returns 1 when dirty
 */
int page_frame_check_dirty(uintptr_t *paddr)
{
    uint32_t           info_index, *current_pdir, vaddr;
    page_frame_info_t *info_frame;

    info_index = calculate_info_index(paddr);
    info_frame = &page_frame_info[info_index];
    int dirty  = 0;

    while (info_frame && info_frame->owner) {
        current_pdir = info_frame->owner->page_directory;
        vaddr        = (uint32_t) info_frame->vaddr;

        if (!(info_frame->info_mode & PE_INFO_KERNEL_DUMMY)) {
            dirty += is_page_dirty(vaddr, current_pdir);
        } else {
            // trying to evict kernel page dir
            // frame should be pinned and this should never happen!
            nointerrupt_enter();
            pr_error("Trying to evict kernel page directory \n");
            nointerrupt_leave();
            return -1;
        }

        info_frame = info_frame->next_shared_info;
    }

    return dirty;
}

/*
 *  Returns a physical address to an evicted page frame
 */
uint32_t *try_evict_page_v2()
{
    uintptr_t         *page_frame_ref = NULL; // physical page frame
    uint32_t           info_index;
    page_frame_info_t *frame_info;

    // index into information structs array relating
    // physical addresses to processes and the
    // relevant virtual address
    if (!(page_frame_ref = select_page_for_eviction())) {
        nointerrupt_enter();
        pr_error("try_to_evict: No suitable page found for eviction\n");
        nointerrupt_leave();
        return NULL;
    }

    info_index = calculate_info_index(page_frame_ref);
    frame_info = &page_frame_info[info_index];

    if (frame_info->info_mode & PE_INFO_PINNED) {
        nointerrupt_enter();
        pr_error("try_to_evict: suggested evicting pinned page\n");
        nointerrupt_leave();
        abortk();
    }
    if (frame_info->info_mode & PE_INFO_KERNEL_DUMMY) {
        nointerrupt_enter();
        pr_error("try_to_evict: suggested evicting kernel page\n");
        nointerrupt_leave();
        abortk();
    }


    uint32_t vaddr = (uint32_t) frame_info->vaddr;

    if (frame_info -> owner == current_running) {
        invalidate_page((uintptr_t *) vaddr);
    }

    // check if page frame is dirty
    if (page_frame_check_dirty(page_frame_ref)) {
        write_page_back_to_disk(vaddr, frame_info->owner, frame_info->paddr);
    }

    // puts this page frame back in the free list,
    // resets the info data and invalidates the page frame
    // in all the page directories referencing it.
    page_free(frame_info->paddr, 1);

    return page_frame_ref;
}


uint32_t *try_evict_page()
{
    return try_evict_page_v2();
    // return try_evict_page_v1();
}


inline void log_interrupt_frame(struct interrupt_frame *stack_frame)
{
   pr_log("instruction pointer:     %08x\n", stack_frame -> ip);
   pr_log("code segment selector:   %08x\n", stack_frame -> cs);
   pr_log("flags:                   %08x\n", stack_frame -> flags);

   pr_log("stack pointer:           %08x\n", stack_frame -> sp);
   pr_log("stack segment selector:  %08x\n", stack_frame -> ss);
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
 *  Error code is the last 3 bits masked as error_code & 0x7, recall 0x7 =
 * 0b111
 *
 * U/S bit: 1 when processor was executing in user mode, 0 when in
 * supervisor mode W/R bit: 1 for write, 0 for read P/V bit: 1 for
 * page-level protection violation, 0 for page not present
 */

#define ec_privilige_level(error_code)     ((error_code) & 0x4) >> 2
#define ec_write(error_code)               ((error_code) & (1 << 1)) >> 1
#define ec_page_not_present(error_code)    !((error_code) & 0x1)
#define ec_privilige_violation(error_code) ((error_code) & 0x1)

// control wether multiple page faults should be handled at once or queued up
lock_t page_fault_debug_lock = LOCK_INIT;

/*
 * Handle page fault
 */
void page_fault_handler(struct interrupt_frame *stack_frame, ureg_t error_code)
{
    uint32_t *fault_address   = (uint32_t *) load_page_fault_addr();
    uint32_t *fault_directory = (uint32_t *) load_current_page_directory();
    invalidate_page(fault_address);
    pcb_t *fault_pcb = current_running;
    fault_pcb -> page_fault_count++;

    if (MEMDEBUG) {
        pr_log("\n\n\n\n\n\n\n");
        pr_log("page_fault_handler: Handling new page fault: error code: %u \n", error_code & 0x7);
        pr_log("page_fault_handler: pid = %u,      pcb->page_fault_count = %u\n", fault_pcb->pid, fault_pcb->page_fault_count);

        pr_log("page_fault_handler: write op? %u \n", (error_code & (1 << 1)) >> 1);

        log_interrupt_frame(stack_frame);

        print_fifo_queue();
        print_page_table_info();

        pr_log("page_fault_handler: fault address: 0x%013x \n", (uint32_t)fault_address);
        pr_log("page_fault_handler: fault directory 0x%013x \n", (uint32_t)fault_directory);
        pr_log("page_fault_handler: fault_pcb -> page_directory 0x%013x \n", (uint32_t)fault_pcb->page_directory);

        pr_log("PAGING_AREA_MAX_PADDR = 0x%013x\n", PAGING_AREA_MAX_PADDR);
    }

    if (ec_privilige_violation(error_code)) {
        // abort - access violation
        pr_error("page_fault_handler: privilege error, virtual address: %p \n", fault_address);
        abortk();
    } else {
        // total_page_fault_page_not_present_counter++;
        //  page not present
        /* clang-format off */
        if (MEMDEBUG) {
            nointerrupt_enter();
            pr_log("page_fault_handler: page not present \n");
            pr_log("page_fault_handler: privilige level (U = 1; S = 0) %u\n", ec_privilige_level(error_code) );
            pr_log("page_fault_handler: read/write operation (W=1, R=0) %u\n", ec_write(error_code) );
            nointerrupt_leave();
            /* clang-format on */
        }
    }

    //lock_acquire(&page_map_lock);

    nointerrupt_leave();

    //lock_acquire(&page_fault_debug_lock);
    lock_acquire(&page_map_lock);
    int success = load_page_from_disk((uint32_t) fault_address, fault_pcb);
    lock_release(&page_map_lock);
    //lock_release(&page_fault_debug_lock);

    nointerrupt_enter();
    if (success >= 0) {
        pr_log(
                "page_fault_handler: loaded page from disk into memory for pid %u,  with page fault count %u\n\n",
                fault_pcb -> pid, fault_pcb -> page_fault_count
        );
        return;
    }

    todo_use(stack_frame);
    todo_use(error_code);
    todo_use(page_map_lock);
    todo_abort();
}
