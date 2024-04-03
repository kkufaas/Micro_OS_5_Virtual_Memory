/*
 * Low-level interrupt handlers that must be written in assembly
 */

/*
 * This file is assembly that is also run through the C preprocessor
 *
 * Be careful with comments. Comments with a hash opener ('#') might be
 * mistaken for preprocessor directives if they are the first thing on the
 * line. C-style comments ('/ * ... * /') are preferred.
 *
 * <https://sourceware.org/binutils/docs/as/Comments.html>
 *
 * It is also possible to use C-style '#define' macros, but when in assembly
 * it is recommended to use assembler macros ('.macro') instead.
 *
 * <https://sourceware.org/binutils/docs/as/Macro.html>
 */

#include "asm-offsets.h.s"
#include "asm_common.h.S"

	.text

.macro	IRQ_ENTRY_WRAPPER \
		irqnum, wrapped_fn, pass_irqnum=0, nointerrupt_leave=0

	call	nointerrupt_enter
	SAVE_GEN_REGS
	SAVE_FP_REGS
	SAVE_DATA_SEGMENTS
	LOAD_KERNEL_DATA_SEGMENTS	scratch=%eax

	movl	current_running, %eax
	incl	PCB_NESTED_COUNT(%eax)

	pushl	$\irqnum
	call	pic_mask_irq	# Mask the IRQ we are servicing.
	call	pic_send_eoi	# Send EOI so other IRQs can come through.
	addl	$4, %esp

	/* Call the wrapped interrupt handler function. */

	.if \nointerrupt_leave;
	call	nointerrupt_leave
	.endif
	.if \pass_irqnum
	push	$\irqnum	# Pass IRQ number as a parameter if requested.
	.endif
	call	\wrapped_fn	# Actually call the wrapped function.
	.if \pass_irqnum
	add	$4,	%esp	# Remove the IRQ number parameter.
	.endif
	.if \nointerrupt_leave
	call	nointerrupt_enter
	.endif

	movl	current_running, %eax
	decl	PCB_NESTED_COUNT(%eax)
	pushl	$\irqnum
	call	pic_unmask_irq	# Reenable this IRQ.
	addl	$4, %esp
	call	nointerrupt_leave_delayed
	RESTORE_DATA_SEGMENTS
	RESTORE_FP_REGS
	RESTORE_GEN_REGS
	iret
.endm

	.globl  timer_isr_entry
timer_isr_entry:
	IRQ_ENTRY_WRAPPER	IRQ_TIMER, preempt

	.globl  keyboard_isr_entry
keyboard_isr_entry:
	IRQ_ENTRY_WRAPPER	IRQ_KEYBOARD, keyboard_interrupt, \
				nointerrupt_leave=1

	.globl  pci5_entry
pci5_entry:
	IRQ_ENTRY_WRAPPER	5, pci_interrupt, pass_irqnum=1, \
				nointerrupt_leave=1

	.globl  pci9_entry
pci9_entry:
	IRQ_ENTRY_WRAPPER	9, pci_interrupt, pass_irqnum=1, \
				nointerrupt_leave=1

	.globl  pci10_entry
pci10_entry:
	IRQ_ENTRY_WRAPPER	10, pci_interrupt, pass_irqnum=1, \
				nointerrupt_leave=1

	.globl  pci11_entry
pci11_entry:
	IRQ_ENTRY_WRAPPER	11, pci_interrupt, pass_irqnum=1, \
				nointerrupt_leave=1

	/*
	 * Low level entry point for page fault handler
	 */
	.globl  page_fault_handler_entry
page_fault_handler_entry:
	call	nointerrupt_enter

	/* Create stack frame */
	.equ	FR_IRET_FRAME,	+8	# Start of EIP,CS,EFLAGS frame
	.equ	FR_ERRCODE,	+4
	.equ	FR_ORIG_BP,	+0
	push	%ebp
	mov	%esp,	%ebp

	SAVE_GEN_REGS
	SAVE_FP_REGS
	SAVE_DATA_SEGMENTS
	LOAD_KERNEL_DATA_SEGMENTS,	scratch=%eax

	mov	current_running,	%eax
	incl	PCB_NESTED_COUNT(%eax)

	/* Call handler in C. */
	mov	FR_ERRCODE(%ebp),	%eax	# Arg 2: Error code
	push	%eax
	lea	FR_IRET_FRAME(%ebp),	%eax	# Arg 1: Frame addr
	push	%eax
	call	page_fault_handler
	addl	$8, %esp			# Pop args

	mov	current_running,	%eax
	decl	PCB_NESTED_COUNT(%eax)

	RESTORE_DATA_SEGMENTS
	RESTORE_FP_REGS
	RESTORE_GEN_REGS

	pop	%ebp
	call	nointerrupt_leave_delayed
	add	$4,	%esp		# Pop error code
	iret

