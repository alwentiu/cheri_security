# Compartmentalisation examples

## Introduction

The group of examples in this folder focuses on compartmentalisation. This term
refers to fine-grained domain switching and isolation (sandboxing) techniques.
Compartmentalisation here is defined in terms of spatial memory isolation. It
relies on CHERI capabilities that provide the following tools: bounds, sealing,
and permissions.

A domain transition can happen when execution is transferred to a new address. When
some form of compartmentalisation is used, it is possible to formulate spatial memory
isolation properties (or guarantees) that can be used to derive a security model.

Two interacting domains (that is, domains in a caller-callee relationship) may
have various trust dispositions, and the most generic case is when they distrust
each other except for a finite set of explicitly defined capabilities. This means
that the full extent of memory accessible after branching into callee and before
returning from it back to the caller (i.e. while we are "in a compartment") can
be deterministically derived from only:

 - Input arguments and the result.
 - Ambient capabilities (`PCC`, `CSP`, `CTPIDR_EL0` etc).
 - Memory allocations (malloc, mmap, etc) that are supposed to be "private" or
   "local" for each domain.

This means we can introduce control over capabilities transferred across domain
boundaries using:

 - Register sanitisation (e.g. writing zero to all unused GPRs).
 - Restricting bounds and permissions of ambient capabilities (e.g. stack isolation).
 - Controlling memory allocations.

Morello allows implementing controlled domain transitions at any point of control
transfer which makes it very fine-grained. In other words, partitioning of an
application into security domains is very flexible. Implementations can choose to
automate partitioning or to allow developers define it as appropriate from security
and performance point of view.

## Key Components

Domain transition should be as simple as possible to minimise performance overhead
and possible attack surface. Ideally, it should be atomic, however this is not
feasible given the extent of things that ought to happen during the transition.
This suggests that we will need a *trampoline*, that is, code that implements
the logic of the transition, and optionally does checks or enforces rules. In an
ideal case trampoline code can be reduced to a single branch instruction however
it might be very difficult to enforce any security properties in this case. This
trampoline is one of the key components of any compartment implementation.

Depending on the implementation, trampoline code may have a glimpse into memory
of both the caller and the callee, that is why it might be possible to say that
while the execution point is inside the trampoline code, we are running in a
*privileged state*. It can be defined and implemented in various ways, but, in a
nutshell, it should suffice to say that code running in such state has access to
operations on compartments and (optionally) may have access to each compartment's
memory.

In general, the behaviour of the trampoline may need to be controlled or adjusted
given the actual requirements for the given inter-domain boundary. This may be done
via the *compartment parameters* that are associate with a particular *compartment
instance*. In addition, we will also need to hold capabilities for various memory
allocations that back up our domain partitioning (for example, independent stacks
for each compartment). Finally, since domain transition is associated with a branch
to some executable address, we will also need corresponding executable capability
to be attached to a compartment instance. All of the above comprises *compartment
descriptor* which is another key component we need to consider.

A compartment descriptor may be implemented in various ways, but it will always
hold at least two capabilities: the data capability and the code capability. They
are sometimes referred to as capability pair.

Each compartment instance can in theory be self-contained and independent, however
it might be more practical to have another component called *compartment manager*
that would be able to control lifecycle of compartments, for example dispense new
instances, deallocated and destroy them after use, and so on. Ideally, compartment
manager should not have access to the memory attached to each compartment it manages.

## Practical Considerations

One of the biggest challenges is keeping compartment instance protected and immutable.
Essentially, from the implementation point of view, an instance of a compartment must
come in the form of a sealed capability. Sealing will prevent using this capability
to access internals of the compartment instance. If this was possible, it would allow
various compartment attacks, such as:

 - Getting read or read-write access to compartment's private data.
 - Forging another compartment instance that can be used to gain access to the memory
   of an already existing compartment.
 - Altering behaviour of the compartment trampoline.

We will refer to such a sealed capability as *compartment handle*.

It should be impossible to explicitly unseal compartment handles. This means that
there must be no valid unsealed capability with the `UNSEAL` permission and with value
equal to the object type of the seal compartment handle. Unsealing should only be
possible via a dedicated branching operation.

The finite set of ambient capabilities must be explicitly defined (otherwise it won't
be possible to formally reason about security properties of compartments). This means
that we should choose environment in which our implementation is supposed to work.
For the purposes of the examples in this folder we will use PCuABI Linux user space
environment.

Morello provides several primitives that can be used to implement domain transition.

## Branch to Sealed Capability Pair

This implementation uses the "BRS (pair of capabilities)" instruction:

    BRS C29, <code>, <data>

### Overview

