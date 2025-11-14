override findcaps_this := $(lastword $(MAKEFILE_LIST))
override findcaps_curdir := $(realpath $(dir $(findcaps_this)))
override findcaps_project := $(notdir $(findcaps_curdir))

ifeq ($(COMPILER_FAMILY),clang)
override FINDCAPS_LFLAGS = -lunwind -lc++abi -Wl,-warning-limit=1
else
override FINDCAPS_LFLAGS =
endif

override findcaps_objfiles = \
	$(OBJDIR)/$(findcaps_project)/findcaps.c.o

main: $(BINDIR)/findcaps

$(BINDIR)/%: $(OBJDIR)/$(findcaps_project)/%.c.o $(OBJDIR)/libutil.a | $(BINDIR)
	$(CC) $(LFLAGS) $^ -o $@ \
	-Wl,--dynamic-linker=/morello/musl/lib/libc.so -v \
	-Wl,-rpath,/morello/musl/lib
 

$(findcaps_objfiles): CFLAGS += -I$(findcaps_curdir)/../util

$(findcaps_objfiles): $(findcaps_this)
