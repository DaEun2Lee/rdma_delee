

struct context {
        struct ibv_context *ctx;
        struct ibv_pd *pd;
        struct ibv_cq *cq;
        struct ibv_comp_channel *comp_channel;

        pthread_t cq_poller_thread;
};

//client ver.
struct connection {
        struct rdma_cm_id *id;
        struct ibv_qp *qp;

        struct ibv_mr *recv_mr;
        struct ibv_mr *send_mr;

        char *recv_region;
        char *send_region;

        int num_completions;
};

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
	TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=>  TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));
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
	conn->send_region = malloc(BUFFER_SIZE);
	conn->recv_region = malloc(BUFFER_SIZE);

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
