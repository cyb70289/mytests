test: main.o 1.o 2.o
	g++ -std=c++11 -o test $^

%.o: %.cc inline.h
	g++ -std=c++11 -c $<

clean:
	rm -f test *.o
