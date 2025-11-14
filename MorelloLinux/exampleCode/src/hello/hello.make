# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override hello_this := $(lastword $(MAKEFILE_LIST))
override hello_curdir := $(realpath $(dir $(hello_this)))
override hello_project := $(notdir $(hello_curdir))

ifeq ($(COMPILER_FAMILY),clang)
override HELLO_LFLAGS = -lunwind -lc++abi -Wl,-warning-limit=1
else
override HELLO_LFLAGS =
endif

override hello_objfiles = \
	$(OBJDIR)/$(hello_project)/hello.c.o \
	$(OBJDIR)/$(hello_project)/subobject.c.o

main: $(BINDIR)/hello
main: $(BINDIR)/subobject

$(BINDIR)/%: $(OBJDIR)/$(hello_project)/%.c.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static

$(BINDIR)/hellocpp: $(OBJDIR)/$(hello_project)/hello.cpp.o | $(BINDIR)
	$(CXX) $(LFLAGS) $^ -o $@ $(HELLO_LFLAGS) -static

$(hello_objfiles): CFLAGS += -I$(hello_curdir)/../util

ifeq ($(COMPILER_FAMILY),clang)
$(OBJDIR)/$(hello_project)/subobject.c.o: CFLAGS += -Xclang -cheri-bounds=subobject-safe
else
$(OBJDIR)/$(hello_project)/subobject.c.o: CFLAGS += -Wno-attributes
endif

$(hello_objfiles): $(hello_this)
