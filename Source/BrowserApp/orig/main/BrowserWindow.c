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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(HAVE_CONFIG_H) && HAVE_CONFIG_H && defined(BUILDING_WITH_CMAKE)
#include "cmakeconfig.h"
#endif
#include "BrowserWindow.h"

//by sunh
//#include "BrowserDownloadsBar.h"
#include "BrowserDownloadDialog.h"
//by sunh end

#include "Certificate.h" 
#include "BrowserSearchBar.h"
#include "BrowserSettingsDialog.h"
#include "BrowserMenuBar.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "TabMng.h"
#include "HistoryMng.h" //add by zlf
enum {
    PROP_0,

    PROP_VIEW
};


struct _BrowserWindow {
    GtkWindow parent;

    GtkAccelGroup *accelGroup;
    BrowserMenuBar *menubar; //by zgh
    GtkWidget *mainBox;
    GtkWidget *toolbar;
    GtkWidget *bookmarkbar;  //by zgh
    GtkWidget *bookmarkbox;
    GtkWidget *status_bar;   //by zgh
    GtkWidget *uriEntry;
    GtkWidget *backItem;
    GtkWidget *forwardItem;
    GtkWidget *zoomInItem;
    GtkWidget *zoomOutItem;
    //by sunh
    GtkWidget *downloadDialog;
    WebKitDownload *download;
    //by sunh end
    GtkWidget *statusLabel;
    GtkWidget *settingsWindow; //by sunhaiming
    WebKitWebView *webView;
    GtkWidget *downloadsBar;
    BrowserSearchBar *searchBar;
    gboolean searchBarVisible;
    gboolean inspectorWindowIsVisible;
    GdkPixbuf *favicon;
    GtkWidget *reloadOrStopButton;
    GtkWidget *homeButton;
    GtkWidget *fullScreenMessageLabel;
    GtkWindow *parentWindow;
    guint fullScreenMessageLabelId;
    guint resetEntryProgressTimeoutId;
    TabMng* tabmng;
    GtkWidget *overlay;
    GtkWidget *boxtab;
};

struct _BrowserWindowClass {
    GtkWindowClass parent;
};

//static const char *defaultWindowTitle = "WebKitGTK+ MiniBrowser";
static const char *defaultWindowTitle = "Cuprum Browser";
static const char *miniBrowserAboutScheme = "minibrowser-about";
/*minimumZoomLevel and maximumZoomLevel should be initialized depending on zoom_factor[]. sunhaiming add.*/
//static const gdouble minimumZoomLevel = 0.5;   
//static const gdouble maximumZoomLevel = 3;
static  gdouble minimumZoomLevel;   
static  gdouble maximumZoomLevel;
static const gdouble zoomStep = 1.2;
static gint windowCount = 0;

G_DEFINE_TYPE(BrowserWindow, browser_window, GTK_TYPE_WINDOW)


gchar* get_cuprum_dir() 
{
    gchar *tail=NULL; 
    gchar *exec_name; 
    gchar *edir_name; 
    exec_name=(gchar *)calloc(sizeof(gchar),BUFSIZE); 
    edir_name=(gchar *)calloc(sizeof(gchar),BUFSIZE); 
     
    readlink("/proc/self/exe", exec_name, BUFSIZE); 
 
    tail=strrchr(exec_name,'//'); 
    memcpy(edir_name,exec_name,tail-exec_name); 
 
    if(exec_name)   free(exec_name); 
    
    return edir_name;
}

/*
  @Description: 从一个图片中获取信息得到pixbuf
  @param:       gchar filename
*/
GdkPixbuf *create_pixbuf(const gchar * filename)
{
    GdkPixbuf *pixbuf;
    GError *error = NULL;
    /*
     * 函数gdk_pixbuf_new_from_file() 从一个图片文件中加载图象数据，从而生成一个新的 pixbuf，
     * 至于文件中包含图象的格式，是由系统自动检测的。如果该函数返回值是NULL 的话，程序就会出现错误。
    */
    pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    if(!pixbuf) {
        fprintf(stderr, "%s\n", error->message);
        g_error_free(error);
    }
    return pixbuf;
}

gchar *argumentToURL(const char *filename)
{
    if (!strcmp(filename, ""))
        return NULL;
    GFile *gfile = g_file_new_for_commandline_arg(filename);
    gchar *fileURL = g_file_get_uri(gfile);
    g_object_unref(gfile);

    return fileURL;
}

void createBrowserWindow(const gchar *uri, WebKitSettings *webkitSettings)
{
    GtkWidget *webView = webkit_web_view_new();
    GtkWidget *mainWindow = browser_window_new(WEBKIT_WEB_VIEW(webView), NULL);
    gtk_widget_set_sensitive(BROWSER_WINDOW(mainWindow)->backItem, FALSE);
    gtk_widget_set_sensitive(BROWSER_WINDOW(mainWindow)->forwardItem, FALSE);
    gchar *url = argumentToURL(uri);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "Cuprum Browser");

    if (webkitSettings)
        webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView), webkitSettings);

    browser_window_load_uri(BROWSER_WINDOW(mainWindow), url);
    g_free(url);

//zgh    gtk_widget_grab_focus(webView);
    
    gtk_widget_show(mainWindow);
}

static char *getInternalURI(const char *uri)
{
    // Internally we use minibrowser-about: as about: prefix is ignored by WebKit.
    if (g_str_has_prefix(uri, "about:") && !g_str_equal(uri, "about:blank"))
        return g_strconcat(miniBrowserAboutScheme, uri + strlen ("about"), NULL);

    return g_strdup(uri);
}

static char *getExternalURI(const char *uri)
{
    // From the user point of view we support about: prefix.
    if (g_str_has_prefix(uri, miniBrowserAboutScheme))
        return g_strconcat("about", uri + strlen(miniBrowserAboutScheme), NULL);

    return g_strdup(uri);
}

static void browserWindowSetStatusText(BrowserWindow *window, const char *text)
{
//    g_print("text: %s\n", text);
    gtk_label_set_text(GTK_LABEL(window->statusLabel), text);
    gtk_widget_set_visible(window->statusLabel, !!text);
//    g_print("visible: %d\n", gtk_widget_get_visible(window->statusLabel));
}

