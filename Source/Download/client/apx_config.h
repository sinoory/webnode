/**
 *  Copyright APPEX, 2015, 
 *  <HFTS-CLIENT>
 *  <Jerry.hua>
 *
 *  @defgroup <configure>
 *  @ingroup  <hfts-client>
 *
 *  <Handle the configuration>
 *
 *  @{
 */
#ifndef APX_CONFIG_H_20150210
#define APX_CONFIG_H_20150210

#define DEFAULT_CONFIG	"~/.config/cdosbrowser/appex_config/conf_file"
#define DEFAULT_LOGFILE "~/.config/cdosbrowser/appex_config/hfts.log"
#define DEFAULT_SERV_NAME	"www.serv.com"	
#define DEFAULT_SERV_IP		0	
#define DEFAULT_SERV_PORT	10000	
#define DEFAULT_ACT_TASK	5
#define DEFAULT_NEXT_TASKID	1	
//#define TASK_INFO	"/etc/config/taskinfo" 

#include "../include/apx_list.h"
#include "../include/apx_type.h"
#include "../include/apx_hftsc_api.h"
#include "../include/uci.h"
#include "apx_task.h"
#include "apx_user.h"

struct apx_task_st;

struct apx_config_st
{
	char serv_name[128];
	u32 serv_ip;
	u16 serv_port;
	u16 active_task_limit;
	u64 next_taskid;
	char config_file[128];
	char log_file[128];
	struct apx_userinfo_st *userinfo;
	u8 conf_init_flag;
};

#endif
/** @} */
