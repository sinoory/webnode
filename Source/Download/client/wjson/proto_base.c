/*-----------------------------------------------------------
*      Copyright (c)  AppexNetworks, All Rights Reserved.
*
*FileName:     apx_proto_base.c 
*
*Description:  http 基本消息封装
* 
*History: 
*      Author              Date        	Modification 
*  ----------      ----------  	----------
* 	xyfeng   		2015-4-20     	Initial Draft 
*------------------------------------------------------------*/
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
#include <stdarg.h>
#include <curl/curl.h>
        
/*---Include Local.h File---*/
#include "zebra.h"
#include "proto_base.h"
        
/*------------------------------------------------------------*/
/*                          Private Macros Defines                          */
/*------------------------------------------------------------*/

//server response content  max len
#define RAPI_RESP_LEN_MAX       ( 5 * 1024 * 1024 )//5M

#define RAPI_DEFAULT_TIMEOUT        ( 30 )

//#define PBASE_DEBUG
#undef rlog
#ifdef PBASE_DEBUG
#define    rlog( fmt, args... )     \
        do { \
            fprintf( stderr, "[%03d-%s]"fmt"\n", __LINE__, __FUNCTION__, ##args ); \
        }while(0)
#else
#define    rlog( fmt, args... )
#endif

     
/*------------------------------------------------------------*/
/*                          Private Type Defines                            */
/*------------------------------------------------------------*/

//http struct
typedef struct _http_s_
{
    CURL *pstHandle;
    //control 
    s32 s32Timeout;//curl执行超时时间
    s32 s32ConTimeout ;//连接超时
    
    //request elements
    s8  *ps8Params;
    s8  *ps8Cookie;
    struct curl_slist *pstHeaders;

    s32 s16ParaLen:12,//0 --> 4K - 1
          s16CookieLen:12,
          download:1,
          upload:1,
          action:1; 
    
    //responese elements
    s32 s32HttpCode;
    s8  *ps8Resp;
    struct buffer *pstBuf;
	pthread_mutex_t	mLock;
}http_st;

        
/*------------------------------------------------------------*/
/*                         Global Variables                                */
/*------------------------------------------------------------*/
/*---Extern Variables---*/
        
/*---Local Variables---*/
        
        
/*------------------------------------------------------------*/
/*                          Local Function Prototypes                       */
/*------------------------------------------------------------*/
        
         
/*------------------------------------------------------------*/
/*                        Functions                                               */
/*------------------------------------------------------------*/


/*init http struct*/
HTTP_T *RAPI_Init( void )
{
    CURL *pstCurl = NULL;
    http_st *pstHttp = NULL;

    curl_global_init( CURL_GLOBAL_DEFAULT );
    pstCurl = curl_easy_init();
    if( NULL == pstCurl ) {
        rlog( "curl_easy_init failed." );
        curl_global_cleanup();
        return NULL;
    }

    pstHttp = calloc( 1, sizeof( http_st ) );
    if( NULL == pstHttp ) {
        rlog( "calloc for http_st( size:%ld ) failed.", sizeof( http_st ) );
         curl_easy_cleanup( pstCurl );
         curl_global_cleanup();
         return NULL;
    }

    pstHttp->pstHandle = pstCurl;
    pstHttp->s32Timeout = RAPI_DEFAULT_TIMEOUT;
    pstHttp->s32ConTimeout = RAPI_DEFAULT_TIMEOUT;
    pstHttp->ps8Params = NULL;
    pstHttp->ps8Cookie = NULL;
    pstHttp->pstHeaders = NULL;
    pstHttp->ps8Resp = NULL;
    pstHttp->pstBuf = buffer_new( 0 );
	pthread_mutex_init( &pstHttp->mLock, NULL );
	
    curl_easy_setopt( pstCurl, CURLOPT_NOPROGRESS, 1L );
    //curl_easy_setopt( pstCurl,  CURLOPT_TIMEOUT, RAPI_DEFAULT_TIMEOUT );
    curl_easy_setopt( pstCurl,  CURLOPT_CONNECTTIMEOUT, RAPI_DEFAULT_TIMEOUT );

    rlog( "http init ok." );
    return ( HTTP_T* )pstHttp;
}
//curl_easy_reset

void RAPI_Reset( HTTP_T *pHttp )
{
	http_st *pstHttp = ( http_st* )pHttp; 

    if( pstHttp ) {
        if( pstHttp->ps8Params ) {
            free( pstHttp->ps8Params );
            pstHttp->ps8Params = NULL;
        }

        if( pstHttp->ps8Cookie ) {
            free( pstHttp->ps8Cookie );
            pstHttp->ps8Cookie = NULL;
        }
        
        if( pstHttp->pstHeaders ) {
            curl_slist_free_all( pstHttp->pstHeaders );
            pstHttp->pstHeaders = NULL;
        }
#if 0
        if( pstHttp->ps8Resp ) {
            free( pstHttp->ps8Resp );
            pstHttp->ps8Resp = NULL;
        }
#endif
        if( pstHttp->pstBuf ) {
            buffer_reset( pstHttp->pstBuf );
        }
        
        pstHttp->action = 0;
        pstHttp->s32HttpCode = 0;
        if( pstHttp->pstHandle ) {
            curl_easy_reset( pstHttp->pstHandle );
            curl_easy_setopt( pstHttp->pstHandle, CURLOPT_NOPROGRESS, 1L );
            //curl_easy_setopt( pstHttp->pstHandle,  CURLOPT_TIMEOUT, pstHttp->s32Timeout );
            curl_easy_setopt( pstHttp->pstHandle,  CURLOPT_CONNECTTIMEOUT, 
                                        pstHttp->s32ConTimeout );
        }
    }
    
    rlog( "http reset ok." );
    return;
}

void RAPI_Cleanup( HTTP_T *pHttp )
{
	http_st *pstHttp = ( http_st* )pHttp; 

    if( pstHttp ) {
        if( pstHttp->ps8Params ) {
            free( pstHttp->ps8Params );
        }
        
        if( pstHttp->ps8Cookie ) {
            free( pstHttp->ps8Cookie );
        }
        
        if( pstHttp->pstHeaders ) {
            curl_slist_free_all( pstHttp->pstHeaders );
        }
#if 0
        if( pstHttp->ps8Resp ) {
            free( pstHttp->ps8Resp );
            pstHttp->ps8Resp = NULL;
        }
#endif
        if( pstHttp->pstBuf ) {
            buffer_free( pstHttp->pstBuf );
        }
                
        if( pstHttp->pstHandle ) {
            curl_easy_cleanup( pstHttp->pstHandle );
        }
        free( pstHttp );
    }
    curl_global_cleanup();
    rlog( "http cleanup ok." );
    return;
}

/**设置URL参数*/
//uri?param1=123&param2=test
s32 RAPI_SetParams( HTTP_T *pHttp, RAPI_PARA_E s8Type, 
                                              s8 *ps8Key, s8 *ps8Value, s32 s32Value )
{
    s8 s8CharSep;
	http_st *pstHttp = ( http_st* )pHttp; 

    if( pstHttp->action ) {
        rlog( "deny to set params while performing request." );
        return RAPI_FAILED;
    }
    
    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle
        || NULL == ps8Key
        || ( s8Type != RAPI_PARA_INT && s8Type != RAPI_PARA_STR ) ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }

    if( pstHttp->ps8Params ) {
        s8CharSep = '&';
    }
    else {
        pstHttp->ps8Params = calloc( 1, RAPI_URL_LEN_MAX );
        if( NULL == pstHttp->ps8Params ) {
            rlog( "calloc for params( size: %d ) failed.", RAPI_URL_LEN_MAX );
            return RAPI_FAILED;
        }
        pstHttp->s16ParaLen = 0;
        s8CharSep = '?';
    }

    if( RAPI_PARA_STR == s8Type ) {
        if( ps8Value ) {
            pstHttp->s16ParaLen += snprintf(
                                                        pstHttp->ps8Params + pstHttp->s16ParaLen, 
                                                        RAPI_URL_LEN_MAX - pstHttp->s16ParaLen,
                                                        "%c%s=%s",
                                                        s8CharSep, ps8Key, ps8Value );
        }
        else {
            pstHttp->s16ParaLen += snprintf(
                                                        pstHttp->ps8Params + pstHttp->s16ParaLen, 
                                                        RAPI_URL_LEN_MAX - pstHttp->s16ParaLen,
                                                        "%c%s=",
                                                        s8CharSep, ps8Key );

        }
    }
    else {
        pstHttp->s16ParaLen += snprintf(
                                                    pstHttp->ps8Params + pstHttp->s16ParaLen, 
                                                    RAPI_URL_LEN_MAX - pstHttp->s16ParaLen,
                                                    "%c%s=%d",
                                                    s8CharSep, ps8Key, s32Value );
    }
    return RAPI_SUCCESS;    
}


