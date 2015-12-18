/*
 * cli.h
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */

#ifndef SRC_LIB_CLI_H_
#define SRC_LIB_CLI_H_

typedef int (*lib_cli_callback)(int argc, char** argv, void* user);
struct lib_cli{
	int fd_read;
	int fd_write;
	char prompt;
	const char* hostname;
	const char* banner;

	lib_cli_callback cb_fxn;
	void*		 cb_user;
};

int lib_cli_init(struct lib_cli *inst);
int lib_cli_register_callback(struct lib_cli *inst, lib_cli_callback cb, void* cb_user);
int lib_cli_set_readfd(struct lib_cli *inst, int fd);
int lib_cli_set_writefd(struct lib_cli *inst, int fd);
int lib_cli_set_promptchar(struct lib_cli *inst, char val);
int lib_cli_set_hostname(struct lib_cli *inst, const char* hostname);
int lib_cli_set_banner(struct lib_cli *inst, const char* banner);
int lib_cli_loop(struct lib_cli *inst, int timeout);

#endif /* SRC_CLI_H_ */
