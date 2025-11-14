# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override strm_this := $(lastword $(MAKEFILE_LIST))
override strm_curdir := $(realpath $(dir $(strm_this)))
override strm_project := $(notdir $(strm_curdir))

override strm_objects = \
	$(OBJDIR)/$(strm_project)/restricted.c.o \
	$(OBJDIR)/$(strm_project)/src/start.S.o \
	$(OBJDIR)/$(strm_project)/src/cman.c.o

$(OBJDIR)/libfree.a: $(free_objects) $(util_objects)
	$(create-archive)

main: $(BINDIR)/restricted | $(BINDIR)

$(BINDIR)/restricted: $(strm_objects) $(OBJDIR)/libfree.a
	$(CC) -nostdlib -ffreestanding $(FREE_LFLAGS) $^ $(PURECAP_CRTLIB) -o $@ -static

$(strm_objects): CFLAGS = $(FREE_CFLAGS) -nostdinc -ffreestanding
$(strm_objects): CFLAGS += -I$(free_curdir)/include -I$(util_curdir) -I$(strm_curdir)/include -I$(COMPILER_INCLUDE_DIR)

$(strm_objects): $(strm_this)
