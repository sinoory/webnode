/* 
 Modify by ZRL
 2014.12.02 修改error page
 2014.12.02 接收console-message信号，为默认标签页功能。
 2014.12.04 修改midori_view_list_versions()
 2014.12.05 解决网页中通过link打开新窗口不显示内容问题，见webkit_web_view_web_view_ready_cb
 2014.12.10 修复网页中打开新窗口或新Tab时，不加载网页问题。修改webkit_web_view_create_web_view_cb()，并回退12.05针对webkit_web_view_web_view_ready_cb()的修改
 2014.12.11 修复12306.cn左键新窗口或新Tab打开购票页面时crash的问题。为midori-view增加load_commited，并在midori_view_web_view_navigation_decision_cb()使用
 2014.12.17 屏蔽search action，见ENABLE_SEARCH_ACTION
 2014.12.22 解决下载crash问题，参见midori_view_web_view_navigation_decision_cb()
*/

//add by luyue 2015/2/5  start
#include <gio/gio.h>
#include <glib.h>
#include <libsoup/soup.h>
#include<json-c/json.h>
//end

//add by luyue 2015/3/2  start
#include "parse_domain.h"
#include "tld_tree.c"
//end
#include "midori-view.h"
#include "midori-browser.h"
#include "midori-searchaction.h"
#include "midori-app.h"
#include "midori-platform.h"
#include "cdosbrowser-core.h"
#include "midori-findbar.h"
#include "midori-web-extension-proxy.h"

#include "marshal.h"

#include "midori-history.h"
#include "midori-array.h"

#include <config.h>

#include "midori-locationaction.h"

#ifdef HAVE_GCR
    #define GCR_API_SUBJECT_TO_CHANGE
    #include <gcr/gcr.h>
#endif

#if !defined (HAVE_WEBKIT2) && defined (HAVE_LIBSOUP_2_29_91)
SoupMessage*
midori_map_get_message (SoupMessage* message);
#endif

#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include "katze/katze.h"
#if HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef G_OS_WIN32
    #include <sys/utsname.h>
#endif

//ykhu
#define AUTH_INFO_MAX_LEN (64)
#define CMD_MAX (AUTH_INFO_MAX_LEN * 16)
#define GST_DEV "gstreamer1.0-dev"
#define GST_TOOLS "gstreamer1.0-tools"
#define GST_BASE "gstreamer1.0-plugins-base"
#define GST_GOOD "gstreamer1.0-plugins-good"
#define GST_UGLY "gstreamer1.0-plugins-ugly"
#define GST_BAD "gstreamer1.0-plugins-bad"

static void
midori_view_item_meta_data_changed (KatzeItem*   item,
                                    const gchar* key,
                                    MidoriView*  view);

static void
_midori_view_set_settings (MidoriView*        view,
                           MidoriWebSettings* settings);

static void
midori_view_uri_scheme_res (WebKitURISchemeRequest* request,
                            gpointer                user_data);

static void
midori_view_download_started_cb (WebKitWebContext* context,
                                   WebKitDownload*   download,
                                   MidoriView * view);

static gboolean
midori_view_display_error (MidoriView*     view,
                           const gchar*    uri,
                           const gchar*    error_icon,
                           const gchar*    title,
                           const gchar*    message,
                           const gchar*    description,
                           const gchar*    suggestions,
                           const gchar*    try_again,
                           void*           web_frame);

//add by luyue 2015/6/5 start
static void
midori_view_check_phish_cb (WebKitWebView* web_view,
                            MidoriView*    view);
//add end

//add by luyue 2015/6/11 start
static void
midori_view_check_popupwindow_cb (WebKitWebView* web_view,
                                  MidoriView*    view);
//add end

//add by luyue 2015/8/8 start
static void
midori_view_reset_user_agent(MidoriView*   view);
//add end

static gboolean
midori_view_web_view_close_cb (WebKitWebView* web_view,
                               GtkWidget*     view);
struct  _ScriptDialogAction 
{
    MidoriAutocompleter *autocompleter;
    GtkWidget* entry_uri;
    GtkWidget* entry_title;
    GtkTreeModel* completion_model;
    MidoriView*         view;
    GtkWidget *dialog;
    GtkWidget *treeview_one;
    GtkWidget *scrolled;
    GtkWidget *popup_frame;
};
typedef struct  _ScriptDialogAction ScriptDialogAction;

struct _MidoriView
{
    MidoriTab parent_instance;

    gchar* title;
    GdkPixbuf* icon;
    gchar* icon_uri;
    gboolean minimized;
    WebKitHitTestResult* hit_test;
    gchar* link_uri;
    gboolean button_press_handled;
    gboolean has_selection;
    gchar* selected_text;
    MidoriWebSettings* settings;
    GtkWidget* web_view;
    KatzeArray* news_feeds;

    gboolean open_tabs_in_the_background;
    MidoriNewPage open_new_pages_in;
    gint find_links;
    gint alerts;

    GtkWidget* tab_label;
    GtkWidget *password_info_bar;//for auth request response
    GtkWidget* menu_item;
    PangoEllipsizeMode ellipsize;
    KatzeItem* item;
    gint scrollh, scrollv;
    GtkWidget* scrolled_window;

    #if GTK_CHECK_VERSION (3, 2, 0)
    GtkWidget* overlay;
    GtkWidget* overlay_label;
    GtkWidget* overlay_find;
    #endif
    /* Web Extension */
    EphyWebExtensionProxy *web_extension;

    // ZRL 标记该view是否已是LOAD_COMMITED状态，用它辅助判断是否可以获取证书信息
    gboolean load_commited;

#if TRACK_LOCATION_TAB_ICON //lxx, 20150204
	//lxx, 该view下是否显示过track-location icon，如果是，则发送信号，隐藏掉track-location相关icon
	gboolean show_track_location_icon;   
	gboolean show_block_javascript_popup_window_icon;    
#endif

    gchar** website_record_array;  //网站鉴定信息

    //add by luyue
   SoupRequest *soup_request;
   GInputStream *inputStream;
   char *once_read_buffer;
   char *total_read_buffer;
   char *tmp_uri; //网址鉴别时保存uri
   char *load_uri; //危险网址检测时，保存界面显示的url
   char *anquanjibie;//保存安全级别
   char *back_uri;//保存安全回退的uri
   char *forward_uri;//保存安全前进的uri
   GtkWidget *web_view1;//危险网址检测，后台使用
   bool danager_uri_flag;//标志位，0开始危险网址检测，否则关闭
   bool phish_check_flag;//标志位，0开始钓鱼网址检测
   bool popupwindow_check_flag;//标志位，0开始恶意网址检测
   bool save_menu_flag;//标志位，1不走下载流程，走页面另存为流程
   char *download_filename;
   char *download_uri;
   char *download_exec;

   //add end

   gboolean media_info_bar_lock;  //ykhu
};

struct _MidoriViewClass
{
    MidoriTabClass parent_class;
};

G_DEFINE_TYPE (MidoriView, midori_view, MIDORI_TYPE_TAB);

enum
{
    PROP_0,

    PROP_TITLE,
    PROP_ICON,
    PROP_MINIMIZED,
    PROP_ZOOM_LEVEL,
    PROP_NEWS_FEEDS,
    PROP_SETTINGS
};

