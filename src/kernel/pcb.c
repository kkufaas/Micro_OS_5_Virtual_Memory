#include <stddef.h>

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/addrs.h>
#include <syslib/screenpos.h>

#include "lib/assertk.h"
#include "lib/printk.h"
#include "lib/todo.h"

#include "pcb.h"
#include "scheduler.h"
#include "sync.h"

/* === Threads as a doubly-linked ring queue === */

/*
 * Insert thread p into queue q
 */
void queue_insert(struct pcb **q, struct pcb *p)
{
    assertf(q, "pointer to queue must be non-null");
    assertf(p->next == NULL, "thread is already in another queue");

    if (!*q) {
        /* Queue is empty: make a queue of one. */
        p->next     = p;
        p->previous = p;
        *q          = p;
    } else {
        /* Insert into queue. */
        p->previous = (*q)->previous; // End of list.
        p->next     = (*q);           // Wrap aroud to beginning.

        p->previous->next = p;
        p->next->previous = p;
    }
}

/*
 * Pull the next thread from queue q
 *
 * Returns a pointer to the removed thread.
 * If the queue is empty, returns null.
 */
struct pcb *queue_shift(struct pcb **q)
{
    struct pcb *p;
    assertf(q, "pointer to queue must be non-null");
    if (!*q) {
        p = NULL;
    } else {
        p = *q;
        if (p->next == p && p->previous == p) {
            /* This was a queue of one. Set queue to empty. */
            *q = NULL;
        } else {
            /* Remove the thread from the queue. */
            p->previous->next = p->next;
            p->next->previous = p->previous;
            *q                = p->next;
        }
        p->previous = NULL;
        p->next     = NULL;
    }
    return p;
}

/*
 * Find position of thread p in queue q
 *
 * Returns -1 if the thread is not in the queue.
 */
static int queue_pos(struct pcb **q, struct pcb *p)
{
    static const int LIMIT = PCB_TABLE_SIZE;

    assertf(q, "pointer to queue must be non-null");

    int pos;
    if (*q == NULL) {
        /* Queue is empty. */
        pos = -1;
        goto end;
    }

    struct pcb *start = *q;
    struct pcb *cur   = *q;
    int         i     = 0;

    for (;;) {
        if (cur == p) {
            /* Found it. */
            pos = i;
            goto end;
        }

        cur = cur->next;
        i++;

        assertf(cur != NULL, "malformed queue does not wrap");
        assertf(i <= LIMIT, "malformed queue goes too far");

        if (cur == start) {
            /* We have wrapped back around to the beginning. */
            pos = -1;
            goto end;
        }
    }

end:
    return pos;
}

/*
 * Remove thread p from queue q
 */
void queue_remove(struct pcb **q, struct pcb *p)
{
    assertf(q, "pointer to queue must be non-null");
    assertf(queue_pos(q, p) >= 0, "thread must be in queue to remove");

    if (*q == p && p->next == p) {
        /* p was the only thread in the queue. Mark it as empty. */
        *q = NULL;
    } else if (*q == p) {
        /* p was the head of the queue. Advance the head. */
        *q = p->next;
    }

    /* Stitch p out of the linked list. */
    p->previous->next = p->next;
    p->next->previous = p->previous;

    p->next     = NULL;
    p->previous = NULL;

}

/* === PCB Allocation === */

/* Statically allocate some storage for the pcb's */
pcb_t pcb[PCB_TABLE_SIZE];

/* Initialize pcb table before allocating pcbs */
void init_pcb_table(void)
{
    /* Here you can initialize the empty PCB array to get it ready to
     * allocate PCBs. */
    todo_noop();
}

/* === Print Process Status Table === */

static struct term procterm = PROCTAB_TERM_INIT;

/*
 * Used for debugging. The clock thread will call this function every
 * so often to update the status info.
 */
void print_pcb_table(void)
{
    /* Reset to top of window and print header. */
    tprintf(&procterm, ANSIF_CUP, 1, 1);
    tprintf(&procterm, "== Process status ==");
    tprintf(&procterm, ANSIF_EL "\n", ANSI_EFWD); // Clear right, then newline

    /*
     * Here you can print a process table. You can use the tprintf (terminal
     * printf) function to print to the designated part of the screen.
     *
     * tprintf supports some ANSI escape codes. There are examples above and
     * below. I recommend using the EL (Erase Line) command to end each row
     * you print, to erase anything leftover from the text that was printed
     * there last time.
     */
    todo_noop();

    /* Clear rest of window below cursor. */
    tprintf(&procterm, ANSIF_ED, ANSI_EFWD);
}

