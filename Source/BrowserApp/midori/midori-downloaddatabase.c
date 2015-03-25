/* midori-downloaddatabase.c generated by valac 0.22.1, the Vala compiler
 * generated from midori-downloaddatabase.vala, do not modify */

/*
 Copyright (C) 2013 Christian Dywan <christian@twotoats.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>


#define MIDORI_TYPE_DATABASE (midori_database_get_type ())
#define MIDORI_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIDORI_TYPE_DATABASE, MidoriDatabase))
#define MIDORI_DATABASE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MIDORI_TYPE_DATABASE, MidoriDatabaseClass))
#define MIDORI_IS_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIDORI_TYPE_DATABASE))
#define MIDORI_IS_DATABASE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIDORI_TYPE_DATABASE))
#define MIDORI_DATABASE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MIDORI_TYPE_DATABASE, MidoriDatabaseClass))

typedef struct _MidoriDatabase MidoriDatabase;
typedef struct _MidoriDatabaseClass MidoriDatabaseClass;
typedef struct _MidoriDatabasePrivate MidoriDatabasePrivate;

#define MIDORI_TYPE_DOWNLOAD_DATABASE (midori_download_database_get_type ())
#define MIDORI_DOWNLOAD_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIDORI_TYPE_DOWNLOAD_DATABASE, MidoriDownloadDatabase))
#define MIDORI_DOWNLOAD_DATABASE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MIDORI_TYPE_DOWNLOAD_DATABASE, MidoriDownloadDatabaseClass))
#define MIDORI_IS_DOWNLOAD_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIDORI_TYPE_DOWNLOAD_DATABASE))
#define MIDORI_IS_DOWNLOAD_DATABASE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIDORI_TYPE_DOWNLOAD_DATABASE))
#define MIDORI_DOWNLOAD_DATABASE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MIDORI_TYPE_DOWNLOAD_DATABASE, MidoriDownloadDatabaseClass))

typedef struct _MidoriDownloadDatabase MidoriDownloadDatabase;
typedef struct _MidoriDownloadDatabaseClass MidoriDownloadDatabaseClass;
typedef struct _MidoriDownloadDatabasePrivate MidoriDownloadDatabasePrivate;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define MIDORI_TYPE_DATABASE_STATEMENT (midori_database_statement_get_type ())
#define MIDORI_DATABASE_STATEMENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIDORI_TYPE_DATABASE_STATEMENT, MidoriDatabaseStatement))
#define MIDORI_DATABASE_STATEMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MIDORI_TYPE_DATABASE_STATEMENT, MidoriDatabaseStatementClass))
#define MIDORI_IS_DATABASE_STATEMENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIDORI_TYPE_DATABASE_STATEMENT))
#define MIDORI_IS_DATABASE_STATEMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIDORI_TYPE_DATABASE_STATEMENT))
#define MIDORI_DATABASE_STATEMENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MIDORI_TYPE_DATABASE_STATEMENT, MidoriDatabaseStatementClass))

typedef struct _MidoriDatabaseStatement MidoriDatabaseStatement;
typedef struct _MidoriDatabaseStatementClass MidoriDatabaseStatementClass;

typedef enum  {
	MIDORI_DATABASE_ERROR_OPEN,
	MIDORI_DATABASE_ERROR_NAMING,
	MIDORI_DATABASE_ERROR_FILENAME,
	MIDORI_DATABASE_ERROR_EXECUTE,
	MIDORI_DATABASE_ERROR_COMPILE,
	MIDORI_DATABASE_ERROR_TYPE
} MidoriDatabaseError;
#define MIDORI_DATABASE_ERROR midori_database_error_quark ()
struct _MidoriDatabase {
	GObject parent_instance;
	MidoriDatabasePrivate * priv;
	gboolean trace;
	sqlite3* _db;
};

struct _MidoriDatabaseClass {
	GObjectClass parent_class;
	gboolean (*init) (MidoriDatabase* self, GCancellable* cancellable, GError** error);
};

struct _MidoriDownloadDatabase {
	MidoriDatabase parent_instance;
	MidoriDownloadDatabasePrivate * priv;
};

struct _MidoriDownloadDatabaseClass {
	MidoriDatabaseClass parent_class;
};


static gpointer midori_download_database_parent_class = NULL;

GType midori_database_get_type (void) G_GNUC_CONST;
GQuark midori_database_error_quark (void);
GType midori_download_database_get_type (void) G_GNUC_CONST;
enum  {
	MIDORI_DOWNLOAD_DATABASE_DUMMY_PROPERTY
};
MidoriDownloadDatabase* midori_download_database_new (GObject* app, GError** error);
MidoriDownloadDatabase* midori_download_database_construct (GType object_type, GObject* app, GError** error);
gboolean midori_database_init (MidoriDatabase* self, GCancellable* cancellable, GError** error);
gboolean midori_download_database_clear (MidoriDownloadDatabase* self, GError** error);
GType midori_database_statement_get_type (void) G_GNUC_CONST;
MidoriDatabaseStatement* midori_database_prepare (MidoriDatabase* self, const gchar* query, GError** error, ...);
gboolean midori_database_statement_exec (MidoriDatabaseStatement* self, GError** error);


MidoriDownloadDatabase* midori_download_database_construct (GType object_type, GObject* app, GError** error) {
	MidoriDownloadDatabase * self = NULL;
	GError * _inner_error_ = NULL;
#line 15 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	self = (MidoriDownloadDatabase*) g_object_new (object_type, "path", "download.db", NULL);
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	midori_database_init ((MidoriDatabase*) self, NULL, &_inner_error_);
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	if (_inner_error_ != NULL) {
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
		if (_inner_error_->domain == MIDORI_DATABASE_ERROR) {
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_propagate_error (error, _inner_error_);
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			_g_object_unref0 (self);
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			return NULL;
#line 122 "midori-downloaddatabase.c"
		} else {
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_clear_error (&_inner_error_);
#line 16 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			return NULL;
#line 130 "midori-downloaddatabase.c"
		}
	}
#line 14 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	return self;
#line 135 "midori-downloaddatabase.c"
}


MidoriDownloadDatabase* midori_download_database_new (GObject* app, GError** error) {
#line 14 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	return midori_download_database_construct (MIDORI_TYPE_DOWNLOAD_DATABASE, app, error);
#line 142 "midori-downloaddatabase.c"
}


gboolean midori_download_database_clear (MidoriDownloadDatabase* self, GError** error) {
	gboolean result = FALSE;
	const gchar* sqlcmd = NULL;
	MidoriDatabaseStatement* statement = NULL;
	const gchar* _tmp0_ = NULL;
	MidoriDatabaseStatement* _tmp1_ = NULL;
	gboolean _tmp2_ = FALSE;
	MidoriDatabaseStatement* _tmp3_ = NULL;
	gboolean _tmp4_ = FALSE;
	GError * _inner_error_ = NULL;
#line 19 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	g_return_val_if_fail (self != NULL, FALSE);
#line 20 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	sqlcmd = "\n" \
"                DELETE FROM download WHERE create_time IN (SELECT crea" \
"te_time FROM download ORDER BY create_time DESC LIMIT 100,-1);\n" \
"                ";
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	_tmp0_ = sqlcmd;
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	_tmp1_ = midori_database_prepare ((MidoriDatabase*) self, _tmp0_, &_inner_error_, NULL);
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	statement = _tmp1_;
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	if (_inner_error_ != NULL) {
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
		if (_inner_error_->domain == MIDORI_DATABASE_ERROR) {
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_propagate_error (error, _inner_error_);
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			return FALSE;
#line 174 "midori-downloaddatabase.c"
		} else {
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_clear_error (&_inner_error_);
#line 23 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			return FALSE;
#line 182 "midori-downloaddatabase.c"
		}
	}
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	_tmp3_ = statement;
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	_tmp4_ = midori_database_statement_exec (_tmp3_, &_inner_error_);
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	_tmp2_ = _tmp4_;
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	if (_inner_error_ != NULL) {
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
		if (_inner_error_->domain == MIDORI_DATABASE_ERROR) {
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_propagate_error (error, _inner_error_);
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			_g_object_unref0 (statement);
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			return FALSE;
#line 201 "midori-downloaddatabase.c"
		} else {
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			_g_object_unref0 (statement);
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			g_clear_error (&_inner_error_);
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
			return FALSE;
#line 211 "midori-downloaddatabase.c"
		}
	}
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	result = _tmp2_;
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	_g_object_unref0 (statement);
#line 24 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	return result;
#line 220 "midori-downloaddatabase.c"
}


static void midori_download_database_class_init (MidoriDownloadDatabaseClass * klass) {
#line 13 "/home/luyue/new7/Webkit2Browser/Source/BrowserApp/midori/midori-downloaddatabase.vala"
	midori_download_database_parent_class = g_type_class_peek_parent (klass);
#line 227 "midori-downloaddatabase.c"
}


static void midori_download_database_instance_init (MidoriDownloadDatabase * self) {
}


GType midori_download_database_get_type (void) {
	static volatile gsize midori_download_database_type_id__volatile = 0;
	if (g_once_init_enter (&midori_download_database_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (MidoriDownloadDatabaseClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) midori_download_database_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (MidoriDownloadDatabase), 0, (GInstanceInitFunc) midori_download_database_instance_init, NULL };
		GType midori_download_database_type_id;
		midori_download_database_type_id = g_type_register_static (MIDORI_TYPE_DATABASE, "MidoriDownloadDatabase", &g_define_type_info, 0);
		g_once_init_leave (&midori_download_database_type_id__volatile, midori_download_database_type_id);
	}
	return midori_download_database_type_id__volatile;
}