enum {
    NEW_TAB,
    NEW_WINDOW,
    NEW_VIEW,
#if TRACK_LOCATION_TAB_ICON//lxx,20150203
    TRACK_LOCATION,
    JAVASCRIPT_POPUP_WINDOW_UI_MESSAGE,
#endif	 
    START_LOAD, //lxx, 20150204
		 START_LOAD_HIDE_BLOCK_JAVASCRIPT_WINDOW_ICON,
    ADD_BOOKMARK,
    ABOUT_CONTENT,
    WEBSITE_DATA,
    WEBSITE_CHECK,//luyue,20150309
    DANGEROUS_URL,//luyue,20150309
    FORWARD_URL,//luyue,2015/3/16
    CDOSEXTENSION_MESSAGE,//luyue,2015/4/7
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
midori_view_finalize (GObject* object);

static void
midori_view_set_property (GObject*      object,
                          guint         prop_id,
                          const GValue* value,
                          GParamSpec*   pspec);

static void
midori_view_get_property (GObject*    object,
                          guint       prop_id,
                          GValue*     value,
                          GParamSpec* pspec);

static gboolean
midori_view_focus_in_event (GtkWidget*     widget,
                            GdkEventFocus* event);

static void
midori_view_settings_notify_cb (MidoriWebSettings* settings,
                                GParamSpec*        pspec,
                                MidoriView*        view);

static GObject*
midori_view_constructor (GType                  type,
                         guint                  n_construct_properties,
                         GObjectConstructParam* construct_properties);

static gboolean
webkit_web_view_console_message_cb (GtkWidget*   web_view,
                                    const gchar* message,
                                    guint        line,
                                    const gchar* source_id,
                                    MidoriView*  view);

//ykhu
static void
webkit_web_view_media_failed_text_cb (GtkWidget*   web_view,
                                    const gchar*   text,
                                    MidoriView*    view);

//add by luyue 2015/3/9
static gboolean
webkit_web_view1_console_message_cb (GtkWidget*   web_view,
                                     const gchar* message,
                                     guint        line,
                                     const gchar* source_id,
                                     MidoriView*  view);

static void
midori_web_view1_website_check_cb(MidoriView*   view,
                                  MidoriView*   old_view);

static void
midori_view_web_view1_load_changed_cb (WebKitWebView*  web_view,
                                       WebKitLoadEvent load_event,
                                       MidoriView*     view);

static void
webkit_web_view1_progress_changed_cb (WebKitWebView* web_view,
                                      GParamSpec*    pspec,
                                      MidoriView*    view);

static void
midori_view_web_view1_dangerous_cb (MidoriView*   view,
                                    MidoriView*   old_view);
//add end 

static void
midori_view_class_init (MidoriViewClass* class)
{
    GObjectClass* gobject_class;
    GtkWidgetClass* gtkwidget_class;
    GParamFlags flags;

    signals[NEW_TAB] = g_signal_new (
        "new-tab",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        midori_cclosure_marshal_VOID__STRING_BOOLEAN,
        G_TYPE_NONE, 2,
        G_TYPE_STRING,
        G_TYPE_BOOLEAN);

    signals[NEW_WINDOW] = g_signal_new (
        "new-window",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    /**
     * MidoriView::new-view:
     * @view: the object on which the signal is emitted
     * @new_view: a newly created view
     * @where: where to open the view
     * @user_initiated: %TRUE if the user actively opened the new view
     *
     * Emitted when a new view is created. The value of
     * @where determines where to open the view according
     * to how it was opened and user preferences.
     *
     * Since: 0.1.2
     *
     * Since 0.3.4 a boolean argument was added.
     */
    signals[NEW_VIEW] = g_signal_new (
        "new-view",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        midori_cclosure_marshal_VOID__OBJECT_ENUM_BOOLEAN,
        G_TYPE_NONE, 3,
        MIDORI_TYPE_VIEW,
        MIDORI_TYPE_NEW_VIEW,
        G_TYPE_BOOLEAN);
#if TRACK_LOCATION_TAB_ICON //lxx, 20150203
    /**
     * MidoriView::track-location:
     * @bTrackLocation: %TRUE if settings allow track-location, %FALSE is settings block track-location
     *
     */
    signals[TRACK_LOCATION] = g_signal_new (
        "track-location",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__BOOLEAN,
//			midori_cclosure_marshal_VOID__OBJECT_ENUM_BOOLEAN,
        G_TYPE_NONE, 1,
        G_TYPE_BOOLEAN);

    // lxx create JAVASCRIPT_POPUP_WINDOW_UI_MESSAGE signal
    signals[JAVASCRIPT_POPUP_WINDOW_UI_MESSAGE] = g_signal_new("javascript-popup-window-ui-message",
            G_TYPE_FROM_CLASS(class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
            g_cclosure_marshal_VOID__STRING,
                         G_TYPE_NONE, 1, G_TYPE_STRING);
#endif
//lxx, 20150204

    /**
     * MidoriView::start-load:
     	    *hide the track-location icon when new navigation started
     *
     */
    signals[START_LOAD] = g_signal_new (
        "start-load",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
    /**
     * MidoriView::start-load-hide-block-javascript-window-icon
     	    *hide the track-location icon when new navigation started
     *
     */
    signals[START_LOAD_HIDE_BLOCK_JAVASCRIPT_WINDOW_ICON] = g_signal_new (
        "start-load-hide-block-javascript-window-icon",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signals[WEBSITE_DATA] = g_signal_new (
        "website-data",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_POINTER);

     //add by luyue 2015/3/9
    signals[WEBSITE_CHECK] = g_signal_new (
        "website-check",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    signals[CDOSEXTENSION_MESSAGE] = g_signal_new (
        "cdosextension-message",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    signals[FORWARD_URL] = g_signal_new (
        "forward-url",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_POINTER);

    signals[DANGEROUS_URL] = g_signal_new (
        "dangerous_url",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
   //add end

    /**
     * MidoriView::add-bookmark:
     * @view: the object on which the signal is emitted
     * @uri: the bookmark URI
     *
     * Emitted when a bookmark is added.
     *
     * Deprecated: 0.2.7
     */
    signals[ADD_BOOKMARK] = g_signal_new (
        "add-bookmark",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    /**
     * MidoriView::about-content:
     * @view: the object on which the signal is emitted
     * @uri: the about URI
     *
     * Emitted when loading the about content
     *
     * Return value: the view content as string
     *
     * Since: 0.5.5
     */
    signals[ABOUT_CONTENT] = g_signal_new (
        "about-content",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        g_signal_accumulator_true_handled,
        NULL,
        midori_cclosure_marshal_BOOLEAN__STRING,
        G_TYPE_BOOLEAN, 1,
        G_TYPE_STRING);

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->constructor = midori_view_constructor;
    gobject_class->finalize = midori_view_finalize;
    gobject_class->set_property = midori_view_set_property;
    gobject_class->get_property = midori_view_get_property;

    gtkwidget_class = GTK_WIDGET_CLASS (class);
    gtkwidget_class->focus_in_event = midori_view_focus_in_event;

    flags = G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS;

    g_object_class_install_property (gobject_class,
                                     PROP_TITLE,
                                     g_param_spec_string (
                                     "title",
                                     "Title",
                                     "The title of the currently loaded page",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_ICON,
                                     g_param_spec_object (
                                     "icon",
                                     "Icon",
                                     "The icon of the view",
                                     GDK_TYPE_PIXBUF,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_ZOOM_LEVEL,
                                     g_param_spec_float (
                                     "zoom-level",
                                     "Zoom Level",
                                     "The current zoom level",
                                     G_MINFLOAT,
                                     G_MAXFLOAT,
                                     1.0f,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriView:news-feeds:
    *
    * The news feeds advertised by the currently loaded page.
    *
    * Since: 0.1.7
    */
    g_object_class_install_property (gobject_class,
                                     PROP_NEWS_FEEDS,
                                     g_param_spec_object (
                                     "news-feeds",
                                     "News Feeds",
                                     "The list of available news feeds",
                                     KATZE_TYPE_ARRAY,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_SETTINGS,
                                     g_param_spec_object (
                                     "settings",
                                     "Settings",
                                     "The associated settings",
                                     MIDORI_TYPE_WEB_SETTINGS,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
midori_view_set_title (MidoriView* view, const gchar* title)
{
    const gchar* uri = midori_tab_get_uri (MIDORI_TAB (view));
    //ZRL 修复当uri为空时crash问题。
    if (uri == NULL)
        return;
    if(strstr (uri, "speeddial-head.html") ||g_str_has_prefix (uri, "about:"))
    {
       gchar*title_temp =  _("Speed Dial");
       katze_assign (view->title, g_strdup (midori_tab_get_display_title (title_temp, uri)));
    }
    else katze_assign (view->title, g_strdup (midori_tab_get_display_title (title, uri)));
    view->ellipsize = midori_tab_get_display_ellipsize (view->title, uri);
    if (view->menu_item)
        gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (
                            view->menu_item))), view->title);
    katze_item_set_name (view->item, view->title);
}

static void
midori_view_apply_icon (MidoriView*  view,
                        GdkPixbuf*   icon,
                        const gchar* icon_name)
{
    katze_item_set_icon (view->item, icon_name);
    /* katze_item_get_image knows about this pixbuf */
    if (icon != NULL)
        g_object_ref (icon);
    g_object_set_data_full (G_OBJECT (view->item), "pixbuf", icon,
                            (GDestroyNotify)g_object_unref);
    katze_object_assign (view->icon, icon);
    g_object_notify (G_OBJECT (view), "icon");

    if (view->menu_item)
    {
        GtkWidget* image = katze_item_get_image (view->item, view->web_view);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (view->menu_item), image);
    }
}

static void
_midori_web_view_load_icon (MidoriView* view)
{
    gint icon_width = 16, icon_height = 16;
    GtkSettings* settings = gtk_widget_get_settings (view->web_view);
    gtk_icon_size_lookup_for_settings (settings, GTK_ICON_SIZE_MENU, &icon_width, &icon_height);
    GdkPixbuf* pixbuf = NULL;
    cairo_surface_t* surface = webkit_web_view_get_favicon (WEBKIT_WEB_VIEW (view->web_view));
    if (surface != NULL
     && (pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
        cairo_image_surface_get_width (surface),
        cairo_image_surface_get_height (surface))))
    {
        GdkPixbuf* pixbuf_scaled = gdk_pixbuf_scale_simple (pixbuf,
            icon_width, icon_height, GDK_INTERP_BILINEAR);
        g_object_unref (pixbuf);
        midori_view_apply_icon (view, pixbuf_scaled, view->icon_uri);
    }
}

static void
midori_view_update_load_status (MidoriView*      view,
                                MidoriLoadStatus load_status)
{
    if (midori_tab_get_load_status (MIDORI_TAB (view)) != load_status)
        midori_tab_set_load_status (MIDORI_TAB (view), load_status);
#ifdef APP_LEVEL_TIME
printf("load end time = %lld\n",g_get_real_time());
#endif
}

#if defined (HAVE_LIBSOUP_2_29_91)
gboolean
midori_view_get_tls_info (MidoriView*           view,
                          void*                 request,
                          GTlsCertificate**     tls_cert,
                          GTlsCertificateFlags* tls_flags,
                          gchar**               hostname)
{
    WebKitWebView* web_view = WEBKIT_WEB_VIEW (view->web_view);
    *hostname = midori_uri_parse_hostname (webkit_web_view_get_uri (web_view), NULL);
    gboolean success = webkit_web_view_get_tls_info (web_view, tls_cert, tls_flags);
    if (*tls_cert != NULL)
        g_object_ref (*tls_cert);
    return success;
}
#endif

#if TRACK_LOCATION_TAB_ICON //lxx, 20150203

//lxx,20150127
static char* 
getHostUri(const char * uri)
{
   if(NULL == uri)
      return NULL;

   char* str = strchr(uri, '/');
   if(str)
   {
      gint num = (int)(str-uri);
      char* subStr = str+2;
      char* str2 = strchr(subStr, '/');
      if(str2)
      {
         gint num2 = (int)(str2-subStr+2);
	 num += num2;
      }
      char *returnStr = (char *)malloc(sizeof(char) * (num + 1) );
      strncpy(returnStr, uri, num);
      returnStr[num] = '\0';
      return returnStr;
   }
   return NULL;
}

static void
permission_response (GtkWidget*              infobar,
                     gint                    response_id,
                     WebKitPermissionRequest *request)

{
   MidoriView* view = g_object_get_data ((GObject *)request, "location-track");
   gboolean bTrackLocation = false;
   switch (response_id) {
      case GTK_RESPONSE_ACCEPT:
         bTrackLocation = true;
         webkit_permission_request_allow (request);
	 break;
      default:
	 bTrackLocation = false;
         webkit_permission_request_deny (request);
	 break;
#if TRACK_LOCATION_TAB_ICON //lxx, 20150202
	g_signal_emit (view, signals[TRACK_LOCATION], 0, bTrackLocation);
#endif
	gtk_widget_destroy (GTK_WIDGET (infobar));
   }
}

//lxx, 20150128
static gboolean
permission_request(WebKitWebView           *web_view,
                   WebKitPermissionRequest *request,
                   MidoriView              *view)
{
   const gchar *uri = webkit_web_view_get_uri(web_view);
   char* hostStr = NULL;
   if(NULL != uri)
      hostStr = getHostUri(uri);
   char* message = g_strdup_printf("%s想要使用您的计算机的所在位置信息",  hostStr);
   midori_view_add_info_bar(view, GTK_MESSAGE_QUESTION, message,
                            G_CALLBACK (permission_response), request,
                            "允许", GTK_RESPONSE_ACCEPT,
                            "拒绝", GTK_RESPONSE_CANCEL, NULL);
   if(NULL != hostStr)
   {
      free(hostStr);
      hostStr = NULL;
   }
   free(message);
   return true;
}

//lxx, 20150127
static gboolean
webkit_web_view_permission_request_cb (WebKitWebView           *web_view,
                                       WebKitPermissionRequest *request,
                                       MidoriView              *view)
{
   gint value = 0;
   g_object_get(view->settings, "track-location", &value, NULL);
   gboolean bTrackLocation = false;

   view->show_track_location_icon = true;//lxx, 20150204

   switch(value) {
      case 0:
         bTrackLocation = false;
	 webkit_permission_request_deny (request);
         #if TRACK_LOCATION_TAB_ICON //lxx, 20150202
	    g_signal_emit (view, signals[TRACK_LOCATION], 0, bTrackLocation);
         #endif
	 break;
      case 1:
	 bTrackLocation = true;
	 webkit_permission_request_allow (request);
         #if TRACK_LOCATION_TAB_ICON //lxx, 20150202
	    g_signal_emit (view, signals[TRACK_LOCATION], 0, bTrackLocation);
         #endif
      break;
	case 2:
	   g_object_set_data ((GObject *)request, "location-track", view);
	   permission_request(web_view, request, view);
	   break;
        default:
	   break;
	}
   return TRUE;
}

static void
webkit_web_view_javascript_popup_window_block_cb(WebKitWebView *web_view,
						 const gchar   *str,
			                         MidoriView    *view)
{
   view->show_block_javascript_popup_window_icon = true;//lxx, 20150204
   g_signal_emit(view, signals[JAVASCRIPT_POPUP_WINDOW_UI_MESSAGE], 0, str);
}
#endif //#if TRACK_LOCATION_TAB_ICON //lxx, 20150203

//ykhu
static gboolean
get_authentication_info(gchar* auth_info)
{
    gboolean ret = FALSE;
    //create a dialog
    GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
    GtkWidget *dialog = gtk_dialog_new_with_buttons (g_strdup_printf(_("Media plugin install")),
                                                     NULL,
                                                     flags,
                                                     _("_Cancel"),
                                                     GTK_RESPONSE_CANCEL,
                                                     _("_OK"),
                                                     GTK_RESPONSE_OK,
                                                     NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK); //Sets dialog default response
    gtk_container_set_border_width(GTK_CONTAINER(dialog),10); //Sets dialog border width
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); //Sets whether the user can resize a window
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE); //Sets whether to keep window above other windows

    //get the content area box
    GtkWidget *box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_set_homogeneous(GTK_BOX(box), TRUE);
    gtk_box_set_spacing(GTK_BOX(box), 10);

    //create a label to show the notice information
    GtkWidget *info_label = gtk_label_new(g_strdup_printf(_("You must have administrator rights to complete this installation!")));
    gtk_label_set_max_width_chars(GTK_LABEL(info_label), 30);
    gtk_label_set_line_wrap(GTK_LABEL(info_label), TRUE);
    gtk_container_add(GTK_CONTAINER(box), info_label);

    //create a label and text box
    GtkWidget *password_label = gtk_label_new(g_strdup_printf(_("_Password:")));
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(password_entry), GTK_INPUT_PURPOSE_PASSWORD);

    //create a grid container
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing((GtkGrid *)grid, 20);
    gtk_grid_set_column_spacing((GtkGrid *)grid, 10);
    gtk_container_add(GTK_CONTAINER(box), grid);

    //assemble widget
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    gtk_grid_attach((GtkGrid *)grid, password_entry, 1, 1, 1, 1);

    //show the dialog
    gtk_widget_show_all(dialog);
    gint dialog_result = gtk_dialog_run(GTK_DIALOG(dialog));

    switch(dialog_result){
        case GTK_RESPONSE_OK:
        {
             const gchar *entry_text = gtk_entry_get_text(GTK_ENTRY(password_entry));
             guint16 entry_len = gtk_entry_get_text_length(GTK_ENTRY(password_entry));
             if (entry_text && entry_len > 0 && entry_len < AUTH_INFO_MAX_LEN) {
                 strncpy(auth_info, entry_text, entry_len);
                 ret = TRUE;
             } else {
                 ret = FALSE;
             }
         }
            break;
        case GTK_RESPONSE_CANCEL:
            ret = FALSE;
            break;
        default:
            ret = FALSE;
            break;
    }

    //destroy the dialog
    gtk_widget_destroy(dialog);

    return ret;
}

//ykhu
static gboolean
exec_install_command(const gchar* auth_info)
{
    gboolean ret = FALSE;
    gchar cmd_install[CMD_MAX] = {0};

    gchar tmp[AUTH_INFO_MAX_LEN * 2]= {0};
    int info_index = 0;
    int info_len = strlen(auth_info);
    int tmp_index = 0;

    while (info_index < info_len) {
        if (auth_info[info_index] == '$' || auth_info[info_index] == '\\') {
            tmp[tmp_index++] = '\\';
        }
        tmp[tmp_index++] = auth_info[info_index++];
    }
    snprintf(cmd_install, CMD_MAX, "echo \"%s\" |sudo -S apt-get -y --force-yes install %s %s %s %s %s %s > /dev/null 2>&1 &", tmp, \
                   GST_DEV, GST_TOOLS, GST_BASE, GST_GOOD, GST_UGLY, GST_BAD);

    pid_t ret_status = system(cmd_install);
    if (ret_status == -1) {
        ret = FALSE;
    } else {
        if (WIFEXITED(ret_status)) {
            if (0 == WEXITSTATUS(ret_status)) {
                ret = TRUE;
            } else {
                ret = FALSE;
            }
        } else {
            ret = FALSE;
        }
    }

    return ret;
}

//ykhu
static void
midori_view_install_media_plugin_cb (GtkWidget*       web_view,
                                     GtkResponseType  response_type,
                                     MidoriView*      view)
{
    switch (response_type) {
        case GTK_RESPONSE_ACCEPT:
        {
            gchar *auth_info = (char *)malloc(AUTH_INFO_MAX_LEN);
            if (auth_info) {
                memset(auth_info, 0 , AUTH_INFO_MAX_LEN);
                if (get_authentication_info(auth_info)) {
                    if (exec_install_command(auth_info)) {
                        gchar *message = g_strdup_printf(_("Plugin installation is successful, in order to make effective plugin, please restart your browser!"));
                        midori_view_add_info_bar(view, GTK_MESSAGE_INFO, message,
                            NULL, NULL,
                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                            NULL);
                    }
                }
                free(auth_info);
                auth_info = NULL;
            }
            break;
        }
        case GTK_RESPONSE_REJECT:
        default:
            break;
    }

    if (view) {
        view->media_info_bar_lock = FALSE;
    }
}

//ykhu
static void
midori_view_media_warning_cb (GtkWidget*      web_view,
                              GtkResponseType response_type,
                              MidoriView*     view)
{
    if (view) {
        view->media_info_bar_lock = FALSE;
    }
}

//ykhu
static void
webkit_web_view_media_failed_text_cb (GtkWidget*   web_view,
                                      const gchar* text,
                                      MidoriView*  view)
{
    if (view && !view->media_info_bar_lock && text) {
        view->media_info_bar_lock = TRUE;
        if (strncasecmp(text, "MissingPlugin", 13) == 0) {
            gchar *message = g_strdup_printf(_("Lack media plugins, if install it?"));
            midori_view_add_info_bar(view, GTK_MESSAGE_WARNING, message,
                G_CALLBACK (midori_view_install_media_plugin_cb), view,
                _("_Deny"), GTK_RESPONSE_REJECT,
                _("_Allow"), GTK_RESPONSE_ACCEPT,
                NULL);
        } else if (strncasecmp(text, "FormatError", 11) == 0) {
            gchar *message = g_strdup_printf(_("Video format or MIME type is not supported."));
            midori_view_add_info_bar(view, GTK_MESSAGE_ERROR, message,
                G_CALLBACK (midori_view_media_warning_cb), view,
                GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                NULL);
        } else if (strncasecmp(text, "NetworkError", 12) == 0) {
            gchar *message = g_strdup_printf(_("Video can't play, a fatally network error!"));
            midori_view_add_info_bar(view, GTK_MESSAGE_ERROR, message,
                G_CALLBACK (midori_view_media_warning_cb), view,
                GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                NULL);
        } else if (strncasecmp(text, "DecodeError", 11) == 0) {
            gchar *message = g_strdup_printf(_("Video can't play, a fatally decoder error!"));
            midori_view_add_info_bar(view, GTK_MESSAGE_ERROR, message,
            G_CALLBACK (midori_view_media_warning_cb), view,
                GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                NULL);
        }
    }
}

//add by luyue 2015/2/9
static void
free_midori_view_get_website_record (MidoriView*        view)
{
   if(view->website_record_array[0] && 
      strlen(view->website_record_array[0]) &&
      strcmp(view->website_record_array[0],"unknown") == 0)
   {
      free(view->website_record_array[0]);
      view->website_record_array[0] = NULL;     
   }
   else
   {
      if(view->website_record_array[1])
      {
         free(view->website_record_array[1]);
         view->website_record_array[1] = NULL;
      }
      if(view->website_record_array[2])
      {
         free(view->website_record_array[2]);
         view->website_record_array[2] = NULL;
      }
      if(view->website_record_array[3])
      {
         free(view->website_record_array[3]);
         view->website_record_array[3] = NULL;
      }
      if(view->website_record_array[4])
      {
         free(view->website_record_array[4]);
         view->website_record_array[4] = NULL;
      }
      if(view->website_record_array[5])
      {
         free(view->website_record_array[5]);
         view->website_record_array[5] = NULL;
      }
   }
   free(view->website_record_array);
   view->website_record_array = NULL;
}

//add by luyue 2015/02/07
static void _get_website_record_info(MidoriView*        view)
{
    char *tmp = NULL;
    char *tmp1 = NULL;

    if( view->website_record_array)
       free_midori_view_get_website_record(view);

    if(strstr(view->total_read_buffer,"没有符合条件的记录"))
    {
       view->website_record_array = (char **)malloc(sizeof(char *));
       view->website_record_array[0] = (char *)malloc(8);
       strcpy(view->website_record_array[0],"unknown");
       free(view->total_read_buffer);
    }
    else
    {  
       view->website_record_array = (char **)malloc(sizeof(char *)*6);
       view->website_record_array[0] = NULL;
       view->website_record_array[1] = NULL;
       view->website_record_array[2] = NULL;
       view->website_record_array[3] = NULL;
       view->website_record_array[4] = NULL;
       view->website_record_array[5] = NULL;
     
       if(strstr(view->total_read_buffer,"主办单位名称"))
       {
          tmp = tmp1 = strstr(view->total_read_buffer,"主办单位名称");
          tmp = tmp +498;
          if(strncmp(tmp,"kind",4)!=0)
          {
             //支持www.12306.cn 
             // add by luyue 2015/3/2
             tmp1 = tmp1 + 378;
             if(strncmp(tmp1,"tr style",4) ==0)
             {
                tmp = NULL;              
                tmp1 = tmp1 + 187;
                tmp = strstr(tmp1,"</s>");
                view->website_record_array[0] = (char *)malloc(strlen(tmp1)-strlen(tmp)+1);
                strncpy(view->website_record_array[0],tmp1,strlen(tmp1)-strlen(tmp));
                view->website_record_array[0][strlen(tmp1)-strlen(tmp)]= '\0';
                tmp1 = tmp+69;
                tmp = strstr(tmp1,"</s>");
                view->website_record_array[1] = (char *)malloc(strlen(tmp1)-strlen(tmp)+1);
                strncpy(view->website_record_array[1],tmp1,strlen(tmp1)-strlen(tmp));
                view->website_record_array[1][strlen(tmp1)-strlen(tmp)]= '\0';
                tmp1 = tmp+69;
                tmp = strstr(tmp1,"</s>");
                view->website_record_array[2] = (char *)malloc(strlen(tmp1)-strlen(tmp)+1);
                strncpy(view->website_record_array[2],tmp1,strlen(tmp1)-strlen(tmp));
                view->website_record_array[2][strlen(tmp1)-strlen(tmp)]= '\0';
                tmp1 = tmp+104;
                tmp = strstr(tmp1,"</s>");
                view->website_record_array[3] = (char *)malloc(strlen(tmp1)-strlen(tmp)+1);
                strncpy(view->website_record_array[3],tmp1,strlen(tmp1)-strlen(tmp));
                view->website_record_array[3][strlen(tmp1)-strlen(tmp)]= '\0';
                tmp1 = tmp+53;
                if(strncmp(tmp1,"<div>",5) ==0)
                {
                   tmp1 = tmp1+5;
                   if(strncmp(tmp1,"www.<em>",8)==0)
                   {
                      //12306.cn
                      tmp1 = tmp1 + 8;
                      tmp = strstr(tmp1,"</em>");
                      view->website_record_array[4] = (char *)malloc(strlen(tmp1)-strlen(tmp)+1+4);
                      strcpy(view->website_record_array[4],"www.");
                      strncat(view->website_record_array[4],tmp1,strlen(tmp1)-strlen(tmp));
                      view->website_record_array[4][strlen(tmp1)-strlen(tmp)+4]= '\0';
                      tmp1 = tmp+82;
                   }
                   else if (strstr(tmp1,"</div>"))
                   {
                      //alipay.com
                      tmp = strstr(tmp1,"</div>");
                      view->website_record_array[4] = (char *)malloc(strlen(tmp1)-strlen(tmp)+1);
                      strncpy(view->website_record_array[4],tmp1,strlen(tmp1)-strlen(tmp));
                      view->website_record_array[4][strlen(tmp1)-strlen(tmp)]= '\0';
                      tmp1 = tmp+77;
                   }
                }
                //add by luyue 2015/5/5 start
                //解决linuxidc.com的问题
                else
                   tmp1 = tmp1+69;
                //add end
                tmp = strstr(tmp1,"</s>");
                view->website_record_array[5] = (char *)malloc(strlen(tmp1)-strlen(tmp)+1);
                strncpy(view->website_record_array[5],tmp1,strlen(tmp1)-strlen(tmp));
                view->website_record_array[5][strlen(tmp1)-strlen(tmp)]= '\0';
                tmp = tmp1 = NULL;
                free(view->total_read_buffer);
                g_signal_emit (GTK_WIDGET(view), signals[WEBSITE_DATA], 0, view->website_record_array);
                return;
             }
             //add end
             tmp = tmp1 = NULL;
          }
          else
          {
             tmp = tmp + 6;
             tmp1 = strstr(tmp,"</div>");
             view->website_record_array[0] = (char *)malloc(strlen(tmp)-strlen(tmp1)+1);
             strncpy(view->website_record_array[0],tmp,strlen(tmp)-strlen(tmp1));
             view->website_record_array[0][strlen(tmp)-strlen(tmp1)]= '\0';
             tmp = tmp1+92;
             tmp1 = strstr(tmp,"</div>");
             view->website_record_array[1] = (char *)malloc(strlen(tmp)-strlen(tmp1)+1);
             strncpy(view->website_record_array[1],tmp,strlen(tmp)-strlen(tmp1));
             view->website_record_array[1][strlen(tmp)-strlen(tmp1)]= '\0';
             tmp = tmp1+157;
             tmp1 = strstr(tmp,"</a>");
             view->website_record_array[2] = (char *)malloc(strlen(tmp)-strlen(tmp1)+1);
             strncpy(view->website_record_array[2],tmp,strlen(tmp)-strlen(tmp1));
             view->website_record_array[2][strlen(tmp)-strlen(tmp1)]= '\0';
             if(strstr(view->total_read_buffer,"<td><b>网站名称"))
             {
                tmp = strstr(view->total_read_buffer,"word-break:break-all;word-wrap:break-word;");
                tmp = tmp +62;
                tmp1 = strstr(tmp,"</td>");
                view->website_record_array[3] = (char *)malloc(strlen(tmp)-strlen(tmp1)-13);
                strncpy(view->website_record_array[3],tmp,strlen(tmp)-strlen(tmp1)-14);
                view->website_record_array[3][strlen(tmp)-strlen(tmp1)-14]= '\0';
             }
             if(strstr(view->total_read_buffer,"网站首页网址"))
             {
                tmp = strstr(view->total_read_buffer,"home_url");
                tmp = tmp +36;
                tmp1 = strstr(tmp,"target");
                view->website_record_array[4] = (char *)malloc(strlen(tmp)-strlen(tmp1)-1);
                strncpy(view->website_record_array[4],tmp,strlen(tmp)-strlen(tmp1)-2);
                view->website_record_array[4][strlen(tmp)-strlen(tmp1)-2]= '\0';
             }
             if(strstr(view->total_read_buffer,"审核时间"))
             {
                tmp = strstr(view->total_read_buffer,"pass_time");
                if(tmp)
                {
                   tmp = tmp +11;
                   if(!strlen(tmp)<16)
                   {
                      tmp1 = tmp+10;
                      if(strncmp(tmp1,"</div>",6)==0)
                      {
                         view->website_record_array[5] = (char *)malloc(11);
                         strncpy(view->website_record_array[5],tmp,10);
                         view->website_record_array[5][10]= '\0';
                      }
                   }
                }
             }
             tmp = NULL;
             tmp1 = NULL;
          }
       }
       free(view->total_read_buffer);
    }
    g_signal_emit (GTK_WIDGET(view), signals[WEBSITE_DATA], 0, view->website_record_array);
}

//add by luyue 2015/02/07
static void 
readCallback(GObject*      object, 
             GAsyncResult* asyncResult, 
             gpointer      data)
{
   MidoriView *view = MIDORI_VIEW (data);
   gssize bytesRead = g_input_stream_read_finish(view->inputStream, asyncResult, NULL);
   if (bytesRead == -1)
   {
      //error happen
      return;
   }
   else if (bytesRead == 0) {
       g_input_stream_close(view->inputStream, 0, 0);
       char *tmp_str = strstr(view->total_read_buffer,"</html>");
       if(tmp_str)
       {
          char *tmp_str1 = (char *) malloc(strlen(view->total_read_buffer)-strlen(tmp_str)+1);
          memset(tmp_str1,0,strlen(view->total_read_buffer)-strlen(tmp_str)+1);
          memcpy(tmp_str1,view->total_read_buffer,strlen(view->total_read_buffer)-strlen(tmp_str));
          free(view->total_read_buffer);
          free(view->once_read_buffer);

          view->total_read_buffer = (char *) malloc(strlen(tmp_str1)+1);
          memset(view->total_read_buffer,0,strlen(tmp_str1)+1);
          strcpy(view->total_read_buffer,tmp_str1);
          free(tmp_str1);
          _get_website_record_info(view);  
          return;
       }
       else return;
   }
   strcat(view->total_read_buffer,view->once_read_buffer);
   memset(view->once_read_buffer,0,1024*10);
   g_input_stream_read_async(view->inputStream, view->once_read_buffer, 1024*10, G_PRIORITY_DEFAULT_IDLE,NULL, readCallback, view);
}

//add by luyue 2015/02/07
static void 
sendRequestCallback(GObject*      object, 
                    GAsyncResult* result, 
                    gpointer      data)
{
    MidoriView *view = MIDORI_VIEW (data);
    view->once_read_buffer = (char *) malloc(1024*10);
    view->total_read_buffer = (char *) malloc(1024*60);
    memset(view->once_read_buffer,0,1024*10);
    memset(view->total_read_buffer,0,1024*60);
    if(g_task_is_valid (result, view->soup_request))
    {
       view->inputStream = soup_request_send_finish(view->soup_request, result, NULL);
       view->soup_request = NULL;
       if(view->inputStream)
          g_input_stream_read_async(view->inputStream, view->once_read_buffer, 1024*10, G_PRIORITY_DEFAULT_IDLE,NULL, readCallback, view);
    }
}

//add by luyue 2015/3/2
static char* 
_get_base_domain (char * base_domain)
{
   char              line[MAX_LENGTH];
   int               len;
   tldnode          *tree;
   string_t         *domain;
   http_result_t     result;
   public_suffix_t  *ps;

   tree = init_tld_tree(tld_string);
    if (tree == NULL)
        return NULL;
    domain = &result.domain;
    strcpy(line,base_domain);
    memset(&result, 0, sizeof(http_result_t));
    len = strlen(line);
    domain->data = line;
    domain->len = len;
    if (parse_domain(tree, &result, domain) != 0)
       return NULL;
    ps = &result.complex_domain;
    return ps->domain.data;
}

//zgh 20150107
static gboolean
midori_view_website_query_idle(gpointer data)
{
    MidoriView *view = MIDORI_VIEW (data);
    GtkWidget* current_web_view = midori_view_get_web_view (view);
    const gchar *uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW(current_web_view));
    WebKitURIRequest *request = webkit_uri_request_new(uri);
    gchar *base_domain = webkit_uri_request_get_uri_host (request);
    gchar *web_tab_uri = NULL;
    if (base_domain == NULL || !strcmp(base_domain,"")) 
    {
        web_tab_uri = g_strdup_printf("http://www.beianbeian.com/search/%s", "null");
    }
    else {
        //add by luyue 2015/3/2 start
        //获取base_domain
        base_domain = _get_base_domain(base_domain);
        //add end
        if (base_domain == NULL || !strcmp(base_domain,"")) 
        {
           web_tab_uri = g_strdup_printf("http://www.beianbeian.com/search/%s", "null");
        }
        else
           web_tab_uri = g_strdup_printf("http://www.beianbeian.com/search/%s", base_domain);
    }

    //add by luyue
    SoupURI *soup_uri = soup_uri_new(web_tab_uri);
    SoupSession *soup_session=soup_session_new();
    char *string =NULL;
    g_object_get(view->settings, "user-agent", &string, NULL);
    g_object_set(soup_session, "user-agent", string, NULL);
   string = NULL;
   if(!view->soup_request) 
    view->soup_request= soup_session_request_uri(soup_session, soup_uri, NULL);
    if(view->soup_request)
       soup_request_send_async(view->soup_request, NULL, sendRequestCallback, view);   

    return false;
}

static gboolean
midori_view_web_view_navigation_decision_cb (WebKitWebView*             web_view,
                                             #ifdef HAVE_WEBKIT2
                                             WebKitPolicyDecision*      decision,
                                             WebKitPolicyDecisionType   decision_type,
                                             #else
                                             WebKitWebFrame*            web_frame,
                                             WebKitNetworkRequest*      request,
                                             WebKitWebNavigationAction* action,
                                             WebKitWebPolicyDecision*   decision,
                                             #endif
                                             MidoriView*                view)
{
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback start time = %lld\n",g_get_real_time());
#endif
    if (decision_type == WEBKIT_POLICY_DECISION_TYPE_RESPONSE)
    {
        //add by luyue 2015/3/10
        if(!view->danager_uri_flag)
        {
           char *tmp1 = NULL;
           if(view->forward_uri)
           {
              tmp1 = (char *)malloc(strlen(view->forward_uri)+1);
              strcpy(tmp1,view->forward_uri);         
              free(view->forward_uri);
              view->forward_uri = NULL;
           }
           const char *tmp_uri = webkit_web_view_get_uri(web_view);
           view->forward_uri = (char *) malloc(strlen(tmp_uri)+1);
           strcpy(view->forward_uri,tmp_uri);
           if(view->anquanjibie)
           {
              if(view->forward_uri && tmp1 && strcmp(tmp1,view->forward_uri))
                 g_signal_emit (view, signals[FORWARD_URL], 0,view->forward_uri);
              free(view->anquanjibie);
              view->anquanjibie = NULL;
           }
           if(tmp1)
           {
              free(tmp1);
              tmp1 = NULL;
           }
        } 
        //add end
        WebKitURIResponse* response = webkit_response_policy_decision_get_response (
            WEBKIT_RESPONSE_POLICY_DECISION (decision));
        const gchar* mime_type = webkit_uri_response_get_mime_type (response);
        midori_tab_set_mime_type (MIDORI_TAB (view), mime_type);
        katze_item_set_meta_string (view->item, "mime-type", mime_type);
        if (!webkit_web_view_can_show_mime_type (web_view, mime_type))
        {
            webkit_policy_decision_download (decision);
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
            return TRUE;
        }
        
#if ENABLE_WEBSITE_AUTH
        const gchar* w_uri = webkit_web_view_get_uri(web_view);
        const gchar* d_uri = webkit_uri_response_get_uri(response);

        if(!memcmp(w_uri, d_uri, strlen(w_uri) + 1))
        {
            //add by luyue 2015/2/9 start
            //刷新页面时，同一url，只做一次网址鉴别查询
            if(view->tmp_uri == NULL || strcmp(view->tmp_uri,w_uri) != 0)
            {
               if( view->website_record_array)
                  katze_assign (view->website_record_array, NULL);
               g_idle_add (midori_view_website_query_idle, view);
               if(view->tmp_uri)
               {
                  free(view->tmp_uri);
                  view->tmp_uri = NULL;
               }
               view->tmp_uri = (char *)malloc(strlen(w_uri)+1);
               strcpy(view->tmp_uri,w_uri);
            }
            //add end
        }
#endif
        if(webkit_web_view_isattachment(web_view, decision, WEBKIT_POLICY_DECISION_TYPE_RESPONSE))
        {
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
            return TRUE;
        }
        webkit_policy_decision_use (decision);
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
        return TRUE;
    }
    else if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
    {
    }
    else if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    {
    }
    else
    {
        g_debug ("Unhandled policy decision type %d", decision_type);
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
        return FALSE;
    }
    //add by luyue 2015/8/8 start
    midori_view_set_user_agent(view,webkit_web_view_get_uri(web_view));
    //add end

    void* request = NULL;
    const gchar* uri = webkit_uri_request_get_uri (
        webkit_navigation_policy_decision_get_request (WEBKIT_NAVIGATION_POLICY_DECISION (decision)));
    if (g_str_has_prefix (uri, "geo:") && strstr (uri, ","))
    {
        gchar* new_uri = sokoke_magic_uri (uri, TRUE, FALSE);
        midori_view_set_uri (view, new_uri);
        g_free (new_uri);
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
        return TRUE;
    }
    else if (g_str_has_prefix (uri, "data:image/"))
    {
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
        return TRUE;
    }
    #if defined (HAVE_GCR)
    else if (/* midori_tab_get_special (MIDORI_TAB (view)) && */ !strncmp (uri, "https", 5))
    {
        /* We show an error page if the certificate is invalid.
           If a "special", unverified page loads a form, it must be that page.
           if (webkit_web_navigation_action_get_reason (action) == WEBKIT_WEB_NAVIGATION_REASON_FORM_SUBMITTED)
           FIXME: Verify more stricly that this cannot be eg. a simple Reload */
        // ZRL 只有view达到LOAD_COMMITED状态时，取证书信息才有意义
        if (decision_type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION && view->load_commited == TRUE)
        {
            GTlsCertificate* tls_cert;
            GTlsCertificateFlags tls_flags;
            gchar* hostname;
            if (!midori_view_get_tls_info (view, request, &tls_cert, &tls_flags, &hostname)
             && tls_cert != NULL)
            {
                GcrCertificate* gcr_cert;
                GByteArray* der_cert;

                g_object_get (tls_cert, "certificate", &der_cert, NULL);
                gcr_cert = gcr_simple_certificate_new (der_cert->data, der_cert->len);
                g_byte_array_unref (der_cert);
                if (hostname && !gcr_trust_is_certificate_pinned (gcr_cert, GCR_PURPOSE_SERVER_AUTH, hostname, NULL, NULL))
                {
                    GError* error = NULL;
                    gcr_trust_add_pinned_certificate (gcr_cert, GCR_PURPOSE_SERVER_AUTH, hostname, NULL, &error);
                    if (error != NULL)
                    {
                        gchar* slots = g_strjoinv (" , ", (gchar**)gcr_pkcs11_get_trust_lookup_uris ());
                        gchar* title = g_strdup_printf ("Error granting trust: %s", error->message);
                        midori_tab_stop_loading (MIDORI_TAB (view));
                        midori_view_display_error (view, NULL, NULL, NULL, title, slots, NULL,
                            _("Trust this website"), NULL);
                        g_free (title);
                        g_free (slots);
                        g_error_free (error);
                    }
                }
                g_object_unref (gcr_cert);
            }
            if (tls_cert != NULL)
                g_object_unref (tls_cert);
            g_free (hostname);
        }
    }
    #endif

    if (katze_item_get_meta_integer (view->item, "delay") == MIDORI_DELAY_PENDING_UNDELAY)
    {
        midori_tab_set_special (MIDORI_TAB (view), FALSE);
        katze_item_set_meta_integer (view->item, "delay", MIDORI_DELAY_UNDELAYED);
    }

    gboolean bvalue = 0;
    g_object_get(view->settings, "night-mode", &bvalue, NULL);
    if(bvalue)
    {
       char *night_level = NULL;
       gchar *backgroundSrc = NULL;
       GError * _inner_error_ = NULL;
       gchar *queryStr = NULL;
       g_file_get_contents (midori_paths_get_res_filename("night_mode/nightingale_view_content.js"),
                            &backgroundSrc,
                            NULL,
                            &_inner_error_);
       g_object_get(view->settings, "night-level", &night_level,NULL);
       if(!night_level)
          queryStr = g_strdup_printf(backgroundSrc,"0.45");
       else 
          queryStr = g_strdup_printf(backgroundSrc,night_level);
       webkit_web_view_run_javascript(web_view, queryStr, NULL, NULL, NULL);
       g_free(queryStr);
       g_free(backgroundSrc);
       if(night_level)
          free(night_level); 
    }
    //add by luyue 2014/12/10
    gboolean handled = FALSE;
    g_signal_emit_by_name (view, "navigation-adblock", uri, &handled);
    if (handled)
    {
        webkit_policy_decision_ignore (decision);
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
        return TRUE;
    }

    g_signal_emit_by_name (view, "navigation-requested", uri, &handled);
    if (handled)
    {
        webkit_policy_decision_ignore (decision);
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
        return TRUE;
    }
    //add by luyue 2015/5/4 start
    //解决空白页点击阅读模式,无法返回原始页面的问题
    if(strcmp(uri,"about:dial#")==0)
       midori_view_reload(view,FALSE);
    //add end
#ifdef APP_LEVEL_TIME
printf("signal(navigation-decision) callback end time = %lld\n",g_get_real_time());
#endif
    return FALSE;
}

static void
midori_view_load_started (MidoriView* view)
{
#ifdef APP_LEVEL_TIME
printf("load start callback start time = %lld\n",g_get_real_time());
#endif
    midori_view_update_load_status (view, MIDORI_LOAD_PROVISIONAL);
    if(strncmp(midori_tab_get_uri (MIDORI_TAB (view)),"file",4))
       midori_tab_set_progress (MIDORI_TAB (view), 0.0);
    midori_tab_set_load_error (MIDORI_TAB (view), MIDORI_LOAD_ERROR_NONE);

#if TRACK_LOCATION_TAB_ICON //lxx, 20150204
	if(true == view->show_track_location_icon)
		g_signal_emit (view, signals[START_LOAD], 0);

	if(true == view->show_block_javascript_popup_window_icon)
		g_signal_emit (view, signals[START_LOAD_HIDE_BLOCK_JAVASCRIPT_WINDOW_ICON], 0);
#endif
#ifdef APP_LEVEL_TIME
printf("load start callback end time = %lld\n",g_get_real_time());
#endif
}

#ifdef HAVE_GCR
const gchar*
midori_location_action_tls_flags_to_string (GTlsCertificateFlags flags);
#endif

static void
midori_view_load_committed (MidoriView* view)
{
#ifdef APP_LEVEL_TIME
printf("commit load callback start time = %lld\n",g_get_real_time());
#endif
    katze_assign (view->icon_uri, NULL);

    GList* children = gtk_container_get_children (GTK_CONTAINER (view));
    for (; children; children = g_list_next (children))
        if (g_object_get_data (G_OBJECT (children->data), "midori-infobar-cb"))
            gtk_widget_destroy (children->data);
    g_list_free (children);
    view->alerts = 0;

    const gchar* uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW  (view->web_view));
    if (g_strcmp0 (uri, katze_item_get_uri (view->item)))
    {
        midori_tab_set_uri (MIDORI_TAB (view), uri);
        katze_item_set_uri (view->item, uri);
        midori_tab_set_special (MIDORI_TAB (view), FALSE);
    }

    katze_item_set_added (view->item, time (NULL));
    g_object_set (view, "title", NULL, NULL);

    if (!strncmp (uri, "https", 5))
    {
        #if defined (HAVE_LIBSOUP_2_29_91)
        void* request = NULL;
        GTlsCertificate* tls_cert;
        GTlsCertificateFlags tls_flags;
        gchar* hostname; /* FIXME leak */
        if (midori_view_get_tls_info (view, request, &tls_cert, &tls_flags, &hostname))
            midori_tab_set_security (MIDORI_TAB (view), MIDORI_SECURITY_TRUSTED);
        #ifdef HAVE_GCR
        else if (!midori_tab_get_special (MIDORI_TAB (view)) && tls_cert != NULL)
        {
            GcrCertificate* gcr_cert;
            GByteArray* der_cert;

            g_object_get (tls_cert, "certificate", &der_cert, NULL);
            gcr_cert = gcr_simple_certificate_new (der_cert->data, der_cert->len);
            g_byte_array_unref (der_cert);
            if (gcr_trust_is_certificate_pinned (gcr_cert, GCR_PURPOSE_SERVER_AUTH, hostname, NULL, NULL))
                midori_tab_set_security (MIDORI_TAB (view), MIDORI_SECURITY_TRUSTED);
            else
            {
                midori_tab_set_security (MIDORI_TAB (view), MIDORI_SECURITY_UNKNOWN);
                midori_tab_stop_loading (MIDORI_TAB (view));
                midori_view_display_error (view, NULL, NULL, NULL, _("Security unknown"),
                    midori_location_action_tls_flags_to_string (tls_flags), NULL,
                    _("Trust this website"),
                    NULL);
            }
            g_object_unref (gcr_cert);
        }
        #endif
        else
        #endif
            midori_tab_set_security (MIDORI_TAB (view), MIDORI_SECURITY_UNKNOWN);
        #if defined (HAVE_LIBSOUP_2_29_91)
        if (tls_cert != NULL)
            g_object_unref (tls_cert);
        g_free (hostname);
        #endif
    }
    else
        midori_tab_set_security (MIDORI_TAB (view), MIDORI_SECURITY_NONE);

    view->find_links = -1;
    view->load_commited = TRUE; // ZRL initialize var

    view->media_info_bar_lock = FALSE;  //ykhu

    midori_view_update_load_status (view, MIDORI_LOAD_COMMITTED);
#ifdef APP_LEVEL_TIME
printf("commit load callback end time = %lld\n",g_get_real_time());
#endif
}

static void
webkit_web_view_progress_changed_cb (WebKitWebView* web_view,
                                     GParamSpec*    pspec,
                                     MidoriView*    view)
{
#ifdef APP_LEVEL_TIME
printf("signal(progress-change) callback start time = %lld\n",g_get_real_time());
#endif
    gdouble progress = 1.0;
    //add by luyue 2015/3/9
    //增加一个url时，不需要设置progress,即不需滚动spinner
    GtkWidget* current_web_view = midori_view_get_web_view (view);
    const gchar *uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW(current_web_view));
    if(uri && strncmp(uri,"file",4)!=0)
    {
       //add end
       g_object_get (web_view, pspec->name, &progress, NULL);
       midori_tab_set_progress (MIDORI_TAB (view), progress);
    }
#ifdef APP_LEVEL_TIME
printf("signal(progress-change) callback end time = %lld\n",g_get_real_time());
#endif
}

static void 
_midori_view_uri_loadres_scheme_data(WebKitURISchemeRequest *request, 
                                     const gchar            *uri) 
{
    gchar* filepath = midori_paths_get_res_filename (&uri[6]);
    gchar* contents;
    gsize length;
    if (g_file_get_contents (filepath, &contents, &length, NULL))
    {
        gchar* content_type = g_content_type_guess (filepath, (guchar*)contents, length, NULL);
        gchar* mime_type = g_content_type_get_mime_type (content_type);
        GInputStream* stream = g_memory_input_stream_new_from_data (contents, length, g_free);
        webkit_uri_scheme_request_finish (request, stream, length, mime_type);
        g_object_unref (stream);
        g_free (mime_type);
        g_free (content_type);
    }
    g_free (filepath);
}

static void 
_midori_view_uri_loadstock_scheme_data(MidoriView*             view, 
                                       WebKitURISchemeRequest* request, 
                                       const gchar *uri) 
{
    GdkPixbuf* pixbuf;
    const gchar* icon_name = &uri[8] ? &uri[8] : "";
    gint icon_size = GTK_ICON_SIZE_MENU;
    static gint icon_size_large_dialog = 0;

    if (!icon_size_large_dialog)
    {
        gint width = 48, height = 48;
        gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &width, &height);
        icon_size_large_dialog = gtk_icon_size_register ("large-dialog", width * 2, height * 2);
    }
    if (g_ascii_isalpha (icon_name[0]))
    {
        if (g_str_has_prefix (icon_name, "dialog/"))
        {
            icon_name = &icon_name [strlen("dialog/")];
            icon_size = icon_size_large_dialog;
        }
        else
            icon_size = GTK_ICON_SIZE_BUTTON;
    }
    else if (g_ascii_isdigit (icon_name[0]))
    {
        guint i = 0;
        while (icon_name[i])
            if (icon_name[i++] == '/')
            {
                gchar* size = g_strndup (icon_name, i - 1);
                icon_size = atoi (size);
                /* Compatibility: map pixel to symbolic size */
                if (icon_size == 16)
                    icon_size = GTK_ICON_SIZE_MENU;
                g_free (size);
                icon_name = &icon_name[i];
            }
    }

    /* Render icon as a PNG at the desired size */
    pixbuf = gtk_widget_render_icon (GTK_WIDGET (view), icon_name, icon_size, NULL);
    if (!pixbuf)
        pixbuf = gtk_widget_render_icon (GTK_WIDGET (view), GTK_STOCK_MISSING_IMAGE, icon_size, NULL);
    if (pixbuf)
    {
        gboolean success;
        gchar* buffer;
        gsize buffer_size;
        gchar* encoded;
        gchar* data_uri;

        success = gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &buffer_size, "png", NULL, NULL);
        g_object_unref (pixbuf);
        if (!success)
            return;

        encoded = g_base64_encode ((guchar*)buffer, buffer_size);
        data_uri = g_strconcat ("data:image/png;base64,", encoded, NULL);
        g_free (encoded);

        GInputStream* stream = g_memory_input_stream_new_from_data (buffer, buffer_size, g_free);
        webkit_uri_scheme_request_finish (request, stream, buffer_size, "image/png");
        g_object_unref (stream);
        g_free (data_uri);
        return;
    }
}

static void
midori_view_uri_scheme_res (WebKitURISchemeRequest* request,
                            gpointer                user_data)
{
    const gchar* uri = webkit_uri_scheme_request_get_uri (request);
    WebKitWebView* web_view = webkit_uri_scheme_request_get_web_view (request);
    MidoriView* view = midori_view_get_for_widget (GTK_WIDGET (web_view));

    /* Only apply custom URIs to special pages for security purposes */
    if (!midori_tab_get_special (MIDORI_TAB (view))) {
        // ZRL 修复有时候进入about:页面，但却被判断view不属于特殊页面，导致res和stock等特殊资源无法加载的问题。
        if (g_str_has_prefix (uri, "res://"))
        {
            _midori_view_uri_loadres_scheme_data(request, uri);
        }
        else if (g_str_has_prefix (uri, "stock://")) {
            _midori_view_uri_loadstock_scheme_data(view, request, uri);
        }
        return;
    }

    if (g_str_has_prefix (uri, "res://"))
    {
        gchar* filepath = midori_paths_get_res_filename (&uri[6]);
        gchar* contents;
        gsize length;
        printf("ZRL midori-view.c midori_view_uri_scheme_res() filepath = %s \n", filepath);
        if (g_file_get_contents (filepath, &contents, &length, NULL))
        {
            gchar* content_type = g_content_type_guess (filepath, (guchar*)contents, length, NULL);
            gchar* mime_type = g_content_type_get_mime_type (content_type);
            GInputStream* stream = g_memory_input_stream_new_from_data (contents, length, g_free);
            webkit_uri_scheme_request_finish (request, stream, length, mime_type);
            g_object_unref (stream);
            g_free (mime_type);
            g_free (content_type);
        }
        g_free (filepath);
    }
    else if (g_str_has_prefix (uri, "stock://"))
    {
        GdkPixbuf* pixbuf;
        const gchar* icon_name = &uri[8] ? &uri[8] : "";
        gint icon_size = GTK_ICON_SIZE_MENU;
        static gint icon_size_large_dialog = 0;

        if (!icon_size_large_dialog)
        {
            gint width = 48, height = 48;
            gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &width, &height);
            icon_size_large_dialog = gtk_icon_size_register ("large-dialog", width * 2, height * 2);
        }

        if (g_ascii_isalpha (icon_name[0]))
        {
            if (g_str_has_prefix (icon_name, "dialog/"))
            {
                icon_name = &icon_name [strlen("dialog/")];
                icon_size = icon_size_large_dialog;
            }
            else
                icon_size = GTK_ICON_SIZE_BUTTON;
        }
        else if (g_ascii_isdigit (icon_name[0]))
        {
            guint i = 0;
            while (icon_name[i])
                if (icon_name[i++] == '/')
                {
                    gchar* size = g_strndup (icon_name, i - 1);
                    icon_size = atoi (size);
                    /* Compatibility: map pixel to symbolic size */
                    if (icon_size == 16)
                        icon_size = GTK_ICON_SIZE_MENU;
                    g_free (size);
                    icon_name = &icon_name[i];
                }
        }

        /* Render icon as a PNG at the desired size */
        pixbuf = gtk_widget_render_icon (GTK_WIDGET (view), icon_name, icon_size, NULL);
        if (!pixbuf)
            pixbuf = gtk_widget_render_icon (GTK_WIDGET (view),
                GTK_STOCK_MISSING_IMAGE, icon_size, NULL);
        if (pixbuf)
        {
            gboolean success;
            gchar* buffer;
            gsize buffer_size;
            gchar* encoded;
            gchar* data_uri;

            success = gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &buffer_size, "png", NULL, NULL);
            g_object_unref (pixbuf);
            if (!success)
                return;

            encoded = g_base64_encode ((guchar*)buffer, buffer_size);
            data_uri = g_strconcat ("data:image/png;base64,", encoded, NULL);
            g_free (encoded);
            GInputStream* stream = g_memory_input_stream_new_from_data (buffer, buffer_size, g_free);
            webkit_uri_scheme_request_finish (request, stream, buffer_size, "image/png");
            g_object_unref (stream);
            g_free (data_uri);
            return;
        }
    }
}

static void
midori_view_infobar_response_cb (GtkWidget* infobar,
                                 gint       response,
                                 gpointer   data_object)
{
    void (*response_cb) (GtkWidget*, gint, gpointer);
    response_cb = g_object_get_data (G_OBJECT (infobar), "midori-infobar-cb");
    if (response_cb != NULL)
        response_cb (infobar, response, data_object);
    gtk_widget_destroy (infobar);
}

/**
 * midori_view_add_info_bar
 * @view: a #MidoriView
 * @message_type: a #GtkMessageType
 * @message: a message string
 * @response_cb: a response callback
 * @user_data: user data passed to the callback
 * @first_button_text: button text or stock ID
 * @...: first response ID, then more text - response ID pairs
 *
 * Adds an infobar (or equivalent) to the view. Activation of a
 * button invokes the specified callback. The infobar is
 * automatically destroyed if the location changes or reloads.
 *
 * Return value: an infobar widget
 *
 * Since: 0.2.9
 **/
GtkWidget*
midori_view_add_info_bar (MidoriView*    view,
                          GtkMessageType message_type,
                          const gchar*   message,
                          GCallback      response_cb,
                          gpointer       data_object,
                          const gchar*   first_button_text,
                          ...)
{
    GtkWidget* infobar;
    GtkWidget* action_area;
    GtkWidget* content_area;
    GtkWidget* label;
    va_list args;
    const gchar* button_text;

    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);
    g_return_val_if_fail (message != NULL, NULL);

    va_start (args, first_button_text);

    infobar = gtk_info_bar_new ();
    for (button_text = first_button_text; button_text;
         button_text = va_arg (args, const gchar*))
    {
        gint response_id = va_arg (args, gint);
        gtk_info_bar_add_button (GTK_INFO_BAR (infobar),
                                 button_text, response_id);
    }
    gtk_info_bar_set_message_type (GTK_INFO_BAR (infobar), message_type);
    content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (infobar));
    action_area = gtk_info_bar_get_action_area (GTK_INFO_BAR (infobar));
    gtk_orientable_set_orientation (GTK_ORIENTABLE (action_area),
                                    GTK_ORIENTATION_HORIZONTAL);
    g_signal_connect (infobar, "response",
        G_CALLBACK (midori_view_infobar_response_cb), data_object);