static void resetStatusText(GtkWidget *widget, BrowserWindow *window)
{
    browserWindowSetStatusText(window, NULL);
}

static void activateUriEntryCallback(BrowserWindow *window)
{
    browser_window_load_uri(window, gtk_entry_get_text(GTK_ENTRY(window->uriEntry)));
}

static void reloadOrStopCallback(BrowserWindow *window)
{
    if (webkit_web_view_is_loading(window->webView))
        webkit_web_view_stop_loading(window->webView);
    else
        webkit_web_view_reload(window->webView);
}

static void goBackCallback(BrowserWindow *window)
{
    webkit_web_view_go_back(window->webView);
}

static void goForwardCallback(BrowserWindow *window)
{
    webkit_web_view_go_forward(window->webView);
}

/**
  add by luyue
  @certificateData: return location for a #GTlsCertificate
  Function:
       return single certificate data by libsoup;
       get single certificate detail information;
       display single certificata detail information;
**/
static void certificateCallback(BrowserWindow *window, GtkEntryIconPosition iconPosition, GdkEvent *event)
{
    if (iconPosition == GTK_ENTRY_ICON_PRIMARY) 
    {
        gchar* certificateData=NULL;
       //return single certificate data by libsoup
       webkit_web_view_certificate(window->webView,&certificateData);
       if(certificateData)
       {
          //get single certificate detail information; 
          get_certificate_data(certificateData);
          //display single certificata detail information;
          display_certificate_info();
       }
    }
    
}

static void settingsCallback(BrowserWindow *window)
{
    if (window->settingsWindow) {
        gtk_window_present(GTK_WINDOW(window->settingsWindow));
        return;
    }
    WebKitSettings *settings = webkit_web_view_get_settings(window->webView);
    settings->parent_uriEntry = window->uriEntry;  //Save to settings, so we can use later. sunhaimig add.

    window->settingsWindow = browser_settings_window_new(settings); 
    gtk_window_set_transient_for(GTK_WINDOW(window->settingsWindow), GTK_WINDOW(window));
    g_object_add_weak_pointer(G_OBJECT(window->settingsWindow), (gpointer *)&window->settingsWindow);
}

static void webViewURIChanged(WebKitWebView *webView, GParamSpec *pspec, BrowserWindow *window)
{
    char *externalURI = getExternalURI(webkit_web_view_get_uri(webView));
    if (!strstr(externalURI, "resources/assets/newtab.html"))
        gtk_entry_set_text(GTK_ENTRY(window->uriEntry), externalURI);
    else
        gtk_entry_set_text(GTK_ENTRY(window->uriEntry), "");
    g_free(externalURI);
}

static void webViewTitleChanged(WebKitWebView *webView, GParamSpec *pspec, BrowserWindow *window)
{
    const char *title = webkit_web_view_get_title(webView);
    gtk_window_set_title(GTK_WINDOW(window), title ? title : defaultWindowTitle);
//   HS_history_new(window, webView); //add by zlf
}

static gboolean resetEntryProgress(BrowserWindow *window)
{
    gtk_entry_set_progress_fraction(GTK_ENTRY(window->uriEntry), 0);
    window->resetEntryProgressTimeoutId = 0;
    return FALSE;
}

static void webViewLoadProgressChanged(WebKitWebView *webView, GParamSpec *pspec, BrowserWindow *window)
{

    gdouble progress = webkit_web_view_get_estimated_load_progress(webView);
    gtk_entry_set_progress_fraction(GTK_ENTRY(window->uriEntry), progress);
    if (progress == 1.0) {
        window->resetEntryProgressTimeoutId = g_timeout_add(500, (GSourceFunc)resetEntryProgress, window);
        g_source_set_name_by_id(window->resetEntryProgressTimeoutId, "[WebKit] resetEntryProgress");
    }
}

static void downloadStarted(WebKitWebContext *webContext, WebKitDownload *download, BrowserWindow *window)
{
    if(window->downloadDialog)
    {
        browserDownloadNew(download, BROWSER_DOWNLOAD_DIALOG(window->downloadDialog));
        return;
    }
    //by sunh 
    window->downloadDialog = browser_download_dialog_new(download);
    gtk_window_set_transient_for(GTK_WINDOW(window->downloadDialog), GTK_WINDOW(window));
    g_object_add_weak_pointer(G_OBJECT(window->downloadDialog), (gpointer *)&window->downloadDialog);
    //by sunh end
}

//by sunh
static void downloadCallback(BrowserWindow *window)
{
    if (window->downloadDialog) {
        gtk_window_present(GTK_WINDOW(window->downloadDialog));
        return;
    }
    
    window->downloadDialog = browser_download_dialog_new(window->download);
    gtk_window_set_transient_for(GTK_WINDOW(window->downloadDialog), GTK_WINDOW(window));
    g_object_add_weak_pointer(G_OBJECT(window->downloadDialog), (gpointer *)&window->downloadDialog);
    gtk_widget_show(window->downloadDialog);
}
//by sunh end

static void browserWindowHistoryItemSelected(BrowserWindow *window, GtkMenuItem *item)
{
    GtkAction *action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(item));
    browserWindowSetStatusText(window, action ? gtk_action_get_name(action) : NULL);
}

static void browserWindowHistoryItemActivated(BrowserWindow *window, GtkAction *action)
{
    WebKitBackForwardListItem *item = g_object_get_data(G_OBJECT(action), "back-forward-list-item");
    if (!item)
        return;

    webkit_web_view_go_to_back_forward_list_item(window->webView, item);
}

static GtkWidget *browserWindowCreateBackForwardMenu(BrowserWindow *window, GList *list)
{
    if (!list)
        return NULL;

    GtkWidget *menu = gtk_menu_new();
    GList *listItem;
    for (listItem = list; listItem; listItem = g_list_next(listItem)) {
        WebKitBackForwardListItem *item = (WebKitBackForwardListItem *)listItem->data;
        const char *uri = webkit_back_forward_list_item_get_uri(item);
        const char *title = webkit_back_forward_list_item_get_title(item);

        GtkAction *action = gtk_action_new(uri, title, NULL, NULL);
        g_object_set_data_full(G_OBJECT(action), "back-forward-list-item", g_object_ref(item), g_object_unref);
        g_signal_connect_swapped(action, "activate", G_CALLBACK(browserWindowHistoryItemActivated), window);

        GtkWidget *menuItem = gtk_action_create_menu_item(action);
        g_signal_connect_swapped(menuItem, "select", G_CALLBACK(browserWindowHistoryItemSelected), window);
        g_object_unref(action);

        gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuItem);
        gtk_widget_show(menuItem);
    }

    g_signal_connect(menu, "hide", G_CALLBACK(resetStatusText), window);

    return menu;
}

