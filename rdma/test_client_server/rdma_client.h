#ifndef RDMA_CLIENT_H
#define RDMA_CLIENT_H

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

char message[1024];

struct context {
	struct ibv_context *ctx;
	struct ibv_pd *pd;
	struct ibv_cq *cq;
	struct ibv_comp_channel *comp_channel;

	pthread_t cq_poller_thread;
};

struct connection {
	struct rdma_cm_id *id;
	struct ibv_qp *qp;

	struct ibv_mr *recv_mr;
	struct ibv_mr *send_mr;

	char *recv_region;
	char *send_region;

	int num_completions;
};

struct rdma_thread{
//#if _USE_IPV6
//        struct sockaddr_in6 addr;
//#else
        struct sockaddr_in addr;
//#endif
//	struct addrinfo *addr;
        struct rdma_cm_event *event;            // = NULL;
        struct rdma_cm_id *conn;            // = NULL;
        struct rdma_event_channel *ec;          // = NULL;

        int status;
};

//FILE *open_record_file();
struct rdma_thread * rdma_init();
int rdma_sock_linker();

void die(const char *reason);

void build_context(struct ibv_context *verbs);
void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
void * poll_cq(void *);
void post_receives(struct connection *conn);
void register_memory(struct connection *conn);

int on_addr_resolved(struct rdma_cm_id *id);
void on_completion(struct ibv_wc *wc);
int on_connection(void *context);
int on_disconnect(struct rdma_cm_id *id);
int on_event(struct rdma_cm_event *event);
int on_route_resolved(struct rdma_cm_id *id);

#endif // RDMA_CLIENT_H
