/**
*	Copyright (c)  AppexNetworks, All Rights Reserved.
*	High speed Files Transport
*	Author:	xyfeng
*
*	@defgroup	libcurl封装
*	@ingroup		
*
*	为任务管理模块提供上传下载接口
*
*	@{
*/
 
#ifndef _APX_TRANSFER_CL_H_
#define _APX_TRANSFER_CL_H_
         
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif        
/*---Include Local.h File---*/
#include "../include/apx_list.h"
#include "../include/apx_type.h"

#ifdef __cplusplus
extern "C" {
#endif /*end __cplusplus */
        
/*------------------------------------------------------------*/
/*                          Macros Defines                                      */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                    Exported Variables                                        */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                         Data Struct Define                                      */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                          Exported Functions                                  */
/*------------------------------------------------------------*/
/**
	@brief	apx_trans_init
		模块初始化
 
	@param[in]	nu	 nu	最大任务个数	
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_init_cl( u32 nu );

/**
	@brief	apx_trans_exit
		模块去初始化
 
	@param[in]	 ( void )	
	@return
		( void )
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
void apx_trans_exit_cl( void );


/**
	@brief	apx_trans_create
		创建一个下载/上传任务
 
	@param[in]	 ( void )	
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_create_cl( void );

/**
	@brief	apx_trans_release
		释放任务
 
	@param[in]	nu		任务索引
	@param[in]	flags		no used
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_release_cl( u32 nu, int flags );


/**
	@brief	apx_trans_start
		开始下载
 
	@param[in]	nu		任务索引
	@param[in]	glb_opt	全局配置
	@param[in]	task_opt	任务配置
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_start_cl( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );

/**
	@brief	apx_trans_stop
		结束下载
 
	@param[in]	nu	任务索引
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_stop_cl( u32 nu);

/**
	@brief	apx_trans_delete_cl
		删除下载
 
	@param[in]	nu	任务索引
	@return
		( void )
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_delete_cl( u32 nu );

/**
	@brief	apx_trans_recv
	
 
	@param[in]	nu	
	@return
		int :
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_recv_cl( u32 nu );

//int apx_trans_setopt(u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt);
/**
	@brief	apx_trans_getopt
		获取配置
 
	@param[in]	nu		任务索引
	@param[in]	glb_opt	全局配置
	@param[in]	task_opt	任务配置
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_getopt_cl( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );

/**
	@brief	apx_trans_getstat
		获取统计信息
 
	@param[in]	nu		任务索引
	@param[out]	task_stat	统计信息
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_getstat_cl( u32 nu, struct apx_trans_stat* task_stat );


/**
	@brief	apx_trans_get_btfile_cl
	
 
	@param[in]	nu	
	@param[in]	task_opt	
	@param[in]	bt_file	
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_get_btfile_cl( u32 nu, struct apx_trans_opt* task_opt, struct btfile* bt_file );

/**
	@brief	apx_trans_precreate_cl
		通过url 获取文件信息:
			文件名
			文件大小
			是否支持多线程下载
 
	@param[in]	task_opt		传入url、proto, 传出fname , fsize bp_continue
	@return
		return 0 if success, else return -1
	history
		      xyfeng     	  	2015-4-14        Initial Draft 
*/
int apx_trans_precreate_cl( struct apx_trans_opt* task_opt );

int apx_trans_del_file_cl( struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt );
         
#ifdef __cplusplus
 }       
#endif /*end __cplusplus */
         
#endif /*end _APX_TRANSFER_CL_H_ */       
         
        

/** @} */
 
