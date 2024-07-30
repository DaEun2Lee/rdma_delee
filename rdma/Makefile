ONY: clean

CFLAGS  := -Wall -g -o
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread

APPS    := server client

all: ${APPS}


clean:
	rm -f ${APPS}
