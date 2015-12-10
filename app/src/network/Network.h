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

int Network_scan_channel(struct mac_mrf24j40 *mac, uint32_t channels, uint8_t * noise_level);

#endif /* SRC_NETWORK_NETWORK_H_ */
