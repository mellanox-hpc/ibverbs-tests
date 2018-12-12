#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <errno.h>

#include <infiniband/mlx5dv.h>

#include "devx_prm.h"

enum {
	MLX5_HCA_CAP_OPMOD_GET_MAX	= 0,
	MLX5_HCA_CAP_OPMOD_GET_CUR	= 1,
};

enum {
	MLX5_CAP_GENERAL = 0,
	MLX5_CAP_ETHERNET_OFFLOADS,
	MLX5_CAP_ODP,
	MLX5_CAP_ATOMIC,
	MLX5_CAP_ROCE,
	MLX5_CAP_IPOIB_OFFLOADS,
	MLX5_CAP_IPOIB_ENHANCED_OFFLOADS,
	MLX5_CAP_FLOW_TABLE,
	MLX5_CAP_ESWITCH_FLOW_TABLE,
	MLX5_CAP_ESWITCH,
	MLX5_CAP_RESERVED,
	MLX5_CAP_VECTOR_CALC,
	MLX5_CAP_QOS,
	MLX5_CAP_FPGA,
};


int query_device(struct ibv_context *ctx);
int query_device(struct ibv_context *ctx) {
	uint32_t in[DEVX_ST_SZ_DW(query_hca_cap_in)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(query_hca_cap_out)] = {0};
	int ret;

	DEVX_SET(query_hca_cap_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_CAP);
	DEVX_SET(query_hca_cap_in, in, op_mod, MLX5_HCA_CAP_OPMOD_GET_MAX | (MLX5_CAP_GENERAL << 1));
	ret = mlx5dv_devx_general_cmd(ctx, in, sizeof(in), out, sizeof(out));
	if (ret)
		return ret;
	return DEVX_GET(query_hca_cap_out, out,
			capability.cmd_hca_cap.port_type);
}

int alloc_pd(struct ibv_context *ctx);
int alloc_pd(struct ibv_context *ctx) {
	uint32_t in[DEVX_ST_SZ_DW(alloc_pd_in)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(alloc_pd_out)] = {0};
	struct mlx5dv_devx_obj *pd;

	DEVX_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
	pd = mlx5dv_devx_obj_create(ctx, in, sizeof(in), out, sizeof(out));
	if (!pd)
		return -1;

	return DEVX_GET(alloc_pd_out, out, pd);
}

int reg_mr(struct ibv_context *ctx, int pd, void *buff, size_t size);
int reg_mr(struct ibv_context *ctx, int pd, void *buff, size_t size) {
	uint32_t in[DEVX_ST_SZ_DW(create_mkey_in)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(create_mkey_out)] = {0};
	struct mlx5dv_devx_umem *mem;
	struct mlx5dv_devx_obj *mr;

	mem = mlx5dv_devx_umem_reg(ctx, buff, size, 7);
	if (!mem)
		return 0;

	printf("%s:%d \n", __func__, __LINE__);

	DEVX_SET(create_mkey_in, in, opcode, MLX5_CMD_OP_CREATE_MKEY);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.access_mode_1_0, MLX5_MKC_ACCESS_MODE_MTT);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.a, 1);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.rw, 1);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.rr, 1);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.lw, 1);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.lr, 1);
	DEVX_SET64(create_mkey_in, in, memory_key_mkey_entry.start_addr, (intptr_t)buff);
	DEVX_SET64(create_mkey_in, in, memory_key_mkey_entry.len, size);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.pd, pd);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.translations_octword_size, 1);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.log_entity_size, 12);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.qpn, 0xffffff);
	DEVX_SET(create_mkey_in, in, memory_key_mkey_entry.mkey_7_0, 0x42);
	DEVX_SET(create_mkey_in, in, translations_octword_actual_size, 1);
	DEVX_SET(create_mkey_in, in, pg_access, 1);
	DEVX_SET(create_mkey_in, in, mkey_umem_id, mem->umem_id);

	mr = mlx5dv_devx_obj_create(ctx, in, sizeof(in), out, sizeof(out));
	if (!mr)
		return 0;
	printf("%s:%d \n", __func__, __LINE__);
	return DEVX_GET(create_mkey_out, out, mkey_index) << 8 | 0x42;
}

