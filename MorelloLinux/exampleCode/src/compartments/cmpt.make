# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override cmpt_this := $(lastword $(MAKEFILE_LIST))
override cmpt_curdir := $(realpath $(dir $(cmpt_this)))
override cmpt_project := $(notdir $(cmpt_curdir))

override cmpt_objfiles = \
	$(OBJDIR)/$(cmpt_project)/src/manager.c.o \
	$(OBJDIR)/$(cmpt_project)/hellobsp.c.o \
	$(OBJDIR)/$(cmpt_project)/hackpwd.c.o \
	$(OBJDIR)/$(cmpt_project)/mallocTest.c.o \
	$(OBJDIR)/$(cmpt_project)/nestedcmpt.c.o \
	$(OBJDIR)/$(cmpt_project)/hellolpb.c.o \
	$(OBJDIR)/$(cmpt_project)/src/lpb.S.o \
	$(OBJDIR)/$(cmpt_project)/hellolb.c.o \
	$(OBJDIR)/$(cmpt_project)/src/lb.S.o \
	$(OBJDIR)/$(cmpt_project)/privdata.c.o \
	$(OBJDIR)/$(cmpt_project)/privdataException.c.o \
	$(OBJDIR)/$(cmpt_project)/privdataMalloc.c.o \
	$(OBJDIR)/$(cmpt_project)/privdataDlopen.c.o \
	$(OBJDIR)/$(cmpt_project)/privdataScanStatic.c.o \
	$(OBJDIR)/$(cmpt_project)/src/switch.S.o \
    $(OBJDIR)/$(cmpt_project)/src/examine_stack.S.o

$(cmpt_objfiles): CFLAGS += -I$(cmpt_curdir)/include -I$(cmpt_curdir)/../util

main: $(BINDIR)/hellobsp
main: $(BINDIR)/hackpwd
main: $(BINDIR)/nestedcmpt
main: $(BINDIR)/hellolpb
main: $(BINDIR)/hellolb
main: $(BINDIR)/privdata
main: $(BINDIR)/privdataException
main: $(BINDIR)/privdataMalloc
main: $(BINDIR)/privdataDlopen
main: $(BINDIR)/privdataScanStatic
main: $(BINDIR)/mallocTest

$(OBJDIR)/$(cmpt_project)/hackpwd.c.o: CFLAGS := $(filter-out -O%,$(CFLAGS)) -O0 -I$(cmpt_curdir)/include -I$(cmpt_curdir)/../util

$(BINDIR)/hellobsp: $(OBJDIR)/$(cmpt_project)/hellobsp.c.o $(OBJDIR)/$(cmpt_project)/src/manager.c.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static

$(BINDIR)/hackpwd: $(OBJDIR)/$(cmpt_project)/hackpwd.c.o $(OBJDIR)/$(cmpt_project)/src/manager.c.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ \
	-Wl,--dynamic-linker=/morello/musl/lib/libc.so -v \
	-Wl,-rpath,/morello/musl/lib

$(BINDIR)/mallocTest: $(OBJDIR)/$(cmpt_project)/mallocTest.c.o $(OBJDIR)/$(cmpt_project)/src/manager.c.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ \
	-Wl,--dynamic-linker=/morello/musl/lib/libc.so -v \
	-Wl,-rpath,/morello/musl/lib

$(BINDIR)/nestedcmpt: $(OBJDIR)/$(cmpt_project)/nestedcmpt.c.o $(OBJDIR)/$(cmpt_project)/src/manager.c.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static

$(BINDIR)/hellolpb: $(OBJDIR)/$(cmpt_project)/hellolpb.c.o $(OBJDIR)/$(cmpt_project)/src/lpb.S.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static

$(BINDIR)/hellolb: $(OBJDIR)/$(cmpt_project)/hellolb.c.o $(OBJDIR)/$(cmpt_project)/src/lb.S.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static

$(BINDIR)/privdata: $(OBJDIR)/$(cmpt_project)/privdata.c.o $(OBJDIR)/$(cmpt_project)/src/switch.S.o $(OBJDIR)/$(cmpt_project)/src/examine_stack.S.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static

$(BINDIR)/privdataMalloc: $(OBJDIR)/$(cmpt_project)/privdataMalloc.c.o $(OBJDIR)/$(cmpt_project)/src/switch.S.o $(OBJDIR)/$(cmpt_project)/src/examine_stack.S.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ \
	-Wl,--dynamic-linker=/morello/musl/lib/libc.so -v \
	-Wl,-rpath,/morello/musl/lib

$(BINDIR)/privdataScan: $(OBJDIR)/$(cmpt_project)/privdataDlopen.c.o $(OBJDIR)/$(cmpt_project)/src/switch.S.o $(OBJDIR)/$(cmpt_project)/src/examine_stack.S.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ \
	-Wl,--dynamic-linker=/morello/musl/lib/libc.so -v \
	-Wl,-rpath,/morello/musl/lib

$(BINDIR)/privdataScanStatic: $(OBJDIR)/$(cmpt_project)/privdataScanStatic.c.o $(OBJDIR)/$(cmpt_project)/src/switch.S.o $(OBJDIR)/$(cmpt_project)/src/examine_stack.S.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static

$(BINDIR)/privdataException: $(OBJDIR)/$(cmpt_project)/privdataException.c.o $(OBJDIR)/$(cmpt_project)/src/switch.S.o $(OBJDIR)/$(cmpt_project)/src/examine_stack.S.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ -static


$(cmpt_objfiles): $(cmpt_this)
