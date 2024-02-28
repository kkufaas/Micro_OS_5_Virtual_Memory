/*
 * Implementation of the mailbox.
 *
 * Implementation notes:
 *
 * The mailbox is protected with a lock to make sure that only
 * one process is within the queue at any time.
 *
 * It also uses condition variables to signal that more space or
 * more messages are available.
 *
 * In other words, this code can be seen as an example of implementing a
 * producer-consumer problem with a monitor and condition variables.
 *
 * Note that this implementation only allows keys from 0 to 4
 * (key >= 0 and key < MAX_Q).
 *
 * The buffer is a circular array.
 */

#define pr_fmt(fmt) "mbox: " fmt

#include "mbox.h"

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/common.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "lib/printk.h"
#include "lib/todo.h"
#include "sync.h"

mbox_t Q[MAX_MBOX];

/* === Helper for working with the circular buffer === */

/*
 * Returns the number of bytes available in the queue
 * Note: Mailboxes with count=0 messages should have head=tail,
 * which means that we return BUFFER_SIZE bytes.
 */
static int space_available(mbox_t *q)
{
    if ((q->tail == q->head) && (q->count != 0)) {
        /* Message in the queue, but no space  */
        return 0;
    }

    if (q->tail > q->head) {
        /* Head has wrapped around  */
        return q->tail - q->head;
    }
    /* Head has a higher index than tail  */
    return q->tail + BUFFER_SIZE - q->head;
}

/* === Debugging functions === */

/* Log a trace for a message send */
static void print_trace(char *s, int q, int msgSize)
{
    static int count = 0;
    count++;
    pr_debug(
            "%-10s%-5d%-5d%-10d%-10d\n", s, count, q, space_available(&Q[q]),
            msgSize
    );
}

static const bool MBOX_DEBUG = false;
static struct term mboxterm = MBOXTAB_TERM_INIT;

/* Print a table of mailbox statuses */
void print_mbox_status(void)
{
    if (!MBOX_DEBUG) return;

    /*
     * Note that the mailbox table positioning on screen might clash with other
     * screen output. You may have to disable those threads/processes in order
     * to see the mailbox table.
     */

    /* Reset to top of window and print header. */
    tprintf(&mboxterm, ANSIF_CUP, 1, 1);
    tprintf(&mboxterm, "== Mailbox status ==");
    tprintf(&mboxterm, ANSIF_EL "\n", ANSI_EFWD); // Clear right, then newline

    /*
     * Here you can print a mailbox table. You can use the tprintf (terminal
     * printf) function to print to the designated part of the screen.
     *
     * tprintf supports some ANSI escape codes. There are examples above and
     * below. I recommend using the EL (Erase Line) command to end each row
     * you print, to erase anything leftover from the text that was printed
     * there last time.
     */
}

/* === Mailbox API === */

/* Initialize mailbox system, called by kernel on startup  */
void mbox_init(void)
{
    todo_noop();
}

/*
 * Open a mailbox with the key 'key'.
 *
 * Returns a mailbox handle which must be used to identify
 * this mailbox in the following functions (parameter q).
 */
int mbox_open(int key)
{
    todo_use(key);
    todo_noop();
    return -1;
}

/* Close the mailbox with handle q  */
int mbox_close(int q)
{
    todo_use(q);
    todo_noop();
    return -1;
}

/*
 * Get number of messages (count) and number of bytes available in the
 * mailbox buffer (space).
 *
 * Note that the buffer is also used for storing the message headers,
 * which means that a message will take MSG_T_HEADER + m->size bytes
 * in the buffer. (MSG_T_HEADER = sizeof(msg_t header))
 */
int mbox_stat(int q, int *count, int *space)
{
    todo_use(q);
    todo_use(count);
    todo_use(space);
    todo_use(space_available(&Q[q]));
    todo_noop();
    return -1;
}

/* Fetch a message from queue 'q' and store it in 'm'  */
int mbox_recv(int q, msg_t *m)
{
    print_trace("Recv", q, -1);
    todo_use(m);
    todo_noop();
    return -1;
}

/* Insert 'm' into the mailbox 'q'  */
int mbox_send(int q, msg_t *m)
{
    int msgSize = MSG_SIZE(m);
    print_trace("Send", q, msgSize);
    todo_noop();
    return -1;
}