struct mlx5_eqe_comp {
	__be32	reserved[6];
	__be32	cqn;
};

union ev_data {
	__be32				raw[7];
	struct mlx5_eqe_comp		comp;
};

struct mlx5_eqe {
	u8		rsvd0;
	u8		type;
	u8		rsvd1;
	u8		sub_type;
	__be32		rsvd2[7];
	union ev_data	data;
	__be16		rsvd3;
	u8		signature;
	u8		owner;
};

int create_eq(struct ibv_context *ctx, void **buff_out, uint32_t uar_id);
int create_eq(struct ibv_context *ctx, void **buff_out, uint32_t uar_id)
{
	uint32_t in[DEVX_ST_SZ_DW(create_eq_in) + DEVX_ST_SZ_DW(pas_umem)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(create_eq_out)] = {0};
	struct mlx5_eqe *eqe;
	struct mlx5dv_devx_obj *eq;
	struct mlx5dv_devx_umem *pas;
	uint8_t *buff;
	void *eqc, *up;
	int i;

	buff = (uint8_t *)memalign(0x1000, 0x1000);
	memset(buff, 0, 0x1000);
	for (i = 0; i < (1<<6); i++) {
		eqe = (struct mlx5_eqe *)(buff + i * sizeof(*eqe));
		eqe->owner = 1;
	}

	pas = mlx5dv_devx_umem_reg(ctx, buff, 0x1000, 7);
	if (!pas)
		return 0;

	DEVX_SET(create_eq_in, in, opcode, MLX5_CMD_OP_CREATE_EQ);

	eqc = DEVX_ADDR_OF(create_eq_in, in, eq_context_entry);
	DEVX_SET(eqc, eqc, log_eq_size, 6);
	DEVX_SET(eqc, eqc, uar_page, uar_id);

	up = DEVX_ADDR_OF(create_eq_in, in, pas);
	DEVX_SET(pas_umem, up, pas_umem_id, pas->umem_id);

	eq = mlx5dv_devx_obj_create(ctx, in, sizeof(in), out, sizeof(out));
	if (!eq)
		return 0;

	*buff_out = buff;
	return DEVX_GET(create_eq_out, out, eq_number);
}

#if 1
int create_cq(struct ibv_context *ctx, void **buff_out, struct mlx5dv_devx_uar *uar, uint32_t **dbr_out, uint32_t eq);
int create_cq(struct ibv_context *ctx, void **buff_out, struct mlx5dv_devx_uar *uar, uint32_t **dbr_out, uint32_t eq) {
	uint32_t in[DEVX_ST_SZ_DW(create_cq_in)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(create_cq_out)] = {0};
	struct mlx5_cqe64 *cqe;
	struct mlx5dv_devx_obj *cq;
	struct mlx5dv_devx_umem *pas, *dbrm;
	uint8_t *buff;
	uint8_t *dbr;
	int ret = 0;
	int i;

	buff = (uint8_t *)memalign(0x1000, 0x1000);
	memset(buff, 0, 0x1000);
	for (i = 0; i < (1<<5); i++) {
		cqe = (struct mlx5_cqe64 *)(buff + i * sizeof(*cqe));
		cqe->op_own = MLX5_CQE_INVALID << 4;
	}

	if (!eq)
		ret = mlx5dv_devx_query_eqn(ctx, 0, &eq);
	if (!uar)
		uar = mlx5dv_devx_alloc_uar(ctx, 0);
	pas = mlx5dv_devx_umem_reg(ctx, buff, 0x1000, 7);
	dbr = (uint8_t *)memalign(0x40, 0x948);
	//dbr = memalign(0x1000, 8);
	dbrm = mlx5dv_devx_umem_reg(ctx, dbr, 0x948, 7);

	if (ret || !uar || !pas || !dbr)
		return 0;

	DEVX_SET(create_cq_in, in, opcode, MLX5_CMD_OP_CREATE_CQ);
	DEVX_SET(create_cq_in, in, cq_context.c_eqn, eq);
	DEVX_SET(create_cq_in, in, cq_context.cqe_sz, 0);
	DEVX_SET(create_cq_in, in, cq_context.log_cq_size, 5);
	DEVX_SET(create_cq_in, in, cq_context.uar_page, uar->page_id);
	DEVX_SET(create_cq_in, in, cq_umem_id, pas->umem_id);
	DEVX_SET(create_cq_in, in, cq_context.dbr_umem_id, dbrm->umem_id);
	DEVX_SET64(create_cq_in, in, cq_context.dbr_addr, 0x940);

	cq = mlx5dv_devx_obj_create(ctx, in, sizeof(in), out, sizeof(out));
	if (!cq)
		return 0;

	if (dbr_out)
		*dbr_out = (uint32_t *)(dbr + 0x940);
	if (buff_out)
		*buff_out = buff;

	return DEVX_GET(create_cq_out, out, cqn);
}

