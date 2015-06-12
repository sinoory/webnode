//code in middleware.h
#ifndef __cplusplus
	typedef enum {false, true }bool;
#endif
#ifdef __cplusplus
extern "C" 
{
#endif
/*  #define bool char
  #define ture 1
  #define false 0
*/

 #if 0
//用户注册接口
 bool registerUser(const char* email, const char* passwd);
//请求token接口，输出的token用于下述邮箱同步接口
 char* requestToken(const char* email, const char* passwd);
//邮箱同步接口（上传）
	bool upload(const char* filePath, const char* email, const char* token);
//邮箱同步接口（下载）
	bool download(const char* filePath, const char* email, const char* token);
//获取云端数据列表接口
	char* storageList(const char* email, const char* token);
#endif

//用户注册接口
bool registerUser(const char* username, const char* nickname, const char* email, const char* passwd);
//用户登录接口
//char* login(const char* username, const char* email, const char* passwd);
void login(char** returnstr, const char* username, const char* email, const char* passwd);
//书签同步接口（上传）
bool upload(const char* message, const char* email, const char* token);
//书签同步接口（下载）
bool download(char** output, const char* email, const char* token);

#ifdef __cplusplus
}
#endif

//g++ -fpic -shared -g -o libmiddlewareAPI.so middlewareAPI.cpp   -L/home/lianxx/work/samples2/samples/lib/ -lmiddleware -I/home/lianxx/work/samples2/samples/include/

