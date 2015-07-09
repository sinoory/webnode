/*-----------------------------------------------------------
*      Copyright (c)  AppexNetworks, All Rights Reserved.
*
*FileName:     apx_file.c 
*
*Description:  文件控制模块
* 
*History: 
*      Author              Date        	Modification 
*  ----------      ----------  	----------
* 	xyfeng   		2015-3-19     	Initial Draft 
*
*------------------------------------------------------------*/
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
/*---Include ANSI C .h File---*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

/*---Include Local.h File---*/
#include "apx_file.h"

/*------------------------------------------------------------*/
/*                          Private Macros Defines                          */
/*------------------------------------------------------------*/
#define	BLOCK_MAX			( 256 )
#define	FILE_1K				( 1024 )
#define	FILE_1M				( 1024 * FILE_1K )
#define	FILE_10M			( 10 * FILE_1M )
#define	FILE_100M			( 10 * FILE_10M )
#define	FILE_1G				( 1024 * FILE_1M )

#define	FILE_TMP_SUFFIX		".tmp"/** 临时文件 */

/**
* 记录数据块信息的文件 
* 文件格式:
*	finfo_st + name[1024] + url[1024] + blkCnt * fblk_st
*/
#define	FILE_INFO_SUFFIX		".info"

#define	FILE_LEN_MAX		( 1024 )

#define	free_func( _ptr_ )	\
do {							\
	if( _ptr_ != NULL ) {		\
		free( _ptr_ );			\
		_ptr_ = NULL;		\
	}						\
}							\
while(0)
	
#define fblock_free( _ptr_ )	free_func( _ptr_ )
#define finfo_free( _ptr_ )		free_func( _ptr_ )

#define HF_FILE_DEBUG

#undef flog
#ifdef HF_FILE_DEBUG

#define    flog( fmt, args... )     \
        do { \
            fprintf( stderr, "[%03d-%s] "fmt"\n", __LINE__, __FUNCTION__, ##args ); \
        }while(0)
        
#else
#define    flog( fmt, args... )
#endif


/*------------------------------------------------------------*/
/*                          Private Type Defines                            */
/*------------------------------------------------------------*/

/** 文件数据块状态*/
typedef	enum	_blk_state_e_
{
	APX_BLK_EMPTY = 0,	/** 未下载 */
	APX_BLK_FULL,		/** 已下载 */
	APX_BLK_HOLD,		/** 正在下载 */
	APX_BLK_ALLOCED,		/** 已分配，还未开始下载 */
	APX_BLK_UNUSED,
	
	APX_BLK_MAX
}blk_state_e;

/** 文件数据块*/
typedef	struct	_apx_fblk_s_
{
	u64	u64Start;		/** 此文件块的开始字节偏移 */
	u64	u64End;			/** 结束字节偏移*/
	u64	u64Offset;		/** 相对于开始字节的偏移 */
	s32	s32Idx;
	u8	sign[41];
	s32	s32Fd;			/** 文件句柄 */
	blk_state_e	eState;	/** 文件数据块状态 */
//	struct list_head	stList;
}fblk_st;

/** 文件信息 ，对应一个文件 */
typedef	struct _apx_fileInfo_s_
{
	s8	*ps8FName;	/** <file>.tmp( 临时文件 )  <file>.info( 文件信息 ) */
	s8	*ps8Url;		/** url */
	s8	*ps8BlkMap;/** 数据块文件内存映射 */
	u32	u32FNameMagic;
	u32	u32UrlMagic;
	u32	u32Upload;	/** 1: upload */
	s32	s32Pause;
	u64	u64Size;	/** 文件大小 */
	u64	u64CurSize;	/** 已下载文件大小 */
	u64	u64UpSize;	/** 下次更新blk信息时的文件大小 */
	s32	s32BlkCnt;	/** 数据块个数 */
//	struct list_head stBlkHead;	// 文件数据块头指针 
	struct list_head	stList;/** 文件信息链表*/
	pthread_mutex_t	mBlkLock;/** 数据块互斥锁*/
	pthread_mutex_t	mInfoLock;/** .info文件更新 互斥锁*/
	fblk_st*	pstBlkInfo;/**数据块信息*/
}finfo_st;


typedef struct _file_head_t_
{
	s32	s32Cnt;
	struct list_head stHeadList;
	pthread_rwlock_t rwLock;/** 文件信息链表读写锁 */
}file_head_st;       
        
/*------------------------------------------------------------*/
/*                         Global Variables                                */
/*------------------------------------------------------------*/
/*---Extern Variables---*/
        
/*---Local Variables---*/
static s32 g_file_init;
static file_head_st g_file_head = {	.s32Cnt = 0,
							.stHeadList = LIST_HEAD_INIT( g_file_head.stHeadList ),
							.rwLock = PTHREAD_RWLOCK_INITIALIZER
							//.mInfoLock = PTHREAD_MUTEX_INITIALIZER
						};

/*------------------------------------------------------------*/
/*                          Local Function Prototypes                       */
/*------------------------------------------------------------*/
static u32 __string_magic( s8 *ps8Str );
static s32 __is_file_exist( s8 *ps8File, off_t *pSize );
static fblk_st* fblock_alloc( size_t nMemb  );
static void fblock_check_cnt( finfo_st *pstFileInfo );
static s32 fblock_new( finfo_st *pstFileInfo );
static void fblock_release( finfo_st *pstFileInfo );
static finfo_st* finfo_alloc( void  );
static finfo_st* finfo_new( s8 *ps8Url, s8 *ps8FName, u64 u64FSize );
static void finfo_release( finfo_st *pstFileInfo );
static finfo_st* __lookup_file_list( s8 *ps8Url, s8 *ps8FName );
static s32 __ftmp_create( finfo_st *pstFileInfo );
static s32 __fblock_open( fblk_st *pstBlkInfo, s8* ps8FileName, u32 upload );
static s32 __finfo_map( finfo_st *pstFileInfo, size_t sSize );
static void __finfo_unmap( finfo_st *pstFileInfo );
static s32 __finfo_parse( finfo_st *pstFileInfo, size_t sSize );
static void __finfo_update( finfo_st *pstFileInfo, s8 first );
static s32 __finfo_create( finfo_st *pstFileInfo );
static void __file_check_fname( finfo_st *pstFileInfo );
static s32 __file_create( finfo_st *pstFileInfo );
static s32 finfo_reload( finfo_st *pstFileInfo );
void apx_file_delete( s8* pFileName );

/*------------------------------------------------------------*/
/*                        Functions                                               */
/*------------------------------------------------------------*/

/**maigc num*/
#define MAGIC_SIZE        24 
/*---------------------------------------------------------
*Function:    __string_magic
*Description:   
*           计算字符串hash值，快速查找
*Parameters: 
*	ps8Str[in]	字符串	
*Return:
*       字符串hash值
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static u32 __string_magic( s8 *ps8Str )
{
	u32 magic = 0;
	s32 pos;
	s8 inbuf[MAGIC_SIZE];

	if( NULL == ps8Str )
	{
		return 0;
	}
	
	memset( inbuf, 0, sizeof( inbuf ) );
	strncpy( ( char* )inbuf, ( char* )ps8Str, sizeof( inbuf ) );
	for( pos = 0; pos < MAGIC_SIZE; pos++ )
	{
		magic += ( u32 )( u8 )inbuf[pos];
		magic <<= 1;
	}
	return magic;
}

/*---------------------------------------------------------
*Function:    __is_file_exist
*Description:   
*           判断文件是否存在，并返回文件大小
*Parameters: 
*	ps8File[in]	文件名称
*	pSize[out]	文件大小
*Return:
*       return 1 if  file exist, other return 0.
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static s32 __is_file_exist( s8 *ps8File, off_t *pSize )
{
	s32 s32Ret = 0;
	struct stat stStat;

	s32Ret = stat( ( char* )ps8File, &stStat );
	if( pSize != NULL )
	{
		*pSize = stStat.st_size;
	}

	return ( 0 == s32Ret && S_ISREG( stStat.st_mode ) );
}

/*---------------------------------------------------------
*Function:    fblock_alloc
*Description:   
*           分配文件数据块结构
*Parameters: 
*	nMemb	block个数
*Return:
*      申请成功返回文件数据块结构
*	否则返回NULL
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static fblk_st* fblock_alloc( size_t nMemb  )
{
	fblk_st *pstBlk = NULL;

	pstBlk = calloc( nMemb, sizeof( fblk_st ) );
	return ( NULL == pstBlk ) ? NULL : pstBlk;
}

/*-------------------------------------------------
*Function:    apx_file_divide_cnt
*Description:   
*           根据文件大小计算文件分块个数
*Parameters: 
*	u64Size[IN]	文件大小
*	upload[IN]	是否上传
*Return:
*       s32 :	返回文件分块数
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
s32 apx_file_divide_cnt( u64 u64Size, u32 upload )
{
	s32 s32BlkCnt = 1;
	u64 divide = 0;

	divide = u64Size / FILE_1M;
	if( 0 == u64Size )
	{
		s32BlkCnt = 1;
	}
	else if( 0 == divide )/** < 1M */
	{
		s32BlkCnt = ( upload ) ? 1 : 2;
	}
	else if( divide < 5 )/** <=5M */
	{
		s32BlkCnt = ( upload ) ? 1 : 8;
	}
	else if( divide < 11 )/** <=10M */
	{
		s32BlkCnt = ( upload ) ? 2 : 12;
	}
	else if( divide < 51 )/** <=50M */
	{
		s32BlkCnt = ( upload ) ? 5 : 60;
	}
	else if( divide < 101 )/** <=100M */
	{
		s32BlkCnt = ( upload ) ? 10 : 120;
	}
	else if( divide < 501 )/** <= 500M */
	{
		s32BlkCnt = ( upload ) ? 50 : 500;
	}
	else if( divide < 1025 )/** <=1024M 1G */
	{
		s32BlkCnt = ( upload ) ? 100 : 1000;
	}
	else
	{
		s32BlkCnt = ( upload ) ? 150 :1500;
	}	

	return s32BlkCnt;
}