It requires a pair of capabilities with specific properties:

 - Both code and data capabilities are sealed with with same object type
   that is bigger than maximum fixed object type `CAP_MAX_FIXED_SEAL_TYPE`.
   On Morello, this is maximum object type `3` used for sentry-like sealed
   function pointers: `RB` (value 1), `LPB` (value 2), and `LB` (value 3).
 - Both capabilities must have the `BRANCH_SEALED_PAIR` permission.
 - The code one should have the `EXECUTE` permission and the data capability
   should not have it.

On success, this instruction puts unsealed version of the data capability in the `C29`
register and jumps to the address in the code capability unsealing it as well.

PCuABI Linux provides a way to obtain a sealing capability that can be used to seal
code and data capabilities with some object type, the `AT_CHERI_SEAL_CAP` auxv element
that is accessible via the new `getauxptr` function:

    __sealer = cheri_perms_and(getauxptr(AT_CHERI_SEAL_CAP), PERM_SEAL);

PCuABI Linux also provides a way to obtain RX and RW capabilities that would have the
`BRANCH_SEALED_PAIR` permission. This can be done using new `mmap` protection flag
`PROT_CAP_INVOKE`.

These two tools are enough to generate a pair of capabilities valid for the Branch to
Sealed Pair operation (BSP). This also means that anyone can construct either a code
or a data capability, pair it with another suitable capability and use this forged
compartment entry to get access to the unsealed versions of either data or code. For
example, we could branch to the address of our choice (i.e. call our function) and have
the unsealed data capability in the `C29` register.

Trampoline code may have some mitigation for this attack, however the only way to fully
eliminate this problem is to disable the use of the `PROT_CAP_INVOKE` protection flag.
For example, this can be done using a new system call. The implementation proposed here
leaves this issue out of scope.

As explained above, upon creating a new instance of the compartment, we should return
a sealed capability. This implementation returns a sentry, i.e. an RB-sealed executable
capability or, in other words, a function pointer. The address points to the start of
the trampoline code (LSB set for correct ISA to be used after branching), and therefore
this capability can be used as any normal function pointer which makes it quite handy
because we can use compartment handle in the same way as the wrapped function:

    // call without compartment:
    void *res = fun(ptr);
    ...
    // call with compartment:
    cmpt_fun_t *fun_in_cmpt = create_cmpt(fun, 4 /* pages */, &flags);
    void *res = fun_in_cmpt(ptr);

A new compartment instance is created for some function pointer that we refer to as a
*target* function. This is supposed to be a function that will be executed inside the
compartment. For brevity we only use functions that take one pointer as an argument and
return a pointer for the result. The support for an arbitrary amount of argument is
trivial using the new Morello ABI for variadic functions and is left as an exercise for
the reader.

