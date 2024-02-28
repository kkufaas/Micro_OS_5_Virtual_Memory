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

	/* Save registers */
	SAVE_GEN_REGS
	SAVE_FP_REGS
	SAVE_DATA_SEGMENTS
	push	%eax
	LOAD_KERNEL_DATA_SEGMENTS	scratch=%eax
	pop	%eax

	/* Call to handler in C */
	pushl	%edx	/* Arg 3 */
	pushl	%ecx	/* Arg 2 */
	pushl	%ebx	/* Arg 1 */
	pushl	%eax	/* Syscall number */
	call	syscall_dispatch
	addl	$16, %esp

	mov	%eax,	FR_RETVAL(%ebp)		# Save return value

	RESTORE_DATA_SEGMENTS
	RESTORE_FP_REGS
	RESTORE_GEN_REGS

	mov	FR_RETVAL(%ebp),	%eax	# Restore return value

	add	$FR_SIZE,	%esp		# Destroy stack frame
	pop	%ebp

	call	nointerrupt_leave_delayed
	iret

