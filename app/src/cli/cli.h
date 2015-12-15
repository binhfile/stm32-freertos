/*
 * cli.h
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */

#ifndef SRC_CLI_H_
#define SRC_CLI_H_

extern void LREP(char* s, ...);
int CLI_loop();

typedef int (*cli_app_callback)(int argc, char** argv, void* user);

struct cli_app_info{
	const char* cmd;
	const char* desc;
	cli_app_callback callback;
};

#define cli_app(elem) void* __cli_app_info_##elem __attribute__((__section__(".cli_app"))) = &elem;

#endif /* SRC_CLI_H_ */
