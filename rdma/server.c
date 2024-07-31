#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <rdma/rdma_cma.h>
#include "socket.h"		//made by delee

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

const int BUFFER_SIZE = 1024;
//@delee
//const char *DEFAULT_PORT = "12345";
const int DEFAULT_PORT = 12345;
//extern char sock_rdma_buffer[BUFFER_SIZE] = {0};
//extern char rdma_sock_buffer[BUFFER_SIZE] = {0};
//// Atomic flag to indicate if the buffer has changed
//extern atomic_bool sock_rdma_buffer_changed = false;
//extern atomic_bool rdma_sock_buffer_changed = false;

struct context {
	struct ibv_context *ctx;
	struct ibv_pd *pd;
	struct ibv_cq *cq;
	struct ibv_comp_channel *comp_channel;

	pthread_t cq_poller_thread;
};

struct connection {
	struct ibv_qp *qp;

	struct ibv_mr *recv_mr;
	struct ibv_mr *send_mr;

	char *recv_region;
	char *send_region;
};

static void die(const char *reason);

static void build_context(struct ibv_context *verbs);
static void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
static void * poll_cq(void *);
static void post_receives(struct connection *conn);
static void register_memory(struct connection *conn);

static void on_completion(struct ibv_wc *wc);
static int on_connect_request(struct rdma_cm_id *id);
static int on_connection(void *context);
//static int on_disconnect(struct rdma_cm_id *id);
static int on_event(struct rdma_cm_event *event);

static struct context *s_ctx = NULL;

struct socket_thread *s_info = NULL;
 struct socket_thread *c_info = NULL;

int main(int argc, char **argv)
{
//	//Open result file
//	fptr = fopen("execution_time.txt", "w");
//	if (fptr == NULL) {
//		printf("Error: Open file!\n");
//		return 1;
//	}

	//@delee
	//This part is socket <-> rdma
        //Create Sock-Server
	s_info = server_thread_init();

        if(s_info == NULL)
                pthread_exit(NULL);

        if(!socket_listen(s_info))
                pthread_exit(NULL);

//      if(socket_connect_request(s_info))
//              pthread_exit(NULL);
//      printf("%s: Create Sock-Server\n", __func__);

        //Create Sock-Client
	c_info = client_thread_init();

        if(c_info == NULL)
                pthread_exit(NULL);
        if(!socket_connect(c_info))
                pthread_exit(NULL);
        printf("%s: Create Sock-Client\n", __func__);

//Set Address for RDMA
#if _USE_IPV6
	struct sockaddr_in6 addr;
#else
	struct sockaddr_in addr;
#endif
	struct rdma_cm_event *event = NULL;
	struct rdma_cm_id *listener = NULL;
	struct rdma_event_channel *ec = NULL;

	memset(&addr, 0, sizeof(addr));
#if _USE_IPV6
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(DEFAULT_PORT);
#else
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DEFAULT_PORT);
#endif

	TEST_Z(ec = rdma_create_event_channel());
	TEST_NZ(rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP));
	TEST_NZ(rdma_bind_addr(listener, (struct sockaddr *)&addr));
	TEST_NZ(rdma_listen(listener, 10)); /* backlog=10 is arbitrary */

	printf("%s: RDMA listening on port %d.\n", __func__, DEFAULT_PORT);

//	//Input message                         (Client-side)
//        char *message = (char*)malloc(SO_BUFFER_SIZE);
//        FILE *file;
//        file = fopen("request.txt", "r");
//        if (file == NULL) {
//                perror("Failed to open file");
//        }
//        size_t bytesRead = fread(message, sizeof(char), SO_BUFFER_SIZE - 1, file);
//        message[bytesRead] = '\0';
//        fclose(file);

	while(true){
		//Socket
		sleep(2);
//		if(s_ctx->send_region != NULL){
//                        socket_send_message(c_info, s_ctx->send_region);
                        if(socket_connect_request(s_info))
                                break;
                        printf("%s: Create Sock-Server\n", __func__);
//                }

		//Receive data from Client      (Server-side)
                int valread = read(s_info->socket, s_info->buffer, SO_BUFFER_SIZE);
                if (valread < 0) {
                        perror("read");
                        socket_end(s_info);
                        pthread_exit(NULL);
                }

                //Print received data           (Server-side)
                printf("%s: Server received data: %s\n", __func__, s_info->buffer);

		//RDMA
		if(rdma_get_cm_event(ec, &event) == 0) {
			//@delee
			//TODO
			// related disconnect
			struct rdma_cm_event event_copy;

			memcpy(&event_copy, event, sizeof(*event));
			rdma_ack_cm_event(event);

			if (on_event(&event_copy))
				break;
			}
	}

	rdma_destroy_id(listener);
	rdma_destroy_event_channel(ec);
	socket_end(s_info);
        socket_end(c_info);

	return 0;
}