int query_lid(struct ibv_context *ctx) {
	uint32_t in[DEVX_ST_SZ_DW(query_hca_vport_context_in)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(query_hca_vport_context_out)] = {0};
	int err;

	DEVX_SET(query_hca_vport_context_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_VPORT_CONTEXT);
	DEVX_SET(query_hca_vport_context_in, in, port_num, 1);

	err = mlx5dv_devx_general_cmd(ctx, in, sizeof(in), out, sizeof(out));
	if (err)
		return -1;

	return DEVX_GET(query_hca_vport_context_out, out, hca_vport_context.lid);
}

enum mlx5_qp_optpar {
	MLX5_QP_OPTPAR_ALT_ADDR_PATH		= 1 << 0,
	MLX5_QP_OPTPAR_RRE			= 1 << 1,
	MLX5_QP_OPTPAR_RAE			= 1 << 2,
	MLX5_QP_OPTPAR_RWE			= 1 << 3,
	MLX5_QP_OPTPAR_PKEY_INDEX		= 1 << 4,
	MLX5_QP_OPTPAR_Q_KEY			= 1 << 5,
	MLX5_QP_OPTPAR_RNR_TIMEOUT		= 1 << 6,
	MLX5_QP_OPTPAR_PRIMARY_ADDR_PATH	= 1 << 7,
	MLX5_QP_OPTPAR_SRA_MAX			= 1 << 8,
	MLX5_QP_OPTPAR_RRA_MAX			= 1 << 9,
	MLX5_QP_OPTPAR_PM_STATE			= 1 << 10,
	MLX5_QP_OPTPAR_RETRY_COUNT		= 1 << 12,
	MLX5_QP_OPTPAR_RNR_RETRY		= 1 << 13,
	MLX5_QP_OPTPAR_ACK_TIMEOUT		= 1 << 14,
	MLX5_QP_OPTPAR_PRI_PORT			= 1 << 16,
	MLX5_QP_OPTPAR_SRQN			= 1 << 18,
	MLX5_QP_OPTPAR_CQN_RCV			= 1 << 19,
	MLX5_QP_OPTPAR_DC_HS			= 1 << 20,
	MLX5_QP_OPTPAR_DC_KEY			= 1 << 21,
};

enum mlx5_qp_state {
	MLX5_QP_STATE_RST			= 0,
	MLX5_QP_STATE_INIT			= 1,
	MLX5_QP_STATE_RTR			= 2,
	MLX5_QP_STATE_RTS			= 3,
	MLX5_QP_STATE_SQER			= 4,
	MLX5_QP_STATE_SQD			= 5,
	MLX5_QP_STATE_ERR			= 6,
	MLX5_QP_STATE_SQ_DRAINING		= 7,
	MLX5_QP_STATE_SUSPENDED			= 9,
	MLX5_QP_NUM_STATE,
	MLX5_QP_STATE,
	MLX5_QP_STATE_BAD,
};

