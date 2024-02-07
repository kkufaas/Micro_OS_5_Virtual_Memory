#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

#define PCB_TABLE_SIZE 32

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
int create_process(uintptr_t start_addr);

void print_pcb_table(void);

#endif /* THREAD_H */
