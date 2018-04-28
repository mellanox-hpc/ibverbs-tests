#include<stdint.h>
#include<infiniband/verbs_exp.h>

/*******************************************************************************************
 * Test behavior definitions
 * 
 * REUSE_SIG_MR - If this is set to 1, we will issue
 * a IBV_EXP_WR_REG_SIG_MR multiple times with the same SIG_MR.
 *
 * REREG_SIG_MR - If this is set to 1 we will issue a IBV_EXP_WR_REG_SIG_MR
 * WR before each RDMA.
 *
 * *** NOTE - Please run with the following three settings ***
 *
 * Test 1: REUSE_SIG_MR=1, REREG_SIG_MR=1 - This test illustrates my first issue - How do I reuse a SIG MR?
 *  This will fail with EINVAL on the second IBV_EXP_WR_REG_SIG_MR WR.
 *  
 * Test 2: REUSE_SIG_MR=0, REREG_SIG_MR=1 - This is my second issue - In this case we will allocate a new SIG MR for each 
 *  IBV_EXP_WR_REG_SIG_MR WR to get around the first problem.  This test fails with a local QP operation error
 *  on the 146th attempt.  What is causing this failure?
 *
 * Test 3: REUSE_SIG_MR=0, REREG_SIG_MR=0 - This test reissues the same RDMA without re-registering the SIG MR.  This
 *  runs for the full 500 loops without error
 *******************************************************************************************/ 
#define REUSE_SIG_MR        1
#define REREG_SIG_MR        1

/*******************************************************************************************
 * Variables that need to be set by Mellanox:
 *
 * I wrote this test in the context of our NVMf target code (read request) to avoid having to write the code
 * to set up the RDMA QP and get a remote address/rkey.  The following variables are extracted from
 * the NVMF/request.  To run this code, you will need to set these variables accordingly.
 *******************************************************************************************/
struct ibv_pd               *pd;            // PD associated with port that request arrived on
struct ibv_qp               *qp;            // QP associated with connection
uint64_t                    remote_addr;    // Taken from NVMF command
uint32_t                    rkey;           // Taken from NVMF command
// This array is used in our NVMF code to manage protection domains based
// on HCA index.
//extern nvmf_rdma_hcaid_st   nvmf_hcaids[];

// Test phases
#define EXEC_SIG_MR         1
#define EXEC_RDMA           2
#define EXEC_INVAL_MR       3

// Buffer size
#define TEST_BUF_SIZE       0x100000

// Test Globals
void                        *buf;
struct ibv_mr               *mr, *sigmr;
struct ibv_exp_sig_attrs    sig_attrs;
struct ibv_sge              sig_sgl;
struct ibv_sge              rdma_sgl;
int                         iterations = 0;
int			    done = 0;

// prototypes
void mellanox_test(ibv_pd *pd, ibv_qp *qp, ibv_sge sge);
void mellanox_exec_test(int phase);
void mellanox_test_completion(struct ibv_wc  *wc);
void mellanox_reg_sig_mr(void);
void mellanox_single_rdma(void);
void mellanox_invalidate_sig_mr(void);

/*******************************************************************************************
 * Test entry point
 *
 * Extract global variables from NVMF structures, allocate resources and begin test
 *******************************************************************************************/