static void browserWindowUpdateNavigationActions(BrowserWindow *window, WebKitBackForwardList *backForwadlist)
{
    gtk_widget_set_sensitive(window->backItem, webkit_web_view_can_go_back(window->webView));
    gtk_widget_set_sensitive(window->forwardItem, webkit_web_view_can_go_forward(window->webView));

    GList *list = g_list_reverse(webkit_back_forward_list_get_back_list_with_limit(backForwadlist, 10));
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(window->backItem),
        browserWindowCreateBackForwardMenu(window, list));
    g_list_free(list);

    list = webkit_back_forward_list_get_forward_list_with_limit(backForwadlist, 10);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(window->forwardItem),
        browserWindowCreateBackForwardMenu(window, list));
    g_list_free(list);
}

static void backForwadlistChanged(WebKitBackForwardList *backForwadlist, WebKitBackForwardListItem *itemAdded, GList *itemsRemoved, BrowserWindow *window)
{
    browserWindowUpdateNavigationActions(window, backForwadlist);
}

static void geolocationRequestDialogCallback(GtkDialog *dialog, gint response, WebKitPermissionRequest *request)
{
    switch (response) {
    case GTK_RESPONSE_YES:
        webkit_permission_request_allow(request);
        break;
    default:
        webkit_permission_request_deny(request);
        break;
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_object_unref(request);
}

static void webViewClose(WebKitWebView *webView, BrowserWindow *window)
{
    gtk_widget_destroy(GTK_WIDGET(window));
}

static void webViewRunAsModal(WebKitWebView *webView, BrowserWindow *window)
{
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(window), window->parentWindow);
}

static void webViewReadyToShow(WebKitWebView *webView, BrowserWindow *window)
{
    WebKitWindowProperties *windowProperties = webkit_web_view_get_window_properties(webView);

    GdkRectangle geometry;
    webkit_window_properties_get_geometry(windowProperties, &geometry);
    if (geometry.x >= 0 && geometry.y >= 0)
        gtk_window_move(GTK_WINDOW(window), geometry.x, geometry.y);
    if (geometry.width > 0 && geometry.height > 0)
        gtk_window_resize(GTK_WINDOW(window), geometry.width, geometry.height);

    if (!webkit_window_properties_get_toolbar_visible(windowProperties))
        gtk_widget_hide(window->toolbar);
    else if (!webkit_window_properties_get_locationbar_visible(windowProperties))
        gtk_widget_hide(window->uriEntry);

    if (!webkit_window_properties_get_resizable(windowProperties))
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    gtk_widget_show(GTK_WIDGET(window));
}

static gboolean fullScreenMessageTimeoutCallback(BrowserWindow *window)
{
    gtk_widget_hide(window->fullScreenMessageLabel);
    window->fullScreenMessageLabelId = 0;
    return FALSE;
}

static gboolean webViewEnterFullScreen(WebKitWebView *webView, BrowserWindow *window)
{
    gchar *titleOrURI = g_strdup(webkit_web_view_get_title(window->webView));
    if (!titleOrURI)
        titleOrURI = getExternalURI(webkit_web_view_get_uri(window->webView));
    gchar *message = g_strdup_printf("%s is now full screen. Press ESC or f to exit.", titleOrURI);
    gtk_label_set_text(GTK_LABEL(window->fullScreenMessageLabel), message);
    g_free(titleOrURI);
    g_free(message);

    gtk_widget_show(window->fullScreenMessageLabel);

    window->fullScreenMessageLabelId = g_timeout_add_seconds(2, (GSourceFunc)fullScreenMessageTimeoutCallback, window);
    g_source_set_name_by_id(window->fullScreenMessageLabelId, "[WebKit] fullScreenMessageTimeoutCallback");
    gtk_widget_hide(window->toolbar);
    window->searchBarVisible = gtk_widget_get_visible(GTK_WIDGET(window->searchBar));
    browser_search_bar_close(window->searchBar);

    return FALSE;
}

static gboolean webViewLeaveFullScreen(WebKitWebView *webView, BrowserWindow *window)
{
    if (window->fullScreenMessageLabelId) {
        g_source_remove(window->fullScreenMessageLabelId);
        window->fullScreenMessageLabelId = 0;
    }
    gtk_widget_hide(window->fullScreenMessageLabel);
    gtk_widget_show(window->toolbar);
    if (window->searchBarVisible) {
        // Opening the search bar steals the focus. Usually, we want
        // this but not when coming back from fullscreen.
        GtkWidget *focusWidget = gtk_window_get_focus(GTK_WINDOW(window));
        browser_search_bar_open(window->searchBar);
        gtk_window_set_focus(GTK_WINDOW(window), focusWidget);
    }

    return FALSE;
}

static GtkWidget *webViewCreate(WebKitWebView *webView, WebKitNavigationAction *navigation, BrowserWindow *window)
{
    WebKitWebView *newWebView = WEBKIT_WEB_VIEW(webkit_web_view_new_with_related_view(webView));
    webkit_web_view_set_settings(newWebView, webkit_web_view_get_settings(webView));

    GtkWidget *newWindow = browser_window_new(newWebView, GTK_WINDOW(window));
    g_signal_connect(newWebView, "ready-to-show", G_CALLBACK(webViewReadyToShow), newWindow);
    g_signal_connect(newWebView, "run-as-modal", G_CALLBACK(webViewRunAsModal), newWindow);
    g_signal_connect(newWebView, "close", G_CALLBACK(webViewClose), newWindow);
    return GTK_WIDGET(newWebView);
}

static gboolean webViewLoadFailed(WebKitWebView *webView, WebKitLoadEvent loadEvent, const char *failingURI, GError *error, BrowserWindow *window)
{
    gtk_entry_set_progress_fraction(GTK_ENTRY(window->uriEntry), 0.);
    return FALSE;
}