/**设置URL请求头*/
s32 RAPI_SetHeaders( HTTP_T *pHttp, s8 *ps8Key, s8 *ps8Value )
{
	http_st *pstHttp = ( http_st* )pHttp; 
    s8 s8Buf[RAPI_STR_LEN_MAX];

    if( pstHttp->action ) {
        rlog( "deny to set headers while performing request." );
        return RAPI_FAILED;
    }
    
    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle
        || NULL == ps8Key ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }

   if( ps8Value ) {
        snprintf( s8Buf, RAPI_STR_LEN_MAX, "%s: %s", ps8Key, ps8Value );
    }
   else {
       snprintf( s8Buf, RAPI_STR_LEN_MAX, "%s: ", ps8Key );
    }

    pstHttp->pstHeaders = curl_slist_append( pstHttp->pstHeaders, s8Buf );
    
    return RAPI_SUCCESS;
}


/**设置cookie*/
s32 RAPI_SetCookies( HTTP_T *pHttp, s8 *ps8Key, s8 *ps8Value )
{
    s8 s8CharSep;
	http_st *pstHttp = ( http_st* )pHttp; 

    if( pstHttp->action ) {
        rlog( "deny to set cookie while performing request." );
        return RAPI_FAILED;
    }
    
    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle
        || NULL == ps8Key ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }

    if( pstHttp->ps8Cookie ) {
        s8CharSep = ';';
    }
    else {
        pstHttp->ps8Cookie = calloc( 1, RAPI_URL_LEN_MAX );
        if( NULL == pstHttp->ps8Cookie ) {
            rlog( "calloc for cookies( size:%d ) failed.", RAPI_URL_LEN_MAX );
            return RAPI_FAILED;
        }
        pstHttp->s16CookieLen = 0;
        s8CharSep = ' ';
    }

    if( ps8Value ) {
        pstHttp->s16CookieLen += snprintf(
                                                    pstHttp->ps8Cookie + pstHttp->s16CookieLen, 
                                                    RAPI_URL_LEN_MAX - pstHttp->s16CookieLen,
                                                    "%c%s=%s",
                                                    s8CharSep, ps8Key, ps8Value );
    }
    else {
        pstHttp->s16CookieLen += snprintf(
                                                    pstHttp->ps8Cookie + pstHttp->s16CookieLen, 
                                                    RAPI_URL_LEN_MAX - pstHttp->s16CookieLen,
                                                    "%c%s=",
                                                    s8CharSep, ps8Key );

        }

    return RAPI_SUCCESS;    
}

