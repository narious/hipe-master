OBJS=build/main.o build/example.o
CC=g++
CFLAGS=-c
LFLAGS=-lgtest -lgtest_main

test: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o $@

build/%.o: src/%.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm test build/*.o

ctags:
	myctags -R . /usr/include/gtest
