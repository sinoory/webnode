
/*
 * Copyright (C) 2006, 2007 Apple Inc.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2011 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cmakeconfig.h"

#include "BrowserWindow.h"
#include <errno.h>
#include <gtk/gtk.h>
#include <string.h>
#include <webkit2/webkit2.h>
#include <stdlib.h>
#include <fcntl.h>

#define MINI_BROWSER_ERROR (miniBrowserErrorQuark())

static const gchar **uriArguments = NULL;
static const char *miniBrowserAboutScheme = "minibrowser-about";

typedef enum {
    MINI_BROWSER_ERROR_INVALID_ABOUT_PATH
} MiniBrowserError;

static GQuark miniBrowserErrorQuark()
{
    return g_quark_from_string("minibrowser-quark");
}

static const GOptionEntry commandLineOptions[] =
{
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &uriArguments, 0, "[URL…]" },
    { 0, 0, 0, 0, 0, 0, 0 }
};

static gboolean parseOptionEntryCallback(const gchar *optionNameFull, const gchar *value, WebKitSettings *webSettings, GError **error)
{
    if (strlen(optionNameFull) <= 2) {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED, "Invalid option %s", optionNameFull);
        return FALSE;
    }

    /* We have two -- in option name so remove them. */
    const gchar *optionName = optionNameFull + 2;
    GParamSpec *spec = g_object_class_find_property(G_OBJECT_GET_CLASS(webSettings), optionName);
    if (!spec) {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED, "Cannot find web settings for option %s", optionNameFull);
        return FALSE;
    }

    switch (G_PARAM_SPEC_VALUE_TYPE(spec)) {
    case G_TYPE_BOOLEAN: {
        gboolean propertyValue = !(value && g_ascii_strcasecmp(value, "true") && strcmp(value, "1"));
        g_object_set(G_OBJECT(webSettings), optionName, propertyValue, NULL);
        break;
    }
    case G_TYPE_STRING:
        g_object_set(G_OBJECT(webSettings), optionName, value, NULL);
        break;
    case G_TYPE_INT: {
        glong propertyValue;
        gchar *end;

        errno = 0;
        propertyValue = g_ascii_strtoll(value, &end, 0);
        if (errno == ERANGE || propertyValue > G_MAXINT || propertyValue < G_MININT) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Integer value '%s' for %s out of range", value, optionNameFull);
            return FALSE;
        }
        if (errno || value == end) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Cannot parse integer value '%s' for %s", value, optionNameFull);
            return FALSE;
        }
        g_object_set(G_OBJECT(webSettings), optionName, propertyValue, NULL);
        break;
    }
    case G_TYPE_FLOAT: {
        gdouble propertyValue;
        gchar *end;

        errno = 0;
        propertyValue = g_ascii_strtod(value, &end);
        if (errno == ERANGE || propertyValue > G_MAXFLOAT || propertyValue < G_MINFLOAT) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Float value '%s' for %s out of range", value, optionNameFull);
            return FALSE;
        }
        if (errno || value == end) {
            g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, "Cannot parse float value '%s' for %s", value, optionNameFull);
            return FALSE;
        }
        g_object_set(G_OBJECT(webSettings), optionName, propertyValue, NULL);
        break;
    }
    default:
        g_assert_not_reached();
    }

    return TRUE;
}

static gboolean isValidParameterType(GType gParamType)
{
    return (gParamType == G_TYPE_BOOLEAN || gParamType == G_TYPE_STRING || gParamType == G_TYPE_INT
            || gParamType == G_TYPE_FLOAT);
}

