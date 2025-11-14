/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libc.h"
#include "morello.h"

static int test_strings(char *argv[], char *envp[]);
static int test_sprintf(char *argv[], char *envp[]);
static int test_morello(char *argv[], char *envp[]);
static int test_memcopy(char *argv[], char *envp[]);

int main(int argc, char *argv[], char *envp[])
{
    int r = 0;
    r += test_strings(argv, envp);
    r += test_sprintf(argv, envp);
    r += test_morello(argv, envp);
    r += test_memcopy(argv, envp);
    if (r) {
        return printf("%d test(s) failed\n", r);
    } else {
        return 0;
    }
}

#define TEST(code, test, fail) ({ \
    count++; \
    code; \
    if ((test)) { \
        printf("Test %s %2d passed: %s\n", name, count, #test); \
    } else { \
        r += 1; \
        printf("Test %s %2d FAILED: %s\n", name, count, #test); \
        fail; \
    } \
})

#define STEST(exp, fmt, ...) \
    TEST(int n = sprintf(str, fmt, __VA_ARGS__), \
    strcmp(str, exp) == 0 && n == (sizeof((exp)) - 1), \
    printf(" - output: `%s`\n", str))

static int test_strings(char *argv[], char *envp[])
{
    int r = 0;
    size_t count = 0;
    const char name[] = "strings";

    TEST({}, strcmp("abc", "ABC") > 0, {});
    TEST({}, strcmp("ABC", "abc") < 0, {});
    TEST({}, strcmp("123", "123") == 0, {});
    TEST(const char *str = "01234567",
        strcmp(str, cheri_bounds_set_exact(str, 4)) > 0, {});
    TEST(const char *str = "01234567",
        strcmp(cheri_bounds_set_exact(str, 5), str) < 0, {});
    TEST({}, strcmp(NULL, NULL) == 0, {});
    TEST(const char str[] = "",
        strlen(str) == 0, {});
    TEST(const char str[] = "01234",
        strlen(str) == 5, {});
    TEST(const char str[] = "0123456789",
        strlen(cheri_bounds_set_exact(str, 3)) == 3, {});
    TEST(const char str[] = "0123456789",
        strlen(cheri_tag_clear(str)) == 0, {});
    TEST(const char src[] = "0123456789"; char dst[64];
        strcpy(dst, src),
        strcmp(src, dst) == 0, {});
    TEST(const char src[] = "0123456789"; char dst[64];
        strcpy(dst, cheri_bounds_set_exact(src, 7)),
        strcmp("0123456", dst) == 0, {});
    TEST(const char src[] = "0123456789"; char dst[7];
        strcpy(dst, src),
        strcmp("0123456", dst) == 0, {});
    TEST(char buf[8]; memset(buf, 'u', sizeof(buf)),
        strcmp(buf, "uuuuuuuu") == 0, {});
    TEST(char buf[8]; memset(buf, 'u', sizeof(buf)+10),
        strcmp(buf, "uuuuuuuu") == 0, {});
    TEST(char buf[8]; memset(buf, 'g', 4); memset(buf + 4, 0, 4),
        strcmp(buf, "gggg") == 0, {});

    return r;
}

static int test_sprintf(char *argv[], char *envp[])
{
    int r = 0;
    size_t count = 0;
    const char name[] = "sprintf";
    char str[256];
    int m;

    STEST("hello", "hello", 0);
    STEST("hello", "%s", "hello");
    STEST("(null)", "%s", NULL);
    STEST("hello (null)", "%s %s", "hello", NULL);
    STEST("hello 42 morello", "%s %d %s", "hello", 42, "morello");
    STEST("CHERI % abc", "%c%c%c%c%c %% %s", 'C', 'H', 'E', 'R', 'I', "abc");
    STEST("13", "%i", 13);
    STEST("-987", "%d", -987);
    STEST("+987", "%+d", 987);
    STEST(" 987", "% d", 987);
    STEST("1024", "%u", 1024);
    STEST("c0ffee", "%x", 0xc0ffee);
    STEST("223338299392", "%li", 13l << 34);
    STEST("8681743812919296", "%ld", 987l << 43);
    STEST("140737488355328", "%lu", 1024ul << 37);
    STEST("303ffb800000000", "%lx", 0xc0ffeeul << 34);
    STEST("0xc0ffee", "%#x", 0xc0ffee);
    STEST("0001234", "%07x", 0x1234);
    STEST("fffffffd", "%x", -3);
    STEST("fffffffffffffffe", "%lx", -2l);
    STEST("4294967293", "%u", -3);
    STEST("18446744073709551614", "%lu", -2l);
    STEST("0", "%p", NULL);
    STEST("0:0000000000000000:0000000000000000", "%#p", NULL);
    TEST(void *cap = NULL; const char exp[] = "0000000000000000 0 [0000000000000000:0000000000000000) ------------------ none 0 of 0";
        m = sprintf(str, "%+#p", cap), strcmp(str, exp) == 0 && m == (sizeof(exp) - 1), {});
    STEST("     hello", "%10s", "hello");
    STEST("hello     ", "%-10s", "hello");
    STEST("hello", "%s%n", "hello", &m);
    TEST(int q = 0; sprintf(str, "%-10d%n", 1, &q), q == 10, {});

    STEST("mismatch 5 %s %c %u", "mismatch %d %s %c %u", 5);
    STEST("mismatch 5", "mismatch %d", 5, "str", false, 2.71f);
    STEST("unsupported 7 (invalid)", "unsupported %d %s", 7, (char *)42ul);
    STEST("unsupported 7 %a (invalid)", "unsupported %d %a %s", 7, 99, (char *)42ul);
    STEST("unsupported -1 %10.4f %-.5e (invalid)", "unsupported %d %10.4f %-.5e %s", -1, 1.2f, 3.14f, "string");

    return r;
}

