/**
 * Copyright (C) 2016      Mellanox Technologies Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * $ gcc mlx5_test.c -libverbs
 * $ ./a.out [ <server> ]
 */

#include <infiniband/arch.h>
#include <infiniband/verbs.h>
#include <infiniband/mlx5_hw.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <getopt.h>
#include <stdio.h>
#include <netdb.h>


#define MLNX_VENDOR_ID       0x02c9
#define CONNECTIB_PART_ID    4113
#define CONNECTX4_PART_ID    4115


#define memory_store_fence()   asm volatile ("" ::: "memory" )
#define pci_store_fence()      asm volatile ("sfence" ::: "memory" )

typedef struct {
    int                lid;
    uint32_t           qpn;
} ibaddr_t;


typedef struct {
    /* Options */
    const char         *server_name;
    unsigned           tcp_port;
    const char         *dev_name;
    int                port_num;
    unsigned           txq_len;
    unsigned           rxq_len;
    unsigned           min_inline;
    size_t             msg_length;

    /* IB objects */
    struct ibv_context *context;
    struct ibv_pd      *pd;
    struct ibv_qp      *qp;
    struct ibv_cq      *tx_cq;
    struct ibv_cq      *rx_cq;
    struct ibv_mr      *mr;

    ibaddr_t           local_addr;
    ibaddr_t           remote_addr;
    void               *buffer;

    /* MLX5 objects */
    struct {
        volatile uint32_t *qp_dbr;
        uint16_t          qp_pi;
        uint16_t          qp_len;

        void              *cq_buf;
        unsigned          cq_ci;
        unsigned          cq_length;
    } rx;

    struct {
        void              *qp_buf;
        volatile uint32_t *qp_dbr;
        volatile void     *qp_bf;
        uint16_t          qp_pi;
        uint16_t          qp_mask;
    } tx;
} test_ctx_t;


static int parse_args(test_ctx_t *ctx, int argc, char **argv)
{
    int c;

    ctx->server_name = NULL;
    ctx->tcp_port    = 12345;
    ctx->dev_name    = NULL;
    ctx->port_num    = 1;
    ctx->txq_len     = 128;
    ctx->rxq_len     = 1024;
    ctx->min_inline  = 0;
    ctx->msg_length  = 8;

    while ( (c = getopt(argc, argv, "hd:i:")) != -1 ) {
        switch (c) {
        case 'd':
           ctx->dev_name = optarg;
           break;
        case 'i':
            ctx->port_num = atoi(optarg);
            break;
        case 'h':
        default:
            fprintf(stderr, "invalid usage\n");
            return -1;
        }
    }

    if (optind < argc) {
         ctx->server_name = argv[optind];
    }

    return 0;
}

static int init_ctx(test_ctx_t *ctx)
{
    struct ibv_device **device_list;
    struct ibv_device_attr dev_attr;
    struct ibv_port_attr port_attr;
    struct ibv_qp_init_attr qp_init_attr;
    int i, num_devices;
    int ret;

    ctx->context = NULL;

    device_list = ibv_get_device_list(&num_devices);
    for (i = 0; i < num_devices; ++i) {
        if ((ctx->dev_name == NULL) ||
            !strcmp(ctx->dev_name, device_list[i]->dev_name))
        {
            ctx->context = ibv_open_device(device_list[i]);
            ret = ibv_query_device(ctx->context, &dev_attr);
            if (ret) {
                fprintf(stderr, "failed to query context: %m\n");
                return ret;
            }

            if ((dev_attr.vendor_id != MLNX_VENDOR_ID) ||
                ((dev_attr.vendor_part_id != CONNECTIB_PART_ID) &&
                 (dev_attr.vendor_part_id != CONNECTX4_PART_ID)))
            {
                ibv_close_device(ctx->context);
                ctx->context = NULL;
                continue;
            }

            break;
        }
    }

    if (ctx->context == NULL) {
        fprintf(stderr, "could not find matching device\n");
        return -1;
    }

    ret = ibv_query_port(ctx->context, ctx->port_num, &port_attr);
    if (ret) {
        fprintf(stderr, "failed to query port: %m\n");
        return -1;
    }

    ctx->local_addr.lid = port_attr.lid;

    ctx->pd = ibv_alloc_pd(ctx->context);
    if (ctx->pd == NULL) {
        fprintf(stderr, "failed to allocate pd: %m\n");
        return -1;
    }

    ctx->tx_cq = ibv_create_cq(ctx->context, ctx->txq_len, NULL, NULL, 0);
    if (ctx->tx_cq == NULL) {
        fprintf(stderr, "failed to create tx cq: %m\n");
        return -1;
    }

    ctx->rx_cq = ibv_create_cq(ctx->context, ctx->rxq_len, NULL, NULL, 0);
    if (ctx->tx_cq == NULL) {
        fprintf(stderr, "failed to create rx cq: %m\n");
        return -1;
    }

    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.send_cq             = ctx->tx_cq;
    qp_init_attr.recv_cq             = ctx->rx_cq;
    qp_init_attr.srq                 = NULL;
    qp_init_attr.cap.max_send_wr     = ctx->txq_len;
    qp_init_attr.cap.max_recv_wr     = ctx->rxq_len;
    qp_init_attr.cap.max_send_sge    = 1;
    qp_init_attr.cap.max_recv_sge    = 1;
    qp_init_attr.cap.max_inline_data = ctx->min_inline;
    qp_init_attr.qp_type             = IBV_QPT_RC;
    qp_init_attr.sq_sig_all          = 0;

    ctx->qp = ibv_create_qp(ctx->pd, &qp_init_attr);
    if (ctx->qp == NULL) {
        fprintf(stderr, "failed to create qp: %m\n");
        return -1;
    }

    ctx->buffer = memalign(4096, ctx->msg_length);
    if (ctx->buffer == NULL) {
        fprintf(stderr, "failed to allocate buffer\n");
        return -1;
    }

    ctx->mr = ibv_reg_mr(ctx->pd, ctx->buffer, ctx->msg_length,
                         IBV_ACCESS_LOCAL_WRITE);
    if (ctx->mr == NULL) {
        fprintf(stderr, "failed to register buffer: %m\n");
        return -1;
    }

    ctx->local_addr.qpn = ctx->qp->qp_num;
    return 0;
}

