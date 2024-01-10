/*
 * Abstractions for PC serial port
 *
 * For an overview of serial port programming, see:
 * <https://wiki.osdev.org/Serial_Ports>
 */
#ifndef SERIAL_H
#define SERIAL_H

#include "cpu_x86.h"

#define PORT_COM1 0x3f8
#define PORT_COM2 0x2f8

int  serial_putc(ioport_t port, char ch);
void serial_puts(ioport_t port, const char *str);

#endif /* SERIAL_H */