enum {
	MLX5_QP_ST_RC				= 0x0,
	MLX5_QP_ST_UC				= 0x1,
	MLX5_QP_ST_UD				= 0x2,
	MLX5_QP_ST_XRC				= 0x3,
	MLX5_QP_ST_MLX				= 0x4,
	MLX5_QP_ST_DCI				= 0x5,
	MLX5_QP_ST_DCT				= 0x6,
	MLX5_QP_ST_QP0				= 0x7,
	MLX5_QP_ST_QP1				= 0x8,
	MLX5_QP_ST_RAW_ETHERTYPE		= 0x9,
	MLX5_QP_ST_RAW_IPV6			= 0xa,
	MLX5_QP_ST_SNIFFER			= 0xb,
	MLX5_QP_ST_SYNC_UMR			= 0xe,
	MLX5_QP_ST_PTP_1588			= 0xd,
	MLX5_QP_ST_REG_UMR			= 0xc,
	MLX5_QP_ST_MAX
};

enum {
	MLX5_QP_PM_MIGRATED			= 0x3,
	MLX5_QP_PM_ARMED			= 0x0,
	MLX5_QP_PM_REARM			= 0x1
};


enum {
	MLX5_RES_SCAT_DATA32_CQE	= 0x1,
	MLX5_RES_SCAT_DATA64_CQE	= 0x2,
	MLX5_REQ_SCAT_DATA32_CQE	= 0x11,
	MLX5_REQ_SCAT_DATA64_CQE	= 0x22,
};

#define RQ_SIZE (1 << 6)
#define SQ_SIZE (1 << 6)
#define CQ_SIZE (1 << 6)
#define EQ_SIZE (1 << 6)

int create_qp(struct ibv_context *ctx, void **buff_out, struct mlx5dv_devx_uar *uar, uint32_t **dbr_out,
	      int cqn, int pd, struct mlx5dv_devx_obj **q);
int create_qp(struct ibv_context *ctx, void **buff_out, struct mlx5dv_devx_uar *uar, uint32_t **dbr_out,
	      int cqn, int pd, struct mlx5dv_devx_obj **q) {
	u8 in[DEVX_ST_SZ_BYTES(create_qp_in)] = {0};
	u8 out[DEVX_ST_SZ_BYTES(create_qp_out)] = {0};
	struct mlx5dv_devx_umem *pas, *dbrm;
	void *buff, *qpc;
	uint8_t *dbr;

	buff = memalign(0x1000, 0x2000);
	memset(buff, 0, 0x2000);
	pas = mlx5dv_devx_umem_reg(ctx, buff, 0x2000, 7);
	dbr = (uint8_t *)memalign(0x40, 0x948);
	dbrm = mlx5dv_devx_umem_reg(ctx, dbr, 0x948, 7);

	if (!pas || !dbrm)
		return 0;

	DEVX_SET(create_qp_in, in, opcode, MLX5_CMD_OP_CREATE_QP);

	qpc = DEVX_ADDR_OF(create_qp_in, in, qpc);
	DEVX_SET(qpc, qpc, st, MLX5_QP_ST_RC);
	DEVX_SET(qpc, qpc, pm_state, MLX5_QP_PM_MIGRATED);
	DEVX_SET(qpc, qpc, pd, pd);
	DEVX_SET(qpc, qpc, uar_page, uar->page_id);
	DEVX_SET(qpc, qpc, cqn_snd, cqn);
	DEVX_SET(qpc, qpc, cqn_rcv, cqn);
	DEVX_SET(qpc, qpc, log_sq_size, 6);
	DEVX_SET(qpc, qpc, log_rq_stride, 2);
	DEVX_SET(qpc, qpc, log_rq_size, 6);
	DEVX_SET(qpc, qpc, cs_req, MLX5_REQ_SCAT_DATA32_CQE);
	DEVX_SET(qpc, qpc, cs_res, MLX5_RES_SCAT_DATA32_CQE);
	DEVX_SET(create_qp_in, in, wq_umem_id, pas->umem_id);
	DEVX_SET(qpc, qpc, dbr_umem_id, dbrm->umem_id);
	DEVX_SET64(qpc, qpc, dbr_addr, 0x940);

	*q = mlx5dv_devx_obj_create(ctx, in, sizeof(in), out, sizeof(out));
	if (!*q)
		return 0;

	*dbr_out = (uint32_t *)(dbr + 0x940);
	*buff_out = buff;

	return DEVX_GET(create_qp_out, out, qpn);
}

