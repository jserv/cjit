LIBMIR = mir/libmir.a
$(LIBMIR): mir/mir.c
	$(MAKE) -C mir
mir/mir.c:
	git clone https://github.com/vnmakarov/mir

utest:
	git clone https://github.com/sheredom/utest.h $@

.DEFAULT_GOAL :=
all: main

main: $(LIBMIR) utest jit-mir.cc main.cc
	g++ -o $@ \
		-I./mir -I. \
		jit-mir.cc \
		main.cc \
		$(LIBMIR) \
		-lpthread

clean:
	rm -f main

distclean: clean
	rm -rf mir utest
