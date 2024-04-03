#include "interrupt.h"

#include <stdbool.h>

#include <syslib/addrs.h>
#include <syslib/compiler_compat.h>
#include <util/util.h>

#include "cpu.h"
#include "hardware/cpu_x86.h"
#include "hardware/intctl_8259.h"
#include "hardware/pit_8235.h"
#include "lib/assertk.h"
#include "lib/printk.h"
#include "memory.h"
#include "scheduler.h"
#include "sync.h"
#include "syscall.h"

/* === Default Interrupt/Exception Handlers === */

static const int IVEC_NOT_CAPTURED = -1; /* Invalid vector value */

/*
 * Dump interrupt/exception information
 *
 * This will the default action for interrupts and exceptions that we do not
 * explicitly handle. We will create interrupt handlers that simply call this
 * function and then halt the kernel.
 */
ATTR_CALLED_FROM_ISR
static void dump_exception(
        int                     vector,
        const char             *exc_desc,
        struct interrupt_frame *stack_frame,
        bool                    has_err_code,
        ureg_t                  err_code
)
{
    if (vector == IVEC_NOT_CAPTURED) {
        pr_error("Unhandled INT %s\n", "[vector not captured]");
    } else {
        pr_error("Unhandled INT %d (%s)\n", vector, exc_desc);
    }

    /* Info from stack frame */
    if (has_err_code) pr_dump("Error code %x\n", err_code);
    else pr_dump("Error code: none\n");
    pr_dump("Stack %p\n", stack_frame);
    pr_dump("CS:IP %#x:%#x\n", stack_frame->cs, stack_frame->ip);
    pr_dump("FLAGS %#x\n", stack_frame->flags);

    /* Info from current process */
    pr_dump("PID %d\n", current_running->pid);
    pr_dump("Nested count %d\n", current_running->nested_count);
    pr_dump("Yields %d\n", current_running->yield_count);
    pr_dump("Preemptions %d\n", current_running->preempt_count);
    pr_dump("Hardware mask %x\n", current_running->int_controller_mask);

    /* Additional info for specific interrupts */
    switch (vector) {
    case IVEC_PF:
        pr_dump("CR2 (fault addr) %x\n", load_page_fault_addr());
        break;
    }
}

/* Generate a default handler for interrupts (no error code) */
#define DFLT_HDLR_INTERRUPT(VECTOR, FN_NAME, EXC_DESC) \
    INTERRUPT_HANDLER \
    static void FN_NAME(struct interrupt_frame *stack_frame) \
    { \
        dump_exception(VECTOR, EXC_DESC, stack_frame, false, 0); \
        abortk(); \
    }

/* Generate a default handler for exceptions (with error code) */
#define DFLT_HDLR_EXCEPTION(VECTOR, FN_NAME, EXC_DESC) \
    INTERRUPT_HANDLER \
    static void FN_NAME(struct interrupt_frame *stack_frame, ureg_t err_code) \
    { \
        dump_exception(VECTOR, EXC_DESC, stack_frame, true, err_code); \
        abortk(); \
    }

/* Default handlers for known interrupts and exceptions */
/* clang-format off */
DFLT_HDLR_INTERRUPT(IVEC_DE, handle_div_zero, "Divide by zero");
DFLT_HDLR_INTERRUPT(IVEC_DB, handle_debug_exc, "Debug");
DFLT_HDLR_INTERRUPT(IVEC_NMI, handle_nmi, "NMI");
DFLT_HDLR_INTERRUPT(IVEC_BP, handle_breakpoint, "Breakpoint");
DFLT_HDLR_INTERRUPT(IVEC_OF, handle_overflow, "Overflow");
DFLT_HDLR_INTERRUPT(IVEC_BR, handle_bound_range_exc, "BOUND Range exceeded");
DFLT_HDLR_INTERRUPT(IVEC_UD, handle_undefined_opcode, "Undefined Opcode");
DFLT_HDLR_INTERRUPT(IVEC_NM, handle_no_math, "No Math coprocessor");
DFLT_HDLR_EXCEPTION(IVEC_DF, handle_double_fault, "Double fault encountered");
DFLT_HDLR_INTERRUPT(IVEC_CSO, handle_co_seg_overrun, "Coprocess or Segment Overrun");
DFLT_HDLR_EXCEPTION(IVEC_TS, handle_invalid_tss, "Invalid TSS Fault");
DFLT_HDLR_EXCEPTION(IVEC_NP, handle_seg_not_present, "Segment Not Present");
DFLT_HDLR_EXCEPTION(IVEC_SS, handle_stack_seg_fault, "Stack Segment fault");
DFLT_HDLR_EXCEPTION(IVEC_GP, handle_gen_protect_fault, "General Protection fault");
/* clang-format on */

/*
 * Generic handler for unknown interrupts and exceptions
 *
 * The CPU calls interrupt handlers by vector number, but it does not pass the
 * vector number to the function. It is assumed that the function will know
 * what interrupt it's handling.
 *
 * This is a generic handler that we will install for exceptions and interrupts
 * that don't have a specific handler. It won't know what vector was actually
 * involed, so it will simply report that an exception occurred.
 */
DFLT_HDLR_INTERRUPT(
        IVEC_NOT_CAPTURED, handle_unknown_generic, "vector not captured"
);

