/*
 * cli_app_uname.c
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */
#include <cli.h>

const char cli_app_uname_description[] = "Print system infomation";

int cli_app_uname_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_uname = {
	.cmd = "uname",
	.desc = cli_app_uname_description,
	.callback = cli_app_uname_callback,
};
cli_app(g_cli_app_uname);
int cli_app_uname_callback(int argc, char** argv, void* user){
	LREP("\r\nSystem version 0.1 build %s %s", __TIME__, __DATE__);
	return 0;
}



