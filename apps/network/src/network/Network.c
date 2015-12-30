/*
 * Network.c
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */
#include "project_config.h"
#include "Network.h"
#include <debug.h>
#include <unistd.h>
#include <stdlib.h>
#include <mac_mrf24j40.h>
#include <setting.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

struct setting_value		g_setting;

#define NWK_LREP(x, args...) LREP("nwk: " x, ##args)
#define NWK_LREP_WARN(x, args...) LREP("nwk %d@%s: " x, __LINE__, __FILE__,  ##args)
#define NWK_PANID(nwk)	(((uint16_t)nwk->mac->phy.pan_id[0]) & 0x00FF) | ((((uint16_t)nwk->mac->phy.pan_id[1]) << 8) & 0xFF00)
#define NETWORK_H_BAR_LEN	80
int Network_scan_channel(struct mac_mrf24j40 *mac, uint32_t channels, uint8_t * noise_level){
	int timeout = 1000;
	int ret = 0;
	unsigned int i, j;
	struct mac_channel_assessment ch_assessment;
	struct timespec t_now, t_ref;
	uint8_t u8val;

	i = 0;
	j = NETWORK_H_BAR_LEN;
	LREP("    MIN");
	while(j > 0){
		LREP("-");
		j--;
	}
	LREP("MAX\r\n");
	while(i < 32){
		if( (((uint32_t)1) << i) & channels ){
			LREP("ch: %02d ", i);
			u8val = 0;
			MAC_mrf24j40_ioctl(mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&i);
			clock_gettime(CLOCK_REALTIME, &t_ref);
			while(1){
				MAC_mrf24j40_ioctl(mac, mac_mrf24j40_ioc_channel_assessment, (unsigned int)&ch_assessment);
				if(ch_assessment.noise_level > u8val) u8val = ch_assessment.noise_level;
				clock_gettime(CLOCK_REALTIME, &t_now);
				if(t_now.tv_sec < t_ref.tv_sec) break;
				if((t_now.tv_sec*1000 + t_now.tv_nsec/1000000) - (t_ref.tv_sec*1000 + t_ref.tv_nsec/1000000) >= timeout){
					break;
				}else{
					usleep(1000* 10);
				}
			}
			*noise_level = u8val;
			noise_level++;
			j = u8val * NETWORK_H_BAR_LEN / 255;
			while(j > 0){
				LREP("-");
				j--;
			}
			LREP(" %02X\r\n", u8val);
		}
		i++;
	}

	return ret;
}
int Network_init(struct network *nwk){
	int i = 0;
	nwk->lock = 0;
	nwk->parent_id = 0;
	for(i = 0; i < NWK_CHILD_CNT; i++){
		nwk->child[i].flags.bits.active = 0;
	}
	return 0;
}
int Network_add_child(struct network *nwk, uint8_t *long_address, uint16_t id){
	int ret = -1;
	int i, j;
	for(i = 0;i < NWK_CHILD_CNT; i++){
		if(nwk->child[i].flags.bits.active && nwk->child[i].id == id){
			break;
		}
	}
	if(i == NWK_CHILD_CNT){
		// not found
		for(i = 0;i < NWK_CHILD_CNT; i++){
			if(!nwk->child[i].flags.bits.active){
				nwk->child[i].flags.bits.active = 1;
				nwk->child[i].id = id;
				for(j = 0; j < 8; j++) nwk->child[i].long_address[j] = long_address[j];
				ret = 0;
				break;
			}
		}
	}
	return ret;
}
int Network_remove_child(struct network *nwk, uint16_t id){
	int i;
	for(i = 0;i < NWK_CHILD_CNT; i++){
		if(nwk->child[i].flags.bits.active && nwk->child[i].id == id){
			nwk->child[i].flags.bits.active = 0;
		}
	}
	return 0;
}
int Network_beacon_request(struct network *nwk){
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet *pkt;
//	struct network_args_beacon_req *req;
	char buff[32];

	pkt = (struct network_packet *)buff;
//	req = (struct network_args_beacon_req *)pkt->args;

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 1;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 0;
	write_param.destPANId 				= 0xffff;
	write_param.destAddress 			= 0xffff;
	write_param.srcAddressMode			= mac_iee802154_addrmode_64bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_16bit;

	pkt->hops = 1;
	pkt->type = network_packet_type_beacon_req;
	len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_beacon_req);

	MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
	return 0;
}
int Network_detect_current_network(struct network *nwk, unsigned int channel, struct network_beacon_info *info, int info_max_count){
	int ret = 0;
	int ival;
    struct mac_mrf24j40_read_param read_param;
    struct network_packet* nwk_packet;
    struct network_args_beacon_res *res;
    char buff[32];

	MAC_mrf24j40_ioctl(nwk->mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&channel);
	Network_beacon_request(nwk);

	ival = MAC_mrf24j40_read(nwk->mac, &read_param, buff, 32, 1000);
	if(ival >= sizeof(struct network_packet)-1 + sizeof(struct network_args_beacon_res)){
		nwk_packet = (struct network_packet*)buff;
		res = (struct network_args_beacon_res *)nwk_packet->args;
		if(nwk_packet->type == network_packet_type_beacon_res){
			info[ret].panId = res->panId;
			info[ret].address = res->address;
			info[ret].rssi  = read_param.rssi;
			info[ret].lqi   = read_param.lqi;
			ret++;
		}
	}else if(ival > 0){
		NWK_LREP("recv %d bytes\r\n", ival);
	}
	return ret;
}
int Network_beacon_respond(struct network *nwk, uint64_t destLongAddress){
	int ret = 0;
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet *pkt;
	struct network_args_beacon_res *res;
	uint8_t buff[32];

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 0;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 0;
	write_param.destPANId 				= 0xFFFF;
	write_param.destAddress 			= destLongAddress;
	write_param.srcAddressMode			= mac_iee802154_addrmode_16bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_64bit;

	pkt = (struct network_packet*)buff;
	res = (struct network_args_beacon_res*)pkt->args;
	pkt->hops = 1;
	pkt->type = network_packet_type_beacon_res;
	res->panId = (((uint16_t)nwk->mac->phy.pan_id[0]) & 0x00FF) | ((((uint16_t)nwk->mac->phy.pan_id[1]) << 8) & 0xFF00);
	res->address = (((uint16_t)nwk->mac->phy.s_address[0]) & 0x00FF) | ((((uint16_t)nwk->mac->phy.s_address[1]) & 0x00FF) << 8);
	len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_beacon_res);

	MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
	return ret;
}
int Network_join_send_done(struct network *nwk){
	int ret = 0, i;
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet *pkt;
	struct network_args_join_done *done;
	uint8_t buff[32];

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 0;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 0;
	write_param.destPANId 				= NWK_PANID(nwk);
	write_param.destAddress 			= nwk->parent_id;
	write_param.srcAddressMode			= mac_iee802154_addrmode_16bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_16bit;

	pkt = (struct network_packet*)buff;
	done = (struct network_args_join_done*)pkt->args;
	pkt->hops = 1;
	pkt->type = network_packet_type_join_done;
	for(i = 0; i < 8; i++) done->long_address[i] = nwk->mac->phy.l_address[i];

	len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_join_done);

	MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
	return ret;
}
int Network_join_request(struct network *nwk, unsigned int channel, uint16_t panId, uint16_t address, struct network_join_info* info){
	int ret = -1;
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet *pkt;
	struct network_args_join_res *res;
	struct mac_mrf24j40_read_param read_param;
	uint8_t buff[32];

	MAC_mrf24j40_ioctl(nwk->mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&channel);

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 0;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 0;
	write_param.destPANId 				= panId;
	write_param.destAddress 			= address;
	write_param.srcAddressMode			= mac_iee802154_addrmode_64bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_16bit;

	pkt = (struct network_packet*)buff;
	pkt->hops = 1;
	pkt->type = network_packet_type_join_req;
	len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_join_req);

	MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
	// get request
	len = MAC_mrf24j40_read(nwk->mac, &read_param, buff, 32, 1000);
	if(len >= sizeof(struct network_packet)-1 + sizeof(struct network_args_join_res)){
		pkt = (struct network_packet*)buff;
		res = (struct network_args_join_res *)pkt->args;
		if(pkt->type == network_packet_type_join_res){
			info->address = res->address;
			ret = 0;
		}
	}else if(len > 0){
		NWK_LREP("recv %d bytes\r\n", len);
	}
	return ret;
}
int Network_join_respond(struct network *nwk, uint64_t destAddr){
	int ret = 0;
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet *pkt;
	struct network_args_join_res *res;
	uint8_t buff[32];
	uint32_t u32Val;

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 0;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 0;
	write_param.destPANId 				= 0xFFFF;
	write_param.destAddress 			= destAddr;
	write_param.srcAddressMode			= mac_iee802154_addrmode_16bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_64bit;

	pkt = (struct network_packet*)buff;
	res = (struct network_args_join_res*)pkt->args;
	pkt->hops = 1;
	pkt->type = network_packet_type_join_res;
	u32Val = rand();
	res->address = u32Val & 0xFFFF;
	len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_join_res);

	NWK_LREP("assign node: %04X\r\n", res->address);

	MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
	return ret;
}
int Network_echo_respond(struct network *nwk, struct network_packet* pkt, int len, struct mac_mrf24j40_read_param *read_param){
	int ret = 0;
	struct mac_mrf24j40_write_param write_param;
	struct network_packet *res_pkt;
	struct network_args_echo_res *res;
	struct network_args_echo_req *req;
	uint8_t buff[144];

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 0;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 1;
	write_param.destPANId 				= (((uint16_t)nwk->mac->phy.pan_id[0]) & 0x00FF) | ((((uint16_t)nwk->mac->phy.pan_id[1]) << 8) & 0xFF00);
	write_param.destAddress 			= read_param->srcAddr;
	write_param.srcAddressMode			= mac_iee802154_addrmode_16bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_16bit;

	res_pkt = (struct network_packet*)buff;
	res = (struct network_args_echo_res*)res_pkt->args;
	res_pkt->hops = 1;
	res_pkt->type = network_packet_type_echo_res;

	req = (struct network_args_echo_req*)pkt->args;

	if((req->flags & 0x01) == 0){
		if(req->length > NWK_ECHO_LENGTH_MAX) req->length = NWK_ECHO_LENGTH_MAX;
#if 0
		for(i = 0; i < req->length; i++){
			res->data[i] = req->data[i];
		}
#else
		memcpy(res->data, req->data, req->length);
#endif
		len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_echo_res);
		res->length = req->length;

		MAC_mrf24j40_write(nwk->mac, &write_param, res_pkt, len);
	}
	return ret;
}
int Network_process_packet(struct network *nwk, struct network_packet* pkt, int len, struct mac_mrf24j40_read_param *read_param){
	int ret = 0;
	int processed = 0;

	if(len >= sizeof(struct network_packet)-1){
		switch(pkt->type){
			case network_packet_type_beacon_req:{
				// request a beacon
				NWK_LREP("recv[rssi:%02X, lqi:%02X] a beacon request from %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X, send back a respond\r\n",
						read_param->rssi, read_param->lqi,
						(uint8_t)((read_param->srcAddr >> 0) & 0x00FF),
						(uint8_t)((read_param->srcAddr >> 8 * 1) & 0x00FF),
						(uint8_t)((read_param->srcAddr >> 8 * 2) & 0x00FF),
						(uint8_t)((read_param->srcAddr >> 8 * 3) & 0x00FF),
						(uint8_t)((read_param->srcAddr >> 8 * 4) & 0x00FF),
						(uint8_t)((read_param->srcAddr >> 8 * 5) & 0x00FF),
						(uint8_t)((read_param->srcAddr >> 8 * 6) & 0x00FF),
						(uint8_t)((read_param->srcAddr >> 8 * 7) & 0x00FF));
				Network_beacon_respond(nwk, read_param->srcAddr);
				processed = 1;
				break;
			}
			case network_packet_type_echo_req:{
				//uint16_t address = ((uint16_t)(read_param->srcAddr & 0x00FFFF));
				//NWK_LREP("recv[rssi:%02X, lqi:%02X] ping request from %04X\r\n",
				//		read_param->rssi, read_param->lqi, address);
                //DUMP(pkt, len, "recv echo");
				Network_echo_respond(nwk, pkt, len, read_param);
				processed = 1;
				break;
			}
			case network_packet_type_join_done:{
				struct network_args_join_done* done = (struct network_args_join_done*)pkt->args;
				NWK_LREP("recv join done from %04X, add to list child ...", ((uint16_t)read_param->srcAddr));
				if(Network_add_child(nwk, done->long_address, read_param->srcAddr)) LREP("FAIL\r\n");
				else LREP("done\r\n");
				processed = 1;
				break;
			}
		}
		if(processed == 0 && g_setting.network_type == setting_network_type_pan_coordinator){
			switch(pkt->type){
				case network_packet_type_join_req:{
					// request to join
					NWK_LREP("recv[rssi:%02X, lqi:%02X] a join request from %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n",
							read_param->rssi, read_param->lqi,
							(uint8_t)((read_param->srcAddr >> 0) & 0x00FF),
							(uint8_t)((read_param->srcAddr >> 8 * 1) & 0x00FF),
							(uint8_t)((read_param->srcAddr >> 8 * 2) & 0x00FF),
							(uint8_t)((read_param->srcAddr >> 8 * 3) & 0x00FF),
							(uint8_t)((read_param->srcAddr >> 8 * 4) & 0x00FF),
							(uint8_t)((read_param->srcAddr >> 8 * 5) & 0x00FF),
							(uint8_t)((read_param->srcAddr >> 8 * 6) & 0x00FF),
							(uint8_t)((read_param->srcAddr >> 8 * 7) & 0x00FF));
					Network_join_respond(nwk, read_param->srcAddr);
					processed = 1;
					break;
				}
			}
		}
		if(!processed){
			NWK_LREP_WARN("packet type %02X not process\r\n", pkt->type);
		}
	}else if(len > 0){
		NWK_LREP_WARN("packet invalid length %u\r\n", len);
	}
	return ret;
}
int Network_loop(struct network *nwk, int timeout){
	int ret = 0, ival;
    struct mac_mrf24j40_read_param read_param;
    char buff[144];
    if(nwk->lock == 0){
    	ival = MAC_mrf24j40_read(nwk->mac, &read_param, buff, 144, timeout);
    	ret = Network_process_packet(nwk, (struct network_packet*)buff, ival, &read_param);
    }else usleep(1000*100);
	return ret;
}
int Network_echo_request(struct network *nwk, uint16_t address, int count, int datalen, struct network_echo_info* info, int not_wait_res){
	int ret = -1, i;
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet *pkt;
	struct network_args_echo_req* req;
	struct network_args_echo_res* res;
	struct mac_mrf24j40_read_param read_param;
	struct network_packet* nwk_packet;
	uint8_t tx[144], rx[144];
	int timeout = 200;
	struct timespec t_now, t_ref;
	int failed_cnt = 0;
    int timeout_cnt;

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 0;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 0;
	write_param.destPANId 				= (((uint16_t)nwk->mac->phy.pan_id[0]) & 0x00FF) | ((((uint16_t)nwk->mac->phy.pan_id[1]) << 8) & 0xFF00);
	write_param.destAddress 			= address;
	write_param.srcAddressMode			= mac_iee802154_addrmode_16bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_16bit;

	pkt = (struct network_packet*)tx;
	req = (struct network_args_echo_req*)pkt->args;
	nwk_packet = (struct network_packet*)rx;
	res = (struct network_args_echo_res*)nwk_packet->args;

	pkt->hops = 1;
	pkt->type = network_packet_type_echo_req;

	info->total 	= count;
	info->passed 	= 0;
	info->failed 	= 0;
	info->timeout 	= 0;
	info->time_diff = 0;
	if(datalen > NWK_ECHO_LENGTH_MAX) datalen = NWK_ECHO_LENGTH_MAX;
	req->length = datalen;
	req->flags = not_wait_res ? 1 : 0;

	nwk->lock 		= 1;
	usleep(1000* 200);
	clock_gettime(CLOCK_REALTIME, &t_ref);
	while(count -- && kb_value() != 0x03){
		failed_cnt++;
		for(i = 0; i < datalen; i++)
			req->data[i] = rand();
		len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_echo_req);

		if(MAC_wait_ready_to_write(nwk->mac, 100) != 1){
			info->timeout++;
		}
		else{
			MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
			if(not_wait_res == 0){
				len = 0;
				timeout_cnt = 2;
				do{
					len = MAC_mrf24j40_read(nwk->mac, &read_param, rx, 144, timeout);
				}while(len == 0 && timeout_cnt --);
				if(len >= sizeof(struct network_packet)-1){
					if(nwk_packet->type == network_packet_type_echo_res){
						i = 0;
	#if 0
						if(res->length == req->length){
							for(i = 0; i < datalen; i++){
								if(res->data[i] != req->data[i]) break;
							}
						}
						if(i == datalen)
	#else
						if(memcmp(res->data, req->data, datalen) == 0)
	#endif
						{
							info->passed ++;
							failed_cnt = 0;
							//LREP("[%d]rx done\r\n", count);
						}else{
							//LREP("false @ %d\r\n", i);
							info->failed++;
							//NWK_LREP_WARN("[%d]rx test failed @%d len %d\r\n", count, i, datalen);
							//DUMP(req->data, datalen, "tx");
							//DUMP(res->data, datalen, "rx");
						}
					}else{
						NWK_LREP_WARN("[%d]packet type %02X not echo-res\r\n", count, nwk_packet->type);
						info->timeout++;
						Network_process_packet(nwk, nwk_packet, len, &read_param);
					}
				}else{
					//NWK_LREP_WARN("[%d]packet invalid length %u\r\n", count, len);
					info->timeout++;
				}
			}else{
				info->passed ++;
				failed_cnt = 0;
			}
		}
		if(failed_cnt > 10) break;
	}
	nwk->lock = 0;
	clock_gettime(CLOCK_REALTIME, &t_now);
	info->time_diff = t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000 - (t_ref.tv_sec * 1000 + t_ref.tv_nsec / 1000000);
	ret = (failed_cnt > 10) ? -1:0;
	return ret;
}
// end of file
