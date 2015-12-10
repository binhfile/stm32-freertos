/*
 * Test.c
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#include "Test.h"
#include <debug.h>
#include <unistd.h>
#include <drv_gpio.h>
#include "network/mac/mac_mrf24j40.h"
#include <Network.h>
#include <Setup.h>
#define TEST_LEN	(90)

extern struct mac_mrf24j40	g_rf_mac;

void MAC_test_loop_received_packets();
void MAC_test_send_and_check_packets();

void Test_menu(){
	uint8_t userInput;
	LREP("------- TEST ------\r\n");
	LREP("1. MAC\r\n");
	LREP("2. Network\r\n");
	LREP("cmd? ");
	do{
		userInput = kbhit(1000);
	}while(!userInput);
	LREP("%c\r\n", userInput);
	switch(userInput){
		case '1':{
			MAC_test();
			break;
		}
		case '2':{
			Network_test();
			break;
		}
		default: break;
	}
}
void MAC_test(){
	uint8_t userInput;
	LREP("------- MAC test ------\r\n");
	LREP("1. Loop received packets\r\n");
	LREP("2. Send and check received packets\r\n");
	LREP("cmd? ");
	do{
		userInput = kbhit(1000);
	}while(!userInput);
	LREP("%c\r\n", userInput);

	switch(userInput){
		case '1':{
			MAC_test_loop_received_packets();
			break;
		}
		case '2':{
			MAC_test_send_and_check_packets();
			break;
		}
		default: break;
	}
}
void MAC_test_loop_received_packets(){
	uint8_t rxBuf[TEST_LEN];
	int i, len, payload_cnt = 0, packet_done_cnt = 0;
	struct mac_mrf24j40_write_param write_param;

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 1;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.destPrsnt	= 1;
	write_param.flags.bits.sourcePrsnt	= 0;
	write_param.flags.bits.repeat		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.destPANId 				= 0xffff;
	write_param.destAddress 			= 0xffff;
	LREP("Begin loop packets\r\n");
	while(kb_value() != 's'){
		len = MAC_mrf24j40_read(&g_rf_mac, rxBuf, TEST_LEN, 1000);
		if(len <= 0){
		}else{
			for(i = 0; i < TEST_LEN; i++){
				rxBuf[i]++;
			}
			MAC_mrf24j40_write(&g_rf_mac, &write_param, rxBuf, TEST_LEN);
			LED_TOGGLE(BLUE);
		}
	}
}
void MAC_test_send_and_check_packets(){
	uint8_t txBuf[TEST_LEN], rxBuf[TEST_LEN];
	uint8_t cnt = 0;
	int i, len, t_diff, payload_cnt = 0, packet_done_cnt = 0, packet_err_cnt = 0, packet_timeout_cnt = 0;
	struct mac_mrf24j40_write_param write_param;
	struct timespec t_ref, t_now;

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 1;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.destPrsnt	= 1;
	write_param.flags.bits.sourcePrsnt	= 0;
	write_param.flags.bits.repeat		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.destPANId 				= 0xffff;
	write_param.destAddress 			= 0xffff;

	LREP("Begin send packets\r\n");
	clock_gettime(CLOCK_REALTIME, &t_ref);
	while(kb_value() != 's'){
		for(i = 0; i < TEST_LEN; i++){
			txBuf[i] = cnt++;
		}
		MAC_mrf24j40_write(&g_rf_mac, &write_param, txBuf, TEST_LEN);
		len = MAC_mrf24j40_read(&g_rf_mac, rxBuf, TEST_LEN, 1000);
		if(len <= 0){
			LREP("timeout\r\n");
			packet_timeout_cnt++;
		}else{
			for(i = 0; i < TEST_LEN; i++){
				txBuf[i]++;
				if(rxBuf[i] != txBuf[i]){
					break;
				}
			}
			if(i < TEST_LEN){
				LREP("error packet\r\n");
				packet_err_cnt++;
				LED_TOGGLE(RED);
			}
			else {
				payload_cnt += TEST_LEN;
				packet_done_cnt++;
				LED_TOGGLE(BLUE);
			}
		}
	}
	clock_gettime(CLOCK_REALTIME, &t_now);
	t_diff = t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000 - (t_ref.tv_sec * 1000 + t_ref.tv_nsec / 1000000);
	LREP("Results:\r\n");
	LREP("Diff time:          %d ms\r\n", t_diff);
	LREP("Total payload done: %d\r\n", payload_cnt);
	LREP("Packets done:       %d\r\n", packet_done_cnt);
	LREP("Packets error:      %d\r\n", packet_err_cnt);
	LREP("Packets timeout:    %d\r\n", packet_timeout_cnt);
	if(t_diff > 0)
	LREP("Speed:              %d.%d KB/s\r\n", payload_cnt / t_diff * 1000 / 1024,
			(payload_cnt / t_diff * 1000 - (payload_cnt / t_diff * 1000 / 1024)*1024)*10/1024);
}
void Network_test(){
	uint8_t userInput;
	uint8_t noise_level[15];

	LREP("------- Network test ------\r\n");
	LREP("1. Scan noise channel\r\n");
	LREP("cmd? ");
	do{
		userInput = kbhit(1000);
	}while(!userInput);
	LREP("%c\r\n", userInput);

	switch(userInput){
		case '1':{
			Network_scan_channel(&g_rf_mac, 0x03fff800, noise_level);
			break;
		}
		case '2':{
			break;
		}
		default: break;
	}
}
// end of file
