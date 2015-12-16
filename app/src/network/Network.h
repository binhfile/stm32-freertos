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
	network_packet_type_beacon_req = 1,
	network_packet_type_beacon_res,

	network_packet_type_join_req,
	network_packet_type_join_res,
};
struct  __attribute__((packed)) network_packet{
	uint8_t hops;	// max hops
	uint8_t type;
	uint8_t args[1];
};
struct network{
	struct mac_mrf24j40 *mac;
};
struct network_beacon_info{
	uint16_t panId;
	uint16_t address;
	uint8_t  lqi;
	uint8_t  rssi;
};
struct network_join_info{
	uint16_t address;
};

struct  __attribute__((packed)) network_args_beacon_req{
};
struct  __attribute__((packed)) network_args_beacon_res{
	uint16_t panId;
	uint16_t address;
};

struct  __attribute__((packed)) network_args_join_req{
};
struct  __attribute__((packed)) network_args_join_res{
	uint16_t address;
};



int Network_scan_channel(struct mac_mrf24j40 *mac, uint32_t channels, uint8_t * noise_level);

int Network_beacon_request(struct network *nwk);
int Network_detect_current_network(struct network *nwk, unsigned int channel, struct network_beacon_info *info, int info_max_count);
int Network_join_request(struct network *nwk, unsigned int channel, uint16_t panId, uint16_t address, struct network_join_info* info);

int Network_loop(struct network *nwk, int timeout);
#endif /* SRC_NETWORK_NETWORK_H_ */