int to_init(struct mlx5dv_devx_obj *obj, int qp) {
	uint32_t in[DEVX_ST_SZ_DW(rst2init_qp_in)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(rst2init_qp_out)] = {0};
	void *qpc = DEVX_ADDR_OF(rst2init_qp_in, in, qpc);

	DEVX_SET(rst2init_qp_in, in, opcode, MLX5_CMD_OP_RST2INIT_QP);
	DEVX_SET(rst2init_qp_in, in, qpn, qp);

	DEVX_SET(qpc, qpc, primary_address_path.vhca_port_num, 1);

	return mlx5dv_devx_obj_modify(obj, in, sizeof(in), out, sizeof(out));
}

int to_rtr(struct mlx5dv_devx_obj *obj, int qp, int type, int lid, uint8_t *gid) {
	uint32_t in[DEVX_ST_SZ_DW(init2rtr_qp_in)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(init2rtr_qp_out)] = {0};
	void *qpc = DEVX_ADDR_OF(rst2init_qp_in, in, qpc);
	uint8_t mac[6];

	mac[0] = gid[8] ^ 0x02;
	mac[1] = gid[9];
	mac[2] = gid[10];
	mac[3] = gid[13];
	mac[4] = gid[14];
	mac[5] = gid[15];

	DEVX_SET(init2rtr_qp_in, in, opcode, MLX5_CMD_OP_INIT2RTR_QP);
	DEVX_SET(init2rtr_qp_in, in, qpn, qp);

	DEVX_SET(qpc, qpc, mtu, 2);
	DEVX_SET(qpc, qpc, log_msg_max, 30);
	DEVX_SET(qpc, qpc, remote_qpn, qp);
	if (type) {
		DEVX_SET(qpc, qpc, primary_address_path.hop_limit, 1);
		memcpy(DEVX_ADDR_OF(qpc, qpc, primary_address_path.rmac_47_32), mac, 6);
	} else {
		DEVX_SET(qpc, qpc, primary_address_path.rlid, lid);
		DEVX_SET(qpc, qpc, primary_address_path.grh, 1);
	}
	memcpy(DEVX_ADDR_OF(qpc, qpc, primary_address_path.rgid_rip), gid,
	       DEVX_FLD_SZ_BYTES(qpc, primary_address_path.rgid_rip));
	DEVX_SET(qpc, qpc, primary_address_path.vhca_port_num, 1);
	DEVX_SET(qpc, qpc, rre, 1);
	DEVX_SET(qpc, qpc, rwe, 1);
	DEVX_SET(qpc, qpc, min_rnr_nak, 12);

	return mlx5dv_devx_obj_modify(obj, in, sizeof(in), out, sizeof(out));
}

int to_rts(struct mlx5dv_devx_obj *obj, int qp) {
	uint32_t in[DEVX_ST_SZ_DW(rtr2rts_qp_in)] = {0};
	uint32_t out[DEVX_ST_SZ_DW(rtr2rts_qp_out)] = {0};
	void *qpc = DEVX_ADDR_OF(rst2init_qp_in, in, qpc);

	DEVX_SET(rtr2rts_qp_in, in, opcode, MLX5_CMD_OP_RTR2RTS_QP);
	DEVX_SET(rtr2rts_qp_in, in, qpn, qp);

	DEVX_SET(qpc, qpc, log_ack_req_freq, 8);
	DEVX_SET(qpc, qpc, retry_count, 7);
	DEVX_SET(qpc, qpc, rnr_retry, 7);
	DEVX_SET(qpc, qpc, primary_address_path.ack_timeout, 14);

	return mlx5dv_devx_obj_modify(obj, in, sizeof(in), out, sizeof(out));
}

