CC = gcc-10
CFLAGS = -O2 -Wall

ARCH=$(shell uname -m)

.PHONY: all
all: big-$(ARCH) small-$(ARCH)

big-$(ARCH): main.c funcs.c
	$(CC) $(CFLAGS) -o $@ $^

small-$(ARCH): main.c funcs.c
	$(CC) $(CFLAGS) -DSMALL_WORKLOAD -o $@ $^

.PHONY: profile
profile: all
	@echo "===================== BIG workload ======================"
	sudo perf stat -e instructions,cycles ./big-$(ARCH)
	@echo "==================== SMALL workload ====================="
	sudo perf stat -e instructions,cycles ./small-$(ARCH)

.PHONY: clean
clean:
	rm -f big-$(ARCH) small-$(ARCH)
