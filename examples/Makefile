all: ey_hello_tiny ey_hello_world ey_hello_universe

ey_hello_tiny: Makefile ey_hello_tiny.c
	gcc ey_hello_tiny.c -Wall -g -O0 -I../src -L../src/.libs -Wl,-rpath ../src/.libs -o ey_hello_tiny -leasyyaml

ey_hello_world: Makefile ey_hello_world.c
	gcc ey_hello_world.c -Wall -g -O0 -I../src -L../src/.libs -Wl,-rpath ../src/.libs -o ey_hello_world -leasyyaml

ey_hello_universe: Makefile ey_hello_universe.c
	gcc ey_hello_universe.c -Wall -g -O0 -I../src -L../src/.libs -Wl,-rpath ../src/.libs -o ey_hello_universe -leasyyaml

clean:
	rm -f ey_hello_tiny ey_hello_world ey_hello_universe

check:
