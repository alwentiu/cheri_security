/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "auxv.h"
#include "morello.h"

#include "rcmpt.h"

#define MAP_PRIVATE     0x02
#define MAP_ANONYMOUS   0x20

#define PROT_READ   1
#define PROT_WRITE  2
#define PROT_EXEC   4
#define PROT_MAX(p) ((p) << 16)

// Note: this is obviously sensitive data, but it can
// be protected using BRS-sealed pair of capabilities
// (see the `src/compartments/privdata.c` example).
static struct {
    void *_thunk;   // ro capability for thunk source (no exec)
    void *_switch;  // sentry with executable permission
    void *_cid;     // unsealed CID capability from auxv
    size_t pgsz;    // page size
    size_t thunk_size;
    size_t thunk_data_offset;
} ctx;

static void *get_cmpt_stack(size_t size)
{
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    void *stack = mmap(NULL, size, prot, flags);
    stack = cheri_align_down(stack + size, sizeof(void *));
    return cheri_perms_and(stack, PERM_GLOBAL | READ_CAP_PERMS | WRITE_CAP_PERMS);
}

// An instance of this object will be loaded
// by the thunk code into 4 registers that will
// be used by the switch code.
typedef struct {
    void *exec;     // executive pointer to switch trampoline (sentry)
    void *target;   // target function (sentry)
    void *tp;       // thread pointer (not used currently)
    void *sp;       // stack pointer
    void *cid;      // CID capability for the compartment
} thunk_data_t;

// Defined in src/start.S
extern void _thunk();
extern char _thunk_data;
extern char _thunk_end;
extern void _switch();
extern char _switch_end;

// Called from `_start` defined in `src/start.S`.
// This function must be called in executive mode.
__attribute__((used))
void *_init_compartments(int argc, char *argv[], char *envp[], auxv_t *auxv)
{
    // This function must not be called in restricted mode:
    if (is_in_restricted()) {
        return NULL;
    }

    // Init global objects and functions:
    void *pcc = cheri_pcc_get();
    // Note: if `false` is used below, the app will segfault
    // after returning from the indirectly called global function.
    init(auxv, /* restricted */ true);

    // Setup compartments:
    ctx.pgsz = getpagesize();
    ctx._cid = getauxptr(AT_CHERI_CID_CAP);

    // Locate original thunk to copy it over for each compartment:
    ctx._thunk = cheri_address_set(pcc, cheri_align_down(cheri_address_get(_thunk), 4));
    ctx._thunk = cheri_perms_and(ctx._thunk, PERM_GLOBAL | PERM_LOAD);
    ctx.thunk_size = cheri_align_up(cheri_address_get(&_thunk_end) - cheri_address_get(_thunk), 4);
    ctx.thunk_data_offset = cheri_align_up(cheri_address_get(&_thunk_data) - cheri_address_get(_thunk), sizeof(void *));
    ctx._thunk = cheri_bounds_set_exact(ctx._thunk, ctx.thunk_size);

    // We'll use it when setting up each compartment:
    ctx._switch = cheri_address_set(pcc, cheri_align_down(cheri_address_get(_switch), 4));
    ctx._switch = cheri_perms_and(ctx._switch, PERM_GLOBAL | READ_CAP_PERMS | EXEC_CAP_PERMS);
    size_t _switch_size = cheri_align_up(cheri_address_get(&_switch_end) - cheri_address_get(_switch), 4);
    ctx._switch = cheri_sentry_create(cheri_bounds_set_exact(ctx._switch, _switch_size) + 1); // +1 for C64

    // Allocate root stack. Todo: use part of the original stack instead
    // of a new private mapping.
    // Note: we place root compartment descriptor at the top of the root
    // compartment stack. It will be overwritten after root compartment
    // initialisation and before switching to main.
    void *root_stack = get_cmpt_stack(ctx.pgsz * 16);
    thunk_data_t *data = (thunk_data_t *)cheri_bounds_set_exact(root_stack - sizeof(thunk_data_t), sizeof(thunk_data_t));
    data->target = NULL; // no need for a function pointer here, we will always call main
    data->exec = NULL; // ditto
    data->tp = NULL; // not used but can be set up here (not currently implemented)
    data->sp = root_stack;
    data->cid = ctx._cid++; // todo: seal this
    return data;
}

// Note: this function is supposed to work in restricted,
// hence no "privileged" operations here.
switch_t *create_compartment(void *target, unsigned stack_pages)
{
    // Generate thunk code:
    int prot = PROT_READ | PROT_WRITE | PROT_MAX(PROT_READ | PROT_WRITE | PROT_EXEC);
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    void *code = mmap(NULL, ctx.pgsz, prot, flags); // 1 page is enough for thunk code and thunk data
    memcpy(code, ctx._thunk, ctx.thunk_size);

    // Setup thunk data used by the switch:
    thunk_data_t *data = (thunk_data_t *)cheri_perms_and(code + ctx.thunk_data_offset, PERM_STORE | PERM_STORE_CAP);
    data->target = cheri_is_sealed(target) ? target : cheri_sentry_create(target);
    data->exec = ctx._switch;
    data->tp = NULL; // not currently used (but could be set up here)
    data->sp = get_cmpt_stack(ctx.pgsz * stack_pages);
    data->cid = ctx._cid++; // todo: seal this

    // Change memory protection flags:
    mprotect(code, ctx.pgsz, PROT_READ | PROT_EXEC);
    char *cache = (char *)cheri_perms_and(code, PERM_LOAD | PERM_STORE);
    __builtin___clear_cache(cache, cache + ctx.pgsz);

    // Fix permissions
    code = cheri_perms_and(code, PERM_GLOBAL | READ_CAP_PERMS | PERM_EXECUTE);
    code = cheri_bounds_set_exact(code, ctx.thunk_size);
    return cheri_sentry_create(code + 1); // +1 for C64
}

long get_compartment_id()
{
    if (is_in_restricted()) {
        void *cid = cheri_cid_get();
        return cheri_address_get(cid);
    } else {
        return -1l;
    }
}

bool is_in_restricted()
{
    return !cheri_check_perms(cheri_pcc_get(), PERM_EXECUTIVE);
}
