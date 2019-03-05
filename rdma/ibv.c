#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <infiniband/verbs.h>

static void *event_thread(void *arg)
{
    struct ibv_async_event event;
    struct ibv_context *ctx = (struct ibv_context *)arg;

    while (ibv_get_async_event(ctx, &event) == 0)
        printf("EVENT: %s\n", ibv_event_type_str(event.event_type));

    printf("ERROR: event\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    const int size = 65536, qsize = 10;
    char buf[size];
    int n;
    struct ibv_device **devlst;
    struct ibv_context *ctx;
    struct ibv_device_attr attr;
    struct ibv_pd *pd;
    struct ibv_mr *mr;
    struct ibv_comp_channel *cc;
    struct ibv_cq *cq;
    struct ibv_qp *qp;

    /* list devices */
    devlst = ibv_get_device_list(&n);
    if (!devlst) {
        perror("ibv_get_device_list");
        return 1;
    }

    for (int i = 0; i < n; ++i) {
        printf("#%d %s\t node-type='%s'\n", i+1, ibv_get_device_name(devlst[i]),
                ibv_node_type_str(devlst[i]->node_type));
    }

    printf("\n");

    if (argc >= 2) {
        while (n--) {
            if (strcmp(argv[1], ibv_get_device_name(devlst[n])) == 0)
                break;
        }
    } else {
        n = 0;
    }

    if (n < 0) {
        printf("device not found!\n");
        return 1;
    }

    /* open device */
    ctx = ibv_open_device(devlst[n]);
    if (!ctx) {
        perror("ibv_open_device");
        return 1;
    }
    printf("%s opened\n", ibv_get_device_name(devlst[n]));

    /* start async event thread */
    pthread_t tid;
    pthread_create(&tid, NULL, event_thread, ctx);

    /* query device capabilities */
    if (ibv_query_device(ctx, &attr)) {
        perror("ibv_query_device");
        return 1;
    }
    printf("max_srq = %d\n", attr.max_srq);
    printf("phys_port_cnt = %d\n", attr.phys_port_cnt);

    /* allocate protection domain */
    pd = ibv_alloc_pd(ctx);
    if (!pd) {
        perror("ibv_allocate_pd");
        return 1;
    }

    /* register memory region */
    mr = ibv_reg_mr(pd, buf, size, IBV_ACCESS_LOCAL_WRITE |\
                                   IBV_ACCESS_REMOTE_READ |\
                                   IBV_ACCESS_REMOTE_WRITE);
    if (!mr) {
        perror("ibv_reg_mr");
        return 1;
    }

    /* create completion channel */
    cc = ibv_create_comp_channel(ctx);
    if (!cc) {
        perror("ibv_create_comp_channel");
        return 1;
    }

    /* create completion queue */
    cq = ibv_create_cq(ctx, qsize, NULL, cc, 0);
    if (!cq) {
        perror("ibv_create_cq");
        return 1;
    }

    /* create queue pair */
    struct ibv_qp_init_attr qp_attr = {
        .send_cq = cq,
        .recv_cq = cq,
        .cap = {
            .max_send_wr  = 1,
            .max_recv_wr  = 1,
            .max_send_sge = 1,
            .max_recv_sge = 1,
        },
        .qp_type = IBV_QPT_RC,
    };
    qp = ibv_create_qp(pd, &qp_attr);
    if (!qp) {
        perror("ibv_create_qp");
        return 1;
    }

    /* post senq request */
    struct ibv_sge sg_list = {
        .addr       = (uintptr_t)buf,
        .length     = size,
        .lkey       = mr->lkey,
    };
    struct ibv_send_wr wr = {
        .next       = NULL,
        .sg_list    = &sg_list,
        .num_sge    = 1,
        .opcode     = IBV_WR_SEND,
    };
    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(qp, &wr, &bad_wr)) {
        printf("ERROR: ibv_post_send() failed\n");
        return 1;
    }

    /* cleanup */
    ibv_destroy_qp(qp);
    ibv_destroy_cq(cq);
    ibv_destroy_comp_channel(cc);
    ibv_dereg_mr(mr);
    ibv_dealloc_pd(pd);
    ibv_close_device(ctx);
    ibv_free_device_list(devlst);

    return 0;
}