    va_end (args);
    label = gtk_label_new (message);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_container_add (GTK_CONTAINER (content_area), label);
    gtk_widget_show_all (infobar);
    gtk_box_pack_start (GTK_BOX (view), infobar, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (view), infobar, 0);
    g_object_set_data (G_OBJECT (infobar), "midori-infobar-cb", response_cb);
    if (data_object != NULL)
        g_object_set_data_full (G_OBJECT (infobar), "midori-infobar-da",
            g_object_ref (data_object), g_object_unref);
    return infobar;
}

void
midori_view_set_html (MidoriView*     view,
                      const gchar*    data,
                      const gchar*    uri,
                      void*           web_frame)
{
    g_return_if_fail (MIDORI_IS_VIEW (view));
    g_return_if_fail (data != NULL);

    WebKitWebView* web_view = WEBKIT_WEB_VIEW (view->web_view);
    if (!uri)
        uri = "about:blank";
    /* XXX: with webkit2 ensure child frames do not set tab URI/special/html */
    katze_item_set_uri (view->item, uri);
    midori_tab_set_special (MIDORI_TAB (view), TRUE);
    webkit_web_view_load_alternate_html (web_view, data, uri, uri);
}

static gboolean
midori_view_display_error (MidoriView*     view,
                           const gchar*    uri,
                           const gchar*    error_icon,
                           const gchar*    title,
                           const gchar*    message,
                           const gchar*    description,
                           const gchar*    suggestions,
                           const gchar*    try_again,
                           void*           web_frame)
{
    gchar* path = midori_paths_get_res_filename ("error.html");
    gchar* template;

    if (g_file_get_contents (path, &template, NULL, NULL))
    {
        gchar* title_escaped;
        const gchar* icon;
        gchar* favicon;
        gchar* result;
        gboolean is_main_frame;

        is_main_frame = TRUE;

        #if !GTK_CHECK_VERSION (3, 0, 0)
        /* g_object_get_valist: object class `GtkSettings' has no property named `gtk-button-images' */
        g_type_class_unref (g_type_class_ref (GTK_TYPE_BUTTON));
        #endif

        gboolean show_button_images = TRUE;
        if (uri == NULL)
            uri = midori_tab_get_uri (MIDORI_TAB (view));
        title_escaped = g_markup_escape_text (title ? title : view->title, -1);
        icon = katze_item_get_icon (view->item);
        favicon = icon && !g_str_has_prefix (icon, "stock://")
          ? g_strdup_printf ("<link rel=\"shortcut icon\" href=\"%s\" />", icon) : NULL;
        result = sokoke_replace_variables (template,
            "{dir}", gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL ?
                "rtl" : "ltr",
            "{title}", title_escaped,
            "{favicon}", katze_str_non_null (favicon),
            "{error_icon}", katze_str_non_null (error_icon),
            "{message}", message,
            "{description}", description,
            "{suggestions}", katze_str_non_null (suggestions),
            "{tryagain}", try_again,
            "{uri}", uri,
            "{hide-button-images}", show_button_images ? "" : "display:none",
            "{autofocus}", is_main_frame ? "autofocus=\"true\" " : "",
            NULL);
        g_free (favicon);
        g_free (title_escaped);
        g_free (template);

        midori_view_set_html (view, result, uri, web_frame);

        g_free (result);
        g_free (path);

        return TRUE;
    }
    g_free (path);

    return FALSE;
}

