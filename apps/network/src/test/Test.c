/*
 * Test.c
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#include "project_config.h"
#include "Test.h"
#include <debug.h>
#include <unistd.h>
#include <Network.h>
#include <setting.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#define TEST_LEN	(90)

extern struct network           g_nwk;
extern struct setting_device	g_setting_dev;

void MAC_test_loop_received_packets(){
	uint8_t rxBuf[TEST_LEN];
	uint8_t txBuf[TEST_LEN];
	int i, len;
	unsigned int uival;
	struct setting_value setting;
    struct rf_mac_read_packet   *read_mac;
    struct rf_mac_write_packet  *write_mac;
    struct pollfd poll_fd[1];

    read_mac = (struct rf_mac_read_packet*)rxBuf;
    write_mac = (struct rf_mac_write_packet*)txBuf;

    write_mac->header.flags.bits.ack_req          = 0;
    write_mac->header.flags.bits.broadcast        = 1;
    write_mac->header.flags.bits.intra_pan        = 1;
    write_mac->header.flags.bits.dest_addr_64bit  = 0;
    write_mac->header.flags.bits.src_addr_64bit   = 1;
    SET_RF_MAC_WRITE_DEST_PAN(&write_mac->header, 0xffff)
    SET_RF_MAC_WRITE_DEST_ADDR_SHORT(&write_mac->header, 0xffff);

	setting_read(&g_setting_dev, &setting);
	uival = 25;
	ioctl(g_nwk.mac_fd, RF_MAC_IOC_WR_CHANNEL, &uival);
	for(i = 0; i < 8 ; i++)
		rxBuf[i] = setting.mac_long_address[i];
	ioctl(g_nwk.mac_fd, RF_MAC_IOC_WR_LONG_ADDRESS, rxBuf);

	//LREP("Begin loop packets\r\n");
	while(kb_value() != 's'){
	    poll_fd[0].fd = g_nwk.mac_fd;
        poll_fd[0].events = POLLIN;
        poll_fd[0].revents = 0;
        len = poll(poll_fd, 1, 1000);
        read_mac->data_len = 0;
        if(len > 0 && (poll_fd[0].revents & POLLIN)){
            len = ioctl(g_nwk.mac_fd, RF_MAC_IOC_RD_PACKET, read_mac);
        }
		if(read_mac->data_len <= 0){
		}else{
			for(i = 0; i < read_mac->data_len; i++){
			    read_mac->data[i]++;
			}
			memcpy(write_mac->data, read_mac->data, read_mac->data_len);
			write_mac->data_len = read_mac->data_len;
			ioctl(g_nwk.mac_fd, RF_MAC_IOC_WR_PACKET, write_mac);
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
	struct timespec t_ref, t_now;
	struct setting_value setting;
    struct rf_mac_read_packet   *read_mac;
    struct rf_mac_write_packet  *write_mac;
    struct pollfd poll_fd[1];

    read_mac = (struct rf_mac_read_packet*)rxBuf;
    write_mac = (struct rf_mac_write_packet*)txBuf;

    write_mac->header.flags.bits.ack_req          = 0;
    write_mac->header.flags.bits.broadcast        = 1;
    write_mac->header.flags.bits.intra_pan        = 1;
    write_mac->header.flags.bits.dest_addr_64bit  = 0;
    write_mac->header.flags.bits.src_addr_64bit   = 1;
    SET_RF_MAC_WRITE_DEST_PAN(&write_mac->header, 0xffff)
    SET_RF_MAC_WRITE_DEST_ADDR_SHORT(&write_mac->header, 0xffff);

	setting_read(&g_setting_dev, &setting);
	uival = 25;
	ioctl(g_nwk.mac_fd, RF_MAC_IOC_WR_CHANNEL, &uival);
	for(i = 0; i < 8 ; i++)
		rxBuf[i] = setting.mac_long_address[i];
	ioctl(g_nwk.mac_fd, RF_MAC_IOC_WR_LONG_ADDRESS, rxBuf);

	//LREP("Begin send packets\r\n");
	clock_gettime(CLOCK_REALTIME, &t_ref);
	while(kb_value() != 's'){
		for(i = 0; i < TEST_LEN; i++){
		    write_mac->data[i] = cnt++;
		}
		write_mac->data_len = TEST_LEN;
		ioctl(g_nwk.mac_fd, RF_MAC_IOC_WR_PACKET, write_mac);

		poll_fd[0].fd = g_nwk.mac_fd;
        poll_fd[0].events = POLLIN;
        poll_fd[0].revents = 0;
        len = poll(poll_fd, 1, 1000);
        read_mac->data_len = 0;
        if(len > 0 && (poll_fd[0].revents & POLLIN)){
            len = ioctl(g_nwk.mac_fd, RF_MAC_IOC_RD_PACKET, read_mac);
        }
		if(read_mac->data_len <= 0){
			LREP("timeout\r\n");
			packet_timeout_cnt++;
		}else{
			for(i = 0; i < TEST_LEN; i++){
			    write_mac->data[i]++;
				if(read_mac->data[i] != write_mac->data[i]){
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
	for(i = 0; i < 7; i++) LREP("%02X:", read_mac->header.src_addr[i]);
	LREP("%02X", read_mac->header.src_addr[i]);
	LREP("@%02X%02X\r\n",read_mac->header.dest_pan_id[1], read_mac->header.dest_pan_id[0]);
}
void Setting_test_erase_and_write(){
	uint8_t buf[512], wBuf[512];
	int i;
	LREP("Erase eeprom ...");

	ioctl(g_setting_dev.eeprom_fd, AT93C_IOC_WR_ERASE_ALL, 0);
	lseek(g_setting_dev.eeprom_fd, 0 , SEEK_SET);
	read(g_setting_dev.eeprom_fd, buf, 512);

	for(i = 0; i < 512; i++){
		if(buf[i] != 0xFF) break;
	}
	if(i == 512) LREP("DONE\r\n");
	else LREP("FALSE\r\n");

	for(i = 0; i < 512; i++){
		wBuf[i] = i;
		buf[i] = 0;
	}
	LREP("Write pattern and verify ...");
	lseek(g_setting_dev.eeprom_fd, 0 , SEEK_SET);
	write(g_setting_dev.eeprom_fd, wBuf, 512);
	lseek(g_setting_dev.eeprom_fd, 0 , SEEK_SET);
	read(g_setting_dev.eeprom_fd, buf, 512);
	for(i = 0; i < 512; i++){
		if(buf[i] != wBuf[i]) break;
	}
	if(i == 512) LREP("DONE\r\n");
	else LREP("FALSE\r\n");

}

// end of file
