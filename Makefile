CC = gcc
CXX = g++
CPPFLAGS = -g -O3 -Wall -march=native
CXXFLAGS = -std=c++11

.PHONY: all
all:

.PHONY: clean
clean:
	rm -f read-le ascii

ascii: ascii.c ascii.S
	gcc ${CPPFLAGS} -o ascii $^
