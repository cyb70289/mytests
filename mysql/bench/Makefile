CPPFLAGS = -g -O3 -Wall -march=native -pthread
CXXFLAGS = -std=c++11

all: bench-lock

bench-lock: bench-lock.cc os0atomic.h

clean:
	rm -f bench-lock
