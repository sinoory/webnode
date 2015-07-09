/**
 *  Copyright APPEXNETWORKS, 2015, 
 *  <HFTS-CLIENT>
 *  <作者>
 *
 *  @defgroup <none>
 *  @ingroup  <hfts-client>
 *
 *  <文件描述>
 *
 *  @{
 */

#ifndef APX_HFTSC_API_H_
#define APX_HFTSC_API_H_

#ifndef APX_TYPE_H
#define APX_TYPE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <sys/wait.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <semaphore.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

#define BIT(nr)			(1UL << (nr))
#endif///ifndef APX_TYPE_H

#define VERSION	0.0.2

/* TRANSFER MANAGMENT MODULE  */

/**
*/
struct apx_trans_opt
{
	u8	type;
	u8	priv;
	u8	proto;
	u8  concurr;
	u32	down_splimit;
	u32	up_splimit;
	char ftp_user[32];
	char ftp_passwd[32];
	char fpath[128];
	char fname[128];
	char bt_select[128];
	char uri[512];
	u64 fsize;
	u8 bp_continue;
	char *cookie;
	char fileId[64];
};


/**
	@brief bt file struct
	btfile is used to return bt file structure..

	@param fn			bt file name
	@param file			file name in bt 
*/

struct btfile
{
	char fn[256];
	char file[40][256];
	int size;
};


/* TASK MANAGMENT MODULE API */

/* DEFINE TASK PRIVILEGE OPERATION */
#define APX_TASK_PRIV_UP	 0  //提升1级
#define APX_TASK_PRIV_DOWN	 1  //降低1级
#define APX_TASK_PRIV_TOP	 2  //  提升到最高级
#define APX_TASK_PRIV_BOTTOM  3  //降低到最低级
#define APX_TASK_PRIV_SET 	4	 //设置优先级（0-255）


/* DEFINE TASK STATE */
#define APX_TASK_STATE_UNDEFINED  0
#define APX_TASK_STATE_START  1

#define APX_TASK_STATE_ACTIVE   1
#define APX_TASK_STATE_WAIT   	2
#define APX_TASK_STATE_STOP     3
#define APX_TASK_STATE_TOBEDEL  4
#define APX_TASK_STATE_FINTOBEDEL	5
#define APX_TASK_STATE_FINISHED		6
#define APX_TASK_STATE_TERMINATED	7

#define APX_TASK_STATE_END   	8
#define APX_TASK_STATE_CREATE   APX_TASK_STATE_STOP


/* DEFINE TASK TYPE */
#define APX_TASK_TYPE_UNKNOWN  0
#define APX_TASK_TYPE_DOWN     1
#define APX_TASK_TYPE_SERVER_DOWN 2
#define APX_TASK_TYPE_SERVER_UP   3

/* DEFINE TASK PROTO */
#define APX_TASK_PROTO_UNKNOWN  0
#define APX_TASK_PROTO_START  1

#define APX_TASK_PROTO_FTP	1
#define APX_TASK_PROTO_HTTP  2
#define APX_TASK_PROTO_HTTPS  3
#define APX_TASK_PROTO_BT   4

#define APX_TASK_PROTO_END   5

int apx_task_restore(int uid);

/*	check uri, trans_opt should be filled with uri/ftp_user/ftp_passwd. 
	when function return, trans_opt->fname/fsize/bp_continue will be set.
	If uri bad, return < 0 										*/
int apx_task_uri_check(struct apx_trans_opt *trans_opt);

int apx_task_create(struct apx_trans_opt *trans_opt);
int apx_task_destroy(int taskid);
int apx_task_release(int taskid);
int apx_task_stop(int taskid);
int apx_task_start(int taskid);
int apx_task_delete(int taskid);
int apx_task_recover(int taskid);
int apx_task_reset(int taskid);
int apx_task_file_name_get(int taskid, char *fpath, int path_lenth, char* fname, int name_lenth);
int apx_task_priv_get(int taskid);
//int apx_task_priv_set(int taskid, u8 action, u8 task_priv);
int apx_task_limit_set(int taskid, u32 down_splimit, u32 up_splimit);
int apx_task_limit_get(int taskid, u32* down_splimit, u32* up_splimit);
int apx_task_ftp_account_set (int taskid, char* ftp_user, char* ftp_passwd);
//int apx_task_concur_set (int taskid, u32 concur_num);
int apx_task_concur_get (int taskid);
int apx_task_speed_get(int taskid, u32 *down_sp, u32 *up_sp);
int apx_task_time_get(int taskid, time_t *create_time, time_t *start_time, time_t *last_start_time, time_t *last_stop_time, time_t * during_time);
int apx_task_proto_get(int taskid);
int apx_task_type_get(int taskid);
int apx_task_bpcontinue_get(int taskid);
int apx_task_state_get(int taskid);
int apx_task_uid_get(int taskid);
int apx_task_uri_get(int taskid, char* uri, u32 uri_lenth);
int apx_task_file_size_get(int taskid, u64 *total_size, u64 *local_size, u64* up_size);
int apx_task_btfile_get(int taskid, struct btfile* bt_file);
int apx_task_btfile_selected(int taskid, char* bt_selected);


