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
#include <setting.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <poll.h>

#define time_diff(now, ref)		((now.tv_sec * 1000 + now.tv_nsec / 1000000) - (ref.tv_sec * 1000 + ref.tv_nsec / 1000000))

struct setting_value		g_setting;

#define NWK_LREP(x, args...) LREP("nwk: " x, ##args)
#define NWK_LREP_WARN(x, args...) LREP("nwk %d@%s: " x, __LINE__, __FILE__,  ##args)
#define NETWORK_H_BAR_LEN	80
int Network_scan_channel(struct network *nwk, uint32_t channels, uint8_t * noise_level){
	int timeout = 1000;
	int ret = 0;
	unsigned int i, j;
	struct rf_mac_channel_assessment ch_assessment;
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
			ioctl(nwk->mac_fd, RF_MAC_IOC_WR_CHANNEL, &i);
			clock_gettime(CLOCK_REALTIME, &t_ref);
			while(1){
			    ioctl(nwk->mac_fd, RF_MAC_IOC_RD_CHANNEL_ASSESSMENT, &ch_assessment);
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
int Network_init(struct network *nwk, int mac_fd){
	int i = 0;
	nwk->lock = 0;
	nwk->parent_id = 0;
	nwk->mac_fd = mac_fd;
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
	struct rf_mac_write_packet *write_param;
	int len;
	struct network_packet *pkt;
	char buff[32];

	write_param = (struct rf_mac_write_packet *)buff;
    pkt = (struct network_packet *)write_param->data;

	write_param->header.flags.bits.ack_req          = 0;
	write_param->header.flags.bits.broadcast        = 1;
	write_param->header.flags.bits.intra_pan        = 0;
	write_param->header.flags.bits.dest_addr_64bit  = 0;
	write_param->header.flags.bits.src_addr_64bit   = 1;
	SET_RF_MAC_WRITE_DEST_PAN(&write_param->header, 0xFFFF);
	SET_RF_MAC_WRITE_DEST_ADDR_SHORT(&write_param->header, 0xFFFF);

	pkt->hops = 1;
	pkt->type = network_packet_type_beacon_req;
	write_param->data_len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_beacon_req);

	ioctl(nwk->mac_fd, RF_MAC_IOC_WR_PACKET, write_param);

	return 0;
}
int Network_detect_current_network(struct network *nwk, unsigned int channel, struct network_beacon_info *info, int info_max_count){
	int ret = 0, i;
	int ival;
    struct network_packet* nwk_packet;
    struct network_args_beacon_res *res;
    char buff[32];
    struct timespec t_ref, t_now;
    struct pollfd poll_fd[1];
    struct rf_mac_read_packet* read_mac;

    ioctl(nwk->mac_fd, RF_MAC_IOC_WR_CHANNEL, &channel);
    ioctl(nwk->mac_fd, RF_MAC_IOC_WR_RESET, 0);

    read_mac = (struct rf_mac_read_packet*)buff;

	clock_gettime(CLOCK_REALTIME, &t_ref);
	Network_beacon_request(nwk);
	do{
	    poll_fd[0].fd = nwk->mac_fd;
	    poll_fd[0].events = POLLIN;
	    poll_fd[0].revents = 0;
	    ival = poll(poll_fd, 1, 1000);
	    read_mac->data_len = 0;
	    if(ival > 0 && (poll_fd[0].revents & POLLIN)){
	        ival = ioctl(nwk->mac_fd, RF_MAC_IOC_RD_PACKET, read_mac);
	    }
		if(read_mac->data_len >= sizeof(struct network_packet)-1 + sizeof(struct network_args_beacon_res)){
			nwk_packet = (struct network_packet*)read_mac->data;
			res = (struct network_args_beacon_res *)nwk_packet->args;
			if(nwk_packet->type == network_packet_type_beacon_res){
				for(i = 0; i < ret; i++){
					if(		info[i].panId == res->panId &&
							info[i].address == res->address &&
							info[i].rssi  == read_mac->header.rssi &&
							info[i].lqi   == read_mac->header.lqi)
						break;
				}
				if(i == ret){
					info[ret].panId = res->panId;
					info[ret].address = res->address;
					info[ret].rssi  = read_mac->header.rssi;
					info[ret].lqi   = read_mac->header.lqi;
					ret++;
				}
				//break;
			}else NWK_LREP("recv type %02X not beacon respond type\r\n", nwk_packet->type);
		}else if(ival > 0){
			NWK_LREP("recv %d bytes\r\n", read_mac->data_len);
		}else break;
		clock_gettime(CLOCK_REALTIME, &t_now);
	}
	while((time_diff(t_now, t_ref) < 1000) && (ret < info_max_count));
	return ret;
}
int Network_beacon_respond(struct network *nwk, uint8_t *destLongAddress){
	int ret = 0;
	int len;
	struct network_packet *pkt;
	struct network_args_beacon_res *res;
	uint8_t buff[32];
	struct rf_mac_write_packet *write_mac;
	uint8_t io_buf[8];
	int i;

	write_mac = (struct rf_mac_write_packet *)buff;
    pkt = (struct network_packet *)write_mac->data;
    res = (struct network_args_beacon_res*)pkt->args;

    write_mac->header.flags.bits.ack_req          = 0;
    write_mac->header.flags.bits.broadcast        = 0;
    write_mac->header.flags.bits.intra_pan        = 0;
    write_mac->header.flags.bits.dest_addr_64bit  = 1;
    write_mac->header.flags.bits.src_addr_64bit   = 0;
    SET_RF_MAC_WRITE_DEST_PAN(&write_mac->header, 0xFFFF);
    for(i = 0; i < 8; i++)
        write_mac->header.dest_addr[i] = destLongAddress[i];

	pkt->hops = 1;
	pkt->type = network_packet_type_beacon_res;
	ioctl(nwk->mac_fd, RF_MAC_IOC_RD_PAN_ID, io_buf);
	res->panId = (((uint16_t)io_buf[0]) & 0x00FF) | ((((uint16_t)io_buf[1]) << 8) & 0xFF00);
	ioctl(nwk->mac_fd, RF_MAC_IOC_RD_SHORT_ADDRESS, io_buf);
	res->address = (((uint16_t)io_buf[0]) & 0x00FF) | ((((uint16_t)io_buf[1]) & 0x00FF) << 8);
	write_mac->data_len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_beacon_res);

	ioctl(nwk->mac_fd, RF_MAC_IOC_WR_PACKET, write_mac);
	return ret;
}
int Network_join_send_done(struct network *nwk){
	int ret = 0, i;
	int len;
	struct network_packet *pkt;
	struct network_args_join_done *done;
	uint8_t buff[32];
	uint8_t io_buf[8];
	struct rf_mac_write_packet *write_mac;

	write_mac = (struct rf_mac_write_packet *)buff;
	pkt = (struct network_packet *)write_mac->data;
    done = (struct network_args_join_done*)pkt->args;

    write_mac->header.flags.bits.ack_req          = 0;
    write_mac->header.flags.bits.broadcast        = 0;
    write_mac->header.flags.bits.intra_pan        = 0;
    write_mac->header.flags.bits.dest_addr_64bit  = 0;
    write_mac->header.flags.bits.src_addr_64bit   = 0;
    ioctl(nwk->mac_fd, RF_MAC_IOC_RD_PAN_ID, io_buf);
    write_mac->header.dest_pan_id[0] = io_buf[0];
    write_mac->header.dest_pan_id[1] = io_buf[1];
    SET_RF_MAC_WRITE_DEST_ADDR_SHORT(&write_mac->header, nwk->parent_id);

	pkt->hops = 1;
	pkt->type = network_packet_type_join_done;
	ioctl(nwk->mac_fd, RF_MAC_IOC_RD_LONG_ADDRESS, io_buf);
	for(i = 0; i < 8; i++) done->long_address[i] = io_buf[i];

	write_mac->data_len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_join_done);

	ioctl(nwk->mac_fd, RF_MAC_IOC_WR_PACKET, write_mac);
	return ret;
}
int Network_join_request(struct network *nwk, unsigned int channel, uint16_t panId, uint16_t address, struct network_join_info* info){
	int ret = -1;
	int len;
	struct network_packet *pkt;
	struct network_args_join_res *res;
	uint8_t buff[32];
	struct timespec t_ref, t_now;
	struct rf_mac_write_packet *write_mac;
	struct rf_mac_read_packet  *read_mac;
	struct pollfd poll_fd[1];

	ioctl(nwk->mac_fd, RF_MAC_IOC_WR_CHANNEL, &channel);

	write_mac = (struct rf_mac_write_packet *)buff;
    pkt = (struct network_packet*)write_mac->data;

    write_mac->header.flags.bits.ack_req          = 1;
    write_mac->header.flags.bits.broadcast        = 0;
    write_mac->header.flags.bits.intra_pan        = 0;
    write_mac->header.flags.bits.dest_addr_64bit  = 0;
    write_mac->header.flags.bits.src_addr_64bit   = 0;
    SET_RF_MAC_WRITE_DEST_PAN(&write_mac->header, panId);
    SET_RF_MAC_WRITE_DEST_ADDR_SHORT(&write_mac->header, address);

	pkt->hops = 1;
	pkt->type = network_packet_type_join_req;
	write_mac->data_len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_join_req);

	ioctl(nwk->mac_fd, RF_MAC_IOC_WR_RESET, 0);

	clock_gettime(CLOCK_REALTIME, &t_ref);
	ioctl(nwk->mac_fd, RF_MAC_IOC_WR_PACKET, write_mac);
	read_mac = (struct rf_mac_read_packet *)buff;
	do{
	    poll_fd[0].fd = nwk->mac_fd;
        poll_fd[0].events = POLLIN;
        poll_fd[0].revents = 0;
        len = poll(poll_fd, 1, 1000);
        read_mac->data_len = 0;
        if(len > 0 && (poll_fd[0].revents & POLLIN)){
            len = ioctl(nwk->mac_fd, RF_MAC_IOC_RD_PACKET, read_mac);
        }
		if(read_mac->data_len >= sizeof(struct network_packet)-1 + sizeof(struct network_args_join_res)){
			pkt = (struct network_packet*)read_mac->data;
			res = (struct network_args_join_res *)pkt->args;
			if(pkt->type == network_packet_type_join_res){
				info->address = res->address;
				ret = 0;
				break;
			}else NWK_LREP("recv type %2X not join respond type\r\n", pkt->type);
		}else if(len > 0){
			NWK_LREP("recv %d bytes\r\n", read_mac->data_len);
		}
		clock_gettime(CLOCK_REALTIME, &t_now);
	}while(time_diff(t_now, t_ref) < 1000);
	return ret;
}
int Network_join_respond(struct network *nwk, uint8_t *destAddr){
	int ret = 0;
	int len, i;
	struct network_packet *pkt;
	struct network_args_join_res *res;
	uint8_t buff[32];
	uint32_t u32Val;
	struct rf_mac_write_packet *write_mac;

	write_mac = (struct rf_mac_write_packet *)buff;
	pkt = (struct network_packet*)write_mac->data;
    res = (struct network_args_join_res*)pkt->args;

    write_mac->header.flags.bits.ack_req          = 0;
    write_mac->header.flags.bits.broadcast        = 0;
    write_mac->header.flags.bits.intra_pan        = 0;
    write_mac->header.flags.bits.dest_addr_64bit  = 0;
    write_mac->header.flags.bits.src_addr_64bit   = 1;
    SET_RF_MAC_WRITE_DEST_PAN(&write_mac->header, 0xFFFF);
    for(i = 0; i < 8; i++)
        write_mac->header.dest_addr[i] = destAddr[i];

	pkt->hops = 1;
	pkt->type = network_packet_type_join_res;
	u32Val = rand();
	res->address = u32Val & 0xFFFF;
	write_mac->data_len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_join_res);

	NWK_LREP("assign node: %04X\r\n", res->address);

	ioctl(nwk->mac_fd, RF_MAC_IOC_WR_PACKET, write_mac);
	return ret;
}
int Network_echo_respond(struct network *nwk, struct network_packet* pkt, int len, struct rf_mac_read_packet *read_param){
	int ret = 0;
	struct network_packet *res_pkt;
	struct network_args_echo_res *res;
	struct network_args_echo_req *req;
	uint8_t buff[144], io_buf[8];
	struct rf_mac_write_packet *write_mac;

    write_mac = (struct rf_mac_write_packet *)buff;
    res_pkt = (struct network_packet*)write_mac->data;
    res = (struct network_args_echo_res*)res_pkt->args;

    write_mac->header.flags.bits.ack_req          = 0;
    write_mac->header.flags.bits.broadcast        = 0;
    write_mac->header.flags.bits.intra_pan        = 1;
    write_mac->header.flags.bits.dest_addr_64bit  = 0;
    write_mac->header.flags.bits.src_addr_64bit   = 0;
    ioctl(nwk->mac_fd, RF_MAC_IOC_RD_PAN_ID, io_buf);
    write_mac->header.dest_pan_id[0] = io_buf[0];
    write_mac->header.dest_pan_id[1] = io_buf[1];
    write_mac->header.dest_addr[0] = read_param->header.dest_addr[0];
    write_mac->header.dest_addr[1] = read_param->header.dest_addr[1];

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
		write_mac->data_len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_echo_res);
		res->length = req->length;

		ioctl(nwk->mac_fd, RF_MAC_IOC_WR_PACKET, write_mac);
	}
	return ret;
}
int Network_process_packet(struct network *nwk, struct network_packet* pkt, int len, struct rf_mac_read_packet *read_param){
	int ret = 0;
	int processed = 0;

	if(len >= sizeof(struct network_packet)-1){
		switch(pkt->type){
			case network_packet_type_beacon_req:{
				// request a beacon
				NWK_LREP("recv[rssi:%02X, lqi:%02X] a beacon request from %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X, send back a respond\r\n",
						read_param->header.rssi, read_param->header.lqi,
						(uint8_t)(read_param->header.src_addr[0]),
						(uint8_t)(read_param->header.src_addr[1]),
						(uint8_t)(read_param->header.src_addr[2]),
						(uint8_t)(read_param->header.src_addr[3]),
						(uint8_t)(read_param->header.src_addr[4]),
						(uint8_t)(read_param->header.src_addr[5]),
						(uint8_t)(read_param->header.src_addr[6]),
						(uint8_t)(read_param->header.src_addr[7]));
				Network_beacon_respond(nwk, read_param->header.src_addr);
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
				NWK_LREP("recv join done from %04X, add to list child ...", GET_RF_MAC_READ_SRC_ADDR_SHORT(&read_param->header));
				if(Network_add_child(nwk, done->long_address, GET_RF_MAC_READ_SRC_ADDR_SHORT(&read_param->header))) LREP("FAIL\r\n");
				else LREP("done\r\n");
				processed = 1;
				break;
			}
		}
		if(processed == 0 && (g_setting.network_type == setting_network_type_pan_coordinator ||
				g_setting.network_type == setting_network_type_router)){
			switch(pkt->type){
				case network_packet_type_join_req:{
					// request to join
					NWK_LREP("recv[rssi:%02X, lqi:%02X] a join request from %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n",
							read_param->header.rssi, read_param->header.lqi,
							(uint8_t)(read_param->header.src_addr[0]),
                            (uint8_t)(read_param->header.src_addr[1]),
                            (uint8_t)(read_param->header.src_addr[2]),
                            (uint8_t)(read_param->header.src_addr[3]),
                            (uint8_t)(read_param->header.src_addr[4]),
                            (uint8_t)(read_param->header.src_addr[5]),
                            (uint8_t)(read_param->header.src_addr[6]),
                            (uint8_t)(read_param->header.src_addr[7]));
					Network_join_respond(nwk, read_param->header.src_addr);
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
    char buff[144];
    struct rf_mac_read_packet  *read_mac;
    struct pollfd poll_fd[1];

    read_mac = (struct rf_mac_read_packet *)buff;
    if(nwk->lock == 0){
        poll_fd[0].fd = nwk->mac_fd;
        poll_fd[0].events = POLLIN;
        poll_fd[0].revents = 0;
        ival = poll(poll_fd, 1, timeout);
        read_mac->data_len = 0;
        if(ival > 0 && (poll_fd[0].revents & POLLIN)){
            ival = ioctl(nwk->mac_fd, RF_MAC_IOC_RD_PACKET, read_mac);
        }
    	ret = Network_process_packet(nwk, (struct network_packet*)buff, ival, read_mac);
    }else usleep(1000*100);
	return ret;
}
int Network_echo_request(struct network *nwk, uint16_t address, int count, int datalen, struct network_echo_info* info, int not_wait_res){
	int ret = -1, i;
	int len;
	struct network_packet *pkt;
	struct network_args_echo_req* req;
	struct network_args_echo_res* res;
	struct network_packet* nwk_packet;
	uint8_t tx[144], rx[144], io_buf[8];
	int timeout = 200;
	struct timespec t_now, t_ref;
	int failed_cnt = 0;
    int timeout_cnt;
    struct rf_mac_read_packet   *read_mac;
    struct rf_mac_write_packet  *write_mac;
    struct pollfd poll_fd[1];

    read_mac = (struct rf_mac_read_packet*)rx;
    write_mac = (struct rf_mac_write_packet*)tx;

    write_mac->header.flags.bits.ack_req          = 0;
    write_mac->header.flags.bits.broadcast        = 0;
    write_mac->header.flags.bits.intra_pan        = 0;
    write_mac->header.flags.bits.dest_addr_64bit  = 0;
    write_mac->header.flags.bits.src_addr_64bit   = 0;
    ioctl(nwk->mac_fd, RF_MAC_IOC_RD_PAN_ID, io_buf);
    write_mac->header.dest_pan_id[0] = io_buf[0];
    write_mac->header.dest_pan_id[1] = io_buf[1];
    SET_RF_MAC_WRITE_DEST_ADDR_SHORT(&write_mac->header, address);


	pkt = (struct network_packet*)write_mac->data;
	req = (struct network_args_echo_req*)pkt->args;
	nwk_packet = (struct network_packet*)read_mac->data;
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
	ioctl(nwk->mac_fd, RF_MAC_IOC_WR_RESET, 0);
	clock_gettime(CLOCK_REALTIME, &t_ref);
	while(count -- && kb_value() != 0x03){
		failed_cnt++;
		for(i = 0; i < datalen; i++)
			req->data[i] = rand();
		write_mac->data_len = sizeof(struct network_packet) - 1 + sizeof(struct network_args_echo_req);

        poll_fd[0].fd = nwk->mac_fd;
        poll_fd[0].events = POLLOUT;
        poll_fd[0].revents = 0;
        ret = poll(poll_fd, 1, timeout);
        if(ret > 0 && (poll_fd[0].revents & POLLOUT)){
            ioctl(nwk->mac_fd, RF_MAC_IOC_WR_PACKET, write_mac);
			if(not_wait_res == 0){
				len = 0;
				timeout_cnt = 2;
				do{
				    poll_fd[0].fd = nwk->mac_fd;
                    poll_fd[0].events = POLLIN;
                    poll_fd[0].revents = 0;
                    ret = poll(poll_fd, 1, timeout);
                    read_mac->data_len = 0;
                    if(ret > 0 && (poll_fd[0].revents & POLLIN)){
                        ret = ioctl(nwk->mac_fd, RF_MAC_IOC_RD_PACKET, read_mac);
                    }
				}while(read_mac->data_len == 0 && timeout_cnt --);
				if(read_mac->data_len >= sizeof(struct network_packet)-1){
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
						Network_process_packet(nwk, nwk_packet, len, read_mac);
					}
				}else{
					//NWK_LREP_WARN("[%d]packet invalid length %u\r\n", count, len);
					info->timeout++;
				}
			}else{
				info->passed ++;
				failed_cnt = 0;
			}
		}else{
		    info->timeout++;
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
