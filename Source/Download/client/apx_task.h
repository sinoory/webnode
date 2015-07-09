/**
 *  Copyright APPEX, 2015, 
 *  <HFTS-CLIENT>
 *  <作者>
 *
 *  @defgroup <configure>
 *  @ingroup  <hfts-client>
 *
 *  <文件描述>
 *
 *  @{
 */
#ifndef APX_TASK_H_20150210
#define APX_TASK_H_20150210

#include "../include/apx_list.h"
#include "../include/apx_type.h"
#include "../include/apx_hftsc_api.h"
#include "apx_file.h"
#include "apx_config.h"
#include "apx_user.h"
#include "apx_transfer_a2.h"
#include "apx_transfer_cl.h"

#define TASK_MAX_CONCUR	100

struct apx_task_st
{
	struct list_head list;
	struct apx_userinfo_st *uinfo;

	/* transfer info */
	int trans_handle;
	struct apx_trans_opt trans_opt;
	struct apx_trans_stat trans_stat;
	
	u64 taskid;
	u64 serv_taskid;
	
	time_t	create_time;
	time_t	start_time;	
	time_t	last_start_time;
	time_t	end_time;
	time_t	during_time;
	
	struct timeval	tick_time;	
	u32		tick_down_bytes, tick_up_bytes;
	
	struct trans_operations *trans_opration;
	
	pthread_mutex_t	nref_lock;
	u32		nref;
};

#endif
/** @} */
