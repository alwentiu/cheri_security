# Freestanding examples

These examples do not use any standard C library, instead a handful
of functions are implemented from scratch. These implementations are
not tuned for performance, instead they are supposed to demonstrate
how explicit use of Morello features and tools can be used to write
safe purecap code.

The applications in this group are supposed to be built for purecap
and rely on [PCuABI user-kernel interface][pcuabi].

## Applications

The `listauxv` binary will run without any parameters and prints the
contents of auxiliary vector. On a PCuABI system, this vector will
include special capabilities along with the standard integer values.
The capabilities are printed with information on bounds, permissions,
and object type.

The `selftest` binary will run without any arguments and will execute
a number of unit tests that check correctness of the utility functions.

The `hackfmt` binary accepts one optional argument that is then used
as format string for the `printf` function:

    int main(int argc, char *argv[], char *envp[])
    {
        int n = printf(argv[1]);
        printf("\nprinted %d characters\n", n);
        return 0;
    }

This illustrates an unsafe use of this function when user input affects
the format string. This is one of the well-known security issues for
which the mitigation is to always use static strings for formatting.
However, the `printf` function is implemented in a way that prevents
any tempering with memory thus eliminating the vulnerability.

Try it yourself:

    ./build/bin/hackfmt %p%p%p

If you run this program without any arguments, it will still work as
expected: the passed `NULL` pointer will be treated as an empty string.
The details of how this works are described below.

## Library

These examples don't use any standard C library, therefore some basic
utilities are implemented here. This includes the following:

- Static initialisation code: to instantiate capabilities at runtime
  (see [Morello EFL spec][elf] for details). This includes functions
  `init_morello_relative` and deprecated `init_cap_relocs`.
- Syscall wrappers for most necessary system calls like `EXIT_GROUP`
  and `WRITE`.
- Standard functions like `(s)printf`, `strlen`, `strcpy`, `strcmp`,
  `memset`, and capability-aware `memcpy`.

The functions mentioned above are using information about capability
bounds to avoid inappropriate memory accesses that would result in a
capability fault leading to a segfault. Explicit validity and bounds
checks allow to mitigate spatial memory vulnerabilities. Refer to the
comments in sources for more details.

[pcuabi]: https://git.morello-project.org/morello/kernel/linux/-/wikis/
[elf]: https://github.com/ARM-software/abi-aa/blob/main/aaelf64-morello/aaelf64-morello.rst
