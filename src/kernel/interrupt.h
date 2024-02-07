#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

#include "hardware/intctl_8259.h"
#include "hardware/pit_8235.h"

extern int spurious_irq_ct;

/* === Initialization === */

static const irqmask_t IRQS_TO_ENABLE =
        ((1 << IRQ_CASCADE) // Cascade interrupt from slave to master
         | (1 << IRQ_MASTER_LOWEST_PRIORITY) // Possible spurious interrupts
         | (1 << IRQ_TIMER)
        );

void init_int_controller(void);
void init_idt(void);

/* === Mask/unmask a hardware interrupt source === */

void mask_hw_int(int irq);
void unmask_hw_int(int irq);

/* === Timer IRQ, used for task preemption === */

/*
 * Target frequency for the task preempt interrupt, in Hz
 *
 * Determines the length of thread time slices.
 */
static const unsigned int PREEMPT_IRQ_TARGET_HZ = 100;

/*
 * Programmable Interval Timer mode used for task preempt interrupt
 *
 * The wiki at OSDev.org notes that operating systems typically use the square
 * wave mode (mode 3) to generate timer interrupts. However, we have found that
 * square wave mode seems to cause many spurious interrpts, while rate
 * generator mode (mode 2) does not, at least when running under Bochs 2.7.
 *
 * See:
 * - OSDev wiki: <https://wiki.osdev.org/Programmable_Interval_Timer>
 */
static const enum pit_mode PREEMPT_IRQ_PIT_MODE = PIT_MODE_SQUARE_WAVE;

void init_pit(void);

#endif /* !INTERRUPT_H */