static gboolean
webkit_web_view_load_error_cb (WebKitWebView*  web_view,
                               WebKitLoadEvent load_event,
                               const gchar*    uri,
                               GError*         error,
                               MidoriView*     view)
{
    /*in WebKit2's UIProcess/API/gtk/WebKitLoaderClient.cpp,
    didFailProvisionalLoadWithErrorForFrame early-returns if the frame isn't
    main, so we know that the pertinent frame here is the view's main frame--so
    it's safe for midori_view_display_error to assume it fills in a main frame*/
    void* web_frame = NULL;
    gchar* title;
    gchar* message;
    gboolean result;

    /* The unholy trinity; also ignored in Webkit's default error handler */
    switch (error->code)
    {
    case WEBKIT_PLUGIN_ERROR_WILL_HANDLE_LOAD:
        /* A plugin will take over. That's expected, it's not fatal. */
    case WEBKIT_NETWORK_ERROR_CANCELLED:
        /* Mostly initiated by JS redirects. */
    case WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE:
        /* A frame load is cancelled because of a download. */
        return FALSE;
    }

    if (!g_network_monitor_get_network_available (g_network_monitor_get_default ()))
    {
        title = g_strdup_printf (_("You are not connected to a network"));
        message = g_strdup_printf (_("Your computer must be connected to a network to reach “%s”. "
                                     "Connect to a wireless access point or attach a network cable and try again."), 
                                     uri);
    } 
    else if (!g_network_monitor_can_reach (g_network_monitor_get_default (), 
                                           g_network_address_parse_uri ("http://www.baidu.com/", 80, NULL), 
                                           NULL, 
                                           NULL))
    {
        title = g_strdup_printf (_("You are not connected to the Internet"));
        message = g_strdup_printf (_("Your computer must be connected to a network to reach “%s”. "
                                     "Connect to a wireless access point or attach a network cable and try again."), 
                                     uri);
    } 
    else
    {
        title = g_strdup_printf (_("Can't find the page you're opening"));
        message = g_strdup_printf (_("The page located at “%s” cannot be found. "
                                     "Check the web address for misspelled words and try again."), 
                                     midori_uri_parse_hostname(uri, NULL));
    }

    result = midori_view_display_error (view, uri, "stock://dialog/network-error", title,
                                        message, error->message, NULL,
                                        _("Try Again"), web_frame);

    midori_tab_set_load_error (MIDORI_TAB (view), MIDORI_LOAD_ERROR_NETWORK);

    g_free (message);
    g_free (title);
    return result;
}

static void
midori_view_apply_scroll_position (MidoriView* view)
{
    if (view->scrollh > -2)
    {
        if (view->scrollh > 0)
        {
            GtkScrolledWindow* scrolled = GTK_SCROLLED_WINDOW (view->scrolled_window);
            GtkAdjustment* adjustment = gtk_scrolled_window_get_hadjustment (scrolled);
            gtk_adjustment_set_value (adjustment, view->scrollh);
        }
        view->scrollh = -3;
    }
    if (view->scrollv > -2)
    {
        if (view->scrollv > 0)
        {
            GtkScrolledWindow* scrolled = GTK_SCROLLED_WINDOW (view->scrolled_window);
            GtkAdjustment* adjustment = gtk_scrolled_window_get_vadjustment (scrolled);
            gtk_adjustment_set_value (adjustment, view->scrollv);
        }
        view->scrollv = -3;
    }
}


DIR *dir = NULL;
struct dirent *ptr=NULL;
char cavas_path[512]={0};

static void
midori_view_load_finished (MidoriView* view)
{
    midori_view_apply_scroll_position (view);
    midori_tab_set_progress (MIDORI_TAB (view), 1.0);
    midori_view_update_load_status (view, MIDORI_LOAD_FINISHED);

    gchar* home = getenv("HOME");
    gchar tmp_file[2048];
    g_sprintf(tmp_file, "%s/.config/cdosbrowser/tmp.txt", home);
    if(g_access(tmp_file, F_OK))
       return;
    char *path;
    if(strlen(cavas_path)==0)
    {
       strcpy(cavas_path,midori_paths_get_res_dir());
       strcat(cavas_path,"/bitmap_html/");
    }
    if(dir ==NULL)
       dir=opendir(cavas_path);
    while((ptr=readdir(dir))!=NULL)
    {
       if((strcmp(ptr->d_name,".")==0) || (strcmp(ptr->d_name,"..")==0))
          continue;
       path=(char *)malloc(strlen(ptr->d_name)+strlen(cavas_path)+1);
       strcpy(path,cavas_path);
       strcat(path,ptr->d_name);
       midori_view_set_uri(view,path);
       free(path);
       path=NULL;
       break;
    }
}

static void
midori_view_web_view_crashed_cb (WebKitWebView* web_view,
                                 MidoriView*    view)
{
    const gchar* uri = webkit_web_view_get_uri (web_view);
    gchar* title = g_strdup_printf (_("Oops - %s"), uri);
    gchar* message = g_strdup_printf (_("Something went wrong with '%s'."), uri);
    midori_view_display_error (view, uri, NULL, title,
        message, "", NULL, _("Try again"), NULL);
    g_free (message);
    g_free (title);
}

static void
midori_view_web_view_load_changed_cb (WebKitWebView*  web_view,
                                      WebKitLoadEvent load_event,
                                      MidoriView*     view)
{
    g_object_freeze_notify (G_OBJECT (view));

    switch (load_event)
    {
    case WEBKIT_LOAD_STARTED:
        midori_view_load_started (view);
        break;
    case WEBKIT_LOAD_REDIRECTED:
        /* Not implemented */
        break;
    case WEBKIT_LOAD_COMMITTED:
        midori_view_load_committed (view);
        break;
    case WEBKIT_LOAD_FINISHED:
        midori_view_load_finished (view);
        break;
    default:
        g_warn_if_reached ();
    }

    g_object_thaw_notify (G_OBJECT (view));
}

static void
midori_web_view_notify_icon_uri_cb (WebKitWebView* web_view,
                                    GParamSpec*    pspec,
                                    MidoriView*    view)
{
#ifdef APP_LEVEL_TIME
printf("signal(notify-icon) callback start time = %lld\n",g_get_real_time());
#endif
    const gchar* uri = webkit_web_view_get_uri (web_view);
    //ZRL 修复当uri为空时crash问题。
    if (uri == NULL)
        return;
    WebKitWebContext* context = webkit_web_context_get_default ();
    WebKitFaviconDatabase* favicon_database = webkit_web_context_get_favicon_database (context);
    gchar* icon_uri = webkit_favicon_database_get_favicon_uri (favicon_database, uri);
    katze_assign (view->icon_uri, icon_uri);
    _midori_web_view_load_icon (view);
#ifdef APP_LEVEL_TIME
printf("signal(notify-icon) callback end time = %lld\n",g_get_real_time());
#endif
}

static void
webkit_web_view_notify_title_cb (WebKitWebView* web_view,
                                 GParamSpec*    pspec,
                                 MidoriView*    view)
{
    const gchar* title = webkit_web_view_get_title (web_view);
    midori_view_set_title (view, title);
    g_object_notify (G_OBJECT (view), "title");
}

#if GTK_CHECK_VERSION(3, 2, 0)
static gboolean
midori_view_overlay_frame_enter_notify_event_cb (GtkOverlay*       overlay,
                                                 GdkEventCrossing* event,
                                                 GtkWidget*        frame)
{
    /* Flip horizontal position of the overlay frame */
    gtk_widget_set_halign (frame,
        gtk_widget_get_halign (frame) == GTK_ALIGN_START
        ? GTK_ALIGN_END : GTK_ALIGN_START);
    return FALSE;
}
#endif

static gboolean
midori_view_web_view_leave_notify_event_cb (WebKitWebView*    web_view,
                                            GdkEventCrossing* event,
                                            MidoriView*       view)
{
    midori_tab_set_statusbar_text (MIDORI_TAB (view), NULL);
    return FALSE;
}

static void
webkit_web_view_hovering_over_link_cb (WebKitWebView*       web_view,
                                       WebKitHitTestResult* hit_test_result,
                                       guint                modifiers,
                                       MidoriView*          view)
{
    katze_object_assign (view->hit_test, g_object_ref (hit_test_result));
    if (!webkit_hit_test_result_context_is_link (hit_test_result))
    {
        //add by luyue 2015/4/27 start
        midori_tab_set_statusbar_text (MIDORI_TAB (view), NULL);
        //add end
        katze_assign (view->link_uri, NULL);
        return;
    }
    const gchar* link_uri = webkit_hit_test_result_get_link_uri (hit_test_result);

    katze_assign (view->link_uri, g_strdup (link_uri));
    if (link_uri && g_str_has_prefix (link_uri, "mailto:"))
    {
        gchar* text = g_strdup_printf (_("Send a message to %s"), &link_uri[7]);
        midori_tab_set_statusbar_text (MIDORI_TAB (view), text);
        g_free (text);
    }
    else
        midori_tab_set_statusbar_text (MIDORI_TAB (view), link_uri);
}

static gboolean
midori_view_always_same_tab (const gchar* uri)
{
    /* No opening in tab, window or app for Javascript or mailto links */
    return g_str_has_prefix (uri, "javascript:") || g_str_has_prefix (uri, "mailto:");
}

#define MIDORI_KEYS_MODIFIER_MASK (GDK_SHIFT_MASK | GDK_CONTROL_MASK \
    | GDK_MOD1_MASK | GDK_META_MASK | GDK_SUPER_MASK | GDK_HYPER_MASK )

static gboolean
midori_view_web_view_button_press_event_cb (WebKitWebView*  web_view,
                                            GdkEventButton* event,
                                            MidoriView*     view)
{
    const gchar* link_uri;
    gboolean background;

    event->state = event->state & MIDORI_KEYS_MODIFIER_MASK;
    link_uri = midori_view_get_link_uri (view);
    view->button_press_handled = FALSE;

    if (midori_debug ("mouse"))
        g_message ("%s button %d\n", G_STRFUNC, event->button);

    switch (event->button)
    {
    case 1:
        if (!link_uri)
            return FALSE;

        if (midori_view_always_same_tab (link_uri))
            return FALSE;

        if (MIDORI_MOD_NEW_TAB (event->state))
        {
            /* Open link in new tab */
            background = view->open_tabs_in_the_background;
            if (MIDORI_MOD_BACKGROUND (event->state))
                background = !background;
            g_signal_emit (view, signals[NEW_TAB], 0, link_uri, background);
            view->button_press_handled = TRUE;
            return TRUE;
        }
        else if (MIDORI_MOD_NEW_WINDOW (event->state))
        {
            /* Open link in new window */
            g_signal_emit (view, signals[NEW_WINDOW], 0, link_uri);
            view->button_press_handled = TRUE;
            return TRUE;
        }
        break;
    case 2:
        if (link_uri)
        {
            if (midori_view_always_same_tab (link_uri))
                return FALSE;

            /* Open link in new tab */
            background = view->open_tabs_in_the_background;
            if (MIDORI_MOD_BACKGROUND (event->state))
                background = !background;
            g_signal_emit (view, signals[NEW_TAB], 0, link_uri, background);
            view->button_press_handled = TRUE;
            return TRUE;
        }
        #if GTK_CHECK_VERSION (3, 4, 0)
        if (katze_object_get_boolean (gtk_widget_get_settings (view->web_view), "gtk-enable-primary-paste"))
        #else
        if (midori_settings_get_middle_click_opens_selection (MIDORI_SETTINGS (view->settings)))
        #endif
        {
        }
        if (MIDORI_MOD_SCROLL (event->state))
        {
            midori_view_set_zoom_level (MIDORI_VIEW (view), 1.0);
            return FALSE; /* Allow Ctrl + Middle click */
        }
        return FALSE;
        break;
    case 3:
        /* Older versions don't have the context-menu signal */
        #if WEBKIT_CHECK_VERSION (1, 10, 0)
        if (event->state & GDK_CONTROL_MASK)
        #endif
        {
            /* Ctrl + Right-click suppresses javascript button handling */
            GtkWidget* menu = gtk_menu_new ();
            midori_view_populate_popup (view, menu, TRUE);
            katze_widget_popup (GTK_WIDGET (web_view), GTK_MENU (menu), event,
                                KATZE_MENU_POSITION_CURSOR);
            view->button_press_handled = TRUE;
            return TRUE;
        }
        break;
#ifdef G_OS_WIN32
    case 4:
#else
    case 8:
#endif
        midori_view_go_back (view);
        view->button_press_handled = TRUE;
        return TRUE;
#ifdef G_OS_WIN32
    case 5:
#else
    case 9:
#endif
        midori_tab_go_forward (MIDORI_TAB (view));
        view->button_press_handled = TRUE;
        return TRUE;
    /*
     * On some fancier mice the scroll wheel can be used to scroll horizontally.
     * A middle click usually registers both a middle click (2) and a
     * horizontal scroll (11 or 12).
     * We catch horizontal scrolls and ignore them to prevent middle clicks from
     * accidentally being interpreted as first button clicks.
     */
    case 11:
    case 12:
        view->button_press_handled = TRUE;
        return TRUE;
    }

    /* We propagate the event, since it may otherwise be stuck in WebKit */
    g_signal_emit_by_name (view, "event", event, &background);
    return FALSE;
}

static gboolean
midori_view_web_view_button_release_event_cb (WebKitWebView*  web_view,
                                              GdkEventButton* event,
                                              MidoriView*     view)
{
    gboolean button_press_handled = view->button_press_handled;
    view->button_press_handled = FALSE;

    return button_press_handled;
}

static gboolean
gtk_widget_key_press_event_cb (WebKitWebView* web_view,
                               GdkEventKey*   event,
                               MidoriView*    view)
{
    guint character;

    event->state = event->state & MIDORI_KEYS_MODIFIER_MASK;

    /* Handle oddities in Russian keyboard layouts */
    if (event->hardware_keycode == ';' || event->hardware_keycode == '=')
        event->keyval = ',';
    else if (event->hardware_keycode == '<')
        event->keyval = '.';

    /* Find links by number: . to show links, type number, Return to go */
    if ( event->keyval == '.' || view->find_links > -1 )
    {
        return FALSE;
    }

    /* Find inline */
    if (event->keyval == ',' || event->keyval == '/' || event->keyval == GDK_KEY_KP_Divide)
        character = '\0';
    else
        return FALSE;

    /* Skip control characters */
    if (character == (event->keyval | 0x01000000))
        return FALSE;

    WebKitHitTestResultContext context = katze_object_get_int (view->hit_test, "context");
    if (!(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE))
    {
        gchar* text = character ? g_strdup_printf ("%c", character) : NULL;
        #if GTK_CHECK_VERSION(3, 2, 0)
        midori_findbar_search_text (MIDORI_FINDBAR (view->overlay_find),
            (GtkWidget*)view, TRUE, katze_str_non_null (text));
        #else
        g_signal_emit_by_name (view, "search-text", TRUE, katze_str_non_null (text));
        #endif
        g_free (text);
        return TRUE;
    }
    return FALSE;
}

static gboolean
gtk_widget_scroll_event_cb (WebKitWebView*  web_view,
                            GdkEventScroll* event,
                            MidoriView*     view)
{
    event->state = event->state & MIDORI_KEYS_MODIFIER_MASK;

    if (MIDORI_MOD_SCROLL (event->state))
    {
        if (event->direction == GDK_SCROLL_DOWN)
            midori_view_set_zoom_level (view,
                midori_view_get_zoom_level (view) - 0.10f);
        else if(event->direction == GDK_SCROLL_UP)
            midori_view_set_zoom_level (view,
                midori_view_get_zoom_level (view) + 0.10f);
        return TRUE;
    }
    else
        return FALSE;
}

static void
midori_web_view_menu_new_window_activate_cb (GtkAction* action,
                                             gpointer   user_data)
{
    MidoriView* view = user_data;
    g_signal_emit (view, signals[NEW_WINDOW], 0, view->link_uri);
}

static void
midori_web_view_menu_link_copy_activate_cb (GtkAction* widget,
                                            gpointer   user_data)
{
    MidoriView* view = user_data;
    if (g_str_has_prefix (view->link_uri, "mailto:"))
        sokoke_widget_copy_clipboard (view->web_view, view->link_uri + 7, NULL, NULL);
    else
        sokoke_widget_copy_clipboard (view->web_view, view->link_uri, NULL, NULL);
}

static void
midori_view_download_uri (MidoriView*        view,
                          const gchar*       uri)
{
    WebKitDownload* download = webkit_web_view_download_uri (WEBKIT_WEB_VIEW (view->web_view), uri);
    WebKitWebContext * web_context = webkit_web_view_get_context (WEBKIT_WEB_VIEW (view->web_view));
    g_signal_emit_by_name (web_context, "download-started", download, view);
}

static void
midori_web_view_menu_save_activate_cb (GtkAction* action,
                                       gpointer   user_data)
{
    MidoriView* view = user_data;
    //add by luyue 2015/6/9
    //保存页面，不走下载流程，走页面另存为流程
    view->save_menu_flag = true;
    MidoriBrowser* browser = midori_browser_get_for_widget (GTK_WIDGET (view));

    KatzeItem* item = katze_item_new ();
    item->uri = g_strdup (view->link_uri);
    MidoriView* new_view = (MidoriView*)midori_view_new_from_view (view,item,NULL);
    new_view->save_menu_flag = true;
    webkit_web_view_load_uri (WEBKIT_WEB_VIEW (new_view->web_view),view->link_uri);
    midori_browser_save_uri (browser,new_view,view->link_uri,"s");
}

static void
midori_web_view_menu_image_new_tab_activate_cb (GtkAction* action,
                                                gpointer   user_data)
{
    MidoriView* view = user_data;
    gchar* uri = katze_object_get_string (view->hit_test, "image-uri");
    if (view->open_new_pages_in == MIDORI_NEW_PAGE_WINDOW)
        g_signal_emit (view, signals[NEW_WINDOW], 0, uri);
    else
        g_signal_emit (view, signals[NEW_TAB], 0, uri,
            view->open_tabs_in_the_background);
    g_free (uri);
}

/**
 * midori_view_get_resources:
 * @view: a #MidoriView
 *
 * Obtain a list of the resources loaded by the page shown in the view.
 *
 * Return value: (transfer full) (element-type WebKitWebResource): the resources
 **/
GList*
midori_view_get_resources (MidoriView* view)
{
    return NULL;
}

static GString*
midori_view_get_data_for_uri (MidoriView*  view,
                              const gchar* uri)
{
    GList* resources = midori_view_get_resources (view);
    GString* result = NULL;
    g_list_foreach (resources, (GFunc)g_object_unref, NULL);
    g_list_free (resources);
    return result;
}

static void
midori_view_clipboard_get_image_cb (GtkClipboard*     clipboard,
                                    GtkSelectionData* selection_data,
                                    guint             info,
                                    gpointer          user_data)
{
    MidoriView* view = MIDORI_VIEW (g_object_get_data (user_data, "view"));
    WebKitHitTestResult* hit_test = user_data;
    gchar* uri = katze_object_get_string (hit_test, "image-uri");
    GdkAtom target = gtk_selection_data_get_target (selection_data);
    if (gtk_targets_include_image (&target, 1, TRUE))
    {
        GString* data = midori_view_get_data_for_uri (view, uri);
        if (data != NULL)
        {
            GInputStream* stream = g_memory_input_stream_new_from_data (data->str, data->len, NULL);
            GError* error = NULL;
            GdkPixbuf* pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, &error);
            g_object_unref (stream);
            if (error != NULL)
            {
                g_critical ("Error copying pixbuf: %s\n", error->message);
                g_error_free (error);
            }
            gtk_selection_data_set_pixbuf (selection_data, pixbuf);
            g_object_unref (pixbuf);
        }
        else
            g_warn_if_reached ();
    }
    if (gtk_targets_include_text (&target, 1))
        gtk_selection_data_set_text (selection_data, uri, -1);
    g_free (uri);
}

static void
midori_web_view_menu_image_copy_activate_cb (GtkAction* action,
                                             gpointer   user_data)
{
    MidoriView* view = user_data;
    gchar* uri = katze_object_get_string (view->hit_test, "image-uri");
    g_object_set_data (G_OBJECT (view->hit_test), "view", view);
    sokoke_widget_copy_clipboard (view->web_view,
        uri, midori_view_clipboard_get_image_cb, view->hit_test);
    g_free (uri);
}

static void
midori_web_view_menu_image_save_activate_cb (GtkAction* action,
                                             gpointer   user_data)
{
    MidoriView* view = user_data;
    gchar* uri = katze_object_get_string (view->hit_test, "image-uri");
    midori_view_download_uri (view, uri);
    g_free (uri);
}

static void
midori_web_view_menu_video_copy_activate_cb (GtkAction* action,
                                             gpointer   user_data)
{
    MidoriView* view = user_data;
    gchar* uri = katze_object_get_string (view->hit_test, "media-uri");
    sokoke_widget_copy_clipboard (view->web_view, uri, NULL, NULL);
    g_free (uri);
}

static void
midori_web_view_menu_video_save_activate_cb (GtkAction* action,
                                             gpointer   user_data)
{
    MidoriView* view = user_data;
    gchar* uri = katze_object_get_string (view->hit_test, "media-uri");
    midori_view_download_uri (view, uri);
    g_free (uri);
}

static void
midori_view_menu_open_link_tab_activate_cb (GtkAction* action,
                                            gpointer   user_data)
{
    MidoriView* view = user_data;
    gchar* data = (gchar*)g_object_get_data (G_OBJECT (action), "uri");
    gchar* uri = sokoke_magic_uri (data, TRUE, FALSE);
    if (!uri)
        uri = g_strdup (data);
    g_signal_emit (view, signals[NEW_TAB], 0, uri,
                   view->open_tabs_in_the_background);
    g_free (uri);
}

static void
midori_web_view_menu_background_tab_activate_cb (GtkAction* action,
                                                 gpointer   user_data)
{
    MidoriView* view = user_data;
    g_signal_emit (view, signals[NEW_TAB], 0, view->link_uri,
                   !view->open_tabs_in_the_background);
}

static void
midori_view_tab_label_menu_window_new_cb (GtkAction* action,
                                          gpointer   user_data)
{
    MidoriView* view = user_data;
    g_signal_emit (view, signals[NEW_WINDOW], 0,
        midori_view_get_display_uri (MIDORI_VIEW (view)));
}

static void
midori_view_inspect_element_activate_cb (GtkAction* action,
                                         gpointer   user_data)
{
    MidoriView* view = user_data;
    WebKitWebInspector* inspector = webkit_web_view_get_inspector (WEBKIT_WEB_VIEW (view->web_view));
    webkit_web_inspector_show (inspector);
}

// ZRL 暂时屏蔽搜索框功能
#if ENABLE_SEARCH_ACTION
static void
midori_view_add_search_engine_cb (GtkWidget*  widget,
                                  MidoriView* view)
{
    MidoriBrowser* browser = midori_browser_get_for_widget (view->web_view);
    GtkActionGroup* actions = midori_browser_get_action_group (browser);
    GtkAction* action = gtk_action_group_get_action (actions, "Search");
    KatzeItem* item = g_object_get_data (G_OBJECT (widget), "item");
    midori_search_action_get_editor (MIDORI_SEARCH_ACTION (action), item, TRUE);
}
#endif

/**
 * midori_view_get_page_context_action:
 * @view: a #MidoriView
 * @hit_test_result: a #WebKitHitTestResult
 *
 * Populates actions depending on the hit test result.
 *
 * Since: 0.5.5
 */
