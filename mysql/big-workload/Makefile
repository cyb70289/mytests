CC = gcc-10
CFLAGS = -O2 -Wall

ARCH=$(shell uname -m)

.PHONY: all
all: high-ipc-big-workload-$(ARCH) high-ipc-small-workload-$(ARCH) \
	 low-ipc-big-workload-$(ARCH) low-ipc-small-workload-$(ARCH)

high-ipc-big-workload-$(ARCH): main.c funcs.c
	$(CC) $(CFLAGS) -DHIGH_IPC_WORKLOAD -o $@ $^

high-ipc-small-workload-$(ARCH): main.c funcs.c
	$(CC) $(CFLAGS) -DHIGH_IPC_WORKLOAD -DSMALL_WORKLOAD -o $@ $^

low-ipc-big-workload-$(ARCH): main.c funcs.c
	$(CC) $(CFLAGS) -o $@ $^

low-ipc-small-workload-$(ARCH): main.c funcs.c
	$(CC) $(CFLAGS) -DSMALL_WORKLOAD -o $@ $^

.PHONY: profile
profile: all
	@echo "===================== High IPC BIG workload ======================"
	sudo perf stat -e instructions,cycles ./high-ipc-big-workload-$(ARCH)
	@echo "==================== High IPC SMALL workload ====================="
	sudo perf stat -e instructions,cycles ./high-ipc-small-workload-$(ARCH)
	@echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo "====================== Low IPC BIG workload ======================"
	sudo perf stat -e instructions,cycles ./low-ipc-big-workload-$(ARCH)
	@echo "===================== Low IPC SMALL workload ====================="
	sudo perf stat -e instructions,cycles ./low-ipc-small-workload-$(ARCH)

.PHONY: clean
clean:
	rm -f *-ipc-*-workload-$(ARCH)
