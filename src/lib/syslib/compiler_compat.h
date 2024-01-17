/*
 * Compiler compatibility
 *
 * This file defines macros for compiler-specific things like function
 * and struct attributes.
 *
 * For now these are all GCC-specific. But having them in a central place
 * will make it easier to switch compilers later.
 *
 * For 'noreturn' simply include <stdnoreturn.h> which defines the 'noreturn'
 * keyword.
 *
 * References:
 *
 * - GCC Attribute Syntax
 *      <https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html>
 *
 * - Clang Attribute Reference
 *      <https://clang.llvm.org/docs/AttributeReference.html>
 */
#ifndef COMPILER_COMPAT_H
#define COMPILER_COMPAT_H

/*
 * Function attr: This function takes a printf pattern and matching arguments
 *
 * Where the function is used, the compiler will check the given arguments
 * against the given pattern to make sure they match. For example if the call
 * has an "%s" in the pattern, the compiler will check that a 'char *' argument
 * is given in the correct position.
 *
 * - PAT_ARG is the argument number of the pattern argument (1-based)
 * - VA_ARG is the argument number of the '...' variable arguments (1-based)
 *
 * See: GCC 'format' attribute:
 *  <https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-format-function-attribute>
 */
#define ATTR_PRINTFLIKE(PAT_ARG, VA_ARG) \
    __attribute__((format(printf, PAT_ARG, VA_ARG)))

/*
 * Function attr: This function may be intentionally unused
 *
 * The compiler should not give an 'unused' warning for this function.
 *
 * See: GCC 'unused' attribute:
 *  <https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-unused-function-attribute>
 */
#define ATTR_UNUSED __attribute__((unused))

/*
 * Function attr: Declare this to the linker as a "weak" symbol
 *
 * This allows libraries to provide a default implementation of a function that
 * may be overridden.
 *
 * See: GCC 'weak' attribute:
 *  <https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-weak-function-attribute>
 */
#define ATTR_WEAK __attribute__((weak))

/*
 * Function attr: This function is an interrupt handler
 *
 * - interrupt: Generates a different function preable/postable suitable for an
 *    interrupt handler (e.g. using an IRET return instruction instead of
 *    a regular RET return)
 *
 * - target("general-regs-only"): Do not use floating-point or SIMD
 *    instructions (MMX/SSD) in generated code for this function. This is
 *    required because neither the x86 interrupt behavior nor the C calling
 *    conventions save the floating-point or SIMD states. Interrupt routines
 *    that use floating-point or SIMD will have to save the state manually.
 *
 * See:
 * - GCC generic 'interrupt' attribute
 *      <https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-interrupt_005fhandler-function-attribute>
 *
 * - GCC x86 'interrupt' attribute
 *      <https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#index-interrupt-function-attribute_002c-x86>
 *
 * - GCC x86 'target("general-regs-only")' attribute:
 *      <https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#index-target_0028_0022general-regs-only_0022_0029-function-attribute_002c-x86>
 */
#define INTERRUPT_HANDLER \
    __attribute__((interrupt)) __attribute__((target("general-regs-only")))

/*
 * Function attr: Make this fn easy to call from hand-written assembly
 *
 * This macro expands to function attributes that make the function easy
 * to call from hand-written assembly.
 *
 * - no_caller_saved_registers: The function will save any registers that it
 *      touches, so the calling code does not need to worry about saving
 *      any registers.
 *
 * - target("general-regs-only"): The function will not use any SIMD
 *      instructions (MMX/SSE). This is required when using
 *      no_caller_saved_registers.
 *
 * See:
 *
 * - GCC x86 'no_caller_saved_registers' attribute:
 *      <https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#index-no_005fcaller_005fsaved_005fregisters-function-attribute_002c-x86>
 *
 * - GCC x86 'target("general-regs-only")' attribute:
 *      <https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#index-target_0028_0022general-regs-only_0022_0029-function-attribute_002c-x86>
 */
#define ATTR_EASY_ASM_CALL \
    __attribute__((no_caller_saved_registers)) \
    __attribute__((target("general-regs-only")))

/*
 * Statement attr: Explicitly mark intentional fallthrough in a switch/case
 */
#define FALLTHROUGH __attribute__((fallthrough))

/*
 * Struct attr: Do not add padding between this struct's members
 *
 * See: GCC 'packed' attr:
 *  <https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html#index-packed-type-attribute>
 */
#define ATTR_PACKED __attribute__((packed))

/*
 * Struct attr: Specify minimum alignment for this type
 *
 * - ALIGNMENT is the alignment in bytes. For example if ALIGNMENT is 8 then
 *  the compiler will ensure instances of the struct are placed on an 8-byte
 *  boundary.
 *
 * See: GCC 'aligned' attr:
 *  <https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html#index-aligned-type-attribute>
 */
#define ATTR_ALIGNED(ALIGNMENT) __attribute__((aligned(ALIGNMENT)))

#endif /* COMPILER_COMPAT_H */
