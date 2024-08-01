#include "rdma_server.h"

const int BUFFER_SIZE = 1024;
const int DEFAULT_PORT = 12345;

struct context *s_ctx = NULL;

//struct server_snic *snic;

struct rdma_thread *r_info;
struct socket_thread *s_info;
struct socket_thread *c_info;

char *sock_rdma_data;

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
	//check
	TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
	TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=10 is arbitrary */
	TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));

//	TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));
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
		socket_send_message(c_info, conn->recv_region);
		sock_rdma_data = c_info->buffer;
		
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

//  memset(&cm_params, 0, sizeof(cm_params));
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
//	memcpy(conn->send_region, snic->s_info->buffer, BUFFER_SIZE);
	memcpy(conn->send_region, sock_rdma_data, BUFFER_SIZE);
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

//	return 0;
	return 1;
}

int on_event(struct rdma_cm_event *event)
{
	int r = 0;

	if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST) {
		printf("%s: event = RDMA_CM_EVENT_CONNECT_REQUEST", __func__);
		r = on_connect_request(event->id);
	} else if (event->event == RDMA_CM_EVENT_ESTABLISHED) {
		printf("%s: event = RDMA_CM_EVENT_ESTABLISHED", __func__);
		r = on_connection(event->id->context);
	} else if (event->event == RDMA_CM_EVENT_DISCONNECTED) {
		printf("%s: event = RDMA_CM_EVENT_DISCONNECTED", __func__);
//		r = on_disconnect(event->id);
		r= 0;
	} else {
		printf("%s: event = %d", __func__, event->event);
		die("on_event: unknown event.");
	}

	return r;
}

struct rdma_thread * rdma_init()
//void rdma_init()
{
//	struct rdma_thread *r_info;
//	snic->r_info = malloc(sizeof(struct rdma_thread));
	r_info = malloc(sizeof(struct rdma_thread));
//Set Address for RDMA
//	snic->r_info->event = NULL;
//        snic->r_info->listener = NULL;
//        snic->r_info->ec = NULL
	r_info->event = NULL;
	r_info->listener = NULL;
	r_info->ec = NULL;

//        memset(&(snic->r_info->addr), 0, sizeof(snic->r_info->addr));
	memset(&(r_info->addr), 0, sizeof(r_info->addr));

//#if _USE_IPV6
//        r_info->addr.sin6_family = AF_INET6;
//        r_info->addr.sin6_port = htons(DEFAULT_PORT);
//#else
//        snic->r_info->addr.sin_family = AF_INET;
//	snic->r_info->addr.sin_port = htons(DEFAULT_PORT);
	r_info->addr.sin_family = AF_INET;
	r_info->addr.sin_port = htons(DEFAULT_PORT);
//#endif

	TEST_Z(r_info->ec = rdma_create_event_channel());
        TEST_NZ(rdma_create_id(r_info->ec, &r_info->listener, NULL, RDMA_PS_TCP));
	TEST_NZ(rdma_bind_addr(r_info->listener, (struct sockaddr *)&(r_info->addr)));
	TEST_NZ(rdma_listen(r_info->listener, 10)); /* backlog=10 is arbitrary */

//	freeaddrinfo(r_info->addr);
	printf("%s: RDMA listening on port %d.\n", __func__, DEFAULT_PORT);

	return r_info;
}

bool rdma_sock_thread_init()
{
	s_info = server_thread_init();
        if(s_info == NULL)
                return false;

        if(!socket_listen(s_info))
                return false;

        printf("%s: Create Sock-Server\n", __func__);


	c_info = client_thread_init();
	if(c_info == NULL)
                return false;

        if(!socket_connect(c_info))
                return false;

        printf("%s: Create Sock-Client\n", __func__);

	r_info = rdma_init();

	return true;
}


void *sock_rdma_thread(void *arg)
{
	if(!rdma_sock_thread_init())
		pthread_exit(NULL);
	printf("%s: rdma_sock_thread_init", __func__);

	//TODO
	 while (rdma_get_cm_event(r_info->ec, &r_info->event) == 0) {
		struct rdma_cm_event event_copy;

		memcpy(&event_copy, r_info->event, sizeof(*r_info->event));
                rdma_ack_cm_event(r_info->event);

		//sock
		//Receive data from Client
		int valread = read(s_info->socket, s_info->buffer, SO_BUFFER_SIZE);
		if (valread < 0) {
			perror("read");
			socket_end(s_info);
			//Print received data
			printf("%s: Server received message: %s\n", __func__, s_info->buffer);

			sock_rdma_data = s_info->buffer;
		}

		int r = 0;
		struct rdma_cm_event *t_event = &event_copy;
		switch (t_event->event) {
			case RDMA_CM_EVENT_CONNECT_REQUEST:
				printf("%s: event = RDMA_CM_EVENT_CONNECT_REQUEST\n", __func__);
				r_info->status = RDMA_CM_EVENT_CONNECT_REQUEST;
				r = on_connect_request(t_event->id);
				//TODO
				pthread_create(&s_ctx->cq_poller_thread, NULL, rdma_sock_thread, NULL);
				break;
			case RDMA_CM_EVENT_ESTABLISHED:
				printf("%s: event = RDMA_CM_EVENT_ESTABLISHED\n", __func__);
				r_info->status = RDMA_CM_EVENT_ESTABLISHED;
				r = on_connection(t_event->id->context);
				break;
			case RDMA_CM_EVENT_DISCONNECTED:
				printf("%s: event = RDMA_CM_EVENT_DISCONNECTED\n", __func__);
				r_info->status = RDMA_CM_EVENT_DISCONNECTED;
				// r = on_disconnect(t_event->id);
				break;
			default:
				printf("%s: event = %d\n", __func__, t_event->event);
				// die("on_event: unknown event.");
				break;
		}
		if(r)
			break;



//			break;
//		}
	}
	pthread_join(s_ctx->cq_poller_thread, NULL);
        socket_end(c_info);
	rdma_destroy_id(r_info->listener);
        rdma_destroy_event_channel(r_info->ec);
        pthread_exit(NULL);
}

void *rdma_sock_thread(void *ctx)
{
	sleep(3);

	//poll_cq
	struct ibv_cq *cq;
	struct ibv_wc *wc;

	wc =NULL;

	while (true) {
		TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
		ibv_ack_cq_events(cq, 1);
		TEST_NZ(ibv_req_notify_cq(cq, 0));

		while (ibv_poll_cq(cq, 1, wc)){
//			on_completion(&wc);
			//on_completion
			if (wc->status != IBV_WC_SUCCESS){
				printf("%s: on_completion: status is not IBV_WC_SUCCESS.", __func__);
				break;
			}

			if (wc->opcode & IBV_WC_RECV) {
				struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;
				printf("%s: RDMA-Server is received message: \n%s\n", __func__, conn->recv_region);
				//TODO
				//send rdma -> socket
				socket_send_message(c_info, conn->recv_region);
				sock_rdma_data = c_info->buffer;
			} else if (wc->opcode == IBV_WC_SEND) {
				printf("%s: RDMA sends completed successfully.\n", __func__);
				sock_rdma_data = NULL;
			}
		}
	}

	pthread_exit(NULL);
}
