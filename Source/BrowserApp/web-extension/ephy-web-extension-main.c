/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=2 sts=2 et: */
/*
 *  Copyright © 2012 Igalia S.L.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include "ephy-web-extension.h"
#include "ephy-web-extension-names.h"
//#include "ephy-debug.h"
//#include "ephy-file-helpers.h"

static void
name_acquired_cb (GDBusConnection *connection,
                  const char *name,
                  EphyWebExtension *extension)
{
  ephy_web_extension_dbus_register (extension, connection);
}

G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data(WebKitWebExtension *extension, GVariant* user_data)
{
  EphyWebExtension *web_extension;
  char *service_name;
  const char *extension_id;
  gboolean is_save_pass_word;
  GError *error = NULL;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  g_variant_get (user_data, "(&sb)", &extension_id, &is_save_pass_word);
  //add by luyue 2015/3/27
  gchar* home = getenv("HOME");
  gchar user_path[2048];
  g_sprintf(user_path, "%s/.config/cdosbrowser/config", home);
  FILE * fp = fopen(user_path, "r");
  if(fp)
  {
     while ((read = getline(&line, &len, fp)) != -1)
     {
        if(strncmp(line,"remember-web-password=true",20)==0)
        {
           is_save_pass_word = true;
           break;
        }
        free(line);
        line = NULL;
     }
     if(line)
     {
        free(line);
        line = NULL;
     }
     fclose(fp);
  }
  printf("is_save_pass_word==%d\n",is_save_pass_word);
  if(is_save_pass_word) return;
  //add end

  web_extension = ephy_web_extension_get ();
  ephy_web_extension_initialize (web_extension, extension, /*dot_dir*/NULL, is_save_pass_word);

  service_name = g_strdup_printf ("%s-%s", EPHY_WEB_EXTENSION_SERVICE_NAME, extension_id);
  g_bus_own_name (G_BUS_TYPE_SESSION,
                  service_name,
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  NULL,
                  (GBusNameAcquiredCallback)name_acquired_cb,
                  NULL,
                  web_extension, NULL);
  g_free (service_name);
}

static void __attribute__((destructor))
ephy_web_extension_shutdown (void)
{
  g_object_unref (ephy_web_extension_get ());
}
