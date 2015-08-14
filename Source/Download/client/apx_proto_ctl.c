/*-----------------------------------------------------------
*      Copyright (c)  AppexNetworks, All Rights Reserved.
*
*FileName:     apx_proro_ctl.c 
*
*Description:  云端通信协议控制模块
* 
*History: 
*      Author              Date        	Modification 
*  ----------      ----------  	----------
* 	xyfeng   		2015-4-21     	Initial Draft 
* 
*------------------------------------------------------------*/
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
#include "apx_proto_ctl.h"  
#include "apx_file.h"
#include "./wjson/proto_base.h"   
#ifndef SIGN_DISABLE
#include "apx_user.h"
extern int current_uid_get();
extern struct apx_userinfo_st *uid2uinfo(u32 uid);
#endif

/*------------------------------------------------------------*/
/*                          Private Macros Defines                          */
/*------------------------------------------------------------*/
#define CLD_DEBUG

#undef clog
#ifdef CLD_DEBUG

#define    clog( fmt, args... )     \
        do { \
            fprintf( stderr, "[%03d-%s] "fmt"\n", __LINE__, __FUNCTION__, ##args ); \
        }while(0)
        
#else
#define    clog( fmt, args... )
#endif

#define    ctlog( fmt, args... )     \
        do { \
            fprintf( stderr, fmt"\n", ##args ); \
        }while(0)
        
#define CLD_FTP_PREFIX		"ftp://"

/*------------------------------------------------------------*/
/*                          Private Type Defines                            */
/*------------------------------------------------------------*/
/** http message type */
typedef enum _http_method_e_
{
	HTTP_GET = 0,
	HTTP_POST,
	HTTP_PUT,
	HTTP_DEL,
	HTTP_HEAD,

	HTTP_METHOD_MAX
}http_method_e;

/** 云端返回的错误消息 */
typedef struct _cloud_err_s_
{
	char msg[BUF_LEN_MAX];
	char errors[BUF_LEN_MAX];
}cld_err_st;

/** 云端响应的内容 */
typedef struct  _cloud_result_s_
{
	u16	status;
	u16 cnt;
	u16 total;
	cldtask_e type;
	union {
		void *ptr;
		cld_userinfo_st userinfo;
		cld_taskinfo_st taskinfo;
		cld_fileinfo_st fileinfo;
		cld_fault_st faultinfo;
		cld_err_st err;
	}u;
}cld_result_st;
        
/** 请求的内容 */
typedef struct _cloud_http_s_
{
	u8 conns;
	u8 priority;
	u8 url[URL_LEN_MAX];
	u8 remark[BUF_LEN_MAX];
}cld_http_st;

	
/** joint url */
typedef s32 ( *pFunJointUrl )( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 );


/** request content */
typedef s8* ( *pFunContent )( void *param1, void *param2 );

/** parse response */
typedef s32 ( *pFunParseResp )( s8* content, s32 len, cld_result_st *result );

/** restful api  */
typedef struct _restful_api_s_
{
	s8 *url;
	http_method_e method;
	pFuncHttp	request; /** http请求函数 */
	pFunJointUrl	joint_url; /** url 拼接函数 */
	pFunParseResp parse_resp; /** 解析响应函数 */
	pFunContent gene_content; /** 创建请求内容函数 */
}restful_api_st;

/*------------------------------------------------------------*/
/*                         Global Variables                                */
/*------------------------------------------------------------*/
/*---Extern Variables---*/
        
/*---Local Variables---*/
static s32 __url_login( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 );
static s32 __url_taskstatus( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 );
static s32 __url_tasklist( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 );
static s32 __url_fileinfo( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 );
static s32 __url_common( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 );
static s32 __parse_userinfo( s8* content, s32 len, cld_result_st *result );
static s32 __parse_taskcreate( s8* content, s32 len, cld_result_st *result );
static s32 __parse_taskstart( s8* content, s32 len, cld_result_st *result );
static s32 __parse_taskinfo( s8* content, s32 len, cld_result_st *result );
static s32 __parse_tasklist( s8* content, s32 len, cld_result_st *result );
static s32 __parse_fileinfo( s8* content, s32 len, cld_result_st *result );
static s32 __parse_faultinfo( s8* content, s32 len, cld_result_st *result );
static s8 * __generate_content_login( void *param1, void *param2 );
static s8* __generate_content_create_http( void *param1, void *param2 );
static s8* __generate_content_preload( void *param1, void *param2 );
static s8* __generate_content_common( void *param1, void *param2 );
static int __parse_filedetail(  wjson wj, cld_fileinfo_st* pFileInfo, freqfrom_e eReqFrom );
static int __parse_taskdetail(  wjson wj, cld_taskinfo_st* pTaskInfo, treqfrom_e eReqFrom );


static u8* g_pPrivateKey= "4389fdsjdsafsaFDSfds778fdsalfds789fds";
static s8 *g_pCloudRootUrl;
static restful_api_st g_restful_url[RESTFUL_MAX] = {
	/** login */
	[RESTFUL_LOGIN] = {
						.url = ( s8* )"/auth",
						.method = HTTP_PUT, 
						.request = RAPI_Put,
						.joint_url = __url_login,
						.parse_resp = __parse_userinfo,
						.gene_content = __generate_content_login
					   },
	/** logout */				   
	[RESTFUL_LOGOUT] = {
						.url = ( s8* )"/unauth",
						.method = HTTP_PUT,
						.request = RAPI_Put,
						.joint_url = __url_common,
						.parse_resp = NULL,
						.gene_content = NULL
					   },
	/** create http  */				   
	[RESTFUL_TASK_CRT_HTTP] = {
						.url = ( s8* )"/task/http",
						.method = HTTP_POST, 
						.request = RAPI_Post,
						.joint_url = __url_common,
						.parse_resp = __parse_taskcreate,
						.gene_content = __generate_content_create_http
					   },

	/** create ftp  */ 			   
	[RESTFUL_TASK_CRT_FTP] = {
						.url = ( s8* )"/task/ftp",
						.method = HTTP_POST, 
						.request = RAPI_Post,
						.joint_url = __url_common,
						.parse_resp = __parse_taskcreate,
						.gene_content = __generate_content_create_http
					   },
	/** start task */				   
	[RESTFUL_TASK_START] = {
						.url = ( s8* )"/task/start",
						.method = HTTP_PUT, 
						.request = RAPI_Put,
						.joint_url = __url_common,
						.parse_resp = __parse_taskstart,
						.gene_content = __generate_content_common
					   },
	/** stop task */				   
	[RESTFUL_TASK_STOP] = {
						.url = ( s8* )"/task/stop",
						.method = HTTP_PUT, 
						.request = RAPI_Put,
						.joint_url = __url_common,
						.parse_resp = __parse_taskinfo,
						.gene_content = __generate_content_common
					   },
	/** delete task */				   
	[RESTFUL_TASK_DEL] = {
						.url = ( s8* )"/task",
						.method = HTTP_DEL, 
						.request = RAPI_Delete,
						.joint_url = __url_common,
						.parse_resp = __parse_taskstart,
						.gene_content = __generate_content_common
					   },
	/** task status */				   
	[RESTFUL_TASK_STATUS] = {
						.url = ( s8* )"/task",
						.method = HTTP_GET, 
						.request = RAPI_Get,
						.joint_url = __url_taskstatus,
						.parse_resp = __parse_taskinfo,
						.gene_content = NULL
					   },
	/** task list */				   
	[RESTFUL_TASK_LIST] = {
						.url = ( s8* )"/task",
						.method = HTTP_GET, 
						.request = RAPI_Get,
						.joint_url = __url_tasklist,
						.parse_resp = __parse_tasklist,
						.gene_content = NULL
					   },
	/** task proxy */				   
	[RESTFUL_TASK_PROXY] = {
						.url = ( s8* )"/proxy",
						.method = HTTP_POST, 
						.request = RAPI_Post,
						.joint_url = __url_common,
						.parse_resp = NULL,
						.gene_content = NULL
					   },
	/** upload preload */
	[RESTFUL_UPLOAD_PRELOAD] = {
						.url = ( s8* )"/upload/preload",
						.method = HTTP_POST, 
						.request = RAPI_Post,
						.joint_url = __url_common,
						.parse_resp = __parse_fileinfo,
						.gene_content = __generate_content_preload
					   },
	/** upload fileinfo */
	[RESTFUL_FILEINFO] = {
						.url = ( s8* )"/fileinfo",
						.method = HTTP_GET, 
						.request = RAPI_Get,
						.joint_url = __url_fileinfo,
						.parse_resp = __parse_fileinfo,
						.gene_content = NULL
					   },
	/** cloud status check */
	[RESTFUL_FAULT_CHK] = {
						.url = ( s8* )"/status",
						.method = HTTP_GET, 
						.request = RAPI_Get,
						.joint_url = __url_common,
						.parse_resp = __parse_faultinfo,
						.gene_content = NULL
					   },
};    

/*------------------------------------------------------------*/
/*                          Local Function Prototypes                       */
/*------------------------------------------------------------*/
static int __iterator_blocks( wjson wj, void* arg1, void* arg2 );
        
/*------------------------------------------------------------*/
/*                        Functions                                               */
/*------------------------------------------------------------*/

/*-------------------------------------------------
*Function:    __calc_passwd_sha1sum
*Description:   
*	计算密码校验值
*	计算公式: sha1sum( user + sha1sum( passwd ) )
*Parameters: 
*	user[IN]			用户名
*	passwd[IN]		密码
*	pu8Sha1[OUT]	sha1校验值
*	psOutLen[IN/OUT]		校验值长度
*Return:
*      void
*History:
*      xyfeng     	  2015-7-8        Initial Draft 
*---------------------------------------------------*/
static void __calc_passwd_sha1sum( s8 *user, s8 *passwd, u8 *pu8Sha1, size_t *psOutLen )
{
	u8 buf[RAPI_STR_LEN_MAX];
	size_t tmp= *psOutLen;
	
	SHA1_buffer( ( u8* )passwd, strlen( ( char* )passwd ), pu8Sha1, &tmp );
	snprintf( ( char* )buf,  sizeof( buf ), "%s%s", user, pu8Sha1 );
	SHA1_buffer( buf, strlen( ( char* )buf ), pu8Sha1, psOutLen );
	return;
}

/** md5sum( user + md5sum( passwd ) ) */
static void __calc_passwd_md5sum( s8 *user, s8 *passwd, u8 *pu8Md5, size_t *psOutLen )
{
	u8 buf[RAPI_STR_LEN_MAX];
	size_t tmp= *psOutLen;
	
	MD5_buffer( ( u8* )passwd, strlen( ( char* )passwd ), pu8Md5, &tmp );
	snprintf( ( char* )buf,  sizeof( buf ), "%s%s", user, pu8Md5 );
	MD5_buffer( buf, strlen( ( char* )buf ), pu8Md5, psOutLen );
	return;
}

/*-------------------------------------------------
*Function:    __sign_result
*Description:   
*           计算签名值
*		hmacsha1	->	_=timestamp & passwd=xxx
*Parameters: 
*	user[IN]			用户名
*	passwd[IN]		密码
*	timestamp[IN]	时间戳
*	sign[OUT]		签名
*	pOutLen[IN/OUT]	签名长度
*Return:
*       void
*History:
*      xyfeng     	  2015-7-8        Initial Draft 
*---------------------------------------------------*/
static void __sign_result( s8 *user, s8 *passwd, u8 *timestamp, u8 *sign, size_t *pOutLen )
{
	size_t len;
	u8 md5sum[APX_SIGN_LEN] = { 0 };
	u8 buf[URL_LEN_MAX] = {0};
		
	len = sizeof( md5sum );
	__calc_passwd_sha1sum( ( s8* )user, ( s8* )passwd, md5sum, &len );

	snprintf( buf, sizeof( buf ), "_=%s&passwd=%s", timestamp, md5sum );
	HmacSha1( buf, strlen( buf ), g_pPrivateKey, strlen( g_pPrivateKey ), sign, pOutLen );

	//clog( "\t%s\t->\t%s, u: [%s], p: [%s], m: [%s], t: [%s]\n", sign, buf, user,passwd, md5sum, timestamp );
	return;
}

/*-------------------------------------------------
*Function:    __url_login
*Description:   
*           登陆: 构造 URL
*		/login?_=${timestamp}
*Parameters: 
*	type[IN]		请求类型
*	pUrl[OUT]	url
*	sLen[IN]		url 大小
*	param1[IN]	null
*	param2[IN]	null
*Return:
*        return 0
*History:
*      xyfeng     	  2015-7-8        Initial Draft 
*---------------------------------------------------*/
static s32 __url_login( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 )
{
	snprintf(  ( char* )pUrl, sLen, "%s%s?_=%s",
			g_pCloudRootUrl, g_restful_url[type].url,
			get_timestamp() );
	return 0;
}

/*-------------------------------------------------
*Function:    __url_taskstatus
*Description:   
*           任务状态: 构造 URL
*		/task/${taskId}?_=${timestamp}&sign=admin:${sign}
*Parameters: 
*	type[IN]		请求类型
*	pUrl[OUT]	url
*	sLen[IN]		url 大小
*	param1[IN]	任务ID
*	param2[IN]	null
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-7-8        Initial Draft 
*---------------------------------------------------*/
static s32 __url_taskstatus( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 )
{
	char *taskId =  ( char* )param1;
	char *pTimestamp = get_timestamp();
#ifndef SIGN_DISABLE
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );
	struct apx_userinfo_st *pUserInfo = uid2uinfo( ( u32 )current_uid_get() );

	if( NULL == pUserInfo )
	{
		clog( "user not login." );
		return -1;
	}
	
	__sign_result( pUserInfo->name, pUserInfo->passwd, pTimestamp, u8Sign, &sSignLen );
	snprintf(  ( char* )pUrl, sLen,
			"%s%s/%s?_=%s&sign=admin:%s",
			g_pCloudRootUrl, g_restful_url[type].url, 
			taskId,
			pTimestamp, u8Sign );
#else
#ifdef SIGN_DEMO
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );

	__sign_result( "admin", "admin", pTimestamp, u8Sign, &sSignLen );
	snprintf(  ( char* )pUrl, sLen,
			"%s%s/%s?_=%s&sign=admin:%s",
			g_pCloudRootUrl, g_restful_url[type].url, 
			taskId,
			pTimestamp, u8Sign );
#else
	snprintf(  ( char* )pUrl, sLen,
			"%s%s/%s?_=%s",
			g_pCloudRootUrl, g_restful_url[type].url, 
			taskId,
			pTimestamp );
#endif
#endif
	return 0;
}

