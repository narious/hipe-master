CC?=gcc
AR?=ar

all: api
api:
	cd build && $(CC) -c -O2 -Wall ../src/*.c
	$(AR) -rsc build/libhipe.a build/*.o
clean:
	rm -v build/*.o
	rm -v build/libhipe.a
install:
	cp build/libhipe.a /usr/local/lib
	cp src/hipe.h src/hipe_instruction.h /usr/local/include
	cp src/hipetec.hh /usr/local/include/hipetec
testing:
	(cd test && $(CC) clock.c -o clock -lhipe)
	(cd test && $(CC) calc.c -o calc -lhipe)
	(cd test && $(CC) jpegviewer.c -o jpegviewer -lhipe)
	(cd test && $(CC) quadrant.c -o quadrant -lhipe)
	(cd test && $(CC) canvas.c -o canvas -lhipe)
