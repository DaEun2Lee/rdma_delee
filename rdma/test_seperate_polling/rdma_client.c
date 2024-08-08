#include "rdma_client.h"
#define DEFAULT_IP = "10.0.0.1";

const int BUFFER_SIZE = 1024;
const int TIMEOUT_IN_MS = 50000;                //default is 500 /* ms */
//const char *DEFAULT_IP = "10.0.0.1";
const int DEFAULT_PORT = 12345;
const unsigned int RDMA_REQUEST_CONNECT_SOCKET = 1;

struct rdma_thread *r_info;
static struct context *s_ctx = NULL;

//for recordimg time 
double start, end;
FILE *r_fptr;

FILE *open_record_file()
{
	r_fptr = fopen("Execution_time.txt", "a");
	if (r_fptr == NULL) {
		perror("Failed to open file");
		exit(EXIT_FAILURE);
	}
	return r_fptr;
}

struct rdma_thread * rdma_init()
{
	r_info = malloc(sizeof(struct rdma_thread));
	r_info->event = NULL;
	r_info->conn = NULL;
	r_info->ec = NULL;
	memset(&(r_info->addr), 0, sizeof(r_info->addr));

	struct addrinfo *addr_info;
	TEST_NZ(getaddrinfo(DEFAULT_IP, "DEFAULT_PORT", NULL, &addr_info));
	//addrinfo  => sockaddr_in
	r_info->addr.sin_family = AF_INET;
	r_info->addr.sin_port = htons(DEFAULT_PORT);
	if (inet_pton(AF_INET, DEFAULT_IP, &(r_info->addr.sin_addr)) <= 0) {
		perror("inet_pton failed");
	}

	TEST_Z(r_info->ec = rdma_create_event_channel());
	TEST_NZ(rdma_create_id(r_info->ec, &r_info->conn, NULL, RDMA_PS_TCP));
	TEST_NZ(rdma_resolve_addr(r_info->conn, NULL, addr_info->ai_addr, TIMEOUT_IN_MS));

	freeaddrinfo(addr_info);

	return r_info;
}