/*-------------------------------------------------
*Function:    __url_tasklist
*Description:   
*           任务列表: 构造 URL
*		/task?_=${timestamp}&limit=${limit}&start=${start}&sign=admin:${sign}
*Parameters: 
*	type[IN]		请求类型
*	pUrl[OUT]	url
*	sLen[IN]		url 大小
*	param1[IN]	开始项
*	param2[IN]	请求数
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-7-8        Initial Draft 
*---------------------------------------------------*/
static s32 __url_tasklist( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 )
{
	int start = 0;
	int limit = 0;
		
	u8 buf[URL_LEN_MAX] = {0};
	size_t buf_len;
	size_t url_len;
	
	char *pTimestamp = get_timestamp();
#ifndef SIGN_DISABLE
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );
	struct apx_userinfo_st *pUserInfo = uid2uinfo( ( u32 )current_uid_get() );

	if( NULL == pUserInfo )
	{
		clog( "user not login." );
		return -1;
	}
#endif

	if( param1 !=NULL )
	{
		start = *( int* )param1;
	}
	if( param2 !=NULL )
	{
		limit = *( int* )param2;
	}
	
	url_len = snprintf(  ( char* )pUrl, sLen,
			"%s%s%s?_=%s", 
			g_pCloudRootUrl, g_restful_url[type].url, 
			( char* )param1, pTimestamp );
	
	buf_len = snprintf( buf, sizeof( buf ), "_=%s", pTimestamp );
	if( limit > 0 )
	{
		url_len += snprintf(  ( char* )pUrl + url_len, sLen - url_len ,"&limit=%d", limit );
#ifndef SIGN_DISABLE
		buf_len += snprintf( buf + buf_len, sizeof( buf ) - buf_len, "&limit=%d", limit );
#endif
	}
	
	if( start > 0 )
	{
		url_len += snprintf(  ( char* )pUrl + url_len, sLen - url_len ,"&start=%d", start );
#ifndef SIGN_DISABLE
		buf_len += snprintf( buf + buf_len, sizeof( buf ) - buf_len, "&start=%d", start );
#endif
	}

#ifndef SIGN_DISABLE
	__calc_passwd_sha1sum( pUserInfo->name, pUserInfo->passwd, u8Sign, &sSignLen );
	snprintf(  buf + buf_len, sizeof( buf ) - buf_len, "&passwd=%s", u8Sign );

	sSignLen = sizeof( u8Sign );
	memset( u8Sign, 0, sSignLen );
	HmacSha1( buf, strlen( buf ), g_pPrivateKey, strlen( g_pPrivateKey ), u8Sign, &sSignLen );

	snprintf(  ( char* )pUrl + url_len, sLen - url_len ,"&sign=admin:%s", u8Sign );
