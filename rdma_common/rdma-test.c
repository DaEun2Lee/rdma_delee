#include "rdma_client.h"
#include "rdma_server.h"

pthread_t s_handler;
pthread_t c_handler;

int main()
{
	pthread_create(&s_handler, NULL, rdma_server, NULL);
//	pthread_create(&c_handler, NULL, rdma_client, NULL);

	pthread_join(s_handler, NULL);
//	pthread_join(c_handler, NULL);
	return 0;
}
