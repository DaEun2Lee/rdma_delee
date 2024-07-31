#include "test_socket.h"

//bool server_thread_init(struct socket_thread *s_info)
struct socket_thread * server_thread_init()
{
	struct socket_thread *s_info;
	s_info = malloc(sizeof(struct socket_thread));

	//Init Socket
	s_info->opt = 1;
	s_info->addrlen = sizeof(s_info->address);
	memset(s_info->buffer, 0 , SO_BUFFER_SIZE);

	//Create Socket File descriptor
	if ((s_info->server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
        }

	// Setting Socket option
	if (setsockopt(s_info->server_fd, SOL_SOCKET, SO_REUSEADDR, &(s_info->opt), sizeof(s_info->opt))) {
		perror("setsockopt");
		close(s_info->server_fd);
		return NULL;
	}

	//Setting IP and Port
	s_info->address.sin_family = AF_INET;
	s_info->address.sin_addr.s_addr = INADDR_ANY;
	s_info->address.sin_port = htons(SO_RCV_PORT);

	//Bind IP and Port to Socket
	if (bind(s_info->server_fd, (struct sockaddr *)&s_info->address, sizeof(s_info->address)) < 0) {
		perror("bind failed");
		close(s_info->server_fd);
		return NULL;
	}

	return s_info;
}

bool socket_listen(struct socket_thread *s_info)
{
	if (listen(s_info->server_fd, 3) < 0) {
		perror("listen");
		close(s_info->server_fd);
		return false;
	}

	printf("%s: Server listening on port %d\n", __func__, SO_RCV_PORT);
	return true;
}
bool socket_connect_request(struct socket_thread *s_info)
{
	//Accept connection request from client
	if ((s_info->socket = accept(s_info->server_fd, (struct sockaddr *)&(s_info->address), (socklen_t*)&(s_info->addrlen))) < 0) {
                perror("accept");
		close(s_info->server_fd);
		return false;
        }
	printf("%s: Socket-Server connected Socket-Client\n", __func__);
	return true;
}

void socket_end(struct socket_thread *s_info)
{
	//Close Socket
	close(s_info->socket);
	close(s_info->server_fd);
}

void *server_thread(void *arg)
{
	struct socket_thread *s_info = server_thread_init();

	if(s_info == NULL)
		pthread_exit(NULL);

	if(!socket_listen(s_info))
		pthread_exit(NULL);

	if(socket_connect_request(s_info))
		pthread_exit(NULL);

	while(1){
		//Receive data from Client 
		int valread = read(s_info->socket, s_info->buffer, SO_BUFFER_SIZE);
		if (valread < 0) {
			perror("read");
			socket_end(s_info);
			pthread_exit(NULL);
		}

		//Print received data
		printf("%s: Server received data: %s\n", __func__, s_info->buffer);
		//@delee
		//TODO
		//RDMA
//		sock_rdma_buffer = buffer;
//		memcpy(sock_rdma_buffer, buffer, SO_BUFFER_SIZE);
//		atomic_store(&sock_rdma_buffer_changed, true);

	}

	socket_end(s_info);

	pthread_exit(NULL);
}

struct socket_thread * client_thread_init()
{
	struct socket_thread *c_info;
	c_info = malloc(sizeof(struct socket_thread));

	//Init Socket
	c_info->socket = 0;
	memset(c_info->buffer, 0 , SO_BUFFER_SIZE);

	//Create Socket
	if ((c_info->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n%s: Socket creation error \n", __func__);
		return NULL;
        }

	c_info->address.sin_family = AF_INET;
        c_info->address.sin_port = htons(SO_SND_PORT);

        // Setting IP
        if (inet_pton(AF_INET, SO_IP, &c_info->address.sin_addr) <= 0) {
		printf("\n%s: Invalid address/ Address not supported \n", __func__);
		close(c_info->socket);
		return NULL;
        }

	return c_info;
}

bool socket_connect(struct socket_thread *c_info)
{
	//Connect to Server
	if (connect(c_info->socket, (struct sockaddr *)&c_info->address, sizeof(c_info->address)) < 0) {
		printf("\n%s: Connection Failed \n", __func__);
		close(c_info->socket);
		return false;
        }
	return true;
}

void socket_send_message(struct socket_thread *c_info, char *message)
{
	//Send Message
	send(c_info->socket, message, strlen(message), 0);
	printf("%s: Client sent message: %s\n", __func__, message);

	// Read response from Server (Option)
	//TODO
	int valread = read(c_info->socket, c_info->buffer, SO_BUFFER_SIZE);
	if (valread > 0) {
		printf("%s: Client received response: %s\n", __func__,c_info->buffer);
	}
}

void *client_thread(void *arg) {

	sleep(3);  	//Waiting for servers to be ready

	struct socket_thread *c_info = client_thread_init();

	if(c_info == NULL)
		pthread_exit(NULL);

	socket_connect(c_info);

	//Input message
	char *message = (char*)malloc(SO_BUFFER_SIZE);
        FILE *file;
        file = fopen("request.txt", "r");
        if (file == NULL) {
                perror("Failed to open file");
        }
        size_t bytesRead = fread(message, sizeof(char), SO_BUFFER_SIZE - 1, file);
        message[bytesRead] = '\0';
        fclose(file);

	socket_send_message(c_info, message);

	socket_end(c_info);

	pthread_exit(NULL);
}
