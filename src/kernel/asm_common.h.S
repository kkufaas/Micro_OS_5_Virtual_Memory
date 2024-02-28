/*
 * Common macros for assembly code
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
#ifndef ASM_COMMON_H_S
#define ASM_COMMON_H_S

	/* Save all registers, except eflags */
.macro SAVE_GEN_REGS
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	pushl	%esi
	pushl	%ebp
.endm

	/* Restore all registers, except eflags */
.macro RESTORE_GEN_REGS
	popl	%ebp
	popl	%esi
	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
.endm

.macro SAVE_EFLAGS
	pushfl
.endm

.macro RESTORE_EFLAGS
	popfl
.endm

	/*
	 * Save all floating point registers on the stack
	 *
	 * There are three options for saving floating point state:
	 *
	 * - FSAVE: Saves x87 FPU state, with FP exception checking
	 * - FNSAVE: Saves x87 FPU state, without FP exception checking
	 * - FXSAVE: Saves x87 FPU and MMX state.
	 *	The FXSAVE memory area must be aligned to a 16-byte boundary.
	 *
	 * We use FNSAVE here because we do not need FSAVE's exception checking
	 * or FXSAVE's MMX storage (we are not using MMX).
	 */
	.equ	FP_STATE_SIZE,	108
.macro SAVE_FP_REGS
	subl	$FP_STATE_SIZE,	%esp
	fnsave	0(%esp)
	fwait
.endm

	/* Restore floating point registers */
.macro RESTORE_FP_REGS
	frstor	0(%esp)
	addl	$FP_STATE_SIZE,	%esp
.endm

	/*
	 * Load kernel data segment descriptor into DS and ES
	 *
	 * During an interrupt, the CPU automatically switches the code segment
	 * descriptor (CS) and the stack segment descriptor (DS). So interrupt
	 * handlers only need to switch the data segments, and only DS and ES
	 * are used in our kernel (they are used in C code).
	 */
.macro LOAD_KERNEL_DATA_SEGMENTS	scratch=%eax
	mov	$KERNEL_DS,	\scratch
	mov	\scratch,	%ds
	mov	\scratch,	%es
.endm

.macro SAVE_DATA_SEGMENTS
	push	%ds
	push	%es
.endm

.macro	RESTORE_DATA_SEGMENTS
	pop	%es
	pop	%ds
.endm

#endif /* ASM_COMMON_H_S */
