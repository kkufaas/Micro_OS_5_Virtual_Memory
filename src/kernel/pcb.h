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

    /* TODO: Design your PCB structure.
     * What state do you need to keep for each task? */

};

typedef struct pcb pcb_t;

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
