#include <stdio.h>
#include <string.h>
#include <utest/utest.h>

#include "jit.h"

namespace {
struct JitC {
    cjit::JitCompiler *compiler;
};
}

#define JITC (utest_fixture->compiler)

UTEST_F_SETUP(JitC) {
    JITC = cjit::create<cjit::MirCompiler>();
}

UTEST_F_TEARDOWN(JitC) {
    delete JITC;
}

UTEST_F(JitC, simple_add) {
    auto *compiler = JITC;
    const char *c_code = "int func_add(int a, int b) { return a + b; } \n";
    cjit::CompiledInfo info = compiler->compile(c_code);

    typedef int (*fun_p)(int, int);
    fun_p fun = (fun_p) info.binary;

    ASSERT_EQ(3, fun(1, 2));
}

static int ext_is_prime(int num) {
    for (int i = 2; i * i <= num; i++)
        if (num % i == 0)
            return 0;
    return 1;
}

UTEST_F(JitC, call_ext) {
    auto *compiler = JITC;
    const char *c_code =
        "extern int is_prime(int);      \
    int count_prime(int max) {          \
    int n = 0;                          \
    for (int i = 3; i < max; i++)       \
        if (is_prime(i))                \
            n++;                        \
    return n;                           \
 } \n";

    // NOTE: we need to add declearation of is_prime in c_code to make it work
    // (for MIR?)
    compiler->importSymbol("is_prime", (void *) ext_is_prime);
    cjit::CompiledInfo info = compiler->compile(c_code);

    typedef int (*fun_p)(int);
    fun_p fun = (fun_p) info.binary;

    ASSERT_EQ(5, fun(15));
}

UTEST_F(JitC, simple_micro) {
    auto *compiler = JITC;
    const char *c_code =
        "           \
    #define POW(x) ((x) * (x))       \n\
    int func_add(int a, int b) {     \
        return POW(a) + POW(b);      \
    }\n";

    // NOTE: newline after macro defination is needed.
    cjit::CompiledInfo info = compiler->compile(c_code);

    typedef int (*fun_p)(int, int);
    fun_p fun = (fun_p) info.binary;

    ASSERT_EQ(13, fun(2, 3));
}

UTEST_MAIN()
