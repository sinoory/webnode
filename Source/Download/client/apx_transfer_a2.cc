/*
 ============================================================================
 Name        : apx_transfer_a2.cc
 Author      : liuyang (liuyang@appexnetworks.com)
 Version     : 1.0
 Copyright   : appexnetworks
 Description : 
 	Aria2 download module.
 ============================================================================
 */
/*-----------------------------------------------------------*/
/*                          Include File Header                               */
/*-----------------------------------------------------------*/
#ifndef _GUN_SOURCE
#define _GUN_SOURCE
#endif
#include <iostream>
#include "../include/aria2/aria2.h"
#include "apx_transfer_a2.h"
#include "apx_wlog.h"
#ifdef __cplusplus
extern "C"{
#endif

/*------------------------------------------------------------*/
/*                         Global Variables                                */
/*------------------------------------------------------------*/

/* handle_lock is used to locking handles struct.*/
pthread_mutex_t handle_lock = PTHREAD_MUTEX_INITIALIZER;
 /*Golbal struct , and it is used to connect to aria2.*/
struct handle *handles;
/*struct handles default size*/
u32 tasknum = 50;
// init flag 
static int init_flag;

/*------------------------------------------------------------*/
/*                        Functions                                               */
/*------------------------------------------------------------*/

// int to c++ string   
static std::string itos( int i );
// c++ string to int
static int sstoi( std::string st );

/*---------------------------------------------------------
 *Function: downloadEvenCallback   
 *Description:   
 *      aria2 event callback func
 *Parameters: 
 *		session		aria2 session
 *		event		aria2 download event
 *		gid			aria2 task gid
 *		userData	user data pointer
 *Return:
 *      return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/

static int downloadEventCallback( aria2::Session* session, aria2::DownloadEvent event,
									aria2::A2Gid gid, void* userData )
{
	if ( session == NULL || userData == NULL )
	{
		std::cerr << "Session or uerData us NULL." << std::endl;
		return -1;
	}
	struct handle* hptr;
	hptr = (struct handle*)userData;

	switch ( event )
	{
		case aria2::EVENT_ON_DOWNLOAD_COMPLETE:
			hptr->status = APX_TASK_STATE_FINISHED;
			std::cerr << "COMPLETE";
			break;
		case aria2::EVENT_ON_DOWNLOAD_PAUSE:
			std::cerr << "PAUSE";
			break;
		case aria2::EVENT_ON_DOWNLOAD_ERROR:
			hptr->status = APX_TASK_STATE_UNDEFINED;
			std::cerr << "ERROR";
			break;
		default:
			std::cerr << "No Event ocurrsed."<< std::endl;
			return -2;
	}
	// Print task gid and url info.
	std::cerr << " [ " << gid << " ] ";
	aria2::DownloadHandle* dh = aria2::getDownloadHandle( session, gid );
	if ( !dh ) return -3;
	if ( dh->getNumFiles() > 0 )
	{
		aria2::FileData f = dh->getFile( 1 );
		// Path may be empty if the file name has not been determined yet.
		if ( f.path.empty() )
		{
			if ( !f.uris.empty() )
			{
				std::cerr << f.uris[0].uri;
			}
		}
		else if ( strlen(hptr->fname) != 0 )
		{
			std::cerr << hptr->fname;
		} 
		else
		{
			std::cerr << f.path;
		}
	}
	aria2::deleteDownloadHandle( dh );
	std::cerr << std::endl;
	return 0;
}

/*---------------------------------------------------------
 *Function:  _set_option  
 *Description:   
 *		set task option
 *Parameters: 
 *		options		aria2 options
 *		task_opt	task info
 *Return:
 *       void
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static void _set_option( aria2::KeyVals* options, struct apx_trans_opt* task_opt )
{
	int i = 1;

	std::pair <std::string, std::string> option ( "continue", "true" );
	options->insert( options->begin(), option );

	std::pair <std::string, std::string> option1 ( "max-connection-per-server", "16" );
	options->insert( options->begin() + i, option1 );
	i++;

	std::pair <std::string, std::string> option2 ( "min-split-size", "2M" );
	options->insert( options->begin() + i, option2 );
	i++;

	std::pair <std::string, std::string> option11 ( "conf-path", "/etc/download/.apx_down_conf" );
	options->insert( options->begin() + i, option11 );
	i++;


	//write task options
	if( task_opt->ftp_user[0] != '\0' )
	{
		std::string user( task_opt->ftp_user );
		std::pair <std::string, std::string> option3 ( "ftp-user", user );
		options->insert( options->begin() + i, option3 );
		i ++;
	}

	if( task_opt->ftp_passwd[0] != '\0' )
	{
		std::string pswd( task_opt->ftp_passwd );
		std::pair <std::string, std::string> option4 ( "ftp-passwd", pswd );
		options->insert( options->begin() + i, option4 );
		i ++;
	}

	if( task_opt->fpath[0] != '\0' )
	{
		std::string path( task_opt->fpath );
		std::pair <std::string, std::string> option5 ( "dir", path );
		options->insert( options->begin() + i, option5 );
		i++;
	}

	if( task_opt->concurr >= 0 )
	{
		std::string curr = itos( task_opt->concurr );
		std::pair <std::string, std::string> option6 ( "split", curr );
		options->insert( options->begin() + i, option6 );
		i++;
	}

	if( task_opt->down_splimit >= 0 )
	{
		std::string down = itos( task_opt->down_splimit );
		down += "K";
		std::pair <std::string, std::string> option7 ( "max-download-limit", down );
		options->insert( options->begin() + i, option7 );
		i++;
	}

	if( task_opt->up_splimit >= 0 )
	{
		std::string up = itos( task_opt->up_splimit );
		up += "K";
		std::pair <std::string, std::string> option8 ( "max-upload-limit", up );
		options->insert( options->begin() + i, option8 );
		i++;
	}

	if( task_opt->fname[0] != '\0' && task_opt->proto != APX_TASK_PROTO_BT )
	{
		std::string fname( task_opt->fname );
		std::pair <std::string, std::string> option9 ( "out", fname );
		options->insert( options->begin() + i, option9 );
		i++;
	}


	if( task_opt->bt_select[0] != '\0' && task_opt->proto == APX_TASK_PROTO_BT )
	{
		std::string bt( task_opt->bt_select );
		std::pair <std::string, std::string> option10 ( "select-file", bt );
		options->insert( options->begin() + i, option10 );
		i++;
	}

	
}

/*---------------------------------------------------------
 *Function:	  _trans_addhttp
 *Description:   
 *      add HTTP(s) url to aria2
 *Parameters: 
 *      nu			task number
 *		task_opt	task info
 *Return:
 *      return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_addhttp( u32 nu, struct apx_trans_opt* task_opt )
{
	int ret = 0;
	aria2::KeyVals options;

	//init options
	std::string uri( task_opt->uri );
	std::vector<std::string> uris = { uri };

	_set_option( &options, task_opt );

	// Add download URL to session
	ret = aria2::addUri( (aria2::Session*)(handles[nu].session), &handles[nu].gid, uris, options );
	if( ret < 0 )
	{
		printf( "Failed to add download %s.\n", task_opt->uri );
		apx_wlog( "_trans_addhttp: Add new http url failed", ret );
	}

	return ret;
}

static int _btfile_name_cut( char *out, char *in, int size, int len, int buf_size )
{
	int ret = 0;
	int pos = 0;
	int llen = len;
	int end = 0;
	char *begin = NULL;

	if( !out || !in )
	{
		ret = -1;
		apx_wlog( "_btfile_name_cut: input pointer NULL", ret );
		return ret;
	}

	while( 1 )
	{
		if( size != 0 &&  size != llen )
		{
			if( size + 1 <= buf_size )
			{
				strncat( out, in, size );
				strncat( out, "/", 1);
				len = len - size - 1;
				end = end + size + 1;
			}
			else
			{
				ret = -2;
				apx_wlog( "_btfile_name_cut : bufsize is not enough", ret );
				break;
			}

			size = atoi( &in[size] );
			begin = strchr( in, ':' );
			if( !begin )
			{
				ret = -3;
				apx_wlog( "_btfile_name_cut : check failed", ret );
				break;
			}

			pos = begin - in + 1; //pass ':'
			llen = llen - pos;
		}
		else
		{
			if( size <= buf_size )
			{
				strncat( out, in, size );
				end = end + size;
				break;
			}
			else
			{
				ret = -4;
				apx_wlog( "_btfile_name_cut : bufsize is not enough", ret );
				break;
			}
		}

		in = &in[pos];
	}

	out[end] = '\0';
	return ret;
}


/*---------------------------------------------------------
 *Function:    _trans_addftp
 *Description:   
 *		add FTP url to aria2
 *Parameters: 
 *		nu			task number
 *		task_opt	task info
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_addftp( u32 nu, struct apx_trans_opt* task_opt )
{
	int ret = 0;
	int i = 1;
	uint64_t gid = (uint64_t)nu;
	aria2::KeyVals options;

	// init options 
	std::string uri( task_opt->uri );
	std::vector<std::string> uris = { uri };

	_set_option( &options, task_opt );

	// Add download URL to session
	ret = aria2::addUri( (aria2::Session*)handles[nu].session, &handles[nu].gid, uris, options );
	if( ret < 0 )
	{
		printf( "Failed to add download %s", task_opt->uri );
		apx_wlog( "_trans_addftp: Add new ftp url failed", ret );
	}

	return ret;
}

/*---------------------------------------------------------
 *Function:    _get_filepath_name
 *Description:   
 *		get file name from bt torrent
 *Parameters: 
 *		metafile_buf	buffer pointer
 *		metafile_size	buffer size
 *		bt				record bt file name
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _get_filepath_name ( char *metafile_buf, long long metafile_size, struct btfile *bt )
{
	int i = 0;
	int ret = 0;
	long long pos;
	long long pst;
	char *begin = NULL;
	char *end = NULL;
	char *buf = metafile_buf;
	int size = 0;
	int len = 0;//file name size

	begin = strstr( metafile_buf, "4:name" );
	if( begin )
	{
		pos = begin - metafile_buf;
		pos += strlen( "4:name" );
		begin = strchr( &metafile_buf[pos], ':' );
		if( !begin )
		{
			ret = -1;
			apx_wlog( "get_filepath_name: check char failed", ret );
			return ret;
		}
		pos = begin - metafile_buf + 1; //pass ':'

		end = strstr( metafile_buf, "12:piece" );
		if ( !end )
		{
			ret = -2;
			apx_wlog( "get_filepath_name: check char failed", ret );
			return ret;
		}
		pst = end - metafile_buf;
		len = pst - pos;
		if ( len > ( sizeof( bt->fn ) - 1 ) )
		{
			len = sizeof( bt->fn ) - 1;
			memcpy ( bt->fn, &metafile_buf[pos], len );
			bt->fn[len] = '\0';
		}
		else
		{
			memcpy ( bt->fn, &metafile_buf[pos], len );
			bt->fn[len]  = '\0';
		}
		pos = 0;
		pst = 0;
		begin = NULL;
		end = NULL;
		//printf ( "bt_file_name: %s\n", bt->fn );
 
	}
	while ( 1 )
	{
		begin = strstr( buf, "4:pathl" );
		if ( begin )
		{
			pos = begin - buf;
			pos += strlen( "4:pathl" );
			begin = strchr( &buf[pos], ':' );
			if( !begin )
			{
				ret = -3;
				apx_wlog( "get_filepath_name: check char failed", ret );
				return ret;
			}
			size = atoi( &buf[pos] );
			pos = begin - buf + 1; //pass ':'

			if ( end = strstr( buf, "eed6:" ))
			{
				pst = end - buf;
				len = pst - pos;
				if( size == len )
				{
					if ( len > ( sizeof( bt->file[i] ) - 1 ) )
					{
						len = sizeof( bt->file[i] ) - 1;
						memcpy( bt->file[i], &buf[pos], len );
						bt->file[i][len] = '\0';
					}
					else
					{
						memcpy( bt->file[i], &buf[pos], len );
						bt->file[i][len] = '\0';
					}
				}
				else
				{
					memset( bt->file[i], 0, sizeof(bt->file[i]) );
					ret = _btfile_name_cut( bt->file[i], &buf[pos], size, len, (int)sizeof( bt->file[i] ) - 1 );
					if( ret != 0 )
					{
						ret = -4;
						apx_wlog( "get_filepath_name: get filename failed", ret );
						return ret;
					}
				}
			}
			else if ( end = strstr( buf, "eee4:" ) )
			{
				pst = end - buf;
				len = pst - pos;
				if( size == len )
				{
					if ( len > ( sizeof( bt->file[i] ) - 1 ) )
					{
						len = sizeof( bt->file[i] ) - 1;
						memcpy( bt->file[i], &buf[pos], len );
						bt->file[i][len] = '\0';
					}
					else
					{
						memcpy( bt->file[i], &buf[pos], len );
						bt->file[i][len] = '\0';
					}
				}
				else
				{
					memset( bt->file[i], 0, sizeof(bt->file[i]) );
					ret = _btfile_name_cut( bt->file[i], &buf[pos], size, len, (int)sizeof( bt->file[i] ) - 1 );
					if( ret != 0 )
					{
						ret = -5;
						apx_wlog( "get_filepath_name: get filename failed", ret );
						return ret;
					}
				}
			}
			else
			{
				ret = -6;
				apx_wlog( "get_filepath_name: check char failed", ret );
				return ret;
			}
			//printf("%d :%s\n", i+1, bt->file[i] );
			buf = &buf[ pst + 3 ];
			pos = 0;
			pst = 0;
			begin = NULL;
			end = NULL;
			i ++;
		}
		else
		{
			bt->size = i;
			break;
		}
	}
	return ret;
}


/*---------------------------------------------------------
 *Function:    _trans_addbt
 *Description:   
 *		add bt torrent path to aria2
 *Parameters: 
 *		nu			task number
 *		task_opt	task info
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_addbt( u32 nu, struct apx_trans_opt* task_opt )
{
	int ret = 0;
	int i = 1;
	uint64_t gid = (uint64_t)nu;
	aria2::KeyVals options;

	//init options
	std::string torrentfile( task_opt->uri );
	_set_option( &options, task_opt );

	// Add download BT to session
	ret = aria2::addTorrent( (aria2::Session*)handles[nu].session, &handles[nu].gid, torrentfile, options );
	if( ret < 0 )
	{
		printf( "Failed to add download %s", task_opt->uri );
		apx_wlog( "_trans_addbt: Add bt torrent failed", ret );
	}

	return ret;
}

/*---------------------------------------------------------
 *Function:   	_trans_shutdown 
 *Description:   
 *		shutdown aria2 download
 *Parameters: 
 *		handle	aria2 session
 *		force	a flag to control shutdown 
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_shutdown( u32* handle, bool force )
{
	int ret = 0;

	ret = aria2::shutdown( (aria2::Session*)handle, force );
	return ret;
}

/*---------------------------------------------------------
 *Function:    _trans_remove
 *Description:   
 *		remove aria2 task
 *Parameters: 
 *		handle	aria2 session
 *		gid		task number
 *		force	a flag
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
// remove task
static int _trans_remove( u32* handle, uint64_t gid, bool force )
{
	int ret = 0;

	ret = aria2::removeDownload( (aria2::Session*)handle, gid, force );
	return ret;
}

/*---------------------------------------------------------
 *Function:    _trans_pause
 *Description:   
 *		pause task
 *Parameters: 
 *		handle	task session
 *		gid		task number
 *		force	a flag
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
//pause task
static int _trans_pause( u32* handle, uint64_t gid, bool force )
{
	int ret = 0;
	force = true;

	ret = aria2::pauseDownload( (aria2::Session*)handle, gid, force );
	return ret;
}

/*---------------------------------------------------------
 *Function:    _trans_unpause
 *Description:   
 *		start pause task
 *Parameters: 
 *		handle	task session
 *		gid		task number
 *Return:
 *      return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
//start paused task
static int _trans_unpause( u32* handle, uint64_t gid )
{
	int ret = 0;

	ret = aria2::unpauseDownload( (aria2::Session*)handle, gid );
	return ret;
}

/*---------------------------------------------------------
 *Function:    itos
 *Description:   
 *		integer variable into string
 *Parameters: 
 *		i	intfger
 *Return:
 *      return string
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static std::string itos( int i )
{
	char ch[10];
	snprintf( ch, sizeof( ch ) - 1, "%d", i );
	std::string str( ch );
	return str;
}

/*---------------------------------------------------------
 *Function:   sstoi
 *Description:   
 *		a string into int
 *Parameters: 
 *		st
 *Return:
 *      return int
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int sstoi( std::string st )
{
	int i;
	char *p = (char*)st.c_str();
	i = atoi( p );

	return i;
}

/*---------------------------------------------------------
 *Function:    _trans_setglbopt
 *Description:   
 *		set global options
 *Parameters: 
 *		handle
 *		glb_opt
 *Return:
 *      return 0 if success, error return -1
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_setglbopt( u32* handle, struct apx_trans_glboptions* glb_opt )
{
	int ret = 0;
	int i = 1;
	aria2::KeyVals options;

	//init global options
	std::pair <std::string, std::string> option1 ( "continue", "true" );
	options.insert( options.begin(), option1 );

	if( glb_opt->path[0] != '\0' )
	{
		std::string str1(glb_opt->path);
		std::pair <std::string, std::string> option2 ( "dir", str1 );
		options.insert( options.begin() + i, option2 );
		i ++;
	}

	if( glb_opt->connections >= 0 )
	{
		std::string str2 = itos( glb_opt->connections );
		std::pair <std::string, std::string> option3 ( "split", str2 );
		options.insert(options.begin() + i, option3);
		i ++;
	}

	if( glb_opt->max_limit_downspeed >= 0 )
	{
		std::string str3 = itos(glb_opt->max_limit_downspeed);
		str3 += "K";
		std::pair <std::string, std::string> option4 ( "max-overall-download-limit", str3 );
		options.insert( options.begin() + i, option4 );
		i ++;
	}

	if( glb_opt->max_limit_uploadspeed >= 0 )
	{
		std::string str4 = itos( glb_opt->max_limit_uploadspeed );
		str4 += "K";
		std::pair <std::string, std::string> option5 ( "max-ovarall-upload-limit", str4 );
		options.insert( options.begin() + i, option5 );
		i ++;
	}
	
	if( glb_opt->max_concurrent_download >= 0 )
	{
		std::string str5 = itos( glb_opt->max_concurrent_download );
		std::pair <std::string, std::string> option6 ( "max-concurrent-downloads", str5 );
		options.insert( options.begin() + i, option6 );
		i ++;
	}

	//write global options
	ret = aria2::changeGlobalOption( (aria2::Session*)handle, options );
	if( ret != 0 )
	{
		printf( "Set global options failed!\n" );
		apx_wlog( "_trans_setglbopt: Set global opt failed", ret );
	}
	return ret;
}

/*---------------------------------------------------------
 *Function:    _trans_gettaskstat
 *Description:   
 *		get task stat
 *Parameters: 
 *		handle  aria2 session
 *		gid		task number
 *		task_opt	task info
 *Return:
 *		return 0 if success, error return negative. 
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_gettaskstat( u32* handle, uint64_t gid, struct apx_trans_stat* task_stat )
{	
	int ret = 0;

	//get task handle from gid
	aria2::DownloadHandle* dh = aria2::getDownloadHandle( (aria2::Session*)handle, gid );
	if( dh ) 
	{
		task_stat->connections			= (u16)dh->getConnections();
		task_stat->down_size			= (u64)dh->getCompletedLength();
		task_stat->up_size				= (u64)dh->getUploadLength();
		task_stat->total_size			= (u64)dh->getTotalLength();
		task_stat->down_speed			= (u32)dh->getDownloadSpeed();
		task_stat->up_speed				= (u32)dh->getUploadSpeed();
		task_stat->trans_errno			= dh->getErrorCode();
		//delete task handle
		aria2::deleteDownloadHandle( dh );
	}

	return ret;
}


/*---------------------------------------------------------
 *Function:    appex_trans_init_a2
 *Description:   
 *		init aria2 download module
 *Parameters: 
 *		nu	the total task number
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_init_a2( u32 nu )
{
	int ret = 0;
	u32 i;

	//check init flag
	if( init_flag++ )
		return ret;
	if( handles )
		return ret;

	//check nu
	if( nu == 0 )
	{
		nu = tasknum;
	}
	else
	{
		tasknum = nu;
	}

	//init handles struct 
	handles = (struct handle*)malloc( sizeof(struct handle) * nu );
	if( !handles )
	{
		ret = -1;
		apx_wlog( "apx_trans_init_a2: malloc failed", ret );
		return ret;
	}
	for( i = 0; i < nu; i++ )
	{
		handles[i].session = NULL;
		handles[i].thread = 0;
		handles[i].gid = 0;
		handles[i].flag = 0;
		bzero( handles[i].fname, sizeof( handles[i].fname ) );
		handles[i].status = APX_TASK_STATE_STOP;
	}

	//init aria2 lib
	ret = aria2::libraryInit();
	if( ret != 0 )
	{
		ret = -2;
		apx_wlog( "apx_trans_a2 init failed.", ret );
		free( handles );
		handles = NULL;
	}
	return ret;
}


/*---------------------------------------------------------
 *Function:    apx_trans_create_a2
 *Description:   
 *		create the session to aria2
 *Parameters: 
 *		void
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_create_a2( void )
{
	u32 nu;
	int ret = 0;
	aria2::Session* session;
	// session is actually singleton: 1 session per process
	// Create default configuration. The libaria2 takes care of signal
	// handling.
	aria2::SessionConfig config;

	//check init flag
	if( init_flag == 0 )
	{
		ret = -1;
		apx_wlog( "apx_trans_create_a2: You haven`t call init interface", ret );
		return ret;
	}

	pthread_mutex_lock( &handle_lock );
	if( handles )
	{
		for( nu = 0; nu < tasknum; nu ++ )
		{
			if( handles[nu].session == NULL )
			{
				// Add event callback
				config.downloadEventCallback = downloadEventCallback;
				config.userData = (void *)&handles[nu];
				// Create new session to aria2
				session = aria2::sessionNew( aria2::KeyVals(), config );

				// printf("tasknum : %d\n", tasknum);
				handles[nu].session = (u32*)session;
				handles[nu].gid = (uint64_t)( nu +1 );
				handles[nu].flag = 1;
				pthread_mutex_unlock( &handle_lock );

				//printf( "nu: %d session: %ld\n", nu, handles[nu].session );
				return nu;;
			}
		}
	}
	pthread_mutex_unlock( &handle_lock );

	printf( "The array handles have been filled fully.\n" );
	ret = -2;
	apx_wlog( "apx_trans_create_a2: The array handles have been filled fully", ret );

	return ret;
}

/*---------------------------------------------------------
 *Function:    _trans_setopt
 *Description:   
 *		set global and task option, and add url to aria2
 *Parameters: 
 *		nu			task gid
 *		glb_opt		global option
 *		task_opt	task option
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_setopt( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt )
{
	int ret = 0;

	
	if( glb_opt )
	{
		ret = _trans_setglbopt( handles[nu].session, glb_opt );
	}
	if( ret != 0 )
	{
		printf( "Failed to set global option.\n" );
		apx_wlog( "_trans_setopt: Failed to set global opt", ret );
		return -1;
	}

	if( task_opt )
	{
		switch( task_opt->proto )
		{
			case APX_TASK_PROTO_HTTP:
				ret = _trans_addhttp( nu, task_opt );
				break;
			case APX_TASK_PROTO_HTTPS:
				ret = _trans_addhttp( nu, task_opt );
				break;
			case APX_TASK_PROTO_FTP:
				ret = _trans_addftp( nu, task_opt );
				break;
			case APX_TASK_PROTO_BT:
				ret = _trans_addbt( nu, task_opt );
				break;
			default:
				ret = -2;
				apx_wlog( "_trans_setopt: proto is wrong", ret );
				break;
		}
	}

	return ret;
}

/*---------------------------------------------------------
 *Function:    _trans_setopt_a2
 *Description:   
 *		set global options and task op[tions
 *Parameters: 
 *		nu			task gid
 *		glb_opt		global option
 *		task_opt	task option
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
//set global options and task option
static int _trans_setopt_a2( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt )
{
	int ret = 0;
	
	if( nu < 0 || nu > tasknum )
	{
		ret = -1;
		printf( "handles num %u is wrong.\n", nu );
		return ret;
	}
	if( !glb_opt && !task_opt )
	{
		ret = -2;
		apx_wlog( "apx_trans_setopt: Options memery is NULL", ret );
		return ret;	
	}

	
	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -3;
		apx_wlog( "apx_trans_setopt: Task handle is NULL", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}
		
	ret = _trans_setopt( nu, glb_opt, task_opt );
	if( ret != 0 )
	{
		ret = -4;
		apx_wlog( "apx_trans_setopt: Set options failed", ret );
	}

	pthread_mutex_unlock( &handle_lock );
	return ret;
}

/*---------------------------------------------------------
 *Function:    _trans_get_btfile
 *Description:   
 *		parse BT torrent, get bt file names.
 *Parameters: 
 *		task_opt	task info
 *		bt_file		record bt file names
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_get_btfile( struct apx_trans_opt* task_opt, struct btfile* bt_file )
{
	long i;
	FILE *fp = NULL;
	char *metafile_buf = NULL; //save bt file buffer 
	long long metafile_size = 0;//bt torrent size
	int ret = 0;
	int len = 0;
 
	//check input
	if( task_opt == NULL || bt_file == NULL )
	{
		ret = -1;
		apx_wlog( "_trans_get_btfile: input gid is wrong or pointer is NULL", ret );
		return ret;
	}


	if( task_opt->proto != APX_TASK_PROTO_BT )
	{
		ret = -2;
		apx_wlog( "_trans_get_btfile: task proto is not BT", ret );
		return ret;
	}

	fp = fopen( task_opt->uri,"rb" );//open bt torrent
	if( fp == NULL )
	{
		ret = -3;
		apx_wlog( "_trans_get_btfile: open bt torrent failed", ret );
		return ret;
	}
	//get bt torrent len
	fseek( fp, 0, SEEK_END ); //set pointer at the end of file
	metafile_size = ftell( fp );
	if( metafile_size == -1 )
	{
		ret = -4;
		fclose(fp);
		apx_wlog( "_trans_get_btfile: get bt file size failed", ret );
		return ret;
	}
 
	fseek( fp, 0, SEEK_SET ); //move pointer to the head of file
	metafile_buf = (char*)malloc( metafile_size + 1 );
	if( metafile_buf == NULL )
	{
		ret = -5;
		fclose(fp);
		apx_wlog( "_trans_get_btfile: buffer alloc memery failed", ret );
		return ret;
	}
	memset( metafile_buf, 0, metafile_size );
	for( i = 0; i < metafile_size; i++ )
	{
		metafile_buf[i] = fgetc( fp );
	}
	metafile_buf[i] = '\0';
	fclose( fp );
	 
	ret = _get_filepath_name( metafile_buf, metafile_size, bt_file );
	if( ret != 0 )
	{
		ret = -6;
		apx_wlog( "_trans_get_btfile: getting bt file name failed", ret );
		goto out;
	}

out:
	free ( metafile_buf );
	metafile_buf = NULL;

	
	return ret;
}

/*---------------------------------------------------------
 *Function:    apx_trans_getopt_a2
 *Description:   
 *		Get global options and task options
 *Parameters: 
 *		nu			task gid
 *		glb_opt		global option
 *		task_opt	task option
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_getopt_a2( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt )
{
	int ret = 0;
	int i = 0;

	if( nu < 0 || nu > tasknum )
	{
		ret = -1;
		apx_wlog( "apx_trans_getopt_a2: Task nu is wrong", ret );
		return ret;
	}
	if( !glb_opt && !task_opt )
	{
		ret = -2;
		apx_wlog( "apx_trans_getopt_a2: Options memery is NULL", ret );
		return ret;	
	}

	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -3;
		apx_wlog( "apx_trans_getopt_a2: Task handle is NULL", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

	//get global options
	aria2::KeyVals glbopts = getGlobalOptions( (aria2::Session*)handles[nu].session );
	if( glbopts.empty() )
		printf( "global options is NULL.\n" );
	for( i = 0 ; i < (int) glbopts.size() ; i++ )
	{
		if( glbopts[i].first == "dir" )
		{
			strncpy( glb_opt->path, glbopts[i].second.c_str(), sizeof( glb_opt->path ) - 1 );
			strncpy( task_opt->fpath, glbopts[i].second.c_str(), sizeof( glb_opt->path ) - 1 );
		}
		else if( glbopts[i].first == "split" )
			glb_opt->connections = sstoi( glbopts[i].second );
		else if( glbopts[i].first == "max-overall-download-limit" )
			glb_opt->max_limit_downspeed = sstoi( glbopts[i].second );
		else if( glbopts[i].first == "max-overall-upload-limit" )
			glb_opt->max_limit_uploadspeed = sstoi( glbopts[i].second );
		else if( glbopts[i].first == "max-concurrent-downloads" )
			glb_opt->max_concurrent_download = sstoi( glbopts[i].second );
	}

	//get task options
	aria2::DownloadHandle* dh= aria2::getDownloadHandle( (aria2::Session*)handles[nu].session, handles[nu].gid );
	if( dh )
	{
		aria2::KeyVals taskopts = dh->getOptions();
		for( i = 0 ; i < (int) taskopts.size() ; i++ )
		{
			if( taskopts[i].first == "dir" )
				strncpy( task_opt->fpath, taskopts[i].second.c_str(), sizeof( task_opt->fpath ) - 1 );
			else if( taskopts[i].first == "split" )
				task_opt->concurr = sstoi( taskopts[i].second );
			else if( taskopts[i].first == "max-download-limit" )
				task_opt->down_splimit = (u32)sstoi( taskopts[i].second );
			else if( taskopts[i].first == "max-upload-limit" )
				task_opt->up_splimit = (u32)sstoi( taskopts[i].second );
			else if( taskopts[i].first == "out" )
				strncpy( task_opt->fname, taskopts[i].second.c_str(), sizeof( task_opt->fname ) - 1  );
			else if( taskopts[i].first == "select-file" )
				strncpy( task_opt->bt_select, taskopts[i].second.c_str(), sizeof( task_opt->bt_select ) - 1  );
		}
	}
	pthread_mutex_unlock( &handle_lock );

	return ret;
}

/*---------------------------------------------------------
 *Function:    apx_trans_getstat_a2
 *Description:   
 *		get task stat
 *Parameters: 
 *		nu			task gid
 *		task_stat	record task stat struct
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_getstat_a2( u32 nu, struct apx_trans_stat* task_stat )
{
	int ret = 0;

	if( nu < 0 || nu > tasknum )
	{
		ret = -1;
		printf( "apx_trans_getstat_a2: handles num is wrong.\n" );
		return ret;
	}
	if( !task_stat )
	{
		ret = -2;
		apx_wlog( "apx_trans_getstat_a2: Task_stat pointer is NULL.", ret );
		return ret;
	}
	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -3;
		apx_wlog( "apx_trans_getstat_a2: Task handle is NULL.", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}
	

	//get task stat from gid
	ret = _trans_gettaskstat( handles[nu].session, handles[nu].gid, task_stat );
	task_stat->state_event = handles[nu].status;

	pthread_mutex_unlock( &handle_lock );
	return ret;
}

/*---------------------------------------------------------
 *Function:    apx_trans_get_btfile_a2
 *Description:   
 *		parse BT torrent, get bt file names.
 *Parameters: 
 *		nu			task number
 *		task_opt	task info
 *		bt_file		record bt file names
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_get_btfile_a2( u32 nu, struct apx_trans_opt* task_opt, struct btfile* bt_file )
{
	long i;
	FILE *fp = NULL;
	char *metafile_buf = NULL; //save bt file buffer 
	long long metafile_size = 0;//bt torrent size
	int ret = 0;
	int len = 0;
 
	//check input
	if( nu < 0 || nu > tasknum || task_opt == NULL || bt_file == NULL )
	{
		ret = -1;
		apx_wlog( "apx_trans_get_btfile_a2: input gid is wrong or pointer is NULL", ret );
		return ret;
	}

	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -2;
		apx_wlog( "apx_trans_get_btfile_a2: Task handle is NULL", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

	if( task_opt->proto != APX_TASK_PROTO_BT )
	{
		ret = -3;
		apx_wlog( "apx_trans_get_btfile_a2: task proto is not BT", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

	fp = fopen( task_opt->uri,"rb" );//open bt torrent
	if( fp == NULL )
	{
		ret = -4;
		apx_wlog( "apx_trans_get_btfile_a2: open bt torrent failed", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}
	//get bt torrent len
	fseek( fp, 0, SEEK_END ); //set pointer at the end of file
	metafile_size = ftell( fp );
	if( metafile_size == -1 )
	{
		ret = -5;
		fclose( fp );
		apx_wlog( "apx_trans_get_btfile_a2: get bt file size failed", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}
 
	fseek( fp, 0, SEEK_SET ); //move pointer to the head of file
	metafile_buf = (char*)malloc( metafile_size + 1 );
	if( metafile_buf == NULL )
	{
		ret = -6;
		fclose( fp );
		apx_wlog( "apx_trans_get_btfile_a2: buffer alloc memery failed", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}
	memset( metafile_buf, 0, metafile_size );
	for( i = 0; i < metafile_size; i++ )
	{
		metafile_buf[i] = fgetc( fp );
	}
	metafile_buf[i] = '\0';
	fclose( fp );
	 
	ret = _get_filepath_name( metafile_buf, metafile_size, bt_file );
	if( ret != 0 )
	{
		ret = -7;
		apx_wlog( "apx_trans_get_btfile_a2: getting bt file name failed", ret );
		goto out;
	}
	len = sizeof( handles[nu].fname ) - 1;
	strncpy( handles[nu].fname, bt_file->fn, len );
	strncpy( task_opt->fname, bt_file->fn, sizeof( task_opt->fname ) -1 );

out:
	free ( metafile_buf );
	metafile_buf = NULL;

	pthread_mutex_unlock( &handle_lock );
	
	return ret;
}
/*---------------------------------------------------------
 *Function:    apx_trans_precreate_a2
 *Description:
 *		check url
 *Parameters:
 *		task_opt	task info
 *Return:
 *		return 0 if success, other return negative. 
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_precreate_a2( struct apx_trans_opt* task_opt )
{
	int ret = 0;
	u32 nu;
	struct apx_trans_stat* tst;

	//check taskopt pointer
	if( !task_opt )
	{
		ret = -1;
		apx_wlog( "apx_trans_precreate_a2: Task_opt pointer is NULL", ret );
		return ret;
	}

	nu = apx_trans_create_a2();
	if( nu == (u32)-1 )
	{
		ret = -2;
		apx_wlog( "apx_trans_precreate_a2: Create gid failed", ret );
		return ret;
	}

	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -3;
		apx_wlog( "apx_trans_precreate_a2: Task handle is NULL", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

	aria2::KeyVals options;
	tst = (struct apx_trans_stat*)malloc( sizeof( struct apx_trans_stat ) );

	std::string uri( task_opt->uri );
	std::vector<std::string> uris = { uri };

	std::pair <std::string, std::string> option1 ( "dry-run", "true" );
	options.insert( options.begin(), option1 );

	/*add test task to url*/
	ret = aria2::addUri( (aria2::Session*)(handles[nu].session), &handles[nu].gid, uris, options );
	if( ret < 0 )
	{
		ret = -4;
		apx_wlog( "Tpx_trans_precreate_a2: he url is wrong", ret );
		pthread_mutex_unlock( &handle_lock );
		goto out;
	}

	pthread_mutex_unlock( &handle_lock );

	/*sessiion run*/
	ret = apx_trans_start_a2( nu, NULL, NULL );
	if( ret != 0 )
	{
		ret = -5;
		apx_wlog( "apx_trans_precreate_a2: Start test url failed", ret );
		goto out1;
	}

	/*wait session run, to try the connection of url*/
	sleep( 3 );

	/*get task stat for judging the url is available or not*/
	ret = apx_trans_getstat_a2( nu, tst );
	if( ret == 0 )
	{
		if( tst->trans_errno != 0 )
		{
			apx_wlog( "apx_trans_precreate_a2: URL is wrong", tst->trans_errno );
			ret = -6;
			goto out1;
		}
	}
	else
	{
		ret = -7;
		apx_wlog( "apx_trans_precreate_a2: Get task stat failed", ret );
		goto out1;
	}
out1:
	apx_trans_release_a2( nu, 0 );
out:
	free( tst );
	return ret;
}

