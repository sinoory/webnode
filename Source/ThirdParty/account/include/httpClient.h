///////////////////////////////////////////////////////////
// Copyright (c) 2015, ShangHai xxxx Inc.
//
// FileName: httpClient.h
//
// Description:
//
// Created: 2015年04月30日 星期四 09时36分03秒
// Revision: Revision: 1.0
// Compiler: g++
//
///////////////////////////////////////////////////////////
#ifndef __HTTPCLIENT_H__
#define __HTTPCLIENT_H__

#include <string>

class HttpClient{
	public:
		HttpClient();
		virtual ~HttpClient();

	public:
		virtual int file_upload(const std::string &strFilename, const std::string &strUrl, std::string &strResponse){};

		virtual int file_download(const std::string &strFilename, const std::string &strUrl, std::string &strResponse){};

		virtual int make_directory(const std::string &strUrl, std::string &strResponse){};

		virtual int single_file_delete(const std::string &strUrl, std::string &strResponse){};

		virtual int batch_file_delete(const std::string &strUrl, std::string &strResponse){};

		virtual int rename_file(const std::string &strUrl, const std::string &headStr, std::string &strResponse){};

		virtual int get_file_attribute(const std::string &strUrl, std::string &strResponse){};

		virtual int file_move_to(const std::string &strUrl,const std::string &strFilename,std::string &strResponse){};

		virtual int file_copy(const std::string &strUrl,const std::string &strFilename,std::string &strResponse){};
		
		virtual int storage_list(const std::string &strUrl, std::string &strResponse){};

		//
		void setDebug(bool bDebug);

	public:
		bool m_bDebug;

};

#endif