static gboolean webViewDecidePolicy(WebKitWebView *webView, WebKitPolicyDecision *decision, WebKitPolicyDecisionType decisionType, BrowserWindow *window)
{
    switch (decisionType) {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION: {
        WebKitNavigationAction *navigationAction = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
        if (webkit_navigation_action_get_navigation_type(navigationAction) != WEBKIT_NAVIGATION_TYPE_LINK_CLICKED
            || webkit_navigation_action_get_mouse_button(navigationAction) != GDK_BUTTON_MIDDLE)
            return FALSE;

        // Opening a new window if link clicked with the middle button.
        WebKitWebView *newWebView = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(webkit_web_view_get_context(webView)));
        GtkWidget *newWindow = browser_window_new(newWebView, GTK_WINDOW(window));
        webkit_web_view_load_request(newWebView, webkit_navigation_action_get_request(navigationAction));
        gtk_widget_show(newWindow);

        webkit_policy_decision_ignore(decision);
        return TRUE;
    }
    case WEBKIT_POLICY_DECISION_TYPE_RESPONSE: {
        WebKitResponsePolicyDecision *responseDecision = WEBKIT_RESPONSE_POLICY_DECISION(decision);
        if (webkit_response_policy_decision_is_mime_type_supported(responseDecision))
            return FALSE;

        WebKitWebResource *mainResource = webkit_web_view_get_main_resource(webView);
        WebKitURIRequest *request = webkit_response_policy_decision_get_request(responseDecision);
        const char *requestURI = webkit_uri_request_get_uri(request);
        if (g_strcmp0(webkit_web_resource_get_uri(mainResource), requestURI))
            return FALSE;

        webkit_policy_decision_download(decision);
        return TRUE;
    }
    case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
    default:
        return FALSE;
    }
}

static gboolean webViewDecidePermissionRequest(WebKitWebView *webView, WebKitPermissionRequest *request, BrowserWindow *window)
{
    if (!WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(request))
        return FALSE;

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               "Geolocation request");

    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "Allow geolocation request?");
    g_signal_connect(dialog, "response", G_CALLBACK(geolocationRequestDialogCallback), g_object_ref(request));
    gtk_widget_show(dialog);

    return TRUE;
}

static void webViewMouseTargetChanged(WebKitWebView *webView, WebKitHitTestResult *hitTestResult, guint mouseModifiers, BrowserWindow *window)
{
    if (!webkit_hit_test_result_context_is_link(hitTestResult)) {
        browserWindowSetStatusText(window, NULL);
        return;
    }
    browserWindowSetStatusText(window, webkit_hit_test_result_get_link_uri(hitTestResult));
}
/*
static gboolean browserWindowCanZoomIn(BrowserWindow *window)
{
    gdouble zoomLevel = webkit_web_view_get_zoom_level(window->webView) * zoomStep;
    return zoomLevel < maximumZoomLevel;
}

static gboolean browserWindowCanZoomOut(BrowserWindow *window)
{
    gdouble zoomLevel = webkit_web_view_get_zoom_level(window->webView) / zoomStep;
    return zoomLevel > minimumZoomLevel;
}

static void browserWindowUpdateZoomActions(BrowserWindow *window)
{
    gtk_widget_set_sensitive(window->zoomInItem, browserWindowCanZoomIn(window));
    gtk_widget_set_sensitive(window->zoomOutItem, browserWindowCanZoomOut(window));
}

static void webViewZoomLevelChanged(GObject *object, GParamSpec *paramSpec, BrowserWindow *window)
{
    browserWindowUpdateZoomActions(window);
}
*/
static void updateUriEntryIcon(BrowserWindow *window)
{
    GtkEntry *entry = GTK_ENTRY(window->uriEntry);
    if (window->favicon)
        gtk_entry_set_icon_from_pixbuf(entry, GTK_ENTRY_ICON_PRIMARY, window->favicon);
    else
        gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_NEW);
}

static void faviconChanged(GObject *object, GParamSpec *paramSpec, BrowserWindow *window)
{
    GdkPixbuf *favicon = NULL;
    cairo_surface_t *surface = webkit_web_view_get_favicon(window->webView);

    if (surface) {
        int width = cairo_image_surface_get_width(surface);
        int height = cairo_image_surface_get_height(surface);
        favicon = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
    }

    if (window->favicon)
        g_object_unref(window->favicon);
    window->favicon = favicon;

    updateUriEntryIcon(window);
    HS_favicon_changed(surface, window->webView); //add by zlf
}

static void webViewIsLoadingChanged(GObject *object, GParamSpec *paramSpec, BrowserWindow *window)
{
    gboolean isLoading = webkit_web_view_is_loading(window->webView);
    gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(window->reloadOrStopButton), isLoading ? GTK_STOCK_STOP : GTK_STOCK_REFRESH);
//    gtk_entry_set_icon_from_stock(GTK_ENTRY(window->uriEntry), GTK_ENTRY_ICON_SECONDARY, isLoading ? GTK_STOCK_STOP : GTK_STOCK_REFRESH);
}

static gboolean inspectorWasOpenedInAnotherWindow(WebKitWebInspector *inspectorWindow, BrowserWindow *window)
{
    window->inspectorWindowIsVisible = TRUE;
    return FALSE;
}

static gboolean inspectorWasClosed(WebKitWebInspector *inspectorWindow, BrowserWindow *window)
{
    window->inspectorWindowIsVisible = FALSE;
    return FALSE;
}

static void zoomInCallback(BrowserWindow *window)
{
    gdouble zoomLevel = webkit_web_view_get_zoom_level(window->webView) * zoomStep;
    webkit_web_view_set_zoom_level(window->webView, zoomLevel);
}

static void zoomOutCallback(BrowserWindow *window)
{
    gdouble zoomLevel = webkit_web_view_get_zoom_level(window->webView) / zoomStep;
    webkit_web_view_set_zoom_level(window->webView, zoomLevel);
}

static void zoomFitCallback(BrowserWindow *window)
{
    gdouble zoomLevel = 1.0;
    webkit_web_view_set_zoom_level(window->webView, zoomLevel);
}

