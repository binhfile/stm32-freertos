#include <cli.h>
#include <stdlib.h>
#include <stdint.h>
#include <Network.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include "project_config.h"

const char cli_app_test_description[] = "Test system";

int cli_app_test_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_test = {
	.cmd = "test",
	.desc = cli_app_test_description,
	.callback = cli_app_test_callback,
};
cli_app(g_cli_app_test);

void cli_app_test_help(){
    LREP("\r\n");
    LREP("\t %s ?\r\n", g_cli_app_test.cmd);
    LREP("\t\tShow help\r\n", g_cli_app_test.cmd);
}
int cli_app_test_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0) cli_app_test_help();
	}else cli_app_test_help();
	return 0;
}





// end of file