void
mellanox_test(ibv_pd *_pd, ibv_qp *_qp, ibv_sge sge)
{
    struct ibv_exp_create_mr_in     mr_in;

    // Extract fields from the nvmf_request structure into globals
    pd = _pd;
    qp = _qp;
    remote_addr = sge.addr;
    rkey = sge.lkey;
    // Allocate buffer
    buf = malloc(TEST_BUF_SIZE);
    assert(buf);
    // register memory region
    mr = ibv_reg_mr(pd,
                    (void*)buf,
                    TEST_BUF_SIZE,
                    IBV_ACCESS_LOCAL_WRITE  |
                    IBV_ACCESS_REMOTE_WRITE |
                    IBV_ACCESS_REMOTE_READ);    
    assert(mr);
    // Allocate one SIG MR
    mr_in.pd = pd;
    mr_in.comp_mask = 0;
    mr_in.attr.max_klm_list_size = 1;
    mr_in.attr.create_flags = IBV_EXP_MR_SIGNATURE_EN;
    mr_in.attr.exp_access_flags = (IBV_ACCESS_LOCAL_WRITE  |
                                   IBV_ACCESS_REMOTE_WRITE |
                                   IBV_ACCESS_REMOTE_READ);
    sigmr = ibv_exp_create_mr(&mr_in);
    assert(sigmr);

    memset(&sig_attrs, 0, sizeof(sig_attrs));
    // Set up SIG ATTRS
	#define CHECK_REF_TAG 0x0f
	#define CHECK_APP_TAG 0x30
	#define CHECK_GUARD   0xc0
    //sig_attrs.check_mask |= CHECK_REF_TAG;
    //sig_attrs.check_mask |= CHECK_APP_TAG;
    //sig_attrs.check_mask |= CHECK_GUARD;
    sig_attrs.mem.sig_type = IBV_EXP_SIG_TYPE_T10_DIF;
    sig_attrs.mem.sig.dif.bg_type = IBV_EXP_T10DIF_CRC;
    sig_attrs.mem.sig.dif.pi_interval = 512;
    sig_attrs.mem.sig.dif.bg = 0;
    sig_attrs.mem.sig.dif.app_tag = 0;
    sig_attrs.mem.sig.dif.ref_tag = 0;
    sig_attrs.mem.sig.dif.ref_remap = 1;   // reftag - Increment each block
    sig_attrs.mem.sig.dif.app_escape = 1;
    sig_attrs.mem.sig.dif.ref_escape = 1;
    sig_attrs.mem.sig.dif.apptag_check_mask = 0xFFFF;
    sig_attrs.wire.sig_type = IBV_EXP_SIG_TYPE_NONE;
    //sig_attrs.wire.sig.dif.bg_type = 0;
    sig_attrs.wire.sig.dif.pi_interval = 0;
    sig_attrs.wire.sig.dif.bg = 0;
    sig_attrs.wire.sig.dif.app_tag = 0;
    sig_attrs.wire.sig.dif.ref_tag = 0;
    sig_attrs.wire.sig.dif.ref_remap = 0;
    sig_attrs.wire.sig.dif.app_escape = 0;
    sig_attrs.wire.sig.dif.ref_escape = 0;
    sig_attrs.wire.sig.dif.apptag_check_mask = 0;
    // Call exec to register the SIG MR
    mellanox_exec_test(EXEC_SIG_MR);
}

/*******************************************************************************************
 * This routine is called at the start of the test and then upon every 
 * completion until it fails or we run 500 times
 *******************************************************************************************/
void
mellanox_exec_test(int phase)
{
    if(iterations < 455) {
        if(phase == EXEC_SIG_MR) {
            // Phase 1 - register SIG MR
            printf("Loop %d: Reg SIG MR\n", iterations);
            mellanox_reg_sig_mr();
	} else if (phase == EXEC_INVAL_MR) {
            printf("Loop %d: Inval SIG MR\n", iterations);
            mellanox_invalidate_sig_mr();
        } else {
            // Phase 2 - RDMA
            printf("Loop %d: RDMA\n", iterations);
            mellanox_single_rdma();
            iterations++;
        }
        // Return.  We will resume testing in the completion handler.
    } else {
        printf("done\n");
	done = 1;
    }
}

/*******************************************************************************************
 * This routine is called from our NVMF CQ processing code (ibv_poll_cq)
 * The 0xABCD at the top of the wr_id identifies the 
 * WC as a mellanox test operation.
 *
 * Note - Our code runs in single-threaded polling mode, so we can't simply loop and
 * sleep waiting for a completion.  Instead we restart the test execution from this
 * completion handler.
 *******************************************************************************************/
void
mellanox_test_completion(struct ibv_wc  *wc)
{
    if(wc->status) {
        printf("Failure: Status %d:%s\n", wc->status, ibv_wc_status_str(wc->status));
        assert(0);
    }
    if((wc->wr_id & 0xF) == 0x1) {
        // Bottom bits set to 0x1 for SIG MR operation
        // Restart the test with Phase 2 (RDMA)
        printf("reg sig mr done\n");
        mellanox_exec_test(2);
    } else if((wc->wr_id & 0xF) == 0x2){
        printf("rdma %d complete\n", iterations);
#if REREG_SIG_MR
        // Re-register the SIG MR
        mellanox_exec_test(EXEC_SIG_MR);
#else
        // Just RDMA over and over again
        mellanox_exec_test(EXEC_RDMA);
#endif
    } else {
        mellanox_exec_test(EXEC_INVAL_MR);
    }
}

/*******************************************************************************************
 * Issue a IBV_EXP_WR_REG_SIG_MR work request
 *******************************************************************************************/