static void showFullscreen(BrowserWindow *window)
{
    GdkWindow* win = gtk_widget_get_window (GTK_WIDGET (window));
    GdkWindowState state = win ? gdk_window_get_state (win) : 0;
    if ( state & GDK_WINDOW_STATE_FULLSCREEN )
            {
        gtk_window_unfullscreen(GTK_WINDOW(window));
        gtk_widget_show(GTK_WIDGET(window->toolbar));
        gtk_widget_show(GTK_WIDGET(window->menubar));
            }
    else
            {
        gtk_window_fullscreen(GTK_WINDOW(window));
        gtk_widget_hide(GTK_WIDGET(window->toolbar));
        gtk_widget_hide(GTK_WIDGET(window->menubar));
            }
}

static void showBookmarkbar(BrowserWindow *window)
{
    gtk_widget_set_visible(window->bookmarkbar, !gtk_widget_get_visible(window->bookmarkbar));
}

//add by zlf start. 2014.11.12
static void showHistoryManagerWindow(BrowserWindow *window)
{
//    g_print("file:%s\n  func:%s\n  line[%d]\n",__FILE__,__func__,__LINE__);
    return HS_showHistoryManagerWindow();
}
static void showHistoryClearWindow(BrowserWindow *window)
{
//    g_print("file:%s\n  func:%s\n  line[%d]\n",__FILE__,__func__,__LINE__);
    return HS_showHistoryClearWindow();
}
//add by zlf end. 2014.11.12
static gboolean toggleWebInspector(BrowserWindow *window, gpointer user_data)
{
    WebKitWebInspector *inspectorWindow;

    inspectorWindow = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(window->webView));
    if (!window->inspectorWindowIsVisible) {
        webkit_web_inspector_show(inspectorWindow);
        window->inspectorWindowIsVisible = TRUE;
    } else
        webkit_web_inspector_close(inspectorWindow);

    return TRUE;
}

static void reloadPage(BrowserWindow *window, gpointer user_data)
{
    webkit_web_view_reload(window->webView);
}

static void reloadPageIgnoringCache(BrowserWindow *window, gpointer user_data)
{
    webkit_web_view_reload_bypass_cache(window->webView);
}

static void homeButtonCallback(BrowserWindow *window)
{
    
}

static void browserWindowFinalize(GObject *gObject)
{
    BrowserWindow *window = BROWSER_WINDOW(gObject);
    if (window->favicon) {
        g_object_unref(window->favicon);
        window->favicon = NULL;
    }

    if (window->accelGroup) {
        g_object_unref(window->accelGroup);
        window->accelGroup = NULL;
    }

    if (window->fullScreenMessageLabelId)
        g_source_remove(window->fullScreenMessageLabelId);

    if (window->resetEntryProgressTimeoutId)
        g_source_remove(window->resetEntryProgressTimeoutId);

    G_OBJECT_CLASS(browser_window_parent_class)->finalize(gObject);

    if (g_atomic_int_dec_and_test(&windowCount))
        gtk_main_quit();
}