static void cleanup_ctx(test_ctx_t *ctx)
{
    ibv_dereg_mr(ctx->mr);
    free(ctx->buffer);
    ibv_destroy_qp(ctx->qp);
    ibv_destroy_cq(ctx->rx_cq);
    ibv_destroy_cq(ctx->tx_cq);
    ibv_dealloc_pd(ctx->pd);
    ibv_close_device(ctx->context);
}

static int exchange_data(test_ctx_t *ctx)
{
    int sockfd, server_sock;
    struct sockaddr_in inaddr;
    struct ibv_qp_attr qp_attr;
    struct hostent *he;
    ibaddr_t local_addr;
    int optval;
    ssize_t ret;

    if (ctx->server_name != NULL) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            fprintf(stderr, "failed to create socket: %m\n");
            return -1;
        }

        he = gethostbyname(ctx->server_name);
        if (he == NULL || he->h_addr_list == NULL) {
            fprintf(stderr, "host %s not found: %s", ctx->server_name, hstrerror(h_errno));
            return -1;
        }

        inaddr.sin_family = he->h_addrtype;
        inaddr.sin_port   = htons(ctx->tcp_port);
        memcpy(&inaddr.sin_addr, he->h_addr_list[0], he->h_length);
        memset(inaddr.sin_zero, 0, sizeof(inaddr.sin_zero));
        ret = connect(sockfd, (struct sockaddr*)&inaddr, sizeof(inaddr));
        if (ret < 0) {
            fprintf(stderr, "connect() failed: %m\n");
            return -1;
        }
    } else {
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock < 0) {
            fprintf(stderr, "failed to create socket: %m\n");
            return -1;
        }

        optval = 1;
        ret = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (ret < 0) {
            fprintf(stderr, "setsockopt(SO_REUSEADDR) failed: %m\n");
            return -1;
        }

        inaddr.sin_family      = AF_INET;
        inaddr.sin_port        = htons(ctx->tcp_port);
        inaddr.sin_addr.s_addr = INADDR_ANY;
        memset(inaddr.sin_zero, 0, sizeof(inaddr.sin_zero));
        ret = bind(server_sock, (struct sockaddr*)&inaddr, sizeof(inaddr));
        if (ret < 0) {
            fprintf(stderr, "bind() failed: %m\n");
            return -1;
        }

        ret = listen(server_sock, 10);
        if (ret < 0) {
            fprintf(stderr, "listen() failed: %m\n");
            return -1;
        }

        printf("Waiting for connection...\n");

        /* Accept next connection */
        sockfd = accept(server_sock, NULL, NULL);
        if (sockfd < 0) {
            fprintf(stderr, "accept() failed: %m\n");
            return -1;
        }

        close(server_sock);
    }

    ret = send(sockfd, &ctx->local_addr,  sizeof(ibaddr_t), 0);
    if (ret != sizeof(ibaddr_t)) {
        fprintf(stderr, "failed to send\n");
        return -1;
    }

    ret = recv(sockfd, &ctx->remote_addr, sizeof(ibaddr_t), 0);
    if (ret != sizeof(ibaddr_t)) {
        fprintf(stderr, "failed to receive\n");
        return -1;
    }

    close(sockfd);

    memset(&qp_attr, 0, sizeof(qp_attr));

    qp_attr.qp_state              = IBV_QPS_INIT;
    qp_attr.pkey_index            = 0;
    qp_attr.port_num              = ctx->port_num;
    qp_attr.qp_access_flags       = IBV_ACCESS_LOCAL_WRITE|
                                    IBV_ACCESS_REMOTE_WRITE|
                                    IBV_ACCESS_REMOTE_READ|
                                    IBV_ACCESS_REMOTE_ATOMIC;
    ret = ibv_modify_qp(ctx->qp, &qp_attr,
                        IBV_QP_STATE      |
                        IBV_QP_PKEY_INDEX |
                        IBV_QP_PORT       |
                        IBV_QP_ACCESS_FLAGS);
    if (ret) {
        fprintf(stderr, "error modifying QP to INIT: %m\n");
        return -1;
    }

    qp_attr.qp_state              = IBV_QPS_RTR;
    qp_attr.ah_attr.dlid          = ctx->remote_addr.lid;
    qp_attr.ah_attr.sl            = 0;
    qp_attr.ah_attr.src_path_bits = 0;
    qp_attr.ah_attr.static_rate   = 0;
    qp_attr.ah_attr.is_global     = 0;
    qp_attr.ah_attr.port_num      = ctx->port_num;
    qp_attr.dest_qp_num           = ctx->remote_addr.qpn;
    qp_attr.rq_psn                = 0;
    qp_attr.path_mtu              = IBV_MTU_4096;
    qp_attr.max_dest_rd_atomic    = 4;
    qp_attr.min_rnr_timer         = 10;
    ret = ibv_modify_qp(ctx->qp, &qp_attr,
                        IBV_QP_STATE              |
                        IBV_QP_AV                 |
                        IBV_QP_PATH_MTU           |
                        IBV_QP_DEST_QPN           |
                        IBV_QP_RQ_PSN             |
                        IBV_QP_MAX_DEST_RD_ATOMIC |
                        IBV_QP_MIN_RNR_TIMER);
    if (ret) {
        fprintf(stderr, "error modifying QP to RTR: %m\n");
        return -1;
    }

    qp_attr.qp_state              = IBV_QPS_RTS;
    qp_attr.sq_psn                = 0;
    qp_attr.timeout               = 20;
    qp_attr.rnr_retry             = 7;
    qp_attr.retry_cnt             = 7;
    qp_attr.max_rd_atomic         = 4;
    ret = ibv_modify_qp(ctx->qp, &qp_attr,
                        IBV_QP_STATE              |
                        IBV_QP_TIMEOUT            |
                        IBV_QP_RETRY_CNT          |
                        IBV_QP_RNR_RETRY          |
                        IBV_QP_SQ_PSN             |
                        IBV_QP_MAX_QP_RD_ATOMIC);
    if (ret) {
        fprintf(stderr, "error modifying QP to RTS: %m\n");
        return -1;
    }

    printf("Connected to LID %d QPN 0x%x\n", ctx->remote_addr.lid,
           ctx->remote_addr.qpn);
    return 0;
}

