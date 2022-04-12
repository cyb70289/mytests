CXXFLAGS = -std=c++11 -g -O3

rvo: main.o lib.a
	g++ -o rvo main.o lib.a

lib.a: lib.o
	ar -cr lib.a lib.o

clean:
	rm -f *.o *.a rvo
