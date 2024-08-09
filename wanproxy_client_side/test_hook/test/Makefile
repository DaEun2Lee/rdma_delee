.PHONY: clean

# 컴파일 플래그 설정
CFLAGS := -Wall -g -ggdb -Werror -DACN_RDMA_DEBUG -std=c11 -I../src
LDLIBS := ${LDLIBS} -lrdmacm -libverbs -lpthread
CC := gcc

# 타겟 애플리케이션
APPS := client

# 모든 타겟을 빌드
all: ${APPS}

# client 타겟 정의
client: client_main.o rdma_client.o
	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS}

# 소스 파일을 오브젝트 파일로 컴파일
client_main.o: client_main.c
	${CC} ${CFLAGS} -c $< -o $@

rdma_client.o: ../src/rdma_client.c
	${CC} ${CFLAGS} -c $< -o $@

# 빌드된 파일 삭제
clean:
	rm -f ${APPS} *.o
