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
	network_packet_type_beacon_res
};
struct  __attribute__((packed)) network_packet{
	uint8_t hops;	// max hops
	uint8_t type;
	uint8_t args[1];
};
struct network{
	struct mac_mrf24j40 *mac;
};
struct network_info{
	uint16_t panId;
	uint8_t  strong;
};
struct  __attribute__((packed)) network_args_beacon_res{
	uint16_t panId;
};



int Network_scan_channel(struct mac_mrf24j40 *mac, uint32_t channels, uint8_t * noise_level);

int Network_send_beacon_req(struct network *nwk);
int Network_detect_current_network(struct network *nwk, unsigned int channel, struct network_info *info, int info_max_count);
int Network_loop(struct network *nwk, int timeout);
#endif /* SRC_NETWORK_NETWORK_H_ */