/*---------------------------------------------------------
*Function:    fblock_check_cnt
*Description:   
*      设置block个数。
*	如果外部没有指定block个数,此处会自动分块
*Parameters: 
*	pstFileInfo	文件信息结构
*Return:
*       void
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static void fblock_check_cnt( finfo_st *pstFileInfo )
{	
	if( pstFileInfo->s32BlkCnt > 0 )
	{
		return;
	}

	pstFileInfo->s32BlkCnt = apx_file_divide_cnt( pstFileInfo->u64Size,
											pstFileInfo->u32Upload );
	return;
}

/*---------------------------------------------------------
*Function:    fblock_new
*Description:   
*           初始化block
*Parameters: 
*	pstFileInfo	文件信息结构
*Return:
*       return 0 if success, else return -1
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static s32 fblock_new( finfo_st *pstFileInfo )
{
	s32 i = 0;
	u32 s32BlkSize = 0;
	fblk_st *pstBlk = NULL;
			
	fblock_check_cnt( pstFileInfo );
	pstBlk = fblock_alloc( pstFileInfo->s32BlkCnt );
	if( NULL == pstBlk )
	{
		return -1;
	}

	s32BlkSize = pstFileInfo->u64Size / pstFileInfo->s32BlkCnt;
	for( i = 0; i < pstFileInfo->s32BlkCnt; i++ )
	{
		pstBlk[i].s32Fd= -1;
		pstBlk[i].s32Idx = i;
		pstBlk[i].u64Start = i * s32BlkSize;
		pstBlk[i].u64End =( s32BlkSize > 0 ) ? ( pstBlk[i].u64Start + s32BlkSize -1 ) : 0;//( i + 1 )* s32BlkSize -1;
	}

	pstBlk[i - 1].u64End = ( pstFileInfo->u64Size > 0 ) ? pstFileInfo->u64Size -1 : 0;
	pstFileInfo->pstBlkInfo = pstBlk;
	return 0;
}

/*-------------------------------------------------
*Function:    fblock_close
*Description:   
*           关闭分块打开的文件
*Parameters: 
*	pstFileInfo[IN]		文件信息
*Return:
*       static void :
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
static void fblock_close( finfo_st *pstFileInfo )
{
	s32 k = 0;
	fblk_st *pstBlk = pstFileInfo->pstBlkInfo;
	
	if( pstBlk != NULL )
	{
		//pthread_mutex_lock( &pstFileInfo->mBlkLock );
		for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
		{
			if( pstBlk[k].s32Fd != -1 )
			{
				close( pstBlk[k].s32Fd );
				pstBlk[k].s32Fd = -1;
			}
		}
		//pthread_mutex_unlock( &pstFileInfo->mBlkLock );
	}
	return;
}

static void fblock_release( finfo_st *pstFileInfo )
{
	fblock_close( pstFileInfo );
	fblock_free( pstFileInfo->pstBlkInfo );
	return;
}


/** 分配文件信息结构 */
/*---------------------------------------------------------
*Function:    finfo_alloc
*Description:   
*           分配文件信息结构内存
*Parameters: 
*	 ( void )	
*Return:
*       return point to struct finfo_st if alloc success, else return NULL
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static finfo_st* finfo_alloc( void  )
{
	finfo_st *pstFInfo = NULL;

	pstFInfo = calloc( 1, sizeof( finfo_st ) );
	return ( NULL == pstFInfo ) ? NULL : pstFInfo;
}

/*---------------------------------------------------------
*Function:    finfo_new
*Description:   
*           分配文件信息结构，并初始化
*Parameters: 
*	ps8Url[in]	url
*	ps8FName[in]	文件名称
*	u64FSize[in]	文件大小
*Return:
*       return point to struct finfo_st if alloc success, else return NULL
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static finfo_st* finfo_new( s8 *ps8Url, s8 *ps8FName, u64 u64FSize )
{
	finfo_st *pstFileInfo = NULL;
	
	pstFileInfo = finfo_alloc();
	if( NULL == pstFileInfo )
	{
		return NULL;
	}
	
	pstFileInfo->ps8FName = ( s8* )strdup( ( char* )ps8FName );
	pstFileInfo->ps8Url= ( s8* )strdup( ( char* )ps8Url );
	pstFileInfo->u32FNameMagic= __string_magic( ps8FName );
	pstFileInfo->u32UrlMagic= __string_magic( ps8Url );
	pstFileInfo->u64Size= u64FSize; 
	pstFileInfo->s32Pause = 0;
	pstFileInfo->u32Upload = 0;
	pthread_mutex_init( &pstFileInfo->mBlkLock, NULL );
	pthread_mutex_init( &pstFileInfo->mInfoLock, NULL );
	
	//pstFileInfo->stBlkHead= LIST_HEAD_INIT( pstFileInfo->stBlkHead );

	return pstFileInfo;
}

static void finfo_release( finfo_st *pstFileInfo )
{

	if( !list_empty(& g_file_head.stHeadList ) )
	{
		if( !list_empty( &pstFileInfo->stList ) )
		{
			list_del_init( &pstFileInfo->stList );
			g_file_head.s32Cnt--;
		}
	}
	
	__finfo_unmap( pstFileInfo );
	if( pstFileInfo->ps8FName != NULL )
	{
		free( pstFileInfo->ps8FName );
		pstFileInfo->ps8FName = NULL;
	}

	if( pstFileInfo->ps8Url != NULL )
	{
		free( pstFileInfo->ps8Url );
		pstFileInfo->ps8Url = NULL;
	}
	
	fblock_release( pstFileInfo );
	pthread_mutex_destroy( &pstFileInfo->mBlkLock );
	pthread_mutex_destroy( &pstFileInfo->mInfoLock );

	finfo_free( pstFileInfo );
	
	return;
}

/*---------------------------------------------------------
*Function:    __lookup_file_list
*Description:   
*           查找文件是否在下载列表中 
*Parameters: 
*	ps8Url[in]	url
*	ps8FName[in]	文件名称
*Return:
*       return point to struct finfo_st if found, else return -1;
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static finfo_st* __lookup_file_list( s8 *ps8Url, s8 *ps8FName )
{
	u32 u32UrlMagic = 0,
		u32FileMagic = 0;
	finfo_st	*pstFileInfo = NULL;
	
	/** calc migic, 查找正在下载列表 */
	u32UrlMagic = __string_magic( ps8Url );
	u32FileMagic = __string_magic( ps8FName );
	
	pthread_rwlock_rdlock( &g_file_head.rwLock );
	list_for_each_entry( pstFileInfo, &g_file_head.stHeadList, stList )
	{
		if( u32FileMagic == pstFileInfo->u32FNameMagic &&
			u32UrlMagic == pstFileInfo->u32UrlMagic &&
			0 == strcmp( ( char* )pstFileInfo->ps8FName, ( char* )ps8FName ) &&
			0 == strcmp( ( char* )pstFileInfo->ps8Url, ( char* )ps8Url ) )
		{
			pthread_rwlock_unlock( &g_file_head.rwLock );
			return pstFileInfo;
		}
	}
	pthread_rwlock_unlock( &g_file_head.rwLock );

	return NULL;
}

