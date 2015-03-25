/*

*/

#include "midori/midori-download-db.h"

#include <glib/gi18n-lib.h>
#include <midori/cdosbrowser-core.h>

static void
midori_download_db_clear_cb (KatzeArray* array,
                         sqlite3*    db)
{
    GError* error = NULL;
    MidoriDownloadDatabase* database = midori_download_database_new (NULL, &error);
    if (error == NULL)
        midori_download_database_clear (database, &error);
    if (error != NULL)
    {
        g_printerr (_("Failed to clear download: %s\n"), error->message);
        g_error_free (error);
    }
    g_object_unref (database);
}

KatzeArray*
midori_download_db_new (char** errmsg)
{
    MidoriDownloadDatabase* database;
    GError* error = NULL;
    sqlite3* db;
    KatzeArray* array;

    g_return_val_if_fail (errmsg != NULL, NULL);

    database = midori_download_database_new (NULL, &error);
    if (error != NULL)
    {
        *errmsg = g_strdup (error->message);
        g_error_free (error);
        return NULL;
    }

    db = midori_database_get_db (MIDORI_DATABASE (database));
    g_return_val_if_fail (db != NULL, NULL);

    array = katze_array_new (KATZE_TYPE_ARRAY);
    g_object_set_data (G_OBJECT (array), "db", db);
    g_signal_connect (array, "clear",
                      G_CALLBACK (midori_download_db_clear_cb), db);
    return array;
}

void
midori_download_db_on_quit (KatzeArray*        array)
{
    GError* error = NULL;
    MidoriDownloadDatabase* database = midori_download_database_new (NULL, &error);
    if (error == NULL)
        midori_download_database_clear (database, &error);
    if (error != NULL)
    {
        /* i18n: Couldn't remove items that are older than n days */
        g_printerr (_("Failed to remove old download items: %s\n"), error->message);
        g_error_free (error);
    }
    g_object_unref (database);
}

