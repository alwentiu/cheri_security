/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "morello.h"

typedef size_t (output_fun_t)(void *h, const void *buf, size_t count);

static int printf_core(output_fun_t *fn, void *h, const char *fmt, va_list args);

static size_t stdout_output(__attribute__((unused)) void *unused, const void *buf, size_t count)
{
    const int fd = 1; // stdout
    return write(fd, buf, count);
}

int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int r = printf_core(stdout_output, NULL, fmt, args);
    va_end(args);
    return r;
}

typedef struct {
    char *dst;
    const char *lim;
} output_buf_t;

static size_t buffer_output(void *h, const void *buf, size_t count)
{
    char *src = (char *)buf;
    const char *lim = (char *)cheri_get_limit(buf);
    output_buf_t *ph = (output_buf_t *)h;
    size_t k = 0;
    for(; src < lim && count && ph->dst < ph->lim; src++, ph->dst++, count--) {
        *(ph->dst) = *src;
        k++;
    }
    return k;
}

int sprintf(char *dst, const char *fmt, ...)
{
    output_buf_t h = { .dst = dst, .lim = (char *)cheri_get_limit(dst) };
    va_list args;
    va_start(args, fmt);
    int r = printf_core(buffer_output, &h, fmt, args);
    va_end(args);
    if (h.dst == h.lim) {
        r--;
        *(--h.dst) = '\0';
    } else {
        *(h.dst) = '\0';
    }
    return r;
}

#define TEST(x, f) (((x) & (f)) == (f))

typedef enum { IDLE, FORMAT, RESET } fmt_phase_t;
typedef enum {
    NONE = 0,
    ZERO_PAD = 1,
    LEFT_ALIGN = 2,
    SIZE_LONG = 4,
    SIGN_SPACE = 8,
    SIGN_PLUS = 16,
    HEX = 32,
    ALTERNATE = 64,
} fmt_feat_t;
typedef struct {
    fmt_phase_t phase;
    fmt_feat_t feat;
    size_t width;
} fmt_state_t;

static const char hex_digits[16] = "0123456789abcdef";
static const char dec_digits[10] = "0123456789";

/**
 * Convert unsigned integer to string using given `base` and `digits`.
 * Optional `prefix` can be used to support negative numbers and other
 * prefixes.
 *
 * The output buffer is provided via `end` and must point to the end of
 * the buffer (digits are printed backwards).
 *
 * Return value: pointer to string containing string representation
 * of the integer without null-terminator. Bounds are set to cover
 * only the printable characters.
 */
static char *uint_to_str(char *end, uint64_t val, const char *digits, size_t base, const char *prefix)
{
    char *buf = end;
    if (val == 0ul) {
        --buf; *buf = '0';
    } else {
        for(size_t rem; val > 0ul; val /= base)
        {
            rem = val % base;
            --buf; *buf = digits[rem];
        }
    }
    if (prefix) {
        if (*prefix == 'x') {
            --buf; *buf = 'x';
            --buf; *buf = '0';
        } else {
            --buf; *buf = *prefix;
        }
    }
    return cheri_bounds_set_exact(buf, end - buf);
}

#define ARG(type, a) ({ nargs--; va_arg(a, type); })

/**
 * Limited implementation of printf: only selected format options are
 * supported. Anything unsupported is printed verbatim or ignored.
 *
 * This implementation is safe: it checks bounds for the format string
 * and the variadic arguments, for example, any extra format specifiers
 * that don't match the supplied variadic arguments would be printed
 * as is.
 *
 * Format: %[flags][width][.precision][length]conversion
 *
 *  - Supported flags: `#`, `0`, `-`, ` `, `+`
 *  - Width is supported
 *  - Precision is not implemented (float point conversions aren't either)
 *  - Supported length options: `l`, `z`, `t`
 *  - Supported conversions: `i`, `d`, `u`, `x`, `c`, `s`, `p`, `n`
 *
 * Refer to man 3 printf for more information.
 *
 * The behaviour of `p` conversion (used for pointers) can be modified:
 *  - `%#p` will print capability tag and bytes, for example:
 *     1:dc10400062d0e2a0:0000ffffe05ce2a0
 *  - `%+#p` will print extended human readable form of a capability, for example:
 *     Address (hex     T  Base (hex)       Limit (hex)       Permissions        Seal Offset   Length
 *     00000000002105c5 1 [0000000000200000:0000000000233000) GrRM---xES----V--- rb   67013 of 208896
 *
 */