#else
#ifdef SIGN_DEMO
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );
	
	__calc_passwd_sha1sum( "admin", "admin", u8Sign, &sSignLen );
	snprintf(  buf + buf_len, sizeof( buf ) - buf_len, "&passwd=%s", u8Sign );
	
	sSignLen = sizeof( u8Sign );
	memset( u8Sign, 0, sSignLen );
	HmacSha1( buf, strlen( buf ), g_pPrivateKey, strlen( g_pPrivateKey ), u8Sign, &sSignLen );
	
	snprintf(  ( char* )pUrl + url_len, sLen - url_len ,"&sign=admin:%s", u8Sign );
#endif
#endif	
	return 0;
}

/*-------------------------------------------------
*Function:    __url_fileinfo
*Description:   
*           文件信息: 构造 URL
*		/fileinfo/${fileId}?_=${timestamp}&sign=admin:${sign}
*Parameters: 
*	type[IN]		请求类型
*	pUrl[OUT]	url
*	sLen[IN]		url 大小
*	param1[IN]	null
*	param2[IN]	null
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-7-8        Initial Draft 
*---------------------------------------------------*/
static s32 __url_fileinfo( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 )
{
	char *pTimestamp = get_timestamp();
#ifndef SIGN_DISABLE
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );
	struct apx_userinfo_st *pUserInfo = uid2uinfo( ( u32 )current_uid_get() );

	if( NULL == pUserInfo )
	{
		clog( "user not login." );
		return -1;
	}
	
	__sign_result( pUserInfo->name, pUserInfo->passwd, pTimestamp, u8Sign, &sSignLen );
	snprintf(  ( char* )pUrl, sLen,
			"%s%s/%s?_=%s&sign=admin:%s", 
			g_pCloudRootUrl, g_restful_url[type].url, 
			( char* )param1,
			pTimestamp, u8Sign );
#else
#ifdef SIGN_DEMO
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );
	
	__sign_result( "admin", "admin", pTimestamp, u8Sign, &sSignLen );
	snprintf(  ( char* )pUrl, sLen,
		"%s%s/%s?_=%s&sign=admin:%s", 
		g_pCloudRootUrl, g_restful_url[type].url, 
		( char* )param1,
		pTimestamp, u8Sign );
#else
	snprintf(  ( char* )pUrl, sLen,
			"%s%s/%s?_=%s", 
			g_pCloudRootUrl, g_restful_url[type].url, 
			( char* )param1,
			pTimestamp );
#endif
#endif
	return 0;
}

/*-------------------------------------------------
*Function:    __url_common
*Description:   
*           构造 URL
*		/${url}?_=${timestamp}&sign=admin:${sign}
*Parameters: 
*	type[IN]		请求类型
*	pUrl[OUT]	url
*	sLen[IN]		url 大小
*	param1[IN]	null
*	param2[IN]	null
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-7-8        Initial Draft 
*---------------------------------------------------*/
static s32 __url_common( restful_e type, s8 *pUrl, size_t sLen, void *param1, void *param2 )
{
	int task_id;
	char *pTimestamp =  get_timestamp();
#ifndef SIGN_DISABLE
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );
	struct apx_userinfo_st *pUserInfo = uid2uinfo( ( u32 )current_uid_get() );

	if( NULL == pUserInfo )
	{
		clog( " user not login Or  do not set cloud server." );
		return -1;
	}
		
	__sign_result( pUserInfo->name, pUserInfo->passwd, pTimestamp, u8Sign, &sSignLen );
	snprintf(  ( char* )pUrl, sLen,
			"%s%s?_=%s&sign=admin:%s",
			g_pCloudRootUrl, g_restful_url[type].url,
			pTimestamp, u8Sign );
#else
#ifdef SIGN_DEMO
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );
	
	__sign_result( "admin", "admin", pTimestamp, u8Sign, &sSignLen );
	snprintf(  ( char* )pUrl, sLen,
			"%s%s?_=%s&sign=admin:%s",
			g_pCloudRootUrl, g_restful_url[type].url,
			pTimestamp, u8Sign );
#else
	snprintf(  ( char* )pUrl, sLen,
			"%s%s?_=%s",
			g_pCloudRootUrl, g_restful_url[type].url,
			pTimestamp );
#endif
#endif
	return 0;
}

/** 解析错误信息 */
static void  __parse_errinfo( wjson wj, cld_result_st *result )
{
	char *pstr;
	
	pstr = wjson_lookup_string_by_name( wj, "message", NULL );
	if( pstr)
	{
		strncpy( result->u.err.msg, pstr, sizeof( result->u.err.msg ) - 1 ); 
		result->u.err.msg[sizeof( result->u.err.msg ) - 1] = 0;
	}

	pstr = wjson_lookup_string_by_name( wj, "errors", NULL );
	if( pstr )
	{
		strncpy( result->u.err.errors, pstr, sizeof( result->u.err.errors ) - 1 ) ;
		result->u.err.errors[sizeof( result->u.err.errors ) - 1] = 0;
	}

	return;
}

/** 解析用户信息 */
static s32 __parse_userinfo( s8* content, s32 len, cld_result_st *result )
{
	char *pStr;
	wjson wj, wjInfo;

	wj = wjson_from_string( ( char* )content, ( u32 )len );
	if( NULL == wj )
	{
		clog( "convert json to string failed( str: %d-%u: %s )", len, ( u32 )len, content );
		return -1;
	}

	result->status = wjson_lookup_number_by_name( wj, "status", result->status );
	if( result->status != HTTP_OK )
	{
		//clog( "response status( %u).", result->status );
		__parse_errinfo( wj, result );
		wjson_free( wj );
		return 0;	
	}
	
	wjInfo = wjson_lookup_by_name( wj, "userinfo" );
	if( NULL == wjInfo )
	{
		clog( "no found key: userinfo." );
		wjson_free( wj );
		return -2;
	}
	
	pStr = wjson_lookup_string_by_name( wjInfo, "name", NULL );
	if( pStr != NULL )
	{
		strncpy( result->u.userinfo.name, pStr, sizeof( result->u.userinfo.name ) - 1 ); 
		result->u.userinfo.name[sizeof( result->u.userinfo.name ) - 1] = 0;
	}
	
	pStr = wjson_lookup_string_by_name( wjInfo, "passwd", NULL );
	if( pStr != NULL )
	{
		strncpy( result->u.userinfo.passwd, pStr, sizeof( result->u.userinfo.passwd ) - 1 ); 
		result->u.userinfo.passwd[sizeof( result->u.userinfo.passwd ) - 1] = 0;
	}
	
	pStr = wjson_lookup_string_by_name( wjInfo, "email", NULL );
	if( pStr != NULL )
	{
		strncpy( result->u.userinfo.email, pStr, sizeof( result->u.userinfo.email ) - 1 ); 
		result->u.userinfo.email[sizeof( result->u.userinfo.email ) - 1] = 0;
	}
	
	pStr = wjson_lookup_string_by_name( wjInfo, "cdatetime", NULL );
	if( pStr != NULL )
	{
		strncpy( result->u.userinfo.createTm, pStr, sizeof( result->u.userinfo.createTm ) - 1 ); 
		result->u.userinfo.createTm[sizeof( result->u.userinfo.createTm ) - 1] = 0;
	}

	pStr = wjson_lookup_string_by_name( wjInfo, "loginTime", NULL );
	if( pStr != NULL )
	{
		strncpy( result->u.userinfo.loginTm, pStr, sizeof( result->u.userinfo.loginTm ) - 1 ); 
		result->u.userinfo.loginTm[sizeof( result->u.userinfo.loginTm ) - 1] = 0;
	}

	pStr = wjson_lookup_string_by_name( wjInfo, "lastLoginTime", NULL );
	if( pStr != NULL )
	{
		strncpy( result->u.userinfo.lastLoginTm, pStr, sizeof( result->u.userinfo.lastLoginTm ) - 1 ); 
		result->u.userinfo.lastLoginTm[sizeof( result->u.userinfo.lastLoginTm ) - 1] = 0;
	}
	
	pStr = wjson_lookup_string_by_name( wjInfo, "remark", NULL );
	if( pStr != NULL )
	{
		strncpy( result->u.userinfo.remark, pStr, sizeof( result->u.userinfo.remark ) - 1 ); 
		result->u.userinfo.remark[sizeof( result->u.userinfo.remark ) - 1] = 0;
	}
	
	result->u.userinfo.admin = wjson_lookup_number_by_name( wjInfo, "admin", 0 );
	result->u.userinfo.status = wjson_lookup_number_by_name( wjInfo, "status", 0 );
	result->u.userinfo.quota = wjson_lookup_number_by_name( wjInfo, "quota", 0 );
	
	result->u.userinfo.usrId = wjson_lookup_number_by_name( wjInfo, "userId", 0 );
	result->u.userinfo.groupId = wjson_lookup_number_by_name( wjInfo, "groupid", 0 );

	result->u.userinfo.downLimit = wjson_lookup_number_by_name( wjInfo, "downSplimit", 0 );
	result->u.userinfo.upLimit = wjson_lookup_number_by_name( wjInfo, "upSplimit", 0 );
	result->u.userinfo.activeTaskLimit = wjson_lookup_number_by_name( wjInfo, "activeTasklimit", 0 );
	wjson_free( wj );
	
	return 0;
}