/*---------------------------------------------------------
 *Function:    threadstart
 *Description:   
 *		thread start running aria2
 *Parameters: 
 *		arg		task gid
 *Return:
 *		void
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static void *threadstart( void *arg )
{
	int ret = 0;

	do
	{
		ret = aria2::run( (aria2::Session*)handles[(u64)arg].session, aria2::RUN_ONCE );
		if( ret == 0 )
		{
			handles[(u64)arg].flag = -1;
			break;
		}
	} while ( handles[(u64)arg].flag == 1 );
}

/*---------------------------------------------------------
 *Function:    apx_trans_start_a2
 *Description:
 *		start download
 *Parameters:
 *		nu			task gid
 *		glb_opt		global option
 *		task_opt	task option
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_start_a2( u32 nu, struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt )
{
	int ret = 0;

	if( nu < 0 || nu > tasknum )
	{
		ret = -1;
		apx_wlog( "apx_trans_start_a2: handles num is wrong", ret );
		return ret;
	}

	if( !glb_opt && !task_opt )
	{
		ret = -2;
		apx_wlog("apx_trans_start_a2: input pointer is NULL", ret );
		return ret;
	}

	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -3;
		apx_wlog( "apx_trans_start_a2: Task handle is NULL", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

	//printf task options for debug
	if( task_opt )
	{
		printf("~~~~~~~~~~~~~~~~TASK OPTIONS~~~~~~~~~~~~~~\n");
		printf("Task_path :               %s\n", task_opt->fpath);
		printf("Task_connects :           %d\n", task_opt->concurr);
		printf("Task_max_down_limit :     %d\n", task_opt->down_splimit);
		printf("Task_max_up_limit :       %d\n", task_opt->up_splimit);
		printf("Task_fname :              %s\n", task_opt->fname);
		printf("Task_bt_select :          %s\n", task_opt->bt_select);
		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	}

	//set global options and task options
	ret = _trans_setopt( nu, glb_opt, task_opt );
	if( ret != 0 )
	{
		ret = -4;
		apx_wlog( "apx_trans_start_a2: Set opt failed before start", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

	//set task status
	handles[nu].status = APX_TASK_STATE_ACTIVE;

	if( handles[nu].thread == 0 )
	{
		//create start thread
		ret = pthread_create( &(handles[nu].thread), NULL, threadstart, (void*)nu );
		if( ret != 0 )
		{
			ret = -5;
			apx_wlog( "apx_trans_start_a2: Thread download_run create failed", ret );
			pthread_mutex_unlock( &handle_lock );
			return ret;
		}
	}

	pthread_mutex_unlock( &handle_lock );
	return ret;
}

/*---------------------------------------------------------
 *Function:    apx_trasn_stop_a2
 *Description:
 *		stop download
 *Parameters:
 *		nu  task gid
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_stop_a2( u32 nu )
{
	int ret = 0;
	
	if( nu < 0 || nu > tasknum )
	{
		ret = -1;
		apx_wlog( "apx_trans_stop_a2: Task gid is wrong", ret );
		return ret;
	}

	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -2;
		apx_wlog( "apx_trans_stop_a2: Task handle is NULL", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

//	std::vector<aria2::A2Gid> gids = aria2::getActiveDownload((aria2::Session*)handles[nu].session);

//	for(auto& gid : gids)
//	{
		ret = _trans_pause( handles[nu].session, handles[nu].gid, NULL );
//		printf("ret : %d.\n", ret);
//		handles[nu].gid = (uint64_t)(nu + 1);
//		handles[nu].flag = 0;
		handles[nu].status = APX_TASK_STATE_STOP;
//	}
	pthread_mutex_unlock( &handle_lock );
	return ret;
}
/*---------------------------------------------------------
 *Function:    apx_trasn_del_file_a2
 *Description:
 *		delelte task file
 *Parameters:
 *		glb_opt		global option
 *		task_opt	task option
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_del_file_a2( struct apx_trans_glboptions* glb_opt, struct apx_trans_opt* task_opt )
{
	int ret = 0;
	char str[1024] = { 0 };
	struct btfile* bt = NULL;

	if( !task_opt )
	{
		ret = -1;
		apx_wlog( "apx_trans_del_file_a2: Task_opt pointer is NULL", ret );
		return ret;
	}

	bt = (struct btfile*)malloc(sizeof(struct btfile));
	if( !bt )
	{
		ret = -2;
		apx_wlog( "apx_trans_del_file_a2: Malloc failed", ret );
		return ret;
	}

	pthread_mutex_lock( &handle_lock );
	ret = _trans_get_btfile( task_opt, bt );
	if( ret == 0 )
	{
		snprintf( str, sizeof(str) - 1, "rm -rf %s/%s", task_opt->fpath, bt->fn );
		system( str );
		snprintf( str, sizeof(str) - 1, "rm -rf %s/%s.aria2", task_opt->fpath, bt->fn );
		system( str );
	}
	else
	{
		snprintf( str, sizeof(str) - 1, "rm -rf %s/%s", task_opt->fpath, task_opt->fname );
		system( str );
		snprintf( str, sizeof(str) - 1, "rm -rf %s/%s.aria2", task_opt->fpath, task_opt->fname );
		system( str );
	}

	free( bt );
	bt = NULL;
	pthread_mutex_unlock( &handle_lock );

	return ret;
}

/*---------------------------------------------------------
 *Function:    _trans_release
 *Description:   
 *		release the session of aria2
 *Parameters: 
 *		nu
 *		flags
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
static int _trans_release( u32 nu, int flags )
{
	int ret = 0;
	int i = 0;

	if( nu < 0 || nu > tasknum )
	{
		ret = -1;
		apx_wlog( "_trans_release: Task gid is wrong", ret );
		return ret;
	}

	if( handles[nu].session == NULL )
	{
		ret = -2;
		apx_wlog( "_trans_release: Task handle is NULL", ret );
		goto out;
	}

	if( flags == 1 )
	{
		//delete aria2 download task
		ret = _trans_remove( handles[nu].session, handles[nu].gid, true );
		if( ret != 0 )
		{
			ret = -3;
			apx_wlog( "_trans_release: Remove task failed", ret );
			goto out;
		}
	}
#if (1)
	//stop thread 
	ret = _trans_shutdown( handles[nu].session, true );
	if( ret != 0 )
	{
		ret = -4;
		apx_wlog( "_trans_release: Shutdown task failed", ret );
		goto out;
	}
	
	for(;;)
	{
		if( handles[nu].flag != -1 )
			usleep( 10000 );
		else 
			break;
	}
#endif
	//shut session
	ret = aria2::sessionFinal( (aria2::Session*)handles[nu].session );
	/*session  release have a question, I will due it in future*/
	if ( ret == -1 )
	{
		ret = -5;
		apx_wlog( "_trans_release: Release session failed", ret );
		goto out;
	}
	else if ( ret == 7 )
	{
		ret = 0;
		apx_wlog( "_trans_release: There were unfinished download", ret );
	}

