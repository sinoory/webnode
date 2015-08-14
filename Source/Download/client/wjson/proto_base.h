/**
*	Copyright (c)  AppexNetworks, All Rights Reserved.
*	High speed Files Transport
*	Author:	xyfeng
*
*	@defgroup	HTTP 消息封装
*	@ingroup		
*
*	为协议控制提供HTTP 通信接口
*
*	@{
*Function List: 
*/
 
#ifndef _APX_PROTO_BASE_H_
#define _APX_PROTO_BASE_H_
         
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
        
/*---Include Local.h File---*/

#ifdef __cplusplus
extern "C" {
#endif /*end __cplusplus */
        
/*------------------------------------------------------------*/
/*                          Macros Defines                                      */
/*------------------------------------------------------------*/
typedef long sptr_t;
#if 0
#define BTYES_PER_LONG  8
#if BTYES_PER_LONG == 4
typedef int sptr_t;
#elif BTYES_PER_LONG == 8
typedef long sptr_t;
#else
#error "Unknown the value of BTYES_PER_LONG."
#endif
#endif
#define RAPI_STR_LEN_MAX        ( 1024 )
/** url 长度限制 */
#define RAPI_URL_LEN_MAX        ( RAPI_STR_LEN_MAX *2 )
#define RAPI_SUCCESS      ( 0 )
#define RAPI_FAILED         ( -1 )
#define RAPI_AGAIN          ( -2 )
        
/*------------------------------------------------------------*/
/*                    Exported Variables                                        */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                         Data Struct Define                                      */
/*------------------------------------------------------------*/
typedef	void	HTTP_T;
typedef s32 ( *pFuncHttp )( HTTP_T *pstHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len );

//http parameter type
typedef enum _rapi_para_e_
{
    RAPI_PARA_INT,
    RAPI_PARA_STR,
    RAPI_PARA_MAX
}RAPI_PARA_E;
        
typedef enum _rapi_msg_e_
{
	RMSG_CONN_TIMEOUT = 0,
	RMSG_TIMEOUT,

	RMSG_DOWNLOAD_FILE,
	RMSG_DOWNLOAD_HANDLE,
	RMSG_UPLOAD,

	RMSG_HTTP_CODE,
	RMSG_RESP,
	RMSG_RESP_LEN,

	
	RMSG_MAX
}RAPI_MSG_E;


/*------------------------------------------------------------*/
/*                          Exported Functions                                  */
/*------------------------------------------------------------*/
HTTP_T *RAPI_Init( void );
void RAPI_Reset( HTTP_T *pHttp );
void RAPI_Cleanup( HTTP_T *pHttp );

s32 RAPI_Get( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len );
s32 RAPI_Put( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len );
s32 RAPI_Post( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len );
s32 RAPI_Delete( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len );
s32 RAPI_Head( HTTP_T *pHttp, s8 *ps8Url, s8 **pps8Resp, s32 *ps32Len );

#define RAPI_SetParamsInt( http, key, value )     \
             RAPI_SetParams( http, RAPI_PARA_INT, ( s8* )key, NULL, value )
    
#define RAPI_SetParamsStr( http, key, value )     \
             RAPI_SetParams( http, RAPI_PARA_STR, ( s8* )key, ( s8* )value, 0 )


s32 RAPI_SetParams( HTTP_T *pHttp, RAPI_PARA_E s8Type, s8 *ps8Key, s8 *ps8Value, s32 s32Value );
s32 RAPI_SetHeaders( HTTP_T *pHttp, s8 *ps8Key, s8 *ps8Value );
s32 RAPI_SetContents( HTTP_T *pHttp, s8 *ps8Str, size_t sLen );
s32 RAPI_SetContentsLimit2K( HTTP_T *pHttp, const s8 *fmt, ... );
s32 RAPI_SetCookies( HTTP_T *pHttp, s8 *ps8Key, s8 *ps8Value );

s32 RAPI_GetOpt( HTTP_T *pHttp, RAPI_MSG_E eMsg, sptr_t sParam );
s32 RAPI_SetOpt( HTTP_T *pHttp, RAPI_MSG_E eMsg, sptr_t sParam );


//wjson
#define WJSON_BUFF_SIZE  (4096U)
#define WJSON_NAME_SIZE  (64U) 

typedef enum {
	WJSON_TYPE_INT ,
    WJSON_TYPE_STRING,
    WJSON_TYPE_OBJECT,
    WJSON_TYPE_ARRAY ,

	WJSON_TYPE_MAX,
} wjson_type_e ;
#define WJSON_TYPE_ERROR WJSON_TYPE_MAX 

typedef void * wjson ;
typedef int (*wjson_iterator)(wjson wj, void * arg1, void * arg2 ) ;

wjson wjson_new_array(void);
wjson wjson_new_object(void);
void  wjson_free(wjson wj);

wjson wjson_add_int    (wjson wj, const char * name, int value);
wjson wjson_add_string (wjson wj, const char * name, char * string);
wjson wjson_add_object (wjson wj, const char * name ); 
wjson wjson_add_array  (wjson wj, const char * name );
int   wjson_for_each( wjson pwj, wjson_iterator iterator, void * arg1, void * arg2 ) ;

wjson_type_e wjson_get_type(wjson wj);
char* wjson_get_name(wjson wj  );
int wjson_get_int(wjson wj, int def );
char* wjson_get_string(wjson wj, char* def );

wjson wjson_lookup_by_name(wjson pwj, const char * name);

char * wjson_lookup_string_by_name(wjson pwj, const char *name, char * def);
int    wjson_lookup_number_by_name(wjson pwj, const char *name, int def);


struct buffer * wjson_to_buffer( wjson _wj ) ;
char *wjson_string( wjson _wj );
wjson wjson_from_string(char * string, u32 length);

//sha1sum
void SHA1_buffer( u8 *pu8Buf, size_t sInLen, u8 *pu8Sha1, size_t *psOutLen );
void SHA1_file( char *pFileName, u8 *pu8Sha1, size_t *psOutLen );
void SHA1_block( FILE *pFile, size_t size, u8 *pu8Sha1, size_t *psOutLen );

//md5sum
void MD5_buffer( u8 *pu8Buf, size_t sInLen, u8 *pu8Md5, size_t *psOutLen  );

//hmacsha1
void HmacSha1( u8 *pu8Data, size_t sDataLen, 
				u8 *pu8Key, size_t sKeyLen,
				u8*pu8Digest, size_t *psOutLen  );

//base64
int Base64encode_len( int len );
int Base64encode( char * coded_dst, const char *plain_src,int len_plain_src );
int Base64decode_len(const char * coded_src);
int Base64decode( char * plain_dst, const char *coded_src );

//timestamp
char* get_timestamp( void );

#ifdef __cplusplus
 }       
#endif /*end __cplusplus */
         
#endif /*end _APX_PROTO_BASE_H_ */       
         
/** @} */
 
