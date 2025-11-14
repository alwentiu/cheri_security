# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override util_this := $(lastword $(MAKEFILE_LIST))
override util_curdir := $(realpath $(dir $(util_this)))
override util_project := $(notdir $(util_curdir))

override util_objects = \
	$(OBJDIR)/$(util_project)/capprint.c.o \
	$(OBJDIR)/$(util_project)/morello.c.o

override util_objects_hybrid = $(util_objects:%.c.o=%.c.ho)

main: $(OBJDIR)/libutil.a

ifeq ($(HYBRID),1)
main: $(OBJDIR)/libutil.ha
endif

$(util_objects): CFLAGS += -nostdinc -I$(COMPILER_INCLUDE_DIR)

$(OBJDIR)/libutil.a: $(util_objects) | $(LIBDIR)
	$(create-archive)

$(OBJDIR)/libutil.ha: $(util_objects_hybrid) | $(LIBDIR)
	$(create-archive)

$(util_objects): $(util_this)
$(util_objects_hybrid): $(util_this)
