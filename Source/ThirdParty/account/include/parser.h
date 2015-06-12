///////////////////////////////////////////////////////////
// Copyright (c) 2015, ShangHai xxxx Inc.
//
// FileName: parser.h
//
// Description:
//
// Created: 2015年04月29日 星期三 16时30分42秒
// Revision: Revision: 1.0
// Compiler: g++
//
///////////////////////////////////////////////////////////
#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <list>
#include <map>

class Parser{
	public:
		Parser();
		~Parser();
	
	public:

#if 0
		bool parser_json_comm(std::string &token,
				std::string &tenant,
				std::list<std::string> &sourceList,
				std::map<std::string, std::string> &destMap,
				const std::string &strJson);
#endif

		bool parser_json_comm(std::string &token,
				std::list<std::string> &sourceList,
				std::map<std::string, std::string> &destMap,
				const std::string &strJson);
		// you can custom json parse here
		//bool parse_json_custom(void){};
	
		std::string parser_json_string(const std::string &strJson, const std::string &key);

	private:

		std::string m_strJson;
		std::string m_token;
		std::string m_tenant;
		//std::string m_source;
		std::string m_destination;
		std::list<std::string> m_sourceList;
		//std::string m_method;

};

#endif //__PARSER_H__
