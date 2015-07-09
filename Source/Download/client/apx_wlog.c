//=========================================================================
//  The information contained herein is the exclusive property of
//  APPEXNETWORKS Technology Co¡£ And shall not be distributed, reproduced,
//  or disclosed in whole in part without prior written permission
//  (C) COPYRIGHT 2015  APPEXNETWORKS TECHNOLOGY CO
//  ALL RIGHTS RESERVED
//  The entire notice above must be reproduced on all authorized copies
//===========================================================================
//
//  Filename: apx_wlog.c
//  Integrate: jerry.hua    (email: huagb@appexnetworks.com)   
//  Tel: 
//  Date:  2015.01.30
//  Description: Log recording and error handle operation
//               
//  Reference:
//
//  Version history:1.0
//------------------------------------------------------------------------------
//  Version   YYYY-MM-DD-INDEX   Modified By         Description
//------------------------------------------------------------------------------
// none
//===========================================================================
#include  "apx_wlog.h"


static pthread_mutex_t log_mutex;
static FILE *log_fp;

int apx_wlog_init(const char *file)
{
	if (file == NULL)
	{
		printf("no logfile.\n");
		return -1;
	}
	
	pthread_mutex_init(&log_mutex, NULL);

	log_fp = fopen(file,"a");
	if(log_fp == NULL)
	{
		printf("open log file %s failed.\n", file);
		return -2;
	}

	return 0;
}

//clean log_mutex and log_fp
void apx_wlog_release()
{
	pthread_mutex_lock(&log_mutex);
	if(log_fp != NULL)
		fclose(log_fp);
	
	log_fp=NULL;
	pthread_mutex_unlock(&log_mutex);
	pthread_mutex_destroy(&log_mutex);	
}

int apx_wlog(const char* szlog,int errcode)
{
	time_t curr_time;
	
	pthread_mutex_lock(&log_mutex);
	if(log_fp == NULL)
	{
		perror("please open log file first.");
		pthread_mutex_unlock(&log_mutex);
		return -1;
	}

	time(&curr_time);

	if(errcode == 0)
	{
		fprintf(log_fp, "%s\t%s\n", ctime(&curr_time), szlog);
#ifdef DEBUG_LOG
		printf("%s\t%s\n", ctime(&curr_time), szlog);
#endif
	}else{
		fprintf(log_fp, "%s\t%s,errcode is %d\n", ctime(&curr_time),szlog, errcode); 
#ifdef DEBUG_LOG
		printf("%s\t%s,errcode is %d\n", ctime(&curr_time),szlog, errcode); 
#endif
	}
	fflush(log_fp);
	fflush(stdout);

	pthread_mutex_unlock(&log_mutex);
	
	return 0;
}

void apx_wlog_quit(const char* szlog,int errcode)
{
	apx_wlog(szlog, errcode);
	apx_wlog_release();
	exit(errcode);
}

//write log with variable-length parameter
int apx_vlog(const char* fmt, ...)
{
	time_t curr_time;
	char tmp_buf[BUF_LEN] = {0};
	int tmp_len = 0;
	va_list ap;

	if(!fmt)
		return -1;
	
	pthread_mutex_lock(&log_mutex);
	if(log_fp == NULL)
	{
		perror("please open log file first.");
		pthread_mutex_unlock(&log_mutex);
		return -1;
	}

	time(&curr_time);
	va_start(ap, fmt);

	tmp_len = vsnprintf(tmp_buf, sizeof(tmp_buf)-1, fmt, ap);
	fprintf(log_fp, "%s\t%s\n", ctime(&curr_time), tmp_buf);
#ifdef DEBUG_LOG
		printf("%s\t%s\n", ctime(&curr_time), tmp_buf);
#endif

	fflush(log_fp);
	fflush(stdout);

	pthread_mutex_unlock(&log_mutex);
	
	return 0;
}

