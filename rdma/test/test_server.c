#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SO_PORT 8080
#define SO_BUFFER_SIZE 1024

int server_fd, new_socket;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);
char buffer[SO_BUFFER_SIZE] = {0};

static int s_create_server();

static void s_close_server();

int s_create_server()
{
	// 소켓 파일 디스크립터 생성
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
                perror("socket failed");
                exit(EXIT_FAILURE);
        }

        // 소켓 옵션 설정
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt");
                close(server_fd);
                exit(EXIT_FAILURE);
        }

        // 주소와 포트 설정
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(SO_PORT);

        // 소켓에 주소와 포트 바인딩
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
                perror("bind failed");
                close(server_fd);
                exit(EXIT_FAILURE);
        }

        // 연결 대기
        if (listen(server_fd, 3) < 0) {
                perror("listen");
                close(server_fd);
                exit(EXIT_FAILURE);
        }

        printf("Listening on port %d\n", SO_PORT);

        // 클라이언트의 연결 수락
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
		perror("accept");
                close(server_fd);
                exit(EXIT_FAILURE);
        }

	return 0;
}

void s_close_server()
{
        // 소켓 종료
        close(new_socket);
        close(server_fd);
}

int s_recv()
{
	// 클라이언트로부터 데이터 수신
	int valread = read(new_socket, buffer, SO_BUFFER_SIZE);
	if (valread < 0) {
		perror("read");
		close(new_socket);
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// 수신한 데이터 출력
	printf("Received data: %s\n", buffer);

	return 0;
}


int main() {
//	int server_fd, new_socket;
//	struct sockaddr_in address;
//	int opt = 1;
//	int addrlen = sizeof(address);
//	char buffer[BUFFER_SIZE] = {0};

//	// 소켓 파일 디스크립터 생성
//	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//		perror("socket failed");
//		exit(EXIT_FAILURE);
//	}
//
//	// 소켓 옵션 설정
//	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//		perror("setsockopt");
//		close(server_fd);
//		exit(EXIT_FAILURE);
//	}
//
//	// 주소와 포트 설정
//	address.sin_family = AF_INET;
//	address.sin_addr.s_addr = INADDR_ANY;
//	address.sin_port = htons(PORT);
//
//	// 소켓에 주소와 포트 바인딩
//	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//		perror("bind failed");
//		close(server_fd);
//		exit(EXIT_FAILURE);
//	}
//
//	// 연결 대기
//	if (listen(server_fd, 3) < 0) {
//		perror("listen");
//		close(server_fd);
//		exit(EXIT_FAILURE);
//	}
//
//	printf("Listening on port %d\n", PORT);
//
//	// 클라이언트의 연결 수락
//	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
//		perror("accept");
//		close(server_fd);
//		exit(EXIT_FAILURE);
//	}

	if (s_create_server())
		printf("\n%s: Fail to create socket create\n", __func__);

	int valread;

	while(1){
		// 클라이언트로부터 데이터 수신
//		int valread = read(new_socket, buffer, BUFFER_SIZE);
		valread = read(new_socket, buffer, SO_BUFFER_SIZE);
		if (valread < 0) {
			perror("read");
			close(new_socket);
			close(server_fd);
			exit(EXIT_FAILURE);
		}

		// 수신한 데이터 출력
		printf("Received data: %s\n", buffer);

	}

	// 소켓 종료
	s_close_server();

	return 0;
}
