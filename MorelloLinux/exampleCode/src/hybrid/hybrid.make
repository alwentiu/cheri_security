# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override hybrid_this := $(lastword $(MAKEFILE_LIST))
override hybrid_curdir := $(realpath $(dir $(hybrid_this)))
override hybrid_project := $(notdir $(hybrid_curdir))

override hybrid_objects = \
	$(OBJDIR)/$(hybrid_project)/hellohybrid.c.ho

main: $(BINDIR)/hellohybrid

$(BINDIR)/hellohybrid: $(hybrid_objects) $(OBJDIR)/libutil.ha | $(BINDIR)
	$(CC) $(LFLAGS_HYBRID) $^ -o $@ -static

$(hybrid_objects): CFLAGS_HYBRID += -I$(hybrid_curdir)/../util

ifeq ($(COMPILER_FAMILY),clang)
$(hybrid_objects): CFLAGS_HYBRID += -mllvm -aarch64-enable-range-checking=true
else
$(hybrid_objects): CFLAGS_HYBRID += -Wno-cheri-implicit-pointer-conversion-to-cap -Wno-cheri-explicit-pointer-conversion-to-cap
endif

$(hybrid_objects): $(hybrid_this)
