#include "rdma_server.h"


int main(){
	pthread_t sock_rdma_tid;

	if (pthread_create(&sock_rdma_tid, NULL, sock_rdma_thread, NULL)) {
                perror("Failed to create sock_rdma thread");
                exit(EXIT_FAILURE);
        }

	pthread_join(sock_rdma_tid, NULL);

	return 0;
}
