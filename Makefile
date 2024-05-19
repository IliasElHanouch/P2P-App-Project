BIN_FILES = servidor
CC = gcc

CFLAGS = -g -Wall -I/usr/include/tirpc
LDFLAGS = -L/usr/lib/tirpc
LDLIBS = -lpthread -lnsl -ltirpc -lm

all: $(BIN_FILES)
.PHONY: all

servidor: servidor.o message.o rpc_management_clnt.o rpc_management_xdr.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

rpc_management_clnt.c rpc_management_clnt.h rpc_management_xdr.c rpc_management_xdr.h: rpc_management.x
	rpcgen -M rpc_management.x

clean:
	rm -f $(BIN_FILES) *.o rpc_management_clnt.* rpc_management_xdr.*
	rm -rf usuarios-conectados/*
	rm -rf usuarios-registrados/*
	rm -rf publicaciones/*

.PHONY: clean