static void browserWindowGetProperty(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    BrowserWindow *window = BROWSER_WINDOW(object);

    switch (propId) {
    case PROP_VIEW:
        g_value_set_object(value, browser_window_get_view(window));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

static void browserWindowSetProperty(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    BrowserWindow* window = BROWSER_WINDOW(object);

    switch (propId) {
    case PROP_VIEW:
        window->webView = g_value_get_object(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
    }
}

//by zgh
static void menuSearchCallback(gpointer *data)
{
    browser_search_bar_open(BROWSER_WINDOW(data)->searchBar);
}

static void collecturiCallback(BrowserWindow *window, GtkEntryIconPosition iconPosition, GdkEvent *event)
{
    if (iconPosition == GTK_ENTRY_ICON_SECONDARY) { 
		 g_print("add bookmarkbutton\n");
		/*
		GtkWidget *bookmarkbtn = gtk_button_new();
		GtkBox *hBox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));`
		
        GtkWidget *image = gtk_image_new_from_pixbuf(window->favicon);
		GtkWidget *lab = gtk_label_new(webkit_web_view_get_title(window->webView));
		gtk_box_pack_start(hBox, image, FALSE, FALSE, 0);
		gtk_box_pack_start(hBox, lab, FALSE, FALSE, 0);
		gtk_container_add(GTK_CONTAINER(bookmarkbtn), hBox);
		gtk_box_pack_start(window->bookmarkbox, bookmarkbtn, FALSE, FALSE, 0);
		
		gtk_widget_show_all(bookmarkbtn);
		*/
		
		 GtkToolItem *item = gtk_tool_button_new(NULL, "");
    GtkWidget *image = gtk_image_new_from_pixbuf(window->favicon);
    cairo_surface_t* fivacon = webkit_web_view_get_favicon(window->webView);
    gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (item), fivacon);

    GtkWidget* label = gtk_label_new (webkit_web_view_get_title(window->webView));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_max_width_chars (GTK_LABEL (label), 25);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_widget_show (label);
    gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (item), label);
    //        g_signal_connect_swapped(item, "clicked", G_CALLBACK(goBackCallback), (gpointer)window);
    gtk_toolbar_insert(GTK_TOOLBAR(window->bookmarkbar), item, -1);
    gtk_widget_show_all(GTK_WIDGET(item));

		/*
        GtkToolItem *item = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
//        g_signal_connect_swapped(item, "clicked", G_CALLBACK(goBackCallback), (gpointer)window);
        gtk_toolbar_insert(GTK_TOOLBAR(window->bookmarkbar), item, -1);
        gtk_widget_show(GTK_WIDGET(item));
        */
        
//        GdkEventButton *eventButton = (GdkEventButton *)event;
//todo 弹出对话框
    }
}

static gboolean buttonmenuCallback(GtkWidget *widget, GdkEventButton *event, BrowserWindow *window)
{
   if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
        {
       gtk_menu_popup(GTK_MENU(getviewmenu_toolbar(window->menubar)), NULL, NULL, NULL, NULL, event->button, event->time);
       return TRUE;
        }
    
    return FALSE;
}

//<wangcui add tab
typedef struct _TabInfo{
    BrowserWindow* window;
    int tabid;
}TabInfo;

static void browserWindowConstructed(GObject *gObject);
static void cbShowTab( GtkWidget *widget,TabInfo* tabinfo){
    //1.hide the current overlay
    gtk_widget_hide(GTK_WIDGET(tabinfo->window->webView));
    gtk_widget_hide(GTK_WIDGET(tabinfo->window->overlay));
    //2.show the tabid overlay
    TabView tv=tabinfo->window->tabmng->getTabView(tabinfo->window->tabmng,tabinfo->tabid);
    gtk_widget_show(GTK_WIDGET(tv.viewOverlay));
    gtk_widget_show(GTK_WIDGET(tv.webView));
    g_print("showtab id=%d overlay=%p view=%p\n",tabinfo->tabid,tv.viewOverlay,tv.webView);
    //3.reset the current webview and overlay
    tabinfo->window->webView=tv.webView;
    tabinfo->window->overlay=tv.viewOverlay;
    //3.re-bind the back-forword item\address\title ,see browserWindowConstructed
    webViewTitleChanged(tabinfo->window->webView,0,tabinfo->window);
    webViewURIChanged(tabinfo->window->webView,0,tabinfo->window);
    WebKitBackForwardList *backForwadlist = webkit_web_view_get_back_forward_list(tabinfo->window->webView);
    backForwadlistChanged(backForwadlist,0,0,tabinfo->window);
//    faviconChanged(0, 0, tabinfo->window);
}


static void cbAddTab( GtkWidget *widget,BrowserWindow* window){
    if(!window->tabmng->canAddTab(window->tabmng)){
        g_print("AddTab can't add more tab\n");
        return;
    }
    gtk_widget_hide(GTK_WIDGET(window->overlay));

    window->webView = webkit_web_view_new();
    browserWindowConstructed((GObject *)window); 
    browser_window_load_uri(BROWSER_WINDOW(window), "http://www.sina.com.cn");
}
//add tab>

static void browser_window_init(BrowserWindow *window)
{
    g_atomic_int_inc(&windowCount);
    
    gchar img_src_dir[BUFSIZE] = {0};
    strcat(img_src_dir, get_cuprum_dir());
    
    gchar img_source_1[BUFSIZE] = {0};
    strcat(img_source_1, img_src_dir);
    strcat(img_source_1, "/resources/img/1.png");
    
    gchar img_source_2[BUFSIZE] = {0};
    strcat(img_source_2, img_src_dir);
    strcat(img_source_2, "/resources/img/2.png");

    gtk_window_set_title(GTK_WINDOW(window), defaultWindowTitle);
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 750);

    window->uriEntry = gtk_entry_new();
    g_signal_connect_swapped(window->uriEntry, "activate", G_CALLBACK(activateUriEntryCallback), (gpointer)window);
    gtk_entry_set_icon_activatable(GTK_ENTRY(window->uriEntry), GTK_ENTRY_ICON_PRIMARY, FALSE);
    g_signal_connect_swapped (G_OBJECT(window->uriEntry), "icon-press", G_CALLBACK(certificateCallback), window);
    updateUriEntryIcon(window);

    /* Keyboard accelerators */
    window->accelGroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), window->accelGroup);

    /* Global accelerators */
    gtk_accel_group_connect(window->accelGroup, GDK_KEY_I, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE,
        g_cclosure_new_swap(G_CALLBACK(toggleWebInspector), window, NULL));
    gtk_accel_group_connect(window->accelGroup, GDK_KEY_F12, 0, GTK_ACCEL_VISIBLE,
        g_cclosure_new_swap(G_CALLBACK(toggleWebInspector), window, NULL));
    gtk_accel_group_connect(window->accelGroup, GDK_KEY_F11, 0, GTK_ACCEL_VISIBLE,
        g_cclosure_new_swap(G_CALLBACK(showFullscreen), window, NULL));

    /* Reload page */ 
    gtk_accel_group_connect(window->accelGroup, GDK_KEY_F5, 0, GTK_ACCEL_VISIBLE,
        g_cclosure_new_swap(G_CALLBACK(reloadPage), window, NULL));
    gtk_accel_group_connect(window->accelGroup, GDK_KEY_R, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE,
        g_cclosure_new_swap(G_CALLBACK(reloadPage), window, NULL));

    /* Reload page ignoring cache */
    gtk_accel_group_connect(window->accelGroup, GDK_KEY_F5, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE,
        g_cclosure_new_swap(G_CALLBACK(reloadPageIgnoringCache), window, NULL));
    gtk_accel_group_connect(window->accelGroup, GDK_KEY_R, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE,
        g_cclosure_new_swap(G_CALLBACK(reloadPageIgnoringCache), window, NULL));
        
    //add by zgh menu
    window->menubar = BROWSER_MENU_BAR(browser_menu_bar_new());
    browser_menu_bar_add_accelerators(BROWSER_MENU_BAR(window->menubar), window->accelGroup);
    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_find", G_CALLBACK(menuSearchCallback), window);
    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_zoom_in", G_CALLBACK(zoomInCallback), window);
    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_zoom_out", G_CALLBACK(zoomOutCallback), window);
    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_zoom_fit", G_CALLBACK(zoomFitCallback), window);
//    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_nopagestyle", G_CALLBACK(menuNopagestyleCallback), window);
//    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_webpagestyle", G_CALLBACK(menuWebpagestyleCallback), window);
    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_fullscreen", G_CALLBACK(showFullscreen), window);
    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_bookmarkbar", G_CALLBACK(showBookmarkbar), window);
    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_history_manager", G_CALLBACK(showHistoryManagerWindow), window);//add by zlf
    g_signal_connect_swapped(G_OBJECT(window->menubar), "menu_history_clear", G_CALLBACK(showHistoryClearWindow), window);//add by zlf
    g_signal_connect(G_OBJECT(window->menubar), "menu_quit", G_CALLBACK(gtk_main_quit), window);
//    gtk_container_set_border_width(GTK_CONTAINER(window->menubar), 0);
    g_signal_connect(G_OBJECT(window->menubar), "button-press-event", G_CALLBACK(buttonmenuCallback), window);

    GtkWidget *toolbar = gtk_toolbar_new();
    window->toolbar = toolbar;
    gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
