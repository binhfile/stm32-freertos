#include <cli.h>
#include <stdlib.h>
#include <stdint.h>
#include <Network.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include "project_config.h"

const char cli_app_test_description[] = "Test system";

extern struct mac_mrf24j40_open_param  rf_mac_init;
extern struct mac_mrf24j40         g_rf_mac;

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
    LREP("\t %s rf\r\n", g_cli_app_test.cmd);
    LREP("\t\tTest loop rf device\r\n", g_cli_app_test.cmd);
    LREP("\t %s rf-reset set/clear\r\n", g_cli_app_test.cmd);
    LREP("\t\tSet/Clear reset signal\r\n", g_cli_app_test.cmd);
    LREP("\t %s rf-cs set/clear\r\n", g_cli_app_test.cmd);
    LREP("\t\tSet/Clear cs signal\r\n", g_cli_app_test.cmd);
}
int cli_app_test_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0) cli_app_test_help();
		else if(strcmp(argv[1], "rf-reset") == 0){
			if(argc >= 3){
				if(strcmp(argv[2], "set") == 0){
					LREP("\r\nSet RF-RESET signal\r\n");
					uint8_t __u8val = '1';
					write(rf_mac_init.fd_reset, &__u8val, 1);
				}else if(strcmp(argv[2], "clear") == 0){
					LREP("\r\nClear RF-RESET signal\r\n");
					uint8_t __u8val = '0';
					write(rf_mac_init.fd_reset, &__u8val, 1);
				}
			}
		}
		else if(strcmp(argv[1], "rf-cs") == 0){
			if(argc >= 3){
				if(strcmp(argv[2], "set") == 0){
					LREP("\r\nSet RF-CS signal\r\n");
					uint8_t __u8val = '1';
					write(rf_mac_init.fd_cs, &__u8val, 1);
				}else if(strcmp(argv[2], "clear") == 0){
					LREP("\r\nClear RF-CS signal\r\n");
					uint8_t __u8val = '0';
					write(rf_mac_init.fd_cs, &__u8val, 1);
				}
			}
		}
		else if(strcmp(argv[1], "rf") == 0){
			LREP("\r\nTest loop rf device\r\n");
			uint8_t u8tx[32], u8rx[32];
			int i;
			struct spi_ioc_transfer xfer;

			u8rx[0] = 0;
			for(i = 0; i < 32; i++) u8tx[i] = u8rx[0]++;
			for(i = 0; i < 32; i++) u8rx[i] = 0;
			xfer.tx_buf			= (unsigned int)u8tx;
			xfer.rx_buf 		= (unsigned int)u8rx;
			xfer.bits_per_word 	= 8;
			xfer.speed_hz 		= 1000*1000;
			xfer.len			= 32;
			ioctl(g_rf_mac.phy.fd_spi, SPI_IOC_MESSAGE(1), (unsigned int)&xfer);
			for(i = 0; i < 32; i++){
				if(i % 16 == 0) LREP("\r\n");
				LREP("%02X ", u8rx[i]);
			}
			LREP("\r\ndone\r\n");
		}
	}else cli_app_test_help();
	return 0;
}





// end of file