MidoriContextAction*
midori_view_get_page_context_action (MidoriView*          view,
                                     WebKitHitTestResult* hit_test_result)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);
    g_return_val_if_fail (hit_test_result != NULL, NULL);

    MidoriBrowser* browser = midori_browser_get_for_widget (GTK_WIDGET (view));
    GdkWindowState state = gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (browser)));
    WebKitHitTestResultContext context = katze_object_get_int (hit_test_result, "context");
    GtkActionGroup* actions = midori_browser_get_action_group (browser);
    MidoriContextAction* menu = midori_context_action_new ("PageContextMenu", NULL, NULL, NULL);
    midori_context_action_add_action_group (menu, actions);

    if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE)
    {
        /* Enforce update of actions - there's no "selection-changed" signal */
        midori_tab_update_actions (MIDORI_TAB (view), actions, NULL, NULL);
        midori_context_action_add_by_name (menu, "Undo");
        midori_context_action_add_by_name (menu, "Redo");
        midori_context_action_add (menu, NULL);
        midori_context_action_add_by_name (menu, "Cut");
        midori_context_action_add_by_name (menu, "Copy");
        midori_context_action_add_by_name (menu, "Paste");
        midori_context_action_add_by_name (menu, "Delete");
        midori_context_action_add (menu, NULL);
        midori_context_action_add_by_name (menu, "SelectAll");
        midori_context_action_add (menu, NULL);
// ZRL 暂时屏蔽搜索框功能
#if ENABLE_SEARCH_ACTION
        KatzeItem* item = midori_search_action_get_engine_for_form (
            WEBKIT_WEB_VIEW (view->web_view), view->ellipsize);
        if (item != NULL)
        {
            GtkAction* action = gtk_action_new ("AddSearchEngine", _("Add _search engine..."), NULL, NULL);
            g_object_set_data (G_OBJECT (action), "item", item);
            g_signal_connect (action, "activate",
                              G_CALLBACK (midori_view_add_search_engine_cb), view);
            midori_context_action_add (menu, action);
        }
#endif
        /* FIXME: input methods */
        /* FIXME: font */
        /* FIXME: insert unicode character */
    }

    if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK)
    {
        if (midori_paths_get_runtime_mode () == MIDORI_RUNTIME_MODE_APP)
        {
            GtkAction* action = gtk_action_new ("OpenLinkTab", _("Open _Link"), NULL, STOCK_TAB_NEW);
            g_object_set_data_full (G_OBJECT (action), "uri", g_strdup (view->link_uri), (GDestroyNotify)g_free);
            g_signal_connect (action, "activate", G_CALLBACK (midori_view_menu_open_link_tab_activate_cb), view);
            midori_context_action_add (menu, action);
        }
        else if (!midori_view_always_same_tab (view->link_uri))
        {
            GtkAction* action = gtk_action_new ("OpenLinkTab", _("Open Link in New _Tab"), NULL, STOCK_TAB_NEW);
            g_object_set_data_full (G_OBJECT (action), "uri", g_strdup (view->link_uri), (GDestroyNotify)g_free);
            g_signal_connect (action, "activate", G_CALLBACK (midori_view_menu_open_link_tab_activate_cb), view);
            midori_context_action_add (menu, action);
            midori_context_action_add_simple (menu, "OpenLinkForegroundTab",
                view->open_tabs_in_the_background
                ? _("Open Link in _Foreground Tab") : _("Open Link in _Background Tab"), NULL, NULL,
                midori_web_view_menu_background_tab_activate_cb, view);
            midori_context_action_add_simple (menu, "OpenLinkWindow", _("Open Link in New _Window"), NULL, STOCK_WINDOW_NEW,
                midori_web_view_menu_new_window_activate_cb, view);
        }

        midori_context_action_add_simple (menu, "CopyLinkDestination", _("Copy Link de_stination"), NULL, NULL,
            midori_web_view_menu_link_copy_activate_cb, view);

        if (!midori_view_always_same_tab (view->link_uri))
        {
            /* GTK_STOCK_SAVE_AS is lacking the underline */
            midori_context_action_add_simple (menu, "SaveLinkAs", _("Save _As…"), NULL, GTK_STOCK_SAVE_AS,
                midori_web_view_menu_save_activate_cb, view);
        }
    }

    if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE)
    {
        if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK)
            midori_context_action_add (menu, NULL);

        midori_context_action_add_simple (menu, "OpenImageNewTab",
            view->open_new_pages_in == MIDORI_NEW_PAGE_WINDOW
            ? _("Open _Image in New Window") : _("Open _Image in New Tab")
            , NULL, STOCK_TAB_NEW,
            midori_web_view_menu_image_new_tab_activate_cb, view);
        //add by luyue 2015/8/10 start
        //非本地图片可以保存
        if(strncmp(midori_tab_get_uri (MIDORI_TAB (view)),"file",4))
        {
           midori_context_action_add_simple (menu, "CopyImage", _("Copy Im_age"), NULL, NULL,
               midori_web_view_menu_image_copy_activate_cb, view);
           midori_context_action_add_simple (menu, "SaveImage", _("Save I_mage"), NULL, GTK_STOCK_SAVE,
               midori_web_view_menu_image_save_activate_cb, view);
        }
       //add end
    }

    if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA)
    {
        midori_context_action_add_simple (menu, "CopyVideoAddress", _("Copy Video _Address"), NULL, NULL,
            midori_web_view_menu_video_copy_activate_cb, view);
        midori_context_action_add_simple (menu, "DownloadVideo", _("Download _Video"), NULL, GTK_STOCK_SAVE,
            midori_web_view_menu_video_save_activate_cb, view);
    }

    if (context == WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT)
    {
        midori_context_action_add_by_name (menu, "Back");
        midori_context_action_add_by_name (menu, "Forward");
        midori_context_action_add_by_name (menu, "Stop");
        midori_context_action_add_by_name (menu, "Reload");
    }

    /* No need to have Copy twice, which is already in the editable menu */
    if (!(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE))
    {
        midori_context_action_add (menu, NULL);
        /* Enforce update of actions - there's no "selection-changed" signal */
        midori_tab_update_actions (MIDORI_TAB (view), actions, NULL, NULL);
        midori_context_action_add_by_name (menu, "Copy");
        midori_context_action_add_by_name (menu, "SelectAll");
        midori_context_action_add_by_name (menu, "Print");
    }

    if (context == WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT)
    {
        midori_context_action_add (menu, NULL);
#if ENABLE_TRASH // ZRL 屏蔽撤销关闭标签功能
        midori_context_action_add_by_name (menu, "UndoTabClose");
#endif
        midori_context_action_add_simple (menu, "OpenInNewWindow", _("Open in New _Window"), NULL, STOCK_WINDOW_NEW,
        midori_view_tab_label_menu_window_new_cb, view);
        midori_context_action_add_by_name (menu, "ZoomIn");
        midori_context_action_add_by_name (menu, "ZoomOut");

        MidoriContextAction* encodings = midori_context_action_new ("Encoding", _("_Encoding"), NULL, NULL);
        midori_context_action_add (menu, GTK_ACTION (encodings));
        midori_context_action_add_by_name (encodings, "EncodingAutomatic");
        midori_context_action_add_by_name (encodings, "EncodingChinese");
        midori_context_action_add_by_name (encodings, "EncodingChineseSimplified");
        midori_context_action_add_by_name (encodings, "EncodingJapanese");
        midori_context_action_add_by_name (encodings, "EncodingKorean");
        midori_context_action_add_by_name (encodings, "EncodingRussian");
        midori_context_action_add_by_name (encodings, "EncodingUnicode");
        midori_context_action_add_by_name (encodings, "EncodingWestern");
        midori_context_action_add (menu, NULL);
        midori_context_action_add_by_name (menu, "BookmarkAdd");
        midori_context_action_add_by_name (menu, "SaveAs");
        midori_context_action_add_by_name (menu, "SourceView");
        if (!g_object_get_data (G_OBJECT (browser), "midori-toolbars-visible"))
            midori_context_action_add_by_name (menu, "Navigationbar");
        if (state & GDK_WINDOW_STATE_FULLSCREEN)
            midori_context_action_add_by_name (menu, "Fullscreen");
    }

    if (katze_object_get_boolean (view->settings, "enable-developer-extras"))
        midori_context_action_add_simple (menu, "InspectElement", _("Inspect _Element"), NULL, NULL,
            midori_view_inspect_element_activate_cb, view);

    g_signal_emit_by_name (view, "context-menu", hit_test_result, menu);
    return menu;
}

/**
 * midori_view_populate_popup:
 * @view: a #MidoriView
 * @menu: a #GtkMenu
 * @manual: %TRUE if this a manually created popup
 *
 * Populates the given @menu with context menu items
 * according to the position of the mouse pointer. This
 * can be used in situations where a custom hotkey
 * opens the context menu or the default behaviour
 * needs to be intercepted.
 *
 * @manual should usually be %TRUE, except for the
 * case where @menu was created by the #WebKitWebView.
 *
 * Since: 0.2.5
 * Deprecated: 0.5.5: Use midori_view_get_page_context_action().
 */
void
midori_view_populate_popup (MidoriView* view,
                            GtkWidget*  menu,
                            gboolean    manual)
{
    g_return_if_fail (MIDORI_IS_VIEW (view));
    g_return_if_fail (GTK_IS_MENU_SHELL (menu));

    GdkEvent* event = gtk_get_current_event();
    gdk_event_free (event);
    MidoriContextAction* context_action = midori_view_get_page_context_action (view, view->hit_test);
    midori_context_action_create_menu (context_action, GTK_MENU (menu), FALSE);
}

#if WEBKIT_CHECK_VERSION (1, 10, 0)
static gboolean
midori_view_web_view_context_menu_cb (WebKitWebView*       web_view,
                                      WebKitContextMenu*   context_menu,
                                      GdkEvent*            event,
                                      WebKitHitTestResult* hit_test_result,
                                      MidoriView*          view)
{
    MidoriContextAction* menu = midori_view_get_page_context_action (view, hit_test_result);
    /* Retain specific menu items we can't re-create easily */
    guint guesses = 0, guesses_max = 10; /* Maximum number of spelling suggestions */
    GList* items = webkit_context_menu_get_items (context_menu), *item, *preserved = NULL;
    for (item = items; item; item = g_list_next (item))
    {
        WebKitContextMenuAction stock_action = webkit_context_menu_item_get_stock_action (item->data);
        if (stock_action == WEBKIT_CONTEXT_MENU_ACTION_SPELLING_GUESS && guesses++ < guesses_max)
            preserved = g_list_append (preserved, g_object_ref (item->data));
    }
    webkit_context_menu_remove_all (context_menu);
    for (item = preserved; item; item = g_list_next (item))
    {
        webkit_context_menu_append (context_menu, item->data);
        g_object_unref (item->data);
    }
    g_list_free (preserved);
    midori_context_action_create_webkit_context_menu (menu, context_menu);
    return FALSE;
}
#endif

static gboolean
midori_view_web_view_close_cb (WebKitWebView* web_view,
                               GtkWidget*     view)
{
    midori_browser_close_tab1 (midori_browser_get_for_widget (view), view);
    return TRUE;
}

static gboolean
webkit_web_view_web_view_ready_cb (GtkWidget*  web_view,
                                   MidoriView* view)
{
    MidoriNewView where = MIDORI_NEW_VIEW_TAB;
    GtkWidget* new_view = GTK_WIDGET (midori_view_get_for_widget (web_view));

    WebKitWindowProperties* features = webkit_web_view_get_window_properties (WEBKIT_WEB_VIEW (web_view));
    gboolean locationbar_visible, menubar_visible, toolbar_visible;
// ZRL no width & height in webkit2gtk-4.0. should get from GdkRectangle
    GdkRectangle rectangle;
    rectangle.width = 0;
    rectangle.height = 0;
    gint width, height;
    g_object_get (features,
                  "locationbar-visible", &locationbar_visible,
                  "menubar-visible", &menubar_visible,
                  "toolbar-visible", &toolbar_visible,
                  "geometry", &rectangle,
                  NULL);
    width = rectangle.width;
    height = rectangle.height;
    midori_tab_set_is_dialog (MIDORI_TAB (view),
     !locationbar_visible && !menubar_visible && !toolbar_visible
     && width > 0 && height > 0);

    /* Open windows opened by scripts in tabs if they otherwise
        would be replacing the page the user opened. */
    if (view->open_new_pages_in == MIDORI_NEW_PAGE_CURRENT)
        if (!midori_tab_get_is_dialog (MIDORI_TAB (view)))
            return TRUE;

    if (view->open_new_pages_in == MIDORI_NEW_PAGE_TAB)
    {
        if (view->open_tabs_in_the_background)
            where = MIDORI_NEW_VIEW_BACKGROUND;
    }
    else if (view->open_new_pages_in == MIDORI_NEW_PAGE_WINDOW)
        where = MIDORI_NEW_VIEW_WINDOW;

    gtk_widget_show (new_view);
    g_signal_emit (view, signals[NEW_VIEW], 0, new_view, where, FALSE);

    if (midori_tab_get_is_dialog (MIDORI_TAB (view)))
    {
        GtkWidget* toplevel = gtk_widget_get_toplevel (new_view);
        if (width > 0 && height > 0)
            gtk_widget_set_size_request (toplevel, width, height);
        g_signal_handlers_disconnect_by_func (view->web_view,
            midori_view_web_view_close_cb, view);
        g_signal_connect (web_view, "close",
                          G_CALLBACK (midori_view_web_view_close_cb), new_view);
    }

    return TRUE;
}

//add by luyue 2015/8/20 start
//解决链接获取德url为about:blank的问题
static void
midori_view_uri_changed_cb(WebKitWebView*  webView,
                           GParamSpec*     pspec,
                           MidoriView*     view)
{
   const char *destUri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(webView));
   MidoriView* new_view;

   if(strcmp(destUri,"about:blank"))
   {
      if (view->open_new_pages_in == MIDORI_NEW_PAGE_CURRENT)
      {
         new_view = view;
         midori_view_set_uri(MIDORI_VIEW (new_view), destUri);
      }
      else
      {
         KatzeItem* item = katze_item_new ();
         item->uri = g_strdup (destUri);
         new_view = (MidoriView*)midori_view_new_from_view (view, item, NULL);
         g_object_set_data_full (G_OBJECT (new_view), "destination-uri", g_strdup (destUri), g_free);
         webkit_web_view_web_view_ready_cb(new_view->web_view,view);
      }
      g_signal_handlers_disconnect_by_func (webView,midori_view_uri_changed_cb, view);
   }
}
//add end

/* ZRL 从navigationAction读取新uri，并为midori-view新建destination-uri属性，保存该uri。*/
static GtkWidget*
webkit_web_view_create_web_view_cb (GtkWidget*      web_view,
                                    WebKitNavigationAction* navigationAction, // ZRL add the parameter for webkit2gtk-4.0
                                    MidoriView*     view)
{
    MidoriView* new_view;
    WebKitURIRequest *naviationRequest = webkit_navigation_action_get_request(navigationAction);
    const gchar *destUri = webkit_uri_request_get_uri(naviationRequest);

    //add by luyue 2015/8/20 start
    //解决部分获取的destUri为about:blank的bug
    //比如百度地图分享
    if(strcmp(destUri,"about:blank"))
    {
       const gchar* uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (web_view));
       if (view->open_new_pages_in == MIDORI_NEW_PAGE_CURRENT)
          new_view = view;
       else
       {
          KatzeItem* item = katze_item_new ();
          item->uri = g_strdup (uri);
          new_view = (MidoriView*)midori_view_new_from_view (view, item, NULL);

          g_signal_connect (new_view->web_view, "ready-to-show",
                            G_CALLBACK (webkit_web_view_web_view_ready_cb), view);
       }
       g_object_set_data_full (G_OBJECT (new_view), "destination-uri", g_strdup (destUri), g_free);
       return new_view->web_view;
    }
    else
    {
       WebKitWebView *webView = WEBKIT_WEB_VIEW(webkit_web_view_new_with_related_view(web_view));
       g_signal_connect (webView,"notify::uri",midori_view_uri_changed_cb, view);
       return webView;
    }
    //add end
}

//add by luyue 2015/7/8 start
static void
midori_view_get_cookie_cb(WebKitCookieManager* cookiemanager,
                          const gchar*         cookie,
                          MidoriView*          view)
{
   const gchar* uri;
   MidoriBrowser* browser =  midori_browser_get_for_widget((GtkWidget*)view);
   if(browser)
   {
      GtkWidget* tab = midori_browser_get_current_tab ( browser);
      uri = midori_tab_get_uri (MIDORI_TAB(tab));
   }
   else uri = NULL;

   if (cookie && strlen(cookie) && uri)
   {
      view->download_exec = (char *)malloc(strlen(view->download_uri)+strlen(view->download_filename)+strlen(cookie)+strlen(uri)+64);
      sprintf(view->download_exec,"/usr/local/libexec/cdosbrowser/cdosbrowser_download %s%s%s %s%s%s %s%s%s %s%s%s","\"",view->download_uri,"\"","\"",view->download_filename,"\"", "\"",cookie,"\"","\"",uri,"\"","&");  
   }
   else if (uri)
   {
      view->download_exec = (char *)malloc(strlen(view->download_uri)+strlen(view->download_filename)+strlen(uri)+64);
      sprintf(view->download_exec,"/usr/local/libexec/cdosbrowser/cdosbrowser_download %s%s%s %s%s%s %s%s%s %s%s%s","\"",view->download_uri,"\"","\"",view->download_filename,"\"","\"","","\"","\"",uri,"\"","&");
   }
   else
   {
      view->download_exec = (char *)malloc(strlen(view->download_uri)+strlen(view->download_filename)+64);
      sprintf(view->download_exec,"/usr/local/libexec/cdosbrowser/cdosbrowser_download %s%s%s %s%s%s %s%s%s %s%s%s","\"",view->download_uri,"\"","\"",view->download_filename,"\"","\"","","\"","\"","","\"","&");
   }
   free(view->download_uri);
   view->download_uri = NULL;
   free(view->download_filename);
   view->download_filename = NULL;
   pid_t pid;
   pid = fork();
   if(pid == 0)
   {
      execl("/bin/bash", "bash", "-c", view->download_exec, (char *)0);
      _exit(127); //子进程正常执行则不会执行此语句    
   }

   g_signal_handlers_disconnect_by_func (cookiemanager,midori_view_get_cookie_cb, view);
}
//add end

static gboolean
midori_view_download_decide_destination_cb (WebKitDownload*   download,
                                            const gchar *     suggested_filename,
                                            MidoriView*       view)
{
    //add by luyue 2015/7/8 start
    if(view->save_menu_flag)
    {
       webkit_download_cancel (download);
       view->save_menu_flag = false;
       return TRUE;
    }
    WebKitURIRequest *request = webkit_download_get_request(download);
    const gchar* opener_uri = webkit_uri_request_get_uri(request);
    //data:image打头的不走下载流程
    if(strncmp(opener_uri,"data:image",10)==0)
    {
       MidoriBrowser* browser = midori_browser_get_for_widget (GTK_WIDGET (view));
       midori_browser_save_uri (browser,view,opener_uri,"index");
       webkit_download_cancel (download);
       return TRUE;
    }    
    view->download_filename = (char *) malloc (strlen(suggested_filename)+1);
    strcpy(view->download_filename,suggested_filename);
    //add by luyue 2015/8/8 start
    midori_view_reset_user_agent(view);
    //add end
    view->download_uri = (char *)malloc (strlen(opener_uri)+1);
    strcpy(view->download_uri,opener_uri);
    WebKitCookieManager* cookiemanager = webkit_web_context_get_cookie_manager(webkit_web_context_get_default());
    if(cookiemanager)
    {
       g_signal_connect (cookiemanager, "get-cookie",
                         G_CALLBACK (midori_view_get_cookie_cb), view);         
       webkit_cookie_manager_get_cookies_with_url(cookiemanager,opener_uri);
    }
    webkit_download_cancel (download);
    //add end
    return TRUE;    //we must return TRUE because we handled the signal
}
                
static void
midori_view_download_started_cb (WebKitWebContext* context,
                                   WebKitDownload* download,
                                   MidoriView      *view)
{
    g_signal_connect (download, "decide-destination",
                      G_CALLBACK (midori_view_download_decide_destination_cb), view);
}

// added by wangyl 2015.8.5 start 
static gchar*  
string_replace(gchar *string_data, 
               gchar *old, 
               gchar *replacement)
{
   gchar* result = NULL;
   GError * _inner_error_ = NULL;
   g_return_val_if_fail (string_data != NULL, NULL);
   g_return_val_if_fail (old != NULL, NULL);
   g_return_val_if_fail (replacement != NULL, NULL);

   gchar* _tmp1_ = g_regex_escape_string (old, -1);
   GRegex* regex = g_regex_new (_tmp1_, 0, 0, &_inner_error_);
   g_free (_tmp1_);
   if (_inner_error_ != NULL) 
   {
      g_clear_error (&_inner_error_);
      return NULL;
   }
   result = g_regex_replace_literal (regex, string_data, (gssize) (-1), 0, replacement, 0, &_inner_error_);
   if (_inner_error_ != NULL) 
   {
      g_regex_unref (regex);
      g_clear_error (&_inner_error_);
      return NULL;
   }
   g_regex_unref (regex);
   return result;		
}

static KatzeArray* 
midori_view_history_read_from_db(MidoriBrowser* browser, const gchar*   filter)
{
   sqlite3* db;
   sqlite3_stmt* statement;
   gint result;
   const gchar* sqlcmd;
   g_return_if_fail (filter != NULL);
   KatzeArray*history = katze_object_get_object (browser, "history");
   db = g_object_get_data (G_OBJECT (history), "db");
   if (!db)
      return katze_array_new (KATZE_TYPE_ITEM);

   if (filter && *filter)
   {
      char* filterstr;
     sqlcmd = "SELECT * FROM ("
		"    SELECT uri, title, day, date FROM history"
		"    WHERE uri LIKE ?1 OR title LIKE ?1 GROUP BY uri "
		"UNION ALL "
		"    SELECT replace (uri, '%s', keywords) AS uri, "
		"    keywords AS title, day, 0 AS date FROM search "
		"    WHERE uri LIKE ?1 OR keywords LIKE ?1 GROUP BY uri "
		") ORDER BY day DESC";

      result = sqlite3_prepare_v2 (db, sqlcmd, -1, &statement, NULL);
      filterstr = g_strdup_printf ("%%%s%%", filter);
      sqlite3_bind_text (statement, 1, filterstr, -1, g_free);
   }

   if (result != SQLITE_OK)
      return katze_array_new (KATZE_TYPE_ITEM);
   return katze_array_from_statement (statement);
}

