CC = gcc
CXX = g++
CPPFLAGS = -g -O3 -Wall -march=native -pthread
CXXFLAGS = -std=c++11

.PHONY: all
all:

.PHONY: clean
clean:
	rm -f read-le ascii 8way pagesize mt-print mt-cond order per-thread \
	      rabin printf order2 callonce utf8-len ceph-hash c-conv

ascii: ascii.c ascii.S
	gcc ${CPPFLAGS} -o ascii $^
