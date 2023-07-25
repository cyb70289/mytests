CC = gcc-10
CXX = g++-10
CPPFLAGS = -g -O2 -Wall -pthread
CXXFLAGS = -std=c++17

.PHONY: all
all: map map2 map3

map3: LDLIBS = -lpapi
map4: LDLIBS = -lpapi

.PHONY: clean
clean:
	rm -rf map map2 map3
