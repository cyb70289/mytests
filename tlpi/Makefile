CFLAGS += -std=c11 -O3 -pthread -Wno-unused-result
LDLIBS += -lrt

OBJS := fd open syscall-atomic append write-file clock file-buf tail fd-exec
OBJS += stdin-buf signal-safe signal-nodefer signal-syscall fork signal-sync
OBJS += zombie sched daemon syslog mmap pipe sock-un sock-un-fd sock-reuse
OBJS += shm client-server

all: ${OBJS}

clean:
	@rm -f *.o ${OBJS}
