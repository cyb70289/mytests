#include <stdio.h>
#include <errno.h>
#include <infiniband/verbs.h>

int main(int argc, char *argv[])
{
    const int size = 4096;
    char buf[size];
    int n, rc;
    struct ibv_device **devlst;
    struct ibv_context *ctx;
    struct ibv_device_attr attr;
    struct ibv_pd *pd;
    struct ibv_mr *mr;
    struct ibv_cq *cq;
    struct ibv_qp *qp;

    /* list devices */
    devlst = ibv_get_device_list(&n);
    if (!devlst) {
        perror("ibv_get_device_list");
        return 1;
    }

    for (int i = 0; i < n; ++i) {
        printf("#%d %s\t ibdev_path=%s\n", i+1,
                devlst[i]->name, devlst[i]->ibdev_path);
    }

    printf("\n");

    if (argc >= 2) {
        while (n--) {
            if (strcmp(argv[1], devlst[n]->name) == 0)
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
    printf("%s opened\n", devlst[n]->name);

    /* query device capabilities */
    rc = ibv_query_device(ctx, &attr);
    if (rc) {
        perror("ibv_query_device");
        return 1;
    }
    printf("max_srq = %d\n", attr.max_srq);

    /* allocate protection domain */
    pd = ibv_alloc_pd(ctx);
    if (!pd) {
        perror("ibv_allocate_pd");
        return 1;
    }

    /* register memory region */
    mr = ibv_reg_mr(pd, buf, size, IBV_ACCESS_LOCAL_WRITE);
    if (!mr) {
        perror("ibv_reg_mr");
        return 1;
    }

    /* create completion queue */
    cq = ibv_create_cq(ctx, 100, NULL, NULL, 0);
    if (!cq) {
        perror("ibv_create_cq");
        return 1;
    }

    /* cleanup */
    ibv_close_device(ctx);

    return 0;
}
