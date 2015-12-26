#include <cli.h>
#include <stdlib.h>
#include <stdint.h>
#include <Network.h>
#include <string.h>
#include <getopt.h>
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
    LREP("\t %s ?\r\n", g_cli_app_ping.cmd);
    LREP("\t\tDisplay help\r\n", g_cli_app_ping.cmd);
    LREP("\t %s [-c count] [-s packetsize] address\r\n", g_cli_app_ping.cmd);
    LREP("\t\t Send and verify echo packet to address\r\n");
    LREP("\t\t Options:\r\n");
    LREP("\t\t -c count\r\n");
    LREP("\t\t\t Number of packet is send\r\n");
    LREP("\t\t -s packetsize\r\n");
    LREP("\t\t\t Size of a packet in bytes\r\n");
}
int cli_app_ping_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0) cli_app_ping_help();
		else {
			if(strlen(argv[1]) > 0){
				uint16_t addr = 0xffff;
				int cnt 	= 1;
				int leng 	= 32;
				int ret;
				int c;
				int done = 1;
				int one_dir = 0;

				optind = 1;
				while((c = getopt(argc, argv, "c:s:o")) != -1){
					switch(c){
						case 'c':{
							cnt = strtol(optarg, 0, 10);
							break;
						}
						case 's':{
							leng = strtol(optarg, 0, 10);
							break;
						}
						case 'o':{
							one_dir = 1;
							break;
						}
						case '?':{
							cli_app_ping_help();
							done = 0;
							break;
						}
						default: {
							LREP("default\r\n");
						}
					}
				}
				if(done){
					if (optind < argc){
						addr = strtol(argv[optind], 0, 16);
					}

					if(cnt < 0) cnt = 1;
					else if(cnt > 1000000) cnt = 1000000;
					if(leng < 0) leng = 10;
					else if(leng > 95) leng = 95;

					struct network_echo_info info;
					LREP("\r\nPing to %04X count %u length %u %s\r\n", addr, cnt, leng, one_dir ? "one dir": "");
					ret = Network_echo_request(&g_nwk, addr, cnt, leng, &info, one_dir);
					LREP("Result:\r\n");
					if(ret > 10) LREP("Failed long times\r\n");
					LREP("Time:      %u ms\r\n", info.time_diff);
					LREP("Total:     %u\r\n", info.total);
					LREP("Successed: %u (%d%%)\r\n", info.passed, (info.total > 0) ? info.passed*100/info.total : 0);
					LREP("Timeout:   %u\r\n", info.timeout);
					LREP("Error:     %u\r\n", info.failed);
					int speed = info.passed*leng*1000 / info.time_diff;
					LREP("Arg speed: %u.%u Kbps (%u.%u KiB)/s\r\n",
							speed * 8 / 1024, ((speed*8 - (speed*8/1024)*1024)*10 / 1024),
							speed / 1024, (speed - (speed / 1024)*1024)*10/1024);
				}
			}
	   }

	}else cli_app_ping_help();
	return 0;
}





// end of file
