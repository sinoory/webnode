/**
*	Copyright (c)  AppexNetworks, All Rights Reserved.
*	High speed Files Transport
*	Author:	xyfeng
*
*	@defgroup	协议控制模块
*	@ingroup		
*
*	为云端通信提供协议控制接口
*
*	@{
*Function List: 
*/
 
#ifndef _APX_PRORO_CTL_H_
#define _APX_PRORO_CTL_H_
         
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
        
/*---Include Local.h File---*/
#include "../include/apx_hftsc_api.h"

#ifdef __cplusplus
extern "C" {
#endif /*end __cplusplus */
        
/*------------------------------------------------------------*/
/*                          Macros Defines                                      */
/*------------------------------------------------------------*/
#define	HTTP_OK	( 200 )
#define	ERR_LEN_MAX 	( 256 )
        
/*------------------------------------------------------------*/
/*                    Exported Variables                                        */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                         Data Struct Define                                      */
/*------------------------------------------------------------*/
/** restful api type */
typedef enum _restful_e_
{
	RESTFUL_LOGIN = 0, /** 登陆 */
	RESTFUL_LOGOUT, /** 注销 */
	
	RESTFUL_TASK_CRT_HTTP, /** 创建http下载任务 */
	RESTFUL_TASK_CRT_BT, /** 创建bt下载任务 */
	RESTFUL_TASK_START, /** 开始下载 */
	RESTFUL_TASK_STOP, /** 暂停下载 */
	RESTFUL_TASK_DEL,	/** 删除下载任务 */
	RESTFUL_TASK_STATUS, /** 获取任务状态 */
	RESTFUL_TASK_LIST, /** 获取任务列表 */
	RESTFUL_UPLOAD_PRELOAD, /** 文件上传预取 */
	RESTFUL_FILEINFO, /** 文件状态*/
	RESTFUL_FAULT_CHK, /** 云端状态检测 */
	
	RESTFUL_MAX
}restful_e;

/** 登陆: 云端返回的用户信息 */
typedef struct _cloud_userinfo_s_
{
	u8	admin;
	u16	status;
	u16	quota;
	
	u32	usrId;
	u32	groupId;
	
	u16	upLimit;
	u16	downLimit;
	u16	activeTaskLimit;
	
	char name[ERR_LEN_MAX];
	char passwd[ERR_LEN_MAX];
	char email[ERR_LEN_MAX];
	
	char createTm[ERR_LEN_MAX];
	char loginTm[ERR_LEN_MAX];
	char lastLoginTm[ERR_LEN_MAX];
	
	char remark[ERR_LEN_MAX];
}cld_userinfo_st;

typedef struct _cloud_btfile_s_
{
	int size;
	int download_size;
	char name[ERR_LEN_MAX];
	char hash[48];
}cld_btfile_st;

/** 任务： 云端返回的任务信息 */
typedef struct _cloud_taskinfo_s_
{
	int priority;
	int conns;
	char url[ERR_LEN_MAX*4];
	char description[ERR_LEN_MAX*2];
	int next_idx;
	cld_btfile_st btfile[0];
}cld_taskinfo_st;

typedef struct _cloud_task_s_
{
	int id;
	int status;
	cld_taskinfo_st *info;
}cld_task_st;

/** 上传状态 */
typedef enum
{
	FILE_UP_NONE = 0, /** 未上传 */
	FILE_UP_DONE, /** 已上传 */
	FILE_UP_HALF, /** 部分上传 */

	FILE_UP_MAX
}fileup_e;

/** 文件上传预取： 云端返回的文件分段信息 */
typedef struct _cloud_fileinfo_s_
{
	fileup_e eStatus;
	u32	preload; /** 1: preload, other: fileinfo */
	s32 blkCnt;
	u8 fileId[64];
	u8 sign[41];
	void *pblk;
}cld_fileinfo_st;

/** 云端状态检测 */
typedef struct _cloud_fault_s_
{
	u32 serverCon:2, /** 存储服务器 */
		storageSpace:2, /** 存储空间 */
		dbCon:2, /** 数据库服务器 */
		dbStorage:2, /** 数据库存储空间 */
		dnsResolve:2; /** DNS解析 */
}cld_fault_st;

typedef struct _cloud_taskdetail_s_
{
	int id;
	int status;
	int priority;
	int conns;
	char url[ERR_LEN_MAX*2];
	char description[ERR_LEN_MAX];
	int next_idx;
	cld_btfile_st btfile[0];
}cld_taskdetail_st;

typedef struct _cloud_filedetail_s_
{
	u64 size;
	u64 cur_size;
	fileup_e eStatus;
	char name[ERR_LEN_MAX];
	char fileId[64];
}cld_filedetail_st;

/** 云端任务列表 */
typedef struct  _cloud_list_s_
{
	u8	type; /** 0: http  1: bt 2: upload file*/
	union {
		cld_taskdetail_st taskinfo;
		cld_filedetail_st fileinfo;
	}u;
}cld_list_st;

/** 云端任务列表 */
typedef struct  _cloud_tasklist_s_
{
	u16 total;
	u16 cnt;
	cld_list_st* plist;
}cld_tasklist_st;

/*------------------------------------------------------------*/
/*                          Exported Functions                                  */
/*------------------------------------------------------------*/
char* apx_cloud_get_url( void);
void apx_cloud_set_url( char* ps8RootUrl );
int apx_cloud_login( char *user, char *passwd, int *http_code, cld_userinfo_st *userinfo );
int apx_cloud_logout();
int apx_cloud_task_create( struct apx_trans_opt *trans_opt, int *task_status );
int apx_cloud_task_start( int task_id, int *http_code, int *task_status );
int apx_cloud_task_stop( int task_id, int *http_code, int *task_status );
int apx_cloud_task_del( int task_id, int *http_code, int *task_status );
int apx_cloud_task_status( int task_id, int *http_code, cld_task_st *taskinfo  );
int apx_cloud_task_list( int *http_code, int start,  int limit, cld_tasklist_st *ptask_list );
void apx_cloud_task_freelist( cld_tasklist_st *ptask_list );
int apx_cloud_upload_proload( s8* fname, u32 blkcnt,  int *http_code, cld_fileinfo_st *fileinfo  );
int apx_cloud_fileinfo( int *http_code, cld_fileinfo_st *fileinfo  );
int apx_cloud_fault_check( int *http_code, cld_fault_st *faultinfo );

#ifdef __cplusplus
 }       
#endif /*end __cplusplus */
         
#endif /*end _APX_PRORO_CTL_H_ */       
         
        

/** @} */
 
