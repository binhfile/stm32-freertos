/*
 * Network.h
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#ifndef SRC_NETWORK_NETWORK_H_
#define SRC_NETWORK_NETWORK_H_

#include <stdint.h>
#include "network/mac/mac_mrf24j40.h"
enum network_packet_type{
	network_packet_type_req_active = 1,	// request current network replay a respond with info
	network_packet_type_res_active		// respond of req_active
};
struct network_packet{
	uint8_t hops;	// max hops
	uint8_t type;
	uint8_t args[1];
};
struct network_args_res_active{
	uint16_t panId;
};
struct network{
	void *mac;

};
struct network_send_param{
	uint8_t hops;
	uint8_t type;
	uint16_t destAddress;
	uint16_t destPANId;
	union{
		uint8_t Val;
		struct{
			unsigned char broadcast : 1;
		}bits;
	}flags;
};


int Network_scan_channel(struct mac_mrf24j40 *mac, uint32_t channels, uint8_t * noise_level);

int Network_send_packet(struct network* nwk, struct network_send_param * param, void* args, int args_len);

#endif /* SRC_NETWORK_NETWORK_H_ */
