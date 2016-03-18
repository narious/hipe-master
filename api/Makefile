all: api
api:
	(cd build && gcc -c -Wall ../src/*.c)
	ar -rsc build/libhipe.a build/*.o
clean:
	rm -v build/*.o
	rm -v build/libhipe.a
install:
	cp build/libhipe.a /usr/lib
	cp src/hipe.h src/hipe_instruction.h /usr/include
testing:
	(cd test && gcc clock.c -o clock -lhipe)
	(cd test && gcc calc.c -o calc -lhipe)
	(cd test && gcc jpegviewer.c -o jpegviewer -lhipe)
	(cd test && gcc quadrant.c -o quadrant -lhipe)
	(cd test && gcc canvas.c -o canvas -lhipe)