static void 
_midori_view_show_speeddial_history(MidoriView*  view,  
                                    const gchar* message) 
{
   KatzeArray* array;
   struct json_object *my_object;
   gchar* data = NULL;
   size_t datalen;
   int string_len,length=0,n=0,page_nth=0,page_n=0,item_nth=0;
   gchar *string_data,*string_data1;
   gchar file_adr[100] = {0};
   gchar image_adr[100] = {0};
   KatzeItem* item;
	
   gchar**part =  g_strsplit (message, "-", 4);
   page_n = atoi(part[2]);
   page_nth = atoi(part[3]);	
   item_nth = page_nth * page_n;
   MidoriBrowser* browser = midori_browser_get_for_widget (GTK_WIDGET (midori_view_get_web_view(view)));
   WebKitFaviconDatabase* dab = webkit_web_context_get_favicon_database(webkit_web_context_get_default ());
	
   array = midori_view_history_read_from_db (browser,"http");
   g_sprintf(file_adr,"%s/temp_data.txt",midori_paths_get_cache_dir());
   length = katze_array_get_length(array);
   if((page_nth-1)*page_n >= length)return;

   FILE * fp = fopen(file_adr,"w+");	
   if(fp == NULL)
     return;
   fwrite("[",1,1,fp);
   KATZE_ARRAY_FOREACH_ITEM (item, array)
   {
      n++;
      if(n > item_nth  )break;
      if(n<=(page_nth-1)*page_n)continue;
      my_object = json_object_new_object();  
      GdkPixbuf*  pixbuf = webkit_favicon_database_try_get_favicon_pixbuf(dab,katze_item_get_uri(item),16,16);
      if(pixbuf != NULL)
      {
         g_sprintf(image_adr,"%s/temp.png",midori_paths_get_cache_dir());
         gdk_pixbuf_save (pixbuf, image_adr, "png", NULL, NULL, "compression", "7", NULL, NULL);
	 g_file_get_contents(image_adr,&data,&datalen,NULL);
	 json_object_object_add(my_object, "image", json_object_new_string(g_base64_encode ((guchar*)data, datalen)));
	 g_free(data);
      }
      else 
         json_object_object_add(my_object, "image",json_object_new_string(""));
      json_object_object_add(my_object, "name", json_object_new_string(katze_item_get_name(item)));
      json_object_object_add(my_object, "uri", json_object_new_string(katze_item_get_uri(item)));
      string_data = json_object_to_json_string(my_object);
      string_data1 = string_replace(string_data,"\\/","/");
      string_len = strlen(string_data1);
      fwrite(string_data1,string_len,1,fp);
      fwrite(",",1,1,fp);
      g_free(string_data1);
      json_object_put(my_object);
      g_object_unref (G_OBJECT(item));
   }
   g_object_unref (G_OBJECT (array));
   fseek(fp,-1,2);
   fwrite("]",1,1,fp);
   int total_size = ftell(fp);
   fseek(fp,0,0);
   char * file_data =(char *) malloc(total_size+1);
   memset(file_data,0,total_size+1);
   fread(file_data,total_size,1,fp);
   char * data_adr = (char *) malloc(total_size+100);
   memset(data_adr,0,total_size+100);
   g_sprintf(data_adr,"getHistoryList('%s','%d');",file_data,page_nth);
   free(file_data);
   fclose(fp); 
   webkit_web_view_run_javascript(WEBKIT_WEB_VIEW (midori_view_get_web_view(view)),data_adr, NULL, NULL, NULL);
   free(data_adr);
}
	
//added by wangyl 2015.8.5 end
// ZRL Implement to receive signal console-message for console.log
static gboolean
webkit_web_view_console_message_cb (GtkWidget*   web_view,
                                    const gchar* message,
                                    guint        line,
                                    const gchar* source_id,
                                    MidoriView*  view)
{
#ifdef APP_LEVEL_TIME
printf("signal(console-message) callback start time = %lld\n",g_get_real_time());
#endif
    if (!strncmp (message, "speed_dial-save", 13))
    {  
        MidoriBrowser* browser = midori_browser_get_for_widget (GTK_WIDGET (view));
        MidoriSpeedDial* dial = katze_object_get_object (browser, "speed-dial");
        GError* error = NULL;
        midori_speed_dial_save_message (dial, message, &error);
        if (error != NULL)
        {
            g_critical ("Failed speed dial message: %s\n", error->message);
            g_error_free (error);
        }
    }
      else if( !strncmp (message, "speeddial-history-", 18))
    {
	_midori_view_show_speeddial_history(view,message);
    }
    else if( !strncmp (message, "speeddial-update-", 17))
    {
	MidoriBrowser* browser = midori_browser_get_for_widget (GTK_WIDGET (midori_view_get_web_view(view)));
	MidoriSpeedDial* dial = katze_object_get_object (browser, "speed-dial");
	gchar**part =  g_strsplit (message, "-", 4);
	GError* error = NULL;
	gchar id[25]={0};
	int i = 0;
	strncpy(id,part[2],strlen(part[2]));
	if(strlen(id) != 13)return FALSE;
	for( i = 0;i<13;i++)if(id[i]<'0'||id[i]>'9')return FALSE;
	midori_speed_dial_add(dial, part[3], id, NULL);
	if (error != NULL)
	{
	  g_critical ("Failed speed dial message: %s\n", error->message);
	  g_error_free (error);
	}
			
    }
    else if( !strncmp (message, "speeddial-remove-", 17))
    {
	gchar id[50]={0};
	MidoriBrowser* browser = midori_browser_get_for_widget (GTK_WIDGET (midori_view_get_web_view(view)));
	MidoriSpeedDial* dial = katze_object_get_object (browser, "speed-dial");
	gchar**part =  g_strsplit (message, "-", 4);
	GError* error = NULL;
	g_sprintf(id,"speed_dial-save-delete %s",part[2]);
	midori_speed_dial_save_message (dial, id, &error);
	if (error != NULL)
	{
		g_critical ("Failed speed dial message: %s\n", error->message);
		g_error_free (error);
	}
			
   }
   else if( !strncmp (message, "speeddial-add-", 14) ) 
   {
	MidoriBrowser* browser = midori_browser_get_for_widget (GTK_WIDGET (midori_view_get_web_view(view)));
        MidoriView*   view1 = MIDORI_VIEW (midori_browser_get_current_tab (browser));
        gchar view_adr[30]={0};
        g_sprintf(view_adr,"%d",view1);
	MidoriSpeedDial* dial = katze_object_get_object (browser, "speed-dial");
	gchar**part =  g_strsplit (message, "-", 4);
	gchar id[25]={0};
	int i = 0;
	strncpy(id,part[2],strlen(part[2]));
	if(strlen(id) != 13)return FALSE;
	for( i = 0;i<13;i++)if(id[i]<'0'||id[i]>'9')return FALSE; 
  	midori_speed_dial_add(dial, part[3], id, view_adr);
   }
    else if(!strncmp (message, "cdosExtension", 13))
    {
       
       g_signal_emit(view, signals[CDOSEXTENSION_MESSAGE], 0, message);   
    }
    else {
        g_signal_emit_by_name (view, "console-message", message, line, source_id);
    }
#ifdef APP_LEVEL_TIME
printf("signal(console-message) callback end time = %lld\n",g_get_real_time());
#endif
    return TRUE;
}

typedef struct {
  MidoriView *web_view;
  guint request_id;
} FormAuthRequestData;

static GtkWidget *
midori_web_view_create_form_auth_save_confirmation_info_bar (MidoriView *web_view,
                                                             const char *hostname,
                                                             const char *username)
{
  GtkWidget *info_bar;
  GtkWidget *action_area;
  GtkWidget *content_area;
  GtkWidget *label;
  char *message;


// ZRL 区分gtk版本
#if GTK_CHECK_VERSION (3, 10, 0)
  info_bar = gtk_info_bar_new_with_buttons (_("保  存"), GTK_RESPONSE_YES, 
                                            NULL);
  gtk_info_bar_set_show_close_button (GTK_INFO_BAR (info_bar), TRUE);
#else
  info_bar = gtk_info_bar_new_with_buttons (_("保  存"), GTK_RESPONSE_YES, _("关  闭"), GTK_RESPONSE_CLOSE,
                                            NULL);
#endif

  action_area = gtk_info_bar_get_action_area (GTK_INFO_BAR (info_bar));
  gtk_orientable_set_orientation (GTK_ORIENTABLE (action_area),
                                  GTK_ORIENTATION_HORIZONTAL);

  label = gtk_label_new (NULL);
  /* Translators: The %s the hostname where this is happening. 
   * Example: mail.google.com.
   */
  message = g_markup_printf_escaped (_("是否要保存该网站用户名和密码： “%s”?"),
                                     hostname);
  gtk_label_set_markup (GTK_LABEL (label), message);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  g_free (message);

  content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar));
  gtk_container_add (GTK_CONTAINER (content_area), label);
  gtk_widget_show (label);

  gtk_box_pack_end (GTK_BOX (web_view),
                      GTK_WIDGET (info_bar),
                      FALSE, FALSE, 0);

  /* We track the info_bar, so we only ever show one */
  if (web_view->password_info_bar)
    gtk_widget_destroy (web_view->password_info_bar);

  web_view->password_info_bar = info_bar;
  g_object_add_weak_pointer (G_OBJECT (info_bar),
                             (gpointer *)&web_view->password_info_bar);

  return info_bar;
}

static void
form_auth_data_save_confirmation_response (GtkInfoBar *info_bar,
                                           gint response_id,
                                           FormAuthRequestData *data)
{
  gtk_widget_destroy (GTK_WIDGET (info_bar));
  if (data->web_view->web_extension) {
    ephy_web_extension_proxy_form_auth_data_save_confirmation_response (data->web_view->web_extension,
                                                                        data->request_id,
                                                                        response_id == GTK_RESPONSE_YES);
  }

  g_slice_free (FormAuthRequestData, data);
}

static void
form_auth_data_save_requested (EphyWebExtensionProxy *web_extension,
                               guint                  request_id,
                               guint64                page_id,
                               const char             *hostname,
                               const char             *username,
                               MidoriView             *web_view)
{
  GtkWidget *info_bar;
  FormAuthRequestData *data;

  if (webkit_web_view_get_page_id (WEBKIT_WEB_VIEW(midori_view_get_web_view(web_view))) != page_id)
    return;

  info_bar = midori_web_view_create_form_auth_save_confirmation_info_bar (web_view, hostname, username);
  data = g_slice_new (FormAuthRequestData);
  data->web_view = web_view;
  data->request_id = request_id;
  g_signal_connect (info_bar, "response",
                    G_CALLBACK (form_auth_data_save_confirmation_response),
                    data);

  gtk_widget_show (info_bar);
}

static void
page_created_cb (MidoriApp* app,
                 guint64 page_id,
                 EphyWebExtensionProxy *web_extension,
                 MidoriView *web_view)
{
   if (webkit_web_view_get_page_id (WEBKIT_WEB_VIEW(midori_view_get_web_view(web_view))) != page_id)
      return;

  web_view->web_extension = web_extension;
  g_object_add_weak_pointer (G_OBJECT (web_view->web_extension), (gpointer *)&web_view->web_extension);
  g_signal_connect_object (web_view->web_extension, "form-auth-data-save-requested",
                           G_CALLBACK (form_auth_data_save_requested),
                           web_view, 0);
}

//zgh 20150108

gchar** midori_view_get_website_record (MidoriView*        view)
{
    return view->website_record_array;
}
//eng zgh

static void
midori_view_init (MidoriView* view)
{
    view->title = NULL;
    view->icon = NULL;
    view->icon_uri = NULL;
    view->hit_test = NULL;
    view->link_uri = NULL;
    view->load_uri = NULL;//luyue 2015/3/9
    view->anquanjibie = NULL;//luyue 2015/3/10
    view->back_uri = NULL;//luyue 2015/3/10
    view->forward_uri = NULL;//luyue 2015/3/10
    view->selected_text = NULL;
    view->news_feeds = NULL;
    view->find_links = -1;
    view->alerts = 0;
    view->load_commited = FALSE;

    view->item = katze_item_new ();

    view->scrollh = view->scrollv = -2;
    view->tmp_uri = NULL; //luyue 2015/3/6 
    view->website_record_array = NULL;  //zgh 20150108

    view->media_info_bar_lock = FALSE;  //ykhu
    view->save_menu_flag = false;

    g_signal_connect (view->item, "meta-data-changed",
        G_CALLBACK (midori_view_item_meta_data_changed), view);

    MidoriApp *app = midori_app_get_default ();
    g_signal_connect (app, "page-created",
                           G_CALLBACK (page_created_cb),
                           view);
}

static void
midori_view_finalize (GObject* object)
{
    MidoriView* view = MIDORI_VIEW (object);

    if (view->settings)
        g_signal_handlers_disconnect_by_func (view->settings,
            midori_view_settings_notify_cb, view);
    g_signal_handlers_disconnect_by_func (view->item,
        midori_view_item_meta_data_changed, view);

    katze_assign (view->title, NULL);
    katze_object_assign (view->icon, NULL);
    katze_assign (view->icon_uri, NULL);

    katze_assign (view->link_uri, NULL);
    katze_assign (view->tmp_uri,NULL);  //add by luyue 2015/3/9
    katze_assign (view->load_uri,NULL); //add by luyue 2015/3/10
    katze_assign (view->anquanjibie,NULL);//add by luyue 2015/3/10
    katze_assign (view->back_uri,NULL);//add by luyue 2015/3/10
    katze_assign (view->forward_uri,NULL);//add by luyue 2015/3/10
    katze_assign (view->selected_text, NULL);
    katze_object_assign (view->news_feeds, NULL);

    katze_object_assign (view->settings, NULL);
    katze_object_assign (view->item, NULL);
    
    katze_assign (view->website_record_array, NULL);

    G_OBJECT_CLASS (midori_view_parent_class)->finalize (object);
}

