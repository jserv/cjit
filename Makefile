CXX ?= g++

LIBMIR = mir/libmir.a
$(LIBMIR): mir/mir.c
	$(MAKE) -C mir
mir/mir.c:
	git clone https://github.com/vnmakarov/mir

ir/ir.o: ir/ir.c
	$(MAKE) -C ir BUILD=release
ir/ir.c:
	git clone https://github.com/dstogov/ir

IR_OBJS = \
    ir/ir.o ir/ir_strtab.o ir/ir_cfg.o \
    ir/ir_sccp.o ir/ir_gcm.o ir/ir_ra.o ir/ir_emit.o \
    ir/ir_load.o ir/ir_save.o ir/ir_emit_c.o ir/ir_dump.o \
    ir/ir_disasm.o ir/ir_gdb.o ir/ir_perf.o ir/ir_check.o \
    ir/ir_cpuinfo.o

utest:
	git clone https://github.com/sheredom/utest.h $@

.DEFAULT_GOAL :=
all: main

CXXFLAGS = -O3 -Wall
CXXFLAGS += -DIR_TARGET_X64
CXXFLAGS += -Imir -I.

main: $(LIBMIR) utest jit-mir.cc main.cc ir/ir.o
	$(CXX) -o $@ $(CXXFLAGS) \
		jit-mir.cc \
		main.cc \
		$(LIBMIR) \
		$(IR_OBJS) \
		-lcapstone \
		-ldl \
		-lpthread

clean:
	rm -f main

distclean: clean
	rm -rf mir ir utest