//    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_set_border_width(GTK_CONTAINER(window->toolbar), 0);
    g_signal_connect(G_OBJECT(toolbar), "button-press-event", G_CALLBACK(buttonmenuCallback), window);

    GtkToolItem *item = gtk_menu_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
    window->backItem = GTK_WIDGET(item);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(item), 0);
    g_signal_connect_swapped(item, "clicked", G_CALLBACK(goBackCallback), (gpointer)window);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    gtk_widget_show(GTK_WIDGET(item));

    item = gtk_menu_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
    window->forwardItem = GTK_WIDGET(item);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(item), 0);
    g_signal_connect_swapped(G_OBJECT(item), "clicked", G_CALLBACK(goForwardCallback), (gpointer)window);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    gtk_widget_show(GTK_WIDGET(item));

    item = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
    window->reloadOrStopButton = GTK_WIDGET(item);
    g_signal_connect_swapped(item, "clicked", G_CALLBACK(reloadOrStopCallback), window);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    gtk_widget_add_accelerator(window->reloadOrStopButton, "clicked", window->accelGroup, GDK_KEY_F5, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_show(window->reloadOrStopButton);
    
    item = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
    window->homeButton = GTK_WIDGET(item);
    g_signal_connect_swapped(item, "clicked", G_CALLBACK(homeButtonCallback), window);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    gtk_widget_show(window->homeButton);
    
    gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(window->uriEntry), GTK_ENTRY_ICON_SECONDARY, create_pixbuf(img_source_1));
    gtk_entry_set_icon_activatable(GTK_ENTRY(window->uriEntry), GTK_ENTRY_ICON_PRIMARY, TRUE);
    gtk_entry_set_icon_tooltip_markup(GTK_ENTRY(window->uriEntry), GTK_ENTRY_ICON_SECONDARY, "将此页加为书签");
    g_signal_connect_swapped (G_OBJECT(window->uriEntry), "icon-press", G_CALLBACK(collecturiCallback), window);
    
        //by sunh  
		item = gtk_tool_button_new_from_stock(GTK_STOCK_GOTO_BOTTOM);
		g_signal_connect_swapped(G_OBJECT(item), "clicked", G_CALLBACK(downloadCallback), window);
		gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
		gtk_widget_show(GTK_WIDGET(item));
		//by sunh end

    item = gtk_tool_item_new();
    gtk_tool_item_set_expand(item, TRUE);
    gtk_container_add(GTK_CONTAINER(item), window->uriEntry);
    gtk_widget_show(window->uriEntry);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    gtk_widget_show(GTK_WIDGET(item));

    //setting
    item = gtk_tool_button_new_from_stock(GTK_STOCK_PREFERENCES);
    g_signal_connect_swapped(G_OBJECT(item), "clicked", G_CALLBACK(settingsCallback), window);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
    gtk_widget_show(GTK_WIDGET(item));
    // -setting


    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    window->mainBox = vbox;
    
    gtk_widget_show_all(GTK_WIDGET(window->menubar));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(window->menubar), FALSE, FALSE, 0);

     //<wangc add tab mamager
    TabMng* tbmng=(TabMng*)malloc(sizeof(TabMng));
    initTabMng(tbmng);
    window->tabmng=tbmng;

    GtkWidget *boxtab = gtk_box_new (FALSE, 0);
    window->boxtab=boxtab;

    GtkWidget * btab = gtk_button_new_with_label ("Add");
    g_signal_connect (G_OBJECT (btab), "clicked",
            G_CALLBACK (cbAddTab), window);
    gtk_box_pack_start (GTK_BOX(boxtab), btab, FALSE, FALSE, 0);
    gtk_widget_show (btab);

    gtk_box_pack_start(GTK_BOX(vbox),boxtab,FALSE, FALSE, 0);
    gtk_widget_show(boxtab);
    //>

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);
    
    //书签栏
    GtkWidget *bookmarkbar = gtk_toolbar_new();
    window->bookmarkbar = bookmarkbar;
    /*
    item =gtk_tool_item_new();
    gtk_tool_item_set_expand(item, TRUE);
    GtkWidget *bookmarkBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(item), bookmarkBox);
    window->bookmarkbox = bookmarkBox;
    gtk_toolbar_insert(GTK_TOOLBAR(bookmarkbar), item, 0);
    gtk_widget_show(bookmarkBox);
    gtk_widget_show(GTK_WIDGET(item));
    */
    gtk_toolbar_set_icon_size (GTK_TOOLBAR (window->bookmarkbar),
                               GTK_ICON_SIZE_MENU);
    gtk_toolbar_set_style (GTK_TOOLBAR (window->bookmarkbar),
                           GTK_TOOLBAR_BOTH);//GTK_TOOLBAR_BOTH_HORIZ
    g_signal_connect(G_OBJECT(bookmarkbar), "button-press-event", G_CALLBACK(buttonmenuCallback), window);
    
    gtk_box_pack_start(GTK_BOX(vbox), bookmarkbar, FALSE, FALSE, 0);
//    gtk_widget_show(bookmarkbar);

/*
    GtkWidget *status_bar = gtk_label_new("");
    gtk_label_set_width_chars(GTK_LABEL(status_bar), 50);
    gtk_misc_set_alignment(GTK_MISC(status_bar), 0, 0);
    gtk_box_pack_end(GTK_BOX(vbox), status_bar, FALSE, FALSE, 0);
//    gtk_widget_show(status_bar);
*/
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);
    HS_init(window);// add by zlf
}