/**设置content*/
s32 RAPI_SetContents( HTTP_T *pHttp, s8 *ps8Str, size_t sLen )
{
	http_st *pstHttp = ( http_st* )pHttp; 

    if( pstHttp->action ) {
        rlog( "deny to set cookie while performing request." );
        return RAPI_FAILED;
    }
    
    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle 
        || NULL == ps8Str 
        || 0 == sLen ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }

    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_POSTFIELDSIZE, (long)sLen  );
    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_POSTFIELDS, ps8Str );
    //curl_easy_setopt( pstHttp->pstHandle, CURLOPT_COPYPOSTFIELDS, ps8Str );
    
    return RAPI_SUCCESS;    
}

/**设置content*/
s32 RAPI_SetContentsLimit2K( HTTP_T *pHttp, const s8 *fmt, ... )
{
    s32 sLen;
    va_list arg;
    static s8 s8Buf[RAPI_URL_LEN_MAX] ;    
	http_st *pstHttp = ( http_st* )pHttp; 

    if( pstHttp->action ) {
        rlog( "deny to set cookie while performing request." );
        return RAPI_FAILED;
    }
    
    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }

    memset( s8Buf, 0, sizeof( s8Buf ) );
    va_start( arg, fmt ) ;    
    sLen = vsnprintf( s8Buf, sizeof( s8Buf ), fmt, arg ) ;    
    va_end( arg ) ; 
    if( ( size_t )sLen >= sizeof( s8Buf ) || sLen < 0 ) {
        rlog( "content length is %d.", sLen );
        return RAPI_FAILED; 
    }

    rlog( "%d:%s:%ld\n", sLen, s8Buf, strlen( s8Buf ) );
    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_POSTFIELDSIZE, (long)sLen  );
    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_POSTFIELDS, s8Buf  );/*libcurl没有拷贝数据，保留的是指针*/

    return RAPI_SUCCESS;    
}