The current implementation supports the following parameters:

 - Compartment stack size (number of pages allocated for the compartment's stack).
 - A few optional parameters that illustrate the idea of how we may alter the
   behaviour of the compartment.

Refer to the [hellobsp.c](hellobsp.c) example application for the most simple use case.

### Implementation Details

This implementation requires initialisation:

    init_cmpt_manager( /* seed */ 1000);

At this point the global `__sealer` and `__cid` capabilities are initialised from the
auxv provided by the PCuABI Linux. The former is used to seal code and data capabilities
while the latter demonstrates how the `CID` (compartment ID) register can be used in
practice.

We assign a unique ID to each compartment instance. The value of this ID modulo max object
type is used as the object type for the code and data capabilities when sealing them. Upon
switching to the compartment, the `CID_EL0` register is set using the compartment ID
generated for this compartment. This may, for example, be used in the target function to
understand which compartment instance we are running in (because the same function can
be used to create different compartment instances). It may also be used to implement
compartment identity check in the trampoline code (this is not currently implemented).
The purpose of doing this in the example is merely to illustrate that we can handle any
additional metadata.

Creating a compartment instance requires several system calls for memory management. The
idea is that a compartment is created once and then is reused multiple times. The domain
transition itself does not require any system calls and therefore should have relatively
low performance overhead.

    // Compartment parameters
    cmpt_flags_t flags = {
        .pcc_system_reg = false,
        .stack_store_local = false,
        .stack_mutable_load = true
    };
    // Create compartment instance
    cmpt_fun_t *fun_in_cmpt = create_cmpt(fun, 4 /* pages */, &flags);
    // Check the result
    if (fun_in_cmpt == NULL) {
        perror("create_cmpt");
        ...
    }

The current implementation allocates one page for trampoline code and read-only compartment
metadata. The corresponding capability will then become an RB-sealed RX code capability
(sentry) with adjusted bounds that is returned as a compartment handle. Trampoline code is
copied into the allocated page. This is necessary because we need the `BRANCH_SEALED_PAIR`
permission in the code capability, and this permission is not present in any PCC-derived
capability.

To be able to create a mapping with the correct protection and owning capability, we use
the `PROT_MAX` macro defined in the PCuABI spec. We originally request this page to have
RW memory protection and then we change it to RX using `mprotect` system call. We then
remove all the unnecessary permissions from the owning capability. Note that by doing so
we have lost any control over the memory mapping (since the `VMEM` permission is removed).
This means that we won't be able to deallocate this compartment, which is intentional to
make sure nothing may happen to the compartment memory. This also means that in the current
implementation each compartment instance will exist for the duration of the process.

We also allocate some memory for RW data used for swapping stacks and any other metadata
(e.g. compartment ID). The corresponding capability will become a BSP-sealed data capability
used in the `BRS` instruction. This BSP-sealed capability is stored in the read-only memory
nearby the trampoline code.

Finally, we allocate compartment stack: an RW capability with the required permissions and
address pointing to its limit.

The trampoline code performs the following steps:

 - Save callee-saved registers on the caller's stack.
 - Read data required to form the code-data capability pair.
 - Branch to sealed pair operation.
 - Read callee's stack pointer using unsealed data capability (along with any metadata).
 - Swap stacks: caller's stack pointer is saved into RW memory of the compartment.
   Only BSP-sealed data capability will point to this memory, so nobody will be able to
   access it.
 - Initialise any ambient capabilities (e.g. `CID_EL0`).
 - Sanitise all GP registers that are not used for arguments.
 - Call target function.
 - Sanitise all GP registers that are not used for the result.
 - Restore any ambient capabilities (e.g. `CID_EL0`).
 - Read data required to form the code-data capability pair.
 - Branch to sealed pair operation.
 - Read caller's stack pointer using unsealed data capability (along with any metadata).
 - Swap stacks: callee's stack pointer is saved into RW memory of the compartment.
 - Restore callee-saved registers from the caller's stack.
 - Return to caller.

Saving all the callee-saved registers is required because of the register sanitisation (the
trampoline itself needs just a few of temporary registers). The code of trampoline is written
in assembly to make sure not unseal capability is spilled to either caller's or callee's stack.

### Limitations

This implementation does not sanitise the stack upon return. Moreover, the stack pointer
is re-used as returned. In theory the target function may corrupt it in some way. This of
course can be mitigated, for example, by restoring original stack pointer upon returning
from the target function, but this is currently not implemented.

This implementation uses way more memory than it should. However, for the sake of keeping
implementation as simple as possible, no optimisations here are pursued.

We only support target functions with one argument and we are not implementing compartment
identity checks. It is also not possible to deallocate a compartment instance.

Current implementation is not thread-safe because of RW buffer for switch metadata what
would be shared across multiple threads.

### Examples

The [hackpwd.c](hackpwd.c) example shows how BSP compartmentalisation can be used to
mitigate a security vulnerability that rely on corrupting upper stack frames.

The [nestedcmpt.c](nestedcmpt.c) shows example of one compartment calling another.

## Other Morello Domain Switches

In addition to the "Branch to Sealed Capability Pair", Morello provides two more similar
compartment switch primitives: "Load Pair and Branch" and "Unseal, Load and Branch". They
are considered below. Note that functionally these examples are more simple compared to
the BSP example above (e.g. there is no register sanitisation or permission adjustment in
the target function pointer), however they still provide basic features like isolation of
stack.

Due to simplicity of creation of capability pairs for these two compartment switches, they
are susceptible to forgery of compartment handles, unlike the BSP switch.

### Load Pair and Branch

This switch uses the "Load Pair of capabilities and Branch (with Link)" instructions. They
operate on a capability that is sealed with a special (fixed) `LPB` type:

    LDPBR C29, [<Cn|CSP>]

During the execution of this instruction the base capability `Cn` is unsealed and two more
capabilities are loaded from its address. These two capabilities form a pair of data and
code capabilities. As soon as we have the capability pair, the rest is similar to the BSP
switch implementation. Refer to the [hellolpb.c](hellolpb.c) example for more details.

This LPB implementation stores caller's stack on itself and then puts LPB-sealed capability
that gives access to the return capability pair in one of the callee-saved registers. If
the underlying target function is PCS-compliant, this register will be preserved. However,
we do check if it's still sealed. This is useful because the `LDPBR` instruction does not
require the base capability to be sealed. Checking for the actual object type is unnecessary
because if it's not `LPB` the following Load Pair and Branch instruction would not unseal
capability and the following memory access will fail.

### Unseal, Load and Branch

This switch uses another memory indirect branch instruction that operates on a capability
sealed with a special object type `LB`:

    BR [C29, #<imm>]

This instruction unseals the capability and loads destination branch address at the given
offset. The unseal copy is stored in the `C29` register. The latter is the data part of the
capability pair and the destination capability loaded at the offset is the code part of it.
Refer to the [hellolb.c](hellolb.c) example for more details.

This ULB implementation also uses caller's stack to store the return capability pair and
relies on callee-saved register for the return domain transition to work.

## Protecting Private Data

Above we touched on the topic of spatial memory isolation but mostly focused on isolation
of stack. The heap memory allocations should be isolated (that is, well-bounded) by the
respective allocators. There are also PC-relative accesses however. One way of introducing
isolation along this axis is setting bounds for the function pointers from which PCC will
be derived upon successful branching. However, due to restrictions of the representability
of bounds and the fact that bounds of a capability cover a continuous region of memory (so,
we can't have a "selective" capability that covers two or more disjoined memory regions),
it might be difficult to achieve in practice. PCC bounds restrictions may also introduce
problems for code relocation.

Instead, we may try to invert the problem: what if we have an object (for example, a global
variable) that we want to be unusable for any code except for a finite set of "permitted"
functions. The implementation of a solution to this problem is described below.

Suppose we have the following simple use case:

    typedef struct {
        unsigned secret;
        // ...
    } priv_data_t;

    static priv_data_t *priv_data;

    static void malware()
    {
        printf("your secret is: %u\n", priv_data->secret);
    }

    static void init();

    int main(int argc, char *argv[])
    {
        // init private data:
        init();
        // use private data:
        const char *encrypted = encrypt_message(..., argv[1], ...);
        printf("encrypted message: %s\n", encrypted);
        // malicious access to private data:
        malware();
        return 0;
    }

Here we have a pointer to a global object that may contain some secret data that we should
be able to use but at the same time we don't want it to be accessed by any malicious code.
To prevent this, we will use capability sealing: after initialising our global object we
will protect it by sealing the only capability that points to it:

    static void protect()
    {
        priv_data = cheri_seal(priv_data, ...);
    }

    int main(int argc, char *argv[])
    {
        // init private data:
        init();
        // protect:
        protect();
        // ...
        return 0;
    }

The `malware` function will obviously fail after this change, but so will our good function
`encrypt_message`. We need a way to unseal `priv_data` inside the permitted function. Since
this is a data pointer (as opposed to a sentry or any other sealed function pointer), one way
to unseal it is to use an un-sealer capability. However, if we introduce such a capability,
we'd have to figure out a way to protect it as well, which brings us back to square one.

The Branch to Sealed Capability Pair instruction combines unsealing during branch operation
with unsealing of a data pointer:

    BRS C29, <code>, <data>

We can use it here similarly to how we used it for the BSP compartmentalisation above. To do
this we will modify our `protect` function to take a pointer to the function that is allowed
to access private data via `priv_data` and return a pointer to a function of the same type.
The returned pointer (capability) can be used in the same way as the original function.

We use `mmap` with the `PROT_CAP_INVOKE` protection flag to generate both the data and the code
capabilities to have the `PERM_CAP_INVOKE` permission set. We copy the switch code (defined in
[switch.S](src/switch.S)) into the code allocation and then append some data required for the
switch to work: the target function pointer and two BSP-sealed function pointers that we will
use for the `BRS` instruction. After this we set the bounds and reduce permissions to the
required minimum. We then RB-seal the code pointer with the offset `+1` for the C64 ISA mode
and return it.

We also need to modify the interface of our function `encrypt_message`: instead of using the
global variable implicitly it will now take additional argument of the same type. However,
all invocations of the `encrypt_message` function will use the global variable which refers
to a sealed capability. The switch code will use this extra argument from the respective
operand of the `BRS` instruction.

As a result, the global variable will contain sealed capability and accessing it from any code
that is not explicitly allowed to access it will not lead to subsequent access to the private
data. Because we replace one function pointer with another of the same type, we don't need to
make any significant changes to the existing code.

    int main(int argc, char *argv[])
    {
        // init private data:
        init();
        // protect:
        good_fun_t *fn = protect(encrypt_message);
        // use the wrapped function:
        const char *encrypted = fn(priv_data,... , argv[1], ...);
        printf("encrypted message: %s\n", encrypted);
        // ...
        return 0;
    }

It is possible, however, that the unsealed copy of the capability passed as argument to the
good function will remain spilled on stack. To mitigate this, we allocate a private stack
and use it for the duration of the good function execution. One more BRS instruction is used
after returning from the target function to restore the previous stack.

Finally, in the example code in [privdata.c](privdata.c) we have additional properties one of
which is the owning capability that was returned by `mmap` when we allocated memory for the
private data. It will also be protected and another permitted function can be used to access
it for deallocation. This is also not implemented yet but should also be easy enough to do.