out:

	handles[nu].session = NULL;
	handles[nu].thread = 0;

	return ret;
}

/*---------------------------------------------------------
 *Function:    apx_trans_release_a2
 *Description:
 *		release the session of aria2
 *Parameters: 
 *		nu		task gid
 *		flags
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_release_a2( u32 nu, int flags )
{
	int ret = 0;

	if( nu < 0 || nu > tasknum )
	{
		ret = -1;
		apx_wlog( "apx_trans_release_a2: Task gid is wrong", ret );
		return ret;
	}

	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -2;
		apx_wlog( "apx_trans_release_a2: Task handle is NULL", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

	ret = _trans_release( nu, flags );
	if( ret < 0 )
	{
		ret = -3;
		apx_wlog( "apx_trans_release_a2: release failed", ret );
	}
	pthread_mutex_unlock( &handle_lock );
	return ret;
}

/*---------------------------------------------------------
 *Function:    apx_trans_delete_a2
 *Description:
 *		delete download task data
 *Parameters:
 *		nu	 task gid
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_delete_a2( u32 nu )
{
	int ret = 0;
	int i = 0;;
	char str[1024] = { 0 };
	char path[128] = { 0 };
	char name[256] = { 0 };

	//check nu
	if( nu < 0 || nu > tasknum )
	{
		ret = -1;
		apx_wlog( "apx_trans_delete_a2: Task gid is wrong", ret );
		return ret;
	}
	pthread_mutex_lock( &handle_lock );
	if( handles[nu].session == NULL )
	{
		ret = -2;
		apx_wlog( "apx_trans_delete_a2: Task handle is NULL", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}

	//get download task path and name
	aria2::KeyVals glbopts = getGlobalOptions( (aria2::Session*)handles[nu].session );
	for( i = 0 ; i < (int) glbopts.size() ; i++ )
	{
		if( glbopts[i].first == "dir" )
		{
			strncpy( path, glbopts[i].second.c_str(), sizeof( path ) - 1 );
			path[ sizeof( path ) - 1 ] = '\0';
		}
	}

	aria2::DownloadHandle* dh= aria2::getDownloadHandle( (aria2::Session*)handles[nu].session, handles[nu].gid );
	if( dh )
	{
		aria2::KeyVals taskopts = dh->getOptions();
		for( i = 0 ; i < (int) taskopts.size() ; i++ )
		{
			if( taskopts[i].first == "dir" )
			{
				strncpy( path, taskopts[i].second.c_str(), sizeof( path ) - 1 );
				path[ sizeof( path ) - 1 ] = '\0';
			}
			else if( taskopts[i].first == "out" )
			{
				strncpy( name, taskopts[i].second.c_str(), sizeof( name ) - 1 );
			}
		}
	}
	
	pthread_mutex_unlock( &handle_lock );

	//release download task to be deleted
	ret = apx_trans_release_a2( nu, 0 );
	if( ret == -1 )
	{
		ret = -3;
		apx_wlog( "apx_trans_delete_a2: Release task handle failed", ret );
		return ret;
	}
	else
		printf( "Release %u handle succeed.\n", nu );

	pthread_mutex_lock( &handle_lock );
	if( strlen(handles[nu].fname) == 0 && strlen(name) == 0 )
	{
		ret = -4;
		apx_wlog( "apx_trans_delete_a2: file name is NULL, con`t delete", ret );
		pthread_mutex_unlock( &handle_lock );
		return ret;
	}
	//delete download data
	if( strlen(handles[nu].fname) ) 
	{
		//printf( "path:%s name:%s\n", path, handles[nu].fname );
		snprintf( str, sizeof(str) - 1, "rm -rf %s/%s", path, handles[nu].fname );
		system( str );
		snprintf( str, sizeof(str) - 1, "rm -rf %s/%s.aria2", path, handles[nu].fname );
		system( str );
	}
	else
	{
		//printf( "path:%s name:%s\n", path, name );
		snprintf( str, sizeof(str) - 1, "rm -rf %s/%s", path, name );
		system( str );
		snprintf( str, sizeof(str) - 1, "rm -rf %s/%s.aria2", path, name );
		system( str );
	}	
	pthread_mutex_unlock( &handle_lock );
	ret = 0;

	return ret;
}

/*---------------------------------------------------------
 *Function:   apx_trans_recv_a2 
 *Description:
 *		call back func
 *Parameters: 
 *       
 *Return:
 *       return 0.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
int apx_trans_recv_a2( u32 nu )
{
	return 0;
}

/*---------------------------------------------------------
 *Function:    apx_trans_exit_a2
 *Description:
 *		quit and  clear module
 *Parameters: 
 *		woid
 *Return:
 *		return 0 if success, other return negative.
 *History:
 *		liuyang		2015-5-19
 *-----------------------------------------------------------*/
void apx_trans_exit_a2( void )
{
	u32 i = 0;
	int ret = 0;

	if( !init_flag  )
		return;
	init_flag = 0;

	pthread_mutex_lock( &handle_lock );
	if( handles )
	{
		for( i; i < tasknum; i++ )
		{
			if( handles[i].session )
			{
				ret = _trans_release( i, 1 );
				if( ret != 0 )
				{
					ret = -1;
					apx_wlog( "apx_trans_exit_a2: Release task handle failed", ret );
				}
			}
		}
		free( handles );
		handles = NULL;
	}
	else
	{
		ret = -2;
		apx_wlog( "apx_trans_exit_a2: handles pointer is NULL", ret );
	}
	aria2::libraryDeinit();
				
	pthread_mutex_unlock( &handle_lock );
}

#ifdef __cplusplus
};
#endif
