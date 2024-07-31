.PHONY: clean

CFLAGS := -Wall -g -ggdb -Werror -DACN_RDMA_DEBUG -std=c11
CXXFLAGS := -Wall -g -ggdb -Werror -DACN_RDMA_DEBUG
LDLIBS := ${LDLIBS} -lrdmacm -libverbs -lpthread
CC := gcc
CXX := g++

#APPS := test_server test_client test_socket
#APPS := test_socket
APPS := server client

#all: ${APPS}

client: client.c
	${CC} ${CXXFLAGS} -o $@ $^ ${LDLIBS}

server: server_main.c rdma_server.c socket.c
	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS}

clean:
	rm -f ${APPS}
