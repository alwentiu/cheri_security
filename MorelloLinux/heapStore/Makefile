# SPDX-License-Identifier: BSD-3-Clause

# This Makefile will compile an helloworld application using Morello Clang and the Musl libc compiled for purecap.

CC=clang
# ELF_PATCH is used to check that the correct elf flag for Morello is present
ELF_PATCH=morello_elf
MUSL_HOME?=../../musl-bin
CLANG_RESOURCE_DIR=$(shell clang -print-resource-dir)

NAME=morello-dispatch
OUT=./bin
# we want the same result no matter where we're cross compiling (x86_64, aarch64)
TARGET?=aarch64-linux-musl_purecap

LIBS = -lrary -L./lib/
LIBOUT = ./lib
CFLAGS = -g -shared -fPIC

all:
	mkdir -p $(OUT)
	mkdir -p $(LIBOUT)

	$(CC) -c -g $(CFLAGS) -march=morello -mabi=purecap \
	--target=$(TARGET) --sysroot $(MUSL_HOME) \
	-fno-stack-protector \
	library.c -o $(OUT)/library.o 
	
	$(CC) -shared -fuse-ld=lld -march=morello -mabi=purecap \
		--target=$(TARGET) --sysroot $(MUSL_HOME) \
		-rtlib=compiler-rt \
		$(OUT)/library.o \
		-o $(LIBOUT)/library.so \
		-Wl,--dynamic-linker=/morello/musl/lib/libc.so -v\
		-Wl,-rpath,/morello/musl/lib\
		-Wl,-z,execstack\

	$(ELF_PATCH) $(LIBOUT)/library.so
	rm $(OUT)/library.o


	$(CC) -c -g -march=morello -mabi=purecap \
		--target=$(TARGET) --sysroot $(MUSL_HOME) \
		-fno-stack-protector \
		main.c -o $(OUT)/$(NAME).o

	# Use -static to build a static binary and remove "-Wl,--dynamic-linker=/morello/musl/lib/libc.so" and "-Wl,-rpath,/morello/musl/lib"
	$(CC) -fuse-ld=lld -march=morello -mabi=purecap \
		--target=$(TARGET) --sysroot $(MUSL_HOME) \
		-rtlib=compiler-rt \
		$(OUT)/$(NAME).o \
		-o $(OUT)/$(NAME) \
		-Wl,--dynamic-linker=/morello/musl/lib/libc.so -v\
		-Wl,-rpath,/morello/musl/lib\
		-Wl,-z,execstack\
		-Wl,-rpath,$(LIBOUT) $(LIBS)
	$(ELF_PATCH) $(OUT)/$(NAME)
	rm $(OUT)/$(NAME).o
clean:
	rm ($OUT)/$(NAME)
