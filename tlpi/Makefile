CFLAGS += -std=c11 -O3 -pthread -Wno-unused-result

OBJS := fd open syscall-atomic append write-file clock file-buf

all: ${OBJS}

clean:
	rm -f *.o ${OBJS}