#undef STEST

static int test_morello(char *argv[], char *envp[])
{
    int r = 0;
    size_t count = 0;
    const char name[] = "morello";

    TEST({}, cheri_is_local(argv) == false, {});
    TEST({}, cheri_is_deref(envp), {});
    TEST({}, cheri_in_bounds(argv), {});
    TEST({}, cheri_length_get_zero(NULL) == 0, {});
    TEST({}, cheri_get_tail(NULL) == 0, {});
    TEST({}, cheri_length_get_zero(argv) == cheri_get_tail(argv), {});
    TEST({}, cheri_length_get_zero(argv) == cheri_length_get(argv), {});
    TEST(char buf[64], strcmp(cap_perms_to_str(buf, argv), "GrRMwWL-----------") == 0, {});
    TEST(void *pcc = cheri_pcc_get(); char buf[64],
        strcmp(cap_perms_to_str(buf, pcc), "GrRM---xES--------") == 0, {});
    TEST(void *sentry = main; char buf[64], strcmp(cap_seal_to_str(buf, sentry), "rb") == 0, {});
    TEST(char buf[64], strcmp(cap_seal_to_str(buf, envp), "none") == 0, {});
    TEST({}, cheri_get_limit(NULL) == NULL, {});

    return r;
}

static int test_memcopy(char *argv[], char *envp[])
{
    int r = 0;
    size_t count = 0;
    const char name[] = "memcopy";

    char dst[64], src[64];
    strcpy(src, "this is morello test 0123456789 for mem copy function 01 234567");

    memset(dst, 0, sizeof(dst));
    TEST(memcpy(dst, src, sizeof(dst)), strcmp(dst, src) == 0 && strlen(dst) == strlen(src), {
        printf("output: `%s`\n", dst);
    });
    memset(dst, 0, sizeof(dst));
    TEST(memcpy(dst, src, 15), strcmp(dst, "this is morello") == 0 && strlen(dst) == 15, {
        printf("output: `%s`\n", dst);
    });
    memset(dst, 0, sizeof(dst));
    TEST(memcpy(dst + 5, src, 15), strcmp(dst + 5, "this is morello") == 0 && strlen(dst + 5) == 15, {
        printf("output: `%s`\n", dst);
    });
    memset(dst, 0, sizeof(dst));
    TEST(memcpy(dst + 5, src + 5, 10), strcmp(dst + 5, "is morello") == 0 && strlen(dst + 5) == 10, {
        printf("output: `%s`\n", dst);
    });

    double x = 3.14;
    typedef struct { int x; bool y; char *p1; double *p2; long z; } object_t;
    object_t s = { .x = 1, .y = true, .p1 = argv[0], .p2 = &x, .z = -1l };
    object_t d = { .x = 2, .y = false, .p1 = NULL, .p2 = NULL, .z = -1l };

    TEST(memcpy(&d, &s, sizeof(object_t)),
        d.x == 1 && d.y && d.p1 == s.p1 && *d.p2 == x && d.z == -1l, {});

    return r;
}


__attribute__((used))
void _start(int argc, char *argv[], char *envp[], auxv_t *auxv)
{
    init(auxv, false);
    exit(main(argc, argv, envp));
}
