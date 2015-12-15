/*
 * cli_app_network.c
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */
#include <cli.h>
#include <stdlib.h>
#include <stdint.h>
#include <Network.h>
#include <string.h>

extern struct mac_mrf24j40         	g_rf_mac;

const char cli_app_network_description[] = "Network layer tools";

int cli_app_network_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_network = {
	.cmd = "network",
	.desc = cli_app_network_description,
	.callback = cli_app_network_callback,
};
cli_app(g_cli_app_network);

void cli_app_network_help(){
    LREP("\r\n");
    LREP("\t %s ?     Display help\r\n", g_cli_app_network.cmd);
    LREP("\t %s scan  Scan free channel\r\n", g_cli_app_network.cmd);
}
int cli_app_network_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0) cli_app_network_help();
		else if(strcmp(argv[1], "scan") == 0){
			uint8_t noise_level[13];
			LREP("\r\n");
			Network_scan_channel(&g_rf_mac, 0x03FFF800, noise_level);
			LREP("\r\n");
	   }

	}else cli_app_network_help();
	return 0;
}



