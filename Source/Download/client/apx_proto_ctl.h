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
#define	HTTP_OK		( 200 )
#define	HTTP_206		( 206 ) /** Partial Content */
#define	HTTP_403		( 403 ) /** Forbidden */
#define	HTTP_404		( 404 ) /** Not Found */
#define	HTTP_410		( 410 ) /** Gone */
#define	HTTP_412		( 412 ) /** Precondition Failed */

#define	APX_SIGN_LEN	( 41 )
#define	APX_ID_LEN		( 64 )
#define	BUF_LEN_MAX 	( 128 )
#define	URL_LEN_MAX 	( 512 )

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
	RESTFUL_TASK_CRT_FTP, /** 创建 FTP 下载任务 */
	RESTFUL_TASK_START, /** 开始下载 */
	RESTFUL_TASK_STOP, /** 暂停下载 */
	RESTFUL_TASK_DEL,	/** 删除下载任务 */
	RESTFUL_TASK_STATUS, /** 获取任务状态 */
	RESTFUL_TASK_LIST, /** 获取任务列表 */
	RESTFUL_TASK_PROXY, /** 中转下载*/
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
	
	char name[BUF_LEN_MAX];
	char passwd[BUF_LEN_MAX];
	char email[BUF_LEN_MAX];
	
	char createTm[BUF_LEN_MAX];
	char loginTm[BUF_LEN_MAX];
	char lastLoginTm[BUF_LEN_MAX];
	
	char remark[BUF_LEN_MAX];
}cld_userinfo_st;

/** 云端任务类型 */
typedef enum
{
	CLD_HTTP = 0,
	CLD_BT, /** 创建 */
	CLD_UPLOAD, /** 创建 */
	CLD_FTP, /** 状态*/
	
	CLDTASK_MAX
}cldtask_e;

/** 任务： 云端返回的任务信息 */
typedef struct _cloud_taskinfo_s_
{
	cldtask_e	type; /** 0: http, 1: bt 2: upload 3: ftp*/
	int status;/** 0 是初始化，还没下载 
				1，开始下载
				2，下载完成
				3，被人为停止
				4，被移除
				5	意外中止
				6，下载完成，正在进行文件处理
				*/
	int size;
	int download_size;
	int priority;
	int conns;
	int speed;	/** byte / s */
	int remain;	/** 剩余时间 */	
	u8 url[URL_LEN_MAX];
	u8 description[BUF_LEN_MAX];
	u8 fileId[APX_ID_LEN];
	u8 ctime[APX_SIGN_LEN]; /** 创建时间 */
	u8 name[BUF_LEN_MAX];
	u8 hash[APX_SIGN_LEN];
}cld_taskinfo_st;

/** 上传状态 */
typedef enum
{
	FILE_UP_NONE = 0, /** 未上传 */
	FILE_UP_DONE, /** 已上传 */
	FILE_UP_HALF, /** 部分上传 */

	FILE_UP_MAX
}fileup_e;

/** 任务信息类型 */
typedef enum
{
	TREQ_NONE = 0,
	TREQ_CRT, /** 创建 */
	TREQ_STOP, /** 创建 */
	TREQ_ST, /** 状态*/
	TREQ_LIST, /** 任务列表 */

	TREQ_MAX
}treqfrom_e;

/** 文件信息类型 */
typedef enum
{
	FREQ_NONE = 0,
	FREQ_PRE, /** 预取 */
	FREQ_UP, /** 上传 */
	FREQ_DOWN, /** 下载*/
	FREQ_ST, /** 状态*/
	FREQ_LIST, /** 任务列表 */

	FREQ_MAX
}freqfrom_e;

/** 文件上传预取： 云端返回的文件分段信息 */
typedef struct _cloud_fileinfo_s_
{
	int size;
	int cur_size;
	fileup_e eStatus;
	char name[BUF_LEN_MAX];
	char fileId[APX_ID_LEN];
	u8 sign[APX_SIGN_LEN];
	u8 ctime[APX_SIGN_LEN];/**  创建时间 */
	
	freqfrom_e eReqType;
	s32 blkCnt;
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

/** 云端任务列表 */
typedef struct  _cloud_list_s_
{
	u8	type; /** 0: http  1: bt 2: upload file 3: ftp*/
	union {
		cld_taskinfo_st taskinfo;
		cld_fileinfo_st fileinfo;
	}u;
}cld_list_st;

/** 云端任务列表 */
typedef struct  _cloud_tasklist_s_
{
	u16 total; /** 总的任务个数 */
	u16 cnt; /** plist数组个数 */
	cld_list_st* plist;
}cld_tasklist_st;

/*------------------------------------------------------------*/
/*                          Exported Functions                                  */
/*------------------------------------------------------------*/
char* apx_cloud_get_url( void);
void apx_cloud_set_url( char* ps8RootUrl );
int apx_cloud_login( char *user, char *passwd, cld_userinfo_st *userinfo );
int apx_cloud_logout();
int apx_cloud_task_create( struct apx_trans_opt *trans_opt, cld_taskinfo_st *taskinfo );
int apx_cloud_task_start( char *taskId );
int apx_cloud_task_stop( char *taskId, cld_list_st *info );
int apx_cloud_task_del( char *taskId );
int apx_cloud_task_status( char *taskId, cld_list_st *info );
int apx_cloud_task_list( int start,  int limit, cld_tasklist_st *ptask_list );
void apx_cloud_task_freelist( cld_tasklist_st *ptask_list );
int apx_cloud_upload_proload( s8* fname, u32 blkcnt, cld_fileinfo_st *fileinfo  );
int apx_cloud_fileinfo( cld_fileinfo_st *fileinfo  );
int apx_cloud_fault_check( cld_fault_st *faultinfo );
s8* apx_build_proxy_json( s8 *url, s8 *headers[], size_t size );
int apx_get_proxy_url( s8** ppUrl );
int apx_cloud_sign( s8 *pUrl, size_t sLen );

#ifdef __cplusplus
 }       
#endif /*end __cplusplus */
         
#endif /*end _APX_PRORO_CTL_H_ */       
         
        

/** @} */
 