s32 RAPI_GetOpt( HTTP_T *pHttp, RAPI_MSG_E eMsg, sptr_t sParam )
{
	http_st *pstHttp = ( http_st* )pHttp; 

	if( !pstHttp )
	{
		return -1;
	}
	
	( void )sParam;
	switch( eMsg ) {
		case RMSG_DOWNLOAD_FILE:
		break;
		
		case RMSG_HTTP_CODE:
			*( sptr_t * )sParam = pstHttp->s32HttpCode;
		break;

		default:
			break;
	}
	
	return RAPI_SUCCESS;	
}


s32 RAPI_SetOpt( HTTP_T *pHttp, RAPI_MSG_E eMsg, sptr_t sParam )
{
	http_st *pstHttp = ( http_st* )pHttp; 

	( void )sParam;
    switch( eMsg ) {
        case RMSG_DOWNLOAD_FILE:
            pstHttp->download = 1;
        break;

        default:
            break;
    }
    
    return RAPI_SUCCESS;    
}

/*
*支持文件下载
*   1. 指定文件绝对路径，自动写入文件    
*   2. 指定文件句柄，自动写入文件
*   3. 通过 GetOpt 获取文件内容, 最大支持到50 M 
*/

/*
*支持文件上传
*   1. 指定文件绝对路径，自动读取文件上传
*   2. 指定文件句柄，自动上传文件
*
*
*   curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L );
*   curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback );
*   curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx );
*   easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);
*/
static size_t write_response( void *buffer, size_t size,
                                                        size_t nmemb, void *stream )
{
    struct buffer *pstBuf = ( struct buffer* )stream;
    buffer_put( pstBuf, buffer, size * nmemb );
    //rlog( "%ld * %ld = %ld", size, nmemb, size * nmemb );
    return size * nmemb;
}