static GOptionEntry* getOptionEntriesFromWebKitSettings(WebKitSettings *webSettings)
{
    GParamSpec **propertySpecs;
    GOptionEntry *optionEntries;
    guint numProperties, numEntries, i;

    propertySpecs = g_object_class_list_properties(G_OBJECT_GET_CLASS(webSettings), &numProperties);
    if (!propertySpecs)
        return NULL;

    optionEntries = g_new0(GOptionEntry, numProperties + 1);
    numEntries = 0;
    for (i = 0; i < numProperties; i++) {
        GParamSpec *param = propertySpecs[i];

        /* Fill in structures only for writable and not construct-only properties. */
        if (!param || !(param->flags & G_PARAM_WRITABLE) || (param->flags & G_PARAM_CONSTRUCT_ONLY))
            continue;

        GType gParamType = G_PARAM_SPEC_VALUE_TYPE(param);
        if (!isValidParameterType(gParamType))
            continue;

        GOptionEntry *optionEntry = &optionEntries[numEntries++];
        optionEntry->long_name = g_param_spec_get_name(param);

        /* There is no easy way to figure our short name for generated option entries.
           optionEntry.short_name=*/
        /* For bool arguments "enable" type make option argument not required. */
        if (gParamType == G_TYPE_BOOLEAN && (strstr(optionEntry->long_name, "enable")))
            optionEntry->flags = G_OPTION_FLAG_OPTIONAL_ARG;
        optionEntry->arg = G_OPTION_ARG_CALLBACK;
        optionEntry->arg_data = parseOptionEntryCallback;
        optionEntry->description = g_param_spec_get_blurb(param);
        optionEntry->arg_description = g_type_name(gParamType);
    }
    g_free(propertySpecs);

    return optionEntries;
}

static gboolean addSettingsGroupToContext(GOptionContext *context, WebKitSettings* webkitSettings)
{
    GOptionEntry *optionEntries = getOptionEntriesFromWebKitSettings(webkitSettings);
    if (!optionEntries)
        return FALSE;

    GOptionGroup *webSettingsGroup = g_option_group_new("websettings",
                                                        "WebKitSettings writable properties for default WebKitWebView",
                                                        "WebKitSettings properties",
                                                        webkitSettings,
                                                        NULL);
    g_option_group_add_entries(webSettingsGroup, optionEntries);
    g_free(optionEntries);

    /* Option context takes ownership of the group. */
    g_option_context_add_group(context, webSettingsGroup);

    return TRUE;
}

static void
aboutURISchemeRequestCallback(WebKitURISchemeRequest *request, gpointer userData)
{
    GInputStream *stream;
    gsize streamLength;
    const gchar *path;
    gchar *contents;
    GError *error;

    path = webkit_uri_scheme_request_get_path(request);
    if (!g_strcmp0(path, "minibrowser")) {
        contents = g_strdup_printf("<html><body><h1>WebKitGTK+ MiniBrowser</h1><p>The WebKit2 test browser of the GTK+ port.</p><p>WebKit version: %d.%d.%d</p></body></html>",
            webkit_get_major_version(),
            webkit_get_minor_version(),
            webkit_get_micro_version());
        streamLength = strlen(contents);
        stream = g_memory_input_stream_new_from_data(contents, streamLength, g_free);

        webkit_uri_scheme_request_finish(request, stream, streamLength, "text/html");
        g_object_unref(stream);
    } else {
        error = g_error_new(MINI_BROWSER_ERROR, MINI_BROWSER_ERROR_INVALID_ABOUT_PATH, "Invalid about:%s page.", path);
        webkit_uri_scheme_request_finish_error(request, error);
        g_error_free(error);
    }
}