static s32 __lookup_list( finfo_st *pstFileInfo )
{
	finfo_st	*pstPos = NULL;
	
	pthread_rwlock_wrlock( &g_file_head.rwLock );
	{
		list_for_each_entry( pstPos, &g_file_head.stHeadList, stList )
		{
			if( pstPos == pstFileInfo )
			{
				pthread_rwlock_unlock( &g_file_head.rwLock );
				return 1;
			}
		}
	}
	pthread_rwlock_unlock( &g_file_head.rwLock );
	return 0;
}

/*-------------------------------------------------
*Function:    __ftmp_create
*Description:   
*           创建临时文件
*Parameters: 
*	pstFileInfo[IN]		文件信息结构
*Return:
*      return 0 if success, else return negative
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
static s32 __ftmp_create( finfo_st *pstFileInfo )
{
	s32 s32Fd;
	s32 s32Bytes = 0;
	off_t off = -1;
	ssize_t sSize = -1;
	s8 s8Name[FILE_LEN_MAX] = {0};
	
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	s32Fd = open( ( char* )s8Name, O_WRONLY|O_CREAT|O_TRUNC, 0644 );
	if( s32Fd < 0 )
	{
		return -1;
	}

	if( 0 == pstFileInfo->u64Size )
	{
		goto file_start;
	}
	/** set file size */
	off = lseek( s32Fd, pstFileInfo->u64Size - sizeof( s32Bytes ), SEEK_SET );
	if( off < 0 )
	{
		close( s32Fd );
		return -2;
	}
	sSize = write( s32Fd, &s32Bytes, sizeof( s32Bytes ) );
	if( sSize < 0 || sSize != sizeof( s32Bytes ) )
	{
		close( s32Fd );
		return -3;
	}

file_start:	
	/** used by first block */
	off = lseek( s32Fd, 0, SEEK_SET );
	if( off < 0 )
	{
		close( s32Fd );
		return -4;
	}
	pstFileInfo->pstBlkInfo[0].s32Fd = s32Fd;

	return 0;
}

/*-------------------------------------------------
*Function:    __fblock_open
*Description:   
*           打开分块对应的文件
*Parameters: 
*	pstBlkInfo[IN]		分块信息
*	ps8FileName[IN]	文件名
*	upload[IN]		是否上传
*Return:
*      return file descriptor if success, else return negative
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
static s32 __fblock_open( fblk_st *pstBlkInfo, s8* ps8FileName, u32 upload )
{
	off_t off = -1;
	mode_t mode = O_WRONLY;
	
	if( pstBlkInfo->s32Fd < 0 )
	{
		s32 s32Fd = -1;
		s8 s8Name[FILE_LEN_MAX] = {0};

		if( !upload )
		{
			mode = O_WRONLY;
			snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, ps8FileName );
		}
		else
		{
			mode = O_RDONLY;
			strncpy( ( char* )s8Name, ps8FileName, sizeof( s8Name ) );
		}
		
		s32Fd = open( ( char* )s8Name, mode );
		if( s32Fd < 0 )
		{
			flog( "%d, %s: %s", s32Fd, s8Name, strerror( errno ) );
			return -1;
		}

		pstBlkInfo->s32Fd = s32Fd;
		off = lseek( s32Fd, pstBlkInfo->u64Start + pstBlkInfo->u64Offset, SEEK_SET );
		if( off < 0 )
		{
			flog( "%s: %s", s8Name, strerror( errno ) );
			close( s32Fd );
			return -2;
		}
	}

	return pstBlkInfo->s32Fd;
}


static s32 __finfo_map( finfo_st *pstFileInfo, size_t sSize )
{
 	s32 s32Fd = -1;
 	s32 s32Bytes = 0;

	s8 *ps8Map = NULL;
	size_t sLength = sSize;
	off_t off = -1;
	ssize_t sRet = -1;
 	s8 s8Name[FILE_LEN_MAX] = {0};
	
 	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
 	s32Fd = open( ( char* )s8Name, O_RDWR|O_CREAT, 0644 );
 	if( s32Fd < 0 )
 	{
		flog( " open failed( %s).", strerror( errno ) );
 		return -1;
 	}

	if( 0 == sLength )
	{/** new file */
		sLength = sizeof( finfo_st ) + 2 * FILE_LEN_MAX + 
				sizeof( fblk_st ) * pstFileInfo->s32BlkCnt;
		
		off = lseek( s32Fd, sLength, SEEK_SET );
		if( off < 0 )
		{
			close( s32Fd );
			flog( " lseek failed( %s).", strerror( errno ) );
			return -2;
		}
		sRet = write( s32Fd, &s32Bytes, sizeof( s32Bytes ) );
		if( sRet < 0 || sRet != sizeof( s32Bytes ) )
		{
			close( s32Fd );
			flog( " write failed( %s).", strerror( errno ) );
			return -3;
		}
	}
	off = lseek( s32Fd, 0, SEEK_SET );
	if( off < 0 )
	{
		close( s32Fd );
		flog( " lseek failed( %s).", strerror( errno ) );
		return -4;
	}

 	ps8Map = mmap( NULL, sLength, PROT_READ|PROT_WRITE, MAP_SHARED, s32Fd, 0 );
 	if( MAP_FAILED == ps8Map )
	{
		close( s32Fd );
		flog( " mmap failed( %s).", strerror( errno ) );
		return -5;
	}
	close( s32Fd );
	pstFileInfo->ps8BlkMap = ps8Map;

	return 0;
}
static void __finfo_unmap( finfo_st *pstFileInfo )
{
 	s32 s32Bytes = 0;
	off_t	tInfoSize;
 	s8 s8Name[FILE_LEN_MAX] = {0};

	if( pstFileInfo->ps8BlkMap != NULL )
	{
		snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
		__is_file_exist( s8Name, &tInfoSize );
	
		munmap( pstFileInfo->ps8BlkMap, tInfoSize - sizeof( s32Bytes ) );
		pstFileInfo->ps8BlkMap = NULL;
	}
}

