CC = gcc
CXX = g++
CPPFLAGS = -g -O3 -Wall -march=native
CXXFLAGS = -std=c++11

.PHONY: clean
clean:
	rm -f read-le
