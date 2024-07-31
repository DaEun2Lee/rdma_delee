#include "rdma_server.h"

const int BUFFER_SIZE = 1024;
const int DEFAULT_PORT = 12345;

struct context *s_ctx = NULL;

struct server_snic *snic;

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

		printf("%s: RDMA-Server is received message: \n%s\n", __func__, conn->recv_region);

		//@delee
		//TODO
		//Change func
		//send rdma -> socket
//		socket_send_message(c_info, conn->recv_region);
//		memset(conn->recv_region, 0, BUFFER_SIZE);
	} else if (wc->opcode == IBV_WC_SEND) {
		printf("%s: sends completed successfully.\n", __func__);
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

	//@delee
	//TODO
	memcpy(conn->send_region, snic->s_info->buffer, BUFFER_SIZE);
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

struct rdma_thread * rdma_init()
{
	struct rdma_thread *r_info;
	r_info = malloc(sizeof(struct rdma_thread));

//Set Address for RDMA
	r_info->event = NULL;
	r_info->listener = NULL;
	r_info->ec = NULL;

        memset(&(snic->r_info->addr), 0, sizeof(snic->r_info->addr));

#if _USE_IPV6
        r_info->addr.sin6_family = AF_INET6;
        r_info->addr.sin6_port = htons(DEFAULT_PORT);
#else
        r_info->addr.sin_family = AF_INET;
        r_info->addr.sin_port = htons(DEFAULT_PORT);
#endif

	TEST_Z(r_info->ec = rdma_create_event_channel());
	TEST_NZ(rdma_create_id(r_info->ec, &r_info->listener, NULL, RDMA_PS_TCP));
	TEST_NZ(rdma_bind_addr(r_info->listener, (struct sockaddr *)&r_info->addr));
	TEST_NZ(rdma_listen(r_info->listener, 10)); /* backlog=10 is arbitrary */

//	freeaddrinfo(r_info->addr);
	printf("%s: RDMA listening on port %d.\n", __func__, DEFAULT_PORT);



	return r_info;
}
struct server_snic * rdma_sock_thread_init()
{
//	struct server_snic *snic;
	snic = malloc(sizeof(struct rdma_thread));

	snic->r_info = rdma_init();
	snic->s_info = server_thread_init();
	snic->c_info = client_thread_init();

	return snic;
}


void *rdma_sock_thread(void *arg)
{
	//arg is struct server_snic * sni
	struct server_snic * snic = (struct server_snic *)arg;
	struct rdma_thread * r_info = snic->r_info;
	struct socket_thread * c_info = snic->c_info;

        sleep(3);       //Waiting for servers to be ready

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

	//TODO
	while(rdma_get_cm_event(r_info->ec, &r_info->event) == 0){

		struct rdma_cm_event event_copy;

		rdma_ack_cm_event(r_info->event);
		if (on_event(&event_copy))
			break;
		socket_send_message(c_info, message);
	}

        socket_end(c_info);

        pthread_exit(NULL);
}

void *sock_rdma_thread(void *arg)
{
	//arg is struct server_snic * sni
	struct server_snic * snic = (struct server_snic *)arg;
	struct rdma_thread * r_info = snic->r_info;
	struct socket_thread * s_info = snic->s_info;

	while(true){
		//Receive data from Client
		int valread = read(s_info->socket, s_info->buffer, SO_BUFFER_SIZE);
		if (valread < 0) {
			perror("read");
			socket_end(s_info);
			pthread_exit(NULL);
		}

		//Print received data
		printf("%s: Server received data: %s\n", __func__, s_info->buffer);
		//TODO
		//sock->rdma
//		memcpy(conn->send_region, s_info->buffer, BUFFER_SIZE);
		//TODO
		// ?event_copy
               	on_connection(r_info->event->id->context);
        }

	pthread_exit(NULL);
}
