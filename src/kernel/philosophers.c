#include <stdlib.h>

#include <ansi_term/ansi_esc.h>
#include <ansi_term/termbuf.h>
#include <ansi_term/tprintf.h>
#include <syslib/screenpos.h>
#include <util/util.h>

#include "hardware/keyboard.h"
#include "lib/assertk.h"
#include "scheduler.h"
#include "sync.h"

/* Dining philosphers threads. */

enum phl_state {
    PHL_THINKING,
    PHL_HUNGRY,
    PHL_EATING,
    PHL_FULL,
};

static const char *PHL_STRS[] = {
        [PHL_THINKING] = "thinking",
        [PHL_HUNGRY]   = "hungry",
        [PHL_EATING]   = "eating",
        [PHL_FULL]     = "full",
};

static const char FORK_ART_NEAR    = '_';
static const char FORK_ART_HOLDING = 'o';

static const char PHL_ART[] = {
        [PHL_THINKING] = 'O',
        [PHL_HUNGRY]   = 'O',
        [PHL_EATING]   = 'O',
        [PHL_FULL]     = 'O',
};

struct philosopher {
    int            forka; // Fork to pick up first
    int            forkb; // Fork to pick up second
    const char    *name;
    unsigned char  led;
    bool           has_forka;
    bool           has_forkb;
    enum phl_state state;
    int            meals;
};

#define PHL_CT         3
#define PHL_NAME_WIDTH 4

/*
 * Advantage to capslock
 *
 * This implementation of the dining philosophers gives a slight advantage
 * to the middle philosopher, capslock.
 *
 * Can you explain why?
 *
 * Can this implementation deadlock? If so, how? If not, why not?
 * Can it experience starvation?
 */

static semaphore_t        forks[PHL_CT];
static struct philosopher philosophers[PHL_CT] = {
        {
                .forka = 0,
                .forkb = 1,
                .name  = "nmlk",
                .led   = KB_LED_NUM,
        },
        {
                .forka = 2,
                .forkb = 1,
                .name  = "caps",
                .led   = KB_LED_CAPS,
        },
        {
                .forka = 0,
                .forkb = 2,
                .name  = "scrl",
                .led   = KB_LED_SCRL,
        },
};

/* Range of time a philosopher spends thinking */
static const int THINK_MS_MIN = 750;
static const int THINK_MS_MAX = 1000;

/* Range of time a philosopher spends eating */
static const int EAT_MS_MIN = THINK_MS_MIN;
static const int EAT_MS_MAX = THINK_MS_MAX;

/* Range of time it takes for a philosopher to reach for a fork */
static const int REACH_MS_MIN = 0;
static const int REACH_MS_MAX = 0;

static volatile int init_complete = 0;

static void do_init(void)
{
    for (int i = 0; i < PHL_CT; i++)
        forks[i] = (semaphore_t) SEMAPHORE_INIT(1);

    init_complete = 1;
}

static void wait_for_init(void)
{
    while (init_complete == 0) yield();
}

static void delay_rand_ms(int min, int max)
{
    int delay = min;
    if (max != min) delay += rand() % (max - min);
    ms_delay(delay);
}

static struct term term = PHL_TERM_INIT;
static lock_t      term_lock;

static void phl_print_state(int phli)
{
    struct philosopher *self = &philosophers[phli];

    lock_acquire(&term_lock);

    tprintf(&term, ANSIF_CUP, phli + 1, 1);
    tprintf(&term, "%2d ", current_running->pid);

    tprintf(&term, "phl %*s ", PHL_NAME_WIDTH, self->name);

    for (int i = 0; i < PHL_CT; i++) {
        bool forknear = i == self->forka || i == self->forkb;
        bool hasfork  = (i == self->forka && self->has_forka)
                       || (i == self->forkb && self->has_forkb);

        char forkart;
        if (forknear && hasfork) forkart = FORK_ART_HOLDING;
        else if (forknear) forkart = FORK_ART_NEAR;
        else forkart = ' ';
        tprintf(&term, "%c", forkart);

        char phlart;
        if (i == phli) phlart = PHL_ART[self->state];
        else phlart = ' ';
        tprintf(&term, "%c", phlart);
    }

    tprintf(&term, " %-8s", PHL_STRS[self->state]);
    tprintf(&term, " %4d meals", self->meals);
    tprintf(&term, ANSIF_EL "\n", ANSI_EFWD); // Clear rest of line
    lock_release(&term_lock);
}

static void philosophize(int phli)
{
    if (phli == 0) do_init();
    else wait_for_init();

    const int previ = (phli + PHL_CT - 1) % PHL_CT;
    const int nexti = (phli + 1) % PHL_CT;

    struct philosopher *self      = &philosophers[phli];
    struct philosopher *phl_right = &philosophers[previ];
    struct philosopher *phl_left  = &philosophers[nexti];

    self->has_forka = false;
    self->has_forkb = false;

    for (;;) {
        /* Think */
        self->state = PHL_THINKING;
        phl_print_state(phli);

        delay_rand_ms(THINK_MS_MIN, THINK_MS_MAX);

        /* Get hungry */
        self->state = PHL_HUNGRY;
        phl_print_state(phli);

        /* Pick up fork A */
        delay_rand_ms(REACH_MS_MIN, REACH_MS_MAX);
        semaphore_down(&forks[self->forka]);
        self->has_forka = true;
        phl_print_state(phli);

        /* Pick up fork B */
        delay_rand_ms(REACH_MS_MIN, REACH_MS_MAX);
        semaphore_down(&forks[self->forkb]);
        self->has_forkb = true;

        /* Eat */
        self->state = PHL_EATING;
        self->meals++;
        kb_set_leds(self->led);
        phl_print_state(phli);

        assertf(phl_right->state != PHL_EATING,
                "phl %d: the one to my right (%d) is also eating!\n", phli,
                previ);

        assertf(phl_left->state != PHL_EATING,
                "phl %d: the one to my left (%d) is also eating!\n", phli,
                nexti);

        delay_rand_ms(EAT_MS_MIN, EAT_MS_MAX);

        /* Done eating */
        self->state = PHL_FULL;
        phl_print_state(phli);

        /* Put down fork A */
        delay_rand_ms(REACH_MS_MIN, REACH_MS_MAX);
        semaphore_up(&forks[self->forka]);
        self->has_forka = false;
        phl_print_state(phli);

        /* Put down fork B */
        delay_rand_ms(REACH_MS_MIN, REACH_MS_MAX);
        semaphore_up(&forks[self->forkb]);
        self->has_forkb = false;
        phl_print_state(phli);
    }
}

void phl_thread0(void) { philosophize(0); }
void phl_thread1(void) { philosophize(1); }
void phl_thread2(void) { philosophize(2); }