static void browserWindowConstructed(GObject *gObject)
{
    BrowserWindow *window = BROWSER_WINDOW(gObject);

    WebKitSettings *settings = webkit_web_view_get_settings(window->webView);
    /*init minimumZoomLevel, maximumZoomLevel  sunhaiming add.*/
    minimumZoomLevel = zoom_factor[0];
    maximumZoomLevel = zoom_factor[settings->pageZoomNum - 1];

//    browserWindowUpdateZoomActions(window);

    g_signal_connect(window->webView, "notify::uri", G_CALLBACK(webViewURIChanged), window);
    g_signal_connect(window->webView, "notify::estimated-load-progress", G_CALLBACK(webViewLoadProgressChanged), window);
    g_signal_connect(window->webView, "notify::title", G_CALLBACK(webViewTitleChanged), window);
    g_signal_connect(window->webView, "create", G_CALLBACK(webViewCreate), window);
    g_signal_connect(window->webView, "load-failed", G_CALLBACK(webViewLoadFailed), window);
    g_signal_connect(window->webView, "decide-policy", G_CALLBACK(webViewDecidePolicy), window);
    g_signal_connect(window->webView, "permission-request", G_CALLBACK(webViewDecidePermissionRequest), window);
    g_signal_connect(window->webView, "mouse-target-changed", G_CALLBACK(webViewMouseTargetChanged), window);
//    g_signal_connect(window->webView, "notify::zoom-level", G_CALLBACK(webViewZoomLevelChanged), window);
    g_signal_connect(window->webView, "notify::favicon", G_CALLBACK(faviconChanged), window);
    g_signal_connect(window->webView, "enter-fullscreen", G_CALLBACK(webViewEnterFullScreen), window);
    g_signal_connect(window->webView, "leave-fullscreen", G_CALLBACK(webViewLeaveFullScreen), window);
    g_signal_connect(window->webView, "notify::is-loading", G_CALLBACK(webViewIsLoadingChanged), window);

    g_signal_connect(webkit_web_view_get_context(window->webView), "download-started", G_CALLBACK(downloadStarted), window);

    window->searchBar = BROWSER_SEARCH_BAR(browser_search_bar_new(window->webView));
    browser_search_bar_add_accelerators(window->searchBar, window->accelGroup);
    gtk_box_pack_start(GTK_BOX(window->mainBox), GTK_WIDGET(window->searchBar), FALSE, FALSE, 0);

    WebKitBackForwardList *backForwadlist = webkit_web_view_get_back_forward_list(window->webView);
    g_signal_connect(backForwadlist, "changed", G_CALLBACK(backForwadlistChanged), window);

    WebKitWebInspector *inspectorWindow = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(window->webView));
    g_signal_connect(inspectorWindow, "open-window", G_CALLBACK(inspectorWasOpenedInAnotherWindow), window);
    g_signal_connect(inspectorWindow, "closed", G_CALLBACK(inspectorWasClosed), window);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_box_pack_start(GTK_BOX(window->mainBox), overlay, TRUE, TRUE, 0);
    gtk_widget_show(overlay);
    window->statusLabel = gtk_label_new(NULL);
    gtk_widget_set_halign(window->statusLabel, GTK_ALIGN_START);
    gtk_widget_set_valign(window->statusLabel, GTK_ALIGN_END);
    gtk_widget_set_margin_left(window->statusLabel, 1);
    gtk_widget_set_margin_right(window->statusLabel, 1);
    gtk_widget_set_margin_top(window->statusLabel, 1);
    gtk_widget_set_margin_bottom(window->statusLabel, 1);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), window->statusLabel);

    gtk_container_add(GTK_CONTAINER(overlay), GTK_WIDGET(window->webView));

    window->fullScreenMessageLabel = gtk_label_new(NULL);
    gtk_widget_set_halign(window->fullScreenMessageLabel, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(window->fullScreenMessageLabel, GTK_ALIGN_CENTER);
    gtk_widget_set_no_show_all(window->fullScreenMessageLabel, TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), window->fullScreenMessageLabel);

    gtk_widget_show(GTK_WIDGET(window->webView));

    //<wangc attach webview to tab
    window->overlay=overlay;
    GtkWidget *btab = gtk_button_new_with_label ("Tab");
    gtk_box_pack_start (GTK_BOX(window->boxtab), btab, FALSE, FALSE, 0);
    int tabid=window->tabmng->generateTabId(window->tabmng);
    TabInfo* ti=(TabInfo*)malloc(sizeof(TabInfo));
    ti->window=window;
    ti->tabid=tabid;
    g_signal_connect (G_OBJECT (btab), "clicked",
            G_CALLBACK (cbShowTab), ti);
    gtk_widget_show (btab);
    window->tabmng->addTab(window->tabmng,tabid,window->overlay,window->webView);
    //>

}

static void browser_window_class_init(BrowserWindowClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);

    gobjectClass->constructed = browserWindowConstructed;
    gobjectClass->get_property = browserWindowGetProperty;
    gobjectClass->set_property = browserWindowSetProperty;
    gobjectClass->finalize = browserWindowFinalize;

    g_object_class_install_property(gobjectClass,
                                    PROP_VIEW,
                                    g_param_spec_object("view",
                                                        "View",
                                                        "The web view of this window",
                                                        WEBKIT_TYPE_WEB_VIEW,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

// Public API.
GtkWidget *browser_window_new(WebKitWebView *view, GtkWindow *parent)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(view), 0);

    BrowserWindow *window = BROWSER_WINDOW(g_object_new(BROWSER_TYPE_WINDOW,
        "type", GTK_WINDOW_TOPLEVEL, "view", view, NULL));

    if (parent) {
        window->parentWindow = parent;
        g_object_add_weak_pointer(G_OBJECT(parent), (gpointer *)&window->parentWindow);
    }

    return GTK_WIDGET(window);
}

WebKitWebView *browser_window_get_view(BrowserWindow *window)
{
    g_return_val_if_fail(BROWSER_IS_WINDOW(window), 0);

    return window->webView;
}

void browser_window_load_uri(BrowserWindow *window, const char *uri)
{
    g_return_if_fail(BROWSER_IS_WINDOW(window));
    g_return_if_fail(uri);

    if (!g_str_has_prefix(uri, "javascript:")) {
        char *internalURI = getInternalURI(uri);

        if (!g_uri_parse_scheme (uri))
        {
            internalURI = g_strconcat("http://", internalURI, NULL);
        }

        if (!internalURI || !strncasecmp (internalURI, "http:", 5)
         || !strncasecmp (internalURI, "https:", 6)
         || !strncasecmp (internalURI, "file:", 5)
         || !strncasecmp (internalURI, "minibrowser-about:", 18))
        {
            g_printerr("\n\t ok!\n");
        }
        else
               return;


        webkit_web_view_load_uri(window->webView, internalURI);
        g_free(internalURI);
        return;
    }

    webkit_web_view_run_javascript(window->webView, strstr(uri, "javascript:"), NULL, NULL, NULL);
}