/*---------------------------------------------------------
*Function:    __finfo_parse
*Description:   
*           解析.info 文件
*Parameters: 
*	pstFileInfo[in]	文件信息结构
*	sSize		.info 文件大小
*Return:
*       return 0 if parse success, else return -1
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static s32 __finfo_parse( finfo_st *pstFileInfo, size_t sSize )
{
	s32 k = 0,
		s32BlkCnt = 0;
	size_t sInfoLen = sizeof( finfo_st ),
		   sBlkLen = sizeof( fblk_st );
	
	s8 *ps8BlkMap = pstFileInfo->ps8BlkMap;
	s8 s8Name[FILE_LEN_MAX] = {0};
	
	fblk_st *pstBlk = NULL;
	fblk_st stBlkTmp;
	finfo_st stFileInfo;

	if( sSize < sInfoLen )
	{
		return -1;
	}
	
	stFileInfo = *( finfo_st* )ps8BlkMap;
	if( stFileInfo.u64Size != pstFileInfo->u64Size ||
		stFileInfo.u32FNameMagic != pstFileInfo->u32FNameMagic ||
		stFileInfo.u32UrlMagic != pstFileInfo->u32UrlMagic )
	{
		return -2;
	}
	memset( s8Name, 0, sizeof( s8Name ) );
	memcpy( s8Name, &ps8BlkMap[sInfoLen], FILE_LEN_MAX );
	s8Name[FILE_LEN_MAX - 1] = 0;
	if( strcmp( ( char* )pstFileInfo->ps8FName, ( char* )s8Name ) != 0 )
	{
		return -3;
	}
	
	sInfoLen += FILE_LEN_MAX;
	memset( s8Name, 0, sizeof( s8Name ) );
	memcpy( s8Name, &ps8BlkMap[sInfoLen], FILE_LEN_MAX );
	s8Name[FILE_LEN_MAX - 1] = 0;
	if( strcmp( ( char* )pstFileInfo->ps8Url, ( char* )s8Name ) != 0 )
	{
		return -4;
	}

	pstFileInfo->s32Pause = 0;
	pstFileInfo->u64CurSize = stFileInfo.u64CurSize;
	pstFileInfo->s32BlkCnt = ( 0 == stFileInfo.s32BlkCnt ) ? 1 : stFileInfo.s32BlkCnt;
	pstFileInfo->u64UpSize = pstFileInfo->u64CurSize + FILE_1M;
	pstBlk = fblock_alloc( pstFileInfo->s32BlkCnt );
	if( NULL == pstBlk )
	{
		return -5;
	}
	
	s32BlkCnt = 0;
	sInfoLen += FILE_LEN_MAX;
	for( k = 0; k< pstFileInfo->s32BlkCnt; k++ )
	{
		stBlkTmp = *( fblk_st* )&ps8BlkMap[sInfoLen + k * sBlkLen];
		if( APX_BLK_EMPTY == stBlkTmp.eState ||
			APX_BLK_HOLD == stBlkTmp.eState ||
			APX_BLK_ALLOCED == stBlkTmp.eState  )
		{
			stBlkTmp.eState = APX_BLK_EMPTY;
			pstBlk[s32BlkCnt] = stBlkTmp;
			pstBlk[s32BlkCnt].s32Idx = s32BlkCnt;
			pstBlk[s32BlkCnt].s32Fd = -1;
			s32BlkCnt++;
		}
	}
	pstFileInfo->s32BlkCnt = s32BlkCnt;
	pstFileInfo->pstBlkInfo = pstBlk;

	return 0;
}

/** finfo_st + fileName + url + fblk_st * n */
static void __finfo_update( finfo_st *pstFileInfo, s8 first )
{
	s32 k = 0,
		s32BlkCnt = 0;
	size_t sInfoLen = sizeof( finfo_st ),
		   sBlkLen = sizeof( fblk_st );
	s8 *ps8BlkMap = pstFileInfo->ps8BlkMap;

	if( NULL == ps8BlkMap )
	{
		return;
	}
	
	*( finfo_st* )ps8BlkMap = *pstFileInfo;
	if( first )
	{
		memcpy( &ps8BlkMap[sInfoLen], 
				pstFileInfo->ps8FName, 
				strlen( ( char* )pstFileInfo->ps8FName ) + 1 );
	}
	
	sInfoLen += FILE_LEN_MAX;
	if( first )
	{
		memcpy( &ps8BlkMap[sInfoLen],
				pstFileInfo->ps8Url,
				strlen( ( char* )pstFileInfo->ps8Url ) + 1 );
	}
	
	s32BlkCnt = 0;
	sInfoLen += FILE_LEN_MAX;
	for( k = 0; k < pstFileInfo->s32BlkCnt; k++ )
	{
		if( APX_BLK_EMPTY == pstFileInfo->pstBlkInfo[k].eState ||
			APX_BLK_HOLD == pstFileInfo->pstBlkInfo[k].eState ||
			APX_BLK_ALLOCED == pstFileInfo->pstBlkInfo[k].eState  )
		{
			*( fblk_st* )&ps8BlkMap[sInfoLen + s32BlkCnt * sBlkLen] = pstFileInfo->pstBlkInfo[k];
			s32BlkCnt++;
		}
	}
	( ( finfo_st* )ps8BlkMap )->s32Pause = 0;
	( ( finfo_st* )ps8BlkMap )->s32BlkCnt = s32BlkCnt;
	
	return;
}

static s32 __finfo_create( finfo_st *pstFileInfo )
 {
	s32 s32Ret = 0;

	s32Ret = __finfo_map( pstFileInfo, 0 );
	if( s32Ret != 0 )
	{
		flog( " __finfo_map failed( ret = %d).", s32Ret );
		return -1;
	}

	__finfo_update( pstFileInfo, 1 );	
 	return 0;
 }