static s32 __parse_taskcreate( s8* content, s32 len, cld_result_st *result )
{
	int ret = 0;
	cld_taskinfo_st *pInfo = NULL;
	wjson wj, wjInfo;

	wj = wjson_from_string( ( char* )content, ( u32 )len );
	if( NULL == wj )
	{
		clog( "convert json to string failed( str: %s )", content );
		return -1;
	}

	result->status = wjson_lookup_number_by_name( wj, "status", result->status );
	if( result->status != HTTP_OK )
	{
	//	clog( "response status( %u).", result->status );
		__parse_errinfo( wj, result );
		wjson_free( wj );
		return 0;	
	}

	wjInfo = wjson_lookup_by_name( wj, "taskinfo" );
	if( NULL == wjInfo )
	{
		clog( "no found key: taskinfo." );
		wjson_free( wj );
		return -2;
	}

	pInfo = &result->u.taskinfo;
	ret = __parse_taskdetail( wjInfo, pInfo, TREQ_CRT );

	wjson_free( wj );
	return ret;
}

static s32 __parse_taskstart( s8* content, s32 len, cld_result_st *result )
{
	wjson wj;
	
	wj = wjson_from_string( ( char* )content, ( u32 )len );
	if( NULL == wj )
	{
		clog( "convert json to string failed( str: %s )", content );
		return -1;
	}

	result->status = wjson_lookup_number_by_name( wj, "status", result->status );
	if( result->status != HTTP_OK )
	{
	//	clog( "response status( %u).", result->status );
		__parse_errinfo( wj, result );
	}
	
	wjson_free( wj );
	return 0;
}

static s32 __parse_taskinfo( s8* content, s32 len, cld_result_st *result )
{
	int ret = 0;
	cld_taskinfo_st *pInfo = NULL;
	wjson wj, wjInfo;

	wj = wjson_from_string( ( char* )content, ( u32 )len );
	if( NULL == wj )
	{
		clog( "convert json to string failed( str: %s )", content );
		return -1;
	}

	result->status = wjson_lookup_number_by_name( wj, "status", result->status );
	if( result->status != HTTP_OK )
	{
	//	clog( "response status( %u).", result->status );
		__parse_errinfo( wj, result );
		wjson_free( wj );
		return 0;	
	}

	wjInfo = wjson_lookup_by_name( wj, "taskInfo" );
	if( wjInfo )
	{
		cldtask_e etype;
		cld_fileinfo_st *pFileInfo = NULL;

		etype = wjson_lookup_number_by_name( wjInfo, "taskType", 0xFF );
		if( etype > CLDTASK_MAX )
		{
			clog( "task type invalid( type: %u).", etype );
			return -1;
		}
		
		result->type= etype;
		if( CLD_UPLOAD == etype )
		{
			pFileInfo = &result->u.fileinfo;
			ret = __parse_filedetail( wjInfo, pFileInfo, FREQ_ST );
		}
		else
		{
			pInfo = &result->u.taskinfo;
			pInfo->type = etype;
			ret = __parse_taskdetail( wjInfo, pInfo, TREQ_ST );
		}
	}

	wjson_free( wj );
	return ret;
}

static int __parse_filedetail(  wjson wj, cld_fileinfo_st* pFileInfo, freqfrom_e eReqFrom )
{
	s32 blkCnt = 0;
	char *pStr = NULL;
	wjson wjBlkInfo;

	pFileInfo->eStatus =wjson_lookup_number_by_name( wj, "status", FILE_UP_NONE );
	if( pFileInfo->eStatus > FILE_UP_HALF )
	{
		clog( "file status err( status: %d ).", pFileInfo->eStatus );
		return ( eReqFrom != FREQ_LIST ) ? -1 : 0;
	}

	pStr = wjson_lookup_string_by_name( wj, "_id", NULL );
	if( NULL == pStr || 0 == strcmp( pStr, "null" ))
	{
		clog( "no found key: fileId Or fileId is null." );
		return ( eReqFrom != FREQ_LIST ) ? -2 : 0;
	}
	
	blkCnt = wjson_lookup_number_by_name( wj, "blockNum", -1 );
	if(  blkCnt < 0 )
	{
		clog( "key: blockNum is valid( block: %d ).",  blkCnt );
		return ( eReqFrom != FREQ_LIST ) ? -3: 0;
	}

	if( FREQ_UP == eReqFrom || FREQ_DOWN == eReqFrom )
	{
		/** fileinfo: check fileId */
		if( ( blkCnt != pFileInfo->blkCnt )
			|| ( strcmp( pStr, pFileInfo->fileId ) != 0 ) )
		{
			clog( "check blkCnt and fileId failed( req: %d -%s, resp: %d - %s).",
				pFileInfo->blkCnt, pFileInfo->fileId, blkCnt, pStr );
			return -4;
		}
	}
	else
	{
		strncpy( pFileInfo->fileId, pStr, sizeof( pFileInfo->fileId ) - 1 );
		if( FREQ_PRE== eReqFrom )
		{//preload
			return 0;
		}
	}

	pStr = wjson_lookup_string_by_name( wj, "fileName", NULL );
	if( NULL == pStr || 0 == strcmp( pStr, "null" ))
	{
		clog( "no found key: fileName Or fileName is null." );
		return ( eReqFrom != FREQ_LIST ) ? -5 : 0;
	}
	strncpy( pFileInfo->name, pStr, sizeof( pFileInfo->name ) - 1 );		

	 pStr = wjson_lookup_string_by_name( wj, "fileSign", NULL );
	 if( NULL == pStr  || 0 == strcmp( pStr, "null" ) )
	 {
		 clog( "no found key: fileSign Or fileSign is null." );
		 return ( eReqFrom != FREQ_LIST ) ? -6 : 0;
	 }
	 strncpy( pFileInfo->sign, pStr, sizeof( pFileInfo->sign ) - 1 );

	 pStr = wjson_lookup_string_by_name( wj, "ctime", NULL );
	 if( NULL != pStr && 0 != strcmp( pStr, "null" ) )
	 {
		 strncpy( pFileInfo->ctime, pStr, sizeof( pFileInfo->ctime ) - 1 );
	 }
	 
	pFileInfo->size =wjson_lookup_number_by_name( wj, "size", 0 );
	if( 0 == pFileInfo->size )
	{
		clog( "file size is 0." );
		return ( eReqFrom != FREQ_LIST ) ? -7 : 0;
	}

	if( FREQ_LIST == eReqFrom || FREQ_NONE == eReqFrom || FREQ_ST == eReqFrom )
	{
		if( FILE_UP_DONE == pFileInfo->eStatus )
		{
			pFileInfo->cur_size = pFileInfo->size;
			return 0;
		}
		else if( FILE_UP_NONE == pFileInfo->eStatus )
		{
			return 0;
		}
	}

	if( FREQ_NONE != eReqFrom )
	{
		wjBlkInfo = wjson_lookup_by_name( wj, "blockInfo" );
		if( wjBlkInfo )
		{
			wjson_for_each( wjBlkInfo, __iterator_blocks, pFileInfo, &eReqFrom );
		}
	}
	return 0;
}

