//add by luyue

#include "Certificate_Password.h"
#include <glib/gi18n.h>
#include <libsoup/soup.h>

const SecretSchema *
certificate_get_password_schema (void)
{
  static const SecretSchema schema = {
    "org.certificate.FormPassword", SECRET_SCHEMA_NONE,
    {
      { FILENAME_KEY, SECRET_SCHEMA_ATTRIBUTE_STRING },
      { "NULL", 0 },
    }
  };
  return &schema;
}

static void
certificate_password_stored (GObject *source,
                             GAsyncResult *result,
                             gpointer unused)
{
   GError *error = NULL;
   secret_password_store_finish (result, &error);
   if (error != NULL) 
      g_error_free (error);
}


void
certificate_secret_password_store   (const char *filename,
                                     const char *password)
{
   char *label;

   g_return_if_fail (password);
   g_return_if_fail (filename);
        
   label = g_strdup_printf ("Password for %s",filename); 
   secret_password_store (CERTIFICATE_GET_PASSWORD_SCHEMA, SECRET_COLLECTION_DEFAULT,
                          label, password, NULL, certificate_password_stored,NULL,
                          FILENAME_KEY,filename,  
                          NULL);
}

char *
certificate_secret_password_lookup (const char *filename)
{
   gchar *password = NULL;
   GError *error = NULL;

   g_return_if_fail (filename);

   password = secret_password_lookup_sync (CERTIFICATE_GET_PASSWORD_SCHEMA, NULL,&error,
                                           FILENAME_KEY,filename, 
                                           NULL);
   if (error != NULL)
      g_error_free (error);

   return password;
}

static void
certificate_password_clear (GObject *source,
                            GAsyncResult *result,
                            gpointer unused)
{
   GError *error = NULL;
   gboolean removed = secret_password_clear_finish (result, &error);

   if (error != NULL) 
       g_error_free (error);
}

void 
certificate_secret_password_removed (const char *filename)
{
   g_return_if_fail (filename);
   secret_password_clear (CERTIFICATE_GET_PASSWORD_SCHEMA, NULL, 
                          certificate_password_clear, NULL,
                          FILENAME_KEY,filename, 
                          NULL);
}



