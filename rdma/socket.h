#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SO_IP "10.0.0.1"                //Dest IP
#define SO_SND_PORT 3300                //sock-> wanproxy
#define SO_RCV_PORT 8080                //sock -> rdma
#define SO_BUFFER_SIZE 1024

//char sock_rdma_buffer[SO_BUFFER_SIZE] = {0};
//char rdma_sock_buffer[SO_BUFFER_SIZE] = {0};

struct socket_thread {
	int server_fd;
	int socket; 			//new_socket;
	struct sockaddr_in address;
	int opt;			//1
	int addrlen;			//addrlen = sizeof(address);
	char buffer[SO_BUFFER_SIZE];	//= {0}
};

//bool server_thread_init(struct socket_thread *s_info);
struct socket_thread * server_thread_init();
bool socket_listen(struct socket_thread *s_info);
bool socket_connect_request(struct socket_thread *s_info);
void socket_end(struct socket_thread *s_info);

struct socket_thread * client_thread_init();
bool socket_connect(struct socket_thread *c_info);
void socket_send_message(struct socket_thread *c_info, char *message);

void *server_thread();
void *client_thread();
void *socket_thread();
