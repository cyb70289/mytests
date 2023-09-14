PAPI_ROOT ?= $(HOME)/papi-7.0.1/install
CPPFLAGS := -I$(PAPI_ROOT)/include -g -O2 -Wall
CXXFLAGS := -std=c++11
LDFLAGS := -L$(PAPI_ROOT)/lib
LDLIBS := -lpapi

.PHONY: all
all: server client

.PHONY: epoll
epoll: CPPFLAGS += -DWITH_EPOLL
epoll: all

server: papi.h
client: papi.h

.PHONY: clean
clean:
	rm -f server client
