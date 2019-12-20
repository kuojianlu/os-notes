HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)

ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib -lcsapp64
else
LDFLAGS += -L/usr/local/lib -l:libcsapp64.a
endif

LDFLAGS += -lpthread

all: echo_server

echo_server: echo_server.o
	gcc -o $@ $^ $(LDFLAGS)

echo_server.o:
	gcc -c echo_server.c

.PHONY: clean
clean:
	rm *.o echo_server