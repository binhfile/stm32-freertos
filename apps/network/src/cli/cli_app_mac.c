/*
 * cli_app_mac.c
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */
#include <cli.h>
#include <stdlib.h>
#include <stdint.h>
#include <Test.h>
#include <string.h>
#include <Network.h>
const char cli_app_mac_description[] = "MAC layer tools";
extern struct network                   g_nwk;
int cli_app_mac_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_mac = {
	.cmd = "mac",
	.desc = cli_app_mac_description,
	.callback = cli_app_mac_callback,
};
cli_app(g_cli_app_mac);

void cli_app_mac_help(){
    LREP("\r\n");
    LREP("\t %s ?        Display help\r\n", g_cli_app_mac.cmd);
    LREP("\t %s loop     Loop received packets to sender\r\n", g_cli_app_mac.cmd);
    LREP("\t %s send     Send and verify received packets from repeater\r\n", g_cli_app_mac.cmd);
    LREP("\t %s channel  Print current channel\r\n", g_cli_app_mac.cmd);
}
int cli_app_mac_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0) cli_app_mac_help();
		else if(strcmp(argv[1], "loop") == 0){
			LREP("\r\nloop packets\r\n");
			MAC_test_loop_received_packets();
		}else if(strcmp(argv[1], "send") == 0){
			LREP("\r\nsend and verify packets\r\n");
			MAC_test_send_and_check_packets();
		}else if(strcmp(argv[1], "channel") == 0){
		    unsigned int ch = 0;
		    ioctl(g_nwk.mac_fd, RF_MAC_IOC_RD_CHANNEL, &ch);
			LREP("\r\n%u\r\n", ch);
		}

	}else cli_app_mac_help();
	return 0;
}



