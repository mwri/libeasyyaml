TESTS = check_easyyaml check_hello_tiny check_hello_world check_hello_universe
check_PROGRAMS = $(TESTS)

clean-local:
	rm -f *.gcda *.gcno *.gcov

check_easyyaml_SOURCES = check_easyyaml.c \
	easyyaml_check.c \
	../src/easyyaml.c
check_easyyaml_CFLAGS = @CHECK_CFLAGS@ -I../src --coverage
check_easyyaml_LDFLAGS = -lyaml
check_easyyaml_LDADD = @CHECK_LIBS@

check_hello_tiny_SOURCES = ./../examples/ey_hello_tiny.c \
	../src/easyyaml.c
check_hello_tiny_CFLAGS = @CHECK_CFLAGS@ -I../src
check_hello_tiny_LDFLAGS = -lyaml
check_hello_tiny_LDADD = @CHECK_LIBS@

check_hello_world_SOURCES = ./../examples/ey_hello_world.c \
	../src/easyyaml.c
check_hello_world_CFLAGS = @CHECK_CFLAGS@ -I../src
check_hello_world_LDFLAGS = -lyaml
check_hello_world_LDADD = @CHECK_LIBS@

check_hello_universe_SOURCES = ./../examples/ey_hello_universe.c \
	../src/easyyaml.c
check_hello_universe_CFLAGS = @CHECK_CFLAGS@ -I../src
check_hello_universe_LDFLAGS = -lyaml
check_hello_universe_LDADD = @CHECK_LIBS@
