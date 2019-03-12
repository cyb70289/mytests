main: main.c test.so test-preload.so
	gcc -o main main.c test.so

test.so: test-c.o test-s.o
	gcc --shared -o $@ $^

test-c.o: test.c
	gcc -fPIC -c -o $@ $^

test-s.o: test.S
	gcc -fPIC -c -o $@ $^

test-preload.so: test-preload.c
	gcc --shared -fPIC -o $@ $^

test: main
	LD_LIBRARY_PATH=./ LD_PRELOAD=./test-preload.so ./main

clean:
	rm -f main *.so *.o