static int printf_core(output_fun_t *fn, void *h, const char *fmt, va_list args)
{
    const char *limit = (char *)cheri_get_limit(fmt); // end of fmt string
    const void *pa = (void *)args;
    size_t nargs = pa == NULL ? 0ul : cheri_length_get(pa) / sizeof(void *); // max num of args
    char buffer[64], *end = buffer + sizeof(buffer); // tmp buffer to store integers converted to string
    int n = 0; // number of chars printed
    fmt_state_t state = { .phase = RESET }; // state of the formatter
    char *p = (char *)fmt, *a; // pointers to current character in the fmt string
    for (; p < limit && *p != '\0'; p++) {
        if (state.phase == RESET) {
            state.phase = IDLE;
            state.feat = NONE;
            state.width = 0;
            a = p;
        } else if (state.phase == IDLE) {
            state.feat = NONE;
            state.width = 0;
        }
        switch (*p) {
            case '%':
            {
                if (state.phase == IDLE) {
                    state.phase = FORMAT;
                    n += fn(h, a, p - a);
                    a = p;
                } else {
                    state.phase = RESET;
                    n += fn(h, p, 1);
                }
                break;
            }
            default:
            {
                if (state.phase == FORMAT) {
                    switch (*p) {
                        case 's':
                            if (nargs > 0) {
                                const char *arg = (char *) ARG(char *, args);
                                if (arg == NULL) {
                                    arg = "(null)";
                                } else if (!cheri_is_deref(arg)) {
                                    arg = "(invalid)";
                                }
                                size_t len = strlen(arg);
                                if (len < state.width) {
                                    bool left = TEST(state.feat, LEFT_ALIGN);
                                    if (left) {
                                        n += fn(h, arg, len);
                                    }
                                    size_t l = state.width - len;
                                    char *pad = (char *)alloca(l);
                                    memset(pad, ' ', l);
                                    n += fn(h, pad, l);
                                    if (!left) {
                                        n += fn(h, arg, len);
                                    }
                                } else {
                                    n += fn(h, arg, len);
                                }
                                state.phase = RESET;
                            } else {
                                state.phase = IDLE;
                            }
                            break;
                        case 'c':
                            if (nargs > 0) {
                                unsigned char arg = (unsigned char)ARG(int, args);
                                n += fn(h, &arg, 1);
                                state.phase = RESET;
                            } else {
                                state.phase = IDLE;
                            }
                            break;
                        case 'n':
                            if (nargs > 0) {
                                int *arg = (int *)ARG(int *, args);
                                if (cheri_is_deref(arg)) {
                                    *arg = n;
                                }
                                state.phase = RESET;
                            } else {
                                state.phase = IDLE;
                            }
                            break;
                        case 'p':
                            if (TEST(state.feat, ALTERNATE) && TEST(state.feat, SIGN_PLUS)) {
                                if (nargs > 0) {
                                    void *cap = (void *)ARG(void *, args);
                                    bool tag = cheri_tag_get(cap);
                                    size_t cap_addr = cheri_address_get(cap);
                                    size_t cap_base = cheri_base_get(cap);
                                    size_t cap_limit = (size_t)cheri_get_limit(cap);
                                    ssize_t cap_offset = (ssize_t)cheri_offset_get(cap);
                                    size_t cap_length = cheri_length_get_zero(cap);
                                    char capstr[128];
                                    char perm[19];
                                    char seal[5];
                                    int sz = sprintf(capstr, "%016lx %c [%016lx:%016lx) %s %-4s %ld of %lu",
                                        cap_addr, tag ? '1' : '0', cap_base, cap_limit,
                                        cap_perms_to_str(perm, cap), cap_seal_to_str(seal, cap),
                                        cap_offset, cap_length);
                                    n += fn(h, capstr, sz);
                                }
                                state.phase = RESET;
                                break;
                            } else if (TEST(state.feat, ALTERNATE)) {
                                if (nargs > 0) {
                                    void *cap = (void *)ARG(void *, args);
                                    bool tag = cheri_tag_get(cap);
                                    size_t lo = cheri_address_get(cap);
                                    size_t hi = cheri_copy_from_high(cap);
                                    char capstr[48];
                                    int sz = sprintf(capstr, "%c:%016lx:%016lx", tag ? '1' : '0', hi, lo);
                                    n += fn(h, &capstr, sz);
                                }
                                state.phase = RESET;
                                break;
                            } else {
                                state.feat |= SIZE_LONG;
                                // no break
                            }
                        case 'x':
                            state.feat |= HEX;
                            // no break
                        case 'u':
                            if (nargs > 0) {
                                unsigned long arg;
                                if (TEST(state.feat, SIZE_LONG)) {
                                    arg = (unsigned long)ARG(unsigned long, args);
                                } else {
                                    arg = (unsigned int)ARG(unsigned int, args);
                                }
                                char *t;
                                if (TEST(state.feat, HEX)) {
                                    const char *prefix = NULL;
                                    if (TEST(state.feat, ALTERNATE)) {
                                        prefix = "x";
                                    }
                                    t = uint_to_str(end, arg, hex_digits, 16, prefix);
                                } else {
                                    t = uint_to_str(end, arg, dec_digits, 10, NULL);
                                }
                                size_t len = cheri_length_get(t);
                                if (len < state.width) {
                                    bool left = TEST(state.feat, LEFT_ALIGN);
                                    if (left) {
                                        n += fn(h, t, len);
                                    }
                                    size_t l = state.width - len;
                                    char *pad = (char *)alloca(l);
                                    memset(pad, left ? ' ' : TEST(state.feat, ZERO_PAD) ? '0' : ' ', l);
                                    n += fn(h, pad, l);
                                    if (!left) {
                                        n += fn(h, t, len);
                                    }
                                } else {
                                    n += fn(h, t, len);
                                }
                                state.phase = RESET;
                            } else {
                                state.phase = IDLE;
                            }
                            break;
                        case 'i':
                        case 'd':
                            if (nargs > 0) {
                                long arg;
                                if (TEST(state.feat, SIZE_LONG)) {
                                    arg = (long)ARG(long, args);
                                } else {
                                    arg = (int)ARG(int, args);
                                }
                                bool neg = arg < 0;
                                const char *sign;
                                if (neg) {
                                    sign = "-";
                                } else if (TEST(state.feat, SIGN_PLUS)) {
                                    sign = "+";
                                } else if (TEST(state.feat, SIGN_SPACE)) {
                                    sign = " ";
                                } else {
                                    sign = NULL;
                                }
                                char *t = neg
                                    ? uint_to_str(end, -arg, dec_digits, 10, sign)
                                    : uint_to_str(end, arg, dec_digits, 10, sign);
                                size_t len = cheri_length_get(t);
                                if (!sign && len < state.width) {
                                    bool left = TEST(state.feat, LEFT_ALIGN);
                                    if (left) {
                                        n += fn(h, t, len);
                                    }
                                    size_t l = state.width - len;
                                    char *pad = (char *)alloca(l);
                                    memset(pad, left ? ' ' : TEST(state.feat, ZERO_PAD) ? '0' : ' ', l);
                                    n += fn(h, pad, l);
                                    if (!left) {
                                        n += fn(h, t, len);
                                    }
                                } else {
                                    n += fn(h, t, len);
                                }
                                state.phase = RESET;
                            } else {
                                state.phase = IDLE;
                            }
                            break;
                        case '0':
                            if (state.width) {
                                state.width = 10 * state.width;
                                break;
                            } else {
                                state.feat |= ZERO_PAD;
                                break;
                            }
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            state.width = 10 * state.width + ((size_t)(*p) - '0');
                            break;
                        case '-':
                            state.feat |= LEFT_ALIGN;
                            break;
                        case 'l':
                        case 'z':
                        case 't':
                            state.feat |= SIZE_LONG;
                            break;
                        case ' ':
                            state.feat |= SIGN_SPACE;
                            break;
                        case '+':
                            state.feat |= SIGN_PLUS;
                            break;
                        case '#':
                            state.feat |= ALTERNATE;
                            break;
                        default:
                            // print unsupported format strings verbatim
                            state.phase = IDLE;
                            break;
                    }
                }
                break;
            }
        }
    }
    if (state.phase != RESET && p > a) {
        n += fn(h, a, p - a);
    }
    return n;
}
