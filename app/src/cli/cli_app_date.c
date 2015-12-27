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
#include <slre.h>

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
    LREP("\t %s -s \"yyyy/MM/dd hh:mm:ss\"  Set system date time\r\n", g_cli_app_date.cmd);
}
int cli_app_date_callback(int argc, char** argv, void* user){
	if(argc >= 2){
		if(strcmp(argv[1], "?") == 0){
			cli_app_date_help();
		}
		else if(strcmp(argv[1], "-s") == 0 && argc >= 3){
			// format 'yyyy/MM/dd hh/mm/ss'
			time_t tm_t;
			struct tm s_tm;
			struct slre_cap caps[7];
			char buff[5];
			int ret = slre_match("\\s*(\\d+)/(\\d+)/(\\d+)\\s+(\\d+):(\\d+):(\\d+)\\s*",
					argv[2], strlen(argv[2]), caps, 7, 0);
			memset(&s_tm, 0, sizeof(s_tm));
			if(ret > 0){
				if(caps[0].len == 4){
					memset(buff, 0, 5);
					memcpy(buff, caps[0].ptr, caps[0].len);
					s_tm.tm_year = strtol(buff, 0, 10);
				}
				if(caps[1].len > 0){
					memset(buff, 0, 5);
					memcpy(buff, caps[1].ptr, caps[1].len);
					s_tm.tm_mon = strtol(buff, 0, 10);
				}
				if(caps[2].len > 0){
					memset(buff, 0, 5);
					memcpy(buff, caps[2].ptr, caps[2].len);
					s_tm.tm_mday = strtol(buff, 0, 10);
				}

				if(caps[3].len > 0) {
					memset(buff, 0, 5);
					memcpy(buff, caps[3].ptr, caps[3].len);
					s_tm.tm_hour = strtol(buff, 0, 10);
				}
				if(caps[4].len > 0) {
					memset(buff, 0, 5);
					memcpy(buff, caps[4].ptr, caps[4].len);
					s_tm.tm_min = strtol(buff, 0, 10);
				}
				if(caps[5].len > 0) {
					memset(buff, 0, 5);
					memcpy(buff, caps[5].ptr, caps[5].len);
					s_tm.tm_sec = strtol(buff, 0, 10);
				}
			}

			if(s_tm.tm_year >= 1970 && s_tm.tm_year <= 2099 &&
					s_tm.tm_mon > 0 && s_tm.tm_mon <=12 &&
					s_tm.tm_mday > 0 && s_tm.tm_mday <= 31 &&
					s_tm.tm_hour >= 0 && s_tm.tm_hour < 24 &&
					s_tm.tm_min >= 0 && s_tm.tm_min < 60 &&
					s_tm.tm_sec >= 0 && s_tm.tm_sec < 60){
				s_tm.tm_year-= 1900;
				tm_t = mktime(&s_tm);
				stime(&tm_t);
			}else{
				LREP("\r\nInvalid param '%d/%d/%d %d:%d:%d' [%d]\r\n",
						s_tm.tm_year,
						s_tm.tm_mon,
						s_tm.tm_mday,
						s_tm.tm_hour,
						s_tm.tm_min,
						s_tm.tm_sec, ret);
			}
		}else {
			LREP("\r\nInvalid param\r\n");
		}
	}else{
		time_t tm_t = time(0);
		struct tm tm_result;
		localtime_r(&tm_t, &tm_result);
		LREP("\r\n%d-%d-%d %d:%d:%d\r\n",
				tm_result.tm_year+1900,
				tm_result.tm_mon+1,
				tm_result.tm_mday,
				tm_result.tm_hour,
				tm_result.tm_min,
				tm_result.tm_sec);
	}
	return 0;
}


