# Morello Examples

This repository contains a number of example programs that are relevant
for Morello and its applications. The aim of these examples is to show
various features and properties of Morello and provide an easy-to-start
codebase for learning and experimentation.

The examples included so far are listed below. It is recommended to go
through them in this order:

 - Simple hello world examples.
 - A series of freestanding apps that are deliberately "safe": see
   [README.md](src/freestanding/README.md) for more details.
 - Several compartmentalisation implementations with relevant test
   applications: see [README.md](src/compartments/README.md).
 - Another way of implementing compartments using switch to restricted
   mode: see [README.md](src/restricted/README.md).
 - Some hybrid examples: see [README.md](src/hybrid/README.md).

## How to Build

Prerequisites:

 - Morello [GCC toolchain (with purecap Glibc)][gnu] or [Morello LLVM][llvm]
   (with [purecap Musl][musl]).
 - Bash 5.x and Make 4.x.

To compile and link the examples, run configure and then make:

    ./configure CC=/path/to/compiler [options]
    make

For options and environment variables, refer to:

    ./configure --help

for options. The LLVM toolchain will require a path to purecap sysroot
(e.g. with Musl), for example:

    ./configure CC=/path/to/clang --sysroot=/path/to/purecap/sysroot

Note: the `configure` script only supports Morello LLVM and Morello GCC
toolchains and may not work out-of-the-box with other toolchains.

All sources are located under the `src` directory and are further grouped
into folders, one folder for each group of examples. The main entry point
for the build is the `makefile` in the project root.

To test, use:

    make test

To run tests with Morello IE, add `TEST_RUNNER`:

    make test TEST_RUNNER="/path/to/morelloie --"

## How to Run

All the example applications are intended to be used on a Morello system
with purecap Linux userspace. It can be Morello board, Morello FVP (for
x86 hosts), or Morello IE (for AArch64 hosts), see (this page)[tools] for
the download details.

## License

[BSD 3-Clause "New" or "Revised" License](LICENCE.txt) (SPDX-License-Identifier:
BSD-3-Clause).

[gnu]: https://developer.arm.com/downloads/-/arm-gnu-toolchain-for-morello-downloads
[llvm]: https://git.morello-project.org/morello/llvm-project-releases
[musl]: https://git.morello-project.org/morello/musl-libc
[tools]: https://developer.arm.com/Tools%20and%20Software/Morello%20Development%20Tools
