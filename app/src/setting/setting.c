/*
 * Setup.c
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#include "setting.h"
#include <debug.h>
#include <stdlib.h>
#if defined(OS_LINUX)
#include <stdio.h>
#include <string.h>
#endif

extern struct setting_device		g_setting_dev;

const char* node_type_to_str(uint8_t type){
	switch(type){
		case setting_network_type_pan_coordinator: return "PAN coordinator"; break;
		case setting_network_type_router: return "Router"; break;
		case setting_network_type_end: return "End device"; break;
		default: return "Unknown"; break;
	}
}
void setting_dump_to_stdio(struct setting_value *val){
	int i;
	LREP("Magic: %02X (%s)\r\n", val->magic_id, (val->magic_id == setting_value_magic_id) ? "correct" : "INCorrect");
	LREP("Note type: %d (%s)\r\n", val->network_type, node_type_to_str(val->network_type));
	LREP("Long address: ");
	for(i = 0; i < 7 ; i ++) LREP("%02X:", val->mac_long_address[i]);
	LREP("%02X", val->mac_long_address[i]);
	LREP("\r\n");
}
uint8_t hex_to_dec(uint8_t hex){
	if(hex >= '0' && hex <= '9') return hex - '0';
	else if(hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
	else if(hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
	else return 0;
}
//void setting_write_from_stdio(struct setting_device *setting){
//	struct setting_value val;
//	int i;
//	uint8_t input;
//	uint32_t u32Val;
//	if(setting_read(setting, &val) < 0){
//		LREP("Read failed\r\n");
//	}else{
//		LREP("Setting:\r\n");
//		// node type
//		LREP("Note type: %d (%s), ", val.network_type, node_type_to_str(val.network_type));
//		do{
//			input = kb_cmd("new");
//			if(input >= '0' && input <= '9') break;
//		}while(1);
//		val.network_type = input - '0';
//		// Long address
//		LREP("Long address: ");
//		for(i = 0; i < 7 ; i ++) LREP("%02X:", val.mac_long_address[i]);
//		LREP("%02X\r\n", val.mac_long_address[i]);
//		LREP("1. Auto create random address\r\n");
//		LREP("2. Manual edit address\r\n");
//		input = kb_cmd("cmd");
//		if(input == '1'){
//			u32Val = rand();
//			val.mac_long_address[0] = (uint8_t)(u32Val & (uint32_t)0x00FF);
//			val.mac_long_address[1] = (uint8_t)((u32Val>> 8) & (uint32_t)0x00FF);
//			val.mac_long_address[2] = (uint8_t)((u32Val>> 16) & (uint32_t)0x00FF);
//			val.mac_long_address[3] = (uint8_t)((u32Val>> 24) & (uint32_t)0x00FF);
//			u32Val = rand();
//			val.mac_long_address[4] = (uint8_t)(u32Val & (uint32_t)0x00FF);
//			val.mac_long_address[5] = (uint8_t)((u32Val>> 8) & (uint32_t)0x00FF);
//			val.mac_long_address[6] = (uint8_t)((u32Val>> 16) & (uint32_t)0x00FF);
//			val.mac_long_address[7] = (uint8_t)((u32Val>> 24) & (uint32_t)0x00FF);
//			LREP("New address: ");
//			for(i = 0; i < 7 ; i ++) LREP("%02X:", val.mac_long_address[i]);
//			LREP("%02X\r\n", val.mac_long_address[i]);
//		}else if(input == '2'){
//			for(i = 0; i < 8 ; i++){
//				LREP("address[%d]: ", i);
//				input = kbhit(1000*30); LREP("%c", input); val.mac_long_address[i] = (hex_to_dec(input) << 4) & 0xF0;
//				input = kbhit(1000*30); LREP("%c", input); val.mac_long_address[i] |= (hex_to_dec(input)) & 0x0F;
//				kbhit(1000*30);// ENTER
//				LREP("\r\n");
//			}
//		}else{
//			LREP("Not write setting\r\n");
//			return;
//		}
//		// write back
//		val.magic_id = setting_value_magic_id;
//		setting_write(setting, &val);
//	}
//}
int setting_read(struct setting_device* dev, struct setting_value* val){
#if defined(OS_FREERTOS)
	return at93c_read(&g_setting_dev.dev, 0, val, sizeof(struct setting_value));
#elif defined(OS_LINUX)
	FILE* f = fopen("config.bin", "rb");
	int len = 0;
	memset(val, 0, sizeof(struct setting_value));
	if(f){
		len = fread(val, 1, sizeof(struct setting_value), f);
		fclose(f);
	}
	return len;
#endif
}
int setting_write(struct setting_device* dev, struct setting_value* val){
#if defined(OS_FREERTOS)
	return at93c_write(&g_setting_dev.dev, 0, val, sizeof(struct setting_value));
#elif defined(OS_LINUX)
	FILE* f = fopen("config.bin", "wb");
	int len = 0;
	if(f){
		len = fwrite(val, 1, sizeof(struct setting_value), f);
		fclose(f);
	}else LREP("Open file failed\r\n");
	return len;
#endif
}

// end of file
