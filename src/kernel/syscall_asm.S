/*
 * Low-level syscall functions that must be written in assembly
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
	 * switch_to_kernel_stack and switch_to_user_stack both switch to its
	 * respective stack without modifying any registers. Note that
	 * interrupts should be disabled when calling these functions, as they
	 * share space for their scratch variables. The stack remains untouched
	 * if nested_count > 0. (This implies that we never switch stacks when
	 * dealing with threads, as they have their nested_count set to 1 in
	 * _start().)
	 */
switch_stack_scratch:
	.long	0
ret_addr:
	.long	0
	.globl	switch_to_kernel_stack
switch_to_kernel_stack:
	/* Save %eax */
	movl	%eax, (switch_stack_scratch)
	/* Pop return address, so we can return after switching stacks */
	popl	%eax
	movl	%eax, (ret_addr)
	movl	current_running, %eax
	/* Don't switch stack if nested_count != 0 */
	cmpl	$0, PCB_NESTED_COUNT(%eax)
	jne	switch_return
	/* Save user stack, and switch to kernel stack */
	movl	%esp, PCB_USER_STACK(%eax)
	movl	PCB_KERNEL_STACK(%eax), %esp
	jmp	switch_return

	/*
	 * This basically does the same as switch_to_kernel_stack, except that
	 * the kernel stack is saved, and we switch to the user stack.
	 */
	.globl	switch_to_user_stack
switch_to_user_stack:
	movl	%eax, (switch_stack_scratch)
	popl	%eax
	movl	%eax, (ret_addr)
	movl	current_running, %eax
	/* Don't switch stack if nested_count != 0 */
	cmpl	$0, PCB_NESTED_COUNT(%eax)
	jne	switch_return
	/* Switch to user stack */
	movl	%esp, PCB_KERNEL_STACK(%eax)
	movl	PCB_USER_STACK(%eax), %esp
	jmp	switch_return

	/* Both s_t_user_stack and s_t_kernel_stack return from this point */
switch_return:
	movl	(ret_addr), %eax
	pushl	%eax
	movl	(switch_stack_scratch), %eax
	ret

	/* Enter the kernel from userspace to make a syscall */
	.globl  syscall_entry_interrupt
syscall_entry_interrupt:
	/* Enter critical section */
	call	nointerrupt_enter

	/* Create a stack frame. */
	push	%ebp
	mov	%esp,	%ebp
	.equ	FR_RETVAL,	-4	# Syscall return value
	.equ	FR_SIZE,	4
	sub	$FR_SIZE,	%esp

	/* Switch stack */
	call	switch_to_kernel_stack

	/* Save registers */
	SAVE_GEN_REGS
	SAVE_FP_REGS

	/* Call to handler in C */
	pushl	%edx	/* Arg 3 */
	pushl	%ecx	/* Arg 2 */
	pushl	%ebx	/* Arg 1 */
	pushl	%eax	/* Syscall number */
	call	syscall_dispatch
	addl	$16, %esp

	mov	%eax,	FR_RETVAL(%ebp)		# Save return value

	RESTORE_FP_REGS
	RESTORE_GEN_REGS

	# Switch to user stack
	call	switch_to_user_stack

	mov	FR_RETVAL(%ebp),	%eax	# Restore return value

	add	$FR_SIZE,	%esp		# Destroy stack frame
	pop	%ebp

	call	nointerrupt_leave_delayed
	iret

