# Make setup for the target machine
# ======================================================================

SRC = ../src

include $(SRC)/Makefile.common

all: boot/bootblock kernel/kernel $(PROCESSES)
all: test

# Compiler flags for target
# ======================================================================

CC = gcc
LD = gcc

# CPU architecture
TARGET_ARCH += -m32#		32-bits
TARGET_ARCH += -march=i386#	Basic 386, no extras like MMX/SSE instructions
TARGET_ARCH += -mpreferred-stack-boundary=2#	Don't pad the stack for perf

# Freestanding code (no standard libraries)
CFLAGS += -ffreestanding#	Use only the compiler-provided subset of libc

# Undo modern linux defaults, for compatibility
CFLAGS += -fno-stack-protector
CFLAGS += -fcf-protection=none

# Undo modern linux defaults, for simpler ASM that's easier to debug
CFLAGS += -fno-pic#				No Position-Independent Code
CFLAGS += -fno-asynchronous-unwind-tables#	No metadata for C++ exceptions

# Linking
LDFLAGS += -static#			No dynamic library loading
LDFLAGS += -Wl,--build-id=none#		No .note.gnu.build-id section
LDFLAGS += -Wl,--nmagic#		Simple linking into all one segment
LDFLAGS += -Wl,-z,norelro#		No dynamic relocation information

# Use our system libraries instead of the host's

LDFLAGS += -nolibc#		Skip the C standard library
LDFLAGS += -nostartfiles#	Skip C runtime start files like crt0.o

CPPFLAGS += -isystem $(SRC)/lib/libc#	Use our libc header files

# Linking executables for target
# ======================================================================

# Function: get a memory address constant from the addrs.h header
extractaddr = $(shell echo '$(1)' \
		| $(CPP) $(CPPFLAGS) -E -include syslib/addrs.h - \
		| grep -v '#')

# Linking bootblock
boot/bootblock: LDFLAGS += -Ttext 0x0

# Linking kernel
kernel/kernel: LDFLAGS += -Ttext $(call extractaddr,KERNEL_PADDR)

