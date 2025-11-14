/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "asm.h"
#include "perms.h"

/** void *__examine_stack(size_t depth, size_t addr_to_look_for)
 * Examines depth bytes below current CSP for a capability which
 * is unsealed, has a set tag, has the BSP permission and with
 * address addr_to_look_for.
 * Returns NULL if no such capability is found.
 */

#define depth       c0
#define addr        c1
#define stack_ptr   c2
#define stack_cap   c3

FUN(__examine_stack):
    mov     c2, csp
    b       __loop
__it:
    sub     c0, c0, #16             //Reduce depth by capability size
    cmp     x0, #0                  //If depth != 0
    b.ne    __loop                  //Do loop body
    mov     w3, #0                  //stack_cap = NULL since the desired cap wasn't found
    b       __end                   //Finish
__loop:
    ldr     c3, [c2], #-16          //Load stack_cap from stack
    chktgd  c3                      //If stack_cap's tag is 0
    b.cc    __it                    //Continue
    chksld  c3                      //If stack_cap is sealed
    b.vs    __it                    //Continue
    gcperm  x4, c3          		//If stack_cap doesn't have the BSP permission
    and     x4, x4, PERM_CAP_INVOKE //
    cmp     x4, PERM_CAP_INVOKE     //
    b.ne    __it                    //Continue
    gcvalue x5, c3                  //If stack_cap's address != addr
    cmp     x5, x1                  //
    b.ne    __it                    //Continue
__end:
    mov     c0, c3                  //Return stack_cap
    ret
END(__examine_stack)
