.PHONY: clean

CFLAGS := -Wall -g -ggdb -Werror -DACN_RDMA_DEBUG -std=c11
CXXFLAGS := -Wall -g -ggdb -Werror -DACN_RDMA_DEBUG
LDLIBS := ${LDLIBS} -lrdmacm -libverbs -lpthread
CC := gcc
CXX := g++

#APPS := test_server test_client test_socket
APPS := test_socket

all: ${APPS}

#test_server: test_server.c
#	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS}
#
#test_client: test_client.c
#	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS}

test_socket: test_socket.c
	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS}

clean:
	rm -f ${APPS}
