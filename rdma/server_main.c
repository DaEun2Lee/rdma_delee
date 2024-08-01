#include "rdma_server.h"


int main(){

////	struct server_snic *snic;
////	snic = 

	if(!rdma_sock_thread_init())
		return 0;
//	printf("%s: Create snic\n", __func__);
//
//	//This part is socket <-> rdma
//	//Create Sock-Server
//	struct socket_thread *s_info = s_info;
//
//	if(s_info == NULL)
////		pthread_exit(NULL);
//		return 0;
//
//	if(!socket_listen(s_info))
////	pthread_exit(NULL);
//		return 0;
//
//	printf("%s: Create Sock-Server\n", __func__);
//
//
//	//Create Sock-Client
//	struct socket_thread *c_info = c_info;
//
//	if(c_info == NULL)
////		pthread_exit(NULL);
//		return 0;
//
//	if(!socket_connect(c_info))
////		pthread_exit(NULL);
//		return 0;
//
//	printf("%s: Create Sock-Client\n", __func__);
//
////	struct rdma_thread *r_info = rdma_init();

//	pthread_t rdma_sock_tid;
	pthread_t sock_rdma_tid;

//	if (pthread_create(&rdma_sock_tid, NULL, rdma_sock_thread, snic)) {
//	if (pthread_create(&rdma_sock_tid, NULL, rdma_sock_thread, NULL)) {
//		perror("Failed to create rdma_sock thread");
//		exit(EXIT_FAILURE);
//	}
//	if (pthread_create(&sock_rdma_tid, NULL, sock_rdma_thread, snic)) {
	if (pthread_create(&sock_rdma_tid, NULL, sock_rdma_thread, NULL)) {
                perror("Failed to create sock_rdma thread");
                exit(EXIT_FAILURE);
        }

//	pthread_join(rdma_sock_tid, NULL);
	pthread_join(sock_rdma_tid, NULL);


	return 0;
}
