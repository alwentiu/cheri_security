# Testing capability leaks

This is a clone of morello-sdk example showing various compartmentalisation primitives. The only addition I made is in the `findcaps` subproject, that shows how to obtain capabilities to various mapped regions of the process, through `dl_iterate_phdr()` library call. This is a function to query all the program headers of the binary that are loaded to memory, so these contain the segments for the main binary, libc, ld (and other libraries if there are any).
This library uses a callback function that needs to be provided as an argument to `dl_iterate_phdr`. 

The new stuff is in `src/findcaps` and `src/compartmentalisation`

# Testing the code

Currently the configuration is set up to run on the morello linux machine, which assumes that the morello sdk is installed in `/morello`. 
To compile, simply run `make`. The compiled files are in `build/bin`.

# To run the code (on plain install)
source /morello/env/morello-sdk
./configure CC=clang --sysroot=$MUSL_HOME
make
