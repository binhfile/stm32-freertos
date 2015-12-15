/*
 * cliappreboot.cpp
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */

#include <cli.h>
#include <reboot.h>
#include <unistd.h>

const char cli_app_reboot_description[] = "Reboot system";

int cli_app_reboot_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_reboot = {
	.cmd = "reboot",
	.desc = cli_app_reboot_description,
	.callback = cli_app_reboot_callback,
};
cli_app(g_cli_app_reboot);
int cli_app_reboot_callback(int argc, char** argv, void* user){
	LREP("\r\nrebooting...");
	usleep(1000*100);
	reboot();
	return 0;
}
