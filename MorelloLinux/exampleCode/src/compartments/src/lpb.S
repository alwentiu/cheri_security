/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "asm.h"

FUN(cmpt_call):
    stp     c29, c30, [csp, #-32]!      // c0 is LPB-sealed pointer to cap pair
    ldpblr  c29, [c0]                   // call cmpt_switch (arg)
    ldp     c29, c30, [csp], #32
    ret
END(cmpt_call)

FUN(cmpt_switch):
    sub     csp, csp, #(2*32)
    stp     c28, c30, [csp, #(1*32)]
    adr     c9, 2f                      // make return cap pair
    add     c9, c9, #1
    seal    c9, c9, rb
    mov     c28, csp
    stp     c28, c9, [csp, #(0*32)]     // store return cap pair
    scbnds  c28, c28, #32
    clrperm c28, c28, wx
    seal    c28, c28, lpb               // LPB-seal pointer to the return cap pair
    ldp     c29, c30, [c29]             // stack, target from unsealed c29
    mov     csp, c29                    // swap stacks
    mov     c0, c1                      // put argument in the right place
    blr     c30                         // call target function
    chksld  c28                         // we use callee-saved register, but we check if it's valid
    b.vs    1f                          // continue if sealed
    mov     w0, #9                      // SIGKILL
    bl      raise                       // abort
1:  ldpbr   c29, [c28]                  // compartment return
2:  mov     csp, c29
    ldp     c28, c30, [csp, #(1*32)]
    add     csp, csp, #(2*32)
    ret
END(cmpt_switch)