static int initCookieManager(WebKitSettings *webkitSettings)
{
    if(!webkitSettings)
        return 0;
    // Enable cooki manager to store cookie message.--- by Jiayu
    WebKitCookieManager* cookiemanager = webkit_web_context_get_cookie_manager(webkit_web_context_get_default());
    int error = 0;
    gchar* home = getenv("HOME");
    gchar cookieDatabasePath[2048];
    g_sprintf(cookieDatabasePath, "%s/.cookie", home);
    if(!g_file_test(cookieDatabasePath, G_FILE_TEST_IS_DIR) || !g_access(cookieDatabasePath, /*S_IWUSR|S_IRUSR*/0755)){
        error = g_mkdir_with_parents(cookieDatabasePath, /*S_IWUSR|S_IRUSR*/ 0755);
    }
    if(!error){
        gchar cookieDatabase[2048];
        g_sprintf(cookieDatabase, "%s/cookie_database", cookieDatabasePath);
        g_printf("cookiedatabase path is %s\n", cookieDatabase);
        webkit_cookie_manager_set_persistent_storage(cookiemanager, cookieDatabase, WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    }else{
        g_printerr("LOG-> Init: Failed to init cookie database\n");
        return 0;
    }
    
    //g_mkdir_with_parents(cookieDatabaePath, );
    //webkit_cookie_manager_set_persistent_storage(cookiemanager, "cookies_database", WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    WebKitCookieAcceptPolicy cookiePolicy = WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS;
    int cookieSetting;
    error = 0;
    g_object_get(webkitSettings,
             key[PROP_COOKIE_SETTING], &cookieSetting,
             NULL);
    switch(cookieSetting){
    case 0:
        cookiePolicy = WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS;
        break;
    case 1:
        cookiePolicy = WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY;
        break;
    case 2:
        cookiePolicy = WEBKIT_COOKIE_POLICY_ACCEPT_NEVER;
        break;
    default:
        error = 1;
        g_printerr("LOG-> Settings: Failed to get the correct cookie setting policy\n");
        break;
    }
    if(error)
        return 0;
    else{
        webkit_cookie_manager_set_accept_policy(cookiemanager,cookiePolicy);
        return 1;
    }
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    g_setenv("WEBKIT_INJECTED_BUNDLE_PATH", WEBKIT_INJECTED_BUNDLE_PATH, FALSE);

    const gchar *multiprocess = g_getenv("MINIBROWSER_MULTIPROCESS");
    if(true){//wangc force multiprocess : (multiprocess && *multiprocess) {
        webkit_web_context_set_process_model(webkit_web_context_get_default(),
            WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES);
    }

    GOptionContext *context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, commandLineOptions, 0);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));

    WebKitSettings *webkitSettings = webkit_settings_new();
    InitSettingsWithFile(webkitSettings);  //Init WebKitSettings With Config File.
    webkit_settings_set_enable_developer_extras(webkitSettings, TRUE);
    webkit_settings_set_enable_webgl(webkitSettings, TRUE);
    if (!addSettingsGroupToContext(context, webkitSettings))
        g_clear_object(&webkitSettings);

    GError *error = 0;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("Cannot parse arguments: %s\n", error->message);
        g_error_free(error);
        g_option_context_free(context);

        return 1;
    }
    g_option_context_free (context);

    g_assert(initCookieManager(webkitSettings));

    // Enable the favicon database, by specifying the default directory.
    webkit_web_context_set_favicon_database_directory(webkit_web_context_get_default(), NULL);

    webkit_web_context_register_uri_scheme(webkit_web_context_get_default(), miniBrowserAboutScheme, aboutURISchemeRequestCallback, NULL, NULL);

    webkit_web_context_set_tls_errors_policy(webkit_web_context_get_default(), WEBKIT_TLS_ERRORS_POLICY_IGNORE);

    if (uriArguments) {
        int i;

        for (i = 0; uriArguments[i]; i++)
            createBrowserWindow(uriArguments[i], webkitSettings);
    } else { 
        //createBrowserWindow("http://www.baidu.com/", webkitSettings);
        //createBrowserWindow("/home/zgh/CuprumBrowser/WebKitBrowser/WebKitBuild/Release/bin/resources/index.html",NULL);
        gchar indexpage[BUFSIZE] = {0};
        gchar *uri;
        gchar *strval;
        int ivalue;
        g_object_get(webkitSettings,
                 key[PROP_ON_STARTUP], &ivalue,
                 NULL);

        //default url:::  http://nfs-cloud.cn:81/appCenter/open/softcenter
        /*Depending on_startup radiobutton to loading corresponding url. sunhaiming add */
        switch (ivalue) {
        case 0:
            strcat(indexpage, get_cuprum_dir());
            strcat(indexpage, "/resources/assets/newtab.html");
            printf("%s\n", indexpage);
            uri = indexpage;
            break;
        case 1:
            uri = "http://nfs-cloud.cn:81/appCenter/open/softcenter";
            break;
        case 2:  
            g_object_get(webkitSettings,
                     key[PROP_HOME_PAGE], &strval,
                     NULL);
            if (!g_strcmp0(strval, "")) {
                uri = "http://nfs-cloud.cn:81/appCenter/open/softcenter";
            } else {
                uri = strval;
            }
            break;
        default:
            printf("error, read PROP_ON_STARTUP  ivalue = %i\n", ivalue);
            break;
        }
        createBrowserWindow(uri, webkitSettings);
    }

    g_clear_object(&webkitSettings);

    gtk_main();

    return 0;
}
