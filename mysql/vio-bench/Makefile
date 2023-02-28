.PHONY: all
all: vio-server vio-client

vio-%: vio-%.cc config.h
	${CXX} -o $@ $< -g -O2 -fno-omit-frame-pointer -Wall -pthread

.PHONY: clean
clean:
	rm -f vio-server vio-client
