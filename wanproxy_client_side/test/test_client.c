#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SO_PORT 8080
#define SO_SERVER_IP "127.0.0.1"  // 서버의 IP 주소
//#define MESSAGE "Hello, Server!"
const int  BUFFER_SIZE = 1024;

int sock = 0;
struct sockaddr_in serv_addr;
char buffer[BUFFER_SIZE] = {0};

static int s_create();
static int s_send(char *snd_data);
static int s_connection();
static void s_close();

//int socket_send(char *snd_data)
//{
////	int sock = 0;
////	struct sockaddr_in serv_addr;
////	char buffer[1024] = {0};
//
//	// 소켓 생성
//	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//		printf("\n Socket creation error \n");
//		return -1;
//	}
//
//	serv_addr.sin_family = AF_INET;
//	serv_addr.sin_port = htons(SO_PORT);
//
//	// IP 주소 변환 및 설정
//	if (inet_pton(AF_INET, SO_SERVER_IP, &serv_addr.sin_addr) <= 0) {
//		printf("\nInvalid address/ Address not supported \n");
//		return -1;
//	}
//
//	// 서버에 연결
//	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
//		printf("\nConnection Failed \n");
//		return -1;
//	}
//
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
//	// 소켓 닫기
//	close(sock);
//
//	return 0;
//}

int s_create()
{
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

	return 0;
}

int s_connection()
{
	// 서버에 연결
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}
	return 0;
}

int s_send(char *snd_data)
{
	// 메시지 전송
	send(sock, snd_data, strlen(snd_data), 0);
	printf("Message sent to server: %s\n", snd_data);

	// 서버로부터의 응답 읽기 (선택 사항)
	int valread = read(sock, buffer, 1024);
	if (valread > 0) {
		printf("Server response: %s\n", buffer);
	}

	return 0;
}

void s_close()
{
	// 소켓 닫기
        close(sock);
}

int main() {
//	char *data = (char*)malloc(BUFFER_SIZE);
//
//	strcpy(data, "Hello, Server!");
//	socket_send(data);

	if (s_create())
		printf("\n%s: Fail to create socket create\n", __func__);

	if (s_connection())
		printf("\n%s: Fail to connect Server\n", __func__);

	char *data = (char*)malloc(BUFFER_SIZE);
        strcpy(data, "Hello, Server!");

	int i = 0;
	while(1){
		if(i >= 10)
			break;
		i++;
		strcpy(data, "Hello, Server!");
		if (s_send(data))
			printf("\n%s: Fail to send data to Server\n", __func__);
	}

	s_close();

	free(data);

	return 0;
}