int recv(uint8_t *rq, uint32_t *rqi, uint32_t *qp_dbr,
	 uint32_t mkey, void *addr, size_t size) {
	struct mlx5_wqe_data_seg *dseg = (struct mlx5_wqe_data_seg *)(rq + *rqi % RQ_SIZE * MLX5_SEND_WQE_BB);
	mlx5dv_set_data_seg(dseg, size, mkey, (intptr_t)addr);
	mlx5dv_set_data_seg(dseg + 1, 0, MLX5_INVALID_LKEY, 0);
	(*rqi)++;
	asm volatile("" ::: "memory");
	qp_dbr[MLX5_RCV_DBR] = htobe32(*rqi & 0xffff);
	return 0;
}

int xmit(uint8_t *sq, uint32_t *sqi, uint32_t *qp_dbr,
	 uint32_t mkey, void *addr, size_t size,
	 void *uar_ptr, uint32_t qp) {
	struct mlx5_wqe_ctrl_seg *ctrl = (struct mlx5_wqe_ctrl_seg *)(sq + *sqi % SQ_SIZE * MLX5_SEND_WQE_BB);
	mlx5dv_set_ctrl_seg(ctrl, *sqi, MLX5_OPCODE_SEND, 0,
			    qp, MLX5_WQE_CTRL_CQ_UPDATE,
			    2, 0, 0);
	struct mlx5_wqe_data_seg *dseg = (struct mlx5_wqe_data_seg *)(ctrl + 1);
	mlx5dv_set_data_seg(dseg, size, mkey, (intptr_t)addr);
	(*sqi)++;
	asm volatile("" ::: "memory");
	qp_dbr[MLX5_SND_DBR] = htobe32(*sqi & 0xffff);
	asm volatile("" ::: "memory");
	*(uint64_t *)((uint8_t *)uar_ptr + 0x800) = *(uint64_t *)ctrl;
	asm volatile("" ::: "memory");
	return 0;
}

enum {
	MLX5_CQ_SET_CI	= 0,
	MLX5_CQ_ARM_DB	= 1,
};

int arm_cq(uint32_t cq, uint32_t cqi, uint32_t *cq_dbr, void *uar_ptr) {
#if HAS_EQ_SUPPORT
	uint64_t doorbell;
	uint32_t sn;
	uint32_t ci;
	uint32_t cmd;

	sn  = cqi & 3;
	ci  = cqi & 0xffffff;
	cmd = MLX5_CQ_DB_REQ_NOT;

	doorbell = sn << 28 | cmd | ci;
	doorbell <<= 32;
	doorbell |= cq;

	cq_dbr[MLX5_CQ_ARM_DB] = htobe32(sn << 28 | cmd | ci);
	asm volatile("" ::: "memory");

	*(uint64_t *)((uint8_t *)uar_ptr + 0x20) = htobe64(doorbell);
	asm volatile("" ::: "memory");
#endif
	return 0;
}

int poll_cq(uint8_t *cq_buff, uint32_t *cqi, uint32_t *cq_dbr) {
	struct mlx5_cqe64 *cqe = (struct mlx5_cqe64 *)(cq_buff + *cqi % CQ_SIZE * sizeof(*cqe));
	int retry = 1600000;

	while (--retry && (mlx5dv_get_cqe_opcode(cqe) == MLX5_CQE_INVALID ||
		((cqe->op_own & MLX5_CQE_OWNER_MASK) ^ !!(*cqi & CQ_SIZE))))
		asm volatile("" ::: "memory");

	if (!retry)
		return 1;

	(*cqi)++;
	asm volatile("" ::: "memory");
	cq_dbr[MLX5_CQ_SET_CI] = htobe32(*cqi & 0xffffff);
	printf("CQ op %d size %x\n", mlx5dv_get_cqe_opcode(cqe), be32toh(cqe->byte_cnt));
	return 0;
}

