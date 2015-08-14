/*
 ============================================================================
 Name        : apx_hftsc.c
 Author      : xxx (xxx@appexnetworks.com)
 Version     :
 Copyright   : appex
 Description : 
 	xxxx
 ============================================================================
 */
	   
#include "apx_hftsc.h"


#define HFTSC_DEBUG 
#ifdef HFTSC_DEBUG
#undef DBG
#define DBG(fmt, args...)	fprintf(stderr, "[%s(%d)]: "fmt, __func__, __LINE__, ##args)
// #define DBG(fmt, args...)
#else
#undef DBG
#define DBG(fmt, args...)	;
#endif

int apx_hftsc_init(char* conf_file)
{
	int ret = 0;
	char logfile[128] = {0};

	ret = apx_conf_init(conf_file);
	if (ret != 0)
	{
		fprintf(stderr, "config file init failed,err code %d\n", ret);
		return -1;
	}

	ret = apx_conf_log_get(logfile, sizeof(logfile));
	if(ret != 0)
	{
		fprintf(stderr, "get log file path failed,err code %d\n", ret);
		return -2;
	}
	
	ret = apx_wlog_init(logfile);
	if(ret != 0)
	{
		fprintf(stderr, "init log file failed,err code %d\n", ret);
		return -3;
	}
	
	ret = apx_task_init();
	if (ret != 0)
		apx_wlog_quit("transfer module init failed", ret);
	
    return 0;
}

void apx_hftsc_exit()
{
	int ret;
	u32 ip;
	u16 port;

	ret = apx_task_exit();
	DBG("apx_task_exit return %d\n", ret);

	ret = apx_conf_serv_get(NULL, 0, &ip, &port);
	DBG("apx_conf_serv_get return %d\n", ret);

	ret = apx_user_logout(ip, port, current_uid_get());
	DBG("apx_user_logout return %d\n", ret);

	ret = apx_conf_release();
	DBG("apx_conf_release return %d\n", ret);

	apx_wlog_release();
}


