PAPI_ROOT ?= $(HOME)/papi-7.0.1/install
URING_ROOT ?= $(HOME)/liburing/install

CPPFLAGS := -I$(PAPI_ROOT)/include -g -O2 -Wall
CXXFLAGS := -std=c++11
LDFLAGS := -L$(PAPI_ROOT)/lib
LDLIBS := -l:libpapi.a

.PHONY: all
all: server client

.PHONY: epoll
epoll: CPPFLAGS += -DWITH_EPOLL
epoll: all

.PHONY: uring
uring: CPPFLAGS += -DWITH_URING -I$(URING_ROOT)/include
uring: LDFLAGS += -L$(URING_ROOT)/lib
uring: LDLIBS += -l:liburing.a
uring: all

server: server.cc papi.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

client: client.cc papi.h uring.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -f server client
