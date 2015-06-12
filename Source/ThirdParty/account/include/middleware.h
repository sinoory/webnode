                                                                                                                                                                                                                                                                                                                                                                                                                                                               ///////////////////////////////////////////////////////////
// Copyright (c) 2015, ShangHai xxxx Inc.
//
// FileName: middleware.h
//
// Description:
//
// Created: 2015年04月29日 星期三 16时36分59秒
// Revision: Revision: 1.0
// Compiler: g++
//
///////////////////////////////////////////////////////////
#ifndef __MIDDLEWARE_H__
#define __MIDDLEWARE_H__

#include <string>

class MiddleWare{
	public:
		MiddleWare();
		~MiddleWare();
	
	public:
		bool data_pipeline(const std::string &method, const std::string &strJson, std::string &strResponse);

		// for broswer app
		int http_method(const std::string &token, const std::string &strUrl, std::string &strResponse);
		int https_post(const std::string &strUrl, const std::string &strPost, std::string &strResponse, const char *pCaPath);
		bool registerUser(const std::string &username, const std::string &nickname, const std::string &email, const std::string &passwd);
		std::string login(const std::string &username, const std::string &email, const std::string &passwd);
		bool upload(const std::string &message, const std::string &email, const std::string &token);
		bool download(std::string &output, const std::string &email, const std::string &token);
};

#endif //__MIDDLEWARE_H__
