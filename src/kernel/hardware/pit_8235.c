#include "pit_8235.h"

#include "cpu_x86.h"

#define PIT_PORT_CH0      0x40
#define PIT_PORT_CH1      0x41
#define PIT_PORT_CH2      0x42
#define PIT_PORT_MODE_CMD 0x43

unsigned int pit_set_irq_freq(enum pit_mode mode, unsigned int target_hz)
{
    unsigned int divisor = PIT_BASE_HZ / target_hz;

    outb(PIT_PORT_MODE_CMD,
         pit_cmd_byte(PIT_CH_IRQ, PIT_RW_LSB_MSB, mode, PIT_BASE2));
    outb(PIT_PORT_CH0, divisor & 0xff);
    outb(PIT_PORT_CH0, divisor >> 8 & 0xff);
    return divisor;
}

