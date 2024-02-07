/*
 * C abstractions for an Intel 8235 Programmable Interval Timer (PIT)
 *
 * References:
 *
 * - OSDev Wiki: <https://wiki.osdev.org/Programmable_Interval_Timer>
 * - Bran's Kernel Development Tutorial:
 *      <http://www.osdever.net/bkerndev/Docs/pit.htm>
 */
#ifndef PIT_8235_H
#define PIT_8235_H

#include <stdbool.h>
#include <stdint.h>

static const unsigned int PIT_BASE_HZ = 1193180;

enum pit_channel {
    PIT_CH_IRQ     = 0,
    PIT_CH1        = 1,
    PIT_CH_SPEAKER = 2,
};

enum pit_mode {
    PIT_MODE_INTERRUPT_ON_COUNT = 0,
    PIT_MODE_ONESHOT            = 1,
    PIT_MODE_RATE_GEN           = 2,
    PIT_MODE_SQUARE_WAVE        = 3,
    PIT_MODE_SOFTWARE_STROBE    = 4,
    PIT_MODE_HARDWARE_STROBE    = 5,
};

enum pit_rw {
    PIT_RW_LSB     = 1,
    PIT_RW_MSB     = 2,
    PIT_RW_LSB_MSB = 3,
};

enum pit_bcd {
    PIT_BASE2 = 0, // Data is a 16-bit counter
    PIT_BCD   = 1, // Data is four BCD decade counters
};

static inline uint8_t pit_cmd_byte(
        enum pit_channel cntr,
        enum pit_rw      rw,
        enum pit_mode    mode,
        enum pit_bcd     bcd
)
{
    return cntr << 6 | rw << 4 | mode << 1 | bcd;
}

unsigned int pit_set_irq_freq(enum pit_mode mode, unsigned int target_hz);

#endif /* PIT_8235_H */
