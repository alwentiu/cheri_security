/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <string.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <errno.h>

#include "cmpt.h"
#include "morello.h"

#ifndef PROT_CAP_INVOKE
#define PROT_CAP_INVOKE 0x2000 // Purecap libc fix-ups
#endif

int getpagesize(void);

static void *_exec_allocate();
static void *_data_allocate();
static void *_stack_allocate(unsigned pages);
static void *_bsp_seal_cap(const void *cap, const void *cid);

static void *__sealer = NULL;
static void *__cid = NULL;

typedef struct {
    void *stack;    // compartment's stack pointer
    void *cid;      // placeholder for caller CID
} cmpt_data_t;

typedef struct {
    void *cid;      // compartment id with CID permission (sealed)
    void *target;   // RB-sealed target function pointer (sentry)
    void *entry;    // sealed compartment entry (BSP-sealed)
    cmpt_data_t *data; // rw pointer to store stack pointers (BSP-sealed)
    void *exit;     // sealed compartment exit (BSP-sealed)
} cmpt_impl_t;

void init_cmpt_manager(size_t seed)
{
    __sealer = cheri_perms_and(getauxptr(AT_CHERI_SEAL_CAP), PERM_SEAL);
    __cid = cheri_address_set(getauxptr(AT_CHERI_CID_CAP), seed);
}

/**
 * We need private labels to automatically calculate
 * addresses and sizes for runtime code relocation.
 */
#define LABEL(name) ".global " name " \n.hidden " name "\n.size " name ",16\n.type " name ",%function\n" name ":\n"

#if defined(__GNUC__) && !defined(__clang__)
__attribute__ ((used))
#else
__attribute__ ((naked,used))
#endif
static void _trampoline()
{ __asm__ volatile(
"   sub     csp, csp, #(6*32)\n"
"   stp     c29, c30, [csp, #(0*32)]\n"
"   stp     c27, c28, [csp, #(1*32)]\n"
"   stp     c25, c26, [csp, #(2*32)]\n"
"   stp     c23, c24, [csp, #(3*32)]\n"
"   stp     c21, c22, [csp, #(4*32)]\n"
"   stp     c19, c20, [csp, #(5*32)]\n"
"   adr     c27, _trampoline_end\n"
"   alignu  c27, c27, #4\n"
"   ldp     c26, c30, [c27, #0]\n"  // cid, target (sentry)
"   ldp     c27, c28, [c27, #32]\n" // entry (BSP-sealed), data (BSP-sealed)
"   brs     c29, c27, c28\n"        // switch to compartment
LABEL("_cmpt_start")
"   mrs     c28, CID_EL0\n"
"   str     c28, [c29, #16]\n"      // swap cid
"   msr     CID_EL0, c26\n"
"   mov     c28, csp\n"
"   ldr     c27, [c29]\n"           // swap callee's and caller's stacks
"   str     c28, [c29]\n"           //
"   mov     c29, c27\n"             //
"   mov     csp, c29\n"             // enable callee's stack and fp
".irp    rn,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28\n"
"   mov    w\\rn, #0\n"             // except c0 (arg) and c30 (target)
".endr\n"
"   blr     c30\n"                  // call target function
".irp    rn,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18\n"
"   mov    w\\rn, #0\n"             // except c0 (res) and callee-saved registers (overwritten later)
".endr\n"
"   adr     c27, _trampoline_end\n"
"   alignu  c27, c27, #4\n"
"   ldp     c28, c27, [c27, #48]\n" // data (BSP-sealed), exit (BSP-sealed)
"   brs     c29, c27, c28\n"        // return from compartment
LABEL("_cmpt_end")
"   ldr     c28, [c29, #16]\n"      // swap cid
"   msr     CID_EL0, c28\n"
"   mov     c28, csp\n"
"   ldr     c27, [c29]\n"           // swap callee's and caller's stacks
"   str     c28, [c29]\n"           //
"   mov     csp, c27\n"             // restore caller's stack
"   ldp     c19, c20, [csp, #(5*32)]\n"
"   ldp     c21, c22, [csp, #(4*32)]\n"
"   ldp     c23, c24, [csp, #(3*32)]\n"
"   ldp     c25, c26, [csp, #(2*32)]\n"
"   ldp     c27, c28, [csp, #(1*32)]\n"
"   ldp     c29, c30, [csp, #(0*32)]\n"
"   add     csp, csp, #(6*32)\n"
"   ret     c30\n"
"   udf     #0\n"
LABEL("_trampoline_end")
);}