static void __file_check_fname( finfo_st *pstFileInfo )
{
	s8 *ps8Str = NULL;
	s8 *ps8NewFile = NULL;
	s32 k = 0,
		s32Ret = 0;
	size_t sLen = 0;
	s8 s8Name[FILE_LEN_MAX];

	/** file*/
	sLen = strlen( ( char* )pstFileInfo->ps8FName );
	s32Ret = __is_file_exist( pstFileInfo->ps8FName, NULL );
	s32Ret <<= 1;
	
	/** .tmp file*/
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	s32Ret |= __is_file_exist( s8Name, NULL );
	if( 0 == s32Ret )
	{/** file and <file>.tmp  都不存在*/
		return;
	}	

	/** rename */
	ps8Str = ( s8* )strrchr( ( char* )pstFileInfo->ps8FName, '.' );
	if( ps8Str != NULL &&
		ps8Str != pstFileInfo->ps8FName &&
		( size_t )( ps8Str - pstFileInfo->ps8FName ) != sLen - 1 )
	{
		*ps8Str++ = '\0';
	}

	ps8NewFile = calloc( 1, sLen + 6 );/** (123) */
	if( NULL == ps8NewFile )
	{
		return;
	}

	k = 0;	
	while( k < 100 )
	{
		k++;
		if( ps8Str != NULL )
		{
			snprintf( ( char* )ps8NewFile, sLen + 6 , "%s(%d).%s",
				pstFileInfo->ps8FName, k, ps8Str );
		}
		else
		{
			snprintf( ( char* )ps8NewFile, sLen + 6 , "%s(%d)",
				pstFileInfo->ps8FName, k );

		}
		s32Ret = __is_file_exist( ps8NewFile, NULL );
		s32Ret <<= 1;

		snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, ps8NewFile );
		s32Ret |= __is_file_exist( s8Name, NULL );
		if( 0 == s32Ret )
		{
			break;
		}
	}
	
	free( pstFileInfo->ps8FName );
	pstFileInfo->ps8FName = ps8NewFile;
	pstFileInfo->u32FNameMagic = __string_magic( ps8NewFile );
	
	return;
}

/*-------------------------------------------------
*Function:    __file_create
*Description:   
*           创建.info文件
*Parameters: 
*	pstFileInfo[in]	文件信息结构
*Return:
*       return 0 if .info file created success, else return -1 
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
static s32 __file_create( finfo_st *pstFileInfo )
{
	s32	s32Ret = 0;
	
	__file_check_fname( pstFileInfo );

	/** .tmp file*/
	s32Ret = __ftmp_create( pstFileInfo );
	if( s32Ret != 0 )
	{
		flog( "__ftmp_create failed( ret = %d ).", s32Ret );
		return -1;
	}

	/** .info file*/
	return  __finfo_create( pstFileInfo );
}

static void __finfo_done( finfo_st *pstFileInfo )
{
 	s8 s8Name[FILE_LEN_MAX] = {0};

	
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	rename( ( char* )s8Name, ( char* )pstFileInfo->ps8FName );

	__finfo_unmap( pstFileInfo );
	
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
	apx_file_delete( s8Name );

	return;
}

/*---------------------------------------------------------
*Function:    finfo_reload
*Description:   
*           判断是否存在临时文件，并加载
*Parameters: 
*	pstFileInfo[in]		文件信息结构
*Return:
*       return 0 if file exist & reload success, else return -1
*History:
*      xyfeng     	  2015-3-19        Initial Draft 
*-----------------------------------------------------------*/
static s32 finfo_reload( finfo_st *pstFileInfo )
{
	s32 s32Ret = 0,
 		s32Bytes = 0;
	off_t	tTmpSize = 0,
		tInfoSize = 0;
	s8 s8Name[FILE_LEN_MAX];

	/** .tmp file*/
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	s32Ret = __is_file_exist( s8Name, &tTmpSize );
	s32Ret <<= 1;
	
	/** .info file*/
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
	s32Ret |= __is_file_exist( s8Name, &tInfoSize );
	if( s32Ret ^ 0x3
		|| (  pstFileInfo->u64Size != 0
		&& pstFileInfo->u64Size != ( u64 )tTmpSize )
		||  0 == tInfoSize )
	{
		return -1;
	}

	/** map .info file */
	s32Ret = __finfo_map( pstFileInfo, tInfoSize - sizeof( s32Bytes ) );
	if( s32Ret != 0 )
	{
		return -2;
	}

	/** parse .info to info struct */
	s32Ret = __finfo_parse( pstFileInfo, tInfoSize );
	if( s32Ret != 0 )
	{
		__finfo_unmap( pstFileInfo );
		return -3;
	}
	
	return 0;
}

void apx_file_dump( FILE_T* pstFile )
{	
	s32 k = 0;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pstFile;

	if( pstFileInfo != NULL )
	{
		if( pstFileInfo->ps8FName )
		{
			printf( "file[%u]:%s\n", pstFileInfo->u32FNameMagic, pstFileInfo->ps8FName );
		}
		if( pstFileInfo->ps8Url )
		{
			printf( "url[%u]:%s\n", pstFileInfo->u32UrlMagic, pstFileInfo->ps8Url );
		}

		printf( "Size:%llu, CurSize=%llu, BlkCnt:%d, Pause: %d\n",
				pstFileInfo->u64Size, pstFileInfo->u64CurSize,
				pstFileInfo->s32BlkCnt, pstFileInfo->s32Pause );
		for( k = 0; k < pstFileInfo->s32BlkCnt; k++ )
		{
			pstBlk = &pstFileInfo->pstBlkInfo[k];
			printf( "	[%d]St:%llu, End:%llu, Offset:%llu\n", k, pstBlk->u64Start, pstBlk->u64End, pstBlk->u64Offset );
			printf( "	[%d]Idx:%d, Fd:%d, State:%d\n", k, pstBlk->s32Idx, pstBlk->s32Fd, pstBlk->eState );
			printf( "	[%d]Sign: %s\n", k, pstBlk->sign );
			printf( " \n" );
		}

	}

	
	return;
}

void apx_file_init( void )
{
	g_file_init = 1;
	return;
}

void apx_file_exit( void )
{
	finfo_st	*pstFileInfo = NULL;
	finfo_st	*pstTmp = NULL;

	if( !g_file_init )
		return;
		
	pthread_rwlock_wrlock( &g_file_head.rwLock );
	list_for_each_entry_safe( pstFileInfo, pstTmp, &g_file_head.stHeadList, stList )
	{
		__finfo_update( pstFileInfo, 1 );
		finfo_release( pstFileInfo );
	}
	pthread_rwlock_unlock( &g_file_head.rwLock );
	pthread_rwlock_destroy( &g_file_head.rwLock );
	g_file_init = 0;
	
	return;
}

