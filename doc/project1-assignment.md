# Project 1: Bootup Mechanism

INF-2201: Operating Systems Fundamentals,
Spring 2024,
UiT The Arctic University of Norway

  - [Demo Project](#demo-project)
  - [Important Source Files](#important-source-files)
      - [Hints for bootblock.S](#hints-for-bootblock.s)
      - [Hints for createimage.c](#hints-for-createimage.c)
  - [Low-level Details](#low-level-details)
      - [Display memory](#display-memory)
      - [BIOS Int 0x13 Function 2](#bios-int-0x13-function-2)
      - [BIOS Int 0x10 Function 0x0e](#bios-int-0x10-function-0x0e)

# Demo Project

This semester the project is being run as a demo. You do not have to
hand anything in. Your task is to set up your build environment and
start familiarizing yourself with the code and the tools before Project
2.

This project focuses on the bootup code for a simple operating system
that we will be developing for the rest of the semester. The bootup code
will be in real mode. A preamble to the kernel will transition the
processor into protected mode.

A PC can be booted up in two modes: cold boot or warm boot. Cold boot
means you power down and power up the machine by pressing the power
button of a PC. Warm boot means that you reset the running OS. Either
way, the PC will reset the processor and run a piece of code in the BIOS
to try to read the first sector from the usb drive if found, otherwise,
it will try to read a sector from the hard disk drive. Once this sector
is read in, the CPU will jump to the beginning of the loaded code.

This project demonstrates a bootloader and a disk image creation utility
(`createimage`). The bootloader resides on the boot sector and its
responsibility is to load the rest of the operating system image from
other sectors to the memory. Createimage is a tool (in this case a Linux
tool) to create a bootable operating system image on a USB disk. The
image will include the bootblock and the rest of the operating system.

The bootblock and createimage from this assignment will be used
throughout the semester.

# Important Source Files

    src/
    |-- boot/
    |   |-- bootblock.S         # The bootblock itself
    |   `-- print16.S           # 16-bit print routines based in BIOS INT 10H
    |-- kernel/
    |   |-- hardware/
    |   |   |-- cpu_x86.h       # C abstractions for x86 CPUs
    |   |   |-- serial.c        # C abstractions for PC serial port
    |   |   `-- serial.h
    |   |-- lib/
    |   |   |-- assertk.c       # In-kernel version of C asser
    |   |   |-- assertk.h
    |   |   |-- printk.c        # Kernel logging facility
    |   |   `-- printk.h
    |   |-- asm-offsets.c       # Export constants from C to ASM
    |   |-- cpu.c               # Kernel CPU initialization (Global Dec. Table)
    |   |-- cpu.h
    |   |-- kernel.c            # Kernel main source
    |   |-- kernel.h
    |   `-- kernel_start.S      # Kernel preamble ASM
    |-- lib/
    |   |-- ansi_term/          # A terminal abstraction on top of VGA text mem
    |   |-- libc/               # Our OS's own subset of the C standard library
    |   |-- syslib/
    |   |   |-- addrs.h         # Important memory addresses
    |   |-- unittest/           # A simple C unittest framework
    |   `-- util/               # Other miscellaneous utilities
    `-- createimage.c           # The createimage program

## Hints for bootblock.S

This file contains the code that will be written on the boot sector of
the USB disk. This has to be written in x86 assembly and should not
exceed 512 bytes (1 sector). This code will be responsible for loading
in the rest of the operating system, setting up a stack for the kernel
and then invoking it.

The bootblock gets loaded at 0x07c0:0000. The bootblock should load the
OS starting at 0x0000:1000. In real mode, you have 640 KBytes starting
at 0x0000:0000. The low memory area has some system data like the
interrupt vectors and BIOS data. So, to avoid overwriting them, the
bootblock only uses memory above 0x0000:1000 for this assignment.

To understand this implementation, you need to learn about x86
architecture, CPU resets and how to read a sector from the USB drive
with BIOS (described below).

Useful information and documentation:

  - Appendix A in Computer Organization and Design by Patterson &
    Hennessy describes how the linker and loader works.

  - [Intel® 64 and IA-32 Architectures Software Developer
    Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html).
    The reference for all things x86 assembler. Get all the software
    developers manuals (Volume 1-3). Note that Intel documents use the
    intel assembler syntax, whereas you use the AT\&T assembler syntax.

  - [A Guide to Programming Pentium/Pentium Pro
    Processors](doc/x86/pentium-programming-kai-lee.pdf). A good
    introduction to get you started (or refreshed) on assembler. Covers
    both Intel and AT\&T syntax.

  - [x86 Assembly
    Guide](https://flint.cs.yale.edu/cs421/papers/x86-asm/asm.html).
    Basics of 32-bit x86 assembly language programming.

## Hints for createimage.c

This program is a Linux tool to create a bootable image. A [man
page](doc/createimage-man-page.md) describing its behavior is available.
You can ignore the “-vm” option for this project.

You should read the man pages of:

  - [createimage](doc/createimage-man-page.md)
  - od(1)
  - objdump(1)

In addition, [Executable and Linkable Format
(ELF)](doc/abi/tool-interface-standard-elf-v1.1.pdf) has more
information about the ELF format that is used by create image.

# Low-level Details

## Display memory

During booting, you can write directly to the screen by writing to the
display RAM which is mapped starting at `0xb800:0000`. Each location on
the screen requires two bytes—one to specify the attribute (**Use
`0x07`**) and the second for the character itself. The text screen is
80x25 characters. So, to write to i-th row and j-th column, you write
the 2 bytes starting at offset `((i-1)*80+(j-1))*2`.

So, the following code sequence writes the character ‘K’ (ascii `0x4b`)
to the top left corner of the screen.

`movw 0xb800,%bx` `movw %bx,%es` `movw $0x074b,%es:(0x0)`

This code sequence is very useful for debugging.

## BIOS Int 0x13 Function 2

Reads a number of 512 bytes sectors starting from a specified location.
The data read is placed into RAM at the location specified by ES:BX. The
buffer must be sufficiently large to hold the data AND must not cross a
64K linear address boundary.

Called with:

    ah = 2 
    al = number of sectors to read, 1 to 36 
    ch = track number, 0 to 79 
    cl = sector number, 1 to 36 
    dh = head number, 0 or 1 
    dl = drive number, 0 to 3 
    es:bx = pointer where to place information read 

Returns:

    ah = return status (0 if successful) 
    al = number of sectors read 
    carry = 0 successful, = 1 if error occurred 

For more information see documentation at Stanislav’s Interrupt List for
[INT 13 - Diskette BIOS
Services](https://stanislavs.org/helppc/int_13.html).

## BIOS Int 0x10 Function 0x0e

This function sends a character to the display at the current cursor
position on the active display page. As necessary, it automatically
wraps lines, scrolls and interprets some control characters for specific
actions. Note : Linefeed is 0x0a and carriage return is 0x0d.

Called with:

    ah = 0x0e 
    al = character to write 
    bh = active page number (Use 0x00) 
    bl = foreground color (graphics mode only) (Use 0x02) 

Returns:

    character displayed 

For more information see documentation at Stanislav’s Interrupt List for
[INT 10 - Video BIOS
Services](https://stanislavs.org/helppc/int_10.html).
