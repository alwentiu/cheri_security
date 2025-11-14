/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "asm.h"
#include "perms.h"

#define argc    c0
#define argv    c1
#define envp    c2
#define auxv    c3
#define temp    c16
#define tmpx    x16

FUN(_start):                                    // MODE
    msr     rddc_el0, czr                       // E        fault if run in restricted
    mov     c19, argc                           // E
    mov     c20, argv                           // E
    mov     c21, envp                           // E
    mov     c22, auxv                           // E
    bl      _init_compartments                  // E        returns root cmpt's descriptor
    ldp     c16, c17, [c0, #32]                 // E        see `thunk_data_t` in `cman.c`
    msr     rctpidr_el0, c16                    // E        thread pointer
    msr     rcsp_el0, c17                       // E        stack pointer
    ldr     c16, [c0, #64]                      // E
    msr     cid_el0, c16                        // E        CID register
    adr     temp, 1f                            // E
    mov     x9, #(PERM_EXECUTIVE | PERM_SYS_REG)// E
    clrperm temp, temp, x9                      // E
    add     temp, temp, #1                      // E        C64
    seal    temp, temp, rb                      // E        sentry is required
    mov     argc, c19                           // E
    mov     argv, c20                           // E
    mov     envp, c21                           // E
    mov     auxv, c22                           // E
    blrr    temp                                // E        switch to restricted mode
    b       exit                                // E        we've left root cmpt: exit
1:  str     clr, [csp, #-16]!                   // R        restricted stack
    mov     c29, csp                            // R
.irp n,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28
    mov     w\n, #0                             // R        except c0-3 (args) and c29 (fp)
.endr
    bl      main                                // R
    ldr     clr, [csp], #16                     // R        restricted stack
    ret                                         // R        return from restricted mode
END(_start)

SYM(_thunk_data)
SYM(_thunk_end)

/**
 * Runs in restricted mode.
 */
.align 4                                        // make sure we have correct alignment
FUN(_thunk):
    sub     csp, csp, #(6*32)                   // restricted stack
    stp     c29, c30, [csp, #(0*32)]
    stp     c27, c28, [csp, #(1*32)]
    stp     c25, c26, [csp, #(2*32)]
    stp     c23, c24, [csp, #(3*32)]
    stp     c21, c22, [csp, #(4*32)]
    stp     c19, c20, [csp, #(5*32)]
    adr     c29, _thunk_data                    // if this function is invoked directly
    clrperm c29, c29, x
    alignd  c29, c29, #2                        // ...it will fail with segfault (c30 will be null)
    ldr     c30, [c29], #16                     // load executive switch function
    blr     c30                                 // call executive trampoline (_switch)
    ldp     c19, c20, [csp, #(5*32)]
    ldp     c21, c22, [csp, #(4*32)]
    ldp     c23, c24, [csp, #(3*32)]
    ldp     c25, c26, [csp, #(2*32)]
    ldp     c27, c28, [csp, #(1*32)]
    ldp     c29, c30, [csp, #(0*32)]
    add     csp, csp, #(6*32)
    ret

.align 4                                        // make sure we have correct alignment
_thunk_data:
    .zero (5*16)                                // this should match sizeof(thunk_data_t)
_thunk_end:
END(_thunk)

.macro get_arg n
    cmp     x16, #(16*(\n))
    b.lt    1f
    ldr     c\n, [c9, #(16*(\n - 1))]
    b       2f
1:  mov     w\n, #0                             // sanitise reg if not used
2:
.endm

SYM(_switch_end)

/**
 * Executive switch, runs in executive mode.
 * A pointer to the cmpt descriptor is passed via c29.
 * All original target's arguments remain in C0 and C9.
 */
FUN(_switch):
    msr     rddc_el0, czr                       // fault if run in restricted: protection against calling in restricted
    ldp     c26, c27, [c29]                     // load `target` and `thread pointer`
    ldp     c29, c28, [c29, #32]                // load `stack pointer` and `cid`
    mrs     c16, rctpidr_el0                    // save caller's thread pointer
    mrs     c17, rcsp_el0                       // save caller's stack pointer
    stp     c16, c17, [csp, #-32]!              // (executive stack)
    mrs     c16, cid_el0                        // save caller's CID
    stp     c30, c16, [csp, #-32]!              // (executive stack)
    msr     rctpidr_el0, c27                    // set callee's thread pointer
    msr     rcsp_el0, c29                       // set callee's stack pointer
    msr     cid_el0, c28                        // set callee's CID
    gclen   x16, c9
    get_arg 8                                   // varargs...
    get_arg 7                                   // todo: support more than 8 args
    get_arg 6
    get_arg 5
    get_arg 4
    get_arg 3
    get_arg 2
    get_arg 1
    mov     c30, c26                            // sanitise registers
.irp n,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28
    mov     w\n, #0                             // except c0-c8 (args), c29 (fp), and c30 (target)
.endr
    // todo: check that target is not executive, abort if this is the case
    blrr    c30                                 // call target (switch to restricted)
    ldp     c30, c16, [csp], #32
    msr     cid_el0, c16                        // restore caller's CID
    ldp     c16, c17, [csp], #32
    msr     rctpidr_el0, c16                    // restore caller's thread pointer
    msr     rcsp_el0, c17                       // restore caller's stack pointer
.irp n,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18
    mov     w\n, #0                             // except c0 (res) and callee-saved (overwritten later)
.endr
    retr    c30                                 // return to restricted (fail if executed in restricted)
_switch_end:
END(_switch)
