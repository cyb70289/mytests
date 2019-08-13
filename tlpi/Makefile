CFLAGS += -std=gnu11 -O3 -pthread -Wno-unused-result

OBJS := fd open syscall-atomic append write-file clock file-buf tail
OBJS += stdin-buf signal-safe signal-nodefer signal-syscall fork

all: ${OBJS}

clean:
	rm -f *.o ${OBJS}
