# Project 5: Virtual Memory

INF-2201: Operating Systems Fundamentals, \
Spring 2024, \
UiT The Arctic University of Norway

  - [Assignment](#assignment)
      - [Precode Files](#precode-files)
      - [Extra challenges](#extra-challenges)
  - [Details and Hints](#details-and-hints)
      - [Precode Infrastructure](#precode-infrastructure)
      - [Detailed Requirements and
        Hints](#detailed-requirements-and-hints)
      - [Essential Resources](#essential-resources)
  - [Report](#report)
  - [Hand-in](#hand-in)

# Assignment

In this project you will implement a simple **demand-paged virtual
memory system** using the disk image as the swapping area. Each process
must have its own separate *virtual address space*, with proper
protection so that a process may not access or modify data belonging to
another process. Additionally the kernel must also be protected from the
processes in a similar manner.

You will need to do the following:

1.  Set up two-level page tables per process, which should also include
    the entire address space of the kernel. Thus, all kernel threads
    share the same page table.

2.  When page faults happen you will need a *page fault handler* to
    allocate a physical pages and fetch virtual pages from disk.

3.  Implement on a *physical page allocation policy* to allocate
    physical pages. When it runs out of free pages, it should use random
    page replacement to evict a physical page to the disk.

## Precode Files

New and important precode files:

  - ‘+’ indicates that the file is new in this project.
  - **bold** indicates that you will need to edit the file as part of
    the assignment.

| \+ | file                | desc                           |
| -- | ------------------- | ------------------------------ |
|    | kernel/keyboard.c   | Keyboard drivers               |
|    | kernel/keyboard.h   |                                |
|    | kernel/mbox.c       | IPC message passing            |
|    | kernel/mbox.h       |                                |
|    | **kernel/memory.c** | Simple virtual memory          |
|    | kernel/memory.h     |                                |
|    | kernel/pcb.c        | Tasks: Processes and threads   |
|    | kernel/pcb.h        |                                |
|    | kernel/sleep.c      | Support for processes to sleep |
|    | kernel/sleep.h      |                                |
|    | kernel/sync.c       | Sync abstractions (locks, etc) |
|    | kernel/sync.h       |                                |
|    | kernel/usb/         | USB drivers                    |
|    | process/process1.c  |                                |
|    | process/process2.c  |                                |
|    | process/process3.c  | IPC test process: producer     |
|    | process/process4.c  | IPC test process: consumer     |
|    | process/shell.c     | Interactive shell              |

Yes, memory.c is the only file that truly needs to be edited to complete
this assignment. But don’t let that fool you, there is a lot of code to
write in that one file.

## Extra challenges

It is not really possible to give extra credit on a pass/fail
assignment, but attempting these extra challenges will boost your
chances of passing and give you a deeper understanding which may shine
through during the exam.

1.  Implement a FIFO replacement policy. You can expand even more by
    implementing a FIFO with second chance replacement policy. See the
    *Modern Operating Systems (5th ed.)* textbook’s Section 3.4, Page
    Replacement Algorithms, for a catalog of algorithms you may wish to
    try.

2.  Implement a separate swap area on the disk, allowing multiple copies
    of the same program to be run. (See the section Detailed
    Requirements and Hints below for simplified design and why this is
    extra credit.)

# Details and Hints

## Precode Infrastructure

The precode includes a solution to Project 4, including IPC message
passing, keyboard driver, disk I/O drivers, and shell, plus user/kernel
protection mechanisms. However, the basic virtual memory system from
Project 4 has been hollowed out, leaving you to rebuild it as the new
and improved virtual memory system.

What remains in memory.c includes:

  - Parts of the simple physical page allocation system from P4. You may
    reuse and expand on this for your implementation.

  - Low-level utility functions for manipulating the CPU’s paging data
    structures.

  - Skeletons for major virtual memory functions that you must
    implement:
    
      - `void init_memory(void)` — Called on kernel start
      - `void setup_process_vmem(pcb_t *p)` — Called on process creation
      - `void page_fault_handler(...)` — Called on page fault

The precode hooks these skeleton functions into the wider OS. For
example, the PCB struct contains useful fields and the scheduler handles
switching page directories on task-switch (that is, switching address
spaces). Your implementation of `setup_process_vmem(pcb_t *p)` can and
should use these PCB fields.

Also, there is a low-level exception handler implemented for you. The
function `page_fault_handler_entry` in interrupt\_asm.S handles the
low-level state saving and then calls the C `page_fault_handler(...)`
function that you must implement.

Note that the skeleton for `page_fault_handler(...)` re-enables
interrupts. You should keep this feature so that the OS can still handle
hardware IRQs while doing swap I/O.

## Detailed Requirements and Hints

Before starting on this project, you should read the relevant sections
of your textbook and the Intel Architecture manuals (see below) to
understand the virtual address translation mechanism needed to set up
page tables. As described in the manuals, the x86 architecture uses a
two-level page table structure with a directory that has 1024 entries
pointing to the actual page tables. You need to understand this
structure very well.

You can make the following assumptions and constraints to simplify your
design and implementation:

  - The amount of memory (number of page frames) available for paging
    must be limited so that the memory requirements of your modest
    number of processes actually triggers paging. The provided code
    suggests limiting the number of pages to 33 (`PAGEABLE_PAGES`
    constant in addrs.h). This value may need tuning depending on your
    design.

  - You may use the area allocated on the disk for the image file as the
    backing store for each process. That is, you can write process state
    back to the executable.

  - You may assume that processes do not use a heap for dynamic memory
    allocation.

  - You need to figure out how to initially map your physical memory
    pages to run kernel threads and how to load code and data of a
    process into its address space. When the address space for a process
    is initialized, all code and data pages for the process should be
    marked as invalid. This triggers the page fault handler to be
    invoked on the first access to any of these pages.

  - Finally, the page fault handler may deal with only data and code
    pages. All other pages, like user stack and page tables can be
    “pinned” so they will never be swapped out of main memory. This
    means that you need to perform pinning when you initialize a
    process’s address space.

## Essential Resources

  - The *Modern Operating Systems (5th Ed.)* textbook has a wealth of
    useful background information:
    
      - Section 3.3, Virtual Memory, gives general background
        information
    
      - Section 3.6, Implementation Issues, gives more detailed
        background. These subsections might be especially useful:
        
          - 3.6.2 Page Fault Handling
          - 3.6.3 Instruction Backup
          - 3.6.4 Locking Page in Memory \[“pinning”\]
    
      - Section 3.4, Page Replacement Algorithms, will be useful for the
        extra credits

  - The [*Intel® 64 and IA-32 Architectures Software Developer
    Manuals*](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html),
    Volume 3, *System Programming Guide*, has all the x86 implementation
    details (these section numbers are based on the December 2022
    edition of the manuals, newer editions may have different
    numbering):
    
      - Chapter 4, Paging, details the x86’s many paging modes.
        
        Focus on the classic 32-bit, two-level paging (Section 4.3,
        32-bit paging).
        
        The other modes simply add more levels to the paging hierarchy
        in order to map more memory on 64-bit systems:
        
        | Mode           | Levels | Virtual Address             | Physical Address |
        | -------------- | -----: | --------------------------- | ---------------- |
        | 32-bit paging  |      2 | 32 bits (4 GiB)             | 32 bits (4 GiB)  |
        | PAE paging     |      3 | 32 bits (4 GiB)             | 52 bits (4 PiB)  |
        | 4-level paging |      4 | 48 bits canonical (256 TiB) | 52 bits (4 PiB)  |
        | 5-level paging |      5 | 57 bits canonical (128 PiB) | 52 bits (4 PiB)  |
        

          - PAE stands for Physical Address Extension, and it allows
            32-bit virtual addresses to map to larger physical
            addresses. Useful for running 32-bit applications on a
            64-bit operating system.
        
          - The 4- and 5-level paging modes map increasingly large
            64-bit canonical addressess to 52-bit physical addresses.
    
      - Other useful sections include:
        
          - Section 4.6, Access Rights
          - Section 4.7, Page-Fault Exceptions
          - Section 4.8, Accessed and Dirty Flags
          - Section 4.10.4, Invalidation of TLBs and Paging-Structure
            Caches

# Report

You must write a short technical report about your work, to be handed in
with your code. The report should give an overview of how you solved
each task (including extra challenges), how you have tested your code,
and any known bugs or issues. In addition you need to describe the
methodology, results, and conclusions for your performance measurements.

See the [How to Write a Report](how-to-write-a-report.md) document in
this directory for more detailed guidelines and advice.

# Hand-in

This project is a mandatory assignment. It will be graded pass/fail by
TAs, and you must pass in order to gain admittance to the exam.

Hand-in will be via Canvas. You should hand in:

1.  Your report. Place the report PDF in a subdirectory of your
    repository called `report/`

2.  Your code repository. Zip up and hand in the entire repository
    (working tree + `.git/` directory), not just a snapshot of the code.

3.  Also upload a second, separate copy of your report PDF. Having the
    report both in the zip and stand-alone on Canvas makes it easier for
    us to grade.
