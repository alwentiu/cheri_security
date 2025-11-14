# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override free_this := $(lastword $(MAKEFILE_LIST))
override free_curdir := $(realpath $(dir $(free_this)))
override free_project := $(notdir $(free_curdir))

override free_objects := \
	$(OBJDIR)/$(free_project)/src/syscalls.c.o \
	$(OBJDIR)/$(free_project)/src/init.c.o \
	$(OBJDIR)/$(free_project)/src/printf.c.o \
	$(OBJDIR)/$(free_project)/src/string.c.o \
	$(OBJDIR)/$(free_project)/src/auxv.c.o

override free_objfiles := $(free_objects)
override free_objfiles += $(OBJDIR)/$(free_project)/listauxv.c.o
override free_objfiles += $(OBJDIR)/$(free_project)/selftest.c.o
override free_objfiles += $(OBJDIR)/$(free_project)/hackfmt.c.o

main: $(BINDIR)/listauxv
main: $(BINDIR)/selftest
main: $(BINDIR)/hackfmt

override FREE_CFLAGS := $(filter-out --sysroot=%,$(CFLAGS))
override FREE_LFLAGS := $(filter-out --sysroot=%,$(LFLAGS))
ifeq ($(COMPILER_FAMILY),clang)
override FREE_LFLAGS += -Wl,--local-caprelocs=elf
endif

$(free_objfiles): CFLAGS = $(FREE_CFLAGS) -nostdinc -ffreestanding -I$(free_curdir)/include -I$(free_curdir)/../util -I$(COMPILER_INCLUDE_DIR)

ifeq ($(COMPILER_FAMILY),gcc)
$(OBJDIR)/$(free_project)/src/printf.c.o: CFLAGS += -Wno-stringop-overflow
endif

$(OBJDIR)/$(free_project)/selftest.c.o: CFLAGS += -I$(free_curdir)/../util

$(BINDIR)/%: $(OBJDIR)/$(free_project)/%.c.o $(free_objects) $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) -nostdlib -ffreestanding $(FREE_LFLAGS) $^ -o $@ -static

$(free_objfiles): $(free_this)
