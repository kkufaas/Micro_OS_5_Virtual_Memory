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

	.globl  timer_isr_entry
timer_isr_entry:
	/* You must add code here to handle the timer interrupt. */
	iret