void
mellanox_reg_sig_mr(void)
{
    int                             rc;
    struct ibv_exp_send_wr          ewr, *bad_ewr = NULL;

    // If REUSE_SIG_MR is set, then we use the one sigmr that
    // was allocated at initialization time.  If not, then we'll
    // allocate a new one on every iteration
#if !REUSE_SIG_MR
    // Allocate SIG MR
    struct ibv_exp_create_mr_in     mr_in;
    mr_in.pd = pd;
    mr_in.comp_mask = 0;
    mr_in.attr.max_klm_list_size = 1;
    mr_in.attr.create_flags = IBV_EXP_MR_SIGNATURE_EN;
    mr_in.attr.exp_access_flags = (IBV_ACCESS_LOCAL_WRITE  |
                                   IBV_ACCESS_REMOTE_WRITE |
                                   IBV_ACCESS_REMOTE_READ);
    sigmr = ibv_exp_create_mr(&mr_in);
    assert(sigmr);
#endif

    memset(&ewr, 0, sizeof(struct ibv_exp_send_wr));
    // Set up sig SGL
    // addr - set to allocated buffer
    // lkey - set to MR registered at initialization
    // length - Full buffer length
    sig_sgl.addr = (intptr_t)buf;
    sig_sgl.lkey = mr->lkey;
    sig_sgl.length = TEST_BUF_SIZE;
    // Set up SIG_MR work request
    // Set the top bits of the wr_id to 0xABCD to identify it as
    // a test operation, and set the bottom nibble to identify it
    // as a REG_SIG_MR
    ewr.wr_id = 0xABCD0000 | 0x1;
    ewr.sg_list = &sig_sgl;
    ewr.ext_op.sig_handover.sig_mr = sigmr;
    ewr.ext_op.sig_handover.sig_attrs = &sig_attrs;
    ewr.ext_op.sig_handover.access_flags = (IBV_ACCESS_LOCAL_WRITE  |
                                           IBV_ACCESS_REMOTE_WRITE |
                                           IBV_ACCESS_REMOTE_READ);
    ewr.ext_op.sig_handover.prot = NULL;
    ewr.num_sge = 1;
    ewr.wr.rdma.rkey = 0;
    ewr.wr.rdma.remote_addr = 0;
    ewr.exp_opcode = IBV_EXP_WR_REG_SIG_MR;
    ewr.exp_send_flags = IBV_SEND_SIGNALED;
    // Issue the WR and return.  Testing will restart upon completion of
    // this WR.
    rc = ibv_exp_post_send(qp, &ewr, &bad_ewr);
    assert(rc == 0);
}

void
mellanox_invalidate_sig_mr(void)
{
    int                             rc;
    struct ibv_exp_send_wr          ewr, *bad_ewr = NULL;
    struct ibv_exp_mr_status status;

    ibv_exp_check_mr_status(sigmr, IBV_EXP_MR_CHECK_SIG_STATUS, &status);

    memset(&ewr, 0, sizeof(struct ibv_exp_send_wr));
    // Set the top bits of the wr_id to 0xABCD to identify it as
    // a test operation, and set the bottom nibble to identify it
    // as a INV_SIG_MR
    ewr.wr_id = 0xABCD0000 | 0x2;
    ewr.ex.invalidate_rkey = sigmr->rkey;
    ewr.exp_opcode = IBV_EXP_WR_LOCAL_INV;
    ewr.exp_send_flags = IBV_SEND_SIGNALED;
    // Issue the WR and return.  Testing will restart upon completion of
    // this WR.
    rc = ibv_exp_post_send(qp, &ewr, &bad_ewr);
    assert(rc == 0);
}

/*******************************************************************************************
 * Issue a WRITE RDMA using the SIG MR that we set up.
 *******************************************************************************************/
void
mellanox_single_rdma(void)
{
    int                             rc;
    struct ibv_send_wr              wr, *bad_wr = NULL;

    // Set up RDMA SGL
    // The address is relative to the address in the SIG MR, which means zero.
    // The length excludes the T10-DIF fields
    // The lkey is the lkey from the corresponding SIG MR
    rdma_sgl.addr = (uint64_t)0;
    rdma_sgl.length = 8192;
    rdma_sgl.lkey = sigmr->lkey;

    // Set up WR
    memset(&wr, 0, sizeof(struct ibv_send_wr));
    // Set the top bits of the wr_id to 0xABCD to identify it as
    // a test operation
    wr.wr_id = 0xABCD0000;
    wr._wr_opcode = IBV_WR_RDMA_WRITE;
    wr.sg_list = &rdma_sgl;
    wr.num_sge = 1;
    wr.wr.rdma.rkey = rkey;
    wr.wr.rdma.remote_addr = remote_addr;
    wr._wr_send_flags = IBV_SEND_SIGNALED;
    rc = ibv_post_send(qp, &wr, &bad_wr);
    assert(rc == 0);
}

TEST_F(sig_test, ext0) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->t10dif()));
	EXEC(send_qp.rdma(this->insert_mr.sge(0,SZD), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());

	ibvt_wc wc(cq);
	mellanox_test(this->pd.pd, this->send_qp.qp, this->mid_mr.sge());
	while(!done) {
		this->cq.do_poll(wc);
		mellanox_test_completion(&wc.wc);
	}

}

