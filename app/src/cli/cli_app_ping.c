#include <cli.h>
#include <stdlib.h>
#include <stdint.h>
#include <Network.h>
#include <string.h>

extern struct network				g_nwk;

const char cli_app_ping_description[] = "Ping to node";

int cli_app_ping_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_ping = {
	.cmd = "ping",
	.desc = cli_app_ping_description,
	.callback = cli_app_ping_callback,
};
cli_app(g_cli_app_ping);

void cli_app_ping_help(){
    LREP("\r\n");
    LREP("\t %s ?        Display help\r\n", g_cli_app_ping.cmd);
    LREP("\t %s address  Send requests packet to address\r\n", g_cli_app_ping.cmd);
}
int cli_app_ping_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0) cli_app_ping_help();
		else {
			if(strlen(argv[1]) > 0){
				uint16_t addr = strtol(argv[1], 0, 10);
				struct network_echo_info info;
				LREP("\r\nPing to %u\r\n", addr);
				Network_echo_request(&g_nwk, addr, 1, 16, &info);
				LREP("Result:\r\n");
				LREP("Total: %u\r\n", info.total);
				LREP("Successed: %u (%d%%)\r\n", info.passed, (info.total > 0) ? info.passed*100/info.total : 0);
			}
	   }

	}else cli_app_ping_help();
	return 0;
}





// end of file
