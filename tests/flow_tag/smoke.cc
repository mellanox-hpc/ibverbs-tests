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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define __STDC_LIMIT_MACROS
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <infiniband/verbs.h>
#include "env.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <malloc.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include </usr/include/netinet/ip.h>
#include <poll.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define MY_DEST_MAC0    0x01
#define MY_DEST_MAC1    0x00
#define MY_DEST_MAC2    0x5e
#define MY_DEST_MAC3    0x29
#define MY_DEST_MAC4    0x23
#define MY_DEST_MAC5    0x4F

#define MY_DEST_MAC7    0x7c
#define MY_DEST_MAC8    0xef
#define MY_DEST_MAC9    0x90
#define MY_DEST_MAC6    0x67
#define SRC_PORT    	1337
#define DST_PORT    	4789

#define BUF_SIZ         96
#define SZ 96
typedef unsigned short u16;
typedef unsigned long u32;


enum {
        PINGPONG_RECV_WRID = 1,
        PINGPONG_SEND_WRID = 2,
};

#define UDP_DEST_PORT 0x12b5
#define UDP_SRC_PORT 0x539
#define IP_DEST 0x0b87d10a 
#define IP_SRC 0x0b87d114  
#define TYP_INIT 0
#define TYP_SMLE 1
#define TYP_BIGE 2

/* Macro for allocating. */
#define ALLOCATE(var,type,size)                                     \
{ if((var = (type*)malloc(sizeof(type)*(size))) == NULL)        \
        { fprintf(stderr," Cannot Allocate\n"); exit(1);}}

