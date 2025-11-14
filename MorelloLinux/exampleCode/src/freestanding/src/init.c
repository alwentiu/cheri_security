/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "auxv.h"
#include "morello.h"

typedef struct {
    uint64_t location;
    uint64_t base;
    size_t offset;
    size_t size;
    size_t permissions;
} cap_relocs_entry_t;

#define __get_addr_of(sym) ({ \
    void *p; \
    __asm__ ( \
    ".weak " sym "\n" "adrp %0, " sym "\n" "add %0, %0, :lo12:" sym "\n" : "=X"(p)); \
    p; \
})

__attribute__((unused))
static int init_cap_relocs(const auxv_t *auxv)
{
    const cap_relocs_entry_t *cap_start = __get_addr_of("__cap_relocs_start");
    const cap_relocs_entry_t *cap_end = __get_addr_of("__cap_relocs_end");
    if (cap_end == cap_start || cap_start == NULL || cap_end == NULL) {
        return 1;
    }
    void *rw = NULL;
    void *rx = NULL;
    for (; auxv->type; auxv++) {
        switch (auxv->type) {
            case AT_CHERI_EXEC_RW_CAP: rw = auxv->ptr; break;
            case AT_CHERI_EXEC_RX_CAP: rx = auxv->ptr; break;
        }
        if (rw && rx) break;
    }
    if (!rw || !rx) {
        return 2;
    }
    for (const cap_relocs_entry_t *r = cap_start; r < cap_end; r++) {
        if (!r->base) continue;
        void *cap = NULL;
        size_t perm = ~r->permissions;
        bool is_fun_ptr = perm & PERM_EXECUTE;
        bool is_writable = perm & PERM_STORE;
        if (is_writable) {
            cap = cheri_address_set(rw, r->base);
        } else {
            cap = cheri_address_set(rx, r->base);
        }
        cap = cheri_perms_and(cap, perm);
        cap = cheri_bounds_set_exact(cap, r->size);
        cap = cheri_offset_set(cap, r->offset);
        if (is_fun_ptr) {
            cap = cheri_sentry_create(cap);
        }
        // store capability
        void **loc = cheri_address_set(rw, r->location);
        loc = cheri_bounds_set_exact(loc, sizeof(void *));
        *loc = cap;
    }
    return 0;
}

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
} rela_t;

typedef struct {
    uint64_t address;
    uint64_t length : 56;
    uint64_t perms : 8;
} cap_rela_t;

#define MORELLO_RELA_PERM_RX 4
#define MORELLO_RELA_PERM_RW 2
#define MORELLO_RELA_PERM_R 1

#define R_MORELLO_RELATIVE 59395

static int init_morello_relative(const auxv_t *auxv, size_t clrperm)
{
    const rela_t *rela_start = __get_addr_of("__rela_dyn_start");
    const rela_t *rela_end = __get_addr_of("__rela_dyn_end");
    if (rela_start == rela_end || rela_start == NULL || rela_end == NULL) {
        return 1;
    }
    void *rw = NULL;
    void *rx = NULL;
    for (; auxv->type; auxv++) {
        switch (auxv->type) {
            case AT_CHERI_EXEC_RW_CAP: rw = auxv->ptr; break;
            case AT_CHERI_EXEC_RX_CAP: rx = auxv->ptr; break;
        }
        if (rw && rx) break;
    }
    if (!rw || !rx) {
        return 2;
    }
    for (const rela_t *r = rela_start; r < rela_end; r++) {
        if (r->r_info != R_MORELLO_RELATIVE) continue;
        void *cap = NULL, **loc = cheri_address_set(rw, r->r_offset);
        const cap_rela_t *u = (cap_rela_t *)loc;
        switch (u->perms) {
            case MORELLO_RELA_PERM_R:
                cap = cheri_perms_and(rx, PERM_GLOBAL | READ_CAP_PERMS);
                break;
            case MORELLO_RELA_PERM_RW:
                cap = cheri_perms_and(rw, PERM_GLOBAL | READ_CAP_PERMS | WRITE_CAP_PERMS);
                break;
            case MORELLO_RELA_PERM_RX:
                cap = cheri_perms_and(rx, PERM_GLOBAL | READ_CAP_PERMS | EXEC_CAP_PERMS);
                break;
            default:
                cap = cheri_perms_and(rx, 0);
                break;
        }
        cap = cheri_address_set(cap, u->address);
        cap = cheri_bounds_set_exact(cap, u->length);
        cap = cap + r->r_addend;
        if (u->perms == MORELLO_RELA_PERM_RX) {
            if (clrperm) {
                cap = cheri_perms_and(cap, ~clrperm);
            }
            cap = cheri_sentry_create(cap);
        }
        *loc = cap;
    }
    return 0;
}

static auxv_t __auxv[AT_ENUM_MAX];

int init(const auxv_t *auxv, bool restricted)
{
    size_t clrperm = restricted ? (PERM_EXECUTIVE | PERM_SYS_REG) : 0ul;
    int r = init_morello_relative(auxv, clrperm);
    if (r != 0) {
        return r;
    }
    for (const auxv_t *entry = auxv; entry->type; entry++) {
        auxv_t *p = &__auxv[entry->type];
        p->type = entry->type;
        p->ptr = entry->ptr;
    }
    return 0;
}

void *getauxptr(unsigned long id)
{
    if (id < AT_ENUM_MAX) {
        return __auxv[id].ptr;
    } else {
        return NULL;
    }
}

unsigned long getauxval(unsigned long id)
{
    if (id < AT_ENUM_MAX) {
        return __auxv[id].val;
    } else {
        return 0ul;
    }
}
