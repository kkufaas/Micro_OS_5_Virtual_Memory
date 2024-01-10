/*
 * Low-level preamble for the kernel
 *
 * This file does necessary setup before jumping into the kernel_main function
 * defined in C.
 */

#include <syslib/addrs.h>
#include "asm-offsets.h.s"

	/*
	 * Ensure that this code is at the beginning of the .text section, so
	 * that simple bootloaders can simply jump to the beginning of the
	 * binary.
	 */
	.section	.text.startup,"ax",@progbits

	/*
	 * Dual 16- and 32-bit boot strategy
	 *
	 * - 16-bit code starts at the beginning of the binary.
	 * - 32-bit code is pointed to by _start.
	 */
	.globl _start
	.equ	_start,	_start32

/* === 16-bit start up code === */

	/*
	 * 16-bit Real Mode starting point
	 */
	.code16
	.globl _start16
_start16:
	jmp	main16		# Jump over other code to main function.


/* === Print routines === */

#include "../boot/print16.S"

	/*
	 * Main program for 16-bit startup
	 */
main16:
	cli				# Disable interrupts for now.
	print_str	$str_hello16	# Say hello.
	call	test16			# Run a quick test.
	call	a20_enable		# Enable A20 address line.
	jmp	protected_mode_switch	# Enable Protected mode.

	#.pushsection	.data
str_hello16:	.asciz	"Kernel 16-bit startup\r\n"
	#.popsection


	/*
	 * Run the trivial test and print results
	 */
test16:
	.equ	TEST_REPS,	1000
	print_str	$str_testing

	pushw	$TEST_REPS
	call	trivial_test	# trivial_test(1000)
	addw	$2,	%sp
	cmpw	$TEST_REPS,	%ax
	jne	2f		# If (trivial_test(1000) != 1000) report failure
1:	/* Test OK */
	print_str	$str_test_ok
	jmp	3f
2:	/* Test failed */
	pushw	$str_test_fail
3:	/* End of test */
	ret

	#.pushsection	.data
str_testing:	.asciz	"Running a trivial test... "
str_test_ok:	.asciz	"Seems Ok. Now go get some sleep :)\r\n"
str_test_fail:	.asciz	"*Failed*\r\n"
	#.popsection

	/*
	 * int trivial_test(n)
	 * {
	 *     if (n > 0) {
	 *         trivial_test(n-1);
	 *     }
	 *     return n;
	 * }
	 */
trivial_test:
	pushw	%bp		# Setup stack frame
	movw	%sp,%bp
	movw	4(%bp),%ax	# Move argument to ax
	testw	%ax,%ax		# Logical compare (sets SF, ZF and PF)
	jg	.L2		# if (argument > 0) goto L2
	xorw	%ax,%ax		# else return 0
	popw	%bp
	retw
.L2:
	decw	%ax
	pushw	%ax
	call	trivial_test	# trivial_test(argument - 1)
				# (Recursive calls until argument == 0)
	addw	$2,%sp		# Pop argument
	incw	%ax
	popw	%bp
	retw			# Return (argument in ax)


/* === Switching to Protected Mode === */

	/*
	 * Enable the A20 address line
	 */
a20_enable:
	print_str	$str_a20
	movb	$0x02,	%al
	outb	%al,	$0x92
	ret

	#.pushsection	.data
str_a20:	.asciz	"Enabling the A20 address line...\r\n"
	#.popsection

	/*
	 * Switch from 16-bit Real Mode to 32-bit Protected Mode
	 */
protected_mode_switch:
	.equ	CR0_PE,	0	# CR0 bit 0 is the protection enable bit
	print_str	$str_pe_working

	/* Install the GDT that is defined in cpu.c
	 *
	 * If the kernel is large enough, the gdt_desc value might be too far
	 * away to reach with a 16-bit offset (e.g. 0x10208). Just in case, we
	 * change the data segment (e.g. 0x01020:0008).
	 */
	mov	$gdt_desc,	%eax
	shr	$4,		%eax
	mov	%ax,		%ds	# DS = gdt_desc >> 4
	mov	$gdt_desc,	%eax
	and	$0x000f,	%eax	# EAX = gdt_desc & 0x000f
	lgdt	(%eax)			# Load GDT.

	/* Set the protection-enable bit. */
	mov	%cr0,		%eax
	bts	$CR0_PE,	%eax
	mov	%eax,		%cr0

	/* Set essential segment registers for _start32.
	 * The intialization in _start32 will handle the rest. */
	mov	$KERNEL_DS,	%eax
	mov	%eax,		%ds		# Set DS.
	ljmp	$KERNEL_CS,	$_start32	# Set CS via long jump.

	#.pushsection	.data
str_pe_working:	.asciz	"Switching to Protected Mode... "
	#.popsection


/* === 32-bit start up code === */

	/*
	 * 32-bit Protected Mode starting point
	 */
	.code32
	.globl _start32
_start32:
	cli				# Disable interrupts for now.

	/* Install the GDT that is defined in cpu.c */
	lgdt	gdt_desc

	/* Set segment regisgers. */
	ljmp	$KERNEL_CS,	$1f	# Set CS via long jump.
1:
	mov	$KERNEL_DS,	%eax	# Set data segments.
	mov	%eax,		%ds
	mov	%eax,		%es
	mov	%eax,		%fs
	mov	%eax,		%gs
	mov	%eax,		%ss	# Set stack segment.
	mov	$STACK_PADDR,	%esp	# Init stack.

	/* Transfer control to C code. */
	call	kernel_main
	jmp	halt			# Just in case kernel_main returns.


halt:	/* Halt loop */
	hlt
	jmp	halt
