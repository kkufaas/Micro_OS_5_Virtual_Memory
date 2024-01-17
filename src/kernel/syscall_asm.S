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
	.globl  syscall_entry
syscall_entry:
	/* In this project, syscall_entry is called like a normal function. */

	/* Create a stack frame. */
	.equ	FR_SYSCALL_NUM,	+8	# Arg 1: syscall number
	.equ	FR_RETADDR,	+4	# Return address
	.equ	FR_ORIG_BP,	+0	# Original %ebp value
	push	%ebp
	mov	%esp,	%ebp
	.equ	FR_RETVAL,	-4	# Place to save return value
	.equ	FR_SIZE,	4
	sub	$FR_SIZE,	%esp

	/*
	 * TODO: Finish implementing syscall entry
	 *
	 * Here you probably need to save some state, and if you have
	 * separate user and kernel stacks for your PCB, you'll want
	 * to switch to the kernel stack.
	 *
	 * You can use the stack frame above as storage while you
	 * juggle registers and stacks.
	 *
	 * Once you are ready, you can call the C function
	 * syscall_dispatch to actually execute the syscall.
	 */
	call	abortk

	add	$FR_SIZE,	%esp		# Destroy stack frame
	pop	%ebp
	ret

