//add by luyue

//实现p12格式证书，保存密码，查询密码，删除密码功能。

#ifndef CERTIFICATE_PASSWORD_H
#define CERTIFICATE_PASSWORD_H

#define SECRET_API_SUBJECT_TO_CHANGE

#include <glib.h>
#include <libsecret/secret.h>

#define FILENAME_KEY      "filename"

const SecretSchema *certificate_get_password_schema (void) G_GNUC_CONST;
#define CERTIFICATE_GET_PASSWORD_SCHEMA certificate_get_password_schema ()


//密码保存
void certificate_secret_password_store (const char *filename,
                                        const char *password);

//密码查询
char* certificate_secret_password_lookup (const char *filename);

//密码删除
void certificate_secret_password_removed (const char *filename);


#endif