int poll_eq(uint8_t *eq_buff, uint32_t *eqi, int expected) {
#if HAS_EQ_SUPPORT
	struct mlx5_eqe *eqe = (struct mlx5_eqe *)(eq_buff + *eqi % EQ_SIZE * sizeof(*eqe));
	int retry = 1600000;
	while (--retry && (eqe->owner & 1) ^ !!(*eqi & EQ_SIZE))
		asm volatile("" ::: "memory");

	if (!retry)
		return 1;

	(*eqi)++;
	asm volatile("" ::: "memory");
	printf("EQ cq %x\n", be32toh(eqe->data.comp.cqn));
	return 0;
#else
	return expected;
#endif
}

int arm_eq(uint32_t eq, uint32_t eqi, void *uar_ptr) {
#if HAS_EQ_SUPPORT
	uint32_t doorbell = (eqi & 0xffffff) | (eq << 24);

	*(uint32_t *)((uint8_t *)uar_ptr + 0x48) = htobe32(doorbell);
	asm volatile("" ::: "memory");
#endif
	return 0;
}
#endif

#include "env.h"

TEST(devx, smoke) {
	int num, devn = 0;
	struct ibv_device **list = ibv_get_device_list(&num);
	struct ibv_context *ctx;
	struct mlx5dv_context_attr attr = {};

	if (getenv("DEVN"))
		devn = atoi(getenv("DEVN"));

	attr.flags = MLX5DV_CONTEXT_FLAGS_DEVX;

	ASSERT_GT(num, devn);
	ctx = mlx5dv_open_device(list[devn], &attr);
	ASSERT_TRUE(ctx);
	ibv_free_device_list(list);

	EXPECT_LE(0, query_device(ctx));
}

static int read_file(const char *dir, const char *file,
		     char *buf, size_t size)
{
	char *path;
	int fd;
	size_t len;

	if (asprintf(&path, "%s/%s", dir, file) < 0)
		return -1;

	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		free(path);
		return -1;
	}

	len = read(fd, buf, size);

	close(fd);
	free(path);

	if (len > 0) {
		if (buf[len - 1] == '\n')
			buf[--len] = '\0';
		else if (len < size)
			buf[len] = '\0';
		else
			return -1;
	}

	return len;
}

int devx_query_gid(struct ibv_context *ctx, uint8_t port_num,
		   int index, uint8_t *gid)
{
	char name[24];
	char attr[41];
	uint16_t val;
	int i;

	snprintf(name, sizeof name, "ports/%d/gids/%d", port_num, index);

	if (read_file(ctx->device->ibdev_path, name, attr, sizeof(attr)) < 0)
		return -1;

	for (i = 0; i < 8; ++i) {
		if (sscanf(attr + i * 5, "%hx", &val) != 1)
			return -1;
		gid[i * 2    ] = val >> 8;
		gid[i * 2 + 1] = val & 0xff;
	}

	return 0;
}

TEST(devx, gid) {
	int num, devn = 0;
	struct ibv_device **list = ibv_get_device_list(&num);
	struct ibv_context *ctx;
	struct mlx5dv_context_attr attr = {};

	if (getenv("DEVN"))
		devn = atoi(getenv("DEVN"));

	attr.flags = MLX5DV_CONTEXT_FLAGS_DEVX;

	ASSERT_GT(num, devn);
	ctx = mlx5dv_open_device(list[devn], &attr);
	ASSERT_TRUE(ctx);
	ibv_free_device_list(list);

	uint8_t gid[16];
	ASSERT_FALSE(devx_query_gid(ctx, 1, 0, gid));
}

