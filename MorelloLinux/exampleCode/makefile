# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

ifeq (,$(wildcard config.make))
$(error "Run configure to generate config.make")
endif

include config.make
include macros.make

override C_SOURCES = $(wildcard src/*/*.c) $(wildcard src/*/*/*.c)
override ASM_SOURCES = $(wildcard src/*/*.S) $(wildcard src/*/*/*.S)
override CXX_SOURCES = $(wildcard src/*/*.cpp) $(wildcard src/*/*/*.cpp)

override SOURCES  = $(C_SOURCES)
override SOURCES += $(ASM_SOURCES)
override SOURCES += $(CXX_SOURCES)

override OBJECTS  = $(C_SOURCES:src/%.c=$(OBJDIR)/%.c.o)
override OBJECTS += $(ASM_SOURCES:src/%.S=$(OBJDIR)/%.S.o)
override OBJECTS += $(CXX_SOURCES:src/%.cpp=$(OBJDIR)/%.cpp.o)

override DEPENDS = $(OBJECTS:%.o=%.d)
override OBJDIRS = $(sort $(dir $(OBJECTS)))

main:

# include subprojects here
include src/util/util.make
include src/hello/hello.make
include src/freestanding/free.make
include src/compartments/cmpt.make
include src/restricted/restricted.make
include src/findcaps/findcaps.make
ifeq ($(HYBRID),1)
include src/hybrid/hybrid.make
else
$(warning Build of hybrid code is not supported)
endif

# common compilation rules: purecap
$(OBJDIR)/%.c.o: src/%.c | $(OBJDIRS)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.S.o: src/%.S | $(OBJDIRS)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.cpp.o: src/%.cpp | $(OBJDIRS)
	$(CXX) -c $(CXXFLAGS) $< -o $@

# common compilation rules: hybrid
$(OBJDIR)/%.c.ho: src/%.c | $(OBJDIRS)
	$(CC) -c $(CFLAGS_HYBRID) $< -o $@

$(OBJDIR)/%.S.ho: src/%.S | $(OBJDIRS)
	$(CC) -c $(CFLAGS_HYBRID) $< -o $@

$(OBJDIR)/%.cpp.ho: src/%.cpp | $(OBJDIRS)
	$(CXX) -c $(CXXFLAGS_HYBRID) $< -o $@

# clean targets
clean:
	@rm -rf $(BUILD)

distclean: clean
	@rm -vf config.make

# rebuild when necessary
-include $(DEPENDS)

$(OBJECTS): makefile config.make

$(OBJDIRS):
	@mkdir -p $@

$(BINDIR) $(LIBDIR):
	@mkdir -p $@

# tests
include test/test.make

.PHONY: main clean distclean