static void fill_mlx5_dseg(test_ctx_t *ctx, struct mlx5_wqe_data_seg *dseg)
{
    dseg->addr       = htonll((uintptr_t)ctx->buffer);
    dseg->lkey       = htonl(ctx->mr->lkey);
    dseg->byte_count = htonl(ctx->msg_length);
}

static void post_recv(test_ctx_t *ctx, uint16_t count)
{
     ctx->rx.qp_pi += count;
     memory_store_fence();
     *ctx->rx.qp_dbr = htonl(ctx->rx.qp_pi);
}

static void post_send(test_ctx_t *ctx)
{
    struct mlx5_wqe_ctrl_seg *ctrl;
    struct mlx5_wqe_data_seg *dseg;
    uint16_t pi = ctx->tx.qp_pi;
    uint8_t ds = 2;

    ctrl = ctx->tx.qp_buf + (MLX5_SEND_WQE_BB * (pi & ctx->tx.qp_mask));

    ctrl->opmod_idx_opcode = (MLX5_OPCODE_SEND << 24) | (htons(pi) << 8);
    ctrl->qpn_ds           = htonl((ctx->qp->qp_num << 8) | ds);
    ctrl->fm_ce_se         = 0;

    dseg = (void*)(ctrl + 1);
    fill_mlx5_dseg(ctx, dseg);

    memory_store_fence();

    *ctx->tx.qp_dbr = htonl(ctx->rx.qp_pi += 1);

    pci_store_fence();

    *(uint64_t*)ctx->tx.qp_bf = *(uint64_t*)ctrl;

    pci_store_fence();
}