static int __parse_taskdetail(  wjson wj, cld_taskinfo_st* pTaskInfo, treqfrom_e eReqFrom )
{
	int ret = 0;
	char *pStr = NULL;
	wjson wjStats, wjTmp;
	
	pTaskInfo->type = wjson_lookup_number_by_name( wj, "taskType", 0xFF );
	if( pTaskInfo->type >= CLDTASK_MAX )
	{
		clog( "task type invalid( type: %u).", pTaskInfo->type );
		return ( eReqFrom != TREQ_LIST ) ? -1 : 0;
	}
	
	pTaskInfo->status = wjson_lookup_number_by_name( wj, "status", -1 );
	if( pTaskInfo->status < 0  )
	{
		clog( "taskStatus is invalid( taskSt: %d).", pTaskInfo->status);
		return ( eReqFrom != TREQ_LIST ) ? -2 : 0;
	}
	
	pTaskInfo->priority = wjson_lookup_number_by_name( wj, "priority", 0 );
	pTaskInfo->conns = wjson_lookup_number_by_name( wj, "threadNum", 0 );

	pStr = wjson_lookup_string_by_name( wj, "url", NULL );
	if( NULL == pStr  || 0 == strcmp( pStr, "null" ) )
	{
		clog( "no found key: url Or url is null." );
		return ( eReqFrom != TREQ_LIST ) ? -3 : 0;
	}
	strncpy( pTaskInfo->url, pStr, sizeof( pTaskInfo->url ) - 1 );

	pStr = wjson_lookup_string_by_name( wj, "_id", NULL );
	if( NULL == pStr  || 0 == strcmp( pStr, "null" ) )
	{
		clog( "no found key: _id Or _id is null." );
		return ( eReqFrom != TREQ_LIST ) ? -4 : 0;
	}
	strncpy( pTaskInfo->fileId, pStr, sizeof( pTaskInfo->fileId ) - 1 );

	pStr = wjson_lookup_string_by_name( wj, "ctime", NULL );
	if( NULL != pStr && 0 != strcmp( pStr, "null" ) )
	{
		strncpy( pTaskInfo->ctime, pStr, sizeof( pTaskInfo->ctime ) - 1 );
	}
	
	pTaskInfo->size = wjson_lookup_number_by_name( wj, "size", 0 );
	pStr = wjson_lookup_string_by_name( wj, "name", NULL );
	if( NULL != pStr && 0 != strcmp( pStr, "null" ) )
	{
		strncpy( pTaskInfo->name, pStr, sizeof( pTaskInfo->name ) - 1 );
	}
	if( TREQ_CRT == eReqFrom )
	{
		return 0;
	}
	
	wjStats = wjson_lookup_by_name( wj, "stats" );
	if( NULL == wjStats )
	{
		return 0;
	}

	if( CLD_FTP == pTaskInfo->type )
	{
		pTaskInfo->download_size = wjson_lookup_number_by_name( wjStats, "transferred", 0 );
		pTaskInfo->speed = wjson_lookup_number_by_name( wjStats, "speed", 0 );
		pTaskInfo->remain = wjson_lookup_number_by_name( wjStats, "eta", 0 );
		return 0;
	}
	
	wjTmp = wjson_lookup_by_name( wjStats, "total" );
	if( wjTmp != NULL )
	{
		pTaskInfo->download_size = wjson_lookup_number_by_name( wjTmp, "downloaded", 0 );
	}
	
	wjTmp = wjson_lookup_by_name( wjStats, "present" );
	if( wjTmp != NULL )
	{
		pTaskInfo->speed = wjson_lookup_number_by_name( wjTmp, "speed", 0 );
	}
	
	wjTmp = wjson_lookup_by_name( wjStats, "future" );
	if( wjTmp != NULL )
	{
		pTaskInfo->remain = wjson_lookup_number_by_name( wjTmp, "eta", 0 );
	}
	
	return 0;
}

static int __iterator_tasklist( wjson wj, void* arg1, void* arg2 )
{
	int ret = 0;
	char *pStr = NULL;
	int *pcnt = ( int* )arg1;
	cld_list_st *pTmp = NULL;
	cld_list_st *pInfo = NULL;
	cld_taskinfo_st *pTaskInfo = NULL;
	cld_fileinfo_st *pFileInfo = NULL;
	cld_result_st *pResult = NULL;

	if( pcnt )
	{
		/** task cnt */
		( *pcnt )++;
		return 0;
	}

	/** task info */
	pResult = ( cld_result_st* )arg2;
	pTmp = ( cld_list_st *)pResult->u.ptr;
	pInfo = &( pTmp[pResult->cnt] );
	pInfo->type = wjson_lookup_number_by_name( wj, "taskType", 0xFF );
	if( pInfo->type > CLDTASK_MAX )
	{
		clog( "task type invalid( type: %u).", pInfo->type );
		return -1;
	}

	if( CLD_UPLOAD == pInfo->type )
	{
		pFileInfo = &pInfo->u.fileinfo;
		__parse_filedetail( wj, pFileInfo, FREQ_LIST );
	}
	else
	{
		pTaskInfo = &pInfo->u.taskinfo;
		pTaskInfo->type = pInfo->type;
		__parse_taskdetail( wj, pTaskInfo, TREQ_LIST );
	}

	pResult->cnt++;
	
	return 0;
}

static s32 __parse_tasklist( s8* content, s32 len, cld_result_st *result )
{
	int ret = 0;
	int task_cnt = 0;
	wjson wj, wjArr, wjInfo;
	cld_list_st *pList = NULL;

	wj = wjson_from_string( ( char* )content, ( u32 )len );
	if( NULL == wj )
	{
		clog( "convert json to string failed( str: %s )", content );
		return -1;
	}

	result->status = wjson_lookup_number_by_name( wj, "status", result->status );
	if( result->status != HTTP_OK )
	{
	//	clog( "response status( %u).", result->status );
		__parse_errinfo( wj, result );
		wjson_free( wj );
		return 0;	
	}

	result->total = wjson_lookup_number_by_name( wj, "totalCnt", 0 );
	if( 0 == result->total )
	{
		goto end;	
	}

	wjArr = wjson_lookup_by_name( wj, "taskArr" );
	if( NULL == wjArr )
	{
		goto end;
	}
	
	task_cnt = 0;
	wjson_for_each( wjArr, __iterator_tasklist, &task_cnt, NULL );
	if( 0 == task_cnt  )
	{
		goto end;
	}

	pList = calloc( 1, task_cnt * sizeof( cld_list_st ) );
	if( NULL == pList )
	{
		clog( "malloc failed( %d * %ld = size: %ld ).", task_cnt , sizeof( cld_list_st ), task_cnt * sizeof( cld_list_st ) );
		goto end;
	}
	
	result->u.ptr = ( void* )pList;
	ret = wjson_for_each( wjArr, __iterator_tasklist, NULL, result );
	if( ret < 0 )
	{
		free( pList );
		return -2;
	}
	
end:
	wjson_free( wj );
	return 0;
}

static int __iterator_blocks( wjson wj, void* arg1, void* arg2 )
{
	s32 s32Idx = 0,
		s32Status = 0;
	char *pStr = NULL;
	freqfrom_e eReqFrom = *( freqfrom_e* )arg2;
	cld_fileinfo_st *pFileInfo = ( cld_fileinfo_st* )arg1;

	if( FREQ_LIST == eReqFrom ||FREQ_ST == eReqFrom  )
	{
		s32Status = wjson_lookup_number_by_name( wj, "status", -1 );
		if( 1== s32Status )
		{
			pFileInfo->cur_size += wjson_lookup_number_by_name( wj, "size", 0 );
		}
		
		return 0;
	}
	
	s32Idx = wjson_lookup_number_by_name( wj, "index", -1 );
	if( s32Idx <= 0 )
	{
		clog( "index is invalid( idx: %d).", s32Idx );
		return -1;
	}
	s32Idx--;

	pStr= wjson_lookup_string_by_name( wj, "blkSign", NULL );
	if( NULL == pStr )
	{
		clog( "no found key: blkSign." );
		return -3;
	}
	clog( "%p, index: %d, sha1sum:%s.", pFileInfo->pblk, s32Idx, pStr );
	apx_block_set_sign( pFileInfo->pblk, s32Idx, pStr );

	if( FREQ_UP == eReqFrom )
	{
		s32Status = wjson_lookup_number_by_name( wj, "status", -1 );
		if( s32Status < 0 )
		{
			clog( "status is invalid( status: %d).", s32Status );
			return -2;
		}
		apx_block_set_status( pFileInfo->pblk, s32Idx, s32Status );
	}

	return 0;
}

static s32 __parse_fileinfo( s8* content, s32 len, cld_result_st *result )
{
	int ret = 0;
	char *pStr = NULL;
	wjson wj, wjInfo, wjBlkInfo;
	fileup_e eStatus;
	cld_fileinfo_st *pFileInfo = NULL;

	wj = wjson_from_string( ( char* )content, ( u32 )len );
	if( NULL == wj )
	{
		clog( "convert json to string failed( str: %s )", content );
		return -1;
	}

	result->status = wjson_lookup_number_by_name( wj, "status", result->status );
	if( result->status != HTTP_OK )
	{
		clog( "response status( %u).", result->status );
		__parse_errinfo( wj, result );
		wjson_free( wj );
		return 0;	
	}
	
	wjInfo = wjson_lookup_by_name( wj, "fileInfo" );
	if( NULL == wjInfo )
	{
		clog( "no found key: fileInfo." );
		wjson_free( wj );
		return -2;
	}

	pFileInfo = &result->u.fileinfo;
	ret = __parse_filedetail( wjInfo, pFileInfo, result->u.fileinfo.eReqType );
	if( ret < 0 )
	{
		clog( " __parse_filedetail failed( ret: %d ).", ret  );
		wjson_free( wj );
		return -3;
	}

	wjson_free( wj );
	return 0;
}

