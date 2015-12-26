/*
 * Network.h
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#ifndef SRC_NETWORK_NETWORK_H_
#define SRC_NETWORK_NETWORK_H_

#include <stdint.h>
#include <semaphore.h>
#include "network/mac/mac_mrf24j40.h"
#include <project_config.h>

#define NWK_LOOK(nwk) (nwk)->lock = 1
#define NWK_UNLOOK(nwk) (nwk)->lock = 0
enum network_packet_type{
	network_packet_type_beacon_req = 1,
	network_packet_type_beacon_res,

	network_packet_type_join_req,
	network_packet_type_join_res,
	network_packet_type_join_done,

	network_packet_type_echo_req,
	network_packet_type_echo_res
};
struct  __attribute__((packed)) network_packet{
	uint8_t hops;	// max hops
	uint8_t type;
	uint8_t args[1];
};
struct nwk_child_info{
	union{
		uint8_t Val;
		struct{
			unsigned char active: 1;
			unsigned char reserver: 7;
		}bits;
	}flags;
	uint8_t long_address[8];
	uint16_t id;
};
struct network{
	struct mac_mrf24j40 *mac;
	int lock;

	uint16_t parent_id;
	struct nwk_child_info child[NWK_CHILD_CNT];
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
struct network_echo_info{
	int total;
	int passed;
	int timeout;
	int failed;
	unsigned int time_diff;
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
struct  __attribute__((packed)) network_args_join_done{
	uint8_t long_address[8];
};
#define NWK_ECHO_LENGTH_MAX		(100)
struct  __attribute__((packed)) network_args_echo_req{
	uint8_t length;
	uint8_t flags;
	uint8_t data[NWK_ECHO_LENGTH_MAX];
};
struct  __attribute__((packed)) network_args_echo_res{
	uint8_t length;
	uint8_t data[NWK_ECHO_LENGTH_MAX];
};


int Network_init(struct network *nwk);
int Network_scan_channel(struct mac_mrf24j40 *mac, uint32_t channels, uint8_t * noise_level);

int Network_beacon_request(struct network *nwk);
int Network_detect_current_network(struct network *nwk, unsigned int channel, struct network_beacon_info *info, int info_max_count);
int Network_join_request(struct network *nwk, unsigned int channel, uint16_t panId, uint16_t address, struct network_join_info* info);
int Network_join_send_done(struct network *nwk);
int Network_echo_request(struct network *nwk, uint16_t address, int count, int datalen, struct network_echo_info* info, int not_wait_res);

int Network_loop(struct network *nwk, int timeout);
#endif /* SRC_NETWORK_NETWORK_H_ */
