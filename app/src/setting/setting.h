/*
 * Setup.h
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#ifndef SRC_SETUP_SETUP_H_
#define SRC_SETUP_SETUP_H_
#include <stdint.h>
#include <at93c.h>
struct setting_device{
	struct at93c_device dev;
};
#define setting_value_magic_id	0x16
enum setting_network_type{
	setting_network_type_end	     = 0,
	setting_network_type_pan_coordinator = 0x01,
	setting_network_type_router 	     = 0x02, 
};
struct __attribute__((packed)) setting_value{
	uint8_t magic_id;				// magic id
	uint8_t network_type;			// network type
	uint8_t mac_long_address[8];	// long address
};
void setting_menu();
int setting_read(struct setting_device* dev, struct setting_value *val);
int setting_write(struct setting_device* dev, struct setting_value* val);

void setting_dump_to_stdio(struct setting_value *val);
#endif /* SRC_SETUP_SETUP_H_ */
