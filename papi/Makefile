PAPI_ROOT ?= $(HOME)/papi-7.0.1/install
CPPFLAGS := -I$(PAPI_ROOT)/include -g -O2 -Wall
LDFLAGS := -L$(PAPI_ROOT)/lib
LDLIBS := -lpapi

.PHONY: all
all: server client

epoll: CPPFLAGS += -DWITH_EPOLL
epoll: all

.PHONY: clean
clean:
	rm -f server client
