# Makefile for server daemon

CC= gcc
CFLAGS= -O3
LDFLAGS= -pthread
RM= /bin/rm -f

SRV_OBJS=rgpu_server.o monitor.o
CLI_OBJS=rgpu_client.o

all: rgpu_server rgpu_client

rgpu_server:	$(SRV_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(SRV_OBJS)

rgpu_client:	$(CLI_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(CLI_OBJS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean: 
	$(RM) *.o rgpu_client rgpu_server


