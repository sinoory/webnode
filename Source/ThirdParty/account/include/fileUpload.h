///////////////////////////////////////////////////////////
// Copyright (c) 2015, ShangHai xxxx Inc.
//
// FileName: fileUpload.h
//
// Description:
//
// Created: 2015年05月04日 星期一 09时37分58秒
// Revision: Revision: 1.0
// Compiler: g++
//
///////////////////////////////////////////////////////////

#ifndef __FILEUPLOAD_H__
#define __FILEUPLOAD_H__

#include "httpClient.h"

class FileUpload :public HttpClient{
	public:
		FileUpload();
		~FileUpload();

	public:
		virtual int file_upload(const std::string &strFilename, const std::string &strUrl, std::string &strResponse);

};

#endif
