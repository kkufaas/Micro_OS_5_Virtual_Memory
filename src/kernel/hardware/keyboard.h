/*
 * PS2 keyboard driver
 *
 * See:
 * - PS2 controller: <https://wiki.osdev.org/%228042%22_PS/2_Controller>
 * - PS2 Keyboard: <https://wiki.osdev.org/Keyboard>
 */
#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

#include <stdint.h>

#define PORT_PS2_DATA 0x60
#define PORT_PS2_STAT 0x64 // Read from port to get status
#define PORT_PS2_CMD  0x64 // Write to port to send command

/* PS2 status register bits */

#define PS2_STAT_OUTBUF_FULL (1 << 0)
#define PS2_STAT_INBUF_FULL  (1 << 1)
#define PS2_STAT_SYS_FLAG    (1 << 2)
#define PS2_STAT_CMD_DATA    (1 << 3)
#define PS2_STAT_BIT4        (1 << 4) // Implementation defined, rarely used
#define PS2_STAT_BIT5        (1 << 5) // Implementation defined, rarely used
#define PS2_STAT_TIMEOUT_ERR (1 << 6)
#define PS2_STAT_PARITY_ERR  (1 << 7)

/* Keyboard LED bits */

#define KB_LED_NONE 0
#define KB_LED_SCRL (1 << 0)
#define KB_LED_NUM  (1 << 1)
#define KB_LED_CAPS (1 << 2)
#define KB_LED_ALL  (KB_LED_SCRL | KB_LED_NUM | KB_LED_CAPS)

/* Keyboard command bytes */

enum kb_cmd {
    KB_SET_LEDS = 0xed,
};

enum kb_resp {
    KB_ACK    = 0xfa,
    KB_RESEND = 0xfe,
};

/* Basic keyboard actions */

void kb_set_leds(uint8_t ledbits);

#endif /* DRIVERS_KEYBOARD_H */