static s32 __parse_faultinfo( s8* content, s32 len, cld_result_st *result )
{
	char *pStr = NULL;
	wjson wj, wjInfo, wjBt;
	fileup_e eStatus;

	wj = wjson_from_string( ( char* )content, ( u32 )len );
	if( NULL == wj )
	{
		clog( "convert json to string failed( str: %s )", content );
		return -1;
	}

	result->status = wjson_lookup_number_by_name( wj, "status", result->status );
	if( result->status != HTTP_OK )
	{
		clog( "response status( %u).", result->status );
		__parse_errinfo( wj, result );
		wjson_free( wj );
		return 0;	
	}
	
	wjInfo = wjson_lookup_by_name( wj, "statusInfo" );
	if( NULL == wjInfo )
	{
		clog( "no found key: statusInfo." );
		wjson_free( wj );
		return -2;
	}
	
	result->u.faultinfo.serverCon =wjson_lookup_number_by_name( wjInfo, "serverCon", 0 );
	result->u.faultinfo.storageSpace =wjson_lookup_number_by_name( wjInfo, "storageSpace", 0 );
	result->u.faultinfo.dbCon =wjson_lookup_number_by_name( wjInfo, "dbCon", 0 );
	result->u.faultinfo.dbStorage =wjson_lookup_number_by_name( wjInfo, "dbStorage", 0 );
	result->u.faultinfo.dnsResolve =wjson_lookup_number_by_name( wjInfo, "dnsResolve", 0 );

	wjson_free( wj );
	return 0;
}

static s8 * __generate_content_login( void *param1, void *param2 )
{
	size_t len;
	u8 sha1sum[APX_SIGN_LEN] = { 0 };
	s8* str;
	wjson wj = wjson_new_object();

	len = sizeof( sha1sum );
	__calc_passwd_sha1sum( ( s8* )param1, ( s8* )param2, sha1sum, &len );

	wjson_add_string( wj, "user", param1 );
	wjson_add_string( wj, "passwd", sha1sum );
	str = ( s8* )wjson_string( wj );
	wjson_free( wj );
	return str;
}

static s8* __generate_content_create_http(void *param1, void *param2 )
{
	s8* str;
	cld_http_st *http = ( cld_http_st* )param1;
	wjson wj = wjson_new_object();

	wjson_add_string( wj, "url", http->url );
	if( http->conns > 0 )
	{
		wjson_add_int( wj, "threadNum", http->conns );
	}
	
	if( http->priority > 0 )
	{
		wjson_add_int( wj, "priority", http->priority );
	}

	if( http->remark != NULL )
	{
		wjson_add_string( wj, "remark", http->remark );
	}

	str = ( s8* )wjson_string( wj );
	wjson_free( wj );
	return str;
}

static s8* __generate_content_preload( void *param1, void *param2 )
{
	s32 ret = 0;
	u32 k = 0;
	u64 u64Size = 0;
	u64 BlkSize = 0;
	long offset;

	u32 blkCnt = *( u32* )param2;
	s8* fname = ( s8* )param1;
	s8 *ftmp = NULL;
	FILE *pFile = NULL;
	
	s8* str = NULL;
	wjson wj = NULL,
		   wjBlkInfo = NULL,
		   wjObj = NULL;;
	u8 sha1sum[APX_SIGN_LEN] = { 0 };
	size_t sha1Len = sizeof( sha1sum );
	
	ret = apx_file_is_exist( fname, (off_t* )&u64Size );
	if( 0 == ret )
	{
		clog( "%s file not found.", fname );
		return NULL;
	}
	
	/** last name */
	ftmp = strrchr( fname, '/');
	ftmp = ftmp ? ( ftmp + 1 ) : fname;
	
	pFile = fopen( fname, "rb" );
	if( !pFile )
	{
		clog( "failed to open `%s': %s\n", fname, strerror( errno ));
		return NULL;
	}
	
	/** file sha1sum */
	fseek( pFile, 0, SEEK_SET );
	SHA1_block( pFile, u64Size, sha1sum, &sha1Len );

	wj = wjson_new_object();
	wjson_add_string( wj, "fileName", ( char* )ftmp );
	wjson_add_int( wj, "size", u64Size );
	wjson_add_int( wj, "blockNum", blkCnt );
	wjson_add_string( wj, "fileSign", ( char* )sha1sum );
	if( 0 == blkCnt || 1 == blkCnt )
	{
		goto end;
	}

	/** block sha1sum */
	wjBlkInfo = wjson_add_array( wj, "blockInfo" );
	BlkSize = u64Size / blkCnt;
	for( k = 0; k < blkCnt -1; k++ )
	{
		sha1Len = sizeof( sha1sum );
		fseek( pFile, k * BlkSize, SEEK_SET );
		SHA1_block( pFile, BlkSize, sha1sum, &sha1Len );

		wjObj = wjson_add_object( wjBlkInfo, NULL );
		wjson_add_int( wjObj, "index", k + 1 );
		wjson_add_string( wjObj, "blkSign", ( char* )sha1sum );
	}
	
	/** last block */
	sha1Len = sizeof( sha1sum );
	fseek( pFile, k * BlkSize, SEEK_SET );
	SHA1_block( pFile, u64Size - k * BlkSize, sha1sum, &sha1Len );
	
	wjObj = wjson_add_object( wjBlkInfo, NULL );
	wjson_add_int( wjObj, "index", k + 1 );
	wjson_add_string( wjObj, "blkSign", ( char* )sha1sum );

end:	
	fclose( pFile );
	str = ( s8* )wjson_string( wj );
	wjson_free( wj );
	return str;
}

static s8* __generate_content_common( void *param1, void *param2 )
{
	s8* str;
	wjson wj = wjson_new_object();

	wjson_add_string( wj, "taskId", ( char* )param1 );
	
	str = ( s8* )wjson_string( wj );
	wjson_free( wj );
	return str;
}

#define type2Str(_type_ )	( _type_ == RESTFUL_LOGIN ) ? "Login" :(\
( _type_ == RESTFUL_LOGOUT ) ? "Logout" :(\
( _type_ == RESTFUL_TASK_CRT_HTTP ) ? "HttpCreate" :(\
( _type_ == RESTFUL_TASK_START ) ? "TaskStart" :(\
( _type_ == RESTFUL_TASK_STOP ) ? "TaskStop" :(\
( _type_ == RESTFUL_TASK_DEL ) ? "TaskDel" :(\
( _type_ == RESTFUL_TASK_STATUS ) ? "TaskStatus" :(\
( _type_ == RESTFUL_TASK_LIST ) ? "TaskList" : (\
( _type_ == RESTFUL_UPLOAD_PRELOAD ) ? "UpPreLoad" : (\
( _type_ == RESTFUL_FILEINFO) ? "FileInfo" : (\
( _type_ == RESTFUL_FAULT_CHK) ? "FaultChk" : "unkown"\
))))))))))

/*-------------------------------------------------
*Function:    __http_request_common
*Description:   
*           执行 HTTP 请求
*Parameters: 
*	type[IN]		请求类型
*	param1[IN]	参数1
*	param2[IN]	参数2
*	result[IN/OUT]	响应内容
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-7-8        Initial Draft 
*---------------------------------------------------*/
static s32 __http_request_common( restful_e type, void* param1, void* param2, cld_result_st *result )
{
	s32 s32Ret;
	s8 *pContent = NULL;
	s8 url[URL_LEN_MAX];
	
	HTTP_T *pstHttp = NULL;
	restful_api_st *pApi = &g_restful_url[type];
	
	sptr_t code;
	s32 size;
	s8 *resp;

	if( NULL == g_pCloudRootUrl )
	{
		clog( "not set cloud url." );
		return -1;
	}
	s32Ret = pApi->joint_url( type, url, sizeof( url ), param1, param2 );
	if( s32Ret < 0 )
	{
		clog( "url joint error(type: %d, method: %d).", type, pApi->method );
		return -1;
	}

	pstHttp = RAPI_Init();
	if( !pstHttp )
	{
		clog( "init failed." );
		return -2;
	}
	ctlog( "%s:\nrequest\n\turl: %s", type2Str( type ), url );
	
	/** build request body */
	if( pApi->gene_content )
	{
		pContent = pApi->gene_content( param1, param2 );
		if( NULL == pContent )
		{
			clog( "generate content failed( type: %d, method:%d, url: %s ).", type, pApi->method, url  );
			RAPI_Cleanup( pstHttp );
			return -3;
		}
		RAPI_SetContents( pstHttp, pContent, strlen( ( char* )pContent ) );
		ctlog( "\tcontent: %s", pContent );
	}

	/** send request */
	s32Ret = pApi->request( pstHttp, url, &resp, &size );
	if( s32Ret != RAPI_SUCCESS ) {
		clog( "http request failed(type: %d, method: %d, url: %s).", type, pApi->method, url );
		if( pContent )
		{
			free( pContent );
			pContent = NULL;
		}
		RAPI_Cleanup( pstHttp );
		return -4;
	}
	if( pContent )
	{
		free( pContent );
		pContent = NULL;
	}
	clog( "resp size: %d [%s]", size, resp );
	/** response code */
	s32Ret = RAPI_GetOpt( pstHttp, RMSG_HTTP_CODE, ( sptr_t )&code);
	if( s32Ret != RAPI_SUCCESS ) {
		clog( "http GetOpt failed(type: %d, method: %d, url: %s).", type, pApi->method, url );
		RAPI_Cleanup( pstHttp );
		return -5;
	}
	RAPI_Cleanup( pstHttp );
	
	if( !result )
	{
		free( resp );
		return ( HTTP_OK == ( u32 )code ) ?  0 : -6;
	}

	/** parse response */
	result->status = ( u32 )code;
	s32Ret = pApi->parse_resp( resp, size, result );
	free(resp );
	
	return ( s32Ret < 0 ) ? -7 : 0;
}


