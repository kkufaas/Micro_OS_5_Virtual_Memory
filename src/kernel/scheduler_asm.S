/*
 * Low-level scheduler functions that must be written in assembly
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

	/*
	 * Save task state and enter the scheduler
	 */
	.globl  scheduler_entry
scheduler_entry:
	/* Save regs and eflags */
	SAVE_EFLAGS
	SAVE_GEN_REGS
	SAVE_FP_REGS
	/* If it is possible for processes to arrive at the scheduler with
	 * different critical section counts (i.e. different numbers of nested
	 * critical sections), then it is necessary to save and restore the
	 * critical section counter as well. */
	push	(nointerrupt_count_val)

	movl	current_running, %eax
	/*
	 * Simulate a push to the stack, so that the saved stack will
	 * include the return address to this function
	 */
	subl	$4, %esp
	movl	%esp, PCB_KERNEL_STACK(%eax)
	addl	$4, %esp
	call	scheduler

	pop	(nointerrupt_count_val)
	RESTORE_FP_REGS
	RESTORE_GEN_REGS
	RESTORE_EFLAGS
	ret

.text

/*
 * Note: Dispatch should only be called within a critical section/with
 * interrupts off.
 */

	.global dispatch
dispatch:
	movl	current_running, %eax
	movl	PCB_KERNEL_STACK(%eax), %esp
	cmpl	$STATUS_FIRST_TIME, PCB_STATUS(%eax)
	je	first_time
	ret
first_time:
	movl	$STATUS_READY, PCB_STATUS(%eax)
	cmpl	$1, PCB_IS_THREAD(%eax)
	je	isathread
	movl	PCB_USER_STACK(%eax), %esp
isathread:
	movb	$1,	(nointerrupt_count_val)
	call	nointerrupt_leave
	jmp	*PCB_START_PC(%eax)