unsigned short get_csum(unsigned short *buf, int nwords)
{
    unsigned long sum;
    for(sum=0; nwords>0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum &0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

uint16_t get_udp_checksum(const void *buff, size_t len)
{
	char src_addr[15], dest_addr[15];
        const uint16_t *buf=(const uint16_t*)buff;

        src_addr[0] = 0x0b; src_addr[1] = 0x87; src_addr[2] = 0xd1; src_addr[3] = 0x0a;
        dest_addr[0] = 0x0b; dest_addr[1] = 0x87; dest_addr[2] = 0xd1; dest_addr[3] = 0x14;
        uint16_t *ip_src=(uint16_t*)&src_addr, *ip_dst=(uint16_t*)&dest_addr;
        uint32_t sum;
        size_t length=len;
        sum = 0;
        while (len > 1)
        {
                sum += *buf++;
                if (sum & 0x80000000)
                        sum = (sum & 0xFFFF) + (sum >> 16);
                len -= 2;
        }

        if ( len & 1 )
                sum += *((uint8_t *)buf);
        sum += *(ip_src++);
        sum += *ip_src;

        sum += *(ip_dst++);
        sum += *ip_dst;

        sum += htons(IPPROTO_UDP);
        sum += htons(length);
        while (sum >> 16)
                sum = (sum & 0xFFFF) + (sum >> 16);
        return ( (uint16_t)(~sum)  );
}

int flow_calc_flow_rules_size()
{
        int tot_size = sizeof(struct ibv_flow_attr);
        tot_size += sizeof(struct ibv_flow_spec_eth);
                tot_size += sizeof(struct ibv_flow_spec_ipv4);
                tot_size += sizeof(struct ibv_flow_spec_tcp_udp);

        return tot_size;
}

struct ibvt_ctx_eth : public ibvt_ctx {
	ibvt_ctx_eth(ibvt_env& e) : ibvt_ctx(e) { }


	virtual bool check_port(struct ibv_port_attr &port_attr ){
		if ((port_attr.state == IBV_PORT_ACTIVE) &&  (port_attr.link_layer == IBV_LINK_LAYER_ETHERNET) )
			return true;
		return false;
	}

};

struct ibvt_raw_qp : public ibvt_qp {
	 ibvt_raw_qp(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) : ibvt_qp(e, p, c) {}

	uintptr_t start;
	size_t length;
        struct ibv_flow	*flow_create_result;

	virtual void init() { 
	
		EXEC(pd.init());
                EXEC(cq.init());
		struct ibv_qp_init_attr_ex attr;
		memset(&attr, 0, sizeof(attr));
                init_attr(attr);
                attr.sq_sig_all = 1;
                attr.cap.max_send_wr = 50;
                attr.cap.max_recv_wr = 50;
                attr.cap.max_send_sge = 1;
                attr.cap.max_recv_sge = 1;
                attr.send_cq = cq.cq;
                attr.recv_cq = cq.cq;
                attr.pd = pd.pd;
                attr.comp_mask = IBV_QP_INIT_ATTR_PD;
		attr.qp_type = IBV_QPT_RAW_PACKET;
		SET(qp, ibv_create_qp_ex(pd.ctx.ctx , &attr)); 

	}

	virtual void connect(ibvt_qp *r) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_INIT;
		attr.port_num = pd.ctx.port_num;
		attr.pkey_index = 0;
		attr.qkey            = 0x11111111;
		attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		flags = IBV_QP_STATE | IBV_QP_PORT ;
		DO(ibv_modify_qp(qp, &attr, flags));

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTR;
		flags = IBV_QP_STATE ;
		DO(ibv_modify_qp(qp, &attr, flags));

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTS;
		attr.timeout = 14;
		attr.retry_cnt = 7;
		attr.rnr_retry = 7;
		attr.sq_psn = 0;
		attr.max_rd_atomic = 1;
		flags = IBV_QP_STATE ;

		DO(ibv_modify_qp(qp, &attr, flags));
	}

	void flow_tag_create_rules( struct ibv_qp  *qp,
                struct ibv_flow_attr *flow_rules) {

                SET(flow_create_result,ibv_create_flow(qp, flow_rules));
	}

	void vxlan_destroy_rules() {

		DO(ibv_destroy_flow(flow_create_result)); 
	}

	void set_up_flow_rules(
                struct ibv_flow_attr **flow_rules) {
#ifdef HAVE_VXLAN
        struct ibv_flow_spec* spec_info;
        struct ibv_flow_attr* attr_info;
	struct ibv_flow_spec_action_tag* flow_tag_ptr;

        void* header_buff;
        int flow_rules_size;
        int flow_tag = 1;

        flow_rules_size = flow_calc_flow_rules_size();
	if (flow_tag) {
		flow_rules_size += sizeof(struct ibv_flow_spec_action_tag);
	}


        ALLOCATE(header_buff, uint8_t, flow_rules_size);
        memset(header_buff, 0, flow_rules_size);
        *flow_rules = (struct ibv_flow_attr*)header_buff;
        attr_info = (struct ibv_flow_attr*)header_buff;

        attr_info->size = flow_rules_size;
        attr_info->priority = 0;
        attr_info->num_of_specs = 3;
	if (flow_tag)
                        attr_info->num_of_specs += 1;

        attr_info->flags = 0;

        attr_info->type = IBV_FLOW_ATTR_NORMAL;
        header_buff = (char*)header_buff + sizeof(struct ibv_flow_attr);
        spec_info = (struct ibv_flow_spec*)header_buff; //ibv_exp_flow_spec
        spec_info->eth.type = IBV_FLOW_SPEC_ETH;
        spec_info->eth.size = sizeof(struct ibv_flow_spec_eth);
        spec_info->eth.val.ether_type = 0;

        spec_info->eth.val.dst_mac[0] = MY_DEST_MAC0;
        spec_info->eth.val.dst_mac[1] = MY_DEST_MAC1;
        spec_info->eth.val.dst_mac[2] = MY_DEST_MAC2;
        spec_info->eth.val.dst_mac[3] = MY_DEST_MAC3; 
        spec_info->eth.val.dst_mac[4] = MY_DEST_MAC4; 
        spec_info->eth.val.dst_mac[5] = MY_DEST_MAC5; 
        memset(spec_info->eth.mask.dst_mac, 0xFF,sizeof(spec_info->eth.mask.dst_mac));
        spec_info->eth.val.src_mac[0] = MY_DEST_MAC7; 
        spec_info->eth.val.src_mac[1] = MY_DEST_MAC8; 
        spec_info->eth.val.src_mac[2] = MY_DEST_MAC9; 
        spec_info->eth.val.src_mac[3] = MY_DEST_MAC3; 
        spec_info->eth.val.src_mac[4] = MY_DEST_MAC4; 
        spec_info->eth.val.src_mac[5] = MY_DEST_MAC6; 
        memset(spec_info->eth.mask.src_mac, 0xFF,sizeof(spec_info->eth.mask.src_mac));

        spec_info->eth.val.ether_type = htons(0x0800);
        spec_info->eth.mask.ether_type = 0xffff;
        memset((void*)&spec_info->eth.mask.ether_type, 0xFF,sizeof(spec_info->eth.mask.ether_type));
        memset(spec_info->eth.mask.dst_mac, 0xFF,sizeof(spec_info->eth.mask.src_mac));

	header_buff = (char*)header_buff + sizeof(struct ibv_flow_spec_eth);
	spec_info = (struct ibv_flow_spec*)header_buff;
	spec_info->ipv4.type = IBV_FLOW_SPEC_IPV4;
	spec_info->ipv4.size = sizeof(struct ibv_flow_spec_ipv4);

        memset((void*)&spec_info->ipv4.mask.dst_ip, 0xFF,sizeof(spec_info->ipv4.mask.dst_ip));
        memset((void*)&spec_info->ipv4.mask.src_ip, 0xFF,sizeof(spec_info->ipv4.mask.src_ip));
        spec_info->ipv4.val.dst_ip = htonl(IP_DEST);
        spec_info->ipv4.val.src_ip = htonl(IP_SRC);
        memset((void*)&spec_info->ipv4.mask.dst_ip, 0xFF,sizeof(spec_info->ipv4.mask.dst_ip));
        memset((void*)&spec_info->ipv4.mask.src_ip, 0xFF,sizeof(spec_info->ipv4.mask.src_ip));

        header_buff = (char*)header_buff + sizeof(struct ibv_flow_spec_ipv4);
        spec_info = (struct ibv_flow_spec*)header_buff;
        spec_info->tcp_udp.type = IBV_FLOW_SPEC_UDP;
        spec_info->tcp_udp.size = sizeof(struct ibv_flow_spec_tcp_udp);


        memset((void*)&spec_info->tcp_udp.mask.dst_port, 0xFF,sizeof(spec_info->ipv4.mask.dst_ip));
        memset((void*)&spec_info->tcp_udp.mask.src_port, 0xFF,sizeof(spec_info->ipv4.mask.src_ip));
        spec_info->tcp_udp.val.dst_port = htons(UDP_DEST_PORT);
        spec_info->tcp_udp.val.src_port = htons(UDP_SRC_PORT);
        memset((void*)&spec_info->tcp_udp.mask.dst_port, 0xFF,sizeof(spec_info->ipv4.mask.dst_ip));
        memset((void*)&spec_info->tcp_udp.mask.src_port, 0xFF,sizeof(spec_info->ipv4.mask.src_ip));
	if (flow_tag) {
                        header_buff = (char*)header_buff + sizeof(struct ibv_flow_spec_tcp_udp); //TODO fix
                        flow_tag_ptr = (struct ibv_flow_spec_action_tag*)header_buff;
                        flow_tag_ptr->type =  IBV_FLOW_SPEC_ACTION_TAG;
                        flow_tag_ptr->size = sizeof(struct ibv_flow_spec_action_tag);
                        flow_tag_ptr->tag_id = 507;
        }
	#endif
}
	
	void send_raw_packet(void* buf,int match)
	{
        	int tx_len = 0;
        	char sendbuf[BUF_SIZ];
        	struct ether_header *eh = (struct ether_header *) sendbuf;
        	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));

        	/* Construct the Ethernet header */
        	memset(sendbuf, 0, BUF_SIZ);
        	/* Ethernet header */
        	eh->ether_shost[0] = MY_DEST_MAC7;
        	eh->ether_shost[1] = MY_DEST_MAC8;
        	eh->ether_shost[2] = MY_DEST_MAC9;
        	eh->ether_shost[3] = MY_DEST_MAC3;
        	eh->ether_shost[4] = MY_DEST_MAC4;
        	eh->ether_shost[5] = MY_DEST_MAC6;
        	if (match){
                	eh->ether_dhost[0] = MY_DEST_MAC0;
                	eh->ether_dhost[1] = MY_DEST_MAC1;
                	eh->ether_dhost[2] = MY_DEST_MAC2;
                	eh->ether_dhost[3] = MY_DEST_MAC3; 
                	eh->ether_dhost[4] = MY_DEST_MAC4; 
                	eh->ether_dhost[5] = MY_DEST_MAC5; 
        	} else {
                	eh->ether_dhost[0] = MY_DEST_MAC0;
                	eh->ether_dhost[1] = MY_DEST_MAC1;
                	eh->ether_dhost[2] = MY_DEST_MAC2;
                	eh->ether_dhost[3] = MY_DEST_MAC3;
                	eh->ether_dhost[4] = MY_DEST_MAC5;
                	eh->ether_dhost[5] = MY_DEST_MAC4;
        	}
        	/* Ethertype field */
        	eh->ether_type = htons(ETH_P_IP);
        	tx_len += sizeof(struct ether_header);

        	/* IP Header */
        	iph->ihl = 5;
        	iph->version = 4;
        	iph->tos = 0; // Low delay
        	iph->id = htons(0);
        	iph->ttl = 64; // hops
        	iph->protocol = 17; // UDP
        	iph->saddr = inet_addr("11.135.209.20");
        	iph->daddr = inet_addr("11.135.209.10");
        	tx_len += sizeof(struct iphdr);
		struct udphdr *udph = (struct udphdr *) (sendbuf + sizeof(struct iphdr) + sizeof(struct ether_header));
        	/* UDP Header */
        	udph->source = htons(SRC_PORT);
        	udph->dest = htons(DST_PORT);
        	udph->check = 0; // skip
        	tx_len += sizeof(struct udphdr);

        	iph->check = get_csum((unsigned short *)(sendbuf+sizeof(struct ether_header)), sizeof(struct iphdr)/2);

        	udph = (struct udphdr *) (sendbuf + sizeof(struct iphdr) + sizeof(struct ether_header));
        	iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
        	udph->len = htons(BUF_SIZ - sizeof(struct ether_header) - sizeof(struct iphdr));
        	iph->tot_len = htons(BUF_SIZ - sizeof(struct ether_header));
        	udph->check = get_udp_checksum((const void*)udph,ntohs(udph->len));
        	/* Send packet */
        	memcpy(buf,sendbuf,tx_len);
	}
	

};

