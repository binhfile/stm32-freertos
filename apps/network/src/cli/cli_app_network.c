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

extern struct network				g_nwk;
const char cli_app_network_description[] = "Network layer tools";

int cli_app_network_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_network = {
	.cmd = "nwk",
	.desc = cli_app_network_description,
	.callback = cli_app_network_callback,
};
cli_app(g_cli_app_network);

void cli_app_network_help(){
    LREP("\r\n");
    LREP("\t %s ?      Display help\r\n", g_cli_app_network.cmd);
    LREP("\t %s scan   Scan free channel\r\n", g_cli_app_network.cmd);
    LREP("\t %s child  Display list children\r\n", g_cli_app_network.cmd);
    LREP("\t %s parent Display parent info\r\n", g_cli_app_network.cmd);
}
int cli_app_network_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0) cli_app_network_help();
		else if(strcmp(argv[1], "scan") == 0){
			uint8_t noise_level[13];
			LREP("\r\n");
			Network_scan_channel(&g_nwk, 0x03FFF800, noise_level);
			LREP("\r\n");
	   }
		else if(strcmp(argv[1], "parent") == 0){
			LREP("\r\n");
			LREP("ID: %04X\r\n", g_nwk.parent_id);
		}
		else if(strcmp(argv[1], "child") == 0){
			LREP("\r\n");
			int i, cnt = 1, j;
			for(i =0; i < NWK_CHILD_CNT; i++){
				if(g_nwk.child[i].flags.bits.active){
					LREP("[%02d] ID: %04X\r\n", cnt++, g_nwk.child[i].id);
					LREP("\t Long address: ");
					for(j = 0; j < 7; j++) LREP("%02X:", g_nwk.child[i].long_address[j]);
					LREP("%02X\r\n", g_nwk.child[i].long_address[j]);
				}
			}
		}
	}else cli_app_network_help();
	return 0;
}



