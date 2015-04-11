#ifndef PASSWORD_MANAGER_H
#define PASSWORD_MANAGER_H

#include<webkit2/webkit2.h>


#define SECRET_API_SUBJECT_TO_CHANGE

#include<glib-object.h>
#include <libsecret/secret.h>
#include<glib.h>

#define URI_KEY           "uri"
#define FORM_USERNAME_KEY "form_username"
#define FORM_PASSWORD_KEY "form_password"
#define USERNAME_KEY      "username"

#define PASSWORD_MANAGER_TYPE		(password_manager_get_type())
#define PASSWORD_MANAGER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), PASSWORD_MANAGER_TYPE, PasswordManager))
#define PASSWORD_MANAGER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), PASSWORD_MANAGER_TYPE, PasswordManagerClass))
#define IS_PASSWORD_MANAGER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), PASSWORD_MANAGER_TYPE))
#define IS_PASSWORD_MANAGER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), PASSWORD_MANAGER_TYPE))
#define PASSWORD_MANAGER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), PASSWORD_MANAGER_TYPE, PasswordManagerClass))

typedef struct _PasswordManager			PasswordManager;
typedef struct _PasswordManagerClass		PasswordManagerClass;
typedef struct _PasswordManagerPrivate		PasswordManagerPrivate;

struct _PasswordManager{
	GObject parent;
	PasswordManagerPrivate* priv;
};

struct _PasswordManagerClass{
	GObjectClass parent_class;
};

#define EPHY_FORM_PASSWORD_SCHEMA ephy_form_auth_data_get_password_schema ()

GType			password_manager_get_type		(void) G_GNUC_CONST;;
PasswordManager*	password_manager_get_default		(void);
void			password_manager_display		(void);
void                    clear_password_all                      (void);

#endif /* PASSWORD_MANAGER_H */
