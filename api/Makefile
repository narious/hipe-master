CC?=gcc
AR?=ar

all: api
api:
	cd build && $(CC) -c -g -O2 -Wall ../src/*.c -lpthread
	$(AR) -rsc build/libhipe.a build/*.o
clean:
	rm -v build/*.o
	rm -v build/libhipe.a
install:
	cp build/libhipe.a /usr/local/lib
	cp src/hipe.h src/hipe_instruction.h /usr/local/include
	cp src/hipe.hpp /usr/local/include/hipe
testing:
	(cd test && $(CC) clock.c -o hipe-clock -lhipe)
	(cd test && $(CC) calc.c -o hipe-calc -lhipe)
	(cd test && $(CC) jpegviewer.c -o hipe-jpegviewer -lhipe)
	(cd test && $(CC) quadrant.c -o hipe-quadrant -lhipe)
	(cd test && $(CC) canvas.c -o hipe-canvas -lhipe)
install-tests:
	cp test/hipe-* /usr/local/bin
