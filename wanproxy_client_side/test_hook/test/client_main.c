#include "../src/rdma_client.h"

int main()
{
//        rdma_sock_linker();
	struct rdma_thread *r_info = rdma_init();
	while (rdma_get_cm_event(r_info->ec, &r_info->event) == 0) {
                struct rdma_cm_event event_copy;

                memcpy(&event_copy, r_info->event, sizeof(*r_info->event)); // *evnet
                rdma_ack_cm_event(r_info->event);

                if (on_event(&event_copy)){
                        break;
                }
        }
//	rdma_destroy_event_channel(r_info->ec);

        return 0;
}