static s32 RAPI_Request( http_st *pstHttp, s8 *ps8Url,
                                                s8 **pps8Resp, s32 *ps32Len  )
{
    CURLcode eRes;
    long lHttpCode = 0;

    if( pps8Resp != NULL ) {
        *pps8Resp = NULL;
    }
    if( ps32Len != NULL ) {
        *ps32Len = 0;
    }

    if( pstHttp->ps8Params != NULL ) {
        s8 s8Url[RAPI_URL_LEN_MAX];
        
        snprintf( s8Url, RAPI_URL_LEN_MAX, "%s%s", ps8Url, pstHttp->ps8Params );
        curl_easy_setopt( pstHttp->pstHandle, CURLOPT_URL, s8Url );
    }
    else {
        curl_easy_setopt( pstHttp->pstHandle, CURLOPT_URL, ps8Url );
    }
        
    if( pstHttp->pstHeaders ) {
        curl_easy_setopt( pstHttp->pstHandle, CURLOPT_HTTPHEADER, pstHttp->pstHeaders );
    }

    if( pstHttp->ps8Cookie ) {
        curl_easy_setopt( pstHttp->pstHandle, CURLOPT_COOKIE, pstHttp->ps8Cookie );
    }
    
	curl_easy_setopt( pstHttp->pstHandle, CURLOPT_WRITEFUNCTION, write_response );
	curl_easy_setopt( pstHttp->pstHandle, CURLOPT_WRITEDATA, pstHttp->pstBuf );
	curl_easy_setopt( pstHttp->pstHandle, CURLOPT_FOLLOWLOCATION, 1L );
	curl_easy_setopt( pstHttp->pstHandle, CURLOPT_SSL_VERIFYHOST, 0L );
	curl_easy_setopt( pstHttp->pstHandle, CURLOPT_SSL_VERIFYPEER, 0L );


    //curl_easy_setopt(pstHttp->pstHandle, CURLOPT_VERBOSE, 1);   
    //curl_easy_setopt(pstHttp->pstHandle, CURLOPT_HEADER, 1);
    
    eRes = curl_easy_perform(  pstHttp->pstHandle );
    if( eRes != CURLE_OK ) {
        rlog( "perfrom failed( Res:%d, %s ).", eRes, curl_easy_strerror( eRes ) );
        return RAPI_FAILED;
    }

    curl_easy_getinfo( pstHttp->pstHandle, CURLINFO_HTTP_CODE, &lHttpCode);
    pstHttp->s32HttpCode = lHttpCode;
    if( pps8Resp != NULL ) {
        *pps8Resp = buffer_getstr( pstHttp->pstBuf );
        pstHttp->ps8Resp = *pps8Resp;
    }
    if( ps32Len != NULL ) {
        *ps32Len = ( s32 )buffer_size( pstHttp->pstBuf );
    }
        
    return RAPI_SUCCESS;
}

s32 RAPI_Get( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len )
{
    s32 s32Ret = RAPI_SUCCESS;
	http_st *pstHttp = ( http_st* )pHttp; 

    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle
        || NULL == ps8Url ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }

    pstHttp->action = 1;
    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_HTTPGET, 1L );
    s32Ret = RAPI_Request( pstHttp, ps8Url, pps8Resp, ps32Len );
    if( s32Ret != RAPI_SUCCESS ) {
        return RAPI_FAILED;
    }

    return RAPI_SUCCESS;
}

s32 RAPI_Post( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len )
{
    s32 s32Ret = RAPI_SUCCESS;
	http_st *pstHttp = ( http_st* )pHttp; 

    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle
        || NULL == ps8Url ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }
    RAPI_SetHeaders( pstHttp , "Content-type", "application/json" );

    pstHttp->action = 1;
    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_POST, 1L );
    s32Ret = RAPI_Request( pstHttp, ps8Url, pps8Resp, ps32Len );
    if( s32Ret != RAPI_SUCCESS ) {
        return RAPI_FAILED;
    }

    return RAPI_SUCCESS;
}

s32 RAPI_Put( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len )
{
    s32 s32Ret = RAPI_SUCCESS;
	http_st *pstHttp = ( http_st* )pHttp; 

    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle
        || NULL == ps8Url ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }
    RAPI_SetHeaders( pstHttp , "Content-type", "application/json" );

    pstHttp->action = 1;
    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_CUSTOMREQUEST, "PUT"); 
    s32Ret = RAPI_Request( pstHttp, ps8Url, pps8Resp, ps32Len );
    if( s32Ret != RAPI_SUCCESS ) {
        return RAPI_FAILED;
    }

    return RAPI_SUCCESS;
}

s32 RAPI_Delete( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len )
{
    s32 s32Ret = RAPI_SUCCESS;
	http_st *pstHttp = ( http_st* )pHttp; 

    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle
        || NULL == ps8Url ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }
    RAPI_SetHeaders( pstHttp , "Content-type", "application/json" );

    pstHttp->action = 1;
    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_CUSTOMREQUEST, "DELETE"); 
    s32Ret = RAPI_Request( pstHttp, ps8Url, pps8Resp, ps32Len );
    if( s32Ret != RAPI_SUCCESS ) {
        return RAPI_FAILED;
    }

    return RAPI_SUCCESS;
}

