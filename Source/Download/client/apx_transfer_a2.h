/**
 *  Copyright APPEX, 2015, 
 *  <HFTS-CLIENT>
 *  <liuyang>
 *
 *  @defgroup <configure>
 *  @ingroup  <hfts-client>
 *
 *  <>
 *
 *  @{
 */
#ifndef APX_TRANSFER_H_20150210
#define APX_TRANSFER_H_20150210
#ifndef _GUN_SOURCE
#define _GUN_SOURCE
#endif
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "../include/apx_type.h"
#include "../include/apx_hftsc_api.h"
#ifdef __cplusplus
extern "C"{
#endif

/**
	@brief Global stat struct
	apx_trans_glbstat is used to return global stat.

	@param downspeed	speed of download
	@param uploadspeed	speed of upload
	@param activenum	the number of active download
	@param waitingnum	the number of waiting to download
	@param stoppednum	the number of stopped download
*/
struct apx_trans_globalstat
{
	int		downspeed;
	int		uploadspeed;
	int		activenum;
	int		waitingnum;
	int		stoppednum;
};


/**
	@brief Task stat struct
	taskstat is used to return task stat.

	@param dir					the path to save download file
	@param totallength			totall length of download file

	@param down_completedlength	completed length of download
	@param up_completedlength	completed length of upload
	@param downspeed			speed of download
	@param uploadspeed			speed of upload
	@param connections			connect number
	@param status				task status num
	@param errorcode			error code bu task return
*/
struct apx_trans_stat
{
	u64		total_size;
	u64		down_size;
	u64		up_size;
	u32		down_speed;
	u32		up_speed;
	u16		connections;
	u8		state;
	u8		state_event;
	int		trans_errno;
};



/**
	@brief Global options struct
	glbstat is used to set global options.

	@param dir						the path to save download files
	@param connections				the maximum number of connections
	@param max_limit_downspeed		the maximum limited downloadspeed
	@param max_limit_uploadspeed	the maximum limited uploadspeed
	@param max_concurrent_download	the maximum number of concurrent download task
*/
struct apx_trans_glboptions
{
	char 	path[256];
	int		connections;
	int		max_limit_downspeed;
	int		max_limit_uploadspeed;
	int		max_concurrent_download;
};


/**
	@brief handle struct
	handle is used to point aria2 session and thread..

	@param session			mark aria2 session
	@param thread			mark start session thread
	@param flag				thread start or stop
	@param gid 				task id
	@param status			task status
*/
struct handle
{
	u32 *session;
	pthread_t thread;
	int flag;
	uint64_t gid;
	int status;
	char fname[256];
};

int apx_trans_init_a2( u32 nu );
int apx_trans_create_a2( void );
int apx_trans_recv_a2( u32 nu );
int apx_trans_getopt_a2( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );
int apx_trans_getstat_a2( u32 nu, struct apx_trans_stat* task_stat );
int apx_trans_start_a2( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );
int apx_trans_stop_a2( u32 nu) ;
int apx_trans_del_file_a2( struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );
int apx_trans_delete_a2( u32 nu );
int apx_trans_precreate_a2( struct apx_trans_opt* task_opt );
int apx_trans_get_btfile_a2( u32 nu, struct apx_trans_opt* task_opt, struct btfile* bt_file );
int apx_trans_release_a2( u32 nu, int flags );
void apx_trans_exit_a2( void );
#ifdef __cplusplus
};
#endif

#endif
/** @} */