/*---------------------------------------------------------
*Function:    apx_file_create
*Description:   
*			新建文件
*Parameters: 
*	pUrl			url
*	pFName		文件名称,绝对路径
*	u64FSize		文件大小
*	s32BlkCnt	分块个数
*	pError		错误码，见axp_file_e
*Return:
*	pError = APX_FILE_SUCESS or APX_FILE_DOWNLOADING，
				返回文件信息结构
*	pError = APX_FILE_DOWNLOADED or  APX_FILE_NO_MEM 
			or APX_FILE_PARAM_ERR, 返回NULL
*History:
*      xyfeng     	  2015-3-18        Initial Draft 
*-----------------------------------------------------------*/
FILE_T* apx_file_create( s8 *ps8Url, s8 *ps8FName, u64 u64FSize,
						s32 s32BlkCnt, s32 *ps32Err, u32 u32Upload )
{
	s32 s32Ret = 0;
	u64 u64Size = u64FSize;
	off_t	tSize = 0;
	finfo_st	*pstFileInfo = NULL;
	apx_file_e eRet = APX_FILE_SUCESS;

	if( ps32Err != NULL )
	{
		*ps32Err = eRet;
	}
	
	if( NULL == ps8Url || NULL == ps8FName )
	{
		eRet =APX_FILE_PARAM_ERR;
		goto END;
	}

	/** 查找正在下载列表 */
	pstFileInfo = __lookup_file_list( ps8Url, ps8FName );
	if( pstFileInfo != NULL )
	{
		eRet =APX_FILE_DOWNLOADING;
		goto END;
	}

	/** 文件是否已经存在*/
	s32Ret = __is_file_exist( ps8FName, &tSize );
	if( 0 == u32Upload )
	{/** download */
		if( s32Ret && ( 0 == u64FSize || ( u64 )tSize == u64FSize ) )
		{
			eRet =APX_FILE_DOWNLOADED;
			goto END;
		}
	}
	else
	{/** upload */
		if( 0 == s32Ret || 0 == tSize )
		{
			eRet = APX_FILE_UNKOWN;
			flog( "%s Not Found.", ps8FName );
			goto END;
		}
		u64Size = ( u64 )tSize;
	}
	
	/** not found. alloc it */
	pstFileInfo = finfo_new( ps8Url, ps8FName, u64Size );
	if( NULL ==  pstFileInfo )
	{
		eRet =APX_FILE_NO_MEM;
		goto END;
	}

	if( u32Upload )
	{
		pstFileInfo->u32Upload = 1;
	}
	else
	{
		/** 断点续传: 加载未完成的文件信息结构*/
		s32Ret = finfo_reload( pstFileInfo );
		if( 0 == s32Ret )
		{
			eRet =APX_FILE_DOWNLOADING;
			goto ADD_LIST;
		}
	}

	/** new file */
	// divide file to block		 
	pstFileInfo->s32BlkCnt = s32BlkCnt;
	s32Ret = fblock_new( pstFileInfo );
	if( s32Ret != 0 )
	{
		eRet =APX_FILE_NO_MEM;
		goto END;
	}

	if( 0 == pstFileInfo->u32Upload )
	{
		/**  touch .tmp & .info*/
		s32Ret = __file_create( pstFileInfo );
		if( s32Ret != 0 )
		{
			eRet =APX_FILE_UNKOWN;
			goto END;
		}
	}
	ADD_LIST:
		pthread_rwlock_wrlock( &g_file_head.rwLock );
		list_add( &pstFileInfo->stList,&g_file_head.stHeadList );
		g_file_head.s32Cnt++;
		pthread_rwlock_unlock( &g_file_head.rwLock );
	
	END:
		if( ps32Err != NULL )
		{
			*ps32Err = eRet;
		}

		if( APX_FILE_DOWNLOADING == eRet ||
			APX_FILE_SUCESS == eRet )
		{
			return pstFileInfo;
		}
		
		if( pstFileInfo != NULL )
		{
			pthread_rwlock_wrlock( &g_file_head.rwLock );
			finfo_release( pstFileInfo );
			pthread_rwlock_unlock( &g_file_head.rwLock );
		}
		
	return NULL;
}

