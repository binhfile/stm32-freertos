/*
 * Network.c
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#include "Network.h"
#include <debug.h>
#include <unistd.h>
#include <stdlib.h>
#include <mac_mrf24j40.h>
#include <setting.h>

struct setting_value		g_setting;

#define NWK_LREP(x, args...) LREP("nwk: " x, ##args)
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
	int ret = 0, i;
	struct mac_mrf24j40_write_param write_param;
	struct network_packet *res_pkt;
	struct network_args_echo_res *res;
	struct network_args_echo_req *req;
	uint8_t buff[32];

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 0;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 0;
	write_param.destPANId 				= read_param->srcAddr;
	write_param.destAddress 			= (((uint16_t)nwk->mac->phy.pan_id[0]) & 0x00FF) | ((((uint16_t)nwk->mac->phy.pan_id[1]) << 8) & 0xFF00);
	write_param.srcAddressMode			= mac_iee802154_addrmode_16bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_16bit;

	res_pkt = (struct network_packet*)buff;
	res = (struct network_args_echo_res*)res_pkt->args;
	res_pkt->hops = 1;
	res_pkt->type = network_packet_type_echo_res;

	req = (struct network_args_echo_req*)pkt->args;
	for(i = 0; i < NWK_ECHO_LENGTH; i++){
		res->data[i] = req->data[i];
	}
	len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_echo_res);

	MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
	return ret;
}
int Network_process_packet(struct network *nwk, struct network_packet* pkt, int len, struct mac_mrf24j40_read_param *read_param){
	int ret = 0;

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
				break;
			}
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
				break;
			}
			case network_packet_type_echo_req:{
				uint16_t address = ((uint16_t)(read_param->srcAddr & 0x00FFFF));
				NWK_LREP("recv[rssi:%02X, lqi:%02X] ping request from %u\r\n",
						read_param->rssi, read_param->lqi, address);
				Network_echo_respond(nwk, pkt, len, read_param);
				break;
			}
		}
		if(g_setting.network_type == setting_network_type_pan_coordinator){
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
					break;
				}
			}
		}
	}
	return ret;
}
int Network_loop(struct network *nwk, int timeout){
	int ret = 0, ival;
    struct mac_mrf24j40_read_param read_param;
    char buff[144];

    ival = MAC_mrf24j40_read(nwk->mac, &read_param, buff, 144, timeout);
	ret = Network_process_packet(nwk, (struct network_packet*)buff, ival, &read_param);
	return ret;
}
int Network_echo_request(struct network *nwk, uint16_t address, int count, int datalen, struct network_echo_info* info){
	int ret = -1, i;
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet *pkt;
	struct network_args_echo_req* req;
	struct network_args_echo_res* res;
	struct mac_mrf24j40_read_param read_param;
	struct network_packet* nwk_packet;
	uint8_t tx[32], rx[144];
	uint8_t cnt = 0;
	int timeout = 200;

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

	info->total = count;
	info->passed = 0;
	while(count -- ){
		for(i = 0; i < NWK_ECHO_LENGTH; i++)
			req->data[i] = cnt++;
		len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_echo_req);

		MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
		len = MAC_mrf24j40_read(nwk->mac, &read_param, rx, 144, timeout);
		if(len >= sizeof(struct network_packet)-1){
			if(nwk_packet->type == network_packet_type_echo_res){
				for(i = 0; i < NWK_ECHO_LENGTH; i++){
					if(res->data[i] != req->data[i]) break;
				}
				if(i == NWK_ECHO_LENGTH) {
					info->passed ++;
				}
			}else{
				Network_process_packet(nwk, (struct network_packet*)rx, len, &read_param);
			}
		}
	}
	ret = 0;
	return ret;
}
// end of file