static int prepare_mlx5(test_ctx_t *ctx)
{
    struct ibv_mlx5_cq_info tx_cq_info, rx_cq_info;
    struct ibv_mlx5_qp_info qp_info;
    struct mlx5_wqe_data_seg *dseg;
    struct ibv_exp_cq_attr cq_attr;
    unsigned i;
    int ret;

    ret = ibv_mlx5_exp_get_qp_info(ctx->qp, &qp_info);
    if (ret) {
        fprintf(stderr, "failed to get qp info: %m\n");
        return -1;
    }

    ret = ibv_mlx5_exp_get_cq_info(ctx->tx_cq, &tx_cq_info);
    if (ret) {
        fprintf(stderr, "failed to get tx cq info: %m\n");
        return -1;
    }

    ret = ibv_mlx5_exp_get_cq_info(ctx->rx_cq, &rx_cq_info);
    if (ret) {
        fprintf(stderr, "failed to get rx cq info: %m\n");
        return -1;
    }

    if (qp_info.rq.stride != 16) {
        fprintf(stderr, "invalid mlx5 rq (stride is %d)\n", qp_info.rq.stride);
        return -1;
    }

    cq_attr.comp_mask    = IBV_EXP_CQ_ATTR_CQ_CAP_FLAGS;
    cq_attr.cq_cap_flags = IBV_EXP_CQ_IGNORE_OVERRUN;
    ret = ibv_exp_modify_cq(ctx->rx_cq, &cq_attr, IBV_EXP_CQ_CAP_FLAGS);
    if (ret) {
        fprintf(stderr, "failed to modify cq: %m\n");
        return -1;
    }

    ctx->rx.qp_len    = qp_info.rq.wqe_cnt;
    ctx->rx.qp_dbr    = &qp_info.dbrec[MLX5_RCV_DBR];
    ctx->rx.qp_pi     = 0;
    ctx->rx.cq_buf    = rx_cq_info.buf;
    ctx->rx.cq_length = rx_cq_info.cqe_cnt;
    ctx->rx.cq_ci     = 0;

    /* Prepare receive queue */
    dseg = qp_info.rq.buf;
    for (i = 0; i < ctx->rx.qp_len; ++i) {
        fill_mlx5_dseg(ctx, &dseg[i]);
    }

    ctx->tx.qp_buf  = qp_info.sq.buf;
    ctx->tx.qp_mask = qp_info.sq.wqe_cnt - 1;
    ctx->tx.qp_dbr  = &qp_info.dbrec[MLX5_SND_DBR];
    ctx->tx.qp_bf   = qp_info.bf.reg;
    ctx->tx.qp_pi   = 0;

    post_recv(ctx, ctx->rx.qp_len);

    return 0;
}

static void wait_recv(test_ctx_t *ctx)
{
    volatile struct mlx5_cqe64 *cqe;
    unsigned index;

    index  = ctx->rx.cq_ci;
    cqe    = ctx->rx.cq_buf + ((index & (ctx->rx.cq_length - 1)) * sizeof(*cqe));

    while (((cqe->op_own & MLX5_CQE_OWNER_MASK) == !(index & ctx->rx.cq_length)) ||
           ((cqe->op_own >> 4) == MLX5_CQE_INVALID))
        ;

    if ((cqe->op_own >> 4) == MLX5_CQE_RESP_SEND) {
        printf("Received one message, byte_count %d\n", ntohl(cqe->byte_cnt));
    } else {
        printf("Unexpected recv opcode: %d\n", cqe->op_own >> 4);
    }

    ++ctx->rx.cq_ci;
}

static int run_test(test_ctx_t *ctx)
{
    struct mlx5_wqe_data_seg *dseg;
    int ret;

    post_send(ctx);
    wait_recv(ctx);
}

int main(int argc, char **argv)
{
    test_ctx_t ctx;
    int ret;

    ret = parse_args(&ctx, argc, argv);
    if (ret < 0) {
        return ret;
    }

    ret = init_ctx(&ctx);
    if (ret < 0) {
        return ret;
    }

    ret = prepare_mlx5(&ctx);
    if (ret < 0) {
        cleanup_ctx(&ctx);
        return ret;
    }

    ret = exchange_data(&ctx);
    if (ret < 0) {
        cleanup_ctx(&ctx);
        return ret;
    }

    ret = run_test(&ctx);

    cleanup_ctx(&ctx);
    return ret;
}