void die(const char *reason)
{
	fprintf(stderr, "%s\n", reason);
	exit(EXIT_FAILURE);
}

void build_context(struct ibv_context *verbs)
{
	if (s_ctx) {
		if (s_ctx->ctx != verbs)
			die("cannot handle events in more than one context.");

		return;
	}

	s_ctx = (struct context *)malloc(sizeof(struct context));

	s_ctx->ctx = verbs;

	TEST_Z(s_ctx->pd = ibv_alloc_pd(s_ctx->ctx));
	TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
	TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=10 is arbitrary */
	TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));

	TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));
}

void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
	memset(qp_attr, 0, sizeof(*qp_attr));

	qp_attr->send_cq = s_ctx->cq;
	qp_attr->recv_cq = s_ctx->cq;
	qp_attr->qp_type = IBV_QPT_RC;

	qp_attr->cap.max_send_wr = 10;
	qp_attr->cap.max_recv_wr = 10;
	qp_attr->cap.max_send_sge = 1;
	qp_attr->cap.max_recv_sge = 1;
}

void * poll_cq(void *ctx)
{
	struct ibv_cq *cq;
	struct ibv_wc wc;

	while (1) {
		TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
		ibv_ack_cq_events(cq, 1);
		TEST_NZ(ibv_req_notify_cq(cq, 0));

		while (ibv_poll_cq(cq, 1, &wc))
			on_completion(&wc);
	}

	return NULL;
}

void post_receives(struct connection *conn)
{
	struct ibv_recv_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	wr.wr_id = (uintptr_t)conn;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;

	sge.addr = (uintptr_t)conn->recv_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = conn->recv_mr->lkey;

	TEST_NZ(ibv_post_recv(conn->qp, &wr, &bad_wr));
}

void register_memory(struct connection *conn)
{
	conn->send_region = (char*)malloc(BUFFER_SIZE);
	conn->recv_region = (char*)malloc(BUFFER_SIZE);

	TEST_Z(conn->send_mr = ibv_reg_mr(
		s_ctx->pd,
		conn->send_region,
		BUFFER_SIZE,
		0));

	TEST_Z(conn->recv_mr = ibv_reg_mr(
		s_ctx->pd,
		conn->recv_region,
		BUFFER_SIZE,
		IBV_ACCESS_LOCAL_WRITE));
}

void on_completion(struct ibv_wc *wc)
{
	if (wc->status != IBV_WC_SUCCESS)
		die("on_completion: status is not IBV_WC_SUCCESS.");

	if (wc->opcode & IBV_WC_RECV) {
		struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;

		printf("%s: RDMA-Server received message: \n%s\n", __func__, conn->recv_region);

		//@delee
		//TODO
		//Change func
		//send rdma -> socket
		socket_send_message(c_info, conn->recv_region);
//		rdma_sock_buffer = conn->recv_region;
////		//This code inform that server have received data from rdma using atomic.
//		memcpy(rdma_sock_buffer, conn->recv_region, BUFFER_SIZE);
//		atomic_store(&rdma_sock_buffer_changed, true);
//		start = record_time_file(fptr, "Start of sending data using RDMA"); 

//		while(1){
//			if(rdma_sock_buffer.lock == false){
////				start = record_time_file(fptr, (char*)"Start of sending data using Sock");
//				rdma_sock_buffer.lock = true;
//				//TODO
//				// data 이어쓰기 
//				memcpy(rdma_sock_buffer.data, conn->recv_region, BUFFER_SIZE);
//	//			rdma_sock_buffer.change = true;
//				rdma_sock_buffer.length = BUFFER_SIZE;
//				rdma_sock_buffer.lock = false;
//				break;
//			} else
//				sleep(1);
//		}

	} else if (wc->opcode == IBV_WC_SEND) {
		printf("send completed successfully.\n");
//		end = record_time_file(fptr, "End of sending data using Sock");
//		execution_time(fptr, start, end, "Sending data using Sock");
		//@delee
//		// This code inform that server have received data from rdma using atomic.
//		atomic_store(&rdma_sock_buffer_changed, false);
//		while(1){
//			if(rdma_sock_buffer.lock == false){
//				rdma_sock_buffer.lock = true;
////				rdma_sock_buffer.data = {0};
//				memset(rdma_sock_buffer.data, 0, BUFFER_SIZE);
//				rdma_sock_buffer.length = 0;
//				rdma_sock_buffer.lock = false;
////				end = record_time_file(fptrf "End of sending data using RDMA");
//				break;
//			} else
//				sleep(1);
//		}
	}
}

