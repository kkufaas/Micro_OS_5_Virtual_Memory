/*
 * Export constants from C to assembly
 *
 * This file makes struct offsets and other constants that must be calculated
 * by the C compiler available to hand-written assembly code.
 *
 *   1. When compiled to assembly, this file will contain '.equ' constant
 *      definitions for the C constant expressions given.
 *
 *   2. The resulting assembly file will then be postprocessed by Make into
 *      something that other assembly files can #include.
 *
 * This approach is adapted from the Linux kernel's build process.
 * See the Makefile for post-processing on the resulting assembly.
 * See the kernel's many asm-offsets.c files for the really gory details.
 */

#include <stddef.h>

#include "kernel.h"

/*
 * Define an assembly constant
 *
 * This macro emits inline asm that contains not opcodes,
 * but constant definitions.
 *
 * Example:
 *
 *    This macro call:        ASM_EQU(PCB_PID, offsetof(struct pcb, pid));
 *    compiles to this ASM:   .equ PCB_PID, 0 # offsetof(struct pcb, pid)
 *
 * Inline asm notes:
 *
 *  - "%c0" formats the 0th value as a bare constant (no $ prefix)
 *  - the "n" constraint limits the value to numeric constants
 *
 * See the GCC manual's chapter on Extended Inline ASM constraints:
 *  https://gcc.gnu.org/onlinedocs/gcc/Simple-Constraints.html
 */
#define ASM_EQU(SYM, EXPR) \
    asm volatile(".equ\t" #SYM ",\t%c0\t# " #EXPR : : "n"(EXPR))

/* ASM_EQU shortcut for a single symbol */
#define ASM_CONST(SYM) ASM_EQU(SYM, SYM)

/* ASM_EQU shortcut for a struct offset  */
#define ASM_OFFSET(SYM, STRUCT, MEMBER) ASM_EQU(SYM, offsetof(STRUCT, MEMBER))

/*
 * This container function is required by the C compiler (inline asm must be in
 * a function body). The preamble and postable will be stripped away by the
 * postprocessing step.
 */
void foo()
{
    ASM_CONST(KERNEL_CS);
    ASM_CONST(KERNEL_DS);

}