static void
midori_view_set_property (GObject*      object,
                          guint         prop_id,
                          const GValue* value,
                          GParamSpec*   pspec)
{
    MidoriView* view = MIDORI_VIEW (object);

    switch (prop_id)
    {
    case PROP_TITLE:
        midori_view_set_title (view, g_value_get_string (value));
        break;
    case PROP_MINIMIZED:
        view->minimized = g_value_get_boolean (value);
        g_signal_handlers_block_by_func (view->item,
            midori_view_item_meta_data_changed, view);
        katze_item_set_meta_integer (view->item, "minimized",
                                     view->minimized ? 1 : -1);
        g_signal_handlers_unblock_by_func (view->item,
            midori_view_item_meta_data_changed, view);
        break;
    case PROP_ZOOM_LEVEL:
        midori_view_set_zoom_level (view, g_value_get_float (value));
        break;
    case PROP_SETTINGS:
        _midori_view_set_settings (view, g_value_get_object (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
midori_view_get_property (GObject*    object,
                          guint       prop_id,
                          GValue*     value,
                          GParamSpec* pspec)
{
    MidoriView* view = MIDORI_VIEW (object);

    switch (prop_id)
    {
    case PROP_TITLE:
        g_value_set_string (value, view->title);
        break;
    case PROP_ICON:
        g_value_set_object (value, view->icon);
        break;
    case PROP_ZOOM_LEVEL:
        g_value_set_float (value, midori_view_get_zoom_level (view));
        break;
    case PROP_NEWS_FEEDS:
        g_value_set_object (value, view->news_feeds);
        break;
    case PROP_SETTINGS:
        g_value_set_object (value, view->settings);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static gboolean
midori_view_focus_in_event (GtkWidget*     widget,
                            GdkEventFocus* event)
{
    /* Always propagate focus to the child web view */
    gtk_widget_grab_focus (midori_view_get_web_view (MIDORI_VIEW (widget)));
    return TRUE;
}

static void
_midori_view_set_settings (MidoriView*        view,
                           MidoriWebSettings* settings)
{
    gboolean zoom_text_and_images;
    gdouble zoom_level;

    if (view->settings)
        g_signal_handlers_disconnect_by_func (view->settings,
            midori_view_settings_notify_cb, view);

    katze_object_assign (view->settings, settings);
    if (!settings)
        return;

    g_object_ref (settings);
    g_signal_connect (settings, "notify",
                      G_CALLBACK (midori_view_settings_notify_cb), view);

    g_object_get (view->settings,
        "zoom-level", &zoom_level,
        "zoom-text-and-images", &zoom_text_and_images,
        "open-new-pages-in", &view->open_new_pages_in,
        "open-tabs-in-the-background", &view->open_tabs_in_the_background,
        NULL);

    webkit_web_view_set_settings (WEBKIT_WEB_VIEW (view->web_view), (void*)settings);
    midori_view_set_zoom_level (view, zoom_level);

   //add by luyue 2015/1/20
   midori_view_set_doublezoom_state(view,settings);
   midori_view_set_zoomtext_state(view,settings);
   midori_view_set_doublezoom_level(view,settings);
   //add by luyue 2015/6/30
   midori_view_set_secure_level(view,settings);
}

//add by luyue 2015/1/20
void
midori_view_set_doublezoom_state (MidoriView*        view,
                                  MidoriWebSettings* settings)
{
   bool value = false;
   g_object_get(settings, "smart-zoom", &value, NULL);
   if(value)
      webkit_web_view_set_doublezoom_state(WEBKIT_WEB_VIEW (view->web_view), value);
   else
      webkit_web_view_set_doublezoom_state(WEBKIT_WEB_VIEW (view->web_view), false);
}

//add by luyue 2015/1/20
void
midori_view_set_zoomtext_state (MidoriView*        view,
                                MidoriWebSettings* settings)
{
   bool value = false;
   g_object_get(settings, "zoom-text-and-images", &value, NULL);
   if(!value)
      webkit_web_view_set_zoomtext_state(WEBKIT_WEB_VIEW (view->web_view), !value);
   else
      webkit_web_view_set_zoomtext_state(WEBKIT_WEB_VIEW (view->web_view), false);
}

//add by luyue 2015/1/21
void
midori_view_set_doublezoom_level (MidoriView*        view,
                                  MidoriWebSettings* settings)
{
   double level = 0.0;
   g_object_get(settings, "smart-zoom-level", &level, NULL);
   if(level)
      webkit_web_view_set_doublezoom_level(WEBKIT_WEB_VIEW (view->web_view), level);
}

//add by luyue 2015/6/30 
void
midori_view_set_secure_level (MidoriView*        view,
                              MidoriWebSettings* settings)
{
   gint value = 0;
   g_object_get(settings, "secure-level", &value, NULL);
   switch(value) {
      case 0://低,全部关闭
         webkit_web_view_set_shellcode_state(WEBKIT_WEB_VIEW (view->web_view), false);//利用漏洞注入检测
         webkit_web_view_set_obfuscatecode_state(WEBKIT_WEB_VIEW (view->web_view), false);//混淆代码检测
         webkit_web_view_set_autodownload_state(WEBKIT_WEB_VIEW (view->web_view), false);//恶意自动下载检测
         view->phish_check_flag = true;//恶意弹窗拦截
         view->popupwindow_check_flag = true;//钓鱼网站检测

         view->danager_uri_flag = true;//高危网址检测
         if(view->back_uri)
         {
            free(view->back_uri);
            view->back_uri = NULL;
         }
         if(view->forward_uri)
         {
            free(view->forward_uri);
            view->forward_uri = NULL;
         }
         break;
      case 1://中，部分开启
         webkit_web_view_set_shellcode_state(WEBKIT_WEB_VIEW (view->web_view), false);//关闭
         webkit_web_view_set_obfuscatecode_state(WEBKIT_WEB_VIEW (view->web_view), true);//开启
         webkit_web_view_set_autodownload_state(WEBKIT_WEB_VIEW (view->web_view), false);//关闭
         view->phish_check_flag = false;//开启
         view->popupwindow_check_flag = false;//开启

         view->danager_uri_flag = true;//关闭
         if(view->back_uri)
         {
            free(view->back_uri);
            view->back_uri = NULL;
         }
         if(view->forward_uri)
         {
            free(view->forward_uri);
            view->forward_uri = NULL;
         }
         break;
      case 2://高，全部开启
         webkit_web_view_set_shellcode_state(WEBKIT_WEB_VIEW (view->web_view), true);
         webkit_web_view_set_obfuscatecode_state(WEBKIT_WEB_VIEW (view->web_view), true);
         webkit_web_view_set_autodownload_state(WEBKIT_WEB_VIEW (view->web_view), true);
         view->phish_check_flag = false;
         view->popupwindow_check_flag = false;
         view->danager_uri_flag = false;
         break;
      default:
         break;
   }
}
//add end

/**
 * midori_view_new_with_title:
 * @title: a title, or %NULL
 * @settings: a #MidoriWebSettings, or %NULL
 * @append: if %TRUE, the view should be appended
 *
 * Creates a new view with the specified parameters that
 * is visible by default.
 *
 * Return value: a new #MidoriView
 *
 * Since: 0.3.0
 * Deprecated: 0.4.3
 **/
GtkWidget*
midori_view_new_with_title (const gchar*       title,
                            MidoriWebSettings* settings,
                            gboolean           append)
{
    KatzeItem* item = katze_item_new ();
    item->name = g_strdup (title);
    if (append)
        katze_item_set_meta_integer (item, "append", 1);
    return midori_view_new_with_item (item, settings);
}

/**
 * midori_view_new_with_item:
 * @item: a #KatzeItem, or %NULL
 * @settings: a #MidoriWebSettings, or %NULL
 *
 * Creates a new view from an item that is visible by default.
 *
 * Return value: a new #MidoriView
 *
 * Since: 0.4.3
 * Deprecated: 0.5.8: Use midori_view_new_from_view instead.
 **/
GtkWidget*
midori_view_new_with_item (KatzeItem*         item,
                           MidoriWebSettings* settings)
{
    return midori_view_new_from_view (NULL, item, settings);
}

/**
 * midori_view_new_from_view:
 * @view: a predating, related #MidoriView, or %NULL
 * @item: a #KatzeItem, or %NULL
 * @settings: a #MidoriWebSettings, or %NULL
 *
 * Creates a new view, visible by default.
 *
 * If a @view is specified the returned new view will share
 * its settings and if applicable re-use the rendering process.
 *
 * When @view should be passed:
 *     The new one created is a new tab/ window for the old @view
 *     A tab was duplicated
 *
 * When @view may be passed:
 *     Old and new view belong to the same website or group
 *
 * Don't pass a @view if:
 *     The new view is a completely new website
 *
 * The @item may contain title, URI and minimized status and will be copied.
 *
 * Usually @settings should be passed from an existing view or browser.
 *
 * Return value: a new #MidoriView
 *
 * Since: 0.5.8
 **/
GtkWidget*
midori_view_new_from_view (MidoriView*        related,
                           KatzeItem*         item,
                           MidoriWebSettings* settings)
{
    MidoriView* view = g_object_new (MIDORI_TYPE_VIEW,
                                     "related", MIDORI_TAB (related),
                                     "title", item ? katze_item_get_name (item) : NULL,
                                     NULL);
    if (!settings && related)
        settings = related->settings;
    if (settings)
        _midori_view_set_settings (view, settings);
    if (item)
    {
        katze_object_assign (view->item, katze_item_copy (item));
        midori_tab_set_minimized (MIDORI_TAB (view),
            katze_item_get_meta_string (view->item, "minimized") != NULL);
    }
    gtk_widget_show ((GtkWidget*)view);
    return (GtkWidget*)view;
}

static void
midori_view_settings_notify_cb (MidoriWebSettings* settings,
                                GParamSpec*        pspec,
                                MidoriView*        view)
{
    const gchar* name;
    GValue value = { 0, };

    name = g_intern_string (g_param_spec_get_name (pspec));
    g_value_init (&value, pspec->value_type);
    g_object_get_property (G_OBJECT (view->settings), name, &value);

    if (name == g_intern_string ("open-new-pages-in"))
        view->open_new_pages_in = g_value_get_enum (&value);
    else if (name == g_intern_string ("open-tabs-in-the-background"))
        view->open_tabs_in_the_background = g_value_get_boolean (&value);
    else if (name == g_intern_string ("enable-javascript"))
    {
        /* Speed dial is only editable with scripts, so regenerate it */
        if (midori_view_is_blank (view))
            midori_view_reload (view, FALSE);
    }

    g_value_unset (&value);
}

/**
 * midori_view_set_settings:
 * @view: a #MidoriView
 * @settings: a #MidoriWebSettings
 *
 * Assigns a settings instance to the view.
 **/
void
midori_view_set_settings (MidoriView*        view,
                          MidoriWebSettings* settings)
{
    g_return_if_fail (MIDORI_IS_VIEW (view));
    g_return_if_fail (MIDORI_IS_WEB_SETTINGS (settings));

    if (view->settings == settings)
        return;

    _midori_view_set_settings (view, settings);
    g_object_notify (G_OBJECT (view), "settings");
}

/**
 * midori_view_load_status:
 * @web_view: a #MidoriView
 *
 * Determines the current loading status of a view. There is no
 * error state, unlike webkit_web_view_get_load_status().
 *
 * Return value: the current #MidoriLoadStatus
 **/
MidoriLoadStatus
midori_view_get_load_status (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), MIDORI_LOAD_FINISHED);

    return midori_tab_get_load_status (MIDORI_TAB (view));
}

/**
 * midori_view_get_progress:
 * @view: a #MidoriView
 *
 * Retrieves the current loading progress as
 * a fraction between 0.0 and 1.0.
 *
 * Return value: the current loading progress
 **/
gdouble
midori_view_get_progress (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), 0.0);

    return midori_tab_get_progress (MIDORI_TAB (view));
}

static GObject*
midori_view_constructor (GType                  type,
                         guint                  n_construct_properties,
                         GObjectConstructParam* construct_properties)
{
    GObject* object = G_OBJECT_CLASS (midori_view_parent_class)->constructor (
        type, n_construct_properties, construct_properties);
    MidoriView* view = MIDORI_VIEW (object);

    view->web_view = GTK_WIDGET (midori_tab_get_web_view (MIDORI_TAB (view)));
    g_object_connect (view->web_view,
                      "signal::load-failed",
                      webkit_web_view_load_error_cb, view,
                      "signal::load-changed",
                      midori_view_web_view_load_changed_cb, view,
                      "signal::notify::estimated-load-progress",
                      webkit_web_view_progress_changed_cb, view,
                      "signal::notify::favicon",
                      midori_web_view_notify_icon_uri_cb, view,
                      "signal::mouse-target-changed",
                      webkit_web_view_hovering_over_link_cb, view,
                      "signal::decide-policy",
                      midori_view_web_view_navigation_decision_cb, view,
                      "signal::context-menu",
                      midori_view_web_view_context_menu_cb, view,
                      "signal::create",
                      webkit_web_view_create_web_view_cb, view,
                      "signal::console-message",
                      webkit_web_view_console_message_cb, view,
                      #if TRACK_LOCATION_TAB_ICON //lxx, 20150203
                      //lxx, 20150127	
                      "signal::permission-request",
		      webkit_web_view_permission_request_cb, view,
                      "signal::javascript-popup-window-block-message",
		      webkit_web_view_javascript_popup_window_block_cb, view,
                      #endif
                      //ykhu
                      "signal::show-media-failed-text",
                      webkit_web_view_media_failed_text_cb, view,
                      "signal::notify::title",
                      webkit_web_view_notify_title_cb, view,
                      "signal::leave-notify-event",
                      midori_view_web_view_leave_notify_event_cb, view,
                      "signal::button-press-event",
                      midori_view_web_view_button_press_event_cb, view,
                      "signal::button-release-event",
                      midori_view_web_view_button_release_event_cb, view,
                      "signal-after::key-press-event",
                      gtk_widget_key_press_event_cb, view,
                      "signal::scroll-event",
                      gtk_widget_scroll_event_cb, view,
                      NULL);

    if (view->settings)
    {
        webkit_web_view_set_settings (WEBKIT_WEB_VIEW (view->web_view), (void*)view->settings);
    }

    if (g_signal_lookup ("web-process-crashed", WEBKIT_TYPE_WEB_VIEW))
        g_signal_connect (view->web_view, "web-process-crashed",
            (GCallback)midori_view_web_view_crashed_cb, view);
    view->scrolled_window = view->web_view;
    WebKitWebContext* context = webkit_web_view_get_context (WEBKIT_WEB_VIEW (view->web_view));
    webkit_web_context_register_uri_scheme (context,
        "res", midori_view_uri_scheme_res, NULL, NULL);
    webkit_web_context_register_uri_scheme (context,
        "stock", midori_view_uri_scheme_res, NULL, NULL);
    g_signal_connect (context, "download-started",
        G_CALLBACK (midori_view_download_started_cb), view);

    #if GTK_CHECK_VERSION(3, 2, 0)
    view->overlay = gtk_overlay_new ();
    gtk_widget_show (view->overlay);
    gtk_container_add (GTK_CONTAINER (view->overlay), view->scrolled_window);
    gtk_box_pack_start (GTK_BOX (view), view->overlay, TRUE, TRUE, 0);

    /* Overlays must be created before showing GtkOverlay as of GTK+ 3.2 */
    {
    GtkWidget* frame = gtk_frame_new (NULL);
    gtk_widget_set_no_show_all (frame, TRUE);
    view->overlay_label = gtk_label_new (NULL);
    gtk_widget_show (view->overlay_label);
    gtk_container_add (GTK_CONTAINER (frame), view->overlay_label);
    gtk_widget_set_halign (frame, GTK_ALIGN_START);
    gtk_widget_set_valign (frame, GTK_ALIGN_END);
    gtk_overlay_add_overlay (GTK_OVERLAY (view->overlay), frame);
    /* Enable enter-notify-event signals */
    gtk_widget_add_events (view->overlay, GDK_ENTER_NOTIFY_MASK);
    g_signal_connect (view->overlay, "enter-notify-event",
        G_CALLBACK (midori_view_overlay_frame_enter_notify_event_cb), frame);
    }
    view->overlay_find = g_object_new (MIDORI_TYPE_FINDBAR, NULL);
    gtk_widget_set_halign (view->overlay_find, GTK_ALIGN_END);
    gtk_widget_set_valign (view->overlay_find, GTK_ALIGN_START);
    gtk_overlay_add_overlay (GTK_OVERLAY (view->overlay),
                             view->overlay_find);
    gtk_widget_set_no_show_all (view->overlay_find, TRUE);
    #else
    gtk_box_pack_start (GTK_BOX (view), view->scrolled_window, TRUE, TRUE, 0);
    #endif

    gtk_widget_show_all (view->scrolled_window);
    return object;
}

static void
midori_view_add_version (GString* markup,
                         gboolean html,
                         gchar*   text)
{
    if (html)
        g_string_append (markup, "<tr><td>");
    g_string_append (markup, text);
    if (html)
        g_string_append (markup, "</td></tr>");
    else
        g_string_append_c (markup, '\n');
    g_free (text);
}

/* ZRL 定制版本信息显示*/
void
midori_view_list_versions (GString* markup,
                           gboolean html)
{
    midori_view_add_version (markup, html, g_strdup_printf ("%s %s",
        g_get_application_name (), PACKAGE_VERSION));
    midori_view_add_version (markup, html, g_strdup_printf ("GTK+ %s (%u.%u.%u)\tGlib %s (%u.%u.%u)",
        GTK_VERSION, gtk_major_version, gtk_minor_version, gtk_micro_version,
        GIO_VERSION, glib_major_version, glib_minor_version, glib_micro_version));
    midori_view_add_version (markup, html, g_strdup_printf ("WebKit2GTK+ %s (%u.%u.%u)\tlibSoup %s",
        WEBKIT_VERSION, webkit_get_major_version (), webkit_get_minor_version (), webkit_get_micro_version (),
        LIBSOUP_VERSION));
    midori_view_add_version (markup, html, g_strdup_printf ("cairo %s (%s)\tlibnotify %s",
        CAIRO_VERSION_STRING, cairo_version_string (),
        LIBNOTIFY_VERSION));
    midori_view_add_version (markup, html, g_strdup_printf ("gcr %s\tgranite %s",
        GCR_VERSION, GRANITE_VERSION));
}

static void
midori_view_get_plugins_cb (GObject*      object,
                            GAsyncResult* result,
                            MidoriView*   view)
{
    GList* plugins = webkit_web_context_get_plugins_finish (WEBKIT_WEB_CONTEXT (object), result, NULL);
    g_object_set_data (object, "nsplugins", plugins);
    midori_view_reload (view, FALSE);
}

void
midori_view_list_plugins (MidoriView* view,
                          GString*    ns_plugins,
                          gboolean    html)
{
    if (!midori_web_settings_has_plugin_support ())
        return;

    if (html)
        g_string_append (ns_plugins, "<br><h2>Netscape Plugins:</h2>");
    else
        g_string_append_c (ns_plugins, '\n');

    WebKitWebContext* context = webkit_web_context_get_default ();
    GList* plugins = g_object_get_data (G_OBJECT (context), "nsplugins");
    if (plugins == NULL)
    {
        midori_view_add_version (ns_plugins, html, g_strdup ("…"));
        webkit_web_context_get_plugins (context, NULL, (GAsyncReadyCallback)midori_view_get_plugins_cb, view);
    }
    else
        for (; plugins != NULL; plugins = g_list_next (plugins))
        {
            if (!midori_web_settings_skip_plugin (webkit_plugin_get_path (plugins->data)))
                midori_view_add_version (ns_plugins, html, g_strdup_printf ("%s\t%s",
                    webkit_plugin_get_name (plugins->data),
                    html ? webkit_plugin_get_description (plugins->data) : ""));
        }
}

//add by luyue 2015/3/19
static void website_check_insert_js_cb(WebKitWebView*  web_view)
{
       sleep(1);
       gchar* jquerySrc = NULL;
       GError * _inner_error_ = NULL;
       g_file_get_contents (midori_paths_get_res_filename("website_check/website_check.js"),
                            &jquerySrc,
                            NULL,
                            &_inner_error_);
       webkit_web_view_run_javascript(web_view, jquerySrc, NULL, NULL, NULL);
       g_free(jquerySrc);
       pthread_exit(0);
}

//add by luyue 2015/3/9
static void
midori_view_web_view1_load_changed_cb (WebKitWebView*  web_view,
                                       WebKitLoadEvent load_event,
                                       MidoriView*     view)
{
    if (load_event == WEBKIT_LOAD_FINISHED)
    {
       pthread_t ntid;
       int ret;
       ret = pthread_create(&ntid, NULL, website_check_insert_js_cb, web_view);
    }     
}
//add end

//add by luyue 2015/3/9
//后台恶意检测时，spinner转动
static void
webkit_web_view1_progress_changed_cb (WebKitWebView* web_view,
                                     GParamSpec*    pspec,
                                     MidoriView*    view)
{
   gdouble progress = 1.0;
   g_object_get (web_view, pspec->name, &progress, NULL);
   if(progress==1.0) progress=0.5;
   midori_tab_set_progress (MIDORI_TAB (view), progress);
}
//add end

//add by luyue 2015/3/9
static gboolean
midori_view_display_message (MidoriView*     view,
                             const gchar*    uri,
                             const gchar*    title,
                             const gchar*    description,
                             const gchar*    message,
                             const gchar*    forward,
                             const gchar*    back)
{
    gchar* path = midori_paths_get_res_filename ("website_check/dangerous_url.html");
    gchar* template;

    if (g_file_get_contents (path, &template, NULL, NULL))
    {
        gchar* title_escaped;
        gchar* result;
        title_escaped = g_markup_escape_text (title ? title : view->title, -1);
        result = sokoke_replace_variables (template,
            "{dir}", gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL ?
                "rtl" : "ltr",
            "{title}", title_escaped,
            "{message}", message,
            "{description}", description,
            "{forward}", forward,
            "{back}", back,
            "{uri}", uri,
            NULL);
        g_free (title_escaped);
        g_free (template);
        midori_view_set_html (view, result, uri, NULL);
        g_free (result);
        g_free (path);
        return TRUE;
    }
    g_free (path);
    return FALSE;
}
//add end

//add by luyue 2015/3/9
static void
midori_view_web_view1_dangerous_cb (MidoriView*   view,
                                   MidoriView*    old_view)
{
    gchar* title = g_strdup_printf ("危险网站 - %s", view->load_uri);
    const gchar *description = "该网站为非安全网站！";
    const gchar* message = "您不应再继续，尤其是如果您以前从未在此网站看到这一警告信息，则更不应继续操作。";
    midori_view_display_message (view, view->load_uri,title,description,message, "仍然继续","返回" );
    g_free (title);
}
//add end

//add by luyue 2015/3/9
static gboolean
webkit_web_view1_console_message_cb (GtkWidget*   web_view,
                                    const gchar* message,
                                    guint        line,
                                    const gchar* source_id,
                                    MidoriView*  view)
{
    if (!strncmp (message, "website_check_info", 18))
    {
       gchar** wqi_array = NULL;
       wqi_array = g_strsplit (message, "#", -1);
       if(!strlen(wqi_array[1]))
       {
           g_signal_emit (view, signals[WEBSITE_CHECK], 0);
       }
       else if(strcmp(wqi_array[1],"高危"))
       {
           WebKitURIRequest *request = webkit_uri_request_new(view->load_uri);
           const gchar *base_domain = webkit_uri_request_get_uri_host (request);
           gchar* home = getenv("HOME");
           gchar user_dir[2048];
           g_sprintf(user_dir, "%s/.config/cdosbrowser", home);
           int error = 0;
           if(!g_file_test(user_dir, G_FILE_TEST_IS_DIR) || !g_access(user_dir, /*S_IWUSR|S_IRUSR*/0755))
              error = g_mkdir_with_parents(user_dir, /*S_IWUSR|S_IRUSR*/ 0755);
           if(!error)
           {
              gchar user_path[2048];
              g_sprintf(user_path, "%s/secure_website.txt", user_dir);
              FILE *fp = fopen(user_path,"a");
              if(!fp)
              {
                 g_signal_emit (view, signals[WEBSITE_CHECK], 0);
                 return TRUE;
              }
              fputs(base_domain,fp);
              fputs("\n",fp);
              fclose(fp);
           }           
           g_signal_emit (view, signals[WEBSITE_CHECK], 0);
       }
       else
       {
           if(view->anquanjibie)
           {
              free(view->anquanjibie);
              view->anquanjibie = NULL;
           }
           view->anquanjibie = (char *)malloc(strlen(wqi_array[1])+1);
           strcpy(view->anquanjibie,wqi_array[1]);
           g_signal_emit (view, signals[DANGEROUS_URL], 0);
           g_signal_emit (view, signals[FORWARD_URL], 0,view->forward_uri);
       }
    }
   return TRUE;
}
//add end

static void
midori_view_reset_user_agent(MidoriView*   view)
{
    gchar* platform;
    gchar* architecture;
    char *string;
    const gchar* os = midori_web_settings_get_system_name (&architecture, &platform);
    const int webcore_major = 537;
    const int webcore_minor = 32;
       string = g_strdup_printf ("Mozilla/5.0 (%s %s %s) AppleWebKit/%d.%d "
                                 "(KHTML, like Gecko) Chrome/18.0.1025.133 Safari/%d.%d " "CDOSBrowser/",
                                 platform, os, architecture,webcore_major, webcore_minor, webcore_major, webcore_minor);
       g_object_set(view->settings, "user-agent", string, NULL);

}

//add by luyue 2015/8/8 start
void
midori_view_set_user_agent (MidoriView*   view,
                            const gchar*  uri)
{
    gchar* platform;
    gchar* architecture;
    char *string;
    const gchar* os = midori_web_settings_get_system_name (&architecture, &platform);
    const int webcore_major = 537;
    const int webcore_minor = 32;
    ssize_t read = -1;

    //present_safiri_agent_uri.txt中预制需要safiri代理网站的主域名
    gchar* path = midori_paths_get_res_filename ("safiri_agent/present_safiri_agent_uri.txt");
    FILE * fp = fopen(path, "r");
    if(fp)
    {
       char* line = NULL;
       size_t len = 0;
       while ((read = getline(&line, &len, fp)) != -1)
       {
           line[strlen(line)-1]='\0';
          if(strstr(uri,line))
          {
             free(line);
             line = NULL;
             break;
          }
          free(line);
          line = NULL;
       }
       fclose(fp);
    }
    if(read == -1)
    {
       //chrome的代理
       string = g_strdup_printf ("Mozilla/5.0 (%s %s %s) AppleWebKit/%d.%d "
                                 "(KHTML, like Gecko) Chrome/18.0.1025.133 Safari/%d.%d " "CDOSBrowser/",
                                 platform, os, architecture,webcore_major, webcore_minor, webcore_major, webcore_minor);
       g_object_set(view->settings, "user-agent", string, NULL);
    }
    else
    {
       //safiri的代理
       string = g_strdup_printf ("Mozilla/5.0 (%s %s %s) AppleWebKit/%d.%d "
                                 "(KHTML, like Gecko) Safari/%d.%d " "CDOSBrowser/",
                                 platform, os, architecture,webcore_major, webcore_minor, webcore_major, webcore_minor);
                                 g_object_set(view->settings, "user-agent", string, NULL);
    }
    string = NULL;
}
//add end

/**
 * midori_view_set_uri:
 * @view: a #MidoriView
 *
 * Opens the specified URI in the view.
 *
 * Since 0.3.0 a warning is shown if the view is not yet
 * contained in a browser. This is because extensions
 * can't monitor page loading if that happens.
 **/
void
midori_view_set_uri (MidoriView*  view,
                     const gchar* uri)
{
#ifdef APP_LEVEL_TIME
printf("set_uri start time = %lld\n",g_get_real_time());
#endif

    g_return_if_fail (MIDORI_IS_VIEW (view));
    g_return_if_fail (uri != NULL);

    //add by luyue 2015/5/4 start
    //解决有些页面要求自动关闭的问题
    g_signal_connect (view->web_view, "close",
                      G_CALLBACK (midori_view_web_view_close_cb), view);
    //add end
    if (!gtk_widget_get_parent (GTK_WIDGET (view)))
        g_warning ("Calling %s() beore adding the view to a browser. This "
                   "breaks extensions that monitor page loading.", G_STRFUNC);

    midori_uri_recursive_fork_protection (uri, TRUE);

    if (!midori_debug ("unarmed"))
    {
        gboolean handled = FALSE;
        if (g_str_has_prefix (uri, "about:"))
        {
	    GtkNotebook *notebook;
            g_signal_emit (view, signals[ABOUT_CONTENT], 0, uri, &handled);/*
	     MidoriBrowser* browser =  midori_browser_get_for_widget((GtkWidget*)view);
            GtkWidget* tab = midori_browser_get_current_tab ( browser);
            g_object_get(browser,"notebook",&notebook,NULL);
            GtkWidget*label =  gtk_notebook_get_tab_label(notebook,tab);
            GtkImage*image =  midori_tally_get_icon((MidoriTally*)label);
            gtk_image_set_from_gicon(image,g_themed_icon_new_with_default_fallbacks ("text-html-symbolic"),GTK_ICON_SIZE_MENU);*/
        }
        if (handled)
        {
	    GtkNotebook *notebook;
            midori_tab_set_uri (MIDORI_TAB (view), uri);
            midori_tab_set_special (MIDORI_TAB (view), TRUE);
            katze_item_set_meta_integer (view->item, "delay", MIDORI_DELAY_UNDELAYED);
            katze_item_set_uri (view->item, midori_tab_get_uri (MIDORI_TAB (view)));
	    // added by wangyl 2015.7.9  start 为了改变tab的图片
	    MidoriBrowser* browser =  midori_browser_get_for_widget((GtkWidget*)view);
	    GtkWidget* tab = midori_browser_get_current_tab ( browser);
            g_object_get(browser,"notebook",&notebook,NULL);
            GtkWidget*label =  gtk_notebook_get_tab_label(notebook,tab);
            GtkImage*image =  midori_tally_get_icon((MidoriTally*)label);
            gtk_image_set_from_gicon(image,g_themed_icon_new_with_default_fallbacks ("text-html-symbolic"),GTK_ICON_SIZE_MENU);
	    // added by wangyl 2015.7.9 end 
	    return;
        }

        if (katze_item_get_meta_integer (view->item, "delay") == MIDORI_DELAY_DELAYED)
        {
            midori_tab_set_uri (MIDORI_TAB (view), uri);
            midori_tab_set_special (MIDORI_TAB (view), TRUE);
            katze_item_set_meta_integer (view->item, "delay", MIDORI_DELAY_PENDING_UNDELAY);
            midori_view_display_error (view, NULL, "stock://dialog/network-idle", NULL,
                _("Page loading delayed:"),
                _("Loading delayed either due to a recent crash or startup preferences."),
                NULL,
                _("Load Page"),
                NULL);
        }
        else if (g_str_has_prefix (uri, "javascript:"))
        {
            gchar* exception = NULL;
            gboolean result = midori_view_execute_script (view, &uri[11], &exception);
            if (!result)
            {
                sokoke_message_dialog (GTK_MESSAGE_ERROR, "javascript:",
                                       exception, FALSE);
                g_free (exception);
            }
        }
        else
        {
            if (sokoke_external_uri (uri))
            {
                gboolean handled = FALSE;
                g_signal_emit_by_name (view, "open-uri", uri, &handled);
                if (handled)
                    return;
            }
//lxx,20150317
	    gchar *new_uri = sokoke_magic_uri ( uri, TRUE, TRUE);
            if (!new_uri)
            {
               gchar* search = katze_object_get_string (view->settings, "location-entry-search");
               new_uri = midori_uri_for_search (search, uri);  
               g_free (search);
            }
            midori_tab_set_uri (MIDORI_TAB (view), new_uri);
            katze_item_set_uri (view->item, midori_tab_get_uri (MIDORI_TAB (view)));
            katze_assign (view->title, NULL);
            midori_tab_set_view_source (MIDORI_TAB (view), FALSE);
            //add by luyue 2015/3/9
            if(view->danager_uri_flag)
            {
               if(!view->phish_check_flag)
                  g_signal_connect (view->web_view, "document-load-finish",
                                    (GCallback)midori_view_check_phish_cb,view);
               if(!view->popupwindow_check_flag)
                  g_signal_connect (view->web_view, "finish-progress",
                                    (GCallback)midori_view_check_popupwindow_cb,view);
               webkit_web_view_load_uri (WEBKIT_WEB_VIEW (view->web_view), new_uri);
               return;
            }
            //对特定的url进行恶意检测
            if(view->anquanjibie)
            {
               if(view->back_uri)
               {
                  free(view->back_uri);
                  view->back_uri = NULL;
               }
               view->back_uri = (char *)malloc(strlen(new_uri)+1);
               strcpy(view->back_uri,new_uri);
            }
            view->load_uri = (char *) malloc(strlen(new_uri)+1);
            strcpy(view->load_uri,new_uri);
            if(!strncmp (new_uri, "http", 4) && strncmp (new_uri, "http://172", 9) && strncmp (new_uri, "http://192", 9))
            {
               WebKitURIRequest *request = webkit_uri_request_new(new_uri);
               const gchar *base_domain = webkit_uri_request_get_uri_host (request);
               gchar *web_tab_uri = g_strdup_printf("http://webscan.360.cn/index/checkwebsite?url=%s", base_domain);
               gchar *base_domain1 = _get_base_domain(base_domain);
               gchar* path = midori_paths_get_res_filename ("website_check/present_secure_website.txt");    
               FILE * fp = fopen(path, "r"); 
               if (!fp || !base_domain1) 
               {
                  midori_web_view1_website_check_cb(view,NULL);
                  return;
               }  
               char * line = NULL;
               size_t len = 0;
               ssize_t read;
               while ((read = getline(&line, &len, fp)) != -1)
               {
                  if(!strncmp(line,base_domain1,strlen(base_domain1)))
                  {
                     midori_web_view1_website_check_cb(view,NULL);
                     free(line);
                     line = NULL;
                     fclose(fp);
                     return;
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
               gchar* home = getenv("HOME");
               gchar user_path[2048];
               g_sprintf(user_path, "%s/.config/cdosbrowser/secure_website.txt", home);
               fp = fopen(user_path, "r");
               if (fp )
               {
                  while ((read = getline(&line, &len, fp)) != -1)
                  {
                     if(!strncmp(line,base_domain,strlen(base_domain)))
                     {
                        midori_web_view1_website_check_cb(view,NULL);
                        free(line);
                        line = NULL;
                        fclose(fp);
                        return;
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
               midori_tab_set_website (MIDORI_TAB (view), true);
               WebKitSettings *localSettings = webkit_settings_new();
               webkit_settings_set_enable_javascript(localSettings, true);
               webkit_settings_set_enable_web_security(localSettings, false);
               webkit_settings_set_auto_load_images(localSettings, false);
               webkit_settings_set_load_icons_ignoring_image_load_setting(localSettings,true);
               webkit_settings_set_enable_plugins(localSettings, false);
               webkit_settings_set_enable_page_cache(localSettings, false);
               view->web_view1 = webkit_web_view_new_with_settings(localSettings);
               g_object_connect (view->web_view1,
                                 "signal::console-message",
                                 webkit_web_view1_console_message_cb,view, 
                                 "signal::load-changed",
                                 midori_view_web_view1_load_changed_cb,view,
                                 "signal::notify::estimated-load-progress",
                                 webkit_web_view1_progress_changed_cb,view,NULL);
               g_object_connect (view,
                                 "signal::website-check",
                                 midori_web_view1_website_check_cb,view,
                                 "signal::dangerous-url",
                                 midori_view_web_view1_dangerous_cb,view,NULL);
               webkit_web_view_load_uri(WEBKIT_WEB_VIEW(view->web_view1),web_tab_uri);
            }
            else
               midori_web_view1_website_check_cb(view,NULL);
        //add end
        }
    }
}

//add by luyue 2015/6/5 start
static void
midori_view_check_phish_cb (WebKitWebView* web_view,
                            MidoriView*    view)
{
   if(!view->title) return;
   gchar *script=NULL;
   FILE *fp;
   int file_size;

   if((fp = fopen(midori_paths_get_res_filename("phish/content_script.js"),"r")) != NULL)
   {
      fseek(fp,0,SEEK_END);
      file_size = ftell(fp);
      fseek(fp,0,SEEK_SET);
      script = (char *)malloc(file_size * sizeof(char)+1);
      fread(script,file_size,sizeof(char),fp);
      script[file_size*sizeof(char)] = '\0';
      fclose(fp);
      webkit_web_view_run_javascript(web_view,script,NULL,NULL,NULL);
      g_free(script);
      script = NULL;
        g_signal_handlers_disconnect_by_func (view->web_view,
            midori_view_check_phish_cb, view);
   }
}
//add end

//add by luyue 2015/6/11 start
static void
midori_view_check_popupwindow_cb (WebKitWebView* web_view,
                                  MidoriView*    view)
{
   if(!view->title) return;
   gchar *script=NULL;
   FILE *fp;
   int file_size;

   if((fp = fopen(midori_paths_get_res_filename("popupwindow/filtering.js"),"r")) != NULL)
   {
      fseek(fp,0,SEEK_END);
      file_size = ftell(fp);
      fseek(fp,0,SEEK_SET);
      script = (char *)malloc(file_size * sizeof(char)+1);
      fread(script,file_size,sizeof(char),fp);
      script[file_size*sizeof(char)] = '\0';
      fclose(fp);
      webkit_web_view_run_javascript(web_view,script,NULL,NULL,NULL);
      g_free(script);
      script = NULL;
   }
}
//add end

//add by luyue 2015/3/9
static void
midori_web_view1_website_check_cb(MidoriView*    view,
                                  MidoriView*    old_view)
{
   if(view->forward_uri)
   {
      free(view->forward_uri);
      view->forward_uri = NULL;
   }
   if(!view->danager_uri_flag)
      g_signal_emit (view, signals[FORWARD_URL], 0,view->forward_uri);
   if(!view->phish_check_flag) 
      g_signal_connect (view->web_view, "document-load-finish",
                        (GCallback)midori_view_check_phish_cb,view);
   if(!view->popupwindow_check_flag)
      g_signal_connect (view->web_view, "finish-progress",
                        (GCallback)midori_view_check_popupwindow_cb,view);
   webkit_web_view_load_uri (WEBKIT_WEB_VIEW (view->web_view), view->load_uri);
}
//add end

/**
 * midori_view_set_overlay_text:
 * @view: a #MidoriView
 * @text: a URI or text string
 *
 * Show a specified URI or text on top of the view.
 * Has no effect with < GTK+ 3.2.0.
 *
 * Since: 0.4.5
 **/
void
midori_view_set_overlay_text (MidoriView*  view,
                              const gchar* text)
{
    //g_return_if_fail (MIDORI_IS_VIEW (view));
	if(!MIDORI_IS_VIEW (view))return;
    #if GTK_CHECK_VERSION (3, 2, 0)
    if (text == NULL)
        gtk_widget_hide (gtk_widget_get_parent (view->overlay_label));
    else
    {
        gtk_label_set_text (GTK_LABEL (view->overlay_label), text);
        gtk_widget_show (gtk_widget_get_parent (view->overlay_label));
    }
    #endif
}

/**
 * midori_view_is_blank:
 * @view: a #MidoriView
 *
 * Determines whether the view is currently empty.
 **/
gboolean
midori_view_is_blank (MidoriView*  view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), TRUE);

    return midori_tab_is_blank (MIDORI_TAB (view));
}

/**
 * midori_view_get_icon:
 * @view: a #MidoriView
 *
 * Retrieves the icon of the view, or a default icon. See
 * midori_view_get_icon_uri() if you need to distinguish
 * the origin of an icon.
 *
 * The returned icon is owned by the @view and must not be modified.
 *
 * Return value: a #GdkPixbuf, or %NULL
 **/
GdkPixbuf*
midori_view_get_icon (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    return view->icon;
}

/**
 * midori_view_get_icon_uri:
 * @view: a #MidoriView
 *
 * Retrieves the address of the icon of the view
 * if the loaded website has an icon, otherwise
 * %NULL.
 * Note that if there is no icon uri, midori_view_get_icon()
 * will still return a default icon.
 *
 * The returned string is owned by the @view and must not be freed.
 *
 * Return value: a string, or %NULL
 *
 * Since: 0.2.5
 **/
const gchar*
midori_view_get_icon_uri (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    return view->icon_uri;
}

/**
 * midori_view_get_display_uri:
 * @view: a #MidoriView
 *
 * Retrieves a string that is suitable for displaying.
 *
 * Note that "about:blank" and "about:dial" are represented as "".
 *
 * You can assume that the string is not %NULL.
 *
 * Return value: an URI string
 **/
const gchar*
midori_view_get_display_uri (MidoriView* view)
{
    const gchar* uri;

    g_return_val_if_fail (MIDORI_IS_VIEW (view), "");

    uri = midori_tab_get_uri (MIDORI_TAB (view));
    /* Something in the stack tends to turn "" into "about:blank".
       Yet for practical purposes we prefer "".  */
    if (!strcmp (uri, "about:blank")
     || !strcmp (uri, "about:dial")
     || !strcmp (uri, "about:new")
     || !strcmp (uri, "about:private"))
        return "";

    return uri;
}

/**
 * midori_view_get_display_title:
 * @view: a #MidoriView
 *
 * Retrieves a string that is suitable for displaying
 * as a title. Most of the time this will be the title
 * or the current URI.
 *
 * You can assume that the string is not %NULL.
 *
 * Return value: a title string
 **/
const gchar*
midori_view_get_display_title (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), "about:blank");

    if (view->title && *view->title)
        return view->title;
    if (midori_view_is_blank (view))
        return _("Blank page");
    return midori_view_get_display_uri (view);
}

/**
 * midori_view_get_link_uri:
 * @view: a #MidoriView
 *
 * Retrieves the uri of the currently focused link,
 * particularly while the mouse hovers a link or a
 * context menu is being opened.
 *
 * Return value: an URI string, or %NULL if there is no link focussed
 **/
const gchar*
midori_view_get_link_uri (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    return view->link_uri;
}

/**
 * midori_view_has_selection:
 * @view: a #MidoriView
 *
 * Determines whether something in the view is selected.
 *
 * This function returns %FALSE if there is a selection
 * that effectively only consists of whitespace.
 *
 * Return value: %TRUE if effectively there is a selection
 **/
gboolean
midori_view_has_selection (MidoriView* view)
{
    return FALSE;
}

/**
 * midori_view_get_selected_text:
 * @view: a #MidoriView
 *
 * Retrieves the currently selected text.
 *
 * Return value: the selected text, or %NULL
 **/
const gchar*
midori_view_get_selected_text (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    if (midori_view_has_selection (view))
        return g_strstrip (view->selected_text);
    return NULL;
}

GtkWidget*
midori_view_duplicate (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    MidoriNewView where = MIDORI_NEW_VIEW_TAB;
    GtkWidget* new_view = midori_view_new_with_item (view->item, view->settings);
    g_signal_emit (view, signals[NEW_VIEW], 0, new_view, where, TRUE);
    midori_view_set_uri (MIDORI_VIEW (new_view), midori_tab_get_uri (MIDORI_TAB (view)));
    return new_view;
}

/**
 * midori_view_get_tab_menu:
 * @view: a #MidoriView
 *
 * Retrieves a menu that is typically shown when right-clicking
 * a tab label or equivalent representation.
 *
 * Return value: a #GtkMenu
 *
 * Since: 0.1.8
 * Deprecated: 0.5.7: Use MidoriNotebook API instead.
 **/
GtkWidget*
midori_view_get_tab_menu (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    GtkWidget* notebook = gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (view)));
    MidoriContextAction* context_action = midori_notebook_get_tab_context_action (MIDORI_NOTEBOOK (notebook), MIDORI_TAB (view));
    GtkMenu* menu = midori_context_action_create_menu (context_action, NULL, FALSE);
    g_object_unref (context_action);
    return GTK_WIDGET (menu);
}

/**
 * midori_view_get_label_ellipsize:
 * @view: a #MidoriView
 *
 * Determines how labels representing the view should be
 * ellipsized, which is helpful for alternative labels.
 *
 * Return value: how to ellipsize the label
 *
 * Since: 0.1.9
 **/
PangoEllipsizeMode
midori_view_get_label_ellipsize (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), PANGO_ELLIPSIZE_END);

    return view->ellipsize;
}

