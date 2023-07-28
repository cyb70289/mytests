CC = gcc-10
CFLAGS = -O2 -Wall

ARCH=$(shell uname -m)

workload-$(ARCH): main.c funcs.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f workload-$(ARCH)
