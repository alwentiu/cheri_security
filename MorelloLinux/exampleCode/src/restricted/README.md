# Switch to Restricted Mode

## Introduction

Morello supports two banks of registers for certain registers such as Default Data Capability `DDC`,
Capability Stack Pointer `CSP`, and Thread ID Register `TPIDR_EL0`. These banks are referred to as
Restricted and Executive. Access to these banks is controlled by the `EXECUTIVE` permission bit
in `PCC`. When this bit is set, we run in the Executive mode, otherwise we run in the Restricted
mode.

| Name             | Executive bank | Restricted bank |
| ------------------------ | ---------- | ----------- |
| Default Data Capability  | DDC_EL0    | RDDC_EL0    |
| Capability Stack Pointer | CSP_EL0    | RCSP_EL0    |
| Thread ID Register       | CTPIDR_EL0 | RCTPIDR_EL0 |

In the Restricted mode we can access one of the banks of these registers while in the Executive mode
both banks can be accessed.

| Accessing via... | Executive  | Restricted |
| ---------------- | ---------- | ---------- |
| DDC              | DDC_EL0    | RDDC_EL0   |
| RDDC_EL0         | RDDC_EL0   | (fault)    |
| CSP              | CSP_EL0    | RCSP_EL0   |
| RCSP_EL0         | RCSP_EL0   | (fault)    |
| CTPIDR_EL0       | TPIDR_EL0  | RTPIDR_EL0 |
| RCTPIDR_EL0      | RTPIDR_EL0 | (fault)    |

This can be used to implement compartmentalisation. The management code would run in the Executive
mode and would be able to set up stack pointer and thread ID register for each compartment while
the isolated code running in such compartment would only "perceive" the environment set up for it.

There are also restrictions imposed by the architecture on interworking between these two modes. To
switch to the Restricted mode we need two things:

 - A restricted function pointer (that is a sentry without the `EXECUTIVE` permission).
 - Branch via special instruction `B(L)RR` (Branch (with Link) to capability Register with
   possible switch to Restricted).

Branching via ordinary `BLR` instruction would clear the tag in the target `PCC` which would result
in a capability fault on instruction fetch right after branching. The same rule applies to returning
from a function running in the Executive mode via a restricted `CLR`. Instead a special instruction
`RETR` (Return with possible switch to Restricted) should be used. Both `BLRR` and `RETR` will fault
if executed in the Restricted mode.

Returning from the Restricted mode (or calling an executive function) only requires a sentry with the
`EXECUTIVE` permission. Ordinary `BLR` and `RET` instructions can be used.

Switching to the Restricted mode without proper setup may not work or result in a security problem,
that is why Morello requires use of the new instructions for this. On the other hand, being able to
request an operation run in the Executive mode from code that runs in the Restricted mode is useful
because this is how we can ask our runtime to switch to another compartment.

## Design Overview

In this example we use our own tiny runtime library to make things very simple. The execution starts
in the Executive mode when the `_start` function is called (see [src/start.S](src/start.S)). We must
run usual initialisation to setup all capabilities before we can proceed. Then we prepare switch to
Restricted mode and call the `main` function. This is where we enter the application code:

    <---------------- E ----------------> <------ R ------> <---- E ---->
    _start --> _init_compartments --> _start --> main --> _start --> exit

We execute `main` in something called a "root compartment". It is like any other compartment, but it
is set up automatically when the app starts.

Application can remain in the root compartment for the duration of the process or it can instantiate
more compartments and execute some code in them.

When switching to another compartment the following things happen:

 - Callee-saved registers are saved to the caller's stack.
 - A compartment descriptor is loaded from compartments private data.
 - Executive switch function is called (switching to Executive mode).
 - Caller's `TPIDR_EL0` and `CSP` are stored on the executive stack
   (not accessible from other compartments).
 - Callee's `TPIDR_EL0` and `CSP` capabilities are loaded and set up
   from the compartment descriptor.
 - Target function's arguments are loaded into registers.
 - All unused registers are sanitised.
 - Target function is called in the Restricted mode (unless the sentry
   has the `EXECUTIVE` permission, see below).
 - The executive link capability is RB-sealed and is saved to `CLR`.

After returning from the target function we do these operations in reverse. All stack allocations
are placed in private mappings and only capabilities that are explicitly provided by the caller
compartment to the callee compartment can be used to exchange data between compartments.

## Code Examples

### Restricting Global Functions

Let's consider the following example in the [restricted.c](restricted.c) file. The first part of it
shows one of the consequences of the rules described above. When using an indirect call to a global
function, we may accidentally switch to Executive mode. This depends on how the capability for this
global function was set up by the runtime initialisation (see `init` function). Such a function will
usually inherit executable capabilities from the `AT_CHERI_EXEC_RX_CAP` root capability provided by
the kernel, and this capability will have both `EXECUTIVE` and `SYSTEM` permissions by default.

This is why we will need to change the initialisation procedure and remove the `EXECUTIVE` and `SYSTEM`
permissions from all function pointers that are supposed to be used by the restricted code. However,
we may also want to keep executive copies of some of the global functions to use them from executive
code (e.g. for setting things up or managing compartments).

This example is rather simple, and executive copies of the standard library functions are not needed
as we can do everything via direct calls.

### Creating a Compartment

We create a new instance of a compartment by calling (see [include/rcmpt.h](include/rcmpt.h)):

    switch_t *cmpt_fun = create_compartment(target_fun, 2 /* pages */);

The returned capability is actually a sentry. It can be used in the same way as the wrapped target
function `target_fun`. This sentry points to code generated on the fly from the `_thunk` function
(see `_thunk` in [src/start.S](src/start.S)). This code will use near-relative load to access the
compartment descriptor with the following information:

    typedef struct {
        void *exec;     // executive pointer to switch trampoline (sentry)
        void *target;   // target function (sentry)
        void *tp;       // thread pointer (not used currently)
        void *sp;       // stack pointer
        void *cid;      // CID capability for the compartment
    } thunk_data_t;

The thunk code branches to the capability in the `exec` field (it points to the `_switch` function
and contains `EXECUTIVE` permission). The switch code runs in Executive mode and therefore can do
the necessary setup before branching to the restricted target function.

Note that at this point if the target function pointer is executive the switch to compartment will
not happen and the target function will remain running in Executive mode. In theory, the code that
creates compartment instances via the `create_compartment` function will not have access to any
executive function pointers. If it does, it can do the switch to the Executive mode just by doing
an indirect call via it, in which case the compartment isolation will be breached. This is another
reason why we should initialise any global function pointers with care. The check if the target
function is restricted is not currently implemented.

Invocation of a compartment is simple: instead of

    res = target_fun(arg0, arg1, arg2);

just use

    res = cmpt_fun(arg0, arg1, arg2);

Note that variadic targets are not supported in the current implementation.

### Nested Compartment Calls

In the same way as we call a compartment from the root compartment, we can also do nested calls
and invoke other compartments. Every switch to the next compartment will use Executive mode and
a separate stack frame on the executive stack to retain caller's data while running the callee.

## Private Data of the Compartment Manager

Note that compartment manager holds global object that contains data that can be used to escape
for a compartment. In this implementation it remains unprotected. To solve this problem, we can
use the BRS-sealed capability pair (see the `privdata` example in the `compartments` folder).
This suggests that in practice the switch-to-restricted compartments might co-exists with the
branch-to-sealed-pair compartments. However, we could also use private memory mapping to store
this global object and keep the capability pointing to it on the executive stack that would be
inaccessible from any compartments (including the root compartment).