/*---------------------------------------------------------
*Function:    apx_cloud_set_url
*Description:   
*           设置云端根域名。
*Parameters: 
*	ps8RootUrl	域名，NULL 表示不设置
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-4-22        Initial Draft 
*-----------------------------------------------------------*/
void apx_cloud_set_url( char* ps8RootUrl )
{
	size_t sLen = 0;

	if( NULL == ps8RootUrl )
	{
		if( g_pCloudRootUrl )
		{
			free( g_pCloudRootUrl );
			g_pCloudRootUrl = NULL;
		}
		return;
	}
	
	/** [ip/url]:port */
	if( strlen( ps8RootUrl ) > 3 )
	{
		if( g_pCloudRootUrl )
		{
			free( g_pCloudRootUrl );
		}

		g_pCloudRootUrl = ( s8* )strdup( ps8RootUrl );
		sLen = strlen( ( char* )g_pCloudRootUrl ) - 1;
		if( g_pCloudRootUrl[sLen] == '/' )
		{
			g_pCloudRootUrl[sLen] = 0;
		}
		//clog( "url:%s", g_pCloudRootUrl );
	}
	return;
}

char* apx_cloud_get_url( void)
{
	return g_pCloudRootUrl;
}

int apx_cloud_sign( s8 *pUrl, size_t sLen )
{
	char *pTimestamp =  get_timestamp();

	if( NULL == pUrl || 0 == sLen )
	{
		clog( "url is null." );
		return -1;
	}

#ifndef SIGN_DISABLE
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );
	struct apx_userinfo_st *pUserInfo = uid2uinfo( ( u32 )current_uid_get() );

	if( NULL == pUserInfo || NULL == pUrl || 0 == sLen )
	{
		clog( " user not login Or  do not set cloud server Or url is null." );
		return -1;
	}
		
	__sign_result( pUserInfo->name, pUserInfo->passwd, pTimestamp, u8Sign, &sSignLen );
	snprintf(  ( char* )pUrl, sLen, "?_=%s&sign=admin:%s",pTimestamp, u8Sign );
#else
#ifdef SIGN_DEMO
	u8 u8Sign[APX_SIGN_LEN] = { 0 };
	size_t sSignLen = sizeof( u8Sign );

	__sign_result( "admin", "admin", pTimestamp, u8Sign, &sSignLen );
	snprintf(  ( char* )pUrl, sLen, "?_=%s&sign=admin:%s", pTimestamp, u8Sign );
#else
	snprintf(  ( char* )pUrl, sLen, "?_=%s", pTimestamp );
#endif
#endif
	return 0;
}

/*---------------------------------------------------------
*Function:    apx_cloud_login
*Description:   
*           登陆云端
*Parameters: 
*	user		用户名
*	passwd	密码
*	userinfo[OUT]		云端返回的用户信息
*Return:
*       return 0 if login success, else return negative.
*History:
*      xyfeng     	  2015-4-22        Initial Draft 
*-----------------------------------------------------------*/
int apx_cloud_login( char *user, char *passwd, cld_userinfo_st *userinfo )
{
	s32 s32Ret = 0;
	cld_result_st stResult;

	if( !user || !passwd || !userinfo )
	{
		clog( "user or passwd is null." );
		return -1;
	}
	
	memset( &stResult, 0, sizeof( cld_result_st ) );

	s32Ret = __http_request_common( RESTFUL_LOGIN, user, passwd, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -2;
	}
	
	if( stResult.status != HTTP_OK )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}
	
	*userinfo = stResult.u.userinfo;
	return 0;
}