static void
midori_view_item_meta_data_changed (KatzeItem*   item,
                                    const gchar* key,
                                    MidoriView*  view)
{
    if (g_str_equal (key, "minimized"))
        g_object_set (view, "minimized",
            katze_item_get_meta_string (item, key) != NULL, NULL);
    else if (g_str_has_prefix (key, "scroll"))
    {
        gint value = katze_item_get_meta_integer (item, key);
        if (view->scrollh == -2 && key[6] == 'h')
            view->scrollh = value > -1 ? value : 0;
        else if (view->scrollv == -2 && key[6] == 'v')
            view->scrollv = value > -1 ? value : 0;
        else
            return;
    }
}

/**
 * midori_view_get_proxy_item:
 * @view: a #MidoriView
 *
 * Retrieves a proxy item that can be used for bookmark storage as
 * well as session management.
 *
 * The item reflects changes to title (name), URI and MIME type (mime-type).
 *
 * Return value: the proxy #KatzeItem
 **/
KatzeItem*
midori_view_get_proxy_item (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    return view->item;
}

/**
 * midori_view_get_zoom_level:
 * @view: a #MidoriView
 *
 * Determines the current zoom level of the view.
 *
 * Return value: the current zoom level
 **/
gfloat
midori_view_get_zoom_level (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), 1.0f);

    if (view->web_view != NULL)
        return webkit_web_view_get_zoom_level (WEBKIT_WEB_VIEW (view->web_view));
    return 1.0f;
}

/**
 * midori_view_set_zoom_level:
 * @view: a #MidoriView
 * @zoom_level: the new zoom level
 *
 * Sets the current zoom level of the view.
 **/
void
midori_view_set_zoom_level (MidoriView* view,
                            gfloat      zoom_level)
{
    g_return_if_fail (MIDORI_IS_VIEW (view));

    webkit_web_view_set_zoom_level (
        WEBKIT_WEB_VIEW (view->web_view), zoom_level);
    g_object_notify (G_OBJECT (view), "zoom-level");
}

gboolean
midori_view_can_zoom_in (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), FALSE);

    return view->web_view != NULL
        && (katze_object_get_boolean (view->settings, "zoom-text-and-images")
        || !g_str_has_prefix (midori_tab_get_mime_type (MIDORI_TAB (view)), "image/"));
}

gboolean
midori_view_can_zoom_out (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), FALSE);

    return view->web_view != NULL
        && (katze_object_get_boolean (view->settings, "zoom-text-and-images")
        || !g_str_has_prefix (midori_tab_get_mime_type (MIDORI_TAB (view)), "image/"));
}

static void
midori_web_resource_get_data_cb (WebKitWebResource *resource,
                          GAsyncResult *result,
                          GOutputStream *output_stream)
{
    guchar *data;
    gsize data_length;
    GInputStream *input_stream;
    GError *error = NULL;

    data = webkit_web_resource_get_data_finish (resource, result, &data_length, &error);
    if (!data) {
        if (error)
           g_error_free (error);
        g_object_unref (output_stream);

        return;
    }

    input_stream = g_memory_input_stream_new_from_data (data, data_length, g_free);
    g_output_stream_splice_async (output_stream, input_stream,
                                  G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                  G_PRIORITY_DEFAULT,
                                  NULL, NULL, NULL);
    g_object_unref (input_stream);
    g_object_unref (output_stream);
}

static void
midori_web_view_save_main_resource_cb (GFile *file,
                                       GAsyncResult *result,
                                       WebKitWebView *view)
{
    GFileOutputStream *output_stream;
    WebKitWebResource *resource;
    GError *error = NULL;

    output_stream = g_file_replace_finish (file, result, &error);
    if (!output_stream) {
        g_printerr ("Failed to save page: %s", error->message);
        g_error_free (error);
        return;
    }

    resource = webkit_web_view_get_main_resource (view);
    webkit_web_resource_get_data (resource, NULL,
                                  (GAsyncReadyCallback)midori_web_resource_get_data_cb,
                                  output_stream);                                
} 

/**
 * midori_view_save_source:
 * @view: a #MidoriView
 * @uri: an alternative destination URI, or %NULL
 * @outfile: a destination filename, or %NULL
 *
 * Saves the data in the view to disk.
 *
 * Return value: the destination filename
 *
 * Since: 0.4.4
 **/
gchar*
midori_view_save_source (MidoriView*  view,
                         const gchar* uri,
                         const gchar* outfile,
                         gboolean     use_dom)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);
    
    if (uri == NULL)
        uri = midori_view_get_display_uri (view);

    if (g_str_has_prefix (uri, "file:///"))
        return g_filename_from_uri (uri, NULL, NULL);

    GFile *file;
    char *converted = NULL;
    WebKitWebView * web_view = WEBKIT_WEB_VIEW (view->web_view);
    g_return_if_fail (uri);

    if (!outfile)
        converted = g_filename_to_utf8 (uri, -1, NULL, NULL, NULL);
    else
        converted = g_strdup (outfile);

    file = g_file_new_for_uri (converted);

    if (g_str_has_suffix (outfile, ".mht"))
    {
        webkit_web_view_save_to_file (WEBKIT_WEB_VIEW (web_view), file, WEBKIT_SAVE_MODE_MHTML,
                                  NULL, NULL, NULL);
    }
    else
        g_file_replace_async (file, NULL, FALSE,
                          G_FILE_CREATE_REPLACE_DESTINATION | G_FILE_CREATE_PRIVATE,
                          G_PRIORITY_DEFAULT, NULL,
                          (GAsyncReadyCallback)midori_web_view_save_main_resource_cb,
                          web_view);
    g_free (converted);
    g_object_unref (file);
    return converted;
}

/**
 * midori_view_reload:
 * @view: a #MidoriView
 * @from_cache: whether to allow caching
 *
 * Reloads the view.
 **/
void
midori_view_reload (MidoriView* view,
                    gboolean    from_cache)
{
    g_return_if_fail (MIDORI_IS_VIEW (view));

    //add by luyue 2015/3/10
    if(view->anquanjibie && !strcmp(view->anquanjibie,"高危"))
    {
       g_signal_emit (view, signals[DANGEROUS_URL], 0);
       return;
    }
    //add end

    if (midori_tab_is_blank (MIDORI_TAB (view)))
    {
        /* Duplicate here because the URI pointer might change */
        gchar* uri = g_strdup (midori_tab_get_uri (MIDORI_TAB (view)));
        midori_view_set_uri (view, uri);
        g_free (uri);
    }
    else if (from_cache)
        webkit_web_view_reload (WEBKIT_WEB_VIEW (view->web_view));
    else
        webkit_web_view_reload_bypass_cache (WEBKIT_WEB_VIEW (view->web_view));
}

/**
 * midori_view_can_go_back
 * @view: a #MidoriView
 *
 * Determines whether the view can go back.
 **/
gboolean
midori_view_can_go_back (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), FALSE);

    if (view->web_view)
    {
        //add by luyue 2015/3/11
        GtkWidget* current_web_view = midori_view_get_web_view (view);
        const gchar *uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (current_web_view));
        //http://www.baidu.com和https://www.baidu.com认为是同一网址
        if(!view->back_uri)
           return webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (view->web_view));
        else
        {
           uri = strchr(uri,'/');
           char *tmp_uri = strchr(view->back_uri,'/');
           if(strncmp(uri,tmp_uri,strlen(tmp_uri)))
           //add end
              return webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (view->web_view));
           else
              return FALSE;
        }
    }
    else
        return FALSE;
}

/**
 * midori_view_go_back
 * @view: a #MidoriView
 *
 * Goes back one page in the view.
 **/
void
midori_view_go_back (MidoriView* view)
{
    g_return_if_fail (MIDORI_IS_VIEW (view));

    webkit_web_view_go_back (WEBKIT_WEB_VIEW (view->web_view));
    /* Force the speed dial to kick in if going back to a blank page */
    if (midori_view_is_blank (view))
        midori_view_set_uri (view, "");
    gchar *uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW (view->web_view));
    if(strstr (uri, "speeddial-head.html"))
    {
     GtkNotebook *notebook;
     midori_tab_set_uri (MIDORI_TAB (view), uri);
     midori_tab_set_special (MIDORI_TAB (view), TRUE);
     katze_item_set_meta_integer (view->item, "delay", MIDORI_DELAY_UNDELAYED);
     katze_item_set_uri (view->item, midori_tab_get_uri (MIDORI_TAB (view)));
     MidoriBrowser* browser =  midori_browser_get_for_widget((GtkWidget*)view);
     GtkWidget* tab = midori_browser_get_current_tab ( browser);
     g_object_get(browser,"notebook",&notebook,NULL);
     GtkWidget*label =  gtk_notebook_get_tab_label(notebook,tab);
     GtkImage*image =  midori_tally_get_icon((MidoriTally*)label);
     gtk_image_set_from_gicon(image,g_themed_icon_new_with_default_fallbacks ("text-html-symbolic"),GTK_ICON_SIZE_MENU);
   }
}

static gchar*
midori_view_get_related_page (MidoriView*  view,
                              const gchar* rel,
                              const gchar* local)
{
    return NULL;
}

/**
 * midori_view_get_previous_page
 * @view: a #MidoriView
 *
 * Determines the previous sub-page in the view.
 *
 * Return value: an URI, or %NULL
 *
 * Since: 0.2.3
 **/
const gchar*
midori_view_get_previous_page (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    /* i18n: word stem of "previous page" type links, case is not important */
    return midori_view_get_related_page (view, "prev", _("previous"));
}

/**
 * midori_view_get_next_page
 * @view: a #MidoriView
 *
 * Determines the next sub-page in the view.
 *
 * Return value: an URI, or %NULL
 *
 * Since: 0.2.3
 **/
const gchar*
midori_view_get_next_page (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    /* i18n: word stem of "next page" type links, case is not important */
    return midori_view_get_related_page (view, "next", _("next"));
}

/**
 * midori_view_print
 * @view: a #MidoriView
 *
 * Prints the contents of the view.
 **/
void
midori_view_print (MidoriView* view)
{
    g_return_if_fail (MIDORI_IS_VIEW (view));

    GtkPrintSettings* settings = gtk_print_settings_new ();
    #if GTK_CHECK_VERSION (3, 6, 0)
    gtk_print_settings_set (settings, GTK_PRINT_SETTINGS_OUTPUT_BASENAME, midori_view_get_display_title (view));
    #endif

    WebKitPrintOperation* operation = webkit_print_operation_new (WEBKIT_WEB_VIEW (view->web_view));
    webkit_print_operation_set_print_settings (operation, settings);
    g_object_unref (settings);

    if (katze_object_get_boolean (view->settings, "print-without-dialog")) {
        webkit_print_operation_print (operation);
    }
    else {
        webkit_print_operation_run_dialog (operation,
            GTK_WINDOW (midori_browser_get_for_widget (view->web_view)));
    }
    g_object_unref (operation);
}

/**
 * midori_view_execute_script
 * @view: a #MidoriView
 * @script: script code
 * @exception: location to store an exception message
 *
 * Execute a script on the view.
 *
 * Returns: %TRUE if the script was executed successfully
 **/
gboolean
midori_view_execute_script (MidoriView*  view,
                            const gchar* script,
                            gchar**      exception)
{
    return FALSE;
}

/**
 * midori_view_get_web_view
 * @view: a #MidoriView
 *
 * Returns: The #WebKitWebView for this view
 *
 * Since: 0.2.5
 * Deprecated: 0.4.8: Use midori_tab_get_web_view() instead.
 **/
GtkWidget*
midori_view_get_web_view (MidoriView* view)
{
    g_return_val_if_fail (MIDORI_IS_VIEW (view), NULL);

    return view->web_view;
}

/**
 * midori_view_get_for_widget:
 * @widget: a #GtkWidget
 *
 * Determines the view appropriate for the specified widget.
 *
 * Return value: a #MidoriView
 *
 * Since 0.4.5
 **/
MidoriView*
midori_view_get_for_widget (GtkWidget* web_view)
{
    g_return_val_if_fail (GTK_IS_WIDGET (web_view), NULL);

    GtkWidget* scrolled = web_view;
    #if GTK_CHECK_VERSION(3, 2, 0)
    GtkWidget* overlay = gtk_widget_get_parent (scrolled);
    GtkWidget* view = gtk_widget_get_parent (overlay);
    #else
    GtkWidget* view = gtk_widget_get_parent (scrolled);
    #endif
    return MIDORI_VIEW (view);
}