/* CONFIGURATE MANAGMENT MODULE API */
int apx_conf_init(char *conf_file);
int apx_conf_writeback(void);
int apx_conf_release(void);
int apx_conf_serv_set (char *name, u32 ip, u16 port);
int apx_conf_serv_get (char *name, int name_size, u32 *ip, u16 *port);
int apx_conf_active_task_limit_set(int task_num);
int apx_conf_active_task_limit_get(void);
int apx_conf_log_set(char *logfile);
int apx_conf_log_get(char *logfile, int logfile_size);
struct apx_userinfo_st * apx_conf_uinfo_get(void);
int apx_conf_uinfo_set(struct apx_userinfo_st * userinfo);
int apx_conf_nextid_get(void);
int apx_conf_nextid_inc(void);

/* USER MANAGMENT MODULE API */
int apx_user_login(u32 ip, u16 port, char *name, char *passwd);
int apx_user_logout(u32 ip, u16 port, u32 uid);
int apx_user_limit_set(u32 uid, u32 down_speed, u32 up_speed, u32 task_num);
int apx_user_limit_get(u32 uid, u32 *down_speed_limit, u32 *up_speed_limit, u32 *task_num_limit);
int apx_user_task_num_get(u32 uid, u16 *active_task_num, u16 *stop_task_num, u16 *finished_task_num);
int apx_user_file_path_set(u32 uid, char *path);
int apx_user_file_path_get(u32 uid, char *path, int path_lenth);
int apx_user_login_time_get(u32 uid, time_t *login_time);
int apx_user_register_time_get(u32 uid, time_t *register_time);
int apx_user_last_login_time_get(u32 uid, time_t *last_login_time);
int apx_user_task_traverse(u32 uid, u8 mode, void (*func)(void *data));

/* NETWORK DETECT*/
typedef enum _apx_network_e_
{
	APX_NET_OK = 0,/**网络正常*/
	APX_NET_INTER_ERR, /**接口不存在或者没有UP*/
	APX_NET_IP_UNSET, /**未配置IP*/
	APX_NET_ROUTE_UNSET, /** 未配置路由或网关*/
	APX_NET_ROUTE_UNREACH, /**路由或网关不通*/
	APX_NET_DNS_UNSET, /**DNS未配置*/
	APX_NET_DNS_UNREACH, /**DNS不通*/
	APX_NET_UNKOWN,
	
	APX_NET_MAX
}APX_NETWORK_E;

/**
	@brief	apx_net_start
		开始进行故障探测
 
	@param[in]	 ( void )	
	@return
		( void  )
	history
		      xyfeng     	  	2015-3-26        Initial Draft 
*/
void apx_net_start( void );

/**
	@brief	apx_net_end
		结束故障探测
 
	@param[in]	 ( void )	
	@return
		( void  )
	history
		      xyfeng     	  	2015-3-26        Initial Draft 
*/
void apx_net_end( void );

/**
	@brief	apx_net_detect_interface
		探测接口状态
		须在apx_net_start之后，apx_net_stop之前调用
 
	@param[in]	 ( void )	
	@return
		APX_NET_UNKOWN:	在apx_net_start之前调用
		APX_NET_INTER_ERR:	接口不存在或没有UP
		APX_NET_OK:			接口正常
	history
		      xyfeng     	  	2015-3-26        Initial Draft 
*/
APX_NETWORK_E apx_net_detect_interface( void );

/**
	@brief	apx_net_detect_ip
		探测IP是否配置
		须在apx_net_start之后，apx_net_stop之前调用
 
	@param[in]	 ( void )	
	@return
		APX_NET_UNKOWN:	在apx_net_start之前调用
		APX_NET_IP_UNSET:	IP未配置是
		APX_NET_OK:			IP已配置
	history
		      xyfeng     	  	2015-3-26        Initial Draft 
*/
APX_NETWORK_E apx_net_detect_ip( void );

/**
	@brief	apx_net_detect_route
		探测网关或路由配置
		须在apx_net_start之后，apx_net_stop之前调用
 
	@param[out]	pRt	不通的网关或路由IP
	@param[out]	pDst	网关或路由对应的目的地址( 如果存在的话 )
	@return
		APX_NET_UNKOWN:		在apx_net_start之前调用
		APX_NET_ROUTE_UNSET:	网关或路由未配置
		APX_NET_ROUTE_UNREACH:	网关或路由不通
		APX_NET_OK:				网关或路由正常
	history
		      xyfeng     	  	2015-3-26        Initial Draft 
*/
APX_NETWORK_E apx_net_detect_route( char *pRt, char *pDst );

/**
	@brief	apx_net_detect_dns
		探测DNS 配置
 
	@param[out]	pDns	不通的DNS 地址
	@return
		APX_NET_DNS_UNSET:	DNS未配置
		APX_NET_DNS_UNREACH:	DNS不通
		APX_NET_OK:				DNS正常
	history
		      xyfeng     	  	2015-3-26        Initial Draft 
*/
APX_NETWORK_E apx_net_detect_dns( char *pDns );

/**
	@brief	apx_network_set_ping_count
			设置ping的次数。
			默认为5次，至少ping通2次才认为是通的
 
	@param[in]	cnt	
	@return	void
	
	history
		      xyfeng     	  	2015-3-25        Initial Draft 
*/
void apx_network_set_ping_count( int cnt );

/* GLOBAL API */
int apx_hftsc_init(char* conf_file);
void apx_hftsc_exit();


#endif
/** @} */

