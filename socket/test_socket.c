#include "test_socket.h"

// Atomic flag to indicate if the buffer has changed
atomic_bool sock_rdma_buffer_changed = false;
atomic_bool rdma_sock_buffer_changed = false;

//bool server_thread_init(struct socket_thread *s_info)
struct socket_thread * server_thread_init()
{
	struct socket_thread *s_info;
	s_info = malloc(sizeof(struct socket_thread));

	//Init Socket
	s_info->opt = 1;
	s_info->addrlen = sizeof(s_info->address);
//	s_info->buffer = {0};
	memset(s_info->buffer, 0 , SO_BUFFER_SIZE);

	// 소켓 파일 디스크립터 생성
	if ((s_info->server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
        }

	// Setting Socket option
//	s_info->server_fd = -12345;
	if (setsockopt(s_info->server_fd, SOL_SOCKET, SO_REUSEADDR, &(s_info->opt), sizeof(s_info->opt))) {
		perror("setsockopt");
		close(s_info->server_fd);
//		pthread_exit(NULL);
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
//		pthread_exit(NULL);
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
	// 클라이언트의 연결 수락
	if ((s_info->new_socket = accept(s_info->server_fd, (struct sockaddr *)&(s_info->address), (socklen_t*)&(s_info->addrlen))) < 0) {
                perror("accept");
		close(s_info->server_fd);
		return false;
        }
	printf("%s: Socket-Server connected Socket-Client\n", __func__);
	return true;
}

void socket_end(struct socket_thread *s_info)
{
	// 소켓 종료
	close(s_info->new_socket);
	close(s_info->server_fd);
}

void *server_thread(void *arg)
{
	struct socket_thread *s_info = server_thread_init();

//	if(!server_thread_init(s_info))
//		pthread_exit(NULL);

	if(s_info == NULL)
		pthread_exit(NULL);

	if(!socket_listen(s_info))
		pthread_exit(NULL);

	if(socket_connect_request(s_info))
		pthread_exit(NULL);

	while(1){
		// 클라이언트로부터 데이터 수신
		int valread = read(s_info->new_socket, s_info->buffer, SO_BUFFER_SIZE);
		if (valread < 0) {
			perror("read");
			socket_end(s_info);
			pthread_exit(NULL);
		}

		// 수신한 데이터 출력
		printf("%s: Server received data: %s\n", __func__, s_info->buffer);
		//@delee
		//TODO
		//RDMA
//		sock_rdma_buffer = buffer;
//		memcpy(sock_rdma_buffer, buffer, SO_BUFFER_SIZE);
//		atomic_store(&sock_rdma_buffer_changed, true);

	}

	// 소켓 종료
	socket_end(s_info);

	pthread_exit(NULL);
}

void *client_thread(void *arg) {
	sleep(3);  // 서버가 준비될 시간을 확보
	int sock = 0;
	struct sockaddr_in serv_addr;

	char *message = (char*)malloc(SO_BUFFER_SIZE);
//	strcpy(message, "Hello from client!");
        FILE *file;
        file = fopen("request.txt", "r");
        if (file == NULL) {
                perror("Failed to open file");
        }
        size_t bytesRead = fread(message, sizeof(char), SO_BUFFER_SIZE - 1, file);
        message[bytesRead] = '\0';
        fclose(file);


	char buffer[SO_BUFFER_SIZE] = {0};

	// 소켓 생성
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		pthread_exit(NULL);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SO_SND_PORT);

	// IP 주소 변환 및 설정
	if (inet_pton(AF_INET, SO_IP, &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		close(sock);
		pthread_exit(NULL);
	}

	// 서버에 연결
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		close(sock);
		pthread_exit(NULL);
	}

//	while(1){

//		if(atomic_load(&rdma_sock_buffer_changed)){
//			strcpy(message, rdma_sock_buffer);
//			atomic_store(&rdma_sock_buffer_changed, false);
//		}

		// 메시지 전송
		send(sock, message, strlen(message), 0);
		printf("Client sent message: %s\n", message);

		// 서버로부터의 응답 읽기 (선택 사항)
		int valread = read(sock, buffer, SO_BUFFER_SIZE);
		if (valread > 0) {
			printf("Client received response: %s\n", buffer);
		}
//	}

	// 소켓 닫기
	close(sock);
	pthread_exit(NULL);
}