struct flow_tag_test : public testing::Test, public ibvt_env {
	struct ibvt_ctx_eth ctx_recv;
	struct ibvt_ctx_eth ctx_send;

	struct ibvt_pd pd_recv;
	struct ibvt_pd pd_send;
	struct ibvt_cq cq_recv;
	struct ibvt_cq cq_send;
	struct ibvt_raw_qp qp_recv;
	struct ibvt_raw_qp qp_send;
	struct ibvt_mr mr_recv;
	struct ibvt_mr mr_send;
	

	uintptr_t start;
	size_t length;

        struct ibv_flow	*flow_create_result ;
        struct ibv_flow_attr	*flow_rules ;

	flow_tag_test() :
		ctx_recv(*this),
		ctx_send(*this),
		pd_recv(*this, ctx_recv),
		pd_send(*this, ctx_send),
		cq_recv(*this, ctx_recv),
		cq_send(*this, ctx_send),
		qp_recv(*this, pd_recv, cq_recv),
		qp_send(*this, pd_send, cq_send),
		mr_recv(*this, pd_recv, SZ),
		mr_send(*this, pd_send, SZ)
	{ }

	virtual void SetUp() {


		INIT(ctx_recv.init());
		INIT(ctx_send.init());
		INIT(qp_send.init());
		INIT(qp_recv.init());
		INIT(mr_recv.init());
		INIT(mr_send.init());

	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

TEST_F(flow_tag_test, t0) {

	int len = BUF_SIZ ;
	CHK_SUT(basic);
	EXEC(qp_recv.set_up_flow_rules(&flow_rules));
	EXEC(qp_recv.recv(mr_recv.sge(0, len)));
	EXEC(qp_recv.connect(NULL));
	EXEC(qp_send.connect(NULL));
	EXEC(qp_recv.flow_tag_create_rules(qp_recv.qp, flow_rules));
        EXEC(qp_send.send_raw_packet(this->mr_send.buff, 1));
	EXEC(qp_send.post_send(this->mr_send.sge(0, len),IBV_WR_SEND));

	EXEC(cq_send.poll());
	EXEC(cq_recv.poll());
	EXEC(qp_recv.vxlan_destroy_rules());

}

TEST_F(flow_tag_test, t1) {

	int len = BUF_SIZ ;
	CHK_SUT(basic);
	EXEC(qp_recv.set_up_flow_rules(&flow_rules));
	EXEC(qp_recv.recv(mr_recv.sge(0, len)));
	EXEC(qp_recv.connect(NULL));
	EXEC(qp_send.connect(NULL));
	EXEC(qp_recv.flow_tag_create_rules(qp_recv.qp, flow_rules));
        EXEC(qp_send.send_raw_packet(this->mr_send.buff, 0));
	EXEC(qp_send.post_send(this->mr_send.sge(0, len),IBV_WR_SEND));

	EXEC(cq_send.poll());
	EXEC(cq_recv.poll_arrive(1));
	EXEC(qp_recv.vxlan_destroy_rules());

}

