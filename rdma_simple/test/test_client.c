#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SO_PORT 8080
#define SO_SERVER_IP "127.0.0.1"  // 서버의 IP 주소
//#define MESSAGE "Hello, Server!"
const int  BUFFER_SIZE = 1024;

int socket_send(char *snd_data)
{
	int sock = 0;
	struct sockaddr_in serv_addr;
	char buffer[1024] = {0};

	// 소켓 생성
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SO_PORT);

	// IP 주소 변환 및 설정
	if (inet_pton(AF_INET, SO_SERVER_IP, &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	// 서버에 연결
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}

	// 메시지 전송
	send(sock, snd_data, strlen(snd_data), 0);
	printf("Message sent to server: %s\n", snd_data);

	// 서버로부터의 응답 읽기 (선택 사항)
	int valread = read(sock, buffer, 1024);
	if (valread > 0) {
		printf("Server response: %s\n", buffer);
	}

	// 소켓 닫기
	close(sock);
	
	return 0;
}

int main() {
	char *data = (char*)malloc(BUFFER_SIZE);

	strcpy(data, "Hello, Server!");
	socket_send(data);

	free(data);

	return 0;
}
