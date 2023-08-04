#include <stdio.h>
#include <string.h>
#include <utest/utest.h>

#include "jit.h"

namespace
{
struct JitC {
    cjit::JitCompiler *compiler;
};
}  // namespace

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

    // NOTE: we need to add declaration of is_prime in c_code to make it work
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

    // NOTE: newline after macro definition is needed.
    cjit::CompiledInfo info = compiler->compile(c_code);

    typedef int (*fun_p)(int, int);
    fun_p fun = (fun_p) info.binary;

    ASSERT_EQ(13, fun(2, 3));
}

typedef int (*mandelbrot_t)(double, double);
static void run(mandelbrot_t mandelbrot)
{
    fflush(stdout);
    for (int y = -39; y < 39; y++) {
        printf("\n");
        for (int x = -39; x < 39; x++) {
            int i = mandelbrot(x / 40.0, y / 40.0);
            printf("%c", i ? ' ' : '*');
        }
    }
    printf("\n");
}

static int mandelbrot_c(double x, double y)
{
    double cr = y - 0.5;
    double ci = x;
    double zi = 0.0;
    double zr = 0.0;
    int i = 0;

    while (1) {
        i++;
        double temp = zr * zi;
        double zr2 = zr * zr;
        double zi2 = zi * zi;
        zr = zr2 - zi2 + cr;
        zi = temp + temp + ci;
        if (zi2 + zr2 > 16)
            return i;
        if (i > 1000)
            return 0;
    }
}

UTEST_F(JitC, mandelbrot_static)
{
    run(mandelbrot_c);
    ASSERT_TRUE(1);
}

UTEST_F(JitC, mandelbrot_mir)
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

    run((mandelbrot_t) info.binary);
    ASSERT_TRUE(info.binary);
}

#include "ir/ir.h"
#include "ir/ir_builder.h"

static void gen_mandelbrot(ir_ctx *ctx)
{
    ir_START();
    /* clang-format off */
    ir_ref x = ir_PARAM(IR_DOUBLE, "x", 1);
    ir_ref y = ir_PARAM(IR_DOUBLE, "y", 2);
    ir_ref cr = ir_SUB_D(y, ir_CONST_DOUBLE(0.5));
    ir_ref ci = ir_COPY_D(x);
    ir_ref zi = ir_COPY_D(ir_CONST_DOUBLE(0.0));
    ir_ref zr = ir_COPY_D(ir_CONST_DOUBLE(0.0));
    ir_ref i = ir_COPY_D(ir_CONST_I32(0));

    ir_ref loop = ir_LOOP_BEGIN(ir_END());
        ir_ref zi_1 = ir_PHI_2(zi, IR_UNUSED);
        ir_ref zr_1 = ir_PHI_2(zr, IR_UNUSED);
        ir_ref i_1 = ir_PHI_2(i, IR_UNUSED);

        ir_ref i_2 = ir_ADD_I32(i_1, ir_CONST_I32(1));
        ir_ref temp = ir_MUL_D(zr_1, zi_1);
        ir_ref zr2 = ir_MUL_D(zr_1, zr_1);
        ir_ref zi2 = ir_MUL_D(zi_1, zi_1);
        ir_ref zr_2 = ir_ADD_D(ir_SUB_D(zr2, zi2), cr);
        ir_ref zi_2 = ir_ADD_D(ir_ADD_D(temp, temp), ci);
        ir_ref if_1 = ir_IF(ir_GT(ir_ADD_D(zi2, zr2), ir_CONST_DOUBLE(16.0)));
            ir_IF_TRUE(if_1);
                ir_RETURN(i_2);
            ir_IF_FALSE(if_1);
                ir_ref if_2 = ir_IF(ir_GT(i_2, ir_CONST_I32(1000)));
                ir_IF_TRUE(if_2);
                    ir_RETURN(ir_CONST_I32(0));
                ir_IF_FALSE(if_2);
                    ir_ref loop_end = ir_LOOP_END();

    /* close loop */
    ir_MERGE_SET_OP(loop, 2, loop_end);
    ir_PHI_SET_OP(zi_1, 2, zi_2);
    ir_PHI_SET_OP(zr_1, 2, zr_2);
    ir_PHI_SET_OP(i_1, 2, i_2);
    /* clang-format on */
}

UTEST_F(JitC, mandelbrot_ir)
{
    uint32_t mflags = 0;
    uint32_t cpuinfo = ir_cpuinfo();
    if (cpuinfo & IR_X86_AVX)
        mflags |= IR_X86_AVX;

    uint32_t flags = IR_OPT_FOLDING | IR_OPT_CFG | IR_OPT_CODEGEN;
    ir_ctx ctx;
    ir_init(&ctx, flags, 256, 1024);
    ctx.mflags = mflags;

    gen_mandelbrot(&ctx);
    ir_build_def_use_lists(&ctx);
    ir_sccp(&ctx);
    ir_build_cfg(&ctx);
    ir_build_dominators_tree(&ctx);
    ir_find_loops(&ctx);
    ir_gcm(&ctx);
    ir_schedule(&ctx);
    ir_match(&ctx);
    ir_assign_virtual_registers(&ctx);
    ir_compute_live_ranges(&ctx);
    ir_coalesce(&ctx);
    ir_reg_alloc(&ctx);
    ir_schedule_blocks(&ctx);
    ir_truncate(&ctx);

    size_t size;
    void *entry = ir_emit_code(&ctx, &size);
    run((mandelbrot_t) entry);

    ir_free(&ctx);
    ASSERT_TRUE(size);
}

UTEST_MAIN()
