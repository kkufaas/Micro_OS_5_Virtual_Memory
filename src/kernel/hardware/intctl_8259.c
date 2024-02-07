#include "intctl_8259.h"

#include "../lib/assertk.h"
#include "cpu_x86.h"

#define PORT_PIC_MASTER_CMD  0x20
#define PORT_PIC_MASTER_DATA 0x21
#define PORT_PIC_SLAVE_CMD   0xa0
#define PORT_PIC_SLAVE_DATA  0xa1

/* Initialization Command Word 1: start initialisation */

#define ICW1_INIT      0b00010000
#define ICW1_NEED_ICW4 0b00000001

/* Initialization Command Word 4: additional configuration */

#define ICW4_SFNM      0b00010000 // Special Fully-Nested Mode
#define ICW4_BUF       0b00001000 // Buffered mode
#define ICW4_MS_MASTER 0b00000100 // If buffering, master vs slave mode
#define ICW4_AEOI      0b00000010 // Auto End Of Interrupt mode
#define ICW4_MPM_8086  0b00000001 // Set for 8086 mode, clear for MCS-80 mode

/* Operation Control Word 2 */

#define OCW2_EOI_NONSPECIFIC 0b00100000
#define OCW2_EOI_SPECIFIC    0b01100000

/* Operation Control Word 3 */

#define OCW3_READ_IRR 0b00001010 // Read Interrupt Request Register (IRR)
#define OCW3_READ_ISR 0b00001011 // Read In-Service Register (ISR)

/* Short helpers */

inline static uint8_t pic_irq_bit_single(irq_t irq)
{
    if (irq < IRQ_SLAVE_START) return 1 << irq;
    else return 1 << (irq - IRQ_SLAVE_START);
}

inline static uint8_t pic_irq_data_port(irq_t irq)
{
    if (irq < IRQ_SLAVE_START) return PORT_PIC_MASTER_DATA;
    else return PORT_PIC_SLAVE_DATA;
}

/* Initialization */

static void pic_init_single(ioport_t cmd_port, uint8_t irq_start, uint8_t icw3)
{
    const ioport_t data_port = cmd_port + 1;

    outb(cmd_port, ICW1_INIT | ICW1_NEED_ICW4); // ICW 1: start init
    outb(data_port, irq_start);                 // ICW 2: IRQ mapping
    outb(data_port, icw3);                      // ICW 3: cascade config
    outb(data_port, ICW4_MPM_8086);             // ICW 4: additional config
}

void pic_init(unsigned char irq_start)
{
    uint8_t icw3_master = 1 << IRQ_CASCADE; // IRQ 2 is coming from a slave
    uint8_t icw3_slave  = IRQ_CASCADE;      // The slave is slave #2

    pic_init_single(PORT_PIC_MASTER_CMD, irq_start, icw3_master);
    pic_init_single(
            PORT_PIC_SLAVE_CMD, irq_start + IRQ_SLAVE_START, icw3_slave
    );
}

/* Mask/unmask individual IRQs */

void pic_mask_irq(irq_t irq)
{
    ioport_t port    = pic_irq_data_port(irq);
    uint8_t  irq_bit = pic_irq_bit_single(irq);

    uint8_t mask = inb(port); // Get mask.
    mask |= irq_bit;          // Set desired bit to mask that IRQ.
    outb(port, mask);         // Set updated mask.
}

void pic_unmask_irq(irq_t irq)
{
    ioport_t port    = pic_irq_data_port(irq);
    uint8_t  irq_bit = pic_irq_bit_single(irq);

    uint8_t mask = inb(port); // Get mask.
    mask &= ~irq_bit;         // Clear desired bit to unmask IRQ.
    outb(port, mask);         // Set updated mask.
}

/* Get/set the bitmask for all 16 IRQs */

irqmask_t pic_get_mask()
{
    irqmask_t m = inb(PORT_PIC_MASTER_DATA);
    irqmask_t s = inb(PORT_PIC_SLAVE_DATA);
    return (s << IRQ_SLAVE_START) | m;
}

void pic_set_mask(irqmask_t mask)
{
    outb(PORT_PIC_MASTER_DATA, (uint8_t) mask);
    outb(PORT_PIC_SLAVE_DATA, (uint8_t) (mask >> IRQ_SLAVE_START));
}

/* Other functions */

void pic_send_eoi(irq_t irq)
{
    assertf(irq <= IRQ_MAX, "bad IRQ number %d, should be less than %d\n", irq,
            IRQ_MAX);

    if (irq < IRQ_SLAVE_START) {
        outb(PORT_PIC_MASTER_CMD, OCW2_EOI_SPECIFIC | irq);
    } else {
        irq_t slave_irq = irq - IRQ_SLAVE_START;
        outb(PORT_PIC_SLAVE_CMD, OCW2_EOI_SPECIFIC | slave_irq);
        outb(PORT_PIC_MASTER_CMD, OCW2_EOI_SPECIFIC | IRQ_CASCADE);
    }
}

static uint8_t pic_read_reg(ioport_t port, uint8_t ocw3)
{
    outb(port, ocw3);
    return inb(port);
}

unsigned long long pic_spurious_ct_master;
unsigned long long pic_spurious_ct_slave;

/*
 * Check for and handle spurious interrupts
 *
 * This function checks to see if an interrupt is spurious or not. If so sends
 * an EOI for the cascaded IRQ if necessary, and returns true. If not, it
 * simply returns false.
 *
 * BACKGROUND
 *
 * Spurious interrupts happen when the interrupt request to the PIC disappears
 * between the time that the PIC raises the interrupt to the CPU and time that
 * the CPU reads the IRQ number from the PIC. This can happen due to noise on
 * the request line.
 *
 * When the CPU asks for an IRQ number but there is none, the PIC simply gives
 * the CPU its lowest priority IRQ number (7). The CPU translates this number
 * as usual (7 for master, 15 for slave) and fires the interrupt routine.
 *
 * Interrupt handlers can check whether they are being called spuriously or not
 * by reading the PIC's In-Service Register (ISR). If the IRQ's in-service flag
 * is not set, then you know the interrupt was spurious and you do not have to
 * handle the interrupt or send an EOI.
 *
 * However, because of the way the slave PIC cascades into the master PIC,
 * a spurious interrupt from the slave will cause a real interrupt on the
 * master's cascade IRQ. So to handle a spurious IRQ 15, you have to send an
 * EOI to the master for the cascade IRQ (2).
 */
ATTR_CALLED_FROM_ISR
bool pic_check_spurious(irq_t irq)
{
    uint8_t isr_lowest = 1 << IRQ_MASTER_LOWEST_PRIORITY;

    if (irq == IRQ_MASTER_LOWEST_PRIORITY) {
        uint8_t isr         = pic_read_reg(PORT_PIC_MASTER_CMD, OCW3_READ_ISR);
        bool    is_spurious = (isr & isr_lowest) == 0;
        return is_spurious;

    } else if (irq == IRQ_SLAVE_LOWEST_PRIORITY) {
        uint8_t isr         = pic_read_reg(PORT_PIC_SLAVE_CMD, OCW3_READ_ISR);
        bool    is_spurious = (isr & isr_lowest) == 0;

        if (is_spurious) pic_send_eoi(IRQ_CASCADE);
        return is_spurious;
    }

    return false;
}

