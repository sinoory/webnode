///////////////////////////////////////////////////////////
// Copyright (c) 2015, ShangHai xxxx Inc.
//
// FileName: fileDownload.h
//
// Description:
//
// Created: 2015年05月04日 星期一 13时53分18秒
// Revision: Revision: 1.0
// Compiler: g++
//
///////////////////////////////////////////////////////////

#ifndef __FILEDOWNLOAD_H__
#define __FILEDOWNLOAD_H__

#include "httpClient.h"

class FileDownload :public HttpClient{
	public:
		FileDownload();
		~FileDownload();

	public:
		virtual int file_download(const std::string &strFilename, const std::string &strUrl, std::string &strResponse);

};

#endif