s32 RAPI_Head( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len )
{
    s32 s32Ret = RAPI_SUCCESS;
	http_st *pstHttp = ( http_st* )pHttp; 

    if( NULL == pstHttp
        || NULL == pstHttp->pstHandle
        || NULL == ps8Url ) {    
        rlog( "parameter error." );
        return RAPI_FAILED;
    }
    //RAPI_SetHeaders( pstHttp , "Content-type", "application/json" );

    pstHttp->action = 1;
    curl_easy_setopt( pstHttp->pstHandle, CURLOPT_CUSTOMREQUEST, "HEAD"); 
    s32Ret = RAPI_Request( pstHttp, ps8Url, pps8Resp, ps32Len );
    if( s32Ret != RAPI_SUCCESS ) {
        return RAPI_FAILED;
    }

    return RAPI_SUCCESS;
}


#ifdef MAIN_PBASE_DEBUG
int main( int argc, char *argv[] )
{
    s8 *ps8Resp = NULL;
    s32 s32Len = 0;
    s32 s32Method = 0;
    s32 s32Ret = RAPI_SUCCESS;
    HTTP_T *pstHttp = NULL;
    pFuncHttp  pFunHttp[] = {
            RAPI_Get,
            RAPI_Post,
            RAPI_Put,
            RAPI_Delete,
            RAPI_Head
        };
	{
		char *str= "{\"taskId\": 123, \"test\":\"\",\"xyfeng\":null,\"sign\": \"xxx\"}";
		char *pstr = NULL;
		size_t len;
		int value;
		wjson wj;
	
		len = strlen( str );
		printf( "len = %ld\n", len );
		wj = wjson_from_string( str, len );
		if( !wj )
		{
			printf( "wjson failed.\n" );
			return -1;
		}
	
		pstr = wjson_lookup_string_by_name( wj, "sign", NULL );
		if( pstr )
		{
			printf( "pstr = %s\n", pstr );
		}
	
		value = wjson_lookup_number_by_name( wj, "taskId", 0 );
		printf( "value = %d\n", value );

		pstr = wjson_lookup_string_by_name( wj, "xyfeng", NULL );
		if( pstr )
		{
			printf( "pstr = %s\n", pstr );
		}

		pstr = wjson_lookup_string_by_name( wj, "test", NULL );
		if( pstr )
		{
			printf( "pstr = [%s]\n", pstr );
		}
        return 0;	
	}

    if( argc < 3 ) {
        printf( "usage: %s method( 1:get 2:post 3:put 4:delete 5:head ) url\n", argv[0] );
        return -1;
    }
	
    s32Method = atoi( argv[1] );
    if( s32Method <= 0 || s32Method > 5 ) {
        rlog( "method type is invalid.\n" );
        return -1;
    }
    s32Method--;
    
    pstHttp = RAPI_Init();
    if( pstHttp != NULL ) {
        RAPI_SetParamsInt( pstHttp , "Int01", 100 );
        RAPI_SetParamsStr( pstHttp , "Str02", "2000" );
        RAPI_SetHeaders( pstHttp , "Header01", "Header1" );
        
        if( s32Method > 0 ) {
            RAPI_SetCookies( pstHttp , "cook01", "post" );
            RAPI_SetContents( pstHttp, "Content Test", strlen( "Content Test" ) );
        }
        
        s32Ret = pFunHttp[s32Method]( pstHttp, argv[2], &ps8Resp, &s32Len );
        if( s32Ret != RAPI_SUCCESS ) {
            rlog( "pFunHttp failed." );
            return -1;
        }
        rlog( "Len = %d.", s32Len );
        rlog( "\n%s\n", ps8Resp );
        
        //RAPI_Reset( pstHttp );
        RAPI_Cleanup( pstHttp );
    }

    return 0;
}

#endif
