OBJS=build/main.o build/example.o build/TimerTest.o build/LogTest.o
CC=g++
CFLAGS=-c
LFLAGS=-lgtest -lgtest_main -lpthread -I../../

test: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o $@

build/%.o: src/%.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm test build/*.o

