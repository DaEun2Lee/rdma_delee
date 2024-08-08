#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include "rdma_client.h"

// 원래 socket 함수의 포인터
static int (*original_socket)(int domain, int type, int protocol) = NULL;

// 후킹된 socket 함수
int socket(int domain, int type, int protocol) {
    // 원래 socket 함수를 로드
    if (!original_socket) {
        original_socket = (int (*)(int, int, int))dlsym(RTLD_NEXT, "socket");
        if (!original_socket) {
            fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
            exit(EXIT_FAILURE);
        }
    }

    // rdma_sock_linker 함수 호출
    return rdma_sock_linker(domain, type, protocol);
}
