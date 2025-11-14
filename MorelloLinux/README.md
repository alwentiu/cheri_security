# Linux Morello

Compilation for linux is more complex, and you have to run these commands beforehand: 

source /morello/env/morello-sdk
./configure CC=clang --sysroot=$MUSL_HOME

The four attacks discussed are in their respective folders.
The example compartmentalisation code is in the exampleCode/compartments folder

These four files are the most relevant:
privdata.c  privdataDlopen.c  privdataException.c  privdataMalloc.c  privdataScanStatic.c 
As they contain the compartmentalised global and the malicious code in  the malicious function.


