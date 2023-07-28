CC = gcc-10
CFLAGS = -g -O2 -Wall

ARCH=$(shell uname -m)

workload-$(ARCH): main.c funcs.c dummy.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f workload-$(ARCH)
