# Major Targets
# ======================================================================
# "Phony" targets are targets that are not themselves files. Instead, they are
# shorthand for common tasks. Here are the major tasks defined in this file:

.PHONY: all		# Default: build the OS disk image and support files
.PHONY: test		# Run automated tests
.PHONY: run		# Run the OS in an emulator
.PHONY: debug		# Run the OS in an emulator in debug mode
.PHONY: clean		# Remove most generated files
.PHONY: distclean	# Remove all non-source files

# Put the 'all' target first so that it becomes the default
all: image test
run: bochsrun
debug: bochsdebug

# Compiling for Target Machine vs Host Machine
# ======================================================================
# We need to be able to compile for two different machines:
#
# 1. the target: the machine that our OS will run on
# 2. the host: this machine which you are using to build and develop the OS
#
# To do that, we have separate output directories for host and target,
# each with its own Makefile that sets up the correct CFLAGS and other
# target-specific options.

.PHONY: always_check

# To make things for the host, we delegate to the Makefile in the host/ dir
host/%: always_check
	@$(MAKE) -C host $*

# To make things for the target, we delegate to the Makefile in the target/ dir
target/%: always_check
	@$(MAKE) -C target $*

# Both Makefiles will reach back into this directory and include
# Makefile.common, which has common definitions for both host and target.

# Creating the Disk Image
# ======================================================================

CREATEIMAGE	= ./host/createimage
BOOTBLOCK	= target/boot/bootblock
KERNEL		= target/kernel/kernel

CREATEIMAGE_FLAGS += --extended

host/createimage:
	$(MAKE) -C host createimage

image: $(CREATEIMAGE) $(BOOTBLOCK) $(KERNEL) $(PROCESSES)
	$(CREATEIMAGE) $(CREATEIMAGE_FLAGS) $(BOOTBLOCK) $(KERNEL) $(PROCESSES)

# Create symbol files for Bochs debugging
bochsdebug: $(addsuffix .sym,$(BOOTBLOCK) $(KERNEL) $(PROCESSES))

# Unit testing
# ======================================================================

.PHONY: unittest
unittest:
	$(MAKE) -C host unittest

test: unittest

# Emulation: Bochs
# ======================================================================

.PHONY: bochsrun bochsdebug bochsclean bochsdistclean

# Launch bochs to test the image
# bochs reads debug commands from stdin, so passing "c" starts it immediately.
bochsrun: image
	echo "c" | bochs $(BOCHSFLAGS)

## Bochs debug symbols
#
# These are lists of symbols that can be loaded into Bochs's internal debugger
# with the "ldsym" command.
#
#     ldsym "kernel.sym"
#     b "_start"
#
# The format is similar to the output of 'nm' in BSD mode. We just have to cut
# out the center field.
#
#     nm output (BSD mode):     7c00 t _start
#     Bochs symbol format:      7c00 _start
#
# See Bochs's documentation:
#     https://bochs.sourceforge.io/doc/docbook/user/internal-debugger.html
#
target/%.sym: target/%
	nm -f bsd $< | cut -d' ' -f1,3 > $@

bochsdebug: image
	bochs $(BOCHSFLAGS)

# On clean, remove debugging symbol files
clean: bochsclean
.PHONY: bochsclean
bochsclean:
	$(RM) *.sym

# On distclean, remove saved run output and debug session
distclean: bochsdistclean
.PHONY: bochsdistclean
bochsdistclean:
	$(RM) serial.out bochsdebug.out

# Cleanup
# ======================================================================

# Clean most generated files, but leave some things that we might want to save,
# such as debugger output and things that take a lot of work to build.
clean:
	$(MAKE) -C host clean
	$(MAKE) -C target clean
	$(RM) $(BOOTBLOCK) $(KERNEL) $(PROCESSES) $(CREATEIMAGE)
	$(RM) image

# Clean everything that is not a source file,
# as in "clean the source tree for distribution."
distclean: clean
	$(RM) *~
	$(RM) \#*
	$(RM) *.bak
