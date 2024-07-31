#include "test_socket.h"


int main() {
//        pthread_t server_tid, client_tid;
	pthread_t socket_tid;
//
//        // 서버 스레드 생성
//        if (pthread_create(&server_tid, NULL, server_thread, NULL) != 0) {
//                perror("Failed to create server thread");
//        exit(EXIT_FAILURE);
//        }
//
//        // 클라이언트 스레드 생성
//        if (pthread_create(&client_tid, NULL, client_thread, NULL) != 0) {
//                perror("Failed to create client thread");
//        exit(EXIT_FAILURE);
//        }

	// 스레드 생성
        if (pthread_create(&socket_tid, NULL, socket_thread, NULL) != 0) {
                perror("Failed to create socket thread");
        exit(EXIT_FAILURE);
        }

        // 스레드가 종료될 때까지 대기
//        pthread_join(server_tid, NULL);
//        pthread_join(client_tid, NULL);
	pthread_join(socket_tid, NULL);
        return 0;
}
