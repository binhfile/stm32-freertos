/*
 * Network.c
 *
 *  Created on: Dec 10, 2015
 *      Author: dev
 */

#include "Network.h"
#include <debug.h>
#include <unistd.h>

#define NETWORK_H_BAR_LEN	80
int Network_scan_channel(struct mac_mrf24j40 *mac, uint32_t channels, uint8_t * noise_level){
	int timeout = 1000;
	int ret = 0;
	unsigned int i, j;
	struct mac_channel_assessment ch_assessment;
	struct timespec t_now, t_ref;
	uint8_t u8val;

	i = 0;
	j = NETWORK_H_BAR_LEN;
	LREP("    MIN");
	while(j > 0){
		LREP("-");
		j--;
	}
	LREP("MAX\r\n");
	while(i < 32){
		if( (((uint32_t)1) << i) & channels ){
			LREP("ch: %02d ", i);
			u8val = 0;
			MAC_mrf24j40_ioctl(mac, mac_mrf24j40_ioc_set_channel, (unsigned int)&i);
			clock_gettime(CLOCK_REALTIME, &t_ref);
			while(1){
				MAC_mrf24j40_ioctl(mac, mac_mrf24j40_ioc_channel_assessment, (unsigned int)&ch_assessment);
				if(ch_assessment.noise_level > u8val) u8val = ch_assessment.noise_level;
				clock_gettime(CLOCK_REALTIME, &t_now);
				if(t_now.tv_sec < t_ref.tv_sec) break;
				if((t_now.tv_sec*1000 + t_now.tv_nsec/1000000) - (t_ref.tv_sec*1000 + t_ref.tv_nsec/1000000) >= timeout){
					break;
				}else{
					usleep(1000* 10);
				}
			}
			*noise_level = u8val;
			noise_level++;
			j = u8val * NETWORK_H_BAR_LEN / 255;
			while(j > 0){
				LREP("-");
				j--;
			}
			LREP(" %02X\r\n", u8val);
		}
		i++;
	}

	return ret;
}
// end of file