/*-------------------------------------------------
*Function:    apx_file_reset
*Description:   
*           重置文件
*Parameters: 
*	pFileInfo[in]	文件信息结构
*Return:
*	return 0 if success, else return -1
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
int apx_file_reset( FILE_T *pFileInfo )
{
	s32 k = 0;
	s8 s8Name[FILE_LEN_MAX];
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	fblk_st *pstBlk = pstFileInfo->pstBlkInfo;
	
	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	pstFileInfo->s32Pause = 0;
	pstFileInfo->u64CurSize = 0;
	pstFileInfo->u64UpSize = pstFileInfo->u64CurSize + FILE_1M;
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );
	
	if( pstBlk != NULL )
	{
		for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
		{
			pstBlk[k].u64Offset = 0;
			pstBlk[k].eState = APX_BLK_EMPTY;
			if( pstBlk[k].s32Fd != -1 )
			{
				close( pstBlk[k].s32Fd );
				pstBlk[k].s32Fd = -1;
			}
		}
	}
	
	__finfo_update( pstFileInfo, 1 );
	
	snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
	apx_file_delete( s8Name );
	__ftmp_create( pstFileInfo );
	
	return 0;
}

/*-------------------------------------------------
*Function:    apx_file_release
*Description:   
*           释放文件
*Parameters: 
*	pFileInfo[in]	文件信息结构
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
void apx_file_release( FILE_T *pFileInfo )
{	
	int flag = 0;
	finfo_st	*pstPos = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( NULL == pstFileInfo )
	{
		return;
	}

	if( __lookup_list( pstFileInfo ) )
	{
		pthread_rwlock_wrlock( &g_file_head.rwLock );
		finfo_release( pstFileInfo );
		pthread_rwlock_unlock( &g_file_head.rwLock );
	}
	return;
}

/*-------------------------------------------------
*Function:    apx_file_delete
*Description:   
*           删除文件
*Parameters: 
*	pFileName[in]		文件名
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
void apx_file_delete( s8* pFileName )
{
	if( pFileName != NULL )
	{
		remove( ( char* )pFileName );
	}
	return;
}

/*-------------------------------------------------
*Function:    apx_file_destroy
*Description:   
*           释放文件 + 删除文件
*Parameters: 
*	pFileInfo[in]	文件信息结构
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
void apx_file_destroy( FILE_T *pFileInfo)
{
	s8 s8Name[FILE_LEN_MAX];
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( NULL == pstFileInfo )
	{
		return;
	}
	if( !__lookup_list( pstFileInfo ) )
	{
		return;
	}
	
	if( !pstFileInfo->u32Upload 
		&& pstFileInfo->ps8FName != NULL )
	{
		__finfo_unmap( pstFileInfo );
		if( pstFileInfo->u64Size == pstFileInfo->u64CurSize )
		{
			apx_file_delete( pstFileInfo->ps8FName );
		}
		else
		{
			snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_INFO_SUFFIX, pstFileInfo->ps8FName );
			apx_file_delete( s8Name );
			
			snprintf( ( char* )s8Name, sizeof( s8Name ), "%s"FILE_TMP_SUFFIX, pstFileInfo->ps8FName );
			apx_file_delete( s8Name );
		}
	}
	
	apx_file_release( pFileInfo );
	return;
}

/*-------------------------------------------------
*Function:    apx_file_pause
*Description:   
*           暂停下载，关闭打开的文件
*Parameters: 
*	pFileInfo[in]	文件信息结构	
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
void apx_file_pause( FILE_T *pFileInfo )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	
	if( pstFileInfo != NULL && 0 == pstFileInfo->s32Pause )
	{
		pstFileInfo->s32Pause = 1;
		fblock_close( pstFileInfo );
		__finfo_update( pstFileInfo, 1 );
	}

	return;
}

/*-------------------------------------------------
*Function:    apx_file_resume
*Description:   
*           恢复下载，打开关闭的文件
*Parameters: 
*	pFileInfo[in]	文件信息结构
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
void apx_file_resume( FILE_T *pFileInfo )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	
	if( pstFileInfo != NULL && 1 == pstFileInfo->s32Pause )
	{
		pstFileInfo->s32Pause = 0;
	}
	return;
}

/*-------------------------------------------------
*Function:    apx_file_blk_info
*Description:   
*           获取未下载的block
*Parameters: 
*	pFileInfo[in]	文件信息结构
*Return:
*       返回未开始下载的block
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
apx_fblk_st* apx_file_blk_info( FILE_T *pFileInfo )
{
	s32 k = 0;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	fblk_st *pstBlk = pstFileInfo->pstBlkInfo;
	
	if( NULL == pFileInfo || NULL == pstBlk )
	{
		return NULL;
	}
	if( pstFileInfo->s32Pause )
	{
		return NULL;
	}

	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
	{
		if( APX_BLK_EMPTY == pstBlk[k].eState )
		{
			pstBlk[k].s32Idx = k;
			pstBlk[k].eState = APX_BLK_ALLOCED;
			pthread_mutex_unlock( &pstFileInfo->mBlkLock );
			return ( apx_fblk_st* )&pstBlk[k];
		}
	}
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );

	return NULL;
}

/*-------------------------------------------------
*Function:    apx_file_blk_reset
*Description:   
*           重置指定分块的下载状态
*Parameters: 
*	pFileInfo[IN]		文件信息结构
*	pstblk[IN/OUT]		分块信息信息
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
void apx_file_blk_reset( FILE_T *pFileInfo, apx_fblk_st *pstblk )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	fblk_st *pstBlkInfo = ( fblk_st* )pstblk;

	if( pstFileInfo != NULL && pstblk != NULL )
	{
		//pthread_mutex_lock( &pstFileInfo->mBlkLock );
		pstBlkInfo->eState = APX_BLK_EMPTY;
		//if( 1 == pstFileInfo->u32Upload )
		{
			pstBlkInfo->u64Offset = 0;
		}
		//pthread_mutex_unlock( &pstFileInfo->mBlkLock );
	}
	return;
}

/*-------------------------------------------------
*Function:    apx_file_blk_cnt
*Description:   
*           获取文件分块个数
*Parameters: 
*	pFileInfo[IN]	文件信息结构
*Return:
*       s32 :	返回文件分块数
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
s32 apx_file_blk_cnt( FILE_T *pFileInfo )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	return ( pstFileInfo != NULL ) ? pstFileInfo->s32BlkCnt : 0;
}

/*-------------------------------------------------
*Function:    apx_file_read
*Description:   
*           读取指定block的数据
*	暂时用不到，暂不实现
*Parameters: 
*	pBuf[in]		读取的数据
*	szSize[in]	数据大小
*	s32Idx[in]	读取的数据对于的block索引
*	pFileInfo[in]	文件信息结构
*Return:
*       size_t :
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
ssize_t apx_file_read( void *pBuf, size_t szSize, s32 s32Idx, FILE_T *pFileInfo )
{
	s32 s32Fd = -1;
	u64	u64Left = 0;
	size_t sLen = szSize;
	ssize_t ssRet = -1;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( NULL == pstFileInfo || s32Idx <0 ||
		s32Idx >= pstFileInfo->s32BlkCnt )
	{
		flog( "parameters error( idx: %d, blkCnt: %d).", s32Idx,  pstFileInfo->s32BlkCnt );
		return -1;
	}
	if( pstFileInfo->s32Pause )
	{
		flog( "alert: file is pause." );
		return -2;
	}

	pstBlk = &( pstFileInfo->pstBlkInfo[s32Idx] );
	if( APX_BLK_FULL == pstBlk->eState )
	{
		return 0;
	}
	if( pstBlk->eState != APX_BLK_ALLOCED  &&
	     pstBlk->eState != APX_BLK_HOLD )
	{
		flog( "block states error( idx: %d, blkCnt: %d state: %d).", s32Idx,  pstFileInfo->s32BlkCnt, pstBlk->eState );
		return -3;
	}
	if( APX_BLK_ALLOCED == pstBlk->eState )
	{
		pstBlk->eState = APX_BLK_HOLD;
	}

	/** left bytes to read*/
	if( pstBlk->u64End > 0 )
	{
		u64Left = pstBlk->u64End -pstBlk->u64Start + 1 -  pstBlk->u64Offset;
		if( u64Left <= szSize )
		{
			sLen = u64Left;
		}
	}
	s32Fd = __fblock_open( pstBlk, pstFileInfo->ps8FName, 1 );
	if( s32Fd < 0 )
	{		
		flog( "%d, %s: %s", s32Fd, pstFileInfo->ps8FName, strerror( errno ) );
		return -5;
	}
	ssRet = read( s32Fd, pBuf, sLen );
	if( ssRet < 0 || ssRet != ( ssize_t )sLen )
	{
		flog( "%s: %s, %d, %ld, %ld", pstFileInfo->ps8FName, strerror( errno ), s32Fd, ssRet, sLen );
		return -4;
	}

	pstBlk->u64Offset += sLen;
	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	pstFileInfo->u64CurSize += sLen;	
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );

	if( u64Left != 0 && u64Left <= szSize )
	{
		/** full bolck */
		close( pstBlk->s32Fd );
		pstBlk->s32Fd = -1;
		pstBlk->eState = APX_BLK_FULL;
		//flog( "%d blk Full( %llu -- %llu ).", pstBlk->s32Idx, pstFileInfo->u64CurSize, pstFileInfo->u64Size );
	#if 0
		/**  判断文件是否已上传完成 */
		if( pstFileInfo->u64Size == pstFileInfo->u64CurSize )
		{
			flog( "file done." );
			//return APX_FILE_FULL;
			return sLen;
		}
		return sLen;
		//return APX_FILE_BLK_FULL;
	#endif
	}
		
	return sLen;
}

/*-------------------------------------------------
*Function:    apx_file_write
*Description:   
*           写入数据到指定的block
*Parameters: 
*	pBuf[in]		文件数据
*	szSize[in]	写入的数据大小
*	s32Idx[in]	数据所对应的数据块索引
*	pFileInfo[in]	文件信息结构 
*Return:
*	return size of buf if success, 
*	return APX_FILE_BLK_FULL, if block is full
*	return APX_FILE_FULL, if file is full
*	other return -1
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
ssize_t apx_file_write( void *pBuf, size_t szSize, s32 s32Idx, FILE_T *pFileInfo )
{
	u8	u8Flag = 0;
	s32 s32Fd = -1;
	u64	u64Left = 0;
	size_t sLen = szSize;
	ssize_t ssRet = -1;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	
	if( NULL == pstFileInfo || s32Idx <0 ||
		s32Idx >= pstFileInfo->s32BlkCnt )
	{
		flog( "parameters error( idx: %d, blkCnt: %d).", s32Idx,  pstFileInfo->s32BlkCnt );
		return -1;
	}
	if( pstFileInfo->s32Pause )
	{
		flog( "alert: file is pause." );
		return -2;
	}

	/** file download done */
	if( NULL == pBuf && 0 == szSize )
	{
		flog( "file full, size: %llu.", pstFileInfo->u64CurSize );
		pstFileInfo->u64Size = pstFileInfo->u64CurSize;
		fblock_close( pstFileInfo );
		goto file_full;
	}

	if( NULL == pBuf
		|| 0 == szSize )
	{
		flog( "parameters error, size or buf is null( idx: %d, blkCnt: %d).", s32Idx,  pstFileInfo->s32BlkCnt );
		return -1;
	}

	pstBlk = &( pstFileInfo->pstBlkInfo[s32Idx] );
	if( pstBlk->eState != APX_BLK_ALLOCED  &&
	     pstBlk->eState != APX_BLK_HOLD )
	{
		flog( "block states error( idx: %d, blkCnt: %d state: %d).", s32Idx,  pstFileInfo->s32BlkCnt, pstBlk->eState );
		return -3;
	}
	if( APX_BLK_ALLOCED == pstBlk->eState )
	{
		pstBlk->eState = APX_BLK_HOLD;
	}
	
	/** left bytes to write*/
	if( pstBlk->u64End > 0 )
	{
		u64Left = pstBlk->u64End -pstBlk->u64Start + 1 -  pstBlk->u64Offset;
		if( u64Left <= szSize )
		{
			sLen = u64Left;
		}
	}
	s32Fd = __fblock_open( pstBlk, pstFileInfo->ps8FName, 0 );
	ssRet = write( s32Fd, pBuf, sLen );
	if( ssRet < 0 || ssRet != ( ssize_t )sLen )
	{
		return -4;
	}

	pstBlk->u64Offset += sLen;
	pthread_mutex_lock( &pstFileInfo->mBlkLock );
	pstFileInfo->u64CurSize += sLen;
	if( pstFileInfo->u64CurSize > pstFileInfo->u64UpSize )
	{
		u8Flag = 1;
		pstFileInfo->u64UpSize = pstFileInfo->u64CurSize + FILE_1M;
	}
	pthread_mutex_unlock( &pstFileInfo->mBlkLock );

	if( u64Left != 0 && u64Left <= szSize )
	{
		/** full bolck */
		close( pstBlk->s32Fd );
		pstBlk->s32Fd = -1;
		pstBlk->eState = APX_BLK_FULL;
		/**  判断文件是否已下载完成 */
		if( pstFileInfo->u64Size == pstFileInfo->u64CurSize )
		{
		file_full:
			//flog( "file done." );
			__finfo_done( pstFileInfo );
			return sLen;
			//return APX_FILE_FULL;
		}
		__finfo_update( pstFileInfo, 0 );
		return sLen;
		//return APX_FILE_BLK_FULL;
	}
	else if( u8Flag )
	{
		__finfo_update( pstFileInfo, 0 );
	}
	
	return sLen;
}

