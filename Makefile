LIBMIR = mir/libmir.a
$(LIBMIR): mir/mir.c
	$(MAKE) -C mir
mir/mir.c:
	git clone https://github.com/vnmakarov/mir

utest:
	git clone https://github.com/sheredom/utest.h $@

main: $(LIBMIR) utest
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
