/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "asm.h"

DEC(cmpt_entry)
DEC(cmpt_return)

FUN(cmpt_call):
    sub     csp, csp, #(3*32)           // c0 is LB-sealed pointer to cap pair
    stp     c29, c30, [csp, #(1*32)]
    stp     c27, c28, [csp, #(2*32)]
    mov     c29, c0                     // must use c29
    br      [c29, #0]                   // branch to cmpt_entry
cmpt_entry:
    ldr     c30, [c29, #48]             // load target sentry from unsealed c29
    ldp     c27, c29, [c29, #16]        // load exit sentry and callee's stack from unsealed c29
    mov     c28, csp
    stp     c27, c28, [csp]             // store return cap pair
    scbnds  c28, c28, #32
    clrperm c28, c28, wx
    seal    c28, c28, lb                // LB-seal pointer to the return cap pair
    mov     csp, c29                    // swap stacks
    mov     c0, c1                      // put argument in the right place
    blr     c30                         // call target function
    chksld  c28                         // we use callee-saved register, but we check if it's valid
    b.vs    1f                          // continue if sealed
    mov     w0, #9                      // SIGKILL
    bl      raise                       // abort
1:  mov     c29, c28                    // must use c29
    br      [c29, #0]                   // compartment return
cmpt_return:
    ldr     c29, [c29, #16]             // restore stack
    mov     csp, c29
    ldp     c27, c28, [csp, #(2*32)]
    ldp     c29, c30, [csp, #(1*32)]
    add     csp, csp, #(3*32)
    ret
END(cmpt_call)
