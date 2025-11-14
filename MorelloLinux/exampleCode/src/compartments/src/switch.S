/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "asm.h"

DEC(__prot_start)
DEC(__prot_end)
DEC(__brs_switch_end)

#define code    c27
#define data    c28
#define tmp     c29
#define target  c30
#define secret  c3

FUN(__brs_switch):
    sub     csp, csp, #(2*32)
    stp     c29, c30, [csp, #(0*32)]
    stp     c27, c28, [csp, #(1*32)]
    adr     c29, __brs_switch_end
    alignd  c29, c29, #4
    ldp     c30, c29, [c29]
    brs     c29, c29, c0
__prot_start:
    mov     c28, c29            //Move unsealed priv data to c28
    ldr     c27, [c28, #(4*16)] //Load private stack ptr
    mov     c29, csp            //Store caller's stack ptr to new stack ptr loc
    str     c29, [c28, #(4*16)] //
    mov     c29, c27            //Move private stack ptr to fp
    mov     csp, c29            //Move private stack ptr to csp
    sub     csp, csp, #16       //Store sealed priv data at start of private stack
    str     c0, [csp, #(0)]     //
    mov     c0, c28             //Set up unsealed priv data as target's first arg
    blr     c30
    ldr     c28, [csp, #(0)]    //Recover sealed priv data from private stack
    add     csp, csp, #16       //
    adr     c29, __brs_switch_end
    alignd  c29, c29, #4
    ldr     c29, [c29, #32]
    brs     c29, c29, c28       //Call __prot_end with brs to unseal priv data
__prot_end:
    mov     c28, c29
    ldr     c27, [c28, #(4*16)] //Load caller stack ptr
    mov     c29, csp            //Store private stack ptr to caller stack ptr loc
    str     c29, [c28, #(4*16)]
    mov     csp, c27            //Move caller stack ptr to csp
    ldp     c29, c30, [csp, #(0*32)]
    ldp     c27, c28, [csp, #(1*32)]
    add     csp, csp, #(2*32)
    ret
    udf     #0
.align 4
__brs_switch_end:
END(__brs_switch)
