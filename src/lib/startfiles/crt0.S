/*
 * Simple C runtime file
 *
 * This file is the interface between the OS's way of starting/ending a process
 * and the C `main` function. It provides the `_start` symbol which is the true
 * start of the program. `_start` should call `main` and then call `exit` when
 * `main` returns.
 */

	/*
	 * Note: Choose section wisely
	 *
	 * Our current image format pulls executable binaries out of their ELF
	 * containers, which means we lose the helpful entry point metadata
	 * that tells us where the `_start` point is located. The easiest way
	 * to compensate for this is to make sure that `_start` is at the
	 * beginning of the binary, so that load address and start address are
	 * the same.
	 *
	 * In order to do that, we have to link this file first. We also have
	 * to watch out for the C compiler breaking the `.text` section into
	 * subsections with different priorities, e.g. `.text.startup`,
	 * `.text.hot`, `.text.unlikely`, etc. If that is the case, then we
	 * need to make sure this startup code is in a high-priority
	 * subsection.
	 *
	 * Subsections and priority depend on the C compiler and the linker
	 * script. Use the following commands to check them:
	 *
	 *      # Disassemble the binary to check if `_start` comes first:
	 *      $ objdump -d process1 | less
	 *
	 *      # Check compiler-generated assembly to see what sections and
	 *      # subsections the compiler uses:
	 *      $ make process1.s && less process1.s
	 *
	 *      # Check the default linker script to see subsection link order:
	 *      $ ld -m elf_i386 --verbose | less
	 *
	 * See also:
	 *
	 * - GNU as, Background on sections:
	 *      <https://sourceware.org/binutils/docs/as/Secs-Background.html>
	 *
	 * - GNU as, `.section` directive:
	 *      <https://sourceware.org/binutils/docs/as/Section.html>
	 */
	.section	.text.startup,"ax",@progbits

	.code32
	.globl	_start
_start:
	call	main
	call	exit
