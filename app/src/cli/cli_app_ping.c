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
				uint16_t addr = strtol(argv[1], 0, 16);
				int cnt = 10;
				int leng = 16;
				int ret;

				if(argc >= 3) cnt = strtol(argv[2], 0, 10);
				if(cnt < 0) cnt = 1;
				else if(cnt > 1000000) cnt = 1000000;
				if(argc >= 4) leng = strtol(argv[3], 0, 10);
				if(leng < 0) leng = 10;
				else if(leng > 100) leng = 100;

				struct network_echo_info info;
				LREP("\r\nPing to %04X count %u length %u\r\n", addr, cnt, leng);
				ret = Network_echo_request(&g_nwk, addr, cnt, leng, &info);
				LREP("Result:\r\n");
				if(ret > 10) LREP("Failed long times\r\n");
				LREP("Time:      %u ms\r\n", info.time_diff);
				LREP("Total:     %u\r\n", info.total);
				LREP("Successed: %u (%d%%)\r\n", info.passed, (info.total > 0) ? info.passed*100/info.total : 0);
				LREP("Timeout:   %u\r\n", info.timeout);
				LREP("Error:     %u\r\n", info.failed);
				LREP("Arg speed: %u.%u KiB/s\r\n", info.passed*leng / info.time_diff * 1000 / 1024,
						(info.passed*leng / info.time_diff * 1000 - (info.passed*leng / info.time_diff * 1000 / 1024)*1024)*10/1024);
			}
	   }

	}else cli_app_ping_help();
	return 0;
}





// end of file
