CPPFLAGS += -Wall -g -march=armv8-a -pthread
CFLAGS += -std=gnu11 -fno-strict-aliasing -Wno-pointer-sign

# run "make D=1" to build debug binary
ifeq ($(origin D),command line)
    $(info build debug binary)
    CPPFLAGS += -O0
else
    CPPFLAGS += -O2 -DNDEBUG
endif

SRCS := test.c qspinlock.c
HDRS := $(shell find . -name "*.h" -printf "%P\n")

.PHONY: all
all: test-lse test-llsc

test-lse: $(SRCS) $(HDRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DCONFIG_ARM64_LSE_ATOMICS -o $@ $(SRCS)

test-llsc: $(SRCS) $(HDRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SRCS)

.PHONY: bench
bench: bench-spinlock bench-qspinlock

bench-spinlock: bench.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -march=armv8-a+lse -o $@ $^

bench-qspinlock: bench.c qspinlock.c $(HDRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DBENCH_QSPINLOCK -DCONFIG_ARM64_LSE_ATOMICS \
	    -o $@ bench.c qspinlock.c

.PHONY: clean
clean:
	rm -f test-lse test-llsc bench-spinlock bench-qspinlock
