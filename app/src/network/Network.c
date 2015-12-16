/*
 * Network.c
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#include "Network.h"
#include <debug.h>
#include <unistd.h>
#include <mac_mrf24j40.h>

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
int Network_send_beacon_req(struct network *nwk){
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet pkt;

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 1;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 1;
	write_param.destPANId 				= 0xffff;
	write_param.destAddress 			= 0xffff;
	write_param.srcAddressMode			= mac_iee802154_addrmode_64bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_16bit;

	pkt.hops = 1;
	pkt.type = network_packet_type_beacon_req;
	len = sizeof(struct network_packet) - 1;

	MAC_mrf24j40_write(nwk->mac, &write_param, &pkt, len);
	return 0;
}
int Network_detect_current_network(struct network *nwk, unsigned int channel, struct network_info *info, int info_max_count){
	int ret = 0;
	int ival;
    struct mac_mrf24j40_read_param read_param;
    struct network_packet* nwk_packet;
    char buff[32];

	MAC_mrf24j40_ioctl(nwk->mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&channel);
	Network_send_beacon_req(nwk);

	ival = MAC_mrf24j40_read(nwk->mac, &read_param, buff, sizeof(struct network_packet)-1, 1000);
	if(ival >= sizeof(struct network_packet)-1){
		nwk_packet = (struct network_packet*)buff;
		if(nwk_packet->type == network_packet_type_beacon_res){
			info[ret].panId = read_param.srcPANid;
			info[ret].strong= 0;
			ret++;
		}
	}
	return ret;
}
int Network_send_beacon_respond(struct network *nwk, uint64_t destLongAddress){
	int ret = 0;
	struct mac_mrf24j40_write_param write_param;
	int len;
	struct network_packet *pkt;
	struct network_args_beacon_res *res;
	uint8_t buff[32];

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 1;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.flags.bits.intraPAN		= 1;
	write_param.destPANId 				= 0xffff;
	write_param.destAddress 			= destLongAddress;
	write_param.srcAddressMode			= mac_iee802154_addrmode_16bit;
	write_param.destAddressMode			= mac_iee802154_addrmode_64bit;

	pkt = (struct network_packet*)buff;
	res = (struct network_args_beacon_res*)pkt->args;
	pkt->hops = 1;
	pkt->type = network_packet_type_beacon_res;
	res->panId = (((uint16_t)nwk->mac->phy.pan_id[0]) & 0x00FF) | ((((uint16_t)nwk->mac->phy.pan_id[0]) << 8) & 0xFF00));
	len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_beacon_res);

	MAC_mrf24j40_write(nwk->mac, &write_param, pkt, len);
	return ret;
}
int Network_loop(struct network *nwk, int timeout){
	int ret = 0, ival;
    struct mac_mrf24j40_read_param read_param;
    struct network_packet* nwk_packet;
    char buff[128];

    ival = MAC_mrf24j40_read(nwk->mac, &read_param, buff, 128, timeout);
	if(ival >= sizeof(struct network_packet)-1){
		nwk_packet = (struct network_packet*)buff;
		switch(nwk_packet->type){
			case network_packet_type_beacon_req:{
				// request a beacon
				NWK_LREP("received a beacon request from %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X, send back a respond\r\n",
						(uint8_t)((read_param.srcAddr >> 0) & 0x00FF),
						(uint8_t)((read_param.srcAddr >> 8 * 1) & 0x00FF),
						(uint8_t)((read_param.srcAddr >> 8 * 2) & 0x00FF),
						(uint8_t)((read_param.srcAddr >> 8 * 3) & 0x00FF),
						(uint8_t)((read_param.srcAddr >> 8 * 4) & 0x00FF),
						(uint8_t)((read_param.srcAddr >> 8 * 5) & 0x00FF),
						(uint8_t)((read_param.srcAddr >> 8 * 6) & 0x00FF),
						(uint8_t)((read_param.srcAddr >> 8 * 7) & 0x00FF));
				Network_send_beacon_respond(nwk, read_param.srcAddr);
				break;
			}
		}
	}
	return ret;
}
// end of file
