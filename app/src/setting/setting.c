/*
 * Setup.c
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#include "setting.h"
#include <debug.h>
extern struct setting_device		g_setting_dev;

const char* node_type_to_str(uint8_t type){
	switch(type){
		case setting_network_type_pan_coordinator: return "PAN coordinator"; break;
		case setting_network_type_router: return "Router"; break;
		case setting_network_type_end: return "End device"; break;
		default: return "Unknown"; break;
	}
}
void setting_dump_to_stdio(struct setting_device *setting){
	struct setting_value val;
	int i;
	memset(&val,0, sizeof(struct setting_value));
	if(setting_read(setting, &val) < 0){
		LREP("Read failed\r\n");
	}else{
		LREP("Setting:\r\n");
		LREP("Magic: %02X (%s)\r\n", val.magic_id, (val.magic_id == setting_value_magic_id) ? "correct" : "INCorrect");
		LREP("Note type: %d (%s)\r\n", val.network_type, node_type_to_str(val.network_type));
		LREP("Long address: ");
		for(i = 0; i < 7 ; i ++) LREP("%02X:", val.mac_long_address[i]);
		LREP("%02X", val.mac_long_address[i]);
		LREP("\r\n");
	}
}
uint8_t hex_to_dec(uint8_t hex){
	if(hex >= '0' && hex <= '9') return hex - '0';
	else if(hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
	else if(hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
	else return 0;
}
void setting_write_from_stdio(struct setting_device *setting){
	struct setting_value val;
	int i;
	uint8_t input;
	if(setting_read(setting, &val) < 0){
		LREP("Read failed\r\n");
	}else{
		LREP("Setting:\r\n");
		// node type
		LREP("Note type: %d (%s), ", val.network_type, node_type_to_str(val.network_type));
		do{
			input = kb_cmd("new");
			if(input >= '0' && input <= '9') break;
		}while(1);
		val.network_type = input - '0';
		// Long address
		LREP("Long address: ");
		for(i = 0; i < 7 ; i ++) LREP("%02X:", val.mac_long_address[i]);
		LREP("%02X", val.mac_long_address[i]);
		LREP(", new?\r\n");
		for(i = 0; i < 8 ; i++){
			LREP("addr[%d] ", i);
			input = kbhit(1000*30); LREP("%c", input); val.mac_long_address[i] = (hex_to_dec(input) << 4) & 0xF0;
			input = kbhit(1000*30); LREP("%c", input); val.mac_long_address[i] |= (hex_to_dec(input)) & 0x0F;
			kbhit(1000*30);// ENTER
			LREP("\r\n");
		}
		// write back
		val.magic_id = setting_value_magic_id;
		setting_write(setting, &val);
	}
}

void setting_menu(){
	uint8_t input;
	do{
		LREP("--------- Setting ----------\r\n");
		LREP("1. Read setting\r\n");
		LREP("2. Write setting\r\n");
		LREP("q. Return\r\n");
		input = kb_cmd("cmd");
		switch(input){
			case 'q': break;
			case '1':{
				setting_dump_to_stdio(&g_setting_dev);
				break;
			}
			case '2':{
				setting_write_from_stdio(&g_setting_dev);
				break;
			}
		}
	}while(input != 'q');

}
int setting_read(struct setting_device* dev, struct setting_value* val){
	return at93c_read(&g_setting_dev.dev, 0, val, sizeof(struct setting_value));
}
int setting_write(struct setting_device* dev, struct setting_value* val){
	return at93c_write(&g_setting_dev.dev, 0, val, sizeof(struct setting_value));
}

// end of file
