/*
 * cli_app_date.c
 *
 *  Created on: Dec 15, 2015
 *      Author: dev
 */

#include <cli.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

const char cli_app_date_description[] = "Print or set the system date and time";

int cli_app_date_callback(int argc, char** argv, void* user);
struct cli_app_info g_cli_app_date = {
	.cmd = "date",
	.desc = cli_app_date_description,
	.callback = cli_app_date_callback,
};
cli_app(g_cli_app_date);

void cli_app_date_help(){
    LREP("\r\n");
    LREP("\t %s ?                       Display help\r\n", g_cli_app_date.cmd);
    LREP("\t %s                         Display system date time\r\n", g_cli_app_date.cmd);
    LREP("\t %s -s yyyy/MM/dd-hh:mm:ss  Set system date time\r\n", g_cli_app_date.cmd);
}
int cli_app_date_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0){
			cli_app_date_help();
		}
		else if(strcmp(argv[1], "-s") == 0 && argc >= 3 &&
				strlen(argv[2]) >= strlen("yyyy/MM/dd hh/mm/ss")){
			// format 'yyyy/MM/dd hh/mm/ss'
			time_t tm_t;
			struct tm s_tm;
			s_tm.tm_year = (argv[2][0] - '0')*1000 +
					(argv[2][1] - '0') * 100 +
					(argv[2][2] - '0') * 10 +
					(argv[2][3] - '0');
			s_tm.tm_mon = (argv[2][5] - '0')*10+(argv[2][6] - '0');
			s_tm.tm_mday = (argv[2][8] - '0')*10+(argv[2][9] - '0');
			s_tm.tm_hour = (argv[2][11] - '0')*10+(argv[2][12] - '0');
			s_tm.tm_min = (argv[2][14] - '0')*10+(argv[2][15] - '0');
			s_tm.tm_sec = (argv[2][17] - '0')*10+(argv[2][18] - '0');

			if(s_tm.tm_year >= 1970 && s_tm.tm_year <= 2099 &&
					s_tm.tm_mon > 0 && s_tm.tm_mon <=12 &&
					s_tm.tm_mday > 0 && s_tm.tm_mday <= 31 &&
					s_tm.tm_hour >= 0 && s_tm.tm_hour < 24 &&
					s_tm.tm_min >= 0 && s_tm.tm_min < 60 &&
					s_tm.tm_sec >= 0 && s_tm.tm_sec < 60){
				tm_t = mktime(&s_tm);
				stime(&tm_t);
			}else{
				LREP("\r\nInvalid param\r\n");
			}
		}else {
			LREP("\r\nInvalid param\r\n");
		}
	}else{
		time_t tm_t = time(0);
		struct tm tm_result;
		localtime_r(&tm_t, &tm_result);
		LREP("\r\n%d-%d-%d %d:%d:%d\r\n",
				tm_result.tm_year,
				tm_result.tm_mon,
				tm_result.tm_mday,
				tm_result.tm_hour,
				tm_result.tm_min,
				tm_result.tm_sec);
	}
	return 0;
}


