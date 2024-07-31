#ifndef RDMA_SERVER_H_
#define RDMA_SERVER_H_

#include <rdma/rdma_cma.h>
#include "socket.h"		//made by delee

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)


//const int BUFFER_SIZE = 1024;
//const int DEFAULT_PORT = 12345;
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

struct rdma_thread{
#if _USE_IPV6
        struct sockaddr_in6 addr;
#else
        struct sockaddr_in addr;
#endif
        struct rdma_cm_event *event;		// = NULL;
        struct rdma_cm_id *listener;		// = NULL;
        struct rdma_event_channel *ec;		// = NULL;
};

struct server_snic{
	struct rdma_thread *r_info;
	struct socket_thread *s_info;
	struct socket_thread *c_info;
};

//struct context *s_ctx = NULL;

//struct server_snic *snic;

void die(const char *reason);

void build_context(struct ibv_context *verbs);
void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
void * poll_cq(void *);
void post_receives(struct connection *conn);
void register_memory(struct connection *conn);

void on_completion(struct ibv_wc *wc);
int on_connect_request(struct rdma_cm_id *id);
int on_connection(void *context);
//static int on_disconnect(struct rdma_cm_id *id);
int on_event(struct rdma_cm_event *event);

struct rdma_thread * rdma_init();
struct server_snic * rdma_sock_thread_init();

void *rdma_sock_thread(void *arg);
void *sock_rdma_thread(void *arg);

#endif // RDMA_SERVER_H_