/*
 * Count of spurious IRQs
 */
int spurious_irq_ct = 0;

/*
 * Interrupt handler for IRQ 7, which may be triggered spuriously
 *
 * We do not use the hardware on IRQ 7 (typically the parallel port),
 * but spurious interrupts can occur on this IRQ, so we must handle them.
 *
 * Previous TAs have seen spurious interrupts on IRQ 7 specifically when the
 * RTC timer interrupt was triggered at a very high frequency (~0.1 MHz).
 *
 * More recently, we have seen spurious interrupts when using the square
 * wave mode of on the Programmable Interrupt Timer (PIT), but not when
 * using the rate generator mode.
 */
INTERRUPT_HANDLER
void handle_spurious_irq(struct interrupt_frame *stack_frame)
{
    static const irq_t irq = IRQ_MASTER_LOWEST_PRIORITY;

    bool is_spurious = pic_check_spurious(irq);
    if (is_spurious) {
        spurious_irq_ct++;
    } else {
        dump_exception(IVEC_IRQ_0 + irq, "IRQ 7", stack_frame, false, 0);
        abortk();
    }
}

/* === Declarations for low-level functions defined in ASM === */

void timer_isr_entry(void);
void keyboard_isr_entry(void);

void pci5_entry(void);
void pci9_entry(void);
void pci10_entry(void);
void pci11_entry(void);

void page_fault_handler_entry(void);

/* === Initialization === */

struct descriptor idt[IDT_SIZE];

static void install_interrupt_handler(
        unsigned int             int_num,
        void                    *handler_fn,
        enum x86_privilege_level required_pl
)
{
    idt[int_num] = igate_init(KERNEL_CS, handler_fn, required_pl, SD_P);
}

void init_idt(void)
{
    /* Fill interrupt table with pointers to the generic handler. */
    for (int i = 0; i < IDT_SIZE; i++) {
        install_interrupt_handler(i, handle_unknown_generic, PL0);
    }

    /* Add handlers for CPU exceptions. */
    install_interrupt_handler(IVEC_DE, handle_div_zero, PL0);
    install_interrupt_handler(IVEC_DB, handle_debug_exc, PL0);
    install_interrupt_handler(IVEC_NMI, handle_nmi, PL0);
    install_interrupt_handler(IVEC_BP, handle_breakpoint, PL0);
    install_interrupt_handler(IVEC_OF, handle_overflow, PL0);
    install_interrupt_handler(IVEC_BR, handle_bound_range_exc, PL0);
    install_interrupt_handler(IVEC_UD, handle_undefined_opcode, PL0);
    install_interrupt_handler(IVEC_NM, handle_no_math, PL0);
    install_interrupt_handler(IVEC_DF, handle_double_fault, PL0);
    install_interrupt_handler(IVEC_CSO, handle_co_seg_overrun, PL0);
    install_interrupt_handler(IVEC_TS, handle_invalid_tss, PL0);
    install_interrupt_handler(IVEC_NP, handle_seg_not_present, PL0);
    install_interrupt_handler(IVEC_SS, handle_stack_seg_fault, PL0);
    install_interrupt_handler(IVEC_GP, handle_gen_protect_fault, PL0);
    install_interrupt_handler(IVEC_PF, page_fault_handler_entry, PL0);

    /* Add handlers for external Interrupt Requests (IRQs) */
    install_interrupt_handler(IVEC_IRQ_0 + IRQ_TIMER, timer_isr_entry, PL0);
    install_interrupt_handler(
            IVEC_IRQ_0 + IRQ_MASTER_LOWEST_PRIORITY, handle_spurious_irq, PL0
    );
    install_interrupt_handler(
            IVEC_IRQ_0 + IRQ_KEYBOARD, keyboard_isr_entry, PL0
    );

    /* Add handlers for PCI interrupts used by the USB driver. */
    install_interrupt_handler(IVEC_IRQ_0 + 5, pci5_entry, PL0);
    install_interrupt_handler(IVEC_IRQ_0 + 9, pci9_entry, PL0);
    install_interrupt_handler(IVEC_IRQ_0 + 10, pci10_entry, PL0);
    install_interrupt_handler(IVEC_IRQ_0 + 11, pci11_entry, PL0);

    /* Create gate for system calls */
    static const int syscall_dpl = 3;
    install_interrupt_handler(
            IVEC_SYSCALL, syscall_entry_interrupt, syscall_dpl
    );

    struct pseudo_descriptor desc = {
            .base_addr = idt,
            .limit     = sizeof(idt),
    };

    lidt(&desc);
}

void init_int_controller(void)
{
    pic_init(IVEC_IRQ_0);
    pic_set_mask(~IRQS_TO_ENABLE);

    /* Set level mode interrupt detection for PCI lines ICH spec */
    outb(0x4d0, 0x28);
    outb(0x4d1, 0x0e);
}

void init_pit(void)
{
    unsigned int divisor =
            pit_set_irq_freq(PREEMPT_IRQ_PIT_MODE, PREEMPT_IRQ_TARGET_HZ);

    unsigned int hz = PIT_BASE_HZ / divisor;
    pr_info("Initialized PIT: %d Hz / %d = ~%d Hz\n", PIT_BASE_HZ, divisor,
            hz);
}