#if 1
TEST(devx, send) {
	int num, devn = 0;
	struct ibv_device **list = ibv_get_device_list(&num);
	struct ibv_context *ctx;
	struct mlx5dv_context_attr attr = {};
	int lid, type;
	unsigned char buff[0x1000];
	for(int i = 0; i < 0x60; i++)
		buff[i] = i + 0x20;

	if (getenv("DEVN"))
		devn = atoi(getenv("DEVN"));

	attr.flags = MLX5DV_CONTEXT_FLAGS_DEVX;

	ASSERT_GT(num, devn);
	ctx = mlx5dv_open_device(list[devn], &attr);
	ASSERT_TRUE(ctx);
	ibv_free_device_list(list);

	struct mlx5dv_devx_uar *uar;
	uar = mlx5dv_devx_alloc_uar(ctx, 0);
	ASSERT_TRUE(uar);

	int pd = alloc_pd(ctx);
	ASSERT_TRUE(pd);

	void *eq_buff;
	int eq = 0;
#if HAS_EQ_SUPPORT
	ASSERT_TRUE(eq = create_eq(ctx, &eq_buff, uar->page_id));
#endif

	void *cq_buff;
	uint32_t *cq_dbr;
	int cq = create_cq(ctx, &cq_buff, uar, &cq_dbr, eq);
	ASSERT_TRUE(cq);
	int mkey = reg_mr(ctx, pd, buff, sizeof(buff));
	ASSERT_TRUE(mkey);

	type = query_device(ctx);
	EXPECT_LE(0, query_device(ctx));

	uint8_t gid[16];
	ASSERT_FALSE(devx_query_gid(ctx, 1, 0, gid));

	if (!type) {
		lid = query_lid(ctx);
		ASSERT_LE(0, lid);
	} else {
		lid = 0;
	}

	void *qp_buff;
	uint32_t *qp_dbr;
	struct mlx5dv_devx_obj *q;
	int qp = create_qp(ctx, &qp_buff, uar, &qp_dbr, cq, pd, &q);
	ASSERT_TRUE(qp);
	ASSERT_FALSE(to_init(q, qp));
	ASSERT_FALSE(to_rtr(q, qp, type, lid, gid));
	ASSERT_FALSE(to_rts(q, qp));

	uint8_t *rq = (uint8_t *)qp_buff;
	uint8_t *sq = (uint8_t *)qp_buff + MLX5_SEND_WQE_BB * RQ_SIZE;
	uint32_t rqi = 0, sqi = 0, cqi = 0, eqi = 0;

	ASSERT_FALSE(arm_eq(eq, eqi, uar->base_addr));
	ASSERT_FALSE(arm_cq(cq, cqi, cq_dbr, uar->base_addr));

	ASSERT_TRUE(poll_eq((uint8_t *)eq_buff, &eqi, 1));
	ASSERT_TRUE(poll_cq((uint8_t *)cq_buff, &cqi, cq_dbr));

	ASSERT_FALSE(recv(rq, &rqi, qp_dbr, mkey, buff, 0x30));
	ASSERT_FALSE(xmit(sq, &sqi, qp_dbr, mkey, buff + 0x30, 0x30, uar->base_addr, qp));

	ASSERT_FALSE(poll_eq((uint8_t *)eq_buff, &eqi, 0));
	ASSERT_FALSE(poll_cq((uint8_t *)cq_buff, &cqi, cq_dbr));
	ASSERT_FALSE(arm_eq(eq, eqi, uar->base_addr));
	ASSERT_FALSE(arm_cq(cq, cqi, cq_dbr, uar->base_addr));
	ASSERT_FALSE(poll_eq((uint8_t *)eq_buff, &eqi, 0));
	ASSERT_FALSE(poll_cq((uint8_t *)cq_buff, &cqi, cq_dbr));
	ASSERT_FALSE(arm_eq(eq, eqi, uar->base_addr));
	ASSERT_FALSE(arm_cq(cq, cqi, cq_dbr, uar->base_addr));
}
#endif

