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
#include <setting.h>
#include <string.h>
#include <fcntl.h>
#define TEST_LEN	(90)

extern struct mac_mrf24j40	g_rf_mac;
extern struct setting_device	g_setting_dev;

void MAC_test_loop_received_packets(){
	uint8_t rxBuf[TEST_LEN];
	int i, len;
	unsigned int uival;
	struct mac_mrf24j40_write_param write_param;
	struct mac_mrf24j40_read_param read_param;
	struct setting_value setting;

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 1;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.destPrsnt	= 1;
	write_param.flags.bits.sourcePrsnt	= 0;
	write_param.flags.bits.repeat		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.destPANId 				= 0xffff;
	write_param.destAddress 			= 0xffff;

	setting_read(&g_setting_dev, &setting);
	uival = 25;
	MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&uival);
	for(i = 0; i < 8 ; i++)
		rxBuf[i] = setting.mac_long_address[i];
	MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_long_address, (unsigned int)rxBuf);

	//LREP("Begin loop packets\r\n");
	while(kb_value() != 's'){
		len = MAC_mrf24j40_read(&g_rf_mac, &read_param, rxBuf, TEST_LEN, 1000);
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
	uint8_t* pu8;
	unsigned int uival;
	int i, len, t_diff, payload_cnt = 0, packet_done_cnt = 0, packet_err_cnt = 0, packet_timeout_cnt = 0;
	struct mac_mrf24j40_write_param write_param;
	struct mac_mrf24j40_read_param read_param;
	struct timespec t_ref, t_now;
	struct setting_value setting;

	write_param.flags.bits.packetType 	= MAC_MRF24J40_PACKET_TYPE_DATA;
	write_param.flags.bits.broadcast	= 1;
	write_param.flags.bits.ackReq		= 0;
	write_param.flags.bits.destPrsnt	= 1;
	write_param.flags.bits.sourcePrsnt	= 0;
	write_param.flags.bits.repeat		= 0;
	write_param.flags.bits.secEn		= 0;
	write_param.destPANId 				= 0xFFFF;
	write_param.destAddress 			= 0xFFFFFFFFFFFFFFFF;

	setting_read(&g_setting_dev, &setting);
	uival = 25;
	MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&uival);
	for(i = 0; i < 8 ; i++)
		rxBuf[i] = setting.mac_long_address[i];
	MAC_mrf24j40_ioctl(&g_rf_mac, mac_mrf24j40_ioc_set_long_address, (unsigned int)rxBuf);

	//LREP("Begin send packets\r\n");
	clock_gettime(CLOCK_REALTIME, &t_ref);
	while(kb_value() != 's'){
		for(i = 0; i < TEST_LEN; i++){
			txBuf[i] = cnt++;
		}
		MAC_mrf24j40_write(&g_rf_mac, &write_param, txBuf, TEST_LEN);
		len = MAC_mrf24j40_read(&g_rf_mac, &read_param, rxBuf, TEST_LEN, 1000);
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
	LREP("Partner: ");
	pu8 = (uint8_t*)&read_param.srcAddr;
	for(i = 0; i < 7; i++) LREP("%02X:", pu8[i]);
	LREP("%02X", pu8[i]);
	LREP("@%02X%02X\r\n",
			((uint8_t)((read_param.srcPANid & 0xFF00) >> 8)),
			((uint8_t)(read_param.srcPANid & 0x00FF)));
}
void Setting_test_erase_and_write(){
	uint8_t buf[AT93C66_SIZE], wBuf[AT93C66_SIZE];
	int i;
	LREP("Erase eeprom ...");
	at93c_ioctl(&g_setting_dev.dev, at93c_ioc_erase_all, 0);
	at93c_read(&g_setting_dev.dev, 0, buf, AT93C66_SIZE);
	for(i = 0; i < AT93C66_SIZE; i++){
		if(buf[i] != 0xFF) break;
	}
	if(i == AT93C66_SIZE) LREP("DONE\r\n");
	else LREP("FALSE\r\n");

	for(i = 0; i < AT93C66_SIZE; i++){
		wBuf[i] = i;
		buf[i] = 0;
	}
	LREP("Write pattern and verify ...");
	at93c_write(&g_setting_dev.dev, 0, wBuf, AT93C66_SIZE);
	at93c_read(&g_setting_dev.dev, 0, buf, AT93C66_SIZE);
	for(i = 0; i < AT93C66_SIZE; i++){
		if(buf[i] != wBuf[i]) break;
	}
	if(i == AT93C66_SIZE) LREP("DONE\r\n");
	else LREP("FALSE\r\n");

}
// end of file
