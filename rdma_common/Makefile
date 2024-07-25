.PHONY: clean

CFLAGS  := -Wall -g
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread

APP := rdma-test

all: ${APP}


clean:
	rm -f ${APP}

