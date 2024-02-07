#include "keyboard.h"

#include "cpu_x86.h"

static void kb_wait_for_inbuf_empty(void)
{
    while (inb(PORT_PS2_STAT) & PS2_STAT_INBUF_FULL)
        ;
}

static void kb_wait_for_ack(void)
{
    while (inb(PORT_PS2_DATA) != KB_ACK)
        ;
}

void kb_set_leds(uint8_t ledbits)
{
    ledbits &= KB_LED_ALL; // Mask out any non-LED bits.

    kb_wait_for_inbuf_empty();
    outb(PORT_PS2_DATA, KB_SET_LEDS);
    kb_wait_for_ack();
    outb(PORT_PS2_DATA, ledbits);
}

