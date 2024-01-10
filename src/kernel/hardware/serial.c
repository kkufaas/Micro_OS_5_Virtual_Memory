#include "serial.h"

#include <stdarg.h>
#include <stdio.h>

#include "cpu_x86.h"

/* I/O port offsets for serial port registers */

#define POFF_DATA      0
#define POFF_INTENABLE 1
#define POFF_INTID     2
#define POFF_LINECTL   3
#define POFF_MODEMCTL  4
#define POFF_LINESTAT  5
#define POFF_MODEMSTAT 6
#define POFF_SCRATCH   7

enum data_bits {
    DBITS_5 = 0,
    DBITS_6 = 1,
    DBITS_7 = 2,
    DBITS_8 = 3,
};

/* Line status register bits */

#define LINESTAT_DR   1 << 0 // Data Ready
#define LINESTAT_OE   1 << 1 // Overflow Error
#define LINESTAT_PE   1 << 2 // Parity Error
#define LINESTAT_FE   1 << 3 // Framing Error
#define LINESTAT_BI   1 << 4 // Break Indicator
#define LINESTAT_THRE 1 << 5 // Trandmitter Holding Register Empty
#define LINESTAT_TEMT 1 << 6 // Transmitter Empty
#define LINESTAT_IE   1 << 7 // Impending Error

static void serial_set_data_bits(ioport_t port, enum data_bits bits)
{
    outb(port + POFF_LINECTL, bits);
}

static void serial_wait_for_transmit_empty(ioport_t port)
{
    for (;;) {
        unsigned char linestat = inb(port + POFF_LINESTAT);
        if (linestat & LINESTAT_THRE) break;
    }
}

int serial_putc(ioport_t port, char ch)
{
    serial_wait_for_transmit_empty(port);
    outb(port, ch); // Send character
    return ch;
}

void serial_puts(ioport_t port, const char *str)
{
    if (!str) return;

    serial_set_data_bits(port, DBITS_8); // Set for 8-bit character output

    for (; *str; str++) {
        serial_putc(port, *str);
    }
}

