# Make setup for the host machine
# ======================================================================

SRC = ../src

include $(SRC)/Makefile.common

all: createimage
all: test

# Unit testing on the host machine
# ======================================================================

TESTPROGS += lib/test_libunittest
TESTPROGS += lib/test_libour_c_core
TESTPROGS += lib/test_libansi_term

$(TESTPROGS): -lunittest
$(TESTPROGS): -lour_c_core
$(TESTPROGS): -lansi_term

.PHONY: unittest
unittest: $(TESTPROGS)
	@echo "Running unit tests..."
	@for testprog in $^; do \
		./$$testprog; \
	done

# Keep test-discovery C files after running the tests
.PRECIOUS: $(addsuffix .c,$(TESTPROGS))

test: unittest

# Unit testing our custom C library on the host machine
# ----------------------------------------------------------------------
# Testing the C library on the host machine is tricky, because we want to
# link both our libc (for testing) and the system's libc (for actual output).
#
# To avoid name collisions and confusion, we build a version of our libc
# that has all global symbols prefixed:
#
#	atoi	becomes	our_atio
#	sprintf	becomes	our_sprintf

# Function: generate a rule snippet that renames all symbols in an archive
# $(1): prefix to add
# $(2): file to edit
define prefix_global_syms
	nm --format bsd $(2) \
		| grep ' [^UN-] ' \
		| cut -d' ' -f3 \
		| sort \
		| uniq \
		| sed -E 's/.*/\0 $(1)\0/g' \
		> $(patsubst %.a,%_renames.txt,$(2))
	objcopy --redefine-syms $(patsubst %.a,%_renames.txt,$(2)) $(2)
endef
lib/libour_c_core.a: $(LIBC_CORE_OBJS)
	ar crs $@ $^
	$(call prefix_global_syms,our_,$@)

lib/test_libour_c_core.c: DISCOVERFLAGS += --prefix our_test_