#define GET_NEAR_ADDR(sym) ({ void *p; __asm__ volatile("adr %0, " sym : "=C"(p)); p; })

#define RW_PERMS (PERM_GLOBAL | READ_CAP_PERMS | WRITE_CAP_PERMS)
#define RX_PERMS (PERM_GLOBAL | READ_CAP_PERMS | EXEC_CAP_PERMS)
#define RWI_PERMS (RW_PERMS | PERM_CAP_INVOKE)
#define RXI_PERMS (RX_PERMS | PERM_CAP_INVOKE)

cmpt_fun_t *create_cmpt(cmpt_fun_t *target, unsigned stack_pages, const cmpt_flags_t *flags)
{
    /**
     * Check that global capabilities have been initialised.
     */
    if (!cheri_is_valid(__sealer)
#if !defined(__GLIBC__) // Morello Glibc currently doesn't return a capability for AT_CHERI_CID_CAP
        || !cheri_is_valid(__cid)
#endif
        ) {
        errno = EFAULT; // not initialised
        return NULL;
    }

    /**
     * Relocate trampoline code.
     */
    const char *t_start = GET_NEAR_ADDR("_trampoline");
    const char *t_end= GET_NEAR_ADDR("_trampoline_end");
    const char *c_start = GET_NEAR_ADDR("_cmpt_start");
    const char *c_end = GET_NEAR_ADDR("_cmpt_end");
    size_t code_sz = t_end - t_start;
    size_t start_offset = c_start - t_start;
    size_t end_offset = c_end - t_start;
    void *code = _exec_allocate();
    if (code == NULL) {
        return NULL;
    }
    memcpy(code, cheri_align_down(t_start, 4), code_sz);

    /**
     * Store compartment switch data.
     */
    code_sz = cheri_align_up(code_sz, sizeof(void *));
    cmpt_impl_t *impl = (cmpt_impl_t *)cheri_perms_and(cheri_bounds_set_exact(code + code_sz, sizeof(cmpt_impl_t)), RW_PERMS);
    // Note: just seal it as the object type here is irrelevant
    // (RB isn't really an appropriate type here because we are not
    // going to execute this capability).
    impl->cid = cheri_sentry_create(__cid++); // every compartment gets its unique id
    impl->target = target;
    if (flags && !flags->pcc_system_reg) {
        // note: this requires resealing
        impl->target = reseal_and_remove_perms(impl->target, PERM_SYS_REG);
    }
    if (!cheri_is_sealed(impl->target)) {
        impl->target = cheri_sentry_create(impl->target);
    }
    void *data = _data_allocate();
    if (data == NULL) {
        munmap(code, cheri_length_get(code));
        return NULL;
    }
    impl->data = (cmpt_data_t *)cheri_perms_and(cheri_bounds_set_exact(data, sizeof(cmpt_data_t)), RWI_PERMS);
    void *stack = _stack_allocate(stack_pages);
    if (stack == NULL) {
        munmap(code, cheri_length_get(code));
        munmap(data, cheri_length_get(data));
        return NULL;
    }
    impl->data->stack = stack; // compartment's (callee's) stack
    impl->data->cid = NULL; // placeholder for caller CID
    if (flags && !flags->stack_store_local) {
        impl->data->stack = cheri_perms_clear(impl->data->stack, PERM_STORE_LOCAL_CAP);
    }
    if (flags && !flags->stack_mutable_load) {
        impl->data->stack = cheri_perms_clear(impl->data->stack, PERM_MUTABLE_LOAD);
    }
    impl->data = _bsp_seal_cap(impl->data, impl->cid);
    impl->entry = _bsp_seal_cap(cheri_perms_and(code + start_offset + 1, RXI_PERMS), impl->cid);
    impl->exit = _bsp_seal_cap(cheri_perms_and(code + end_offset + 1, RXI_PERMS), impl->cid);

    /**
     * Fixup memory protection: RW -> RX.
     */
    mprotect(code, code_sz, PROT_READ | PROT_EXEC);
    __builtin___clear_cache(code, code + code_sz);

    /**
     * Correct bounds, permissions and LSB for the result.
     * Finally, seal it and return a sentry.
     */
    code = cheri_bounds_set_exact(code, code_sz + sizeof(cmpt_impl_t) + sizeof(void *));
    code = cheri_perms_and(code + 1, RX_PERMS);
    return cheri_sentry_create(code);
}

