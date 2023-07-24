#include <stdio.h>
#include <string.h>
#include <utest/utest.h>

#include "jit.h"

namespace
{
struct JitC {
    cjit::JitCompiler *compiler;
};
}

#define JITC (utest_fixture->compiler)

UTEST_F_SETUP(JitC)
{
    JITC = cjit::create<cjit::MirCompiler>();
}

UTEST_F_TEARDOWN(JitC)
{
    delete JITC;
}

UTEST_F(JitC, simple_add)
{
    auto *compiler = JITC;
    const char *c_code = "int func_add(int a, int b) { return a + b; }\n";
    cjit::CompiledInfo info = compiler->compile(c_code);

    typedef int (*fun_p)(int, int);
    fun_p fun = (fun_p) info.binary;

    ASSERT_EQ(3, fun(1, 2));
}

static int ext_is_prime(int num)
{
    for (int i = 2; i * i <= num; i++)
        if (num % i == 0)
            return 0;
    return 1;
}

UTEST_F(JitC, call_ext)
{
    auto *compiler = JITC;
    const char *c_code =
        " \
	extern int is_prime(int); \
        int count_prime(int max) { \
            int n = 0; \
            for (int i = 3; i < max; i++) \
                if (is_prime(i)) \
                    n++; \
            return n; \
        } \n";

    // NOTE: we need to add declearation of is_prime in c_code to make it work
    // (for MIR?)
    compiler->importSymbol("is_prime", (void *) ext_is_prime);
    cjit::CompiledInfo info = compiler->compile(c_code);

    typedef int (*fun_p)(int);
    fun_p fun = (fun_p) info.binary;

    ASSERT_EQ(5, fun(15));
}

UTEST_F(JitC, simple_micro)
{
    auto *compiler = JITC;
    const char *c_code =
        " \
        #define POW(x) ((x) * (x)) \n\
        int func_add(int a, int b) { \
            return POW(a) + POW(b); \
        }\n";

    // NOTE: newline after macro defination is needed.
    cjit::CompiledInfo info = compiler->compile(c_code);

    typedef int (*fun_p)(int, int);
    fun_p fun = (fun_p) info.binary;

    ASSERT_EQ(13, fun(2, 3));
}

UTEST_F(JitC, mandelbrot)
{
    auto *compiler = JITC;
    const char *c_code =
        "           \
        int mandelbrot(double x, double y) \
        { \
            double cr = y - 0.5; \
            double ci = x; \
            double zi = 0.0; \
            double zr = 0.0; \
            int i = 0; \
            while (1) { \
                i++; \
                double temp = zr * zi; \
                double zr2 = zr * zr; \
                double zi2 = zi * zi; \
                zr = zr2 - zi2 + cr; \
                zi = temp + temp + ci; \
                if (zi2 + zr2 > 16) \
                    return i; \
                if (i > 1000) \
                    return 0; \
            } \
        }\n";

    cjit::CompiledInfo info = compiler->compile(c_code);

    typedef int (*fun_p)(double, double);
    fun_p fun = (fun_p) info.binary;

    for (int y = -39; y < 39; y++) {
        printf("\n");
        for (int x = -39; x < 39; x++) {
            int i = fun(x / 40.0, y / 40.0);
            printf("%c", i ? ' ' : '*');
        }
    }
    printf("\n");
    ASSERT_TRUE(fun);
}

UTEST_MAIN()
