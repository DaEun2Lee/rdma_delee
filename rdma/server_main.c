#include "rdma_server.h"


int main(){

	//Open result file
	FILE *fptr = fopen("execution_time_main.txt", "w");
	if (fptr == NULL) {
		printf("Error: Open file!\n");
		return 1;
	}

	double start_m = record_time_file(fptr, "Start rdma_sock_thread_init()");

	if(!rdma_sock_thread_init())
		return 0;

	double end_m = record_time_file(fptr, "End rdma_sock_thread_init()");
	execution_time(fptr, start_m, end_m, "Execution Time of rdma_sock_thread_init()");

	pthread_t sock_rdma_tid;

	if (pthread_create(&sock_rdma_tid, NULL, sock_rdma_thread, NULL)) {
                perror("Failed to create sock_rdma thread");
                exit(EXIT_FAILURE);
        }

	pthread_join(sock_rdma_tid, NULL);

	fclose(fptr);
	return 0;
}