void *reseal_and_remove_perms(void *sentry, size_t perms)
{
    // note: we need unsealed key capability for build to work
    // therefore we may have to use PCC hoping that sentry is within PCC's bounds
    void *key;
    if (cheri_is_sealed(sentry)) {
        key = cheri_pcc_get();
    } else {
        key = sentry;
    }
    return cheri_sentry_create(
        cheri_cap_build(key, (unsigned __intcap)cheri_perms_clear(sentry, perms)));
}

/**
 * BSP-seals capability using compartment ID as object type value.
 */
static void *_bsp_seal_cap(const void *cap, const void *cid)
{
    if (cheri_check_perms(cap, PERM_CAP_INVOKE)
#if !defined(__GLIBC__) // Morello Glibc currently doesn't return a capability for AT_CHERI_CID_CAP
        && cheri_tag_get(cid) && cheri_check_perms(cid, PERM_CMPT_ID)
#endif
    ) {
        size_t otype = cheri_address_get(cid);
        if (otype < 4ul) { // avoid reserved object types RB, LPB, and LB
            otype = 4ul;
        }
        return (void *)cheri_seal(cap, cheri_address_set(__sealer, otype & 0x7fff));
    } else {
        return (void *)cheri_tag_clear(cap);
    }
}

/**
 * Allocates memory enough to save trampoline code and data
 * and returns capability suitable for BSP-sealing as code.
 * May return NULL if memory cannot be allocated.
 */
static void *_exec_allocate()
{
    size_t pgsz = getpagesize();
    int prot = PROT_READ | PROT_WRITE | PROT_CAP_INVOKE | PROT_MAX(PROT_READ | PROT_WRITE | PROT_EXEC);
    void *mem = mmap(NULL, pgsz, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    // Note: setting bounds is going to be redundant here
    // once kernel returns bounded capability.
    return cheri_bounds_set(mem, pgsz);
}

/**
 * Allocates memory enough to save writeable data (see cmpt_data_t)
 * and returns capability suitable for BSP-sealing as data.
 * May return NULL if memory cannot be allocated.
 */
static void *_data_allocate()
{
    size_t pgsz = getpagesize();
    int prot = PROT_READ | PROT_WRITE | PROT_CAP_INVOKE;
    void *mem = mmap(NULL, pgsz, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    // Note: setting bounds is going to be redundant here
    // once kernel returns bounded capability.
    return cheri_bounds_set(mem, pgsz);
}

/**
 * Allocates given number of pages for compartments stack.
 * Returns valid unsealed capability with its address pointing
 * to its limit.
 */
static void *_stack_allocate(unsigned pages)
{
    size_t pgsz = getpagesize();
    size_t sz = pgsz * pages;
    void *mem = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    // Note: setting bounds and permissions is going to be
    // redundant here once kernel returns bounded capability.
    return cheri_perms_and(cheri_bounds_set(mem, sz), RW_PERMS) + sz;
}