/*---------------------------------------------------------
*Function:    apx_cloud_logout
*Description:   
*           注销
*Parameters: 
*	 ( void )	
*Return:
*       int :
*History:
*      xyfeng     	  2015-4-22        Initial Draft 
*-----------------------------------------------------------*/
int apx_cloud_logout()
{


	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_task_create
*Description:   
*           创建下载任务
*Parameters: 
*	trans_opt[IN]	任务信息
*	taskinfo[OUT]	任务状态
*Return:
*       return task id if success, else return negative.
*History:
*      xyfeng     	  2015-4-22        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_task_create( struct apx_trans_opt *trans_opt, cld_taskinfo_st *taskinfo )
{
	s32 s32Ret = 0;
	size_t sLen1, sLen2, usrLen, pwLen;
	char *pFtp = NULL;

	restful_e restful_type;
	cld_http_st stHttp;
	cld_result_st stResult;
		
	if( NULL == trans_opt || NULL == taskinfo || 0 == strlen( trans_opt->uri ) )
	{
		clog( "parameters is null." );
		return -1;
	}

	if( trans_opt->proto != APX_TASK_PROTO_HTTP
		&&  trans_opt->proto != APX_TASK_PROTO_FTP )
	{
		clog( "unkown protocol( proto: %d ).", trans_opt->proto );
		return -2;
	}
	
	memset( &stResult, 0, sizeof( cld_result_st ) );
	memset( &stHttp, 0, sizeof( cld_http_st ) );
	
	stHttp.conns = trans_opt->concurr;
	stHttp.priority = trans_opt->priv;
	if( trans_opt->proto == APX_TASK_PROTO_HTTP )
	{
		restful_type = RESTFUL_TASK_CRT_HTTP;
		strncpy( stHttp.url, trans_opt->uri, sizeof( stHttp.url ) );
	}
	else/** ftp */
	{
		restful_type = RESTFUL_TASK_CRT_FTP;
		usrLen = strlen( trans_opt->ftp_user );
		pwLen = strlen( trans_opt->ftp_passwd );
		
		if( 0 == usrLen )
		{
			strncpy( stHttp.url, trans_opt->uri, sizeof( stHttp.url ) );
		}
		else
		{
			pFtp = strcasestr(  trans_opt->uri, CLD_FTP_PREFIX );
			if( NULL == pFtp )
			{
				strncpy( stHttp.url, trans_opt->uri, sizeof( stHttp.url ) );
			}
			else
			{
				sLen2 = sizeof( stHttp.url );
				sLen1 = snprintf( stHttp.url, sLen2, CLD_FTP_PREFIX"%s:",	\
							trans_opt->ftp_user );
				if( pwLen > 0 )
				{
					sLen1 += snprintf( stHttp.url + sLen1, sLen2 - sLen1,	\
						"%s@", trans_opt->ftp_passwd );
				}
				snprintf( stHttp.url + sLen1, sLen2 - sLen1,	"%s", pFtp + 6 );/** sizeof( "ftp://" ) */
			}
		}
	}
	

	s32Ret = __http_request_common( restful_type, &stHttp, NULL, &stResult );
	if( s32Ret < 0 || stResult.status != HTTP_OK )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -1;
	}

	*taskinfo = stResult.u.taskinfo;
	
	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_task_start
*Description:   
*           开始任务下载
*Parameters: 
*	taskId[IN]		任务ID
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-4-22        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_task_start( char *taskId )
{
	s32 s32Ret = 0;
	cld_result_st stResult;

	if( NULL == taskId || 0 == strlen( taskId ) )
	{
		clog( "parameters is null." );
		return -1;
	}
	
	memset( &stResult, 0, sizeof( cld_result_st ) );
	s32Ret =  __http_request_common( RESTFUL_TASK_START, taskId, NULL, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -2;
	}

	if( stResult.status != HTTP_OK )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}
	
	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_task_stop
*Description:   
*           暂停下载任务
*Parameters: 
*	task_id[IN]		任务ID
*	cld_list_st[OUT]	任务信息
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-4-22        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_task_stop( char *taskId, cld_list_st *info )
{
	s32 s32Ret = 0;
	cld_result_st stResult;

	if( NULL == taskId || 0 == strlen( taskId ) )
	{
		clog( "parameters is null." );
		return -1;
	}
	
	memset( &stResult, 0, sizeof( cld_result_st ) );
	s32Ret = __http_request_common( RESTFUL_TASK_STOP, taskId, NULL, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -2;
	}

	if( stResult.status != HTTP_OK )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}

	if( info != NULL )
	{
		info->type = stResult.type;
		if( CLD_UPLOAD != info->type )
		{
			info->u.taskinfo = stResult.u.taskinfo;
		}
		else
		{
			info->u.fileinfo = stResult.u.fileinfo;
		}
	}
	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_task_del
*Description:   
*           删除下载任务
*Parameters: 
*	task_id[IN]		任务ID
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-4-22        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_task_del( char *taskId )
{
	s32 s32Ret = 0;
	cld_result_st stResult;

	if( NULL == taskId || 0 == strlen( taskId ) )
	{
		clog( "parameters is null." );
		return -1;
	}
	
	memset( &stResult, 0, sizeof( cld_result_st ) );
	s32Ret = __http_request_common( RESTFUL_TASK_DEL, taskId, NULL, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -2;
	}
	
	if( stResult.status != HTTP_OK )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}
	
	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_task_status
*Description:   
*           获取下载任务状态
*Parameters: 
*	task_id[IN]		任务ID
*	cld_list_st[OUT]		任务信息
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-4-22        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_task_status( char *taskId, cld_list_st *info  )
{
	s32 s32Ret = 0;
	cld_result_st stResult;

	if( NULL == taskId || 0 == strlen( taskId )  || NULL == info )
	{
		clog( "parameters is null." );
		return -1;
	}
	
	memset( &stResult, 0, sizeof( cld_result_st ) );
	s32Ret = __http_request_common( RESTFUL_TASK_STATUS, taskId, NULL, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -2;
	}

	
	if( stResult.status != HTTP_OK )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}

	if( info != NULL )
	{
		info->type = stResult.type;
		if( CLD_UPLOAD != info->type )
		{
			info->u.taskinfo = stResult.u.taskinfo;
		}
		else
		{
			info->u.fileinfo = stResult.u.fileinfo;
		}
	}
	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_task_freelist
*Description:   
*           释放任务列表
*Parameters: 
*	ptask_list[IN]		任务列表
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-7-6        Initial Draft 
*---------------------------------------------------*/
void apx_cloud_task_freelist( cld_tasklist_st *ptask_list )
{
	int k = 0;
	cld_taskinfo_st *ptaskInfo = NULL;

	if( NULL == ptask_list || NULL == ptask_list->plist )
	{
		return;
	}
	
	free( ptask_list->plist );
	ptask_list->plist = NULL;
	
	return;
}

/*-------------------------------------------------
*Function:    apx_cloud_task_list
*Description:   
*           获取云端任务列表
*Parameters: 
*	start[IN]			分页开始 ID
*	limit[IN]			获取记录数
*	ptask_list[OUT]	任务详细信息
*Return:
*       int :
*History:
*      xyfeng     	  2015-7-6        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_task_list( int start,  int limit,
							cld_tasklist_st *ptask_list )
{
	s32 s32Ret = 0;
	cld_result_st stResult;

	if( NULL == ptask_list )
	{
		clog( "parameters is null." );
		return -1;
	}

	ptask_list->plist = NULL;
	memset( &stResult, 0, sizeof( cld_result_st ) );
	s32Ret = __http_request_common( RESTFUL_TASK_LIST, &start, &limit, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		if( stResult.u.ptr != NULL )
		{
			free( stResult.u.ptr );
			stResult.u.ptr = NULL;
		}
		return -2;
	}

	
	if( stResult.status != HTTP_OK )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}

	ptask_list->total 	= 	stResult.total;
	ptask_list->cnt 	= 	stResult.cnt;
	ptask_list->plist	 = 	( cld_list_st* )stResult.u.ptr;	
	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_upload_proload
*Description:   
*           预取上传文件分段信息
*Parameters: 
*	fname[IN]		文件绝对路径
*	blkcnt[IN]		文件分块数
*	cld_fileinfo_st[IN/OUT]	文件详细信息
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-6-4        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_upload_proload( s8* fname, u32 blkcnt, cld_fileinfo_st *fileinfo  )
{
	s32 s32Ret = 0;
	cld_result_st stResult;

	if( NULL == fname
		|| NULL == fileinfo )
	{
		clog( "filename parameters is null." );
		return -1;
	}
	
	memset( &stResult, 0, sizeof( cld_result_st ) );
	stResult.u.fileinfo = *fileinfo;
	stResult.u.fileinfo.eReqType = FREQ_PRE;

	s32Ret = __http_request_common( RESTFUL_UPLOAD_PRELOAD, fname, &blkcnt, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -2;
	}

	
	if( HTTP_OK != stResult.status )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}

	*fileinfo = stResult.u.fileinfo;
	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_upload_fileinfo
*Description:   
*           获取文件上传状态
*Parameters: 
*	fileinfo[IN/OUT]	云端返回的文件上传详细信息
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_fileinfo( cld_fileinfo_st *fileinfo  )
{
	s32 s32Ret = 0;
	cld_result_st stResult;

	if( NULL == fileinfo )
	{
		clog( "parameters is null." );
		return -1;
	}
	
	memset( &stResult, 0, sizeof( cld_result_st ) );
	stResult.u.fileinfo = *fileinfo;
	//stResult.u.fileinfo.eReqType = FREQ_UP/FREQ_DOWN/FREQ_NONE;

	s32Ret =  __http_request_common( RESTFUL_FILEINFO, stResult.u.fileinfo.fileId, NULL, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -2;
	}
	
	
	if( HTTP_OK != stResult.status )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}

	*fileinfo = stResult.u.fileinfo;
	return 0;
}

/*-------------------------------------------------
*Function:    apx_cloud_fault_check
*Description:   
*           云端状态检测
*Parameters: 
*	faultinfo[IN]		云端状态
*Return:
*       return 0 if success, else return negative.
*History:
*      xyfeng     	  2015-6-26        Initial Draft 
*---------------------------------------------------*/
int apx_cloud_fault_check( cld_fault_st *faultinfo )
{
	s32 s32Ret = 0;
	cld_result_st stResult;
	
	memset( &stResult, 0, sizeof( cld_result_st ) );
	s32Ret = __http_request_common( RESTFUL_FAULT_CHK, NULL, NULL, &stResult );
	if( s32Ret < 0 )
	{
		clog( "request failed( status: %d ).", stResult.status  );
		return -2;
	}
	

	if( stResult.status != HTTP_OK )
	{
		clog( "cloud err( msg: %s, details: %s ).", stResult.u.err.msg, stResult.u.err.errors );
		return -3;
	}

	if( faultinfo !=NULL )
	{
		*faultinfo = stResult.u.faultinfo;
	}

	return 0;
}

int apx_get_proxy_url( s8** ppUrl )
{
	s32 s32Ret = 0;
	s8 url[URL_LEN_MAX];

	if( 0 == ppUrl )
	{
		clog( "parameters is null." );
		return -1;
	}

	if( NULL == g_pCloudRootUrl )
	{
		clog( "do not set cloud url." );
		return -2;
	}

	*ppUrl = NULL;
	s32Ret = g_restful_url[RESTFUL_TASK_PROXY].joint_url( \
				RESTFUL_TASK_PROXY, url, sizeof( url ), NULL, NULL );
	if( s32Ret < 0 )
	{
		clog( "task_proxy: joint_url failed." );
		return -3;
	}

	*ppUrl = strdup( url );
	return 0;
}

/*-------------------------------------------------
*Function:    apx_build_proxy_json
*Description:   
*           构建中转下载 JSON
*Parameters: 
*	url[IN]		下载链接
*	headers[IN]	headers 数组	
*	size[IN]		数组大小
*Return:
*       返回 JSON 串，使用后需要释放内存
*History:
*      xyfeng     	  2015-7-22        Initial Draft 
*---------------------------------------------------*/
s8* apx_build_proxy_json( s8 *url, s8 *headers[], size_t size )
{
	size_t k  = 0;
	s8 *pJson = NULL;
	wjson wj, wjHead;

	if( NULL == url )
	{
		return NULL;
	}
	
	wj = wjson_new_object();
	wjson_add_string( wj, "url", url );

	if( 0 == size || NULL == headers )
	{
		goto json_string;
	}
	
	wjHead = wjson_add_array( wj, "headers" );
	for( k = 0; k < size; k++ )
	{
		if( headers[k] != NULL )
		{
			wjson_add_string( wjHead, NULL, headers[k] );
		}
	}
		
	json_string:	
		pJson = ( s8* )wjson_string( wj );
		wjson_free( wj );

	return pJson;
}

