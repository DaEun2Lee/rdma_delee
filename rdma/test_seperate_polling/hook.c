#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "rdma_client.h"

// 원래의 socket() 함수를 가리킬 함수 포인터
int (*original_socket)(int domain, int type, int protocol) = NULL;

int rdma_sock_linker();

// 사용자 정의 함수
int socket(int domain, int type, int protocol) {
    if (!original_socket) {
        // 원래의 socket() 함수 주소를 가져옵니다.
        original_socket = dlsym(RTLD_NEXT, "socket");
        if (!original_socket) {
            fprintf(stderr, "Error in dlsym: %s\n", dlerror());
            return -1;
        }
    }
	printf("rdma_sock_linker");
    // 사용자 정의 함수 호출
    rdma_sock_linker();

    // 원래의 socket() 함수 호출
    return original_socket(domain, type, protocol);
}
