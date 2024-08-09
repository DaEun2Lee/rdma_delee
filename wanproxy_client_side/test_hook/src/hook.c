#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "rdma_client.h"

// 원래의 socket() 함수를 가리킬 함수 포인터
//int (*original_socket)(int domain, int type, int protocol) = NULL;
static int (*original_connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen) = NULL;

extern int rdma_sock_linker();

// 사용자 정의 함수
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	if (!original_connect) {
		// 원래의 socket() 함수 주소를 가져옵니다.
		original_connect = dlsym(RTLD_NEXT, "connect");
	}
	printf("[ :::::::::::::: Start of connect Hook :::::::::::::: ]\n");
	// Log the hook
	printf("HOOK: connect hooked!\n");
	printf("rdma_sock_linker\n");
	// 사용자 정의 함수 호출
	rdma_sock_linker();
	printf("After rdma_sock_linker\n");

	int ret = original_connect(sockfd, addr, addrlen);
	// Log the result
    if (ret == 0) {
        printf("HOOK: Connection successful.\n");
    } else {
        // Check the errno value
        if (errno == EINPROGRESS) {
            printf("HOOK: Connection in progress (non-blocking).\n");
        } else {
            printf("HOOK: Connection failed, errno: %d.\n", errno);
        }
    }

    printf("[ :::::::::::::: End of connect Hook :::::::::::::: ]\n");

    return ret;
}