/*-------------------------------------------------
*Function:    apx_file_size
*Description:   
*           获取文件大小
*Parameters: 
*	pFileInfo[in]		 文件信息结构
*Return:
*       文件大小
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
u64 apx_file_size( FILE_T *pFileInfo )
{
	return ( pFileInfo != NULL ) ? ( ( finfo_st* )pFileInfo )->u64Size : 0;
}

/*-------------------------------------------------
*Function:    apx_file_cur_size
*Description:   
*           获取已下载的文件大小
*Parameters: 
*	pFileInfo[in]		 文件信息结构
*Return:
*       已下载的文件大小
*History:
*      xyfeng     	  2015-3-20        Initial Draft 
*---------------------------------------------------*/
u64 apx_file_cur_size( FILE_T *pFileInfo )
{
	return ( pFileInfo != NULL ) ? ( ( finfo_st* )pFileInfo)->u64CurSize : 0;
}

/*-------------------------------------------------
*Function:    apx_file_set_cur_size
*Description:   
*           设置已下载文件大小
*Parameters: 
*	pFileInfo[IN]		文件信息结构
*	u64Size[IN]		文件大小
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
void apx_file_set_cur_size( FILE_T *pFileInfo, u64 u64Size )
{
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( pstFileInfo )
	{
		pthread_mutex_lock( &pstFileInfo->mBlkLock );
		pstFileInfo->u64CurSize = u64Size;
		pthread_mutex_unlock( &pstFileInfo->mBlkLock );
	}
	return;
}

s32 apx_file_is_exist( s8 *path, off_t *pSize )
{
	return ( path ) ?  __is_file_exist( path, pSize ) : 0;
}

s32 apx_file_mkdir( s8* path )
{
	s32 ret = 0;
	size_t k = 0,
		   len = 0;
	s8 s8Dir[512] = {0};

	if( NULL == path )
	{
		return -1;
	}

	strncpy( s8Dir,   path, sizeof( s8Dir ) - 1 );
	len = strlen( s8Dir );
	if( s8Dir[len-1] != '/' )
	{
		strncat( s8Dir, "/", sizeof( s8Dir ) - 1 );
	}
	
	len = strlen( s8Dir );
	for( k = 1; k < len; k++ )
	{
		if( s8Dir[k] == '/' )
		{
			s8Dir[k] = 0;
			ret = access( s8Dir, F_OK );
			if( ret < 0 )
			{
				ret = mkdir( s8Dir, 0755 );
				if( ret < 0 && errno != EEXIST )
				{
					return   -1;
				}
			}
			s8Dir[k]   =   '/';
		}
	}
	
	return   0;
}

void apx_file_update_upsize( FILE_T *pFileInfo )
{
	s32 k = 0;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;

	if( pstFileInfo != NULL )
	{
		pstBlk = pstFileInfo->pstBlkInfo;
		
		if( pstBlk != NULL )
		{
			for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
			{
				if( APX_BLK_FULL == pstBlk[k].eState )
				{
					pstFileInfo->u64CurSize += pstBlk[k].u64End -pstBlk[k].u64Start + 1;
				}
			}
		}
	}
	
	return;
}

/*-------------------------------------------------
*Function:    apx_block_point
*Description:   
*           获取文件分块指针
*Parameters: 
*	pFileInfo[IN]	文件信息结构
*Return:
*       void* :		返回文件分块指针
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
void* apx_block_point( FILE_T *pFileInfo )
{
	return ( pFileInfo != NULL ) ? ( ( finfo_st* )pFileInfo)->pstBlkInfo : NULL;
}

void apx_block_reset( FILE_T *pFileInfo )
{
	s32 k = 0;
	fblk_st *pstBlk = NULL;
	finfo_st *pstFileInfo = ( finfo_st* )pFileInfo;
	
	if( pstFileInfo != NULL )
	{
		pstBlk = pstFileInfo->pstBlkInfo;
		
		if( pstBlk != NULL )
		{
			for( k = 0; k < pstFileInfo->s32BlkCnt; k++  )
			{
				pstBlk[k].eState = APX_BLK_EMPTY;
				pstBlk[k].u64Offset = 0;
			}
		}
	}
	
	return;
}

/*-------------------------------------------------
*Function:    apx_block_set_status
*Description:   
*           设置指定分块的下载状态
*Parameters: 
*	pBlk[IN]		分块指针
*	index[IN]		分块索引
*	status[IN]	分块状态
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
void apx_block_set_status( void* pBlk, int index, int status )
{
	fblk_st *pBlkInfo = ( fblk_st* )pBlk;

	if( pBlkInfo != NULL )
	{
		pBlkInfo += index;
		if( pBlkInfo != NULL )
		{
			if( 0 == status )
			{
				pBlkInfo->eState = APX_BLK_EMPTY;
				pBlkInfo->u64Offset = 0;
			}
			else
			{
				pBlkInfo->eState = APX_BLK_FULL;
			}
		}
	}
	return;
}

/*-------------------------------------------------
*Function:    apx_block_set_sign
*Description:   
*           设置指定分块的sha1sum校验值
*Parameters: 
*	pBlk[IN]		分块指针
*	index[IN]		分块索引
*	pSign[IN]		sha1sum校验值
*Return:
*       ( void  )
*History:
*      xyfeng     	  2015-6-29        Initial Draft 
*---------------------------------------------------*/
void apx_block_set_sign( void* pBlk, int index, char* pSign )
{
	fblk_st *pBlkInfo = ( fblk_st* )pBlk;
	
	if( pBlkInfo && pSign )
	{
		pBlkInfo += index;
		if( pBlkInfo != NULL )
		{
			strncpy( pBlkInfo->sign, pSign, sizeof( pBlkInfo->sign ) -1 );
		}
	}
	return;
}


