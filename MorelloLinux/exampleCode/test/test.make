# Copyright (c) 2023 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

override test_this := $(lastword $(MAKEFILE_LIST))
override test_curdir := $(realpath $(dir $(test_this)))

test:
	$(TEST_RUNNER) $(BINDIR)/hello
	$(TEST_RUNNER) $(BINDIR)/subobject
	$(TEST_RUNNER) $(BINDIR)/selftest
	$(TEST_RUNNER) $(BINDIR)/listauxv
	$(TEST_RUNNER) $(BINDIR)/hellobsp
	$(TEST_RUNNER) $(BINDIR)/nestedcmpt || test $$? -eq 3
	$(TEST_RUNNER) $(BINDIR)/hellolpb
	$(TEST_RUNNER) $(BINDIR)/hellolb
	$(TEST_RUNNER) $(BINDIR)/privdata
	$(TEST_RUNNER) $(BINDIR)/restricted
	$(TEST_RUNNER) $(BINDIR)/hellohybrid

.PHONY: test
