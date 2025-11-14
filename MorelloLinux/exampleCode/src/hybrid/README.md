# Hybrid Examples

## Introduction

This folder contains some examples that show a different way of compiling source
code for the Morello targets. This way is commonly referred to as "hybrid". CHERI
capabilities implement source-level pointers (or references) on the machine level.
An integer pointer (address-only pointer) is another implementation of references.
When compiling for the Morello target, one of these two implementations should be
default while another would be alternative. The default implementation does not
require any source changes while alternative implementation must be annotated on
the source level.

| Mode    | Default `T&` and `T*` | Alternative `T&` and `T*` | ISA |
| ------- | --------------------- | ------------------------- | --- |
| Hybrid  | Address               | Capability                | A64 |
| Purecap | Capability            | Address                   | C64 |

When you compile source code for the Morello target, you select which compilation
mode is used (by specifying corresponding compilation flags) and this affects how
the `T&` and `T*` language-level entities are interpreted and converted into the
machine-level implementation. In a nutshell, in the Hybrid mode all pointers are
addresses unless annotated as capabilities while in the Purecap mode all pointers
are capabilities:

    // Hybrid code:
    char *ptr = "string"; // address (sizeof(char *) is 8)
    char *__capability cap = "string"; // capability (sizeof(char *__capability) is 16)

When compiling in Hybrid mode, the machine code is generated for the A64 ISA mode
and in Purecap compilation mode the intended ISA mode is C64. However, ISA mode
and the interpretation of language-level pointers are orthogonal. Moreover, the
actual ISA mode depends on the CPU runtime state (the `PSTATE.C64` bit) while the
binary code generated once would stay the same and would be just interpreted in
a different way. For example, the following instruction encoding `0xc2400020` will
be interpreted in a different way depending on the `PSTATE.C64` bit:

    ldr c0, [x1, #0]    //  (PSTATE.C64 == 0)
    ldr c0, [c1, #0]    //  (PSTATE.C64 == 1)

Both interpretations load a capability into the `C0` register using address from
either `X1` register or `C1` register. In both cases memory access will be done
via a capability. In the former case this capability will be derived from the
default data capability `DDC` using the address stored in the `X1` register. In
the latter case the capability in the `C1` register will be explicitly used.

Using hybrid compilation objects one can implement a particular memory access via
an explicitly declared capability while the rest of the memory accesses may remain
carried out via the `DDC` capability. Hybrid compilation mode allows for gradual
introduction of capabilities into your code.

In Purecap mode all pointers are treated as capabilities by default. The scheme
discussed above allows for an alternative form of a pointer in Purecap that would
be just an integer address (as in the default pointer interpretation in Hybrid),
however it is not yet implemented. It is not yet possible to express such a pointer
on a C language level, although you can use integer addresses on assembly level.

## Hybrid Runtime

Although Hybrid mode allows you to declare a pointer as a capability and then use
it to control memory accesses, in practice you will most likely need your runtime
(e.g. C library or dynamic linker) to be capability-aware. The following list may
be used as a starting point for producing a so-called hybrid C library for CHERI
or Morello. Note that this is not an exhaustive list and that some of these features
may not be implemented at all.

* Global object initialisation and relocations (e.g. processing `R_MORELLO_RELATIVE`).
* The `getauxptr` function to access capabilities in `auxv`.
* The `memcpy` function and friends should use capability loads and stores to
  preserve capability tags if present.
* Memory allocation functions `malloc` and `mmap` and friends should support
  returning bounded capabilities instead of raw addresses. A similar change
  is required for `free` and `munmap` as well as for `realloc` and `mremap`.
* Functions that return one of the input pointers should change the declaration
  of the corresponding input argument to allow capability instead of raw address.
  Note, that this will require changes in all places where such function is used
  because we'll need to explicitly cast raw addresses to capabilities. In some
  cases a hybrid C library should may have dual interfaces for some functions.
  A capability-aware implementation should carry an input capability and return
  a new capability derived from the former one.

A compiler that supports Hybrid mode of compilation should deduce bounds and permissions
for explicitly declared capabilities pointing to objects with automatic storage. The
same should apply to `alloca` results.
