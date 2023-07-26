CC = gcc-10
CXX = g++-10
CPPFLAGS = -g -O2 -Wall -pthread
CXXFLAGS = -std=c++17

.PHONY: all
all: chrono-overhead papi-overhead

papi-overhead: LDLIBS = -lpapi

.PHONY: clean
clean:
	rm -rf chrono-overhead papi-overhead
