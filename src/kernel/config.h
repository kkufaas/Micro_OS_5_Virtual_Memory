/*
 * These current settings provide a stable user interface.
 * 
 * -------------------------  unstable settings  ----------------------------------------
 * Unstable: dont limit number of processes
 * When users share kernel page tables, we can somtimes (often)
 * run all 4 processes
 * simultaneously depending on launch order,
 * but cannot use the shell before one of them completes. 
 * 
 * If we start running the plane (1), then 3, 4 and then 2 before 3 and 4 completes,
 * they we get a bad data error
 * Sometimes bad data have been observed in order 1, 2, 3, 4 as well.
 * 
 * With a FIFO strategy we are able pin the shell and
 * launch all processes in order 1, 2, 3, 4 and use the shell commands, except
 * the fire command.
 * However, order 3, 1, 4, 2 (usually) gets a bad data error
 * 
 * Slightly unstable: limit number of processe but don't share kernel page tables
 * If processes don't share a kernel page table, one can run three processes at once
 * when not pinning the shell, but not use the shell to fire bullets from the plane.
 * shell commands like ls seem to work better.
 * --------------------------------------------------------------------------------------
 *
 * 
 *  
 * -------------------------  stable settings  ----------------------------------------
 * Stable: limit number of processes and have processes share page tables.
 * To see paging and swapping in action, spam 'fire' in the shell
 * --------------------------------------------------------------------------------------

 * As explained below, our system is sensitive to thrashing, which we think
 * is due to a bug in our code. The eviciton strategy also seems to influence
 * the occurence of bad data in the mailbox test processes.
*/



///////////////////////////////////////////////////////////////////////////////////////
// some memory settings
////////////////////////////////////////////////////////////////////////////////////////
#define PROCESSES_SHARE_KERNEL_PAGE_TABLE 1
#define PIN_SHELL 0

enum {
    EVICTION_STRATEGY_FIFO = 1,
    EVICTION_STRATEGY_RANDOM = 2,
};
#define EVICTION_STRATEGY EVICTION_STRATEGY_FIFO
////////////////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////////////
// Shell behaviour
///////////////////////////////////////////////////////////////////////////////////////
// There are three or four pinned pages per process depending on sharing strategy.
// Pinned for stack, page directory and user space page table.
// kernel page table is either shared among processes or pinned, depending on strategy.
// On average about three to four pages per process are required to avoid thrashing,
// totaling about 7 pages per process.
// This last number could be modeled better with. eg
// a percentage of the size of the images loaded to memory. The idea is to
// keep competition for page frames at tolerable level to avoid thrashing.

// limits the number of running processes if set to 1
#define SCHEDULE_PROCESS_LAUNCHING 1

#define AVERAGE_PAGES_PER_PROCESS 7
#define NEW_PROCESS_WAIT_TIME_FOR_PAGES 1000 // millisecs
///////////////////////////////////////////////////////////////////////////////////////