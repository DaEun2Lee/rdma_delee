#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
//#define BUFFER_SIZE 1024

//@delee
#define SO_IP "127.0.0.1" //보내려는 주소
//#define SND_PORT 8080
//#define RCV_PORT 8081
#define SO_BUFFER_SIZE 1024
char sock_rdma_buffer[SO_BUFFER_SIZE] = {0};
char rdma_sock_buffer[SO_BUFFER_SIZE] = {0};
// Atomic flag to indicate if the buffer has changed
atomic_bool sock_rdma_buffer_changed = false;
atomic_bool rdma_sock_buffer_changed = false;

//void send_rdma_sock(char *snd_data)
//{
//	// 메시지 전송
//	send(sock, snd_data, strlen(snd_data), 0);
//	printf("Message sent to server: %s\n", snd_data);
//
//	// 서버로부터의 응답 읽기 (선택 사항)
//	int valread = read(sock, buffer, 1024);
//	if (valread > 0) {
//		printf("Server response: %s\n", buffer);
//	}
//
//	return 0;
//}

void *server_thread(void *arg) {
	int server_fd, new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[SO_BUFFER_SIZE] = {0};

	// 소켓 파일 디스크립터 생성
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// 소켓 옵션 설정
//#ifdef SO_REUSEPORT
//	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//#else
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//#endif
		perror("setsockopt");
		close(server_fd);
		pthread_exit(NULL);
	}

	// 주소와 포트 설정
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// 소켓에 주소와 포트 바인딩
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		close(server_fd);
		pthread_exit(NULL);
	}

	// 연결 대기
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		close(server_fd);
		pthread_exit(NULL);
	}

	printf("Server listening on port %d\n", PORT);

	// 클라이언트의 연결 수락
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
		perror("accept");
		close(server_fd);
		pthread_exit(NULL);
	}

	while(1){
		// 클라이언트로부터 데이터 수신
		int valread = read(new_socket, buffer, SO_BUFFER_SIZE);
		if (valread < 0) {
			perror("read");
			close(new_socket);
			close(server_fd);
			pthread_exit(NULL);
		}

		// 수신한 데이터 출력
		printf("Server received data: %s\n", buffer);
		//@delee
		//TODO
		//RDMA
//		sock_rdma_buffer = buffer;
		memcpy(sock_rdma_buffer, buffer, SO_BUFFER_SIZE);
		atomic_store(&sock_rdma_buffer_changed, true);

	}

	// 소켓 종료
	close(new_socket);
	close(server_fd);
	pthread_exit(NULL);
}

void *client_thread(void *arg) {
	sleep(3);  // 서버가 준비될 시간을 확보
	int sock = 0;
	struct sockaddr_in serv_addr;

	char *message = (char*)malloc(SO_BUFFER_SIZE);
	strcpy(message, "Hello from client!");

	char buffer[SO_BUFFER_SIZE] = {0};

	// 소켓 생성
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		pthread_exit(NULL);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

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

	while(1){

		if(atomic_load(&rdma_sock_buffer_changed)){
			strcpy(message, rdma_sock_buffer);
			atomic_store(&rdma_sock_buffer_changed, false);
		}

		// 메시지 전송
		send(sock, message, strlen(message), 0);
		printf("Client sent message: %s\n", message);

		// 서버로부터의 응답 읽기 (선택 사항)
		int valread = read(sock, buffer, SO_BUFFER_SIZE);
		if (valread > 0) {
			printf("Client received response: %s\n", buffer);
		}
	}

	// 소켓 닫기
	close(sock);
	pthread_exit(NULL);
}
//
//int main() {
//	pthread_t server_tid, client_tid;
//
//	// 서버 스레드 생성
//	if (pthread_create(&server_tid, NULL, server_thread, NULL) != 0) {
//	        perror("Failed to create server thread");
//	exit(EXIT_FAILURE);
//	}
//
//	// 클라이언트 스레드 생성
//	if (pthread_create(&client_tid, NULL, client_thread, NULL) != 0) {
//		perror("Failed to create client thread");
//	exit(EXIT_FAILURE);
//	}
//
//	// 스레드가 종료될 때까지 대기
//	pthread_join(server_tid, NULL);
//	pthread_join(client_tid, NULL);
//
//	return 0;
//}
