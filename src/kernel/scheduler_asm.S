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
	 * This function gets called to enter the scheduler, saving registers
	 * before doing so.
	 */
	.globl  scheduler_entry
scheduler_entry:
	/*
	 * TODO: Implement scheduler entry
	 */
	call	abortk

.text

	.global dispatch
dispatch:
	/* TODO: Implement task dispatch.
	 * For a previously running task, switch to the correct stack,
	 * restore necessary state, and then return to resume execution.
	 * For a new task, set up necessary state and then jump to the
	 * start address. */
	call	abortk

