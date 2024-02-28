#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

#include "hardware/intctl_8259.h"
#include "interrupt.h"

#define PCB_TABLE_SIZE 128

/*
 * The process control block is used for storing various information
 * about a thread or process
 */
struct pcb {
    /* Queue: PCBs as a doubly-linked list */
    struct pcb     *next;
    struct pcb     *previous;

    /* Basic process info and state */
    uint32_t pid;        /* Process id of this process */
    uint32_t is_thread;  /* Thread or process */
    uint32_t start_pc;   /* Start address of a process or thread */
    uint32_t status;     /* One of the STATUS_* constants */
    uint32_t user_stack; /* Pointer to base of the user stack */
    /*
     * Used to set up kernel stack, and temporary save esp in
     * scheduler()/dispatch()
     */
    uint32_t kernel_stack;

    /* For priority scheduling */
    uint32_t priority; /* This process' priority */

    /*
     * 0: process in user mode
     * 1: process/thread in kernel mode
     * 2: process/thread that was in kernel mode when it got interrupted
     *    (and is still in kernel mode).
     *
     * It is used to decide if we should switch to/from the kernel_stack,
     * in switch_to_kernel/user_stack
     */
    uint32_t nested_count; /* Number of nested interrupts */

    /* Number of times process has been preempted */
    uint32_t preempt_count;

    uint32_t yield_count; /* Number of yields made by this process */

    /* For memory protection */

    /*
     * Pointer to base of the kernel stack (used to restore an
     * empty ker. st.)
     */
    uint32_t  base_kernel_stack;
    uint32_t  ds; /* Data segment selector */
    uint32_t  cs; /* Code segment selector */

    irqmask_t int_controller_mask;

    /*
     * Time at which this process should transition from STATUS_SLEEPING
     * to STATUS_READY.
     */
    uint64_t wakeup_time;

    /* For virtual memory / paging */

    uint32_t *page_directory; /* Virtual memory page directory */
    /* The base physical address of the process this is just for project 4 */
    uint32_t base;
    /* Size of memory allocated for this process (code + data + user stack) */
    uint32_t limit;

};

typedef struct pcb pcb_t;

/* === Get PCB info === */

int getpid(void);

/* === Threads as a doubly-linked ring queue === */

void        queue_insert(struct pcb **q, struct pcb *p);
struct pcb *queue_shift(struct pcb **q);
void        queue_remove(struct pcb **q, struct pcb *p);

/* === PCB Allocation === */

/* An array of pcb structures we can allocate pcbs from */
extern pcb_t pcb[PCB_TABLE_SIZE];

void init_pcb_table(void);

int create_thread(uintptr_t start_addr);
int create_process(uint32_t base, uint32_t size);

/* === Dynamic Process Loading === */

/* Read the directory from the USB stick and copy it to 'buf' */
int readdir(unsigned char *buf);

/* Load a process from the USB stick */
int loadproc(int location, int size);

/* Remove pcb from its current queue and insert it into the free_pcb queue */
void free_pcb(pcb_t *pcb);

void print_pcb_table(void);

#endif /* THREAD_H */
