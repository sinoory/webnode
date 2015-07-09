/**
 *  Copyright APPEX, 2015, 
 *  <HFTS-CLIENT>
 *  <xyfeng>
 *
 *  @defgroup <configure>
 *  @ingroup  <hfts-client>
*
*	<为下载模块提供文件控制接口>
*
 *  @{
 */
 
#ifndef APX_FILE_H_20150210
#define APX_FILE_H_20150210
         
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
#include <stddef.h>

/*---Include Local.h File---*/
#include "../include/apx_list.h"
#include "../include/apx_type.h"
        
#ifdef __cplusplus
extern "C" {
#endif /*end __cplusplus */
        
/*------------------------------------------------------------*/
/*                          Macros Defines                                      */
/*------------------------------------------------------------*/
#define	APX_FILE_BLK_FULL		( -0x1A1B1C1D )       /** block写满了 */
#define	APX_FILE_FULL			( -0x2A2B2C2D )       /** 文件下载完了*/

/*------------------------------------------------------------*/
/*                    Exported Variables                                        */
/*------------------------------------------------------------*/
        
        
/*------------------------------------------------------------*/
/*                         Data Struct Define                                      */
/*------------------------------------------------------------*/
typedef	void	FILE_T;

/** 提供给创建文件的返回码*/
typedef	enum	_apx_file_e_
{
	APX_FILE_SUCESS = 0,
	APX_FILE_DOWNLOADING = -5,	/** 文件未下载完成*/
	APX_FILE_DOWNLOADED,	/** 文件已下载完成*/
	APX_FILE_NO_MEM,		/** 内存不足*/
	APX_FILE_PARAM_ERR, /** 参数错误*/
	APX_FILE_UNKOWN, /** 参数错误*/
	
	APX_FILE_MAX
}apx_file_e;

/** 文件数据块*/
typedef	struct	_apx_fblock_s_
{
	u64	u64Start;		/** 此文件块的开始字节偏移 */
	u64	u64End;			/** 结束字节偏移*/
	u64	u64Offset;		/** 相对于开始字节的偏移 */
	s32	s32Idx;
	u8	sign[41];
	char padding[8];
}apx_fblk_st;
        
/*------------------------------------------------------------*/
/*                          Exported Functions                                  */
/*------------------------------------------------------------*/
/**
	@brief	apx_file_init
 		文件管理模块初始化
 
	@param[in]	 void	
	@return
		( void  )
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
void apx_file_init(void );

/**
	@brief	apx_file_exit
 		文件管理模块资源回收
 
	@param[in]	 void	
	@return
		( void  )
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
void apx_file_exit(void );


/**
	@brief	apx_file_create
			新建文件
 
	@param[in]	 pUrl			url
	@param[in]	 pFName		文件名称,绝对路径
	@param[in]	 u64FSize		文件大小
	@param[in]	 s32BlkCnt	分块个数
	@param[out]	 pError		错误码，见axp_file_e
	@return
		pError = APX_FILE_SUCESS or APX_FILE_DOWNLOADING，
						返回文件信息结构
		pError = APX_FILE_DOWNLOADED or  APX_FILE_NO_MEM 
					or APX_FILE_PARAM_ERR, 返回NULL
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
FILE_T* apx_file_create( s8 *ps8Url, s8 *ps8FName, u64 u64FSize,
							s32 s32BlkCnt, s32 *ps32Err, u32 u32Upload );

/**
	@brief	apx_file_reset
		重置文件
 
	@param[in]	 pFileInfo 	文件信息结构
	@return
		return 0 if success, else return -1
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
int apx_file_reset( FILE_T *pFileInfo );

/**
	@brief	apx_file_release
 		释放文件
 
	@param[in]	 pFileInfo		文件信息结构	
	@return
		( void  )
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
void apx_file_release( FILE_T *pFileInfo );

/**
	@brief	apx_file_destroy
 		重置文件
 
	@param[in]	 pFileInfo		文件信息结构
	@return
		( void  )
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
void apx_file_destroy( FILE_T *pFileInfo);

/**
	@brief	apx_file_read
 		读取文件数据
 
	@param[in]	 pBuf		文件数据
	@param[in]	 szSize		读取的数据大小
	@param[in]	 s32Idx		数据所对应的数据块索引
	@param[in]	 pFileInfo 	文件信息结构
	@return
		return size of buf if success, other return -1
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
ssize_t apx_file_read( void *pBuf, size_t szSize, s32 s32Idx, FILE_T *pFileInfo );

/**
	@brief	apx_file_write
 		写入文件数据
 
	@param[in]	 pBuf	文件数据
	@param[in]	 szSize	写入的数据大小
	@param[in]	 s32Idx	数据所对应的数据块索引
	@param[in]	 pFileInfo 文件信息结构	
	@return
		return size of buf if success, 
		return APX_FILE_BLK_FULL, if block is full
		return APX_FILE_FULL, if file is full
		other return -1
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
ssize_t apx_file_write( void *pBuf, size_t szSize, s32 s32Idx, FILE_T *pFileInfo );

/**
	@brief	apx_file_size
 		文件大小
 
	@param[in]	 pFileInfo 	文件信息结构
	@return
		文件大小
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
u64 apx_file_size( FILE_T *pFileInfo );

/**
	@brief	apx_file_cur_size
 		已下载文件的文件大小
 
	@param[in]	 pFileInfo 	文件信息结构
	@return
		已下载文件的文件大小
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
u64 apx_file_cur_size( FILE_T *pFileInfo );

/**
	@brief	apx_file_pause
 		暂停下载，关闭打开的文件
 
	@param[in]	 pFileInfo 	文件信息结构
	@return
		( void  )
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
void apx_file_pause( FILE_T *pFileInfo );

/**
	@brief	apx_file_resume
 		恢复下载，重新打开文件
 
	@param[in]	 pFileInfo 	文件信息结构
	@return
		( void  )
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
void apx_file_resume( FILE_T *pFileInfo );

/**
	@brief	apx_file_blk_info
 		获取未下载完的block
	
	@param[in]	 pFileInfo 	文件信息结构
	@return
		返回未下载完的数据块
		NULL表示没有未下载完的block
	history
		       Author                Date              Modification
		   ----------       ----------       ------------
		      xyfeng     	  	2015-3-19        Initial Draft 
*/
apx_fblk_st* apx_file_blk_info( FILE_T *pFileInfo );
void apx_file_blk_reset( FILE_T *pFileInfo, apx_fblk_st *pstblk );
s32 apx_file_blk_cnt( FILE_T *pFileInfo );
s32 apx_file_is_exist( s8 *path, off_t *pSize );
s32 apx_file_mkdir( s8* path );
s32 apx_file_divide_cnt( u64 u64Size, u32 upload );
void* apx_block_point( FILE_T *pFileInfo );
void apx_block_reset( FILE_T *pFileInfo );
void apx_block_set_status( void* pBlk, int index, int status );
void apx_block_set_sign( void* pBlk, int index, char* pSign );
void apx_file_set_cur_size( FILE_T *pFileInfo, u64 u64Size );
void apx_file_update_upsize( FILE_T *pFileInfo );

#ifdef __cplusplus
 }       
#endif /*end __cplusplus */
         
#endif /*end APX_FILE_H_20150210 */       
/** @} */
 
