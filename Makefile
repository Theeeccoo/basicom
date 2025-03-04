build/echo_cpp: clean coroutine.h build/coroutine.a
	g++ -I. -Wall -Wextra -ggdb -o build/echo_cpp echo.cpp build/coroutine.a

build/coroutine.a: build/coroutine.o
	ar -rcs build/coroutine.a build/coroutine.o

build/coroutine.o: coroutine.c coroutine.h
	mkdir -p build
	gcc -Wall -Wextra -ggdb -c -o build/coroutine.o coroutine.c

clean:
	rm -f -r build/