//@TODO
int rdma_sock_linker()
{
	r_fptr = open_record_file();

	//Input file contents in rdma_region.
	FILE *file;
//	char message[BUFFER_SIZE];
	file = fopen("request.txt", "r");
	if (file == NULL) {
		perror("Failed to open file");
		return 1;
	}
	size_t bytesRead = fread(message, sizeof(char), BUFFER_SIZE - 1, file);
	message[bytesRead] = '\0';
	fclose(file);

	r_info = rdma_init();


	while (rdma_get_cm_event(r_info->ec, &r_info->event) == 0) {
		struct rdma_cm_event event_copy;

		memcpy(&event_copy, r_info->event, sizeof(*r_info->event)); // *evnet
		rdma_ack_cm_event(r_info->event);

		if (on_event(&event_copy)){
			break;
		}
	}

	rdma_destroy_event_channel(r_info->ec);

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

int on_addr_resolved(struct rdma_cm_id *id)
{
	struct ibv_qp_init_attr qp_attr;
	struct connection *conn;

//      printf("address resolved.\n");

	build_context(id->verbs);
	build_qp_attr(&qp_attr);

	TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

	id->context = conn = (struct connection *)malloc(sizeof(struct connection));

	conn->id = id;
	conn->qp = id->qp;
	conn->num_completions = 0;

	register_memory(conn);
	post_receives(conn);

	TEST_NZ(rdma_resolve_route(id, TIMEOUT_IN_MS));

	return 0;
}

void on_completion(struct ibv_wc *wc)
{
	struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;

	if (wc->status != IBV_WC_SUCCESS)
		die("on_completion: status is not IBV_WC_SUCCESS.");

	if (wc->opcode & IBV_WC_RECV) {
//		printf("RDMA-Client received message: \n%s\n", conn->recv_region);
		end = record_time_file(r_fptr, "RECV message");
		execution_time(r_fptr, start, end, "SEND/RECV Time");
        } else if (wc->opcode == IBV_WC_SEND) {
//		printf("%s: send completed successfully.\n", __func__);
//		printf("%s: RDMA-Client send message : %s\n", __func__, conn->send_region);
        } else {
		die("on_completion: completion isn't a send or a receive.");
	}

	if (++conn->num_completions == 2)
		rdma_disconnect(conn->id);
}

int on_connection(void *context)
{
	struct connection *conn = (struct connection *)context;
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	snprintf(conn->send_region, BUFFER_SIZE, "message from active/client side with pid %d", getpid());

	//Input message
	memcpy(conn->send_region, message, BUFFER_SIZE);

	memset(&wr, 0, sizeof(wr));

	wr.wr_id = (uintptr_t)conn;
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

int on_connection_req_sock(void *context)
{
	struct connection *conn = (struct connection *)context;
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	snprintf(conn->send_region, BUFFER_SIZE, "message from active/client side with pid %d", getpid());

	//Input message
	memcpy(conn->send_region, message, BUFFER_SIZE);
//	printf("%s: RDMA-Client send message:\n%s\n\n", __func__, conn->send_region);
//	printf("connected. posting send...\n");

	memset(&wr, 0, sizeof(wr));

	wr.wr_id = (uintptr_t)conn;
	wr.opcode = IBV_WR_SEND_WITH_IMM;                       //IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;
	wr.imm_data = htonl(RDMA_REQUEST_CONNECT_SOCKET);       //(0x12345678);

	sge.addr = (uintptr_t)conn->send_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = conn->send_mr->lkey;

	TEST_NZ(ibv_post_send(conn->qp, &wr, &bad_wr));

	return 0;
}

int on_disconnect(struct rdma_cm_id *id)
{
	struct connection *conn = (struct connection *)id->context;

//	printf("%s: RDMA-Client disconnected.\n", __func__);

	rdma_destroy_qp(id);

	ibv_dereg_mr(conn->send_mr);
	ibv_dereg_mr(conn->recv_mr);

	free(conn->send_region);
	free(conn->recv_region);

	free(conn);

	rdma_destroy_id(id);

	return 1; /* exit event loop */
//      return 0;
}

int on_event(struct rdma_cm_event *event)
{
	int r = 0;

	if (event->event == RDMA_CM_EVENT_ADDR_RESOLVED) {
//		printf("%s: event = RDMA_CM_EVENT_ADDR_RESOLVED\n", __func__);
		r = on_addr_resolved(event->id);

	} else if (event->event == RDMA_CM_EVENT_ROUTE_RESOLVED) {
//		printf("%s: event = RDMA_CM_EVENT_ROUTE_RESOLVED\n", __func__);
		r = on_route_resolved(event->id);

	} else if (event->event == RDMA_CM_EVENT_ESTABLISHED) {
//		printf("%s: event = RDMA_CM_EVENT_ESTABLISHED\n", __func__);
		//for recording
		start = record_time_file(r_fptr, "Start Send Message");
		r = on_connection(event->id->context);

	} else if (event->event == RDMA_CM_EVENT_DISCONNECTED) {
//		printf("%s: event = RDMA_CM_EVENT_DISCONNECTED\n", __func__);
		r = on_disconnect(event->id);

	} else if (event->event == RDMA_CM_EVENT_UNREACHABLE) {
		die("on_event: event = RDMA_CM_EVENT_UNREACHABLE\n");

	} else {
//		printf("%s: event = %d\n", __func__, event->event);
		die("on_event: unknown event.");
	}

	return r;
}

int on_route_resolved(struct rdma_cm_id *id)
{
	struct rdma_conn_param cm_params;

	printf("route resolved.\n");

	memset(&cm_params, 0, sizeof(cm_params));
	TEST_NZ(rdma_connect(id, &cm_params));

	return 0;
}