int on_connect_request(struct rdma_cm_id *id)
{
  struct ibv_qp_init_attr qp_attr;
  struct rdma_conn_param cm_params;
  struct connection *conn;

  printf("received connection request.\n");

  build_context(id->verbs);
  build_qp_attr(&qp_attr);

  TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

  id->context = conn = (struct connection *)malloc(sizeof(struct connection));
  conn->qp = id->qp;

  register_memory(conn);
  post_receives(conn);

  memset(&cm_params, 0, sizeof(cm_params));
  TEST_NZ(rdma_accept(id, &cm_params));

  return 0;
}

int on_connection(void *context)
{
	struct connection *conn = (struct connection *)context;
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

//	snprintf(conn->send_region, BUFFER_SIZE, "message from passive/server side with pid %d", getpid());

	//@delee
	//TODO
	memcpy(conn->send_region, s_info->buffer, BUFFER_SIZE);
//	strcpy(conn->send_region, "Send DATA using RDMA send.");
//	//This code inform that server have received data from sock using atomic.
//	if(atomic_load(&sock_rdma_buffer_changed)){
////		strcpy(conn->send_region, sock_rdma_buffer);
//		memcpy(conn->send_region, sock_rdma_buffer, BUFFER_SIZE);
//		atomic_store(&sock_rdma_buffer_changed, false);
//	}
//	//sock -> rdma
//	while(sock_rdma_buffer.length > 0){
//		if(sock_rdma_buffer.lock == false){
//			sock_rdma_buffer.lock = true;
//			memcpy(conn->send_region, sock_rdma_buffer.data, BUFFER_SIZE);
//			printf("%s: RDMA-Server send data: \n%s\n", __func__, sock_rdma_buffer.data);
////			sock_rdma_buffer.data = {0};
//			memset(rdma_sock_buffer.data, 0, BUFFER_SIZE);
////			sock_rdma_buffer.change = false;
//			sock_rdma_buffer.length = 0;
//			sock_rdma_buffer.lock = false;
//			break;
//		} else
//			sleep(1);
//        }

	printf("%s: connected. posting send...\n", __func__);

	memset(&wr, 0, sizeof(wr));

	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;

	sge.addr = (uintptr_t)conn->send_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = conn->send_mr->lkey;

	TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));

	return 0;
}

//int on_disconnect(struct rdma_cm_id *id)
//{
//	struct connection *conn = (struct connection *)id->context;
//
//	printf("peer disconnected.\n");
//
//	rdma_destroy_qp(id);
//
//	ibv_dereg_mr(conn->send_mr);
//	ibv_dereg_mr(conn->recv_mr);
//
//	free(conn->send_region);
//	free(conn->recv_region);
//
//	free(conn);
//
//	rdma_destroy_id(id);
//
//	return 0;
//}

int on_event(struct rdma_cm_event *event)
{
	int r = 0;

	if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
		r = on_connect_request(event->id);
	else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
		r = on_connection(event->id->context);
	else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
//		r = on_disconnect(event->id);
		r= 0;
	else
		die("on_event: unknown event.");

	return r;
}

