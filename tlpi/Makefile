CFLAGS += -std=c11 -O3 -pthread -Wno-unused-result

OBJS := fd open syscall-atomic append write-file clock file-buf tail fd-exec
OBJS += stdin-buf signal-safe signal-nodefer signal-syscall fork signal-sync
OBJS += zombie sched daemon syslog

all: ${OBJS}

clean:
	rm -f *.o ${OBJS}
