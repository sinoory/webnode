/*
 Modified by ZRL
 2014.12.10 修复网页中打开新窗口或新Tab时，不加载网页问题。参考midori_view_new_view_cb()
 2014.12.16 修复点击地址栏搜索图标crash问题，打开原光辉屏蔽的代码，但注销信号接收。参见midori_browser_init()
 2014.12.16 屏蔽撤销关闭书签功能
 2014.12.16 实现网页的保存功能。参见midori_browser_save_uri()
 2014.12.17 屏蔽search action，见ENABLE_SEARCH_ACTION
 2014.12.30 屏蔽书签编辑窗口中创建启动器功能,见midori_browser_edit_bookmark_dialog_new()
*/

//#define BOOKMARK_SYNC//书签与服务器同步开关

#include "midori-browser.h"
#include "midori-app.h"
#include "midori-extension.h"
#include "midori-array.h"
#include "midori-view.h"
#include "midori-preferences.h"
#include "midori-panel.h"
#include "midori-locationaction.h"
#include "midori-searchaction.h"
#include "midori-findbar.h"
#include "midori-platform.h"
#include "midori-privatedata.h"
#include "cdosbrowser-core.h"
#include "midori-privatedata.h"
#include "midori-bookmarks-db.h"
#include "katze-cellrenderercomboboxtext.h"
#include "../settings/BrowserSettingsDialog.h"
#include "Certificate.h"
#include "midori-extension.h"
#include "marshal.h"
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <config.h>

#ifdef HAVE_GRANITE
    #include <granite.h>
#endif

#ifdef HAVE_ZEITGEIST
    #include <zeitgeist.h>
#endif

#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif

#include <sqlite3.h>

#ifdef HAVE_X11_EXTENSIONS_SCRNSAVER_H
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/scrnsaver.h>
    #include <gdk/gdkx.h>
#endif

struct _MidoriBrowser
{
    GtkWindow parent_instance;
    GtkActionGroup* action_group;
    GtkWidget* menubar;
    GtkWidget* throbber;
    GtkWidget* navigationbar;
    GtkWidget* bookmarkbar;

    GtkWidget* panel;
    GtkWidget* notebook;

    GtkWidget* inspector;
    GtkWidget* inspector_view;

    GtkWidget* find;

    GtkWidget* statusbar;
    GtkWidget* statusbar_contents;
    gchar* statusbar_text;

    gint last_window_width, last_window_height;
    guint alloc_timeout;
    guint panel_timeout;

    MidoriWebSettings* settings;
    KatzeArray* proxy_array;
    MidoriBookmarksDb* bookmarks;
    KatzeArray* trash;
    KatzeArray* search_engines;
    KatzeArray* history;
    MidoriHistoryDatabase* history_database;
    MidoriSpeedDial* dial;
    gboolean show_tabs;

    gboolean show_navigationbar;
    gboolean show_statusbar;
    guint maximum_history_age;
    guint last_web_search;

    gboolean bookmarkbar_populate;
	//20141217 zlf
    GtkWidget* sari_panel_windows;
   //luyue add by 2015/1/20
    GtkWidget* smart_zoom_button;
    GtkWidget* smart_zoom_image;
    char * forward_url;
   //add end
   //luyue add by 2015/4/10
   GtkWidget* page_window;
   GtkWidget* page_frame;
   //add end
   GtkWidget* settings_dialog;//added by wangyl 2015/9/25
};

GtkWidget *page_button, *page_record_box;
GtkWidget *page_lab_signal_text, *page_view;
gboolean page_isConnectSignal;

//char* BookmarkToken;//用户登陆时得到，用于导入导出书签
char* BookmarkEmail;

G_DEFINE_TYPE (MidoriBrowser, midori_browser, GTK_TYPE_WINDOW)

enum
{
    PROP_0,
    PROP_MENUBAR,
    PROP_NAVIGATIONBAR,
    PROP_NOTEBOOK,
    PROP_PANEL,
    PROP_URI,
    PROP_TAB,
    PROP_LOAD_STATUS,
    PROP_STATUSBAR,
    PROP_STATUSBAR_TEXT,
    PROP_SETTINGS,
    PROP_PROXY_ITEMS,
    PROP_BOOKMARKS,
    PROP_TRASH,
    PROP_SEARCH_ENGINES,
    PROP_HISTORY,
    PROP_SPEED_DIAL,
    PROP_SHOW_TABS,
};

enum
{
    NEW_WINDOW,
    ADD_TAB,
    REMOVE_TAB,
    MOVE_TAB,
    SWITCH_TAB,
    ACTIVATE_ACTION,
    SEND_NOTIFICATION,
    PAGES_LOADED,
    POPULATE_TOOL_MENU,
    POPULATE_TOOLBAR_MENU,
    QUIT,
    SHOW_PREFERENCES,
    LAST_SIGNAL
};

enum
{
    PANEL_BOOKMARK,
    PANEL_HISTORY,
    PANEL_TRANSFER,    
    PANEL_MAX,
};

static guint signals[LAST_SIGNAL];
//20141217 zlf
static void 
midori_browser_actiave_bookmark_in_window(GtkAction*     action,
                                          MidoriBrowser* browser);
static void 
midori_browser_actiave_history_in_window(GtkAction*     action,
                                         MidoriBrowser* browser);
static void 
midori_browser_actiave_transfer_in_window(GtkAction*     action,
                                          MidoriBrowser* browser);
static void
midori_browser_show_panel_window(MidoriBrowser* browser, 
                                 gboolean       show);

static void
midori_panel_window_hide (GtkWidget*      window, 
                          MidoriBrowser*  browser);

static void
midori_browser_dispose (GObject* object);

static void
midori_browser_finalize (GObject* object);

static void
midori_browser_set_property (GObject*      object,
                             guint         prop_id,
                             const GValue* value,
                             GParamSpec*   pspec);

static void
midori_browser_get_property (GObject*    object,
                             guint       prop_id,
                             GValue*     value,
                             GParamSpec* pspec);

gboolean
midori_browser_open_bookmark (MidoriBrowser* browser,
                              KatzeItem*     item);

static void
midori_bookmarkbar_populate (MidoriBrowser* browser);
static void
midori_bookmarkbar_populate_idle (MidoriBrowser* browser);

static void
midori_bookmarkbar_clear (GtkWidget* toolbar);

static void
_midori_browser_set_toolbar_style (MidoriBrowser*     browser,
                                   MidoriToolbarStyle toolbar_style);

static void
midori_browser_settings_notify (MidoriWebSettings* web_settings,
                                GParamSpec*        pspec,
                                MidoriBrowser*     browser);

void
midori_panel_set_toolbar_style (MidoriPanel*    panel,
                                GtkToolbarStyle style);

static void
midori_browser_set_bookmarks (MidoriBrowser*     browser,
                              MidoriBookmarksDb* bookmarks);

static void
midori_browser_add_speed_dial (MidoriBrowser* browser, gchar* uri, gchar* title);

static void
midori_browser_step_history (MidoriBrowser* browser,
                             MidoriView*    view);
                             
static void //add by zgh 20141225
_update_tooltip_if_changed (GtkAction* action,
                            const gchar* text);

#define _action_by_name(brwsr, nme) \
    gtk_action_group_get_action (brwsr->action_group, nme)
#define _action_set_sensitive(brwsr, nme, snstv) \
    gtk_action_set_sensitive (_action_by_name (brwsr, nme), snstv);
#define _action_set_visible(brwsr, nme, vsbl) \
    gtk_action_set_visible (_action_by_name (brwsr, nme), vsbl);
#define _action_set_active(brwsr, nme, actv) \
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION ( \
    _action_by_name (brwsr, nme)), actv);
#define _action_set_label(brwsr, nme, label) \
    gtk_action_set_label (_action_by_name (brwsr, nme), label);

static void
midori_browser_disconnect_tab (MidoriBrowser* browser,
                               MidoriView*    view);

static gboolean
midori_browser_toolbar_item_button_press_event_cb (GtkWidget*      toolitem,
                                                   GdkEventButton* event,
                                                   MidoriBrowser*  browser);

static void
_midori_browser_set_toolbar_items (MidoriBrowser* browser,
                                   const gchar*   items);

void
midori_app_set_browsers (MidoriApp*     app,
                         KatzeArray*    browsers,
                         MidoriBrowser* browser);

static gboolean
midori_browser_is_fullscreen (MidoriBrowser* browser)
{
    GdkWindow* window = gtk_widget_get_window (GTK_WIDGET (browser));
    GdkWindowState state = window ? gdk_window_get_state (window) : 0;
    return state & GDK_WINDOW_STATE_FULLSCREEN;
}

static gboolean
_toggle_tabbar_smartly (MidoriBrowser* browser,
                        gboolean       ignore_fullscreen)
{
    gboolean has_tabs = midori_browser_get_n_pages (browser) > 1;
    gboolean show_tabs = !midori_browser_is_fullscreen (browser) || ignore_fullscreen;
    if (!browser->show_tabs)
        show_tabs = FALSE;
    midori_notebook_set_labels_visible (MIDORI_NOTEBOOK (browser->notebook), show_tabs);
    return has_tabs;
}

#if ENABLE_TRASH // ZRL 屏蔽撤销关闭标签功能
static void
midori_browser_trash_clear_cb (KatzeArray*    trash,
                               MidoriBrowser* browser)
{
    gboolean trash_empty = katze_array_is_empty (browser->trash);
#if ENABLE_TRASH // ZRL 屏蔽撤销关闭标签功能
    _action_set_sensitive (browser, "UndoTabClose", !trash_empty);
    _action_set_sensitive (browser, "Trash", !trash_empty);
#endif
}
#endif

static void
_midori_browser_update_actions (MidoriBrowser* browser)
{
    gboolean has_tabs = _toggle_tabbar_smartly (browser, FALSE);
    _action_set_sensitive (browser, "TabPrevious", has_tabs);
    _action_set_sensitive (browser, "TabNext", has_tabs);

#if ENABLE_TRASH // ZRL 屏蔽撤销关闭标签功能
    if (browser->trash)
        midori_browser_trash_clear_cb (browser->trash, browser);
#endif
}

static void
midori_browser_update_secondary_icon (MidoriBrowser* browser,
                                      MidoriView*    view,
                                      GtkAction*     action)
{
    //ZRL 当隐私模式时，没有bookmark功能，因此图标隐藏。
    if (!browser->bookmarks) {
        midori_location_action_set_secondary_icon (
            MIDORI_LOCATION_ACTION (action), NULL);
        return;
    }
    if (midori_bookmarks_db_exist_by_uri(browser->bookmarks, midori_tab_get_uri(MIDORI_TAB (view))))
    {
        midori_location_action_set_secondary_icon_tooltip (
                MIDORI_LOCATION_ACTION (action), STOCK_BOOKMARKED, _("Bookmark already exist"));   //STOCK_BOOKMARK_ADD
    }
    else
    {
        midori_location_action_set_secondary_icon_tooltip(
                MIDORI_LOCATION_ACTION(action), STOCK_UNBOOKMARK, _("Add to Bookmarks bar"));
    }
}

//add by luyue 2015/3/16
static void
midori_view_forward_url_cb(GtkWidget*     view,
                           char *         uri,
                           MidoriBrowser* browser)
{
   if(browser->forward_url)
   {
      free(browser->forward_url);
      browser->forward_url = NULL;
   }
   if(!uri)
   {
      browser->forward_url = NULL;
      return;
   }
   browser->forward_url = (char *)malloc(strlen(uri)+1);
   strcpy(browser->forward_url,uri);
}
//add end

static void
_midori_browser_update_interface (MidoriBrowser* browser,
                                  MidoriView*    view)
{
    GtkAction* action;

    _action_set_sensitive (browser, "Back", midori_view_can_go_back (view));
    //add by luyue 2015/3/11
    GtkWidget* current_web_view = midori_view_get_web_view (view);
    const gchar *uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW(current_web_view));
    if(!browser->forward_url || (uri && strcmp(uri,browser->forward_url)))
    {
       _action_set_sensitive (browser, "Forward", midori_tab_can_go_forward (MIDORI_TAB (view)));
       _action_set_visible (browser, "Forward", midori_tab_can_go_forward (MIDORI_TAB (view)));    //zgh
    }
    else
    {
       _action_set_sensitive (browser, "Forward", NULL);
       _action_set_visible (browser, "Forward", NULL);
    }
    //add end
    _action_set_sensitive (browser, "Previous",
        midori_view_get_previous_page (view) != NULL);
    _action_set_sensitive (browser, "Next",
        midori_view_get_next_page (view) != NULL);

    _action_set_sensitive (browser, "SaveAs", midori_tab_can_save (MIDORI_TAB (view)));
    _action_set_sensitive (browser, "ZoomIn", midori_view_can_zoom_in (view));
    _action_set_sensitive (browser, "ZoomOut", midori_view_can_zoom_out (view));
    _action_set_sensitive (browser, "ZoomNormal",
        midori_view_get_zoom_level (view) != 1.0f);
    _action_set_sensitive (browser, "Encoding",
        midori_tab_can_view_source (MIDORI_TAB (view)));
    _action_set_sensitive (browser, "SourceView",
        midori_tab_can_view_source (MIDORI_TAB (view)));
    action = _action_by_name (browser, "NextForward");
    if (midori_tab_can_go_forward (MIDORI_TAB (view)))
    {
        g_object_set (action,
                      "stock-id", GTK_STOCK_GO_FORWARD,
                      "tooltip", _("Go forward to the next page"),
                      "sensitive", TRUE, NULL);
    }
    else
    {
        g_object_set (action,
                      "stock-id", GTK_STOCK_MEDIA_NEXT,
                      "tooltip", _("Go to the next sub-page"),
                      "sensitive", midori_view_get_next_page (view) != NULL, NULL);
    }

    action = _action_by_name (browser, "Location");

    if (midori_tab_is_blank (MIDORI_TAB (view)))
    {
        gchar* icon_names[] = { "edit-find-symbolic", "edit-find", NULL };
        GIcon* icon = g_themed_icon_new_from_names (icon_names, -1);
        midori_location_action_set_primary_icon (
            MIDORI_LOCATION_ACTION (action), icon, _("Web Search…"));
        g_object_unref (icon);
    }
#if ENABLE_WEBSITE_AUTH
    else
        midori_location_action_set_security_hint (MIDORI_LOCATION_ACTION (action), GTK_WIDGET (view));
#else
    else
        midori_location_action_set_security_hint (
            MIDORI_LOCATION_ACTION (action), midori_tab_get_security (MIDORI_TAB (view)));
#endif
    midori_browser_update_secondary_icon (browser, view, action);   //zgh 1203 +放开 1224
}

static void
_midori_browser_set_statusbar_text (MidoriBrowser* browser,
                                    MidoriView*    view,
                                    const gchar*   text)
{
    #if GTK_CHECK_VERSION (3, 2, 0)
    gboolean is_location = FALSE;
    #else
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    gboolean is_location = widget && GTK_IS_ENTRY (widget)
        && GTK_IS_ALIGNMENT (gtk_widget_get_parent (widget));
    #endif

    katze_assign (browser->statusbar_text, midori_uri_format_for_display (text));
    if (view == NULL)
        return;

    if (!gtk_widget_get_visible (browser->statusbar) && !is_location
     && text && *text)
    {
        #if GTK_CHECK_VERSION (3, 2, 0)
        midori_view_set_overlay_text (view, browser->statusbar_text);
        #else
        GtkAction* action = _action_by_name (browser, "Location");
        MidoriLocationAction* location_action = MIDORI_LOCATION_ACTION (action);
        midori_location_action_set_text (location_action, browser->statusbar_text);
        #endif
    }
    else if (!gtk_widget_get_visible (browser->statusbar) && !is_location)
    {
        #if GTK_CHECK_VERSION (3, 2, 0)
        midori_view_set_overlay_text (view, NULL);
        #else
        GtkAction* action = _action_by_name (browser, "Location");
        MidoriLocationAction* location_action = MIDORI_LOCATION_ACTION (action);
        midori_browser_update_secondary_icon (browser, view, action);   //zgh 1203 +放开1224
        midori_location_action_set_text (location_action, 
            midori_view_get_display_uri (view));
        #endif
    }
    else
    {
        gtk_statusbar_pop (GTK_STATUSBAR (browser->statusbar), 1);
        gtk_statusbar_push (GTK_STATUSBAR (browser->statusbar), 1,
                            katze_str_non_null (browser->statusbar_text));
    }
}

void
midori_browser_set_current_page_smartly (MidoriBrowser* browser,
                                         gint           n)
{
    if (!katze_object_get_boolean (browser->settings,
        "open-tabs-in-the-background"))
        midori_browser_set_current_page (browser, n);
}

/**
 * midori_browser_set_current_tab_smartly:
 * @browser: a #MidoriBrowser
 * @view: a #GtkWidget
 *
 * Switches to the tab containing @view iff open-tabs-in-the-background is %FALSE.
 *
 * Since: 0.4.9
 **/
void
midori_browser_set_current_tab_smartly (MidoriBrowser* browser,
                                        GtkWidget*     view)
{
    if (!katze_object_get_boolean (browser->settings,
        "open-tabs-in-the-background"))
        midori_browser_set_current_tab (browser, view);
}

static void
_midori_browser_update_progress (MidoriBrowser* browser,
                                 MidoriView*    view)
{
    GtkAction* action;
    gdouble progress = midori_view_get_progress (view);
    gboolean loading = progress > 0.0;

    action = _action_by_name (browser, "Location");
    midori_location_action_set_progress (MIDORI_LOCATION_ACTION (action), progress);

    _action_set_sensitive (browser, "Reload", !loading);
    _action_set_sensitive (browser, "Stop", loading);

    action = _action_by_name (browser, "ReloadStop");
    if (!loading)
    {
        g_object_set (action,
                      "stock-id", NULL,
                      "icon_name", STOCK_REFRESH,
                      "tooltip", _("Reload the current page"), NULL);
    }
    else
    {
        g_object_set (action,
                      "stock-id", GTK_STOCK_STOP,
                      "tooltip", _("Stop loading the current page"), NULL);
    }
}

/**
 * midori_browser_update_history:
 * @item: a #KatzeItem
 * @type: "website", "bookmark" or "download"
 * @event: "access", "leave", "modify", "delete"
 *
 * Since: 0.4.7
 **/
void
midori_browser_update_history (KatzeItem*   item,
                               const gchar* type,
                               const gchar* event)
{
    g_return_if_fail (!KATZE_ITEM_IS_SEPARATOR (item));

    #ifdef HAVE_ZEITGEIST
    const gchar* inter;
    if (strstr (event, "access"))
        inter = ZEITGEIST_ZG_ACCESS_EVENT;
    else if (strstr (event, "leave"))
        inter = ZEITGEIST_ZG_LEAVE_EVENT;
    else if (strstr (event, "modify"))
        inter = ZEITGEIST_ZG_MODIFY_EVENT;
    else if (strstr (event, "create"))
        inter = ZEITGEIST_ZG_CREATE_EVENT;
    else if (strstr (event, "delete"))
        inter = ZEITGEIST_ZG_DELETE_EVENT;
    else
        g_assert_not_reached ();

    /* FIXME: Should insert folders into the log (ZEITGEIST_NFO_BOOKMARK_FOLDER) */
    if (KATZE_ITEM_IS_FOLDER (item))
        return;

    zeitgeist_log_insert_events_no_reply (zeitgeist_log_get_default (),
        zeitgeist_event_new_full (inter, ZEITGEIST_ZG_USER_ACTIVITY,
                                  "application://cdosbrowser.desktop",
                                  zeitgeist_subject_new_full (
            katze_item_get_uri (item),
            strstr (type, "bookmark") ? ZEITGEIST_NFO_BOOKMARK : ZEITGEIST_NFO_WEBSITE,
            zeitgeist_manifestation_for_uri (katze_item_get_uri (item)),
            katze_item_get_meta_string (item, "mime-type"), NULL, katze_item_get_name (item), NULL),
                                  NULL),
        NULL);
    #endif
}

static void
midori_browser_update_history_title (MidoriBrowser* browser,
                                     KatzeItem*     item)
{
    sqlite3* db;
    static sqlite3_stmt* stmt = NULL;

    g_return_if_fail (katze_item_get_uri (item) != NULL);

    db = g_object_get_data (G_OBJECT (browser->history), "db");
    g_return_if_fail (db != NULL);
    if (!stmt)
    {
        const gchar* sqlcmd;

        sqlcmd = "UPDATE history SET title=? WHERE uri = ? and date=?";
        sqlite3_prepare_v2 (db, sqlcmd, -1, &stmt, NULL);
    }
    sqlite3_bind_text (stmt, 1, katze_item_get_name (item), -1, 0);
    sqlite3_bind_text (stmt, 2, katze_item_get_uri (item), -1, 0);
    sqlite3_bind_int64 (stmt, 3, katze_item_get_added (item));

    if (sqlite3_step (stmt) != SQLITE_DONE)
        g_printerr (_("Failed to update title: %s\n"), sqlite3_errmsg (db));
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);

    midori_browser_update_history (item, "website", "access");
}

/**
 * midori_browser_assert_action:
 * @browser: a #MidoriBrowser
 * @name: action, setting=value expression or extension=true|false
 *
 * Assert that @name is a valid action or setting expression,
 * if it fails the program will terminate with an error.
 * To be used with command line interfaces.
 *
 * Since: 0.5.0
 **/
void
midori_browser_assert_action (MidoriBrowser* browser,
                              const gchar*   name)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (name != NULL);

    if (strchr (name, '='))
    {
        gchar** parts = g_strsplit (name, "=", 0);
        GObjectClass* class = G_OBJECT_GET_CLASS (browser->settings);
        GParamSpec* pspec = g_object_class_find_property (class, parts[0]);
        if (pspec != NULL)
        {
            GType type = G_PARAM_SPEC_TYPE (pspec);
            if (!(
                (type == G_TYPE_PARAM_BOOLEAN && (!strcmp (parts[1], "true") || !strcmp (parts[1], "false")))
             || type == G_TYPE_PARAM_STRING
             || type == G_TYPE_PARAM_INT
             || type == G_TYPE_PARAM_FLOAT
             || type == G_TYPE_PARAM_DOUBLE
             || type == G_TYPE_PARAM_ENUM))
                midori_error (_("Value '%s' is invalid for %s"), parts[1], parts[0]);
        }
        else
        {
            gchar* extension_path = midori_paths_get_extension_lib_path (PACKAGE_NAME);
            GObject* extension = midori_extension_load_from_file (extension_path, parts[0], FALSE, FALSE);
            g_free (extension_path);
            if (!extension || (strcmp (parts[1], "true") && strcmp (parts[1], "false")))
                midori_error (_("Unexpected setting '%s'"), name);
        }
        g_strfreev (parts);
    }
    else
    {
        GtkAction* action = _action_by_name (browser, name);
        if (!action)
            midori_error (_("Unexpected action '%s'."), name);
    }
}

static void
_midori_browser_activate_action (MidoriBrowser* browser,
                                 const gchar*   name)
{
    g_return_if_fail (name != NULL);

    if (strchr (name, '='))
    {
        gchar** parts = g_strsplit (name, "=", 0);
        GObjectClass* class = G_OBJECT_GET_CLASS (browser->settings);
        GParamSpec* pspec = g_object_class_find_property (class, parts[0]);
        if (pspec != NULL)
        {
            GType type = G_PARAM_SPEC_TYPE (pspec);
            if (type == G_TYPE_PARAM_BOOLEAN && !strcmp ("true", parts[1]))
                g_object_set (browser->settings, parts[0], TRUE, NULL);
            else if (type == G_TYPE_PARAM_BOOLEAN && !strcmp ("false", parts[1]))
                g_object_set (browser->settings, parts[0], FALSE, NULL);
            else if (type == G_TYPE_PARAM_STRING)
                g_object_set (browser->settings, parts[0], parts[1], NULL);
            else if (type == G_TYPE_PARAM_INT || type == G_TYPE_PARAM_UINT)
                g_object_set (browser->settings, parts[0], atoi (parts[1]), NULL);
            else if (type == G_TYPE_PARAM_FLOAT || type == G_TYPE_PARAM_DOUBLE)
                g_object_set (browser->settings, parts[0], g_ascii_strtod (parts[1], NULL), NULL);
            else if (type == G_TYPE_PARAM_ENUM)
            {
                GEnumClass* enum_class = G_ENUM_CLASS (g_type_class_peek (pspec->value_type));
                GEnumValue* enum_value = g_enum_get_value_by_name (enum_class, parts[1]);
                if (enum_value != NULL)
                    g_object_set (browser->settings, parts[0], enum_value->value, NULL);
                else
                    g_warning (_("Value '%s' is invalid for %s"), parts[1], parts[0]);
            }
            else
                g_warning (_("Value '%s' is invalid for %s"), parts[1], parts[0]);
        }
        else
        {
            gchar* extension_path = midori_paths_get_extension_lib_path (PACKAGE_NAME);
            GObject* extension = midori_extension_load_from_file (extension_path, parts[0], TRUE, FALSE);
            MidoriApp* app = midori_app_new_proxy (NULL);
            g_object_set (app,
                "settings", browser->settings,
                NULL);
            /* FIXME: tabs of multiple windows */
            KatzeArray* browsers = katze_array_new (MIDORI_TYPE_BROWSER);
            katze_array_add_item (browsers, browser);
            midori_app_set_browsers (app, browsers, browser);
            g_free (extension_path);
            if (extension && !strcmp (parts[1], "true"))
                midori_extension_activate (extension, parts[0], TRUE, app);
            else if (extension && !strcmp (parts[1], "false"))
                midori_extension_deactivate (MIDORI_EXTENSION (extension));
            else
                g_warning (_("Unexpected setting '%s'"), name);
        }
        g_strfreev (parts);
    }
    else
    {
        GtkAction* action = _action_by_name (browser, name);
        if (action)
            gtk_action_activate (action);
        else
            g_warning (_("Unexpected action '%s'."), name);
    }
}

static void
midori_view_notify_icon_cb (MidoriView*    view,
                            GParamSpec*    pspec,
                            MidoriBrowser* browser)
{
    if (midori_browser_get_current_tab (browser) != (GtkWidget*)view)
        return;

    if (midori_paths_get_runtime_mode () == MIDORI_RUNTIME_MODE_APP)
        gtk_window_set_icon (GTK_WINDOW (browser), midori_view_get_icon (view));
}

static void
midori_view_notify_load_status_cb (GtkWidget*      widget,
                                   GParamSpec*     pspec,
                                   MidoriBrowser*  browser)
{
    MidoriView* view = MIDORI_VIEW (widget);
    MidoriLoadStatus load_status = midori_view_get_load_status (view);

    if (widget == midori_browser_get_current_tab (browser))
    {
        if (load_status == MIDORI_LOAD_COMMITTED)
        {
            const gchar* uri = midori_view_get_display_uri (view);
            GtkAction* action = _action_by_name (browser, "Location");
            midori_location_action_set_text (
                MIDORI_LOCATION_ACTION (action), uri);

            /* Focus the urlbar on blank pages */
            if (midori_view_is_blank (view))
                midori_browser_activate_action (browser, "Location");
        }

        _midori_browser_update_interface (browser, view);
        _midori_browser_set_statusbar_text (browser, view, NULL);
    }

    if (load_status == MIDORI_LOAD_FINISHED)
        katze_item_set_meta_string (midori_view_get_proxy_item (view),
                                    "history-step", NULL);

    g_object_notify (G_OBJECT (browser), "load-status");
}

static void
midori_view_notify_progress_cb (GtkWidget*     view,
                                GParamSpec*    pspec,
                                MidoriBrowser* browser)
{
    if (view == midori_browser_get_current_tab (browser))
        _midori_browser_update_progress (browser, MIDORI_VIEW (view));
}

static void
midori_view_notify_uri_cb (GtkWidget*     widget,
                           GParamSpec*    pspec,
                           MidoriBrowser* browser)
{
    if (widget == midori_browser_get_current_tab (browser))
    {
        MidoriView* view = MIDORI_VIEW (widget);
        const gchar* uri = midori_view_get_display_uri (view);
        GtkAction* action = _action_by_name (browser, "Location");
        midori_location_action_set_text (MIDORI_LOCATION_ACTION (action), uri);
        _action_set_sensitive (browser, "Back", midori_view_can_go_back (view));
        _action_set_sensitive (browser, "Forward", midori_tab_can_go_forward (MIDORI_TAB (view)));
        g_object_notify (G_OBJECT (browser), "uri");
    }
}

static void
midori_browser_set_title (MidoriBrowser* browser,
                          const gchar*   title)
{
    const gchar* custom_title = midori_settings_get_custom_title (MIDORI_SETTINGS (browser->settings));
    if (custom_title && *custom_title)
        gtk_window_set_title (GTK_WINDOW (browser), custom_title);
    else if (katze_object_get_boolean (browser->settings, "enable-private-browsing"))
    {
        gchar* window_title = g_strdup_printf (_("%s (Private Browsing)"), title);
        gtk_window_set_title (GTK_WINDOW (browser), window_title);
        g_free (window_title);
    }
    else
        gtk_window_set_title (GTK_WINDOW (browser), title);
}

static void
midori_view_notify_title_cb (GtkWidget*     widget,
                             GParamSpec*    pspec,
                             MidoriBrowser* browser)
{
    MidoriView* view = MIDORI_VIEW (widget);
    if (widget == midori_browser_get_current_tab (browser))
    {
        midori_browser_set_title (browser, midori_view_get_display_title (view));
        g_object_notify (G_OBJECT (browser), "title");
    }

    midori_browser_step_history (browser, view);
}

static void
midori_browser_step_history (MidoriBrowser* browser,
                             MidoriView*    view)
{
    gint value;
    if (midori_view_get_load_status (view) != MIDORI_LOAD_COMMITTED)
        return;
    if (!browser->history_database || !browser->maximum_history_age)
        return;
    g_object_get(browser->settings,"history-setting", &value, NULL);//get current settings
    if(value != 0)
        return;// do not memory history

    KatzeItem* proxy = midori_view_get_proxy_item (view);
    const gchar* proxy_uri = katze_item_get_uri (proxy);
    if (midori_uri_is_blank (proxy_uri))
        return;

    const gchar* history_step = katze_item_get_meta_string (proxy, "history-step");
    if (history_step == NULL)
    {
        GError* error = NULL;
        time_t now = time (NULL);
        katze_item_set_added (proxy, now);
        gint64 day = sokoke_time_t_to_julian (&now);
        if(strstr(katze_item_get_uri (proxy),"speeddial-head.html"))return;
        midori_history_database_insert (browser->history_database,
            katze_item_get_uri (proxy),
            katze_item_get_name (proxy),
            katze_item_get_added (proxy), day, &error);
        if (error != NULL)
        {
            g_printerr (_("Failed to insert new history item: %s\n"), error->message);
            g_error_free (error);
            return;
        }
        katze_item_set_meta_string (proxy, "history-step", "update");
        /* FIXME: No signal for adding/ removing */
        katze_array_add_item (browser->history, proxy);
        katze_array_remove_item (browser->history, proxy);
    }
    else if (!strcmp (history_step, "update"))
    {
        if (proxy->name != NULL)
            midori_browser_update_history_title (browser, proxy);
    }
    else if (!strcmp (history_step, "ignore"))
    {
        /* This is set when restoring sessions */
    }
    else
        g_warning ("Unexpected history-step: %s", history_step);
}

static void
midori_view_notify_zoom_level_cb (GtkWidget*     view,
                                  GParamSpec*    pspec,
                                  MidoriBrowser* browser)
{
    if (view == midori_browser_get_current_tab (browser))
        _action_set_sensitive (browser, "ZoomNormal",
            midori_view_get_zoom_level (MIDORI_VIEW (view)) != 1.0f);
}

static void
midori_view_notify_statusbar_text_cb (GtkWidget*     view,
                                      GParamSpec*    pspec,
                                      MidoriBrowser* browser)
{
    gchar* text;

    if (view == midori_browser_get_current_tab (browser))
    {
        g_object_get (view, "statusbar-text", &text, NULL);
        _midori_browser_set_statusbar_text (browser, MIDORI_VIEW (view), text);
        g_free (text);
    }
}

//add by zgh 20141216
static KatzeArray*
midori_browser_history_read_from_db(MidoriBrowser* browser)
{

    sqlite3* db;
    sqlite3_stmt* statement;
    gint result = -1;
    const gchar* sqlcmd;
    
    if (browser->history_database == NULL)
        return katze_array_new (KATZE_TYPE_ITEM);

    db = g_object_get_data (G_OBJECT (browser->history), "db");

    if (!db)
        return katze_array_new (KATZE_TYPE_ITEM);

    sqlcmd = "SELECT uri, title, date "
             "FROM history "
             "ORDER BY date DESC LIMIT 0,15";
    result = sqlite3_prepare_v2 (db, sqlcmd, -1, &statement, NULL);

    if (result != SQLITE_OK)
        return katze_array_new (KATZE_TYPE_ITEM);

    return katze_array_from_statement (statement);

}

static gboolean
midori_bookmark_folder_button_reach_parent (GtkTreeModel* model, GtkTreeIter *iter, gint64 parentid)
{
    do
    {
        gint64 id;

        gtk_tree_model_get (model, iter, 1, &id, -1);

        if (parentid == id)
            return TRUE;

        if (gtk_tree_model_iter_has_child (model, iter))
        {
            GtkTreeIter child;
            gtk_tree_model_iter_children (model, &child, iter);
            if (midori_bookmark_folder_button_reach_parent (model, &child, parentid))
            {
                *iter = child;
                return TRUE;
            }
        }
    }
    while (gtk_tree_model_iter_next (model, iter));

    return FALSE;
}

typedef struct _FolderEntry
{
    const gchar *title;
    gint64 id;
    gint64 parentid;
} FolderEntry;

static void
midori_bookmark_folder_free_folder_entry (FolderEntry* folder)
{
    g_free ((gpointer)folder->title);
}

static GtkWidget*
midori_bookmark_folder_button_new (MidoriBookmarksDb* array,
                                   gint64      selected_parentid)
{
    GtkTreeStore* model;
    GtkWidget* combo;
    GtkCellRenderer* renderer;
    guint n;
    sqlite3* db;
    sqlite3_stmt* statement;
    gint result;
    const gchar* sqlcmd = "SELECT title, id, parentid FROM bookmarks WHERE uri='' ORDER BY parentid, title ASC";
    gint64 current_parentid;
    GtkTreeIter tree_iter;
    GtkTreeIter stock_parent_iter;
    GtkTreeIter* parent_iter;
    GList *folders = NULL;

    db = g_object_get_data (G_OBJECT (array), "db");
    g_return_val_if_fail (db != NULL, NULL);

    /* folder combo box model content:
    ** 0: title
    ** 1: id
    */
    model = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_INT64);
    combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));

    /* setup combo layout
    ** 0: a folder icon
    ** 1: the folder name
     */

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));

    renderer = gtk_cell_renderer_pixbuf_new ();
    g_object_set (G_OBJECT (renderer),
        "stock-id", GTK_STOCK_DIRECTORY,
        "stock-size", GTK_ICON_SIZE_MENU,
        NULL);
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);

    renderer = katze_cell_renderer_combobox_text_new ();
    g_object_set (G_OBJECT (renderer),
        "width-chars", 40,    /* FIXME: figure out a way to define an acceptable string length */
        "ellipsize", PANGO_ELLIPSIZE_END,
        "unfolded-text", _("Select [text]"),
        NULL);
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "text", 0);

    /* read the folders list from the database */
    /* FIXME: this should be a service of midori/midori-bookmarks-db */

    if ((result = sqlite3_prepare_v2 (db, sqlcmd, -1, &statement, NULL)) == SQLITE_OK)
    {
        while ((result = sqlite3_step (statement)) == SQLITE_ROW)
        {
            FolderEntry* folder = g_new (FolderEntry, 1);

            folder->title = g_strdup ((const gchar*)sqlite3_column_text (statement, 0));
            folder->id = sqlite3_column_int64 (statement, 1);
            folder->parentid = sqlite3_column_int64 (statement, 2);

            folders = g_list_append (folders, folder);
        }

        sqlite3_clear_bindings (statement);
        sqlite3_reset (statement);
    }

    /* populate the combo box */
    /* FIXME: here we should have the root bookmark array's name and id, not hard encoded values */

    gtk_tree_store_insert_with_values (model, &tree_iter, NULL, G_MAXINT,
        0, _("Bookmarks"), 1, (gint64)-1, -1);
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &tree_iter);

    current_parentid = -1;
    parent_iter = NULL;
    n = 1;
    while (g_list_first (folders))
    {
        gboolean something_done = FALSE;
        GList* list_iter = g_list_first (folders);

        do
        {
            FolderEntry* folder = list_iter->data;
            const gchar* title = folder->title;
            gint64 id = folder->id;
            gint64 parentid = folder->parentid;

            if (parentid != current_parentid)  /* optimize case of sub-folders of the same parent */
            {
                if (!parentid)
                {
                    /* folder's parent is the stree store root */

                    current_parentid = -1;
                    parent_iter = NULL;
                }
                else if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &tree_iter))
                {
                    if (midori_bookmark_folder_button_reach_parent (
                            GTK_TREE_MODEL (model), &tree_iter, parentid))
                    {
                        /* folder's parent found in the tree store */

                        current_parentid = parentid;
                        stock_parent_iter = tree_iter;
                        parent_iter = &stock_parent_iter;
                    }
                    else
                    {
                        /* folder's parent not found, skip it */

                        list_iter = g_list_next (list_iter);
                        continue;
                    }
                }
                else
                    g_assert_not_reached ();
            }

            /* insert folder in the tree store and remove it from the folders list */

            gtk_tree_store_insert_with_values (model, &tree_iter, parent_iter, G_MAXINT,
                0, title, 1, id, -1);

            if (id == selected_parentid)
                gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &tree_iter);

            n++;

            something_done = TRUE;

            g_free ((gpointer)title);
            folders = g_list_delete_link (folders, list_iter);

            list_iter = g_list_first (folders);
        }
        while (list_iter);

        if (!something_done)  /* avoid infinite loop in case of orphan folders */
            break;
    }

    if (g_list_first (folders))
    {
        GList* iter;
        g_printerr ("midori_bookmark_folder_button_new: orphan folder(s) detected in bookmarks db\n");

        for (iter = g_list_first (folders) ; iter ; iter = g_list_next (iter))
        {
            FolderEntry* folder = iter->data;
            const gchar* title = folder->title;
            gint64 id = folder->id;
            gint64 parentid = folder->parentid;

            g_printerr ("  id=%" G_GINT64_FORMAT ", parentid=%" G_GINT64_FORMAT ", title=%s\n",
                id, parentid, title);
        }

        g_list_free_full (folders, (GDestroyNotify)midori_bookmark_folder_free_folder_entry);
    }

    if (n < 2)
        gtk_widget_set_sensitive (combo, FALSE);

    return combo;
}

static gint64
midori_bookmark_folder_button_get_active (GtkWidget* combo)
{
    gint64 id = -1;
    GtkTreeIter iter;

    g_return_val_if_fail (GTK_IS_COMBO_BOX (combo), 0);

    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
    {
        GtkTreeModel* model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
        gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 1, &id, -1);
    }

    return id;
}

static void
midori_browser_edit_bookmark_title_changed_cb (GtkEntry*      entry,
                                               GtkDialog*     dialog)
{
    const gchar* title = gtk_entry_get_text (entry);
    gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_ACCEPT,
        title != NULL && title[0] != '\0');
}

static void
midori_browser_edit_bookmark_add_speed_dial_cb (GtkWidget* button,
                                                KatzeItem* bookmark)
{
    MidoriBrowser* browser = midori_browser_get_for_widget (button);
    gchar* title = (gchar*)g_object_get_data (G_OBJECT (button), "ENTRY_TITLE");
    gchar* uri = (gchar*)g_object_get_data (G_OBJECT(button), "ENTRY_URI");
    midori_browser_add_speed_dial (browser, uri, title);
    GtkWidget* dialog = gtk_widget_get_toplevel (button);
    gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_DELETE_EVENT);
}

/* Private function, used by MidoriBookmarks and MidoriHistory */
/* static */ gboolean
midori_browser_edit_bookmark_dialog_new (MidoriBrowser* browser,
                                         KatzeItem*     bookmark_or_parent,
                                         gboolean       new_bookmark,
                                         gboolean       is_folder,
                                         GtkWidget*     proxy)
{
    KatzeItem*   bookmark = bookmark_or_parent;
    const gchar* title;
    GtkWidget* dialog;
    GtkWidget* content_area;
    GtkWidget* view;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* label;
    const gchar* value;
    GtkWidget* entry_title;
    GtkWidget* entry_uri;
    GtkWidget* combo_folder;
    GtkWidget* check_toolbar;
    gboolean return_status = FALSE;

    if (is_folder)
        title = new_bookmark ? _("New Folder") : _("Edit Folder");
    else
        title = new_bookmark ? _("New Bookmark") : _("Edit Bookmark");
    #ifdef HAVE_GRANITE
    if (proxy != NULL)
    {
        /* FIXME: granite: should return GtkWidget* like GTK+ */
        dialog = (GtkWidget*)granite_widgets_pop_over_new ();
        granite_widgets_pop_over_move_to_widget (
            GRANITE_WIDGETS_POP_OVER (dialog), proxy, TRUE);
    }
    else
    #endif
    {
        dialog = gtk_dialog_new_with_buttons (title, GTK_WINDOW (browser),
            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR, NULL, NULL);
    }
    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        new_bookmark ? GTK_STOCK_ADD : GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

    if (!is_folder)
        label = gtk_label_new (_("Type a name for this bookmark, and choose where to keep it."));
    else
        label = gtk_label_new (_("Type a name for this folder, and choose where to keep it."));

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 6);
    gtk_box_pack_start (GTK_BOX (content_area), vbox, FALSE, FALSE, 0);
    gtk_window_set_icon_name (GTK_WINDOW (dialog),
        new_bookmark ? GTK_STOCK_ADD : GTK_STOCK_REMOVE);
    if (new_bookmark)
    {
        view = midori_browser_get_current_tab (browser);
        if (is_folder)
        {
            bookmark = (KatzeItem*)katze_array_new (KATZE_TYPE_ARRAY);
            katze_item_set_name (bookmark,
                midori_view_get_display_title (MIDORI_VIEW (view)));
        }else{
                bookmark = NULL;
                bookmark = g_object_new (KATZE_TYPE_ITEM,
                    "uri", midori_view_get_display_uri (MIDORI_VIEW (view)),
                    "name", midori_view_get_display_title (MIDORI_VIEW (view)), NULL);
        }
        katze_item_set_meta_integer (
            bookmark, "parentid",
            (!bookmark_or_parent
                ? 0
                : katze_item_get_meta_integer (bookmark_or_parent, "id")));
    }

    entry_title = gtk_entry_new ();
    gtk_entry_set_activates_default (GTK_ENTRY (entry_title), TRUE);
    value = katze_item_get_name (bookmark);
    gtk_entry_set_text (GTK_ENTRY (entry_title), katze_str_non_null (value));
    midori_browser_edit_bookmark_title_changed_cb (GTK_ENTRY (entry_title),
                                                   GTK_DIALOG (dialog));
    g_signal_connect (entry_title, "changed",
        G_CALLBACK (midori_browser_edit_bookmark_title_changed_cb), dialog);
    gtk_box_pack_start (GTK_BOX (vbox), entry_title, FALSE, FALSE, 0);

    entry_uri = NULL;
    if (!is_folder)
    {
        entry_uri = katze_uri_entry_new (
            gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT));
        gtk_entry_set_activates_default (GTK_ENTRY (entry_uri), TRUE);
	const gchar *uri = katze_item_get_uri (bookmark);
	//added by wangyl 2015.7.9
	if(g_str_has_prefix (uri, "file:"))gtk_entry_set_text (GTK_ENTRY (entry_uri), "about:dial");
	else
        gtk_entry_set_text (GTK_ENTRY (entry_uri), katze_item_get_uri (bookmark));
        gtk_box_pack_start (GTK_BOX (vbox), entry_uri, FALSE, FALSE, 0);
    }

    combo_folder = midori_bookmark_folder_button_new (browser->bookmarks,
        katze_item_get_meta_integer (bookmark, "parentid"));
    gtk_box_pack_start (GTK_BOX (vbox), combo_folder, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    check_toolbar = gtk_check_button_new_with_mnemonic (_("Show in Bookmarks _Bar"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_toolbar), TRUE);
//zgh 20150324        katze_item_get_meta_boolean (bookmark, "toolbar"));
    gtk_box_pack_start (GTK_BOX (hbox), check_toolbar, FALSE, FALSE, 0);
	#if 0//这个按钮功能已经不起作用，所以在界面上除去 by wangyl 2015.8.18
    if (new_bookmark && !is_folder)
    {
        hbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

        label = gtk_button_new_with_mnemonic (_("Add to _Speed Dial"));
        g_object_set_data (G_OBJECT (label), "ENTRY_TITLE", (gpointer)katze_item_get_name (bookmark));
        g_object_set_data (G_OBJECT (label), "ENTRY_URI", (gpointer)katze_item_get_uri (bookmark));
        g_signal_connect (label, "clicked",
            G_CALLBACK (midori_browser_edit_bookmark_add_speed_dial_cb), bookmark);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    }
	#endif

    gtk_widget_show_all (content_area);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    if (midori_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gint64 selected;

        katze_item_set_name (bookmark,
            gtk_entry_get_text (GTK_ENTRY (entry_title)));
        katze_item_set_meta_integer (bookmark, "toolbar",
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_toolbar)));
        if (!is_folder)
            katze_item_set_uri (bookmark,
                gtk_entry_get_text (GTK_ENTRY (entry_uri)));
        //add by luyue 2015/4/29 start
        //创建目录时，第一级目录不能重名
        if(new_bookmark && is_folder)
        {
           GtkTreeModel* model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_folder));
           GtkTreeIter iter;
           char *pitem;
           gtk_tree_model_get_iter_first(model,&iter);
           do
           {
              gtk_tree_model_get (GTK_TREE_MODEL(model),&iter,0,&pitem,-1);//将指定单元格的值传给pitem
              if(strcmp(gtk_entry_get_text (GTK_ENTRY (entry_title)),pitem) == 0)
                 break;
           }while(gtk_tree_model_iter_next(model,&iter));
           if(strcmp(gtk_entry_get_text (GTK_ENTRY (entry_title)),pitem) == 0)
           {
              GtkWidget* dialog1 = gtk_message_dialog_new(NULL,
                                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                                         GTK_MESSAGE_WARNING,
                                                         GTK_BUTTONS_CLOSE,
                                                         "目录已存在");
              g_signal_connect_swapped (dialog1, "response", G_CALLBACK (gtk_widget_destroy), dialog1);
              gtk_widget_show (dialog1);
              gtk_widget_destroy (dialog);
              return FALSE;
           }
        }
        //add end
        selected = midori_bookmark_folder_button_get_active (combo_folder);
        katze_item_set_meta_integer (bookmark, "parentid", selected);

        if (new_bookmark)
            midori_bookmarks_db_add_item (browser->bookmarks, bookmark);
        else
            midori_bookmarks_db_update_item (browser->bookmarks, bookmark);

        return_status = TRUE;
    }

    gtk_widget_destroy (dialog);
    return return_status;
}

//add by lyb start
/* Private function, used by MidoriHistory */
/* static */ gboolean
midori_browser_edit_bookmark_from_history_dialog_new (MidoriBrowser* browser,
                                         KatzeItem*     bookmark_or_parent,
                                         gboolean       new_bookmark,
                                         gboolean       is_folder,
                                         GtkWidget*     proxy)
{
    KatzeItem*   bookmark = bookmark_or_parent;
    const gchar* title;
    GtkWidget* dialog;
    GtkWidget* content_area;
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* label;
    const gchar* value;
    GtkWidget* entry_title;
    GtkWidget* entry_uri;
    GtkWidget* combo_folder;
    GtkWidget* check_toolbar;
    gboolean return_status = FALSE;

    if (is_folder)
        title = new_bookmark ? _("New Folder") : _("Edit Folder");
    else
        title = new_bookmark ? _("New Bookmark") : _("Edit Bookmark");

    dialog = gtk_dialog_new_with_buttons (title, GTK_WINDOW (browser),
        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR, NULL, NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        new_bookmark ? GTK_STOCK_ADD : GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    
    if (!is_folder)
        label = gtk_label_new (_("Type a name for this bookmark, and choose where to keep it."));
    else
        label = gtk_label_new (_("Type a name for this folder, and choose where to keep it."));
         
    vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 6);
    gtk_box_pack_start (GTK_BOX (content_area), vbox, FALSE, FALSE, 0);
    gtk_window_set_icon_name (GTK_WINDOW (dialog),
        new_bookmark ? GTK_STOCK_ADD : GTK_STOCK_REMOVE);


      if (is_folder)
        {
                bookmark = (KatzeItem*)katze_array_new (KATZE_TYPE_ARRAY);
                katze_item_set_name (bookmark,NULL);
                katze_item_set_meta_integer (
                    bookmark, "parentid",
                    (!bookmark_or_parent
                        ? 0
                        : katze_item_get_meta_integer (bookmark_or_parent, "id")));
        }
    
    entry_title = gtk_entry_new ();
    gtk_entry_set_activates_default (GTK_ENTRY (entry_title), TRUE);
    value = katze_item_get_name (bookmark);
    gtk_entry_set_text (GTK_ENTRY (entry_title), katze_str_non_null (value));
    midori_browser_edit_bookmark_title_changed_cb (GTK_ENTRY (entry_title),
                                                   GTK_DIALOG (dialog));
    g_signal_connect (entry_title, "changed",
        G_CALLBACK (midori_browser_edit_bookmark_title_changed_cb), dialog);
    gtk_box_pack_start (GTK_BOX (vbox), entry_title, FALSE, FALSE, 0);
    
    entry_uri = NULL;
    if (!is_folder)
    {
        entry_uri = katze_uri_entry_new (
            gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT));
        gtk_entry_set_activates_default (GTK_ENTRY (entry_uri), TRUE);
	const gchar *uri = katze_item_get_uri (bookmark);
	if(strlen(uri) == 0)gtk_entry_set_text (GTK_ENTRY (entry_uri), "about:dial");
	else
        gtk_entry_set_text (GTK_ENTRY (entry_uri), katze_item_get_uri (bookmark));
        gtk_box_pack_start (GTK_BOX (vbox), entry_uri, FALSE, FALSE, 0);
    }

    combo_folder = midori_bookmark_folder_button_new (browser->bookmarks,
        katze_item_get_meta_integer (bookmark, "parentid"));
    gtk_box_pack_start (GTK_BOX (vbox), combo_folder, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    check_toolbar = gtk_check_button_new_with_mnemonic (_("Show in Bookmarks _Bar"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_toolbar), TRUE);
//zgh 20150324        katze_item_get_meta_boolean (bookmark, "toolbar"));
    gtk_box_pack_start (GTK_BOX (hbox), check_toolbar, FALSE, FALSE, 0);
	#if 0 //这个按钮功能已经不起作用，所以在界面上除去 by wangyl 2015.8.18 
    if (new_bookmark && !is_folder)
    {
        hbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

        label = gtk_button_new_with_mnemonic (_("Add to _Speed Dial"));
        g_object_set_data (G_OBJECT (label), "ENTRY_TITLE", (gpointer)katze_item_get_name (bookmark));
        g_object_set_data (G_OBJECT (label), "ENTRY_URI", (gpointer)katze_item_get_uri (bookmark));
        g_signal_connect (label, "clicked",
            G_CALLBACK (midori_browser_edit_bookmark_add_speed_dial_cb), bookmark);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    }
    #endif 
    gtk_widget_show_all (content_area);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    if (midori_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gint64 selected;

        katze_item_set_name (bookmark,
            gtk_entry_get_text (GTK_ENTRY (entry_title)));
        katze_item_set_meta_integer (bookmark, "toolbar",
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_toolbar)));
        if (!is_folder)
            katze_item_set_uri (bookmark,
                gtk_entry_get_text (GTK_ENTRY (entry_uri)));
        //创建目录时，第一级目录不能重名
        if(is_folder)
        {   
           GtkTreeModel* model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_folder));
           GtkTreeIter iter;
           char *pitem;
           gtk_tree_model_get_iter_first(model,&iter);
           do
           {
              gtk_tree_model_get (GTK_TREE_MODEL(model),&iter,0,&pitem,-1);//将指定单元格的值传给pitem
              if(strcmp(gtk_entry_get_text (GTK_ENTRY (entry_title)),pitem) == 0)
                 break;
           }while(gtk_tree_model_iter_next(model,&iter));
           if(strcmp(gtk_entry_get_text (GTK_ENTRY (entry_title)),pitem) == 0)
           {
              GtkWidget* dialog1 = gtk_message_dialog_new(NULL,
                                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                                         GTK_MESSAGE_WARNING,
                                                         GTK_BUTTONS_CLOSE,
                                                         "目录已存在");
              g_signal_connect_swapped (dialog1, "response", G_CALLBACK (gtk_widget_destroy), dialog1);
              gtk_widget_show (dialog1);
              gtk_widget_destroy (dialog);
              return FALSE;
           }
        }
        selected = midori_bookmark_folder_button_get_active (combo_folder);
        katze_item_set_meta_integer (bookmark, "parentid", selected);

        if (new_bookmark)
            midori_bookmarks_db_add_item (browser->bookmarks, bookmark);
        else
            midori_bookmarks_db_update_item (browser->bookmarks, bookmark);

        return_status = TRUE;
    }

    gtk_widget_destroy (dialog);
    return return_status;
}                                         

//end

void
midori_browser_save_uri (MidoriBrowser* browser,
                         MidoriView*    view,
                         const gchar*   uri,
                         gchar*         title)
{
    static gchar* last_dir = NULL;
    GtkWidget* dialog;
    if(!title)
       title = midori_view_get_display_title (view);
    gchar* filename = NULL;
    dialog = (GtkWidget*)midori_file_chooser_dialog_new (_("Save file as"),
        GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_SAVE);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

    ///文件选择类型过滤

    //保存为.html文件，只保存html
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name (filter, _("网页，仅HTML(.html)"));
		 gtk_file_filter_add_pattern(filter,"*.html");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog),filter);

    //后缀为.mht文件，可保存网页的全部内容
    if(!strstr(title,"index"))
    {
       filter = gtk_file_filter_new();
       gtk_file_filter_set_name (filter, _("网页，全部(.mht)"));
       gtk_file_filter_add_pattern(filter,"*.mht");
       gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog),filter);
    }
    if (uri == NULL)
        uri = midori_view_get_display_uri (view);

    if (last_dir && *last_dir)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), last_dir);
    else
    {
        gchar* dirname = midori_uri_get_folder (uri);
        if (dirname == NULL || strstr(dirname,"speedial"))
        {
              gchar* home = getenv("HOME");
              gchar download_setting_file[256];
              g_sprintf(download_setting_file, "%s/.config/cdosbrowser/appex_config/settings.ini", home);
              FILE *fp = fopen(download_setting_file,"r");
              if(!fp)
              {
                 dirname = (char *)malloc(strlen(home)+1+strlen("/下载"));
                 strcpy(dirname,home);
                 strcat(dirname,"/下载");
              }
              else
              {
                 char arr[256];
                 while ((fgets (arr, sizeof(arr)-1, fp)) != NULL)
                 {
                    if (strncmp(arr,"path",4))
                       continue;
                    else
                    {
                       dirname = (char *)malloc(strlen(arr)-4);
                       strncpy(dirname,arr+5,strlen(arr)-6);
                       dirname[strlen(arr)-5]='/';
                       dirname[strlen(arr)-6]='\0';
                       break;
                    }
                 }
                 if(!dirname)
                 {
                    dirname = (char *)malloc(strlen(home)+1+strlen("/下载"));
                    strcpy(dirname,home);
                    strcat(dirname,"/下载");
                 } 
                 fclose(fp);
              }
        }
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirname);
        g_free (dirname);
    }
    if(title)
    {
       //add by luyue 2015/8/19 start
       //gtk不支持非utf-8显示
       filename = g_locale_to_utf8(title,-1,0,0,0);
       //add end
    }
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filename);

    if (midori_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
       GtkFileFilter *filtertwo = gtk_file_chooser_get_filter( GTK_FILE_CHOOSER (dialog));
       const gchar * filternamee =  gtk_file_filter_get_name(filtertwo);
       // ZRL 实现网页的保存功能。
       char *filename1 = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
       if(NULL == filename1)return;

       if(uri != NULL)
       {
          gchar* suggested_filename = NULL;
          //图片的格式保存到html中
          if(strstr(filename1,"index"))
          {
             char *save_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
             suggested_filename = g_strconcat (save_filename, ".html", NULL);
             FILE *fp=fopen(suggested_filename,"w");
             char *context = (char *)malloc(strlen(uri)+48);
             sprintf(context,"<html><body><img src=%s></body></html>",uri);
             fprintf(fp,"%s\n",context);
             free(context);
             context = NULL;
             fclose(fp);
          }
          else
          {
             if(0 == strcmp(filternamee, _("网页，全部(.mht)")))
                suggested_filename = g_strconcat (filename1, ".mht", NULL);
             else if(0 == strcmp(filternamee, _("网页，仅HTML(.html)")))
                suggested_filename = g_strconcat (filename1, ".html", NULL);
             midori_view_save_source (view, uri, suggested_filename, false); 
          }
          g_free (suggested_filename);
       }
       g_free (filename1);
       katze_assign (last_dir,gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog)));
    }
    gtk_widget_destroy (dialog);
}

static void
midori_browser_speed_dial_refresh1_cb (MidoriSpeedDial* dial,
                                      MidoriBrowser*   browser)
{
   MidoriView *view1;
   GList      *browsers;
   MidoriView *view = MIDORI_VIEW (midori_browser_get_current_tab (browser));
   MidoriApp  *app = midori_app_get_default();	

   browsers = midori_app_get_browsers(app);
   for(; browsers; browsers=g_list_next(browsers))
   {
      GList* tabs = midori_browser_get_tabs (MIDORI_BROWSER(browsers->data));
      for (; tabs != NULL; tabs = g_list_next (tabs))
      {
         view1 = MIDORI_VIEW(tabs->data);
	 if(view == view1)continue;
	 if (strstr(midori_tab_get_uri (tabs->data), "speeddial-head.html")||!strcmp (midori_tab_get_uri (tabs->data), "about:dial"))
	    midori_view_reload (tabs->data, FALSE);
      }
      g_list_free (tabs);
   }
   g_list_free(browsers);
}

static void
midori_browser_speed_dial_refresh_cb (MidoriSpeedDial* dial,
                                      MidoriBrowser*   browser)
{
   gchar image_adr[1024]={0};
   GList   	*browsers;
   MidoriView  *view1,*view2;
   bool is_view_exist =false;
   //当有多个speeddial页时，在不同的页面上都添加缩略图，保证各个页面都能显示添加的缩略图 start	
   MidoriView*   view = (MidoriView*)atoi(midori_speed_dial_get_viewadr(dial));
   GList* tabs1 = midori_browser_get_tabs (browser);
   for (; tabs1 != NULL; tabs1 = g_list_next (tabs1))
   {
      view2 = MIDORI_VIEW(tabs1->data);
      if(view == view2)is_view_exist = true;
   }
   if(is_view_exist == false)return; 
   // end
    g_sprintf(image_adr,"refresh('%s','%s','%s');",midori_speed_dial_get_imageid(dial),midori_speed_dial_get_imagename(dial),
             midori_speed_dial_get_imagetitle(dial));
   webkit_web_view_run_javascript(WEBKIT_WEB_VIEW (midori_view_get_web_view(view)),image_adr, NULL, NULL, NULL);
   MidoriApp* app = midori_app_get_default();	
   browsers=midori_app_get_browsers(app);
   for(; browsers; browsers=g_list_next(browsers))
   {
      GList* tabs = midori_browser_get_tabs (MIDORI_BROWSER(browsers->data));
      for (; tabs != NULL; tabs = g_list_next (tabs)){
         view1 = MIDORI_VIEW(tabs->data);
	 if(view == view1)continue;
	 if (strstr (midori_tab_get_uri (tabs->data), "speeddial-head.html")||!strcmp (midori_tab_get_uri (tabs->data), "about:dial"))
	    midori_view_reload (tabs->data, FALSE);
      }
      g_list_free (tabs);
   }
   g_list_free(browsers);
}

static void
midori_browser_add_speed_dial (MidoriBrowser* browser, gchar* uri, gchar* title)
{
    if (!uri || !title)
    {
    GtkWidget* view = midori_browser_get_current_tab (browser);
    midori_speed_dial_add (browser->dial,
        midori_view_get_display_uri (MIDORI_VIEW (view)),
        midori_view_get_display_title (MIDORI_VIEW (view)), NULL);
    }
    else
    {
        midori_speed_dial_add (browser->dial, uri, title, NULL);
    }
}

static gboolean
midori_browser_tab_leave_notify_event_cb (GtkWidget*        widget,
                                          GdkEventCrossing* event,
                                          MidoriBrowser*    browser)
{
    _midori_browser_set_statusbar_text (browser, MIDORI_VIEW (widget), NULL);
    return TRUE;
}

static void
midori_view_destroy_cb (GtkWidget*     view,
                        MidoriBrowser* browser)
{
    gchar *uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW (midori_view_get_web_view(view)));
    gchar adr_message[80] = {0};
    if(strstr(uri,"speeddial-head.html"))
    {
       MidoriSpeedDial* dial = katze_object_get_object (browser, "speed-dial");
       g_sprintf(adr_message,"speed_dial-save-stop %d",view);
       midori_speed_dial_save_message (dial, adr_message, NULL);
    }
    if (browser->proxy_array)
    {
        KatzeItem* item = midori_view_get_proxy_item (MIDORI_VIEW (view));
        if (katze_array_get_item_index (browser->proxy_array, item) != -1
         && !midori_tab_is_blank (MIDORI_TAB (view)))
        {
            if (browser->trash)
            {
                katze_array_add_item (browser->trash, item);
            }
            midori_browser_update_history (item, "website", "leave");
        }
        midori_browser_disconnect_tab (browser, MIDORI_VIEW (view));
        g_signal_emit (browser, signals[REMOVE_TAB], 0, view);
    }
}

static void
midori_view_attach_inspector_cb (GtkWidget*     view,
                                 GtkWidget*     inspector_view,
                                 MidoriBrowser* browser)
{
    GtkWidget* toplevel = gtk_widget_get_toplevel (inspector_view);
    GtkWidget* scrolled = gtk_widget_get_parent (browser->inspector_view);
    if (browser->inspector_view == inspector_view)
        return;

    gtk_widget_hide (toplevel);
    gtk_widget_destroy (browser->inspector_view);
    gtk_widget_reparent (inspector_view, scrolled);
    gtk_widget_show_all (browser->inspector);
    browser->inspector_view = inspector_view;
    gtk_widget_destroy (toplevel);
    if (!katze_object_get_boolean (browser->settings, "last-inspector-attached"))
        g_object_set (browser->settings, "last-inspector-attached", TRUE, NULL);
}

static void
midori_view_detach_inspector_cb (GtkWidget*     view,
                                 GtkWidget*     inspector_view,
                                 MidoriBrowser* browser)
{
    GtkWidget* scrolled = gtk_widget_get_parent (GTK_WIDGET (inspector_view));
    GtkWidget* paned = gtk_widget_get_parent (scrolled);
    browser->inspector_view = gtk_viewport_new (NULL, NULL);
    gtk_container_remove (GTK_CONTAINER (scrolled), GTK_WIDGET (inspector_view));
    gtk_container_add (GTK_CONTAINER (scrolled), browser->inspector_view);
    gtk_widget_hide (paned);
    if (katze_object_get_boolean (browser->settings, "last-inspector-attached"))
        g_object_set (browser->settings, "last-inspector-attached", FALSE, NULL);
}

static gboolean
midori_browser_notify_new_tab_timeout_cb (MidoriBrowser *browser)
{
    #ifndef G_OS_WIN32
    gtk_window_set_opacity (GTK_WINDOW (browser), 1);
    #endif
    return G_SOURCE_REMOVE;
}

static void
midori_browser_notify_new_tab (MidoriBrowser* browser)
{
    if (katze_object_get_boolean (browser->settings, "flash-window-on-new-bg-tabs"))
    {
        #ifndef G_OS_WIN32
        gtk_window_set_opacity (GTK_WINDOW (browser), 0.8);
        #endif
        midori_timeout_add (100,
            (GSourceFunc) midori_browser_notify_new_tab_timeout_cb, browser, NULL);
    }
}

static bool
midori_view_forward_external (GtkWidget*    view,
                              const gchar*  uri,
                              MidoriNewView where)
{
    if (midori_paths_get_runtime_mode () == MIDORI_RUNTIME_MODE_APP)
    {
        gboolean handled = FALSE;
        g_signal_emit_by_name (view, "open-uri", uri, &handled);
        return handled;
    }
    else if (midori_paths_get_runtime_mode () == MIDORI_RUNTIME_MODE_PRIVATE)
    {
        if (where == MIDORI_NEW_VIEW_WINDOW)
        {
            sokoke_spawn_app (uri, TRUE);
            return TRUE;
        }
    }
    return FALSE;
}

static void
midori_view_new_tab_cb (GtkWidget*     view,
                        const gchar*   uri,
                        gboolean       background,
                        MidoriBrowser* browser)
{
    if (midori_view_forward_external (view, uri, MIDORI_NEW_VIEW_TAB))
        return;

    GtkWidget* new_view = midori_browser_add_uri (browser, uri);

    if (!background)
        midori_browser_set_current_tab (browser, new_view);
    else
        midori_browser_notify_new_tab (browser);
}

static void
midori_view_new_window_cb (GtkWidget*     view,
                           const gchar*   uri,
                           MidoriBrowser* browser)
{
    if (midori_view_forward_external (
        view ? view : midori_browser_get_current_tab (browser),
        uri, MIDORI_NEW_VIEW_WINDOW))
        return;

    MidoriBrowser* new_browser;
    g_signal_emit (browser, signals[NEW_WINDOW], 0, NULL, &new_browser);
    g_assert (new_browser != NULL);
    midori_view_new_tab_cb (view, uri, FALSE, new_browser);
}

#if TRACK_LOCATION_TAB_ICON //lxx, 20150202
static void
midori_track_location_cb(GtkWidget*     view,
			 gboolean       bTrackLocation,
                         MidoriBrowser* browser)
{
   GtkWidget* tab = midori_browser_get_current_tab(browser);
   GtkWidget *label = gtk_notebook_get_tab_label(MIDORI_NOTEBOOK (browser->notebook)->notebook, tab);
   GtkButton* loc_simbo =  midori_tally_get_loc_simbo((MidoriTally*)label);
   GtkWidget *icon;

   switch (bTrackLocation) 
   {
      case 1:
         midori_tally_set_track_location((MidoriTally*)label, true);
	 icon = gtk_image_new_from_icon_name(STOCK_ALLOW_LOCATION, GTK_ICON_SIZE_MENU);
	 break;
      default:
	 midori_tally_set_track_location((MidoriTally*)label, false);
	 icon = gtk_image_new_from_icon_name(STOCK_BLOCK_LOCATION, GTK_ICON_SIZE_MENU);
	 break;
   }
   gtk_button_set_image (loc_simbo, icon);
   gtk_widget_show(GTK_WIDGET(loc_simbo));
}

//lxx, 20150204
static void
midori_start_load_hide_location_icon_cb(GtkWidget*     view,
					MidoriBrowser* browser)
{
   g_return_if_fail (MIDORI_IS_VIEW (view));

   GtkWidget* tab = midori_browser_get_current_tab(browser);
   GtkWidget *label = gtk_notebook_get_tab_label(MIDORI_NOTEBOOK (browser->notebook)->notebook, tab);
   GtkButton* loc_simbo =  midori_tally_get_loc_simbo((MidoriTally*)label);
   gtk_widget_hide(GTK_WIDGET(loc_simbo));
}

static void
midori_start_load_hide_block_javascript_window_icon_cb(GtkWidget*     view,
						       MidoriBrowser* browser)
{
   g_return_if_fail (MIDORI_IS_VIEW (view));

   GtkWidget* tab = midori_browser_get_current_tab(browser);
   GtkWidget *label = gtk_notebook_get_tab_label(MIDORI_NOTEBOOK (browser->notebook)->notebook, tab);
   GtkButton* block_simbo =  midori_tally_get_block_simbo((MidoriTally*)label);
   gtk_widget_hide(GTK_WIDGET(block_simbo));
}

static void
midori_javascript_popup_window_icon_cb(GtkWidget*     view,																		       const gchar*   str,
				       MidoriBrowser* browser)
{
   GtkWidget* tab = midori_browser_get_current_tab(browser);
   GtkWidget *label = gtk_notebook_get_tab_label(MIDORI_NOTEBOOK (browser->notebook)->notebook, tab);
   GtkButton* block_simbo =  midori_tally_get_block_simbo((MidoriTally*)label);
   midori_tally_set_block_uri_title ((MidoriTally*)label, str);
   GtkWidget *icon = gtk_image_new_from_icon_name(STOCK_BLOCK_POPUPS, GTK_ICON_SIZE_MENU);
   gtk_button_set_image (block_simbo, icon);
   gtk_widget_show(GTK_WIDGET(block_simbo));
}
#endif

static void
midori_view_new_view_cb (GtkWidget*     view,
                         GtkWidget*     new_view,
                         MidoriNewView  where,
                         gboolean       user_initiated,
                         MidoriBrowser* browser)
{
    if (midori_tab_get_is_dialog (MIDORI_TAB (view)))
    {
        /* Dialog: URL, no toolbars, no tabs */
        MidoriBrowser* new_browser;
        g_signal_emit (browser, signals[NEW_WINDOW], 0, NULL, &new_browser);
        g_assert (new_browser != NULL);
        gtk_window_set_transient_for (GTK_WINDOW (new_browser), GTK_WINDOW (browser));
        gtk_window_set_destroy_with_parent (GTK_WINDOW (new_browser), TRUE);
        g_object_set (new_browser,
                      "show-tabs", FALSE,
                      NULL);
        sokoke_widget_set_visible (new_browser->menubar, FALSE);
        sokoke_widget_set_visible (new_browser->bookmarkbar, FALSE);
        sokoke_widget_set_visible (new_browser->statusbar, FALSE);
        _action_set_visible (new_browser, "CompactMenu", FALSE);
        _action_set_visible (new_browser, "Preferences", TRUE); //zgh
        _midori_browser_set_toolbar_items (new_browser, "Location");
        sokoke_widget_set_visible (new_browser->panel, FALSE);
        midori_browser_add_tab (new_browser, new_view);
        midori_browser_set_current_tab (new_browser, new_view);
        return;
    }

    if (midori_view_forward_external (new_view,
        katze_item_get_uri (midori_view_get_proxy_item (MIDORI_VIEW (new_view))),
        where))
        return;

    if (where == MIDORI_NEW_VIEW_WINDOW)
    {
        MidoriBrowser* new_browser;
        g_signal_emit (browser, signals[NEW_WINDOW], 0, NULL, &new_browser);
        g_assert (new_browser != NULL);
        midori_browser_add_tab (new_browser, new_view);
        midori_browser_set_current_tab (new_browser, new_view);

        // ZRL 以新窗口打开时主动设置新视图的uri，激活加载流程
        const gchar* dest_uri = g_object_get_data (G_OBJECT (new_view), "destination-uri");
        midori_view_set_uri(MIDORI_VIEW (new_view), dest_uri);
    }
    else {
        midori_browser_notify_new_tab (browser);
        // ZRL 以新Tab打开时步骤：midori_browser_add_tab, midori_browser_set_current_tab, midori_view_set_uri
        const gchar* dest_uri = g_object_get_data (G_OBJECT (new_view), "destination-uri");
        midori_browser_add_tab (browser, new_view);
        if (where != MIDORI_NEW_VIEW_BACKGROUND)
            midori_browser_set_current_tab (browser, new_view);
        midori_view_set_uri(MIDORI_VIEW (new_view), dest_uri);
    }

    if (!user_initiated)
    {
        GdkWindow* window = gtk_widget_get_window (GTK_WIDGET (browser));
        GdkWindowState state = gdk_window_get_state (window);
        if ((state | GDK_WINDOW_STATE_MAXIMIZED)
         || (state | GDK_WINDOW_STATE_FULLSCREEN))
        {
            if (where == MIDORI_NEW_VIEW_WINDOW)
                g_signal_emit (browser, signals[SEND_NOTIFICATION], 0,
                    _("New Window"), _("A new window has been opened"));
            else if (!browser->show_tabs)
                g_signal_emit (browser, signals[SEND_NOTIFICATION], 0,
                    _("New Tab"), _("A new tab has been opened"));
        }
    }
}

static void
midori_view_search_text_cb (GtkWidget*     view,
                            gboolean       found,
                            gchar*         typing,
                            MidoriBrowser* browser)
{
    midori_findbar_search_text (MIDORI_FINDBAR (browser->find), view, found, typing);
}

gint
midori_browser_get_n_pages (MidoriBrowser* browser)
{
    return midori_notebook_get_count (MIDORI_NOTEBOOK (browser->notebook));
}

static void
_midori_browser_remove_tab (MidoriBrowser* browser,
                            GtkWidget*     widget)
{
    gtk_widget_destroy (widget);
}

static void
midori_browser_connect_tab (MidoriBrowser* browser,
                            GtkWidget*     view)
{
    KatzeItem* item = midori_view_get_proxy_item (MIDORI_VIEW (view));
    katze_array_add_item (browser->proxy_array, item);

    g_object_connect (view,
                      "signal::notify::icon",
                      midori_view_notify_icon_cb, browser,
                      "signal::notify::load-status",
                      midori_view_notify_load_status_cb, browser,
                      "signal::notify::progress",
                      midori_view_notify_progress_cb, browser,
                      "signal::notify::uri",
                      midori_view_notify_uri_cb, browser,
                      "signal::notify::title",
                      midori_view_notify_title_cb, browser,
                      "signal::notify::zoom-level",
                      midori_view_notify_zoom_level_cb, browser,
                      "signal::notify::statusbar-text",
                      midori_view_notify_statusbar_text_cb, browser,
                      "signal::attach-inspector",
                      midori_view_attach_inspector_cb, browser,
                      "signal::detach-inspector",
                      midori_view_detach_inspector_cb, browser,
                      "signal::new-tab",
                      midori_view_new_tab_cb, browser,
                      "signal::new-window",
                      midori_view_new_window_cb, browser,
                      "signal::new-view",
                      midori_view_new_view_cb, browser,
                      "signal::forward-url",                //add by luyue 2015/3/16
                      midori_view_forward_url_cb, browser,       //add by luyue 2015/3/16
#if TRACK_LOCATION_TAB_ICON //lxx, 20150202
                      "signal::track-location",
                      midori_track_location_cb, browser,
                      "signal::start-load",
                      midori_start_load_hide_location_icon_cb, browser,
                      "signal::start-load-hide-block-javascript-window-icon",
                      midori_start_load_hide_block_javascript_window_icon_cb, browser,
                      "signal::javascript-popup-window-ui-message",
                      midori_javascript_popup_window_icon_cb, browser,
#endif
                      "signal::search-text",
                      midori_view_search_text_cb, browser,
                      "signal::leave-notify-event",
                      midori_browser_tab_leave_notify_event_cb, browser,
                      "signal::destroy",
                      midori_view_destroy_cb, browser,
                      NULL);
}

static void
midori_browser_disconnect_tab (MidoriBrowser* browser,
                               MidoriView*    view)
{
    KatzeItem* item = midori_view_get_proxy_item (view);
    katze_array_remove_item (browser->proxy_array, item);

    /* We don't ever want to be in a situation with no tabs,
       so just create an empty one if the last one is closed.
       The only exception is when we are closing the window,
       which is indicated by the proxy array having been unset. */
    if (katze_array_is_empty (browser->proxy_array))
    {
        midori_browser_add_uri (browser, "about:new");
        midori_browser_set_current_page (browser, 0);
        MidoriApp *app = midori_app_get_default();
        if (midori_app_get_browsers_num(app) < 2)
           system("/usr/local/libexec/cdosbrowser/cdosbrowser_download quit &");
	g_idle_add (gtk_widget_destroy, GTK_WIDGET (browser));//added by wangyl 2015.7.3
    }

    _midori_browser_update_actions (browser);

    g_object_disconnect (view,
                         "any_signal",
                         midori_view_notify_icon_cb, browser,
                         "any_signal",
                         midori_view_notify_load_status_cb, browser,
                         "any_signal",
                         midori_view_notify_progress_cb, browser,
                         "any_signal",
                         midori_view_notify_uri_cb, browser,
                         "any_signal",
                         midori_view_notify_title_cb, browser,
                         "any_signal",
                         midori_view_notify_zoom_level_cb, browser,
                         "any_signal",
                         midori_view_notify_statusbar_text_cb, browser,
                         "any_signal::attach-inspector",
                         midori_view_attach_inspector_cb, browser,
                         "any_signal::detach-inspector",
                         midori_view_detach_inspector_cb, browser,
                         "any_signal::new-tab",
                         midori_view_new_tab_cb, browser,
                         "any_signal::new-window",
                         midori_view_new_window_cb, browser,
                         "any_signal::new-view",
                         midori_view_new_view_cb, browser,
                         "any_signal::search-text",
                         midori_view_search_text_cb, browser,
                         "any_signal::leave-notify-event",
                         midori_browser_tab_leave_notify_event_cb, browser,
                         NULL);
}

//lxx add +, 20141229
static gint 
_midori_browser_get_num_of_tabs()
{
   MidoriApp *app;
   GList  *browsers;
   gint tabNum = 0;
   int i = 0;	

   app = midori_app_get_default();	
   browsers = midori_app_get_browsers(app);
   for(; browsers; browsers=g_list_next(browsers))
   {
      i++;
      tabNum += midori_browser_get_n_pages(MIDORI_BROWSER(browsers->data));
   }
   g_list_free(browsers);
   return tabNum;
}

static gboolean 
_midori_show_much_tab_warning(MidoriBrowser* browser)
{
   gboolean bvalue = 0, bshow = 0;
	
   g_object_get(browser->settings, "much-tab-warning", &bvalue, NULL);
   if(bvalue)
   {
      gint itabNum = _midori_browser_get_num_of_tabs();
      if(itabNum > 30)
         bshow = 1;
   }
   return bshow;
}

static void 
_midori_much_tab_warning_bar(GtkWidget*     view)
{
   GtkWidget *info_bar;
   GtkWidget* content_area;
   GtkWidget* message_label;

   info_bar = gtk_info_bar_new();
   message_label = gtk_label_new ("您开启了多个标签页，已经导致浏览器速度变慢了，请您注意！");
   gtk_widget_show(message_label);
   content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar));
   gtk_container_add (GTK_CONTAINER (content_area), message_label);
   gtk_info_bar_add_button (GTK_INFO_BAR (info_bar), _("_OK"), GTK_RESPONSE_OK);
   g_signal_connect (info_bar, "response", G_CALLBACK (gtk_widget_hide), NULL);
   gtk_widget_show(info_bar);
   gtk_box_pack_end (GTK_BOX (view), GTK_WIDGET (info_bar), FALSE, FALSE, 0);
}
//lxx add -, 20141229

static void
_midori_browser_add_tab (MidoriBrowser* browser,
                         GtkWidget*     view)
{
    KatzeItem* item = midori_view_get_proxy_item (MIDORI_VIEW (view));
    guint n;

    midori_browser_connect_tab (browser, view);

    if (!katze_item_get_meta_boolean (item, "append") &&
        katze_object_get_boolean (browser->settings, "open-tabs-next-to-current"))
    {
        n = midori_browser_get_current_page (browser) + 1;
        katze_array_move_item (browser->proxy_array, item, n);
    }
    else
        n = -1;
    katze_item_set_meta_integer (item, "append", -1);
    gint  lastPageIndex = gtk_notebook_get_n_pages(MIDORI_NOTEBOOK(browser->notebook)->notebook); 
    if(MIDORI_NOTEBOOK(browser->notebook)->btn_end ==0)
    {
        midori_notebook_insert (MIDORI_NOTEBOOK (browser->notebook), MIDORI_TAB (view), lastPageIndex-1);
        midori_browser_set_current_page(browser, lastPageIndex-1);
    }
    else
    {
        midori_notebook_insert (MIDORI_NOTEBOOK (browser->notebook), MIDORI_TAB (view), lastPageIndex);
        midori_browser_set_current_page(browser, lastPageIndex);
    }
    

//lxx add for much tab warning+
    if(_midori_show_much_tab_warning(browser))
    {
	_midori_much_tab_warning_bar(view);
	g_object_set(browser->settings, "much-tab-warning", 0, NULL);
    }
//lxx add for much tab warning-
}

static void
_midori_browser_quit (MidoriBrowser* browser)
{
    /* Nothing to do */
}

static void
_update_tooltip_if_changed (GtkAction* action,
                            const gchar* text)
{
    gchar *old;
    g_object_get (action, "tooltip", &old, NULL);
    if (g_strcmp0(old, text)) {
        g_object_set (action,
                      "tooltip", text, NULL);
    }
    g_free (old);
}

static void
_update_reload_tooltip (GtkWidget*   widget,
                        GdkEventKey* event,
                        gboolean released)
{
    MidoriBrowser* browser = MIDORI_BROWSER (widget);

    /* Update the reload/stop tooltip in case we are holding the hard refresh modifiers*/
    GtkAction *reload_stop = _action_by_name (browser, "ReloadStop");
    GtkAction *reload = _action_by_name (browser, "Reload");
    GdkModifierType mask;
    gdk_window_get_pointer (gtk_widget_get_window (widget), NULL, NULL, &mask);
    const gchar *target;

    if ( mask & GDK_SHIFT_MASK)
    {
        target = _("Reload page without caching");
    }
    else
    {
        target = _("Reload the current page");
    }
    _update_tooltip_if_changed (reload_stop, target);
    _update_tooltip_if_changed (reload, target);
}

static gboolean
midori_browser_key_release_event (GtkWidget*   widget,
                                  GdkEventKey* event)
{
    _update_reload_tooltip (widget, event, TRUE);
    return FALSE;
}

static gboolean
midori_browser_key_press_event (GtkWidget*   widget,
                                GdkEventKey* event)
{
    GtkWindow* window = GTK_WINDOW (widget);
    MidoriBrowser* browser = MIDORI_BROWSER (widget);
    GtkWidgetClass* widget_class;
    guint clean_state;

    _update_reload_tooltip(widget, event, FALSE);
    /* Interpret Ctrl(+Shift)+Tab as tab switching for compatibility */
    if (midori_browser_get_nth_tab (browser, 1) != NULL
     && event->keyval == GDK_KEY_Tab
     && (event->state & GDK_CONTROL_MASK))
    {
        midori_browser_activate_action (browser, "TabNext");
        return TRUE;
    }
    else if (event->keyval == GDK_KEY_ISO_Left_Tab
     && (event->state & GDK_CONTROL_MASK)
     && (event->state & GDK_SHIFT_MASK))
    {
        midori_browser_activate_action (browser, "TabPrevious");
        return TRUE;
    }
    /* Interpret Ctrl+= as Zoom In for compatibility */
    else if ((event->keyval == GDK_KEY_KP_Equal || event->keyval == GDK_KEY_equal)
          && (event->state & GDK_CONTROL_MASK))
    {
        midori_browser_activate_action (browser, "ZoomIn");
        return TRUE;
    }
    /* Interpret F5 as reloading for compatibility */
    else if (event->keyval == GDK_KEY_F5)
    {
        midori_browser_activate_action (browser, "Reload");
        return TRUE;
    }

    if (event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))
        if (sokoke_window_activate_key (window, event))
            return TRUE;

    clean_state = event->state & gtk_accelerator_get_default_mod_mask();
    if (!clean_state && gtk_window_propagate_key_event (window, event))
        return TRUE;

    if (!(event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
        if (sokoke_window_activate_key (window, event))
            return TRUE;

    if (event->state && gtk_window_propagate_key_event (window, event))
        return TRUE;

    /* Interpret (Shift+)Backspace as going back (forward) for compatibility */
    if ((event->keyval == GDK_KEY_BackSpace)
     && (event->state & GDK_SHIFT_MASK))
    {
        midori_browser_activate_action (browser, "Forward");
        return TRUE;
    }
    else if (event->keyval == GDK_KEY_BackSpace)
    {
        midori_browser_activate_action (browser, "Back");
        return TRUE;
    }

    widget_class = g_type_class_peek_static (g_type_parent (GTK_TYPE_WINDOW));
    return widget_class->key_press_event (widget, event);
}

static void
midori_browser_class_init (MidoriBrowserClass* class)
{
#ifdef APP_LEVEL_TIME
printf("create browser start time = %lld\n",g_get_real_time());
#endif
    GtkWidgetClass* gtkwidget_class;
    GObjectClass* gobject_class;
    GParamFlags flags;

    /**
     * MidoriBrowser::new-window:
     * @browser: the object on which the signal is emitted
     * @window: a new browser window, or %NULL
     *
     * Emitted when a new browser window was created.
     *
     * Note: Before 0.1.7 the second argument was an URI string.
     *
     * Note: Since 0.2.1 the return value is a #MidoriBrowser
     *
     * Return value: a new #MidoriBrowser
     */
    signals[NEW_WINDOW] = g_signal_new (
        "new-window",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        G_STRUCT_OFFSET (MidoriBrowserClass, new_window),
        0,
        NULL,
        midori_cclosure_marshal_OBJECT__OBJECT,
        MIDORI_TYPE_BROWSER, 1,
        MIDORI_TYPE_BROWSER);

    signals[ADD_TAB] = g_signal_new (
        "add-tab",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET (MidoriBrowserClass, add_tab),
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        GTK_TYPE_WIDGET);

    signals[REMOVE_TAB] = g_signal_new (
        "remove-tab",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET (MidoriBrowserClass, remove_tab),
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        GTK_TYPE_WIDGET);

    /**
     * MidoriBrowser::move-tab:
     * @browser: the object on which the signal is emitted
     * @notebook: the notebook containing the tabs
     * @cur_pos: the current position of the tab
     * @new_pos: the new position of the tab
     *
     * Emitted when a tab is moved.
     *
     * Since: 0.3.3
     */
     signals[MOVE_TAB] = g_signal_new (
        "move-tab",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        midori_cclosure_marshal_VOID__OBJECT_INT_INT,
        G_TYPE_NONE, 3,
        GTK_TYPE_WIDGET, G_TYPE_INT, G_TYPE_INT);

    /**
     * MidoriBrowser::switch-tab:
     * @browser: the object on which the signal is emitted
     * @old_view: the previous tab
     * @new_view: the new tab
     *
     * Emitted when a tab is switched.
     * There's no guarantee what the current tab is.
     *
     * Since: 0.4.7
     */
     signals[SWITCH_TAB] = g_signal_new (
        "switch-tab",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        0,
        0,
        NULL,
        midori_cclosure_marshal_VOID__OBJECT_OBJECT,
        G_TYPE_NONE, 2,
        G_TYPE_OBJECT, G_TYPE_OBJECT);

    signals[ACTIVATE_ACTION] = g_signal_new (
        "activate-action",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET (MidoriBrowserClass, activate_action),
        0,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    /**
     * MidoriBrowser::send-notification:
     * @browser: the object on which the signal is emitted
     * @title: the title for the notification
     * @message: the message for the notification
     *
     * Emitted when a browser wants to display a notification message,
     * e.g. when a download has been completed or a new tab was opened.
     *
     * Since: 0.1.7
     */
    signals[SEND_NOTIFICATION] = g_signal_new (
        "send-notification",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        midori_cclosure_marshal_VOID__STRING_STRING,
        G_TYPE_NONE, 2,
        G_TYPE_STRING,
        G_TYPE_STRING);
	/** 
     * MidoriBrowser::send-notification:
     * @browser: the object on which the signal is emitted
     * @title: the title for the notification
     * @message: the message for the pages
     *
     * Emitted when  browser The browser is first opened and all pages are loaded
     *      // add by wangyl 2015.6.17
     */ 
	signals[PAGES_LOADED] = g_signal_new (
        "pages_loaded",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        midori_cclosure_marshal_VOID__STRING_STRING,
        G_TYPE_NONE, 0);

    /**
     * MidoriBrowser::populate-tool-menu:
     * @browser: the object on which the signal is emitted
     * @menu: the #GtkMenu to populate
     *
     * Emitted when a Tool menu is displayed, such as the
     * toplevel Tools in the menubar or the compact menu.
     *
     * Since: 0.1.9
     */
    signals[POPULATE_TOOL_MENU] = g_signal_new (
        "populate-tool-menu",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        GTK_TYPE_MENU);
    /**
     * MidoriBrowser::populate-toolbar-menu:
     * @browser: the object on which the signal is emitted
     * @menu: the #GtkMenu to populate
     *
     * Emitted when a toolbar menu is displayed on right-click.
     *
     * Since: 0.3.4
     */
    signals[POPULATE_TOOLBAR_MENU] = g_signal_new (
        "populate-toolbar-menu",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        GTK_TYPE_MENU);

    signals[QUIT] = g_signal_new (
        "quit",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
        G_STRUCT_OFFSET (MidoriBrowserClass, quit),
        0,
        NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    /**
     * MidoriBrowser::show-preferences:
     * @browser: the object on which the signal is emitted
     * @preferences: the #KatzePreferences to populate
     *
     * Emitted when a preference dialogue displayed, to allow
     * adding of a new page, to be used sparingly.
     *
     * Since: 0.3.4
     */
    signals[SHOW_PREFERENCES] = g_signal_new (
        "show-preferences",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__OBJECT,
        G_TYPE_NONE, 1,
        KATZE_TYPE_PREFERENCES);

    class->add_tab = _midori_browser_add_tab;
    class->remove_tab = _midori_browser_remove_tab;
    class->activate_action = _midori_browser_activate_action;
    class->quit = _midori_browser_quit;

    gtkwidget_class = GTK_WIDGET_CLASS (class);
    gtkwidget_class->key_press_event = midori_browser_key_press_event;
    gtkwidget_class->key_release_event = midori_browser_key_release_event;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->dispose = midori_browser_dispose;
    gobject_class->finalize = midori_browser_finalize;
    gobject_class->set_property = midori_browser_set_property;
    gobject_class->get_property = midori_browser_get_property;

    flags = G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS;

    g_object_class_install_property (gobject_class,
                                     PROP_MENUBAR,
                                     g_param_spec_object (
                                     "menubar",
                                     "Menubar",
                                     "The menubar",
                                     GTK_TYPE_MENU_BAR,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_NAVIGATIONBAR,
                                     g_param_spec_object (
                                     "navigationbar",
                                     "Navigationbar",
                                     "The navigationbar",
                                     GTK_TYPE_TOOLBAR,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_NOTEBOOK,
                                     g_param_spec_object (
                                     "notebook",
                                     "Notebook",
                                     "The notebook containing the views",
                                     GTK_TYPE_CONTAINER,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_PANEL,
                                     g_param_spec_object (
                                     "panel",
                                     "Panel",
                                     "The side panel embedded in the browser",
                                     MIDORI_TYPE_PANEL,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_URI,
                                     g_param_spec_string (
                                     "uri",
                                     "URI",
                                     "The current URI",
                                     "",
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_TAB,
                                     g_param_spec_object (
                                     "tab",
                                     "Tab",
                                     "The current tab",
                                     GTK_TYPE_WIDGET,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class,
                                     PROP_LOAD_STATUS,
                                     g_param_spec_enum (
                                     "load-status",
                                     "Load Status",
                                     "The current load status",
                                     MIDORI_TYPE_LOAD_STATUS,
                                     MIDORI_LOAD_FINISHED,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:statusbar:
    *
    * The widget representing the statusbar contents. This is
    * not an actual #GtkStatusbar but rather a #GtkBox.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_STATUSBAR,
                                     g_param_spec_object (
                                     "statusbar",
                                     "Statusbar",
                                     "The statusbar",
                                     GTK_TYPE_BOX,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:statusbar-text:
    *
    * The text that is displayed in the statusbar.
    *
    * This value reflects changes to the text visible in the statusbar, such
    * as the uri of a hyperlink the mouse hovers over or the description of
    * a menuitem.
    *
    * Setting this value changes the displayed text until the next change.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_STATUSBAR_TEXT,
                                     g_param_spec_string (
                                     "statusbar-text",
                                     "Statusbar Text",
                                     "The text that is displayed in the statusbar",
                                     "",
                                     flags));

    /**
    * MidoriBrowser:settings:
    *
    * An associated settings instance that is shared among all web views.
    *
    * Setting this value is propagated to every present web view. Also
    * every newly created web view will use this instance automatically.
    *
    * If no settings are specified a default will be used.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_SETTINGS,
                                     g_param_spec_object (
                                     "settings",
                                     "Settings",
                                     "The associated settings",
                                     MIDORI_TYPE_WEB_SETTINGS,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:proxy-items:
    *
    * The open views, automatically updated, for session management.
    *
    * Since: 0.4.8
    */
    g_object_class_install_property (gobject_class,
                                     PROP_PROXY_ITEMS,
                                     g_param_spec_object (
                                     "proxy-items",
                                     "Proxy Items",
                                     "The open tabs as an array",
                                     KATZE_TYPE_ARRAY,
                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:bookmarks:
    *
    * The bookmarks folder, containing all bookmarks.
    *
    * This is actually a reference to a bookmarks instance,
    * so if bookmarks should be used it must be initially set.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_BOOKMARKS,
                                     g_param_spec_object (
                                     "bookmarks",
                                     "Bookmarks",
                                     "The bookmarks folder, containing all bookmarks",
                                     TYPE_MIDORI_BOOKMARKS_DB,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:trash:
    *
    * The trash, that collects all closed tabs and windows.
    *
    * This is actually a reference to a trash instance, so if a trash should
    * be used it must be initially set.
    *
    * Note: In the future the trash might collect other types of items.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_TRASH,
                                     g_param_spec_object (
                                     "trash",
                                     "Trash",
                                     "The trash, collecting recently closed tabs and windows",
                                     KATZE_TYPE_ARRAY,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:search-engines:
    *
    * The list of search engines to be used for web search.
    *
    * This is actually a reference to a search engines instance,
    * so if search engines should be used it must be initially set.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_SEARCH_ENGINES,
                                     g_param_spec_object (
                                     "search-engines",
                                     "Search Engines",
                                     "The list of search engines to be used for web search",
                                     KATZE_TYPE_ARRAY,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
    * MidoriBrowser:history:
    *
    * The list of history items.
    *
    * This is actually a reference to a history instance,
    * so if history should be used it must be initially set.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_HISTORY,
                                     g_param_spec_object (
                                     "history",
                                     "History",
                                     "The list of history items",
                                     KATZE_TYPE_ARRAY,
                                     flags));

    /**
    * MidoriBrowser:speed-dial:
    *
    * The speed dial configuration file.
    *
    * Since: 0.3.4
    * Since 0.4.7 this is a Midori.SpeedDial instance.
    */
    g_object_class_install_property (gobject_class,
                                     PROP_SPEED_DIAL,
                                     g_param_spec_object (
                                     "speed-dial",
                                     "Speeddial",
                                     "Speed dial",
                                     MIDORI_TYPE_SPEED_DIAL,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    /**
     * MidoriBrowser:show-tabs:
     *
     * Whether or not to show tabs.
     *
     * If disabled, no tab labels are shown. This is intended for
     * extensions that want to provide alternative tab labels.
     *
     * Since 0.1.8
    */
    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_TABS,
                                     g_param_spec_boolean (
                                     "show-tabs",
                                     "Show Tabs",
                                     "Whether or not to show tabs",
                                     TRUE,
                                     flags));

    #if !GTK_CHECK_VERSION (3, 0, 0)
    /* Add 2px space between tool buttons */
    gtk_rc_parse_string (
        "style \"tool-button-style\"\n {\n"
        "GtkToolButton::icon-spacing = 2\n }\n"
        "widget \"MidoriBrowser.*.MidoriBookmarkbar.Gtk*ToolButton\" "
        "style \"tool-button-style\"\n"
        "widget \"MidoriBrowser.*.MidoriFindbar.Gtk*ToolButton\" "
        "style \"tool-button-style\"\n");
    #endif
}

static void
_action_window_new_activate (GtkAction*     action,
                             MidoriBrowser* browser)
{
    midori_view_new_window_cb (NULL, "about:home", browser);
}

static void
_action_tab_new_activate (GtkAction*     action,
                          MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_add_uri (browser, "about:new");
    midori_browser_set_current_tab (browser, view);
}

static void
_action_private_browsing_activate (GtkAction*     action,
                                   MidoriBrowser* browser)
{
    sokoke_spawn_app ("about:private", TRUE);
}

static void
_action_open_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    #if !GTK_CHECK_VERSION (3, 1, 10)
    static gchar* last_dir = NULL;
    gboolean folder_set = FALSE;
    #endif
    gchar* uri = NULL;
    GtkWidget* dialog;
    GtkWidget* view;

    dialog = (GtkWidget*)midori_file_chooser_dialog_new (_("Open file"),
        GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_OPEN);

     /* base the start folder on the current view's uri if it is local */
    /*
     view = midori_browser_get_current_tab (browser);
     if ((uri = (gchar*)midori_view_get_display_uri (MIDORI_VIEW (view))))
     {
         gchar* filename = g_filename_from_uri (uri, NULL, NULL);
         if (filename)
         {
             gchar* dirname = g_path_get_dirname (filename);
             if (dirname && g_file_test (dirname, G_FILE_TEST_IS_DIR))
             {
                 gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirname);
                 #if !GTK_CHECK_VERSION (3, 1, 10)
                 folder_set = TRUE;
                 #endif
             }

             g_free (dirname);
             g_free (filename);
         }
     }
     */
     //#if !GTK_CHECK_VERSION (3, 1, 10)
     //if (!folder_set && last_dir && *last_dir)
         gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), getenv("HOME"));
     //#endif

     if (midori_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
     {
         #if !GTK_CHECK_VERSION (3, 1, 10)
         gchar* folder;
         folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
         katze_assign (last_dir, folder);
         #endif
         uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
         midori_browser_set_current_uri (browser, uri);
         g_free (uri);

     }
    gtk_widget_destroy (dialog);
}

static void
_action_save_as_activate (GtkAction*     action,
                          MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    midori_browser_save_uri (browser, MIDORI_VIEW (view), NULL,NULL);
}

static void
_action_compact_add_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    GtkWidget* dialog;
    GtkBox* box;
    const gchar* actions[] = { "BookmarkAdd", "AddNewsFeed" };
    guint i;

    dialog = g_object_new (GTK_TYPE_DIALOG,
        "transient-for", browser,
        "title", _("Add a new bookmark"), NULL);
    box = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog)));

    for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
        gchar* label;
        GtkWidget* button;

        action = _action_by_name (browser, actions[i]);
        label = katze_object_get_string (action, "label");
        button = gtk_button_new_with_mnemonic (label);
        g_free (label);
        gtk_widget_set_name (button, "GtkButton-thumb");
        gtk_box_pack_start (box, button, TRUE, TRUE, 4);
        gtk_activatable_set_related_action (GTK_ACTIVATABLE (button), action);
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (gtk_widget_destroy), dialog);
    }

    gtk_widget_show (dialog);
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy), dialog);
}

static void
_action_tab_close_activate (GtkAction*     action,
                            MidoriBrowser* browser)
{
    GtkWidget* widget = midori_browser_get_current_tab (browser);
    midori_browser_close_tab (browser, widget);
}

static void
_action_window_close_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
    gboolean val = FALSE;
    GdkEvent* event = gtk_get_current_event();
    g_signal_emit_by_name (G_OBJECT (browser), "delete-event", event, &val);
    gdk_event_free (event);
    if (!val)
        gtk_widget_destroy (GTK_WIDGET (browser));
}

static void
_action_print_activate (GtkAction*     action,
                        MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);

    midori_view_print (MIDORI_VIEW (view));
}

static void
_action_quit_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    int result = system("/usr/local/libexec/cdosbrowser/cdosbrowser_download quit &");
    if(result >= 0)
       g_signal_emit (browser, signals[QUIT], 0);
}

static void
_action_edit_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    gboolean can_cut = FALSE, can_copy = FALSE, can_paste = FALSE;
    gboolean has_selection, can_select_all = FALSE;

    if (WEBKIT_IS_WEB_VIEW (widget))
    {
        midori_tab_update_actions (MIDORI_TAB (midori_browser_get_current_tab (browser)), browser->action_group, NULL, NULL);
        return;
    }
    else if (GTK_IS_EDITABLE (widget))
    {
        GtkEditable* editable = GTK_EDITABLE (widget);
        has_selection = gtk_editable_get_selection_bounds (editable, NULL, NULL);
        can_cut = has_selection && gtk_editable_get_editable (editable);
        can_copy = has_selection;
        can_paste = gtk_editable_get_editable (editable);
        can_select_all = TRUE;
    }
    else if (GTK_IS_TEXT_VIEW (widget))
    {
        GtkTextView* text_view = GTK_TEXT_VIEW (widget);
        GtkTextBuffer* buffer = gtk_text_view_get_buffer (text_view);
        has_selection = gtk_text_buffer_get_has_selection (buffer);
        can_cut = gtk_text_view_get_editable (text_view);
        can_copy = has_selection;
        can_paste = gtk_text_view_get_editable (text_view) && has_selection;
        can_select_all = TRUE;
    }
    _action_set_sensitive (browser, "Cut", can_cut);
    _action_set_sensitive (browser, "Copy", can_copy);
    _action_set_sensitive (browser, "Paste", can_paste);
    _action_set_sensitive (browser, "Delete", can_cut);
    _action_set_sensitive (browser, "SelectAll", can_select_all);
}

static void
_action_cut_activate (GtkAction*     action,
                      MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget) && g_signal_lookup ("cut-clipboard", G_OBJECT_TYPE (widget)))
        g_signal_emit_by_name (widget, "cut-clipboard");
#ifdef HAVE_WEBKIT2
    else if (WEBKIT_IS_WEB_VIEW (widget))
        webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (widget), WEBKIT_EDITING_COMMAND_CUT);
#endif
}

static void
_action_copy_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget) && g_signal_lookup ("copy-clipboard", G_OBJECT_TYPE (widget)))
        g_signal_emit_by_name (widget, "copy-clipboard");
#ifdef HAVE_WEBKIT2
    else if (WEBKIT_IS_WEB_VIEW (widget))
        webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (widget), WEBKIT_EDITING_COMMAND_COPY);
#endif
}

static void
_action_paste_activate (GtkAction*     action,
                        MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget) && g_signal_lookup ("paste-clipboard", G_OBJECT_TYPE (widget)))
        g_signal_emit_by_name (widget, "paste-clipboard");
#ifdef HAVE_WEBKIT2
    else if (WEBKIT_IS_WEB_VIEW (widget))
        webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (widget), WEBKIT_EDITING_COMMAND_PASTE);
#endif
}

static void
_action_delete_activate (GtkAction*     action,
                         MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget))
    {
        if (GTK_IS_EDITABLE (widget))
            gtk_editable_delete_selection (GTK_EDITABLE (widget));
        else if (WEBKIT_IS_WEB_VIEW (widget))
       //midify by luyue 2015/3/16
       //modiri不支持webkit2的delete操作。
       //Webkit2/UIProcess/API/gtk/WebKitEditingCommands.h中无WEBKIT_EDITING_COMMAND_DELETE
       //用cut代替delete
           webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (widget), WEBKIT_EDITING_COMMAND_CUT);
        else if (GTK_IS_TEXT_VIEW (widget))
            gtk_text_buffer_delete_selection (
                gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)), TRUE, FALSE);
    }
}
static void
_action_select_all_activate (GtkAction*     action,
                             MidoriBrowser* browser)
{
    GtkWidget* widget = gtk_window_get_focus (GTK_WINDOW (browser));
    if (G_LIKELY (widget))
    {
        if (GTK_IS_EDITABLE (widget))
            gtk_editable_select_region (GTK_EDITABLE (widget), 0, -1);
#ifdef HAVE_WEBKIT2
        else if (WEBKIT_IS_WEB_VIEW (widget))
            webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (widget), WEBKIT_EDITING_COMMAND_SELECT_ALL);
#endif
        else if (g_signal_lookup ("select-all", G_OBJECT_TYPE (widget)))
        {
            if (GTK_IS_TEXT_VIEW (widget))
                g_signal_emit_by_name (widget, "select-all", TRUE);
            else if (GTK_IS_TREE_VIEW (widget))
            {
                gboolean dummy;
                g_signal_emit_by_name (widget, "select-all", &dummy);
            }
            else
                g_signal_emit_by_name (widget, "select-all");
        }
    }
}

static void
_action_find_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    midori_findbar_invoke (MIDORI_FINDBAR (browser->find),
        midori_view_get_selected_text (MIDORI_VIEW (view)));
}

static void
midori_browser_navigationbar_notify_style_cb (GObject*       object,
                                              GParamSpec*    arg1,
                                              MidoriBrowser* browser)
{
    MidoriToolbarStyle toolbar_style;

    g_object_get (browser->settings, "toolbar-style", &toolbar_style, NULL);
    _midori_browser_set_toolbar_style (browser, toolbar_style);
}

/**
 * midori_browser_get_settings:
 *
 * Retrieves the settings instance of the browser.
 *
 * Return value: a #MidoriWebSettings instance
 *
 * Since: 0.2.5
 **/
MidoriWebSettings*
midori_browser_get_settings (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);

    return browser->settings;
}

static gboolean
midori_browser_toolbar_popup_context_menu_cb (GtkWidget*     widget,
                                              gint           x,
                                              gint           y,
                                              gint           button,
                                              MidoriBrowser* browser)
{
    MidoriContextAction* menu = midori_context_action_new ("ToolbarContextMenu", NULL, NULL, NULL);
    midori_context_action_add_action_group (menu, browser->action_group);
    midori_context_action_add_by_name (menu, "Menubar");
    midori_context_action_add_by_name (menu, "Bookmarkbar");
    midori_context_action_add_by_name (menu, "Statusbar");

    GtkMenu* context_menu = midori_context_action_create_menu (menu, NULL, FALSE);
    g_signal_emit (browser, signals[POPULATE_TOOLBAR_MENU], 0, context_menu);
    katze_widget_popup (widget, GTK_MENU (context_menu), NULL,
        button == -1 ? KATZE_MENU_POSITION_LEFT : KATZE_MENU_POSITION_CURSOR);
    return TRUE;
}

//add by zgh 20141217
static gboolean
midori_history_activate_item (GtkAction*     action,
                              KatzeItem*     item,
                              MidoriBrowser* browser)
{
    return midori_browser_open_bookmark (browser, item);
}
//add by zgh 20141217
static gboolean
midori_history_activate_item_alt (GtkAction*      action,
                                      KatzeItem*      item,
                                      GtkWidget*      proxy,
                                      GdkEventButton* event,
                                      MidoriBrowser*  browser)
{
    g_assert (event);
#if 0   //zgh
    if (MIDORI_EVENT_NEW_TAB (event))
    {
        GtkWidget* view = midori_browser_add_item (browser, item);
        midori_browser_set_current_tab_smartly (browser, view);
    }
    else if (MIDORI_EVENT_CONTEXT_MENU (event))
    {
        midori_browser_bookmark_popup (proxy, NULL, item, browser);
    }
    else if (event->button == 1)
    {
        midori_bookmarkbar_activate_item (action, item, browser);
    }
#endif
    return TRUE;
}

static void
midori_browser_bookmark_popup (GtkWidget*      proxy,
                               GdkEventButton* event,
                               KatzeItem*      item,
                               MidoriBrowser*  browser);

static gboolean
midori_bookmarkbar_activate_item (GtkAction* action,
                                  KatzeItem* item,
                                  MidoriBrowser* browser)
{
    return midori_browser_open_bookmark (browser, item);;
}

static gboolean
midori_bookmarkbar_activate_item_alt (GtkAction*      action,
                                      KatzeItem*      item,
                                      GtkWidget*      proxy,
                                      GdkEventButton* event,
                                      MidoriBrowser*  browser)
{
    g_assert (event);

    if (MIDORI_EVENT_NEW_TAB (event))
    {
        GtkWidget* view = midori_browser_add_item (browser, item);
        midori_browser_set_current_tab_smartly (browser, view);
    }
    else if (MIDORI_EVENT_CONTEXT_MENU (event))
    {
        midori_browser_bookmark_popup (proxy, NULL, item, browser);
    }
    else if (event->button == 1)
    {
        midori_bookmarkbar_activate_item (action, item, browser);
    }

    return TRUE;
}

/* static */ gboolean
midori_browser_open_bookmark (MidoriBrowser* browser,
                              KatzeItem*     item)
{
    const gchar* uri = katze_item_get_uri (item);
    gchar* uri_fixed;

    if (!(uri && *uri))
        return FALSE;

    /* Imported bookmarks may lack a protocol */
    uri_fixed = sokoke_magic_uri (uri, TRUE, FALSE);
    if (!uri_fixed)
        uri_fixed = g_strdup (uri);

    if (katze_item_get_meta_boolean (item, "app"))
        sokoke_spawn_app (uri_fixed, FALSE);
    else
    {
        midori_browser_set_current_uri (browser, uri_fixed);
        gtk_widget_grab_focus (midori_browser_get_current_tab (browser));
    }
    g_free (uri_fixed);
    return TRUE;
}

static void
_action_tools_populate_popup (GtkAction*     action,
                              GtkMenu*       default_menu,
                              MidoriBrowser* browser)
{
    MidoriContextAction* menu = midori_context_action_new ("ToolsMenu", NULL, NULL, NULL);
    midori_context_action_add_action_group (menu, browser->action_group);
    midori_context_action_add_by_name (menu, "ClearPrivateData");
    midori_context_action_add_by_name (menu, "InspectPage");
    midori_context_action_add_by_name (menu, "PageInfo");   //zgh 20141225
#ifdef BOOKMARK_SYNC
    midori_context_action_add_by_name (menu, "Bookmarks_Sync");
#endif
    g_signal_emit (browser, signals[POPULATE_TOOL_MENU], 0, default_menu);
    #ifdef G_OS_WIN32
    midori_context_action_add (menu, NULL);
    midori_context_action_add_by_name (menu, "Preferences");
    #endif
    midori_context_action_create_menu (menu, default_menu, TRUE);
}

//add by zgh 20141210
static gboolean
_action_historys_populate_folder (GtkAction*     action,
                                   GtkMenuShell*  menu,
                                   MidoriBrowser* browser)
{
    KatzeArray* array;
    GtkWidget* menuitem;
    if (browser->history_database == NULL)
        return FALSE;
    
    array = midori_browser_history_read_from_db(browser);

    /* Clear items from dummy array here */
    gtk_container_foreach (GTK_CONTAINER (menu),
        (GtkCallback)(gtk_widget_destroy), NULL);

    /* "ManageHistorys" at the top */
    //if (folder == KATZE_ARRAY (browser->history_database))
    {    
       menuitem = gtk_action_create_menu_item (_action_by_name (browser, "ManageHistorys"));
       gtk_menu_item_set_label(GTK_MENU_ITEM(menuitem), N_("管理历史记录(H)"));
       gtk_menu_shell_append (menu, menuitem);
       gtk_widget_show (menuitem);
    }
    menuitem = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (menu, menuitem);
    gtk_widget_show (menuitem);

    if (!katze_array_is_empty (array))  //zgh
    {
        katze_array_action_generate_menu (KATZE_ARRAY_ACTION (action), array,
                                      menu, GTK_WIDGET (browser));
    }
    else
    {
        menuitem = gtk_image_menu_item_new_with_label (_("History_Empty"));
        gtk_widget_set_sensitive (menuitem, FALSE);
        gtk_menu_shell_append (menu, menuitem);
        gtk_widget_show (menuitem);
    }

    return TRUE;
}

#ifdef BOOKMARK_SYNC //lxx, 20150612
static GtkWidget *
createSecondMenu(MidoriBrowser* browser,char *name)
{
     GtkWidget *rootFileItem;
     GtkWidget *fileMenu;
     GtkWidget *importMenuItem;
     GtkWidget *exportMenuItem;
     rootFileItem = gtk_action_create_menu_item(_action_by_name (browser, name));
     fileMenu = gtk_menu_new();
     importMenuItem=gtk_action_create_menu_item (_action_by_name (browser, "From_Server"));
     exportMenuItem=gtk_action_create_menu_item (_action_by_name (browser, "From_Local"));
     gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu),importMenuItem);
     gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu),exportMenuItem);
     gtk_menu_item_set_submenu(GTK_MENU_ITEM(rootFileItem), fileMenu);
     return rootFileItem;
}

static GtkWidget *
createSecondMenu2(MidoriBrowser* browser,char *name)
{
     GtkWidget *rootFileItem;
     GtkWidget *fileMenu;
     GtkWidget *importMenuItem;
     GtkWidget *exportMenuItem;
     rootFileItem = gtk_action_create_menu_item(_action_by_name (browser, name));
     fileMenu = gtk_menu_new();
     importMenuItem=gtk_action_create_menu_item (_action_by_name (browser, "From_Server2"));
     exportMenuItem=gtk_action_create_menu_item (_action_by_name (browser, "From_Local2"));
     gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu),importMenuItem);
     gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu),exportMenuItem);

     gtk_menu_item_set_submenu(GTK_MENU_ITEM(rootFileItem), fileMenu);
     return rootFileItem;
}
#endif //BOOKMARK_SYNC

static gboolean
_action_bookmarks_populate_folder (GtkAction*     action,
                                   GtkMenuShell*  menu,
                                   KatzeArray*    folder,
                                   MidoriBrowser* browser)
{
    if (browser->bookmarks == NULL)
        return FALSE;

    midori_bookmarks_db_populate_folder (browser->bookmarks, folder);

    /* Clear items from dummy array here */
    gtk_container_foreach (GTK_CONTAINER (menu),
        (GtkCallback)(gtk_widget_destroy), NULL);

    /* "Add Bookmark", "Import Bookmarks", and "Export Bookmarks" at the top */
    if (folder == KATZE_ARRAY (browser->bookmarks))
    {
        GtkWidget* menuitem;
        menuitem = gtk_action_create_menu_item (_action_by_name (browser, "ManageBookmarks"));
        gtk_menu_shell_append (menu, menuitem);
        gtk_widget_show (menuitem);
        menuitem = gtk_action_create_menu_item (_action_by_name (browser, "BookmarkAdd"));
        gtk_menu_shell_append (menu, menuitem);
        gtk_widget_show (menuitem);
#ifndef BOOKMARK_SYNC
        menuitem = gtk_action_create_menu_item (_action_by_name (browser, "BookmarksImport"));
        gtk_menu_shell_append (menu, menuitem);
        gtk_widget_show (menuitem);
        menuitem = gtk_action_create_menu_item (_action_by_name (browser, "BookmarksExport"));
        gtk_menu_shell_append (menu, menuitem);
        gtk_widget_show (menuitem);
#else
        gtk_menu_shell_append(menu,createSecondMenu(browser,"BookmarksImport"));
         gtk_menu_shell_append(menu,createSecondMenu2(browser,"BookmarksExport"));
#endif
    }
    
    if (!katze_array_is_empty (folder))  //zgh
    {
        GtkWidget* menuitem = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (menu, menuitem);
        gtk_widget_show (menuitem);
        
        katze_array_action_generate_menu (KATZE_ARRAY_ACTION (action), folder,
                                      menu, GTK_WIDGET (browser));
    }
    return TRUE;
}

static void
_action_window_populate_popup (GtkAction*     action,
                               GtkMenu*       default_menu,
                               MidoriBrowser* browser)
{
    MidoriContextAction* menu = midori_context_action_new ("WindowMenu", NULL, NULL, NULL);
    midori_context_action_add_action_group (menu, browser->action_group);
    midori_context_action_add (menu, NULL);
    midori_context_action_add_by_name (menu, "LastSession");
    midori_context_action_add_by_name (menu, "TabCurrent");
    midori_context_action_add_by_name (menu, "NextView");
    midori_context_action_add_by_name (menu, "TabNext");
    midori_context_action_add_by_name (menu, "TabPrevious");
    midori_context_action_create_menu (menu, default_menu, TRUE);
}

static void
_action_window_activate_item_alt (GtkAction*      action,
                                  KatzeItem*      item,
                                  GtkWidget*      proxy,
                                  GdkEventButton* event,
                                  MidoriBrowser*  browser)
{
    midori_browser_set_current_item (browser, item);
}

// ZRL 屏蔽mailto和HelpFAQ
static void
_action_compact_menu_populate_popup (GtkAction*     action,
                                     GtkMenu*       default_menu,
                                     MidoriBrowser* browser)
{
    MidoriContextAction* menu = midori_context_action_new ("CompactMenu", NULL, NULL, NULL);
    midori_context_action_add_action_group (menu, browser->action_group);
    midori_context_action_add_by_name (menu, "WindowNew");
    midori_context_action_add_by_name (menu, "PrivateBrowsing");
    midori_context_action_add (menu, NULL);
    midori_context_action_add_by_name (menu, "Find");
    midori_context_action_add_by_name (menu, "Print");
    midori_context_action_add_by_name (menu, "Fullscreen");
    midori_context_action_add (menu, NULL);
    midori_context_action_add_by_name (menu, "Homepage");   //zgh
    midori_context_action_add (menu, NULL);
    midori_context_action_add_by_name (menu, "BookmarkAdd");
    midori_context_action_add_by_name (menu, "BookmarksImport");
    midori_context_action_add_by_name (menu, "BookmarksExport");
    midori_context_action_add_by_name (menu, "ClearPrivateData");
    g_signal_emit (browser, signals[POPULATE_TOOL_MENU], 0, default_menu);
    midori_context_action_add (menu, NULL);
    #ifndef HAVE_GRANITE
    midori_context_action_add_by_name (menu, "HelpBugs");
    #endif
    midori_context_action_add_by_name (menu, "About");
    midori_context_action_create_menu (menu, default_menu, FALSE);
}
static void
midori_browser_preferences_window_hide_cb (GtkWidget*      window, 
                                MidoriBrowser*  browser)
{
    gtk_widget_hide(window);
    return; 
}

static void
_action_preferences_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{/*//modified by wangyl 2015/9/25
   static GtkWidget* dialog = NULL;

   if (!dialog)
   {
      dialog = browser_settings_window_new(browser->settings); 
      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed), &dialog);
      gtk_widget_show (dialog);
      g_signal_connect(G_OBJECT(dialog), "delete-event", midori_preferences_window_hide , browser);
   }
   else
      gtk_window_present (GTK_WINDOW (dialog));*/
   MidoriApp  *app = midori_app_get_default();
   GList*  browsers=midori_app_get_browsers(app);
   for(; browsers; browsers=g_list_next(browsers))
   {
     browser = MIDORI_BROWSER(browsers->data);
     if(browser->settings_dialog)break;
   }
   gtk_widget_show_all(browser->settings_dialog);
   gtk_window_present(browser->settings_dialog);
}

static gboolean
midori_browser_has_native_menubar (void)
{
    static const gchar* ubuntu_menuproxy = NULL;
    if (ubuntu_menuproxy == NULL)
        ubuntu_menuproxy = g_getenv ("UBUNTU_MENUPROXY");
    return ubuntu_menuproxy && (strstr (ubuntu_menuproxy, ".so") || !strcmp (ubuntu_menuproxy, "1"));
}

static void
_action_menubar_activate (GtkToggleAction* menubar_action,
                          MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (menubar_action);
    GtkAction* menu_action = _action_by_name (browser, "CompactMenu");
    GString* toolbar_items;
    GList* children;
    gchar* items;

    if (midori_browser_has_native_menubar ())
        active = FALSE;

    toolbar_items = g_string_new (NULL);
    children = gtk_container_get_children (GTK_CONTAINER (browser->navigationbar));
    for (; children != NULL; children = g_list_next (children))
    {
        GtkAction* action = gtk_activatable_get_related_action (
            GTK_ACTIVATABLE (children->data));
        if (!action)
            continue;
        if (action == ((GtkAction*)menu_action))
        {
            if (active)
            {
                gtk_container_remove (GTK_CONTAINER (browser->navigationbar),
                                      GTK_WIDGET (children->data));
            }
            continue;
        }
        else if (MIDORI_IS_PANED_ACTION (action))
        {
            MidoriPanedAction* paned_action = MIDORI_PANED_ACTION (action);
            g_string_append_printf (toolbar_items, "%s,%s",
                midori_paned_action_get_child1_name (paned_action),
                midori_paned_action_get_child2_name (paned_action));
        }
        else
            g_string_append (toolbar_items, gtk_action_get_name (action));
        g_string_append_c (toolbar_items, ',');
    }
    g_list_free (children);

    if (katze_object_get_boolean (browser->settings, "show-menubar") != active)
        g_object_set (browser->settings, "show-menubar", active, NULL);

    items = g_string_free (toolbar_items, FALSE);
    g_object_set (browser->settings, "toolbar-items", items, NULL);
    g_free (items);

    sokoke_widget_set_visible (browser->menubar, active);
    g_object_set_data (G_OBJECT (browser), "midori-toolbars-visible",
        gtk_widget_get_visible (browser->menubar)
        || gtk_widget_get_visible (browser->navigationbar)
        ? (void*)0xdeadbeef : NULL);
}

static void
_action_navigationbar_activate (GtkToggleAction* action,
                                MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    g_object_set (browser->settings, "show-navigationbar", active, NULL);
    sokoke_widget_set_visible (browser->navigationbar, active);

    g_object_set_data (G_OBJECT (browser), "midori-toolbars-visible",
        gtk_widget_get_visible (browser->menubar)
        || gtk_widget_get_visible (browser->navigationbar)
        ? (void*)0xdeadbeef : NULL);
}

static void
_action_bookmarkbar_activate (GtkToggleAction* action,
                              MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    g_object_set (browser->settings, "show-bookmarkbar", active, NULL);
    sokoke_widget_set_visible (browser->bookmarkbar, active);
}

static void
_action_statusbar_activate (GtkToggleAction* action,
                            MidoriBrowser*   browser)
{
    gboolean active = gtk_toggle_action_get_active (action);
    g_object_set (browser->settings, "show-statusbar", active, NULL);
    sokoke_widget_set_visible (browser->statusbar, active);
}

static void
_action_reload_stop_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    gchar* stock_id;
    g_object_get (action, "stock-id", &stock_id, NULL);

    /* Refresh or stop, depending on the stock id */
    if (!stock_id || !strcmp (stock_id, GTK_STOCK_REFRESH))
    {
        GdkModifierType state = (GdkModifierType)0;
        gint x, y;
        GdkWindow* window;
        gboolean from_cache = TRUE;

        if (!strcmp (gtk_action_get_name (action), "ReloadUncached"))
            from_cache = FALSE;
        else if ((window = gtk_widget_get_window (GTK_WIDGET (browser))))
        {
            gdk_window_get_pointer (window, &x, &y, &state);
            if (state & GDK_SHIFT_MASK)
                from_cache = FALSE;
        }
        midori_view_reload (MIDORI_VIEW (view), from_cache);
    }
    else
        midori_tab_stop_loading (MIDORI_TAB (view));
    g_free (stock_id);
}

static void
_action_zoom_activate (GtkAction*     action,
                       MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);

    if (g_str_equal (gtk_action_get_name (action), "ZoomIn"))
           {
	   if (3.00f - midori_view_get_zoom_level (MIDORI_VIEW (view)) > 0.00f)
	     midori_view_set_zoom_level (MIDORI_VIEW (view),
            midori_view_get_zoom_level (MIDORI_VIEW (view)) + 0.10f);		
	   }
    else if (g_str_equal (gtk_action_get_name (action), "ZoomOut"))
	   {
      if (midori_view_get_zoom_level (MIDORI_VIEW (view)) - 0.30f > 0.00f)
        midori_view_set_zoom_level (MIDORI_VIEW (view),
            midori_view_get_zoom_level (MIDORI_VIEW (view)) - 0.10f);
	   }
    else
        midori_view_set_zoom_level (MIDORI_VIEW (view), 1.0f);
}

static void
_action_view_encoding_activate (GtkAction*     action,
                                GtkAction*     current,
                                MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    const gchar* name = gtk_action_get_name (current);
    WebKitWebView* web_view = WEBKIT_WEB_VIEW (midori_view_get_web_view (MIDORI_VIEW (view)));

    const gchar* encoding;
    if (!strcmp (name, "EncodingAutomatic"))
        encoding = NULL;
    else if (!strcmp (name, "EncodingChinese"))
        encoding = "BIG5";
    else if (!strcmp (name, "EncodingChineseSimplified"))
        encoding = "GB18030";
    else if (!strcmp (name, "EncodingJapanese"))
        encoding = "SHIFT_JIS";
    else if (!strcmp (name, "EncodingKorean"))
        encoding = "EUC-KR";
    else if (!strcmp (name, "EncodingRussian"))
        encoding = "KOI8-R";
    else if (!strcmp (name, "EncodingUnicode"))
        encoding = "UTF-8";
    else if (!strcmp (name, "EncodingWestern"))
        encoding = "ISO-8859-1";
    else
        g_assert_not_reached ();
    webkit_web_view_set_custom_charset (web_view, encoding);
}

static void
_action_source_view (GtkAction*     action,
                     MidoriBrowser* browser,
                     gboolean       use_dom)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    /* TODO: midori_view_save_source isn't async and not WebKit2-friendly */
    GtkWidget* source = midori_view_new_with_item (NULL, browser->settings);
    GtkWidget* source_view = midori_view_get_web_view (MIDORI_VIEW (source));
    midori_tab_set_view_source (MIDORI_TAB (source), TRUE);
    webkit_web_view_load_uri (WEBKIT_WEB_VIEW (source_view), midori_tab_get_uri (MIDORI_TAB (view)));
    midori_browser_add_tab (browser, source);
}

static void
_action_source_view_activate (GtkAction*     action,
                              MidoriBrowser* browser)
{
    _action_source_view (action, browser, FALSE);
}

static void
_action_source_view_dom_activate (GtkAction*     action,
                                  MidoriBrowser* browser)
{
    _action_source_view (action, browser, TRUE);
}


static void
_action_caret_browsing_activate (GtkAction*     action,
                                 MidoriBrowser* browser)
{
    gint response;
    GtkWidget* dialog;

    if (!katze_object_get_boolean (browser->settings, "enable-caret-browsing"))
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (browser),
            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
            GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
            _("Toggle text cursor navigation"));
        gtk_window_set_title (GTK_WINDOW (dialog), _("Toggle text cursor navigation"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
            _("Pressing F7 toggles Caret Browsing. When active, a text cursor appears in all websites."));
        gtk_dialog_add_buttons (GTK_DIALOG (dialog),
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            _("_Enable Caret Browsing"), GTK_RESPONSE_ACCEPT,
            NULL);

        response = midori_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        if (response != GTK_RESPONSE_ACCEPT)
            return;
    }

    g_object_set (browser->settings, "enable-caret-browsing",
        !katze_object_get_boolean (browser->settings, "enable-caret-browsing"), NULL);
}

static void
_action_fullscreen_activate (GtkAction*     action,
                             MidoriBrowser* browser)
{
    GdkWindowState state;

    if (!gtk_widget_get_window (GTK_WIDGET (browser)))
        return;

    state = gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (browser)));
    if (state & GDK_WINDOW_STATE_FULLSCREEN)
    {
        //add by luyue 2015/4/27 start
        //退出全屏时，关闭左下角连接提示
        GtkWidget* view = midori_browser_get_current_tab (browser);
        midori_view_set_overlay_text (MIDORI_VIEW (view), NULL);
        //add end
        if (katze_object_get_boolean (G_OBJECT (browser->settings), "show-menubar"))
            {
            gtk_widget_show (browser->menubar);
            _action_set_visible(browser, "Preferences", TRUE);  //zgh
            }

        if (katze_object_get_boolean (G_OBJECT (browser->settings), "show-panel"))
            gtk_widget_show (browser->panel);

        if (katze_object_get_boolean (G_OBJECT (browser->settings), "show-bookmarkbar"))
            gtk_widget_show (browser->bookmarkbar);

        if (browser->show_navigationbar)
            gtk_widget_show (browser->navigationbar);

        if (browser->show_statusbar)
            gtk_widget_show (browser->statusbar);
        _toggle_tabbar_smartly (browser, TRUE);

        gtk_window_unfullscreen (GTK_WINDOW (browser));
    }
    else
    {
        gtk_widget_hide (browser->menubar);
        gtk_widget_hide (browser->panel);
        gtk_widget_hide (browser->bookmarkbar);
        gtk_widget_hide (browser->navigationbar);
        gtk_widget_hide (browser->statusbar);
        midori_notebook_set_labels_visible (MIDORI_NOTEBOOK (browser->notebook), FALSE);

        gtk_window_fullscreen (GTK_WINDOW (browser));
    }
}

static void
_action_readable_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    gchar* filename;
    gchar* stylesheet;
    gint i;

    filename = midori_paths_get_res_filename ("faq.css");
    stylesheet = NULL;
    if (!g_file_get_contents (filename, &stylesheet, NULL, NULL))
    {
        #ifdef G_OS_WIN32
        katze_assign (filename, midori_paths_get_data_filename ("doc/midori/faq.css", FALSE));
        #else
        katze_assign (filename, g_build_filename (DOCDIR, "faq.css", NULL));
        #endif
        g_file_get_contents (filename, &stylesheet, NULL, NULL);
    }
    if (!(stylesheet && *stylesheet))
    {
        g_free (filename);
        g_free (stylesheet);
        midori_view_add_info_bar (MIDORI_VIEW (view), GTK_MESSAGE_ERROR,
            "Stylesheet faq.css not found", NULL, view,
            GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
        return;
    }

    i = 0;
    while (stylesheet[i])
    {
        /* Replace line breaks with spaces */
        if (stylesheet[i] == '\n' || stylesheet[i] == '\r')
            stylesheet[i] = ' ';
        /* Change all single quotes to double quotes */
        else if (stylesheet[i] == '\'')
            stylesheet[i] = '\"';
        i++;
    }

    midori_tab_inject_stylesheet (MIDORI_TAB (view), stylesheet);
    g_free (stylesheet);
}

static gboolean
_action_navigation_activate (GtkAction*     action,
                             MidoriBrowser* browser)
{
    MidoriView* view;
    GtkWidget* tab;
    gchar* uri;
    const gchar* name;
    gboolean middle_click;

    g_assert (GTK_IS_ACTION (action));

    if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (action),
                                            "midori-middle-click")))
    {
        middle_click = TRUE;
        g_object_set_data (G_OBJECT (action),
                           "midori-middle-click",
                           GINT_TO_POINTER(0));
    }
    else
        middle_click = FALSE;

    tab = midori_browser_get_current_tab (browser);
    view = MIDORI_VIEW (tab);
    name = gtk_action_get_name (action);

    if (!strcmp (name, "NextForward"))
        name = midori_tab_can_go_forward (MIDORI_TAB (view)) ? "Forward" : "Next";

    if (g_str_equal (name, "Back"))
    {
        if (middle_click)
        {
            WebKitWebView* web_view = WEBKIT_WEB_VIEW (midori_view_get_web_view (view));
            WebKitBackForwardList* list = webkit_web_view_get_back_forward_list (web_view);
            WebKitBackForwardListItem* item = webkit_back_forward_list_get_back_item (list);
            const gchar* back_uri = webkit_back_forward_list_item_get_uri (item);
            GtkWidget* new_view = midori_browser_add_uri (browser, back_uri);
            midori_browser_set_current_tab_smartly (browser, new_view);
        }
        else
        {
            midori_view_go_back (view);
            _action_set_visible(browser, "Forward", TRUE);  //zgh
        }

        return TRUE;
    }
    else if (g_str_equal (name, "Forward"))
    {
        if (middle_click)
        {
            WebKitWebView* web_view = WEBKIT_WEB_VIEW (midori_view_get_web_view (view));
            WebKitBackForwardList* list = webkit_web_view_get_back_forward_list (web_view);
            WebKitBackForwardListItem* item = webkit_back_forward_list_get_forward_item (list);
            const gchar* forward_uri = webkit_back_forward_list_item_get_uri (item);
            GtkWidget* new_view = midori_browser_add_uri (browser, forward_uri);
            midori_browser_set_current_tab_smartly (browser, new_view);
        }
        else
          midori_tab_go_forward (MIDORI_TAB (view));

        return TRUE;
    }
    else if (g_str_equal (name, "Previous"))
    {
        /* Duplicate here because the URI pointer might change */
        uri = g_strdup (midori_view_get_previous_page (view));

        if (middle_click)
        {
            GtkWidget* new_view = midori_browser_add_uri (browser, uri);
            midori_browser_set_current_tab_smartly (browser, new_view);
        }
        else
            midori_view_set_uri (view, uri);

        g_free (uri);
        return TRUE;
    }
    else if (g_str_equal (name, "Next"))
    {
        /* Duplicate here because the URI pointer might change */
        uri = g_strdup (midori_view_get_next_page (view));

        if (middle_click)
        {
            GtkWidget* new_view = midori_browser_add_uri (browser, uri);
            midori_browser_set_current_tab_smartly (browser, new_view);
        }
        else
            midori_view_set_uri (view, uri);

        g_free (uri);
        return TRUE;
    }
    else if (g_str_equal (name, "Homepage"))
    {
        if (middle_click)
        {
            GtkWidget* new_view = midori_browser_add_uri (browser, "about:home");
            midori_browser_set_current_tab_smartly (browser, new_view);
        }
        else
	{
	    gchar *strval = katze_object_get_string(browser->settings, "homepage");
            midori_view_set_uri (view, strval);
	    g_free (strval);
	}

        return TRUE;
    }
    return FALSE;
}

static void
_action_location_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    if (!gtk_widget_get_visible (browser->navigationbar))
        gtk_widget_show (browser->navigationbar);
}

static void
_action_location_focus_out (GtkAction*     action,
                            MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);

    if (!browser->show_navigationbar || midori_browser_is_fullscreen (browser))
        gtk_widget_hide (browser->navigationbar);

    midori_browser_update_secondary_icon (browser, MIDORI_VIEW (view), action); //zgh 1203 +放开1224
}

static void
_action_location_reset_uri (GtkAction*     action,
                            MidoriBrowser* browser)
{
    midori_location_action_set_text (MIDORI_LOCATION_ACTION (action),
        midori_browser_get_current_uri (browser));
}

static void
_action_location_submit_uri (GtkAction*     action,
                             const gchar*   uri,
                             gboolean       new_tab,
                             MidoriBrowser* browser)
{
    gchar* new_uri;

    uri = katze_skip_whitespace (uri);
    new_uri = sokoke_magic_uri (uri, TRUE, FALSE);
    if (!new_uri)
    {
        const gchar* keywords = NULL;
        const gchar* search_uri = NULL;
        KatzeItem* item;

        /* Do we have a keyword and a string? */
        if (browser->search_engines
         && (item = katze_array_find_token (browser->search_engines, uri)))
        {
            keywords = strchr (uri, ' ');
            if (keywords != NULL)
                keywords++;
            else
                keywords = "";
            search_uri = katze_item_get_uri (item);
        }

        if (keywords == NULL)
        {
            keywords = uri;
            search_uri = midori_settings_get_location_entry_search (
                MIDORI_SETTINGS (browser->settings));
        }
        new_uri = midori_uri_for_search (search_uri, keywords);

        if (browser->history != NULL)
        {
            time_t now = time (NULL);
            gint64 day = sokoke_time_t_to_julian (&now);
            sqlite3* db = g_object_get_data (G_OBJECT (browser->history), "db");
            static sqlite3_stmt* statement = NULL;

            if (!statement)
            {
                const gchar* sqlcmd;
                sqlcmd = "INSERT INTO search (keywords, uri, day) VALUES (?,?,?)";
                sqlite3_prepare_v2 (db, sqlcmd, strlen (sqlcmd) + 1, &statement, NULL);
            }
            sqlite3_bind_text (statement, 1, keywords, -1, 0);
            sqlite3_bind_text (statement, 2, search_uri, -1, 0);
            sqlite3_bind_int64 (statement, 3, day);

            if (sqlite3_step (statement) != SQLITE_DONE)
                g_printerr (_("Failed to insert new history item: %s\n"),
                        sqlite3_errmsg (db));
            sqlite3_reset (statement);
            if (sqlite3_step (statement) == SQLITE_DONE)
                sqlite3_clear_bindings (statement);
        }
    }

    if (new_tab)
    {
        GtkWidget* view = midori_browser_add_uri (browser, new_uri);
        midori_browser_set_current_tab (browser, view);
    }
    else
        midori_browser_set_current_uri (browser, new_uri);
    g_free (new_uri);
    gtk_widget_grab_focus (midori_browser_get_current_tab (browser));
}

static gboolean
_action_location_secondary_icon_released (GtkAction*     action,
                                          GtkWidget*     widget,
                                          MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
   // KatzeItem* bookmark = (KatzeItem*)katze_array_new (KATZE_TYPE_ARRAY);
    KatzeItem* bookmark =  g_object_new (KATZE_TYPE_ITEM,NULL);
    katze_item_set_name (bookmark,
    midori_view_get_display_title (MIDORI_VIEW (view)));
    if(strstr (midori_view_get_display_uri (MIDORI_VIEW (view)), "speeddial-head.html"))
       katze_item_set_uri (bookmark,"about:dial");
    else
       katze_item_set_uri (bookmark, 
    midori_view_get_display_uri (MIDORI_VIEW (view)));
    katze_item_set_meta_integer (bookmark, "toolbar", TRUE);
    katze_item_set_meta_integer (bookmark, "parentid", 0);

    midori_bookmarks_db_add_item (browser->bookmarks, bookmark);
    midori_location_action_set_secondary_icon_tooltip (
    MIDORI_LOCATION_ACTION (action), STOCK_BOOKMARKED, _("Bookmark already exist"));  //add by zgh 1224
    return TRUE;
}

#if ENABLE_SEARCH_ACTION
static void
_action_search_submit (GtkAction*     action,
                       const gchar*   keywords,
                       gboolean       new_tab,
                       MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* url;
    gchar* search;

    item = katze_array_get_nth_item (browser->search_engines, browser->last_web_search);
    if (item)
        url = katze_item_get_uri (item);
    else /* The location entry search is our fallback */
        url = midori_settings_get_location_entry_search (MIDORI_SETTINGS (browser->settings));

    search = midori_uri_for_search (url, keywords);

    if (new_tab)
    {
        GtkWidget* view = midori_browser_add_uri (browser, search);
        midori_browser_set_current_tab_smartly (browser, view);
    }
    else
        midori_browser_set_current_uri (browser, search);

    g_free (search);
}

static void
_action_search_activate (GtkAction*     action,
                         MidoriBrowser* browser)
{
    GSList* proxies = gtk_action_get_proxies (action);
    for (; proxies != NULL; proxies = g_slist_next (proxies))
        if (GTK_IS_TOOL_ITEM (proxies->data))
        {
            if (!gtk_widget_get_visible (browser->navigationbar))
                gtk_widget_show (browser->navigationbar);
            return;
        }

    midori_browser_set_current_uri (browser, "about:search");
    gtk_widget_grab_focus (midori_browser_get_current_tab (browser));
}

static void
_action_search_notify_current_item (GtkAction*     action,
                                    GParamSpec*    pspec,
                                    MidoriBrowser* browser)
{
    MidoriSearchAction* search_action;
    KatzeItem* item;
    guint idx;

    search_action = MIDORI_SEARCH_ACTION (action);
    item = midori_search_action_get_current_item (search_action);
    if (item)
        idx = katze_array_get_item_index (browser->search_engines, item);
    else
        idx = 0;

    g_object_set (browser->settings, "last-web-search", idx, NULL);
    browser->last_web_search = idx;
}

static void
_action_search_notify_default_item (GtkAction*     action,
                                    GParamSpec*    pspec,
                                    MidoriBrowser* browser)
{
    MidoriSearchAction* search_action;
    KatzeItem* item;

    search_action = MIDORI_SEARCH_ACTION (action);
    item = midori_search_action_get_default_item (search_action);
    if (item)
        g_object_set (browser->settings, "location-entry-search",
                      katze_item_get_uri (item), NULL);
}

static void
_action_search_focus_out (GtkAction*     action,
                          MidoriBrowser* browser)
{
    if ((gtk_widget_get_visible (browser->statusbar)
            && !browser->show_navigationbar)
            || midori_browser_is_fullscreen (browser))
    {
        gtk_widget_hide (browser->navigationbar);
    }
}
#endif

static void
midori_browser_bookmark_open_activate_cb (GtkWidget*     menuitem,
                                          MidoriBrowser* browser)
{
    KatzeItem* item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    midori_browser_open_bookmark (browser, item);
}

static void
midori_browser_bookmark_open_in_tab_activate_cb (GtkWidget*     menuitem,
                                                 MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* uri;

    item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    if (KATZE_IS_ARRAY (item))
    {
        KatzeItem* child;
        KatzeArray* array;

        array = midori_bookmarks_db_query_recursive (browser->bookmarks,
            "*", "parentid = %q", katze_item_get_meta_string (item, "id"), FALSE);

        KATZE_ARRAY_FOREACH_ITEM (child, KATZE_ARRAY (array))
        {
            if ((uri = katze_item_get_uri (child)) && *uri)
            {
                GtkWidget* view = midori_browser_add_item (browser, child);
                midori_browser_set_current_tab_smartly (browser, view);
            }
        }
        g_object_unref (G_OBJECT (array));
    }
    else
    {
        if ((uri = katze_item_get_uri (item)) && *uri)
        {
            GtkWidget* view = midori_browser_add_item (browser, item);
            midori_browser_set_current_tab_smartly (browser, view);
        }
    }
}

static void
midori_browser_bookmark_open_in_window_activate_cb (GtkWidget*     menuitem,
                                                    MidoriBrowser* browser)
{
    KatzeItem* item;
    const gchar* uri;

    item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    uri = katze_item_get_uri (item);
    midori_view_new_window_cb (NULL, uri, browser);
}

static void
midori_browser_bookmark_edit_activate_cb (GtkWidget* menuitem,
                                          GtkWidget* widget)
{
    MidoriBrowser* browser = midori_browser_get_for_widget (widget);
    KatzeItem* item = g_object_get_data (G_OBJECT (menuitem), "KatzeItem");

    if (KATZE_ITEM_IS_BOOKMARK (item))
        midori_browser_edit_bookmark_dialog_new (browser, item, FALSE, FALSE, widget);
    else
        midori_browser_edit_bookmark_dialog_new (browser, item, FALSE, TRUE, widget);
}

static void
midori_browser_bookmark_delete_activate_cb (GtkWidget*     menuitem,
                                            MidoriBrowser* browser)
{
    KatzeItem* item;
    GtkAction* action = _action_by_name (browser, "Location");

    item = (KatzeItem*)g_object_get_data (G_OBJECT (menuitem), "KatzeItem");
    midori_bookmarks_db_remove_item (browser->bookmarks, item);
    midori_location_action_set_secondary_icon_tooltip (
                MIDORI_LOCATION_ACTION (action), STOCK_UNBOOKMARK, _("Add to Bookmarks bar"));  //add by zgh 1224

    //zghtodo 更新书签菜单项
    GtkWidget* proxy = (GtkWidget*)g_object_get_data (G_OBJECT (menuitem), "Proxy");
    GtkWidget* menu = gtk_widget_get_parent(proxy);
    if (GTK_IS_MENU_SHELL (menu))
    {
        GtkAction *bookmarkAction = gtk_action_group_get_action(browser->action_group, "Bookmarks");
        KatzeArray* array = katze_array_action_get_array (KATZE_ARRAY_ACTION(bookmarkAction));
        _action_bookmarks_populate_folder(KATZE_ARRAY_ACTION(bookmarkAction), (GtkMenuShell *)menu, array, browser);
    }
}

static void
midori_browser_bookmark_popup (GtkWidget*      widget,
                               GdkEventButton* event,
                               KatzeItem*      item,
                               MidoriBrowser*  browser)
{
    MidoriContextAction* menu = midori_context_action_new ("BookmarkContextMenu", NULL, NULL, NULL);
    if (KATZE_ITEM_IS_FOLDER (item))
    {
        gint child_bookmarks_count = midori_bookmarks_db_count_recursive (browser->bookmarks,
            "uri <> ''", NULL, item, FALSE);

        GtkAction* action = gtk_action_new ("BookmarkOpenAllTabs", _("Open all in _Tabs"), NULL, STOCK_TAB_NEW);
        gtk_action_set_sensitive (action, child_bookmarks_count > 0);
        g_object_set_data (G_OBJECT (action), "KatzeItem", item);
        g_signal_connect (action, "activate",
            G_CALLBACK (midori_browser_bookmark_open_in_tab_activate_cb), browser);
        midori_context_action_add (menu, action);
    }
    else
    {
        GtkAction* action = gtk_action_new ("BookmarkOpen", NULL, NULL, GTK_STOCK_OPEN);
        gtk_action_set_sensitive (action, katze_item_get_uri (item) != NULL);
        g_object_set_data (G_OBJECT (action), "KatzeItem", item);
        g_signal_connect (action, "activate",
            G_CALLBACK (midori_browser_bookmark_open_activate_cb), browser);
        midori_context_action_add (menu, action);
        action = gtk_action_new ("BookmarkOpenTab", NULL, NULL, STOCK_TAB_NEW);
        gtk_action_set_sensitive (action, katze_item_get_uri (item) != NULL);
        g_object_set_data (G_OBJECT (action), "KatzeItem", item);
        g_signal_connect (action, "activate",
            G_CALLBACK (midori_browser_bookmark_open_in_tab_activate_cb), browser);
        midori_context_action_add (menu, action);
        action = gtk_action_new ("BookmarkOpenWindow", _("Open in New _Window"), NULL, STOCK_WINDOW_NEW);
        gtk_action_set_sensitive (action, katze_item_get_uri (item) != NULL);
        g_object_set_data (G_OBJECT (action), "KatzeItem", item);
        g_signal_connect (action, "activate",
            G_CALLBACK (midori_browser_bookmark_open_in_window_activate_cb), browser);
        midori_context_action_add (menu, action);
    }

    midori_context_action_add (menu, NULL);
    GtkAction* action = gtk_action_new ("BookmarkEdit", NULL, NULL, GTK_STOCK_EDIT);
    gtk_action_set_sensitive (action, !KATZE_ITEM_IS_SEPARATOR (item));
    g_object_set_data (G_OBJECT (action), "KatzeItem", item);
    g_signal_connect (action, "activate",
        G_CALLBACK (midori_browser_bookmark_edit_activate_cb), browser);
    midori_context_action_add (menu, action);
    action = gtk_action_new ("BookmarkDelete", NULL, NULL, GTK_STOCK_DELETE);
    g_object_set_data (G_OBJECT (action), "KatzeItem", item);
    g_object_set_data (G_OBJECT (action), "Proxy", widget);
    g_signal_connect (action, "activate",
        G_CALLBACK (midori_browser_bookmark_delete_activate_cb), browser);
    midori_context_action_add (menu, action);

    GtkMenu* context_menu = midori_context_action_create_menu (menu, NULL, FALSE);
    katze_widget_popup (widget, context_menu, event, KATZE_MENU_POSITION_CURSOR);
}

static gboolean
midori_browser_menu_button_press_event_cb (GtkWidget*      toolitem,
                                           GdkEventButton* event,
                                           MidoriBrowser*  browser)
{
    if (event->button != 3)
        return FALSE;

    /* GtkMenuBar catches button events on children with submenus,
       so we need to see if the actual widget is the menubar, and if
       it is an item, we forward it to the actual widget. */
    if ((GTK_IS_BOX (toolitem) || GTK_IS_MENU_BAR (toolitem)))
    {
        if (gtk_widget_get_window (toolitem) != event->window)
            return FALSE;

        midori_browser_toolbar_popup_context_menu_cb (
            GTK_IS_BIN (toolitem) && gtk_bin_get_child (GTK_BIN (toolitem)) ?
                gtk_widget_get_parent (toolitem) : toolitem,
            event->x, event->y, event->button, browser);
        return TRUE;
    }
    else if (GTK_IS_MENU_ITEM (toolitem))
    {
        gboolean handled;
        g_signal_emit_by_name (toolitem, "button-press-event", event, &handled);
        return handled;
    }
    return FALSE;
}

static void
_action_bookmark_add_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
    GtkWidget* proxy = NULL;
    GSList* proxies = gtk_action_get_proxies (action);
    for (; proxies != NULL; proxies = g_slist_next (proxies))
    if (GTK_IS_TOOL_ITEM (proxies->data))
    {
        proxy = proxies->data;
        break;
    }

    if (g_str_equal (gtk_action_get_name (action), "BookmarkFolderAdd"))
        midori_browser_edit_bookmark_dialog_new (browser, NULL, TRUE, TRUE, proxy);
    else
    {
	//modified by wangyl 2015.7.17 to solve a bug
        gboolean result = midori_browser_edit_bookmark_dialog_new (browser, NULL, TRUE, FALSE, proxy);
	if (result == true){
	GtkAction* action1 = _action_by_name (browser, "Location");
        midori_location_action_set_secondary_icon_tooltip (
                MIDORI_LOCATION_ACTION (action1), STOCK_BOOKMARKED, _("Bookmark already exist"));
	}
    }
}

static void
_action_bookmarks_import_from_local_activate (GtkAction*     action,
                                   MidoriBrowser* browser)
{
    typedef struct
    {
        const gchar* path;
        const gchar* name;
        const gchar* icon;
    } BookmarkClient;
    static const BookmarkClient bookmark_clients[] = {
        { ".local/share/data/Arora/bookmarks.xbel", N_("Arora"), "arora" },
        { ".kazehakase/bookmarks.xml", N_("Kazehakase"), "kazehakase-icon" },
        { ".opera/bookmarks.adr", N_("Opera"), "opera" },
        { ".kde/share/apps/konqueror/bookmarks.xml", N_("Konqueror"), "konqueror" },
        { ".gnome2/epiphany/bookmarks.rdf", N_("Epiphany"), "epiphany" },
        { ".mozilla/firefox/*/bookmarks.html", N_("Firefox (%s)"), "firefox" },
        { ".config/midori/bookmarks.xbel", N_("Midori 0.2.6"), "midori" },
    };

    GtkWidget* dialog;
    GtkWidget* content_area;
    GtkSizeGroup* sizegroup;
    GtkWidget* hbox;
    GtkWidget* label;
    GtkWidget* combo;
    GtkComboBox* combobox;
    GtkListStore* model;
    GtkCellRenderer* renderer;
    GtkWidget* combobox_folder;
    gint icon_width = 16;
    guint i;
    KatzeArray* bookmarks;

    dialog = gtk_dialog_new_with_buttons (
        _("Import bookmarks…"), GTK_WINDOW (browser),
        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        _("_Import bookmarks"), GTK_RESPONSE_ACCEPT,
        NULL);
    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_window_set_icon_name (GTK_WINDOW (dialog), STOCK_BOOKMARKS);

    gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
    gtk_container_set_border_width (GTK_CONTAINER (content_area), 5);
    sizegroup =  gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    label = gtk_label_new_with_mnemonic (_("_Application:"));
    gtk_size_group_add_widget (sizegroup, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    model = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING,
                                   G_TYPE_STRING, G_TYPE_INT);
    combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (model));
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "icon-name", 1);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "width", 3);
    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "text", 0);
    combobox = GTK_COMBO_BOX (combo);
    gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (browser)),
                                       GTK_ICON_SIZE_MENU, &icon_width, NULL);
    for (i = 0; i < G_N_ELEMENTS (bookmark_clients); i++)
    {
        const gchar* location = bookmark_clients[i].path;
        const gchar* client = bookmark_clients[i].name;
        gchar* path = NULL;

        /* Interpret * as 'any folder' */
        if (strchr (location, '*') != NULL)
        {
            gchar** parts = g_strsplit (location, "*", 2);
            GDir* dir;
            path = g_build_filename (g_get_home_dir (), parts[0], NULL);
            if ((dir = g_dir_open (path, 0, NULL)))
            {
                const gchar* name;
                while ((name = g_dir_read_name (dir)))
                {
                    gchar* file = g_build_filename (path, name, parts[1], NULL);
                    if (g_access (file, F_OK) == 0)
                    {
                        /* If name is XYZ.Name, we use Name only */
                        gchar* real_name = strchr (name, '.');
                        gchar* display = strstr (_(client), "%s")
                            ? g_strdup_printf (_(client),
                                  real_name ? real_name + 1 : name)
                            : g_strdup (_(client));
                        gtk_list_store_insert_with_values (model, NULL, G_MAXINT,
                            0, display, 1, bookmark_clients[i].icon,
                            2, file, 3, icon_width, -1);
                        g_free (display);
                    }
                    g_free (file);
                }
                g_dir_close (dir);
            }
            g_free (path);
            g_strfreev (parts);
            continue;
        }

        path = g_build_filename (g_get_home_dir (), location, NULL);
        if (g_access (path, F_OK) == 0)
            gtk_list_store_insert_with_values (model, NULL, G_MAXINT,
                0, _(client), 1, bookmark_clients[i].icon,
                2, path, 3, icon_width, -1);
        g_free (path);
    }

    gtk_list_store_insert_with_values (model, NULL, G_MAXINT,
        0, _("Import from XBEL or HTML file"), 1, NULL, 2, NULL, 3, icon_width, -1);
    gtk_combo_box_set_active (combobox, 0);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, TRUE, 0);

    combobox_folder = midori_bookmark_folder_button_new (browser->bookmarks, 0);
    gtk_box_pack_start (GTK_BOX (content_area), combobox_folder, FALSE, TRUE, 0);
    gtk_widget_show_all (content_area);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    if (midori_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        GtkTreeIter iter;
        gchar* path = NULL;
        gint64 selected;
        GError* error;

        if (gtk_combo_box_get_active_iter (combobox, &iter))
            gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 2, &path, -1);
        selected = midori_bookmark_folder_button_get_active (combobox_folder);

        gtk_widget_destroy (dialog);
        if (!path)
        {
            GtkWidget* file_dialog;

            file_dialog = (GtkWidget*)midori_file_chooser_dialog_new (_("Import from a file"),
                GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_OPEN);
            if (midori_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_OK)
                path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));
            gtk_widget_destroy (file_dialog);
        }

        error = NULL;
        bookmarks = katze_array_new (KATZE_TYPE_ARRAY);
        if (path && !midori_array_from_file (bookmarks, path, NULL, &error))
        {
            sokoke_message_dialog (GTK_MESSAGE_ERROR,
                _("Failed to import bookmarks"),
                error ? error->message : "", FALSE);
            if (error)
                g_error_free (error);
        }
        midori_bookmarks_db_import_array (browser->bookmarks, bookmarks, selected);
        g_object_unref (bookmarks);
        g_free (path);
    }
    else
        gtk_widget_destroy (dialog);
}

#ifdef BOOKMARK_SYNC
static int winDownFlag;
static int winUpFlag;

void
midori_browser_bookmarks_delete_exist(MidoriBookmarksDb* bookmarks,
                                      KatzeArray*        array)
{
    GList* list;
    KatzeItem* item;

    g_return_if_fail (IS_MIDORI_BOOKMARKS_DB (bookmarks));
    g_return_if_fail (KATZE_IS_ARRAY (array));

    KATZE_ARRAY_FOREACH_ITEM_L (item, array, list)
    {
       if(midori_bookmarks_db_exist_by_uri(bookmarks, item->uri))
          katze_array_remove_item(array, item);
    }
    g_list_free (list);
}

//多线程
static void
handle_data_and_window()
{
   static int count = 0;
   static gint64 selected = -1;
   char* path = "bookmarks.xbel";

   GError* error = NULL;
   KatzeArray* bookmarks = katze_array_new (KATZE_TYPE_ARRAY);
   if (path && !midori_array_from_file (bookmarks, path, NULL, &error))
   {
      sokoke_message_dialog (GTK_MESSAGE_ERROR, 
			     _("Failed to import bookmarks"),
                             error ? error->message : "", FALSE);
      if (error)
         g_error_free (error);
   }
   MidoriApp *app = midori_app_get_default();
   MidoriBrowser *browser = midori_app_get_browser(app);
   //判断该条书签是否已存在在浏览器现有书签列表中
   midori_browser_bookmarks_delete_exist(browser->bookmarks, bookmarks);

   midori_bookmarks_db_import_array (browser->bookmarks, bookmarks, selected);
   if(0 == selected)selected = -1;
   else selected = 0;
   g_object_unref (bookmarks);
   remove(path);

   //提示用户
   if(1 == winDownFlag)
      system("notify-send 导入书签完成！");
   else if(2 == winDownFlag)
      system("notify-send 导入书签失败！");

   _action_set_sensitive (browser, "From_Server", TRUE);
   _action_set_label(browser, "From_Server", _("_From Server")); 
}

static void 
DownloadThread(void *dat)
{
   FILE *fp;
   MidoriApp *app = midori_app_get_default();
   MidoriBrowser *browser = midori_app_get_browser(app);
   gchar *path = "bookmarks.xbel";

   if((fp=fopen(/*"bookmarks.xbel"*/path,"w"))==NULL)
      return;
   char* username = NULL;
   g_object_get(browser->settings, "bookmark-user-name", &username, NULL);
   char* email = NULL;
   g_object_get(browser->settings, "bookmark-user-email", &email, NULL);
   char* password = NULL;
   g_object_get(browser->settings, "bookmark-user-password", &password, NULL);

   if("" == password)
   {
      GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                		 GTK_DIALOG_MODAL,
                                   		 GTK_MESSAGE_WARNING,
                                   		 GTK_BUTTONS_OK,						                                   									 "您还未登陆，请先登陆！!");
      gtk_dialog_run(GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
   }

   char* token;
   login(&token, username, email, password);

   char* stringRec = NULL;
   if(download(&stringRec, email, token))
   {
      winDownFlag = 1;
      fwrite (stringRec , strlen(stringRec), 1 , fp );
   }
   else
   {
      winDownFlag = 2;
   }
   if(NULL != token)
   {
      free(token);
      token = NULL;
   }
   fclose(fp);
   if(NULL != stringRec)
   {
      free(stringRec);
      stringRec= NULL;
   }

   pthread_kill(g_object_get_data(browser, "main-pthread"), SIGUSR2); //给主线程发送信号

   return;
}

static void
_action_bookmarks_import_from_server_activate (GtkAction*     action,
                                               MidoriBrowser* browser)
{
   _action_set_sensitive (browser, "From_Server", FALSE);
   _action_set_label(browser, "From_Server", _("importing...")); 

   int threadID = 0;
   int ret = 0;

   pthread_t mainThreadId = pthread_self();
   g_object_set_data(browser, "main-pthread", mainThreadId);
   signal(SIGUSR2, handle_data_and_window);
   ret = pthread_create(&threadID, NULL, DownloadThread, NULL);	//    g_thread_new(NULL,(GThreadFunc)download,&data);
} 

static void
handle_data_and_window_up()
{
   MidoriApp *app = midori_app_get_default();
   MidoriBrowser *browser = midori_app_get_browser(app);
   if(1 == winUpFlag)
      system("notify-send 导出书签完成！");
   else if(2 == winUpFlag)
      system("notify-send 导出书签失败！");
   _action_set_sensitive (browser, "From_Server2", TRUE);
   _action_set_label(browser, "From_Server2", _("_From Server2")); 
}

static void 
uploadThread(void *dat)
{
    gchar* path = "cdos-bookmarks.xbel";

    MidoriApp *app = midori_app_get_default();
    MidoriBrowser *browser = midori_app_get_browser(app);

    //获取要上传的文件指针   
    FILE* r_file = fopen(path, "rb");   
    if (0 == r_file)   
    {   
       return;   
    }       
    char* email = NULL;
    g_object_get(browser->settings, "bookmark-user-email", &email, NULL);
    char* username = NULL;
    g_object_get(browser->settings, "bookmark-user-name", &username, NULL);
    char* password = NULL;
    g_object_get(browser->settings, "bookmark-user-password", &password, NULL);
    if("" == password)
    {
       GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                		  GTK_DIALOG_MODAL,
                                   		  GTK_MESSAGE_WARNING,
                                   	          GTK_BUTTONS_OK,									                                   						  "您还未登陆，请先登陆！!");
       gtk_dialog_run(GTK_DIALOG (dialog));
       gtk_widget_destroy (dialog);
    }
    char* token = NULL;
    login(&token, username, email, password);

    // 获取文件大小   
    fseek(r_file, 0, 2);   
    int file_size = ftell(r_file);   
    rewind(r_file);   
    char* buffer = NULL;
    buffer = (char*)malloc((file_size + 1)*sizeof(char));
    buffer[file_size] ='\0';
    fread (buffer, file_size, 1, r_file);
    if(upload(buffer, email, token))
       winUpFlag = 1;    
   else   
       winUpFlag = 2;  
   free(buffer);
   if(NULL != token)
   {
      free(token);
      token = NULL;
   }
   pthread_kill(g_object_get_data(browser, "main-pthread"), SIGUSR1); //给主线程发送信号
   return;
}

static void
_action_bookmarks_export_from_server_activate (GtkAction*     action,
                                               MidoriBrowser* browser)
{
    _action_set_sensitive (browser, "From_Server2", FALSE);
			   _action_set_label(browser, "From_Server2", _("exporting...")); 

    GtkWidget* file_dialog;
    GtkFileFilter* filter;
    const gchar* format = "xbel";

    gchar* path = "cdos-bookmarks.xbel";
    GError* error;
    KatzeArray* bookmarks;
    GtkWidget *progress_bar;
    int threadID = 0;
    int ret = 0;

    error = NULL;
    bookmarks = midori_bookmarks_db_query_recursive (browser->bookmarks,"*", "parentid IS NULL", NULL, TRUE);
    if (!midori_array_to_file (bookmarks, path, format, &error))
    {
        sokoke_message_dialog (GTK_MESSAGE_ERROR,_("Failed to export bookmarks"), error ? error->message : "", FALSE);
        if (error)
            g_error_free (error);
    }
    g_object_unref (bookmarks);

    pthread_t mainThreadId = pthread_self();
    g_object_set_data(browser, "main-pthread", mainThreadId);
    signal(SIGUSR1, handle_data_and_window_up);

    ret = pthread_create(&threadID, NULL, uploadThread, NULL);
}

char* userNameApply = "";
char* nicknameApply = "";
char* emailApply = "";
char* userPasswordApply = "";
char* userPasswordApplyConform = "";

static void 
userNameApplyCallback(GtkEntry *entry, gpointer data) 
{
    if(NULL == entry)return;
    userNameApply =  gtk_entry_get_text(GTK_ENTRY(entry));
}

static void 
userPasswordApplyCallback(GtkEntry *entry, gpointer data)
{
    if(NULL == entry)return;
    userPasswordApply =  gtk_entry_get_text(GTK_ENTRY(entry));
}

static void 
nicknameApplyCallback(GtkEntry *entry, gpointer data)
{
    if(NULL == entry)return;
    nicknameApply =  gtk_entry_get_text(GTK_ENTRY(entry));
}

static void 
emailApplyCallback(GtkEntry *entry, gpointer data)
{
    if(NULL == entry)return;
    emailApply =  gtk_entry_get_text(GTK_ENTRY(entry));
}

static void 
userPasswordApplyConformCallback(GtkEntry *entry, gpointer data) 
{
   if(NULL == entry)return;
   userPasswordApplyConform =  gtk_entry_get_text(GTK_ENTRY(entry));
}

static void 
userRegisterApplyCallback(GtkEntry *entry, gpointer data)
{
   if(strcmp(userPasswordApply, userPasswordApplyConform))
   {
      GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                		 GTK_DIALOG_MODAL,
                                   		 GTK_MESSAGE_WARNING,
                                   		 GTK_BUTTONS_OK,
                                                 "password and conform password is not the same, please reset!");
      gtk_dialog_run(GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
   }
   else
   {
      gtk_widget_destroy(GTK_WIDGET(data));
      bool ret = registerUser/*(registerUser("qqqq", "bbb", "ccc@ddd.com", "asdfgh"));*/(userNameApply, nicknameApply, emailApply, userPasswordApply);
      if(1 == ret)
      {
         GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                		    GTK_DIALOG_MODAL,
                                   		    GTK_MESSAGE_WARNING,
                                   		    GTK_BUTTONS_OK,
                                   		    "user register success!");
   	 gtk_dialog_run(GTK_DIALOG (dialog));
	 gtk_widget_destroy (dialog);
      }
      else
      {
         GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                    GTK_DIALOG_MODAL,
                                   		    GTK_MESSAGE_WARNING,
                                   		    GTK_BUTTONS_OK,
                                   	            "user register failed!");
   	 gtk_dialog_run(GTK_DIALOG (dialog));
	 gtk_widget_destroy (dialog);
      }
   }
}

static void 
userRegisterCallback(GtkButton *buttons, gpointer data) 
{
   gtk_widget_destroy(GTK_WIDGET(data));

   GtkWidget *window;
   GtkWidget *button;
   GtkWidget *label;
   GtkWidget *widget;
   GtkGrid *grid;

   window = gtk_window_new (GTK_WINDOW_TOPLEVEL/*GTK_WINDOW_POPUP*/);
   gtk_window_set_resizable (GTK_WINDOW(window), FALSE);
   gtk_window_set_title (GTK_WINDOW (window), "用户注册");

   gtk_container_set_border_width (GTK_CONTAINER (window), 0);
   gtk_window_set_position(GTK_WINDOW (window),GTK_WIN_POS_CENTER);

   grid = (GtkGrid*)gtk_grid_new();//创建网格
   gtk_grid_set_row_spacing (grid, 8);

   gtk_grid_set_column_spacing (grid, 5);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 0, 1, 1);	
   label = gtk_label_new("用户名：");
   gtk_grid_attach( grid, label, 1, 1, 1, 1);
		widget = gtk_entry_new();

   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(userNameApplyCallback), NULL);
   gtk_grid_attach( grid, widget, 2, 1, 3, 1);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 5, 1, 1, 1);


   label = gtk_label_new("昵称：");
   gtk_grid_attach( grid, label, 1, 3, 1, 1);
		widget = gtk_entry_new();
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(nicknameApplyCallback), NULL);
   gtk_grid_attach( grid, widget, 2, 3, 3, 1);


   label = gtk_label_new("邮箱：");
   gtk_grid_attach( grid, label, 1, 4, 1, 1);
		widget = gtk_entry_new();
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(emailApplyCallback), NULL);
   gtk_grid_attach( grid, widget, 2, 4, 3, 1);


   label = gtk_label_new("密码：");
   gtk_grid_attach( grid, label, 1, 5, 1, 1);
   widget = gtk_entry_new();
   gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
   gtk_entry_set_invisible_char (GTK_ENTRY(widget), '*');
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(userPasswordApplyCallback), NULL);
   gtk_grid_attach( grid, widget, 2, 5, 3, 1);

   label = gtk_label_new("确认密码：");
   gtk_grid_attach( grid, label, 1, 6, 1, 1);

   widget = gtk_entry_new();
   gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
   gtk_entry_set_invisible_char (GTK_ENTRY(widget), '*');
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(userPasswordApplyConformCallback), NULL);
   gtk_grid_attach( grid, widget, 2, 6, 3, 1);

   button = gtk_button_new_with_label("注 册");
   g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(userRegisterApplyCallback), window);
   gtk_grid_attach(grid, button, 2, 7, 2, 1);

   gtk_container_add (GTK_CONTAINER (window),GTK_WIDGET(grid));

   gtk_widget_show_all(window);
}

static bool isLogin = 0;
char* userName = NULL;
char* userPassword = NULL;

static void 
userNameCallback(GtkEntry *entry, gpointer data) 
{
    if(NULL == entry)return;
    userName =  gtk_entry_get_text(GTK_ENTRY(entry));
}

static void 
userPasswordCallback(GtkEntry *entry, gpointer data)
{
    if(NULL == entry)return;
    userPassword =  gtk_entry_get_text(GTK_ENTRY(entry));
}

static void 
userEmailCallback(GtkEntry *entry, gpointer data)
{
    if(NULL == entry)return;
    BookmarkEmail =  gtk_entry_get_text(GTK_ENTRY(entry));
}

static void 
userLoginCallback(GtkButton *button, gpointer data) 
{
   MidoriApp *app = midori_app_get_default();
   MidoriBrowser *browser = midori_app_get_browser(app);

   if(1 == isLogin)
   {
      if((NULL == userName) || (NULL == userPassword))
      {
         g_object_get(browser->settings, "bookmark-user-name", &userName, NULL);
	 g_object_get(browser->settings, "bookmark-user-email", &BookmarkEmail, NULL);
      }
      if((NULL == userName) || (NULL == BookmarkEmail))
         return;
      char* token = NULL;
      login(&token, userName, BookmarkEmail, userPassword);
      if(strcmp(token, "check user is failed."))
      {
         g_object_set(browser->settings, "bookmark-user-email", BookmarkEmail, NULL);
	 g_object_set(browser->settings, "bookmark-user-name", userName, NULL);
	 g_object_set(browser->settings, "bookmark-user-password", userPassword, NULL);
      }
      if(NULL != token)
      {
         free(token);
	 token = NULL;
      }
   }
   else if(0 == isLogin)
   {
      bool bvalue = 0;
      g_object_set(browser->settings, "auto-login", bvalue, NULL);
      g_object_set(browser->settings, "bookmark-user-password", "", NULL);
   }
   gtk_widget_destroy(GTK_WIDGET(data));
}


static void 
autoLoginCallback(GtkToggleButton *togglebutton, gpointer data)
{
   MidoriApp *app = midori_app_get_default();
   MidoriBrowser *browser = midori_app_get_browser(app);

   bool bvalue = gtk_toggle_button_get_active(togglebutton); 
   g_object_set(browser->settings, "auto-login", bvalue, NULL);
   if(0 == bvalue)
      g_object_set(browser->settings, "bookmark-user-password", "", NULL);
} 

static void 
_action_bookmarks_sync_activate ( GtkAction*     action,
                                  MidoriBrowser* browser)
{
g_print("_action_bookmarks_sync_activate\n");
   GtkWidget *window;
   GtkWidget *button;
   GtkWidget *label;
   GtkWidget *widget;
   GtkGrid *grid;

   window = gtk_window_new (GTK_WINDOW_TOPLEVEL/*GTK_WINDOW_POPUP*/);
   gtk_window_set_resizable (GTK_WINDOW(window), FALSE);
   gtk_window_set_title (GTK_WINDOW (window), "用户登陆");

   gtk_container_set_border_width (GTK_CONTAINER (window), 0);
   gtk_window_set_position(GTK_WINDOW (window),GTK_WIN_POS_CENTER);

   bool autoLogin = 0;
   bool bvalue = 1;

   g_object_get(browser->settings, "auto-login", &bvalue, NULL);
		gchar *userNameStr = NULL;
		g_object_get(browser->settings, "bookmark-user-name", &userNameStr, NULL);
		gchar *userEmailStr = NULL;
		g_object_get(browser->settings, "bookmark-user-email", &userEmailStr, NULL);
		gchar *password = NULL;
		g_object_get(browser->settings, "bookmark-user-password", &password, NULL);
		if(bvalue && ("" != userNameStr) && ("" != userEmailStr) && ("" != password) )
			autoLogin = 1;

   grid = (GtkGrid*)gtk_grid_new();//创建网格

   gtk_grid_set_row_spacing (grid, 8);
   gtk_grid_set_column_spacing (grid, 5);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 0, 1, 1);	
   label = gtk_label_new("用户名：");

   gtk_grid_attach( grid, label, 1, 1, 1, 1);
   widget = gtk_entry_new();
   gchar *strval = NULL;
   g_object_get(browser->settings, "bookmark-user-name", &strval, NULL);
   gtk_entry_set_text(GTK_ENTRY(widget),strval);
   if(autoLogin)
      gtk_widget_set_sensitive(GTK_WIDGET(widget), false);
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(userNameCallback), NULL);
   gtk_grid_attach( grid, widget, 2, 1, 3, 1);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 5, 1, 1, 1);
   label = gtk_label_new("邮箱：");
   gtk_grid_attach( grid, label, 1, 2, 1, 1);
   widget = gtk_entry_new();
   g_object_get(browser->settings, "bookmark-user-email", &strval, NULL);
   gtk_entry_set_text(GTK_ENTRY(widget),strval);
   if(autoLogin)
      gtk_widget_set_sensitive(GTK_WIDGET(widget), false);
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(userEmailCallback), NULL);
   gtk_grid_attach( grid, widget, 2, 2, 3, 1);
   label = gtk_label_new("密码：");
   gtk_grid_attach( grid, label, 1, 3, 1, 1);
   widget = gtk_entry_new();
   if(autoLogin)
   {
      gtk_entry_set_text(GTK_ENTRY(widget), "******");
      gtk_widget_set_sensitive(GTK_WIDGET(widget), false);
   }
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(userPasswordCallback), NULL);
   gtk_grid_attach( grid, widget, 2, 3, 3, 1);
   gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
   gtk_entry_set_invisible_char (GTK_ENTRY(widget), '*');

   button = gtk_check_button_new_with_label("自动登陆");
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(autoLoginCallback), NULL);
   gtk_grid_attach( grid, button, 1, 4, 2, 1);
   if(bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   if(autoLogin)
   {
      button = gtk_button_new_with_label("注 销");
      isLogin = 0;
   }
   else
   {
      button = gtk_button_new_with_label("登 陆");
      isLogin = 1;
   }

   g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(userLoginCallback), window);
   gtk_grid_attach(grid, button, 1, 5, 2, 1);
   gchar* str = gtk_button_get_label(button);

   button = gtk_button_new_with_label("注 册");
   g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(userRegisterCallback), window);
   gtk_grid_attach(grid, button, 4, 5, 2, 1);
   gtk_container_add (GTK_CONTAINER (window),GTK_WIDGET(grid));
   gtk_widget_show_all(window);
}
#endif //BOOKMARK_SYNC

static void
_action_bookmarks_export_from_local_activate (GtkAction*     action,
                                              MidoriBrowser* browser)
{
    GtkWidget* file_dialog;
    GtkFileFilter* filter;
    const gchar* format;
    gchar* path = NULL;
    GError* error;
    KatzeArray* bookmarks;

    wrong_format:
    file_dialog = (GtkWidget*)midori_file_chooser_dialog_new (_("Save file as"),
        GTK_WINDOW (browser), GTK_FILE_CHOOSER_ACTION_SAVE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
                                       "bookmarks.xbel");
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("XBEL Bookmarks"));
    gtk_file_filter_add_mime_type (filter, "application/xml");
    gtk_file_filter_add_pattern (filter, "*.xbel");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_dialog), filter);
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Netscape Bookmarks"));
    gtk_file_filter_add_mime_type (filter, "text/html");
    gtk_file_filter_add_pattern (filter, "*.html");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_dialog), filter);
    if (midori_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_OK)
        path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));
    gtk_widget_destroy (file_dialog);

    if (path == NULL)
        return;

    if (g_str_has_suffix (path, ".xbel"))
        format = "xbel";
    else if (g_str_has_suffix (path, ".html"))
        format = "netscape";
    else
    {
        sokoke_message_dialog (GTK_MESSAGE_ERROR,
            _("Midori can only export to XBEL (*.xbel) and Netscape (*.html)"),
            "", TRUE);
        katze_assign (path, NULL);
        goto wrong_format;
    }

    error = NULL;
    bookmarks = midori_bookmarks_db_query_recursive (browser->bookmarks,
        "*", "parentid IS NULL", NULL, TRUE);
    if (!midori_array_to_file (bookmarks, path, format, &error))
    {
        sokoke_message_dialog (GTK_MESSAGE_ERROR,
            _("Failed to export bookmarks"), error ? error->message : "", FALSE);
        if (error)
            g_error_free (error);
    }
    g_object_unref (bookmarks);
    g_free (path);
}

// ZRL 暂时屏蔽搜索框功能
#if ENABLE_SEARCH_ACTION
static void
_action_manage_search_engines_activate (GtkAction*     action,
                                        MidoriBrowser* browser)
{
    static GtkWidget* dialog = NULL;

    if (!dialog)
    {
        dialog = midori_search_action_get_dialog (
            MIDORI_SEARCH_ACTION (_action_by_name (browser, "Search")));
        g_signal_connect (dialog, "destroy",
            G_CALLBACK (gtk_widget_destroyed), &dialog);
        gtk_widget_show (dialog);
    }
    else
        gtk_window_present (GTK_WINDOW (dialog));
}
#endif

static void
_action_clear_private_data_activate (GtkAction*     action,
                                     MidoriBrowser* browser)
{
    static GtkWidget* dialog = NULL;

    if (!dialog)
    {
        dialog = midori_private_data_get_dialog (browser);
        g_signal_connect (dialog, "destroy",
            G_CALLBACK (gtk_widget_destroyed), &dialog);
        gtk_widget_show (dialog);
    }
    else
        gtk_window_present (GTK_WINDOW (dialog));
}

static void
_action_inspect_page_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    WebKitWebView* web_view = WEBKIT_WEB_VIEW (midori_view_get_web_view (MIDORI_VIEW (view)));
    WebKitWebInspector* inspector = webkit_web_view_get_inspector (web_view);
    webkit_web_inspector_show (inspector);
}

static gchar* 
midori_get_pageinfo_time (gchar* year, gchar* month, gchar* day)
{

    if (!strcmp(month, "Jan")) month = "1";
    else if (!strcmp(month, "Feb")) month = "2";
    else if (!strcmp(month, "Mar")) month = "3";
    else if (!strcmp(month, "Apr")) month = "4";
    else if (!strcmp(month, "May")) month = "5";
    else if (!strcmp(month, "Jun")) month = "6";
    else if (!strcmp(month, "Jul")) month = "7";
    else if (!strcmp(month, "Aug")) month = "8";
    else if (!strcmp(month, "Sep")) month = "9";
    else if (!strcmp(month, "Oct")) month = "10";
    else if (!strcmp(month, "Nov")) month = "11";
    else  if (!strcmp(month, "Dec")) month = "12";
    else month = "-1";

    strcat (year, "/");
    strcat (year, month);
    strcat (year, "/");
    strcat (year, day);
    return year;
}

static void 
midori_browser_show_record (GtkWidget* button, MidoriBrowser* browser)
{
    gtk_widget_set_visible(GTK_WIDGET(browser->page_frame), !gtk_widget_get_visible(GTK_WIDGET(browser->page_frame)));
    if(gtk_widget_get_visible(GTK_WIDGET(browser->page_frame)))
    {
       //按钮置灰
       gtk_widget_set_sensitive (page_button, FALSE);
    }
}

static void 
midori_browser_pageinfo_button_cb(GtkWidget* view, gchar** data, GtkWidget* widget)
{
    if (!widget)
        return;
    if (strcmp(data[0], "unknown"))
        gtk_widget_set_sensitive (widget, TRUE);
}

static void 
midori_browser_pageinfo_labtext_cb(GtkWidget* view, gchar** data, GtkWidget* widget)
{
    if(!widget)
        return;
    if (strcmp(data[0], "unknown"))
        gtk_label_set_text (GTK_LABEL(widget), _("already put on record"));
    else
        gtk_label_set_text (GTK_LABEL(widget), _("not put on record"));
}

static void 
midori_browser_pageinfo_recordbox_cb (GtkWidget* view, gchar** data, GtkWidget* widget)
{
    if (!widget)
        return;
    GtkWidget *hbox, *title, *text;
    if (data && strcmp(data[0], "unknown"))
    {
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
        title = gtk_label_new (_("host unit"));
        gtk_misc_set_alignment (GTK_MISC (title), 0.0, 0.5);
        gtk_label_set_width_chars (GTK_LABEL (title), 16);
        text = gtk_label_new (data[0]);
        gtk_box_pack_start (GTK_BOX (hbox), title, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX(widget), hbox, FALSE, FALSE, 0);
        gtk_widget_show_all (hbox);

        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
        title = gtk_label_new (_("host unit type"));
        gtk_misc_set_alignment (GTK_MISC (title), 0.0, 0.5);
        gtk_label_set_width_chars (GTK_LABEL (title), 16);
        if (data[1])
            text = gtk_label_new (data[1]);
        else
            text = gtk_label_new ("");
        gtk_box_pack_start (GTK_BOX (hbox), title, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX(widget), hbox, FALSE, FALSE, 0);
        gtk_widget_show_all (hbox);
        
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
        title = gtk_label_new (_("licence number"));
        gtk_misc_set_alignment (GTK_MISC (title), 0.0, 0.5);
        gtk_label_set_width_chars (GTK_LABEL (title), 16);
        if (data[2])
            text = gtk_label_new (data[2]);
        else
            text = gtk_label_new ("");
        gtk_box_pack_start (GTK_BOX (hbox), title, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX(widget), hbox, FALSE, FALSE, 0);
        gtk_widget_show_all (hbox);
        
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
        title = gtk_label_new (_("website name"));
        gtk_misc_set_alignment (GTK_MISC (title), 0.0, 0.5);
        gtk_label_set_width_chars (GTK_LABEL (title), 16);
        if (data[3])
            text = gtk_label_new (data[3]);
        else
            text = gtk_label_new ("");
        gtk_box_pack_start (GTK_BOX (hbox), title, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX(widget), hbox, FALSE, FALSE, 0);
        gtk_widget_show_all (hbox);
        
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
        title = gtk_label_new (_("website homepage"));
        gtk_misc_set_alignment (GTK_MISC (title), 0.0, 0.5);
        gtk_label_set_width_chars (GTK_LABEL (title), 16);
        if (data[4])
            text = gtk_label_new (data[4]);
        else
            text = gtk_label_new ("");
        gtk_box_pack_start (GTK_BOX (hbox), title, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX(widget), hbox, FALSE, FALSE, 0);
        gtk_widget_show_all (hbox);
        
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
        title = gtk_label_new (_("audit time"));
        gtk_misc_set_alignment (GTK_MISC (title), 0.0, 0.5);
        gtk_label_set_width_chars (GTK_LABEL (title), 16);
        if (data[5])
            text = gtk_label_new (data[5]);
        else
            text = gtk_label_new ("");
        gtk_box_pack_start (GTK_BOX (hbox), title, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), text, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX(widget), hbox, FALSE, FALSE, 0);
        gtk_widget_show_all (hbox);
    }
}

//add by luyue 2015/3/10 start
static void 
midori_browser_show_certificate (GtkWidget* button, gchar* certificateData)
{
   get_single_certificate_data(certificateData);
   display_single_certificate();
}
 
static int
close_page_window (GtkWidget* window, MidoriBrowser* browser)
{
   if(page_isConnectSignal)
      g_object_disconnect (page_view,
                           "any_signal::website_data",
                            midori_browser_pageinfo_recordbox_cb, page_record_box,
                            "any_signal::website_data",
                            midori_browser_pageinfo_button_cb, page_button,
                            "any_signal::website_data",
                            midori_browser_pageinfo_labtext_cb, page_lab_signal_text,
                            NULL);
  return false;
}
//add end

void 
_action_pageinfo_activate ( GtkAction*     action,
                            MidoriBrowser* browser)
{
    GtkWidget *dialog, *content_area, *lab_title, *lab_text, *image, *hbox;
    page_isConnectSignal = false;
    const gchar *title = midori_view_get_display_title(MIDORI_VIEW ((midori_browser_get_current_tab (browser))));
    const gchar *uri = midori_browser_get_current_uri(browser); 
    if ( !strcmp (uri, "") )
    {
        dialog = gtk_message_dialog_new(NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("Web Page not exist"));
        g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);
        gtk_widget_show (dialog);
        return;
    }
    gchar *date;
    gchar protocol[10] = {0}, connect[1024+1] = {0};
    sscanf(uri, "%[^:]", protocol);
    sscanf(uri, "%*[^/]//%[^/]", connect);
    if (!strstr (protocol, "http"))
        strcpy (connect, uri);
    page_view = midori_browser_get_current_tab (browser);
    WebKitWebView * web_view = WEBKIT_WEB_VIEW (midori_view_get_web_view (MIDORI_VIEW (page_view)));
    WebKitWebResource  *resource = webkit_web_view_get_main_resource (web_view);
    WebKitURIResponse* response = webkit_web_resource_get_response(resource);
    gchar* type = NULL;
    if(response)
       type = webkit_uri_response_get_mime_type (response);
    gboolean isEncrypt = false;
    gchar *encrypt = g_strdup_printf (_("Connect Http %s"), connect);
    SoupMessageHeaders* headers = NULL;
    if(response)
        headers = webkit_uri_response_get_http_headers (response);
    if (!headers)
        date = "asdf, 31 Dec 2014";
    else
        date = soup_message_headers_get (headers, "Date");
    gchar year[16] = {0}, Month[4] = {0}, day[4] = {0};
    if (date)
    {
       sscanf(date, "%*s%s%s%s", day, Month, year);
       midori_get_pageinfo_time(year, Month, day);
    }
        
    GTlsCertificate* tls_cert;
    GTlsCertificateFlags tls_flags;
    gchar* hostname;
    void* request = NULL;

    midori_view_get_tls_info (MIDORI_VIEW (page_view), request, &tls_cert, &tls_flags, &hostname);
    
    if (tls_cert != NULL)
    {
       isEncrypt = true;
       encrypt = g_strdup_printf (_("Connect Https %s"), connect);
    }
    if(GTK_IS_WIDGET (browser->page_window))
    {
      gtk_widget_destroy(browser->page_window);
      browser->page_window = NULL;
    }
    browser->page_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title((GtkWindow *)browser->page_window, _("PageInfo"));
    gtk_window_set_transient_for( (GtkWindow *)browser->page_window, (GtkWindow *)browser );
    gtk_window_set_position((GtkWindow *)browser->page_window,GTK_WIN_POS_CENTER);
    lab_text = gtk_label_new (title);
    gtk_label_set_max_width_chars (GTK_LABEL(lab_text), 60);
    gtk_label_set_ellipsize (GTK_LABEL (lab_text), PANGO_ELLIPSIZE_END);
    image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
                                      GTK_ICON_SIZE_DIALOG);
    content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add (GTK_CONTAINER (browser->page_window), content_area);
    hbox =  gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), lab_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (lab_text);
    gtk_widget_show (image);
    gtk_widget_show (hbox);

    page_record_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width (GTK_CONTAINER (page_record_box), 10);
    gtk_widget_show (page_record_box);
    browser->page_frame = gtk_frame_new(NULL);
    gtk_container_add (GTK_CONTAINER(browser->page_frame), page_record_box);
    page_button = gtk_button_new_with_label(_("learn more"));
    g_signal_connect (page_button, "clicked", G_CALLBACK (midori_browser_show_record), browser);
    gtk_widget_show (page_button);

    gchar** website_record_array = midori_view_get_website_record(MIDORI_VIEW(page_view));
    if (website_record_array && website_record_array[0]) //如果不为NULL
    {   
       if (strcmp(website_record_array[0], "unknown"))
       {
            hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
            lab_title = gtk_label_new (_("host unit"));
            gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
            gtk_label_set_width_chars (GTK_LABEL (lab_title), 16);
            if (website_record_array[0])
                lab_text = gtk_label_new (website_record_array[0]);
            else
                lab_text = gtk_label_new ("");
            gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), lab_text, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX(page_record_box), hbox, FALSE, FALSE, 0);
            gtk_widget_show_all (hbox);
            
            hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
            lab_title = gtk_label_new (_("host unit type"));
            gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
            gtk_label_set_width_chars (GTK_LABEL (lab_title), 16);
            if (website_record_array[1])
                lab_text = gtk_label_new (website_record_array[1]);
            else
                lab_text = gtk_label_new ("");
            gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), lab_text, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX(page_record_box), hbox, FALSE, FALSE, 0);
            gtk_widget_show_all (hbox);

            hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
            lab_title = gtk_label_new (_("licence number"));
            gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
            gtk_label_set_width_chars (GTK_LABEL (lab_title), 16);
            if (website_record_array[2])
                lab_text = gtk_label_new (website_record_array[2]);
            else
                lab_text = gtk_label_new ("");
            gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), lab_text, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX(page_record_box), hbox, FALSE, FALSE, 0);
            gtk_widget_show_all (hbox);

            hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
            lab_title = gtk_label_new (_("website name"));
            gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
            gtk_label_set_width_chars (GTK_LABEL (lab_title), 16);
            if (website_record_array[3])
                lab_text = gtk_label_new (website_record_array[3]);
            else
                lab_text = gtk_label_new ("");
            gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), lab_text, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX(page_record_box), hbox, FALSE, FALSE, 0);
            gtk_widget_show_all (hbox);

            hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
            lab_title = gtk_label_new (_("website homepage"));
            gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
            gtk_label_set_width_chars (GTK_LABEL (lab_title), 16);
            if (website_record_array[4])
                lab_text = gtk_label_new (website_record_array[4]);
            else
                lab_text = gtk_label_new ("");
            gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), lab_text, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX(page_record_box), hbox, FALSE, FALSE, 0);
            gtk_widget_show_all (hbox);
            
            hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
            lab_title = gtk_label_new (_("audit time"));
            gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
            gtk_label_set_width_chars (GTK_LABEL (lab_title), 16);
            if (website_record_array[5])
                lab_text = gtk_label_new (website_record_array[5]);
            else
                lab_text = gtk_label_new ("");
            gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), lab_text, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX(page_record_box), hbox, FALSE, FALSE, 0);
            gtk_widget_show_all (hbox);
            page_lab_signal_text = gtk_label_new(_("already put on record"));
        }
        else
        {
            page_lab_signal_text = gtk_label_new(_("not put on record"));
            gtk_widget_set_sensitive (page_button, FALSE);
        }
    }
    else
    {
        page_isConnectSignal = true;
        gtk_widget_set_sensitive (page_button, FALSE);
        page_lab_signal_text = gtk_label_new(_("in query"));
        g_signal_connect (page_view, "website_data", G_CALLBACK(midori_browser_pageinfo_labtext_cb), page_lab_signal_text);
        g_signal_connect (page_view, "website_data", G_CALLBACK(midori_browser_pageinfo_button_cb), page_button);
        g_signal_connect (page_view, "website_data", G_CALLBACK(midori_browser_pageinfo_recordbox_cb), page_record_box);
    }
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    lab_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lab_title),
        "<span weight='bold' foreground='green' font_desc='10'>网站鉴定：</span>");
    gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_title), 10);
    gtk_label_set_ellipsize (GTK_LABEL (lab_title), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(hbox), GTK_WIDGET(page_lab_signal_text), FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX(hbox), GTK_WIDGET(page_button), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show_all (hbox);   
    gtk_box_pack_start (GTK_BOX (content_area), GTK_WIDGET(browser->page_frame), FALSE, FALSE, 0);
    //协议
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    lab_title = gtk_label_new(_("Protocol"));
    gtk_label_set_markup(GTK_LABEL(lab_title),
        "<span weight='bold' font_desc='10'>协议：</span>");
    gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_title), 10);
    gtk_label_set_ellipsize (GTK_LABEL (lab_title), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
    lab_text = gtk_label_new(protocol);
    gtk_box_pack_start (GTK_BOX(hbox), lab_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (lab_title);
    gtk_widget_show (lab_text);
    gtk_widget_show (hbox);
    
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    lab_title = gtk_label_new(_("Type"));
    gtk_label_set_markup(GTK_LABEL(lab_title),
        "<span weight='bold' font_desc='10'>类型：</span>");
    gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_title), 10);
    gtk_label_set_ellipsize (GTK_LABEL (lab_title), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
    lab_text = gtk_label_new(type);
    gtk_box_pack_start (GTK_BOX(hbox), lab_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (lab_title);
    gtk_widget_show (lab_text);
    gtk_widget_show (hbox);
    
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    lab_title = gtk_label_new(_("Connect"));
    gtk_label_set_markup(GTK_LABEL(lab_title),
        "<span weight='bold' font_desc='10'>连接：</span>");
    gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_title), 10);
    gtk_label_set_ellipsize (GTK_LABEL (lab_title), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
    lab_text = gtk_label_new(encrypt);
    gtk_box_pack_start (GTK_BOX(hbox), lab_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (lab_title);
    gtk_widget_show (lab_text);
    gtk_widget_show (hbox);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    lab_title = gtk_label_new(_("Domain address"));
    gtk_label_set_markup(GTK_LABEL(lab_title),
        "<span weight='bold' font_desc='10'>区域：</span>");
    gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_title), 10);
    gtk_label_set_ellipsize (GTK_LABEL (lab_title), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
    if (!strcmp(protocol, "file"))
        lab_text = gtk_label_new("Local");
    else
        lab_text = gtk_label_new ("Internet");
    gtk_box_pack_start (GTK_BOX(hbox), lab_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (lab_title);
    gtk_widget_show (lab_text);
    gtk_widget_show (hbox);
    
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    lab_title = gtk_label_new(_("Address"));
    gtk_label_set_markup(GTK_LABEL(lab_title),
        "<span weight='bold' font_desc='10'>地址：</span>");
    gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_title), 10);
    gtk_label_set_ellipsize (GTK_LABEL (lab_title), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
    lab_text = gtk_label_new(uri);
    gtk_label_set_max_width_chars (GTK_LABEL(lab_text), 60);
    gtk_label_set_ellipsize (GTK_LABEL (lab_text), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX(hbox), lab_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (lab_title);
    gtk_widget_show (lab_text);
    gtk_widget_show (hbox);
    
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    lab_title = gtk_label_new(_("Create time"));
    gtk_label_set_markup(GTK_LABEL(lab_title),
        "<span weight='bold' font_desc='10'>创建时间：</span>");
    gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_title), 10);
    gtk_label_set_ellipsize (GTK_LABEL (lab_title), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
    lab_text = gtk_label_new(year);
    gtk_box_pack_start (GTK_BOX(hbox), lab_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (lab_title);
    gtk_widget_show (lab_text);
    gtk_widget_show (hbox);
    
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    lab_title = gtk_label_new(_("Modify time"));
    gtk_label_set_markup(GTK_LABEL(lab_title),
        "<span weight='bold' font_desc='10'>修改时间：</span>");
    gtk_misc_set_alignment (GTK_MISC (lab_title), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_title), 10);
    gtk_label_set_ellipsize (GTK_LABEL (lab_title), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start (GTK_BOX (hbox), lab_title, FALSE, FALSE, 0);
    lab_text = gtk_label_new(year);
    gtk_misc_set_alignment (GTK_MISC (lab_text), 0.0, 0.5);
    gtk_label_set_width_chars (GTK_LABEL (lab_text), 60);
    gtk_box_pack_start (GTK_BOX(hbox), lab_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (lab_title);
    gtk_widget_show (lab_text);
    gtk_widget_show (hbox);
    
    //add by luyue 2015/3/10
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    GtkWidget *certificate_button = gtk_button_new_with_label(_("certificate"));
    gtk_widget_set_sensitive(certificate_button,isEncrypt);
    gtk_box_pack_end (GTK_BOX (hbox), certificate_button, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
    gtk_widget_show (certificate_button);
    gtk_widget_show (hbox);
    if (isEncrypt)
    {
        gchar* certificateData;
        g_object_get(tls_cert, "certificate-pem", &certificateData,NULL);
        g_signal_connect(certificate_button, "clicked", G_CALLBACK(midori_browser_show_certificate), certificateData);
    }       
    //add end
    gtk_widget_show (content_area);
    gtk_widget_show(browser->page_window);
    //close window
    g_signal_connect(G_OBJECT(browser->page_window),"delete_event", G_CALLBACK(close_page_window),browser);
}

static void
_action_tab_move_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    const gchar* name = gtk_action_get_name (action);
    gint new_pos;
    gint cur_pos = midori_browser_get_current_page (browser);
    GtkWidget* widget = midori_browser_get_nth_tab (browser, cur_pos);

    if (!strcmp (name, "TabMoveFirst"))
        new_pos = 0;
    else if (!strcmp (name, "TabMoveBackward"))
    {
        if (cur_pos > 0)
            new_pos = cur_pos - 1;
        else
            new_pos = midori_browser_get_n_pages (browser) - 1;
    }
    else if (!strcmp (name, "TabMoveForward"))
    {
        if (cur_pos == (midori_browser_get_n_pages (browser) - 1))
            new_pos = 0;
        else
            new_pos = cur_pos + 1;
    }
    else if (!strcmp (name, "TabMoveLast"))
        new_pos = midori_browser_get_n_pages (browser) - 1;
    else
        g_assert_not_reached ();

    midori_notebook_move (MIDORI_NOTEBOOK (browser->notebook), MIDORI_TAB (widget), new_pos);
    g_signal_emit (browser, signals[MOVE_TAB], 0, browser->notebook, cur_pos, new_pos);
    midori_notebook_set_index(MIDORI_NOTEBOOK (browser->notebook),new_pos);       //add by lyb
}

static void
_action_tab_previous_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
    gint n = midori_browser_get_current_page (browser);
    midori_browser_set_current_page (browser, n - 1);
}

static void
_action_tab_next_activate (GtkAction*     action,
                           MidoriBrowser* browser)
{
    /* Advance one tab or jump to the first one if we are at the last one */
    gint n = midori_browser_get_current_page (browser);
    if (n == midori_browser_get_n_pages (browser) - 1)
        n = -1;
    midori_browser_set_current_page (browser, n + 1);
}

static void
_action_tab_minimize_activate (GtkAction*     action,
                               MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    g_object_set (view, "minimized",
                  !katze_object_get_boolean (view, "minimized"), NULL);
}

static void
_action_tab_duplicate_activate (GtkAction*     action,
                                MidoriBrowser* browser)
{
    GtkWidget* view = g_object_get_data (G_OBJECT (action), "tab");
    if (view == NULL)
        view = midori_browser_get_current_tab (browser);
    midori_view_duplicate (MIDORI_VIEW (view));
}

static void
_action_tab_close_other_activate (GtkAction*     action,
                                  MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_get_current_tab (browser);
    GList* tabs = midori_browser_get_tabs (browser);
    for (; tabs; tabs = g_list_next (tabs))
    {
        if (tabs->data != view)
            midori_browser_close_tab (browser, tabs->data);
    }
    g_list_free (tabs);
}

static void
_action_about_activate (GtkAction*     action,
                        MidoriBrowser* browser)
{
    gchar* comments = g_strdup_printf ("%s\n%s",
        _("A Security web browser."),
        _("See about:version for version info."));
    GtkWidget* dialog = gtk_about_dialog_new ();
    GError *error = NULL;
    GdkPixbuf *logo = gdk_pixbuf_new_from_file(midori_paths_get_res_filename("about_logo.png"),&error);
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog),logo);
    g_object_set (dialog,
        "wrap-license", FALSE,
        "copyright", "Copyright © 2014-2015 cdosrepobuild <cdosrepobuild@iscas.ac.cn>",
        "transient-for", browser,
        "version", PACKAGE_VERSION,
        "comments", comments,
        NULL);
    g_free (comments);
    gtk_widget_show (dialog);
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy), dialog);
}

static void
_action_help_link_activate (GtkAction*     action,
                            MidoriBrowser* browser)
{
   system("/usr/bin/cdosfeedback -app -browser");
}

static gboolean
midori_browser_panel_timeout (GtkWidget* hpaned)
{
    return FALSE;
}

static void
midori_panel_notify_position_cb (GObject*       hpaned,
                                 GParamSpec*    pspec,
                                 MidoriBrowser* browser)
{
    if (!browser->panel_timeout)
        browser->panel_timeout = midori_timeout_add_seconds (5,
            (GSourceFunc)midori_browser_panel_timeout, hpaned, NULL);
}

static gboolean
midori_panel_cycle_child_focus_cb (GtkWidget*     hpaned,
                                   gboolean       reversed,
                                   MidoriBrowser* browser)
{
    /* Default cycle goes between all GtkPaned widgets.
       If focus is in the panel, focus the location as if it's a paned.
       If nothing is focussed, simply go to the location.
       Be sure to suppress the default because the signal can recurse. */
    GtkWidget* focus = gtk_window_get_focus (GTK_WINDOW (browser));
    if (gtk_widget_get_ancestor (focus, MIDORI_TYPE_PANEL)
     || !gtk_widget_get_ancestor (focus, GTK_TYPE_PANED))
    {
        g_signal_stop_emission_by_name (hpaned, "cycle-child-focus");
        midori_browser_activate_action (browser, "Location");
        return TRUE;
    }
    return FALSE;
}

static void
midori_panel_notify_page_cb (MidoriPanel*   panel,
                             GParamSpec*    pspec,
                             MidoriBrowser* browser)
{
    gint page = midori_panel_get_current_page (panel);
    if (page > -1)
        g_object_set (browser->settings, "last-panel-page", page, NULL);
}

static void
midori_panel_notify_show_titles_cb (MidoriPanel*   panel,
                                    GParamSpec*    pspec,
                                    MidoriBrowser* browser)
{
    gboolean show_titles = katze_object_get_boolean (panel, "show-titles");
    g_signal_handlers_block_by_func (browser->settings,
        midori_browser_settings_notify, browser);
    g_object_set (browser->settings, "compact-sidepanel", !show_titles, NULL);
    g_signal_handlers_unblock_by_func (browser->settings,
        midori_browser_settings_notify, browser);
}

//20141217 zlf add
static void
midori_panel_notify_open_in_window_cb(MidoriPanel*   panel,
                                      GParamSpec*    pspec,
                                      MidoriBrowser* browser)
{
    gboolean open_in_window = katze_object_get_boolean (panel, "open-panels-in-windows");
    if (open_in_window)//seperate in window
        midori_browser_show_panel_window(browser, true);
    else 
    {//show in browser window
       if(browser->sari_panel_windows)
       {
          GtkWidget* vpaned = gtk_widget_get_parent (browser->notebook);
          GtkWidget* hpaned = gtk_widget_get_parent (vpaned); //vbox
          gtk_paned_pack1 (GTK_PANED (hpaned), browser->panel, FALSE, FALSE);
          gtk_widget_hide(browser->sari_panel_windows);            
       }
    }
    return;
}

static gboolean
midori_panel_close_cb (MidoriPanel*   panel,
                       MidoriBrowser* browser)
{
    _action_set_active (browser, "Panel", FALSE);
    return FALSE;
}

static void
midori_browser_switched_tab_cb (MidoriNotebook* notebook,
                                GtkWidget*      old_widget,
                                MidoriView*     new_view,
                                MidoriBrowser*  browser)
{
    GtkAction* action;
    const gchar* text;
    const gchar* uri;

    gint currPageIndex = midori_browser_get_current_page(browser);
    gint  lastPageIndex = gtk_notebook_get_n_pages(MIDORI_NOTEBOOK(browser->notebook)->notebook); 
 //   if(lastPageIndex - 1 == currPageIndex && notebook->btn_end ==0 )
  //      new_view = gtk_notebook_get_nth_page(MIDORI_NOTEBOOK(browser->notebook)->notebook,lastPageIndex - 2);
    if (old_widget != NULL)
    {
        action = _action_by_name (browser, "Location");
        text = midori_location_action_get_text (MIDORI_LOCATION_ACTION (action));
        g_object_set_data_full (G_OBJECT (old_widget), "midori-browser-typed-text",
                                g_strdup (text), g_free);
    }

     if(!MIDORI_IS_VIEW (new_view) || new_view == MIDORI_VIEW (old_widget))return;
    //return_if_fail (new_view != MIDORI_VIEW (old_widget));

    uri = g_object_get_data (G_OBJECT (new_view), "midori-browser-typed-text");
    if (!uri)
        uri = midori_view_get_display_uri (new_view);
    midori_browser_set_title (browser, midori_view_get_display_title (new_view));
    action = _action_by_name (browser, "Location");
    midori_location_action_set_text (MIDORI_LOCATION_ACTION (action), uri);
    if (midori_paths_get_runtime_mode () == MIDORI_RUNTIME_MODE_APP)
        gtk_window_set_icon (GTK_WINDOW (browser), midori_view_get_icon (new_view));
    g_signal_emit (browser, signals[SWITCH_TAB], 0, old_widget, new_view);
    _midori_browser_set_statusbar_text (browser, new_view, NULL);
    _midori_browser_update_interface (browser, new_view);
    _midori_browser_update_progress (browser, new_view);
}

static void
midori_browser_notify_tab_cb (GtkWidget*     notebook,
                              GParamSpec*    pspec,
                              MidoriBrowser* browser)
{
    g_object_freeze_notify (G_OBJECT (browser));
    g_object_notify (G_OBJECT (browser), "uri");
    g_object_notify (G_OBJECT (browser), "title");
    g_object_notify (G_OBJECT (browser), "tab");
    g_object_thaw_notify (G_OBJECT (browser));
}

static void
midori_browser_tab_moved_cb (GtkWidget*     notebook,
                             MidoriView*    view,
                             guint          page_num,
                             MidoriBrowser* browser)
{
    KatzeItem* item = midori_view_get_proxy_item (view);
    katze_array_move_item (browser->proxy_array, item, page_num);
}

static void
midori_browser_notebook_create_window_cb (GtkWidget*     notebook,
                                          GtkWidget*     view,
                                          gint           x,
                                          gint           y,
                                          MidoriBrowser* browser)
{
    MidoriBrowser* new_browser;
    g_signal_emit (browser, signals[NEW_WINDOW], 0, NULL, &new_browser);
    if (new_browser)
    {
        gtk_window_move (GTK_WINDOW (new_browser), x, y);
        g_object_ref (view);
        midori_browser_disconnect_tab (browser, MIDORI_VIEW (view));
        midori_notebook_remove (MIDORI_NOTEBOOK (browser->notebook), MIDORI_TAB (view));
        midori_browser_add_tab (new_browser, view);
        g_object_unref (view);
    }
}

static void
midori_browser_notebook_new_tab_cb (GtkWidget*     notebook,
                                    MidoriBrowser* browser)
{
    GtkWidget* view = midori_browser_add_uri (browser, "about:new");
    midori_browser_set_current_tab (browser, view);
}

static void
midori_browser_notebook_context_menu_cb (MidoriNotebook*      notebook,
                                         MidoriContextAction* menu,
                                         MidoriBrowser*       browser)
{
    midori_context_action_add_action_group (menu, browser->action_group);
    midori_context_action_add (menu, NULL);
    midori_context_action_add_by_name (menu, "TabNew");
#if ENABLE_TRASH // ZRL 屏蔽撤销关闭书签功能
    midori_context_action_add_by_name (menu, "UndoTabClose");
#endif
}

static void
midori_browser_notebook_tab_context_menu_cb (MidoriNotebook*      notebook,
                                             MidoriTab*           tab,
                                             MidoriContextAction* menu,
                                             MidoriBrowser*       browser)
{
    midori_context_action_add_action_group (menu, browser->action_group);
    midori_context_action_add (menu, NULL);
    midori_context_action_add_by_name (menu, "TabNew");
#if ENABLE_TRASH // ZRL 屏蔽撤销关闭书签功能
    midori_context_action_add_by_name (menu, "UndoTabClose");
#endif
    if (MIDORI_IS_VIEW (tab))
    {
        GtkAction* action = gtk_action_new ("TabDuplicate", _("_Duplicate Current Tab"), NULL, NULL);
        g_object_set_data (G_OBJECT (action), "tab", tab);
        g_signal_connect (action, "activate",
            G_CALLBACK (_action_tab_duplicate_activate), browser);
        midori_context_action_add (menu, action);
    }
}

static const GtkActionEntry entries[] =
{
    //by zgh
    { "File", NULL, N_("_File") },
    { "WindowNew", STOCK_WINDOW_NEW,
        N_("New _Window"), "<Ctrl>n",
        N_("Open a new window"), G_CALLBACK (_action_window_new_activate) },
    { "TabNew", STOCK_TAB_NEW,
        NULL, "<Ctrl>t",
        N_("Open a new tab"), G_CALLBACK (_action_tab_new_activate) },
    { "PrivateBrowsing", NULL,
        N_("New P_rivate Browsing Window"), "<Ctrl><Shift>p",
        NULL, G_CALLBACK (_action_private_browsing_activate), },
    { "Open", GTK_STOCK_OPEN,
        NULL, "<Ctrl>o",
        N_("Open a file"), G_CALLBACK (_action_open_activate) },
    { "SaveAs", GTK_STOCK_SAVE_AS,
        N_("_Save Page As…"), "<Ctrl>s",
        N_("Save to a file"), G_CALLBACK (_action_save_as_activate) },
    { "CompactAdd", GTK_STOCK_ADD,
        NULL, NULL,
        NULL, G_CALLBACK (_action_compact_add_activate) },
    { "TabClose", GTK_STOCK_CLOSE,
        N_("_Close Tab"), "<Ctrl>w",
        N_("Close the current tab"), G_CALLBACK (_action_tab_close_activate) },
    { "WindowClose", NULL,
        N_("C_lose Window"), "<Ctrl><Shift>w",
        NULL, G_CALLBACK (_action_window_close_activate) },
    { "Print", GTK_STOCK_PRINT,
        NULL, "<Ctrl>p",
        N_("Print the current page"), G_CALLBACK (_action_print_activate) },
    { "Quit", GTK_STOCK_QUIT,
        N_("Close a_ll Windows"), "<Ctrl><Shift>q",
        NULL, G_CALLBACK (_action_quit_activate) },

    { "Edit", NULL, N_("_Edit"), NULL, NULL, G_CALLBACK (_action_edit_activate) },
    { "Cut", GTK_STOCK_CUT,
        NULL, "<Ctrl>x",
        NULL, G_CALLBACK (_action_cut_activate) },
    { "Copy", GTK_STOCK_COPY,
        NULL, "<Ctrl>c",
        NULL, G_CALLBACK (_action_copy_activate) },
    { "Paste", GTK_STOCK_PASTE,
        NULL, "<Ctrl>v",
        NULL, G_CALLBACK (_action_paste_activate) },
    { "Delete", GTK_STOCK_DELETE,
        NULL, "Delete",
        NULL, G_CALLBACK (_action_delete_activate) },
    { "SelectAll", GTK_STOCK_SELECT_ALL,
        NULL, "<Ctrl>a",
        NULL, G_CALLBACK (_action_select_all_activate) },        
    { "Find", GTK_STOCK_FIND,
        N_("_Find…"), "<Ctrl>f",
        N_("Find a word or phrase in the page"), G_CALLBACK (_action_find_activate) },
    { "Preferences", GTK_STOCK_PREFERENCES,
        NULL, "<Ctrl><Shift>n",
        N_("Configure the application preferences"), G_CALLBACK (_action_preferences_activate) },

    { "View", NULL, N_("_View") },
    { "Toolbars", NULL, N_("_Toolbars") },
    { "Reload", GTK_STOCK_REFRESH,
        NULL, "<Ctrl>r",
        N_("Reload the current page"), G_CALLBACK (_action_reload_stop_activate) },
    { "ReloadUncached", GTK_STOCK_REFRESH,
        N_("Reload page without caching"), "<Ctrl><Shift>r",
        NULL, G_CALLBACK (_action_reload_stop_activate) },
    { "Stop", GTK_STOCK_STOP,
        NULL, "Escape",
        N_("Stop loading the current page"), G_CALLBACK (_action_reload_stop_activate) },
    { "ReloadStop", GTK_STOCK_STOP,
        NULL, "",
        N_("Reload the current page"), G_CALLBACK (_action_reload_stop_activate) },
    { "ZoomIn", GTK_STOCK_ZOOM_IN,
        NULL, "<Ctrl>plus",
        N_("Increase the zoom level"), G_CALLBACK (_action_zoom_activate) },
    { "ZoomOut", GTK_STOCK_ZOOM_OUT,
        NULL, "<Ctrl>minus",
        N_("Decrease the zoom level"), G_CALLBACK (_action_zoom_activate) },
    { "ZoomNormal", GTK_STOCK_ZOOM_100,
        NULL, "<Ctrl>0",
        NULL, G_CALLBACK (_action_zoom_activate) },
    { "Encoding", NULL, N_("_Encoding") },
    { "SourceView", NULL,
        N_("View So_urce"), "<Ctrl><Alt>U",
        NULL, G_CALLBACK (_action_source_view_activate) },
    { "SourceViewDom", NULL,
        N_("View _DOM Source"), "<Ctrl><Alt><Shift>U",
        NULL, G_CALLBACK (_action_source_view_dom_activate) },
    { "CaretBrowsing", NULL,
        N_("Ca_ret Browsing"), "F7",
        NULL, G_CALLBACK (_action_caret_browsing_activate) },
    { "Fullscreen", GTK_STOCK_FULLSCREEN,
        NULL, "F11",
        N_("Toggle fullscreen view"), G_CALLBACK (_action_fullscreen_activate) },
    { "Readable", NULL,
       N_("_Readable"), "<Ctrl><Alt>R",
        NULL, G_CALLBACK (_action_readable_activate) },

    { "Go", NULL, N_("_Go") },
    { "Back", GTK_STOCK_GO_BACK,
        NULL, "<Alt>Left",
        N_("Go back to the previous page"), G_CALLBACK (_action_navigation_activate) },
    { "Forward", GTK_STOCK_GO_FORWARD,
        NULL, "<Alt>Right",
        N_("Go forward to the next page"), G_CALLBACK (_action_navigation_activate) },
    { "Previous", GTK_STOCK_MEDIA_PREVIOUS,
        NULL, "<Alt><Shift>Left",
        /* i18n: Visit the previous logical page, ie. in a forum or blog */
        N_("Go to the previous sub-page"), G_CALLBACK (_action_navigation_activate) },
    { "Next", GTK_STOCK_MEDIA_NEXT,
        NULL, "<Alt><Shift>Right",
        /* i18n: Visit the following logical page, ie. in a forum or blog */
        N_("Go to the next sub-page"), G_CALLBACK (_action_navigation_activate) },
    { "NextForward", GTK_STOCK_MEDIA_NEXT,
        N_("Next or Forward"), "",
        N_("Go to the next sub-page or next page in history"), G_CALLBACK (_action_navigation_activate) },
    { "Homepage", GTK_STOCK_HOME,
        N_("_Homepage"), "<Alt>Home",
        N_("Go to your homepage"), G_CALLBACK (_action_navigation_activate) },
    //by sunh     
    { "DownloadDialog", STOCK_DOWNLOAD,
        NULL, "<Ctrl>l",
        N_("Transfers"), G_CALLBACK (midori_browser_actiave_transfer_in_window) },
    //by sunh end
	
    //add by zgh    20141210
    { "ManageHistorys", STOCK_HISTORY,
        N_("ManageHistorys"), "<Alt><Shift>h",
        N_("ManageHistorys"), G_CALLBACK (midori_browser_actiave_history_in_window/*_action_managehistorys_activate*/) },
    //add by zgh 20141218
    { "ManageBookmarks", STOCK_BOOKMARKS,
	N_("ManageBookmarks"), "<Alt><Shift>b",
	N_("ManageBookmarks"), G_CALLBACK (midori_browser_actiave_bookmark_in_window) },
    { "BookmarkAdd", STOCK_BOOKMARK_ADD,
        NULL, "<Ctrl>d",
        N_("Add a new bookmark"), G_CALLBACK (_action_bookmark_add_activate) },
    { "BookmarkFolderAdd", NULL,
        N_("Add a new _folder"), "",
        NULL, G_CALLBACK (_action_bookmark_add_activate) },

#ifdef BOOKMARK_SYNC //lxx, 20150612
    { "BookmarksImport", NULL, N_("_Import bookmarks…") },

    { "From_Server", NULL,
        N_("_From Server"), "",
        NULL, G_CALLBACK (_action_bookmarks_import_from_server_activate) },
    { "From_Local", NULL,
        N_("_From Local"), "",
        NULL, G_CALLBACK (_action_bookmarks_import_from_local_activate) },	

    { "BookmarksExport", NULL, N_("_Export bookmarks…") },
    { "From_Server2", NULL,
        N_("_From Server2"), "",
        NULL, G_CALLBACK (_action_bookmarks_export_from_server_activate) },
    { "From_Local2", NULL,
        N_("_From Local2"), "",
        NULL, G_CALLBACK (_action_bookmarks_export_from_local_activate) },
#else
    { "BookmarksImport", NULL,
        N_("_Import bookmarks…"), "",
        NULL, G_CALLBACK (_action_bookmarks_import_from_local_activate) },
    { "BookmarksExport", NULL,
        N_("_Export bookmarks…"), "",
        NULL, G_CALLBACK (_action_bookmarks_export_from_local_activate) },
#endif
// ZRL 暂时屏蔽搜索框功能
#if ENABLE_SEARCH_ACTION 
    { "ManageSearchEngines", GTK_STOCK_PROPERTIES,
        N_("_Manage Search Engines…"), "<Ctrl><Alt>s",
        NULL, G_CALLBACK (_action_manage_search_engines_activate) },
#endif
    { "ClearPrivateData", NULL,
        N_("_Clear Private Data…"), "<Ctrl><Shift>Delete",
        NULL, G_CALLBACK (_action_clear_private_data_activate) },
    { "InspectPage", NULL,
        N_("_Inspect Page"), "<Ctrl><Shift>i",
        NULL, G_CALLBACK (_action_inspect_page_activate) },
    { "PageInfo", NULL, //add by zgh 20141225
        N_("Page _Info"), "<Ctrl>i",       //<Shift>p change into  <Ctrl>i  by lyb
        NULL, G_CALLBACK (_action_pageinfo_activate) },
#ifdef BOOKMARK_SYNC
    { "Bookmarks_Sync", NULL, 
        N_("_Bookmarks Sync"), "<Ctrl>U",
        NULL, G_CALLBACK (_action_bookmarks_sync_activate) },
#endif
    { "TabPrevious", GTK_STOCK_GO_BACK,
        N_("_Previous Tab"), "<Ctrl>Page_Up",
        NULL, G_CALLBACK (_action_tab_previous_activate) },
    { "TabNext", GTK_STOCK_GO_FORWARD,
        N_("_Next Tab"), "<Ctrl>Page_Down",
        NULL, G_CALLBACK (_action_tab_next_activate) },
    { "TabMoveFirst", NULL, N_("Move Tab to _first position"), NULL,
        NULL, G_CALLBACK (_action_tab_move_activate) },
    { "TabMoveBackward", NULL, N_("Move Tab _Backward"), "<Ctrl><Shift>Page_Up",
        NULL, G_CALLBACK (_action_tab_move_activate) },
    { "TabMoveForward", NULL, N_("_Move Tab Forward"), "<Ctrl><Shift>Page_Down",
        NULL, G_CALLBACK (_action_tab_move_activate) },
    { "TabMoveLast", NULL, N_("Move Tab to _last position"), NULL,
        NULL, G_CALLBACK (_action_tab_move_activate) },
    { "TabMinimize", NULL,
        N_("Only show the Icon of the _Current Tab"), "",
        NULL, G_CALLBACK (_action_tab_minimize_activate) },
    { "TabDuplicate", NULL,
        N_("_Duplicate Current Tab"), "",
        NULL, G_CALLBACK (_action_tab_duplicate_activate) },
    { "TabCloseOther", NULL,
        N_("Close Ot_her Tabs"), "",
        NULL, G_CALLBACK (_action_tab_close_other_activate) },
    { "LastSession", NULL,
        N_("Open last _session"), NULL,
        NULL, NULL },

    { "Help", NULL, N_("_Help") },
    { "HelpBugs", NULL,
        N_("_Report a Problem…"), NULL,
        NULL, G_CALLBACK (_action_help_link_activate) },
    { "About", GTK_STOCK_ABOUT,
        NULL, "",
        NULL, G_CALLBACK (_action_about_activate) },
    { "Dummy", NULL, N_("_Tools") },
};

static const guint entries_n = G_N_ELEMENTS (entries);

static const GtkToggleActionEntry toggle_entries[] =
{
    { "Menubar", NULL,
        N_("_Menubar"), "",
        NULL, G_CALLBACK (_action_menubar_activate),
        FALSE },
    { "Navigationbar", NULL,
        N_("_Navigationbar"), "",
        NULL, G_CALLBACK (_action_navigationbar_activate),
        FALSE },
    { "Bookmarkbar", NULL,
        N_("_Bookmarkbar"), "",
        NULL, G_CALLBACK (_action_bookmarkbar_activate),
        FALSE },
    { "Statusbar", NULL,
        N_("_Statusbar"), "",
        NULL, G_CALLBACK (_action_statusbar_activate),
        FALSE },
};

static const guint toggle_entries_n = G_N_ELEMENTS (toggle_entries);

static const GtkRadioActionEntry encoding_entries[] =
{
    { "EncodingAutomatic", NULL,
        N_("_Automatic"), "",
        NULL, 1 },
    { "EncodingChinese", NULL,
        N_("Chinese Traditional (BIG5)"), "",
        NULL, 1 },
    { "EncodingChineseSimplified", NULL,
        N_("Chinese Simplified (GB18030)"), "",
        NULL, 1 },
    { "EncodingJapanese", NULL,
        /* i18n: A double underscore "__" is used to prevent the mnemonic */
        N_("Japanese (SHIFT__JIS)"), "",
        NULL, 1 },
    { "EncodingKorean", NULL,
        N_("Korean (EUC-KR)"), "",
        NULL, 1 },
    { "EncodingRussian", NULL,
        N_("Russian (KOI8-R)"), "",
        NULL, 1 },
    { "EncodingUnicode", NULL,
        N_("Unicode (UTF-8)"), "",
        NULL, 1 },
    { "EncodingWestern", NULL,
        N_("Western (ISO-8859-1)"), "",
        NULL, 1 },
};

static const guint encoding_entries_n = G_N_ELEMENTS (encoding_entries);

typedef struct {
     MidoriBrowser* browser;
     guint timeout;
} MidoriInactivityTimeout;

static gboolean
midori_inactivity_timeout (gpointer data)
{
    #ifdef HAVE_X11_EXTENSIONS_SCRNSAVER_H
    MidoriInactivityTimeout* mit = data;
    static Display* xdisplay = NULL;
    static XScreenSaverInfo* mit_info = NULL;
    static int has_extension = -1;
    int event_base, error_base;

    if (has_extension == -1)
    {
        GdkDisplay* display = gtk_widget_get_display (GTK_WIDGET (mit->browser));
        if (GDK_IS_X11_DISPLAY (display))
        {
            xdisplay = GDK_DISPLAY_XDISPLAY (display);
            has_extension = XScreenSaverQueryExtension (xdisplay, &event_base, &error_base);
        }
        else
            has_extension = 0;
    }

    if (has_extension)
    {
        if (!mit_info)
            mit_info = XScreenSaverAllocInfo ();
        XScreenSaverQueryInfo (xdisplay, RootWindow (xdisplay, 0), mit_info);
        if (mit_info->idle / 1000 > mit->timeout)
        {
            midori_private_data_clear_all (mit->browser);
            midori_browser_activate_action (mit->browser, "Homepage");
        }
    }
    #else
    /* TODO: Implement for other windowing systems */
    #endif

    return TRUE;
}

void
midori_browser_set_inactivity_reset (MidoriBrowser* browser,
                                     gint           inactivity_reset)
{
    if (inactivity_reset > 0)
    {
        MidoriInactivityTimeout* mit = g_new (MidoriInactivityTimeout, 1);
        mit->browser = browser;
        mit->timeout = inactivity_reset;
        midori_timeout_add_seconds (
            inactivity_reset, midori_inactivity_timeout, mit, NULL);
    }
}

static gboolean
midori_browser_window_state_event_cb (MidoriBrowser*       browser,
                                      GdkEventWindowState* event)
{
    MidoriWindowState window_state = MIDORI_WINDOW_NORMAL;
    if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
        window_state = MIDORI_WINDOW_MINIMIZED;
    else if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
        window_state = MIDORI_WINDOW_MAXIMIZED;
    else if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
        window_state = MIDORI_WINDOW_FULLSCREEN;
    g_object_set (browser->settings, "last-window-state", window_state, NULL);

    return FALSE;
}

static gboolean
midori_browser_alloc_timeout (MidoriBrowser* browser)
{
    GtkWidget* widget = GTK_WIDGET (browser);
    GdkWindowState state = gdk_window_get_state (gtk_widget_get_window (widget));

    if (!(state &
        (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN)))
    {
        GtkAllocation allocation;
        gtk_widget_get_allocation (widget, &allocation);
        if (allocation.width != browser->last_window_width)
        {
            browser->last_window_width = allocation.width;
            g_object_set (browser->settings,
                "last-window-width", browser->last_window_width, NULL);
        }
        if (allocation.height != browser->last_window_height)
        {
            browser->last_window_height = allocation.height;
            g_object_set (browser->settings,
                "last-window-height", allocation.height, NULL);
        }
    }

    browser->alloc_timeout = 0;
    return FALSE;
}

static void
midori_browser_size_allocate_cb (MidoriBrowser* browser,
                                 GtkAllocation* allocation)
{
    GtkWidget* widget = GTK_WIDGET (browser);

    if (!browser->alloc_timeout && gtk_widget_get_realized (widget))
    {
        gpointer last_page;

        if ((last_page = g_object_get_data (G_OBJECT (browser), "last-page")))
        {
            midori_panel_set_current_page (MIDORI_PANEL (browser->panel),
                GPOINTER_TO_INT (last_page));
            g_object_set_data (G_OBJECT (browser), "last-page", NULL);
        }

        browser->alloc_timeout = midori_timeout_add_seconds (5,
            (GSourceFunc)midori_browser_alloc_timeout, browser, NULL);
    }
}

static void 
closeWindowWarningCallback(GtkToggleButton *togglebutton, MidoriBrowser* browser)
{
   gboolean bvalue = 0;
   g_object_get(browser->settings, "close-window-warning", &bvalue, NULL);
   g_object_set(browser->settings, "close_window_warning", !bvalue,NULL);
}

static int
midori_browser_destroy_cb (MidoriBrowser* browser)
{
   char content_text[256];
   gint result;
   int tab_num;
   gboolean bvalue = 0;

   MidoriApp *app = midori_app_get_default();

   GList* tabs = midori_browser_get_tabs (browser);
   for (tab_num=0; tabs != NULL; tabs = g_list_next (tabs),tab_num++);      
   g_list_free (tabs);

   g_object_get(browser->settings, "close-window-warning", &bvalue, NULL);
   if(!bvalue || tab_num<2)
   {
      g_object_set_data (G_OBJECT (browser), "midori-browser-destroyed", (void*)1);
      if (G_UNLIKELY (browser->panel_timeout))
         g_source_remove (browser->panel_timeout);
      if (G_UNLIKELY (browser->alloc_timeout))
         g_source_remove (browser->alloc_timeout);

      /* Destroy panel first, so panels don't need special care */
      if(GTK_IS_WIDGET (browser->panel))
         gtk_widget_destroy (browser->panel);
      if (GTK_IS_WIDGET(browser->settings_dialog))
         gtk_widget_destroy (browser->settings_dialog);
      //zgh 销毁管理器窗体
      if(GTK_IS_WIDGET (browser->sari_panel_windows))
         gtk_widget_hide (browser->sari_panel_windows);
      if (midori_app_get_browsers_num(app) < 2)
         system("/usr/local/libexec/cdosbrowser/cdosbrowser_download quit &");
      return false; 
   }
   sprintf(content_text,"您即将关闭%d个标签页，确定要继续吗?",tab_num);
   GtkWidget *dialog = gtk_message_dialog_new((GtkWindow *)browser,
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_OK_CANCEL,
                                   content_text);
   GtkWidget *box = gtk_message_dialog_get_message_area ((GtkMessageDialog *)dialog);
   GtkWidget *widget = gtk_check_button_new_with_label("关闭多个标签页时警告我");
   gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
   gtk_widget_show (widget);
   gtk_widget_show (box);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
   g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(closeWindowWarningCallback), browser);
   result = gtk_dialog_run(GTK_DIALOG (dialog));
   if(result == GTK_RESPONSE_OK)
   {
      gtk_widget_destroy (dialog);
      g_object_set_data (G_OBJECT (browser), "midori-browser-destroyed", (void*)1);
      if (G_UNLIKELY (browser->panel_timeout))
         g_source_remove (browser->panel_timeout);
      if (G_UNLIKELY (browser->alloc_timeout))
         g_source_remove (browser->alloc_timeout);

      /* Destroy panel first, so panels don't need special care */
      if(GTK_IS_WIDGET (browser->panel))
         gtk_widget_destroy (browser->panel);
      if (GTK_IS_WIDGET(browser->settings_dialog))
         gtk_widget_destroy (browser->settings_dialog);
      //zgh 销毁管理器窗体
      if(GTK_IS_WIDGET (browser->sari_panel_windows))
         gtk_widget_hide (browser->sari_panel_windows);

      if (midori_app_get_browsers_num(app) < 2)
         system("/usr/local/libexec/cdosbrowser/cdosbrowser_download quit &");
      return false; 
   }
   else
   {
      gtk_widget_destroy (dialog);
      return true;          
   }
}

static const gchar* ui_markup =
    "<ui>"
        "<menubar>"
            "<menu action='File'>"
                "<menuitem action='WindowNew'/>"
                "<menuitem action='TabNew'/>"
                "<menuitem action='PrivateBrowsing'/>"
                "<separator/>"
                "<menuitem action='Open'/>"
                "<separator/>"

                "<menuitem action='SaveAs'/>"
                "<menuitem action='Print'/>"
                "<separator/>"
                "<menuitem action='Quit'/>"
            "</menu>"
            "<menu action='Edit'>"
                "<separator/>"
                "<menuitem action='Cut'/>"
                "<menuitem action='Copy'/>"
                "<menuitem action='Paste'/>"
                "<menuitem action='Delete'/>"
                "<separator/>"
                "<menuitem action='SelectAll'/>"
                "<separator/>"
                "<menuitem action='Find'/>"
                #ifndef G_OS_WIN32
                "<separator/>"
                "<menuitem action='Preferences'/>"
                #endif
            "</menu>"
            "<menu action='View'>"
                "<menu action='Toolbars'>"
                    "<menuitem action='Menubar'/>"
                    "<menuitem action='Bookmarkbar'/>"
                    "<menuitem action='Statusbar'/>"
                "</menu>"
                "<menuitem action='ZoomIn'/>"
                "<menuitem action='ZoomOut'/>"
                "<menuitem action='ZoomNormal'/>"
                "<separator/>"
                "<menu action='Encoding'>"
                    "<menuitem action='EncodingAutomatic'/>"
                    "<menuitem action='EncodingChinese'/>"
                    "<menuitem action='EncodingChineseSimplified'/>"
                    "<menuitem action='EncodingJapanese'/>"
                    "<menuitem action='EncodingKorean'/>"
                    "<menuitem action='EncodingRussian'/>"
                    "<menuitem action='EncodingUnicode'/>"
                    "<menuitem action='EncodingWestern'/>"
                "</menu>"
                "<menuitem action='SourceView'/>"
                "<menuitem action='Fullscreen'/>"
            "</menu>"
            //add history by zgh
            "<menu action='Historys'>"
                "<menuitem action='ManageHistorys'/>"
            "</menu>"
            "<menu action='Bookmarks'>"
                "<menuitem action='ManageBookmarks'/>"
                "<menuitem action='BookmarkAdd'/>"

#ifdef BOOKMARK_SYNC
            "<menu action='BookmarksImport'>"
                        "<menuitem action='From_Server'/>"
                        "<menuitem action='From_Local'/>"
                    "</menu>"
        "<menu action='BookmarksExport'>"
                    "<menuitem action='From_Server2'/>"
                    "<menuitem action='From_Local2'/>"
                "</menu>"
#else
                "<menuitem action='BookmarksImport'/>"
                "<menuitem action='BookmarksExport'/>"
#endif
            "</menu>"
            "<menuitem action='Tools'/>"
            "<menu action='Help'>"
                "<menuitem action='HelpBugs'/>"
                "<separator/>"
                "<menuitem action='About'/>"
        "</menu>"
        /* For accelerators to work all actions need to be used
           *somewhere* in the UI definition */
        /* These also show up in Unity's HUD */
        "<menu action='Dummy'>"
            "<menuitem action='TabMoveFirst'/>"
            "<menuitem action='TabMoveBackward'/>"
            "<menuitem action='TabMoveForward'/>"
            "<menuitem action='TabMoveLast'/>"
            "<menuitem action='BookmarkAdd'/>"
            "<menuitem action='BookmarkFolderAdd'/>"
// ZRL 暂时屏蔽搜索框功能
#if ENABLE_SEARCH_ACTION 
            "<menuitem action='ManageSearchEngines'/>"
#endif
            //add by lyb start
            "<menuitem action='TabClose'/>"
            "<menuitem action='WindowClose'/>"
            "<menuitem action='Stop'/>"
            "<menuitem action='Back'/>"
            "<menuitem action='Forward'/>"
            "<menuitem action='Reload'/>"
            "<menuitem action='Homepage'/>"
            //end
            "<menuitem action='ClearPrivateData'/>"
            "<menuitem action='TabPrevious'/>"
            "<menuitem action='DownloadDialog'/>"
            "<menuitem action='TabNext'/>"
            "<menuitem action='TabMinimize'/>"
            "<menuitem action='TabDuplicate'/>"
            "<menuitem action='TabCloseOther'/>"
            "<menuitem action='LastSession'/>"
#if ENABLE_TRASH
            "<menuitem action='UndoTabClose'/>"
            "<menuitem action='TrashEmpty'/>"
#endif
            "<menuitem action='Preferences'/>"
            "<menuitem action='InspectPage'/>"
            "<menuitem action='PageInfo'/>"  //zgh 20141225
#ifdef BOOKMARK_SYNC
            "<menuitem action='Bookmarks_Sync'/>"  
#endif
            "<menuitem action='ReloadUncached'/>"
            "<menuitem action='CaretBrowsing'/>"
            "</menu>"
        "</menubar>"
        "<toolbar name='toolbar_navigation'>"
        "</toolbar>"
    "</ui>";
// added by wangyl 2015/9/25 为了每次打开对话框快而且不影响浏览器打开的时间
static void 
midori_browser_settings_window_new_cb(MidoriBrowser* browser)
{
   if(!browser->settings_dialog)
   {
     browser->settings_dialog = browser_settings_window_new(browser->settings); 
     g_signal_connect(G_OBJECT(browser->settings_dialog), "delete-event",midori_browser_preferences_window_hide_cb , browser);
   }
   pthread_exit(0);
}

static void
midori_browser_realize_cb (GtkStyle*      style,
                           MidoriBrowser* browser)
{
    GdkScreen* screen = gtk_widget_get_screen (GTK_WIDGET (browser));
    if (screen)
    {
        GtkIconTheme* icon_theme = gtk_icon_theme_get_for_screen (screen);
        if (gtk_icon_theme_has_icon (icon_theme, "midori"))
            gtk_window_set_icon_name (GTK_WINDOW (browser), "midori");
        else
            gtk_window_set_icon_name (GTK_WINDOW (browser), MIDORI_STOCK_WEB_BROWSER);
    }
    int n;
    MidoriApp* app = midori_app_get_default();  
    n= midori_app_get_browsers_num(app);
    if(n>1)return;
    pthread_t ntid;
    int ret;
    ret = pthread_create(&ntid, NULL, midori_browser_settings_window_new_cb, browser);
}

// add by wangyl 2015.6.17 start
static bool 
isCdosbrowserDefault()
{
   gchar result[20] = {0};
   system("xdg-settings get default-web-browser > /tmp/.default-browser");
   FILE *fp = fopen("/tmp/.default-browser","r");
   fgets(result, 20, fp);
   fclose(fp);	
   if(0 == strncmp(result, "cdosbrowser.desktop", 19))
      return true;
   return false;
}

static void
midori_browser_infobar_response_cb(GtkWidget* infobar,
                                 gint       response,
                                 gpointer   data_object)
{
    void (*response_cb) (GtkWidget*, gint, gpointer);
    response_cb = g_object_get_data (G_OBJECT (infobar), "midori-infobar-cb1");
    if (response_cb != NULL)
        response_cb (infobar, response, data_object); 
    gtk_widget_destroy (infobar);
}

static GtkWidget*
midori_browser_add_info_bar (MidoriView*    view,
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
                      G_CALLBACK (midori_browser_infobar_response_cb), data_object);

    va_end (args);
    label = gtk_label_new (message);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_container_add (GTK_CONTAINER (content_area), label);
    gtk_widget_show_all (infobar);
    gtk_box_pack_start (GTK_BOX (view), infobar, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (view), infobar, 0);
    g_object_set_data (G_OBJECT (infobar), "midori-infobar-cb1", response_cb);
    if (data_object != NULL)
       g_object_set_data_full (G_OBJECT (infobar), "midori-infobar-da",
                               g_object_ref (data_object), g_object_unref);
    return infobar;
}

static void
midori_browser_install_response_cb (GtkWidget*     infobar,
                                    gint           response_id,
                                    MidoriBrowser* browser)
{
   switch (response_id) 
   {
      case GTK_RESPONSE_ACCEPT:
         system("xdg-settings set default-web-browser cdosbrowser.desktop");
	 break;
      case GTK_RESPONSE_CLOSE:
	 g_object_set(browser->settings,  "whether-query", 1,NULL);
         break;
      default:
	 break;
   }	 
}
 
static void 
midori_browser_pages_loaded_cb(MidoriBrowser* browser)
{
   gboolean value = katze_object_get_boolean(browser->settings, "whether-query");
   if(isCdosbrowserDefault() || value == TRUE)return;
   gtk_notebook_set_current_page(MIDORI_NOTEBOOK(browser->notebook)->notebook,0);
   GtkWidget* first_tab =  midori_browser_get_current_tab (browser);
   MidoriView* view = MIDORI_VIEW (first_tab);
   midori_browser_add_info_bar(view,GTK_MESSAGE_QUESTION,"是否将当前浏览器设为默认浏览器？",
		               G_CALLBACK (midori_browser_install_response_cb), browser,GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
		               GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,"不再询问",GTK_RESPONSE_CLOSE,NULL);
}
// add by wangyl 2015.6.17 end 

static void
midori_browser_set_history (MidoriBrowser* browser,
                            KatzeArray*    history)
{
    if (browser->history == history)
        return;

    if (history)
        g_object_ref (history);
    katze_object_assign (browser->history, history);
    katze_object_assign (browser->history_database, NULL);

    if (!history)
        return;

    //根据参数history，设置菜单栏中历史记录菜单的显示
    _action_set_visible (browser, "Historys", history != NULL);

    GError* error = NULL;
    browser->history_database = midori_history_database_new (NULL, &error);
    if (error != NULL)
    {
        g_printerr (_("Failed to initialize history: %s"), error->message);
        g_printerr ("\n");
        g_error_free (error);
        return;
    }
    g_object_set (_action_by_name (browser, "Location"), "history",
                  browser->history, NULL);
}

static void
midori_browser_accel_switch_tab_activate_cb (GtkAccelGroup*  accel_group,
                                             GObject*        acceleratable,
                                             guint           keyval,
                                             GdkModifierType modifiers)
{
    GtkAccelGroupEntry* entry;

    if ((entry = gtk_accel_group_query (accel_group, keyval, modifiers, NULL)))
    {
        gint n;
        MidoriBrowser* browser;
        GtkWidget* view;

        /* Switch to n-th tab. 9 and 0 go to the last tab. */
        n = keyval - GDK_KEY_0;
        browser = g_object_get_data (G_OBJECT (accel_group), "midori-browser");
        if ((view = midori_browser_get_nth_tab (browser, n < 9 ? n - 1 : -1)))
            midori_browser_set_current_tab (browser, view);
    }
}

static void
midori_browser_add_actions (MidoriBrowser* browser)
{
    /* 0,053 versus 0,002 compared to gtk_action_group_add_ API */
    guint i;
    GSList* group = NULL;
    for (i = 0; i < G_N_ELEMENTS (entries); i++)
    {
        GtkActionEntry entry = entries[i];
        GtkAction* action = gtk_action_new (entry.name,
            _(entry.label), _(entry.tooltip), entry.stock_id);
        if (entry.callback)
            g_signal_connect (action, "activate", entry.callback, browser);
        gtk_action_group_add_action_with_accel (browser->action_group,
            GTK_ACTION (action), entry.accelerator);
    }
    for (i = 0; i < G_N_ELEMENTS (toggle_entries); i++)
    {
        GtkToggleActionEntry entry = toggle_entries[i];
        GtkToggleAction* action = gtk_toggle_action_new (entry.name,
            _(entry.label), _(entry.tooltip), entry.stock_id);
        if (entry.is_active)
            gtk_toggle_action_set_active (action, TRUE);
        if (entry.callback)
            g_signal_connect (action, "activate", entry.callback, browser);
        gtk_action_group_add_action_with_accel (browser->action_group,
            GTK_ACTION (action), entry.accelerator);
    }
    for (i = 0; i < G_N_ELEMENTS (encoding_entries); i++)
    {
        GtkRadioActionEntry entry = encoding_entries[i];
        GtkRadioAction* action = gtk_radio_action_new (entry.name,
            _(entry.label), _(entry.tooltip), entry.stock_id, entry.value);
        if (i == 0)
        {
            group = gtk_radio_action_get_group (action);
            gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
            g_signal_connect (action, "changed",
                G_CALLBACK (_action_view_encoding_activate), browser);
        }
        else
        {
            gtk_radio_action_set_group (action, group);
            group = gtk_radio_action_get_group (action);
        }
        gtk_action_group_add_action_with_accel (browser->action_group,
            GTK_ACTION (action), entry.accelerator);
    }
}

static gboolean
midori_browser_idle (gpointer data)
{
    MidoriBrowser* browser = MIDORI_BROWSER (data);

    if (browser->bookmarkbar_populate)
    {
        midori_bookmarkbar_populate_idle (browser);

        browser->bookmarkbar_populate = FALSE;
    }

    return FALSE;
}

//zgh 20150319 修改工具栏图标
static void _midori_browser_set_action_icon_name(MidoriBrowser *browser, gchar* action_name, gchar* icon_name, char* label)
{
    GtkAction *action = gtk_action_group_get_action(browser->action_group, action_name);
    g_object_set (action, 
                  "stock-id", NULL,
                  "icon_name", icon_name,
                  "label", label,
                  NULL);
}

static void midori_browser_set_action_icon_name (MidoriBrowser *browser)
{
    _midori_browser_set_action_icon_name (browser, "Back", STOCK_GO_BACK, _("Go back"));
    _midori_browser_set_action_icon_name (browser, "Forward", STOCK_GO_FORWARD, _("go forward"));
    _midori_browser_set_action_icon_name (browser, "Homepage", STOCK_HOME_PAGE, _("homepage"));
    _midori_browser_set_action_icon_name (browser, "DownloadDialog", STOCK_DOWNLOAD, NULL);
    _midori_browser_set_action_icon_name (browser, "Preferences", STOCK_SETTING, _("preferences"));
    _midori_browser_set_action_icon_name (browser, "ReloadUncached", STOCK_REFRESH, NULL);
    _midori_browser_set_action_icon_name (browser, "CompactMenu", STOCK_COMPACTMENU, NULL);
}

static void
midori_browser_init (MidoriBrowser* browser)
{
    GtkWidget* vbox;
    GtkUIManager* ui_manager;
    GtkAccelGroup* accel_group;
    guint i;
    GClosure* accel_closure;
    GError* error;
    GtkAction* action;
    GtkSettings* gtk_settings;
    GtkWidget* hpaned;
    GtkWidget* vpaned;
    GtkWidget* scrolled;
    KatzeArray* dummy_array;

    browser->settings = midori_web_settings_new ();
    browser->proxy_array = katze_array_new (KATZE_TYPE_ARRAY);
    browser->bookmarks = NULL;
    browser->history = NULL;
    browser->history_database = NULL;
    browser->trash = NULL;
    browser->search_engines = NULL;
    browser->dial = NULL;
    browser->sari_panel_windows = NULL; //zgh 20150212
    browser->forward_url = NULL; //luyue 2015/3/17
    browser->page_window = NULL; //luyue 2015/4/10

    /* Setup the window metrics */
    g_signal_connect (browser, "realize",
                      G_CALLBACK (midori_browser_realize_cb), browser);
    g_signal_connect (browser, "pages_loaded",
                      G_CALLBACK (midori_browser_pages_loaded_cb), browser);//add by wangyl 2015.6.17
    g_signal_connect (browser, "window-state-event",
                      G_CALLBACK (midori_browser_window_state_event_cb), NULL);
    g_signal_connect (browser, "size-allocate",
                      G_CALLBACK (midori_browser_size_allocate_cb), NULL);
    g_signal_connect (browser, "delete_event",
                      G_CALLBACK (midori_browser_destroy_cb), NULL);
    gtk_window_set_role (GTK_WINDOW (browser), "browser");
    gtk_window_set_icon_name (GTK_WINDOW (browser), MIDORI_STOCK_WEB_BROWSER);
    #if GTK_CHECK_VERSION (3, 4, 0)
    #ifndef HAVE_GRANITE
    gtk_window_set_hide_titlebar_when_maximized (GTK_WINDOW (browser), TRUE);
    #endif
    #endif
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (browser), vbox);
    gtk_widget_show (vbox);

    /* Let us see some ui manager magic */
    browser->action_group = gtk_action_group_new ("Browser");
    gtk_action_group_set_translation_domain (browser->action_group, GETTEXT_PACKAGE);
    midori_browser_add_actions (browser);
    ui_manager = gtk_ui_manager_new ();
    accel_group = gtk_ui_manager_get_accel_group (ui_manager);
    gtk_window_add_accel_group (GTK_WINDOW (browser), accel_group);
    gtk_ui_manager_insert_action_group (ui_manager, browser->action_group, 0);

    g_object_set_data (G_OBJECT (accel_group), "midori-browser", browser);
    accel_closure = g_cclosure_new (G_CALLBACK (
        midori_browser_accel_switch_tab_activate_cb), browser, NULL);
    for (i = 0; i < 10; i++)
    {
        gchar* accel_path = g_strdup_printf ("<Manual>/Browser/SwitchTab%d", i);
        gtk_accel_map_add_entry (accel_path, GDK_KEY_0 + i, GDK_MOD1_MASK);
        gtk_accel_group_connect_by_path (accel_group, accel_path, accel_closure);
        g_free (accel_path);
    }
    g_closure_unref (accel_closure);

    error = NULL;
    if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_markup, -1, &error))
    {
        g_message ("User interface couldn't be created: %s", error->message);
        g_error_free (error);
    }

    /* Hide the 'Dummy' which only holds otherwise unused actions */
    _action_set_visible (browser, "Dummy", FALSE);

    action = g_object_new (KATZE_TYPE_SEPARATOR_ACTION,
                           "name", "Separator",
                           "label", _("_Separator"),
                           NULL);
    gtk_action_group_add_action (browser->action_group, action);
    g_object_unref (action);

    // ZRL 修改字符串翻译 2014.12.16
    action = g_object_new (MIDORI_TYPE_LOCATION_ACTION,
                           "name", "Location",
                           "label", _("_Location…"),
                           "stock-id", GTK_STOCK_JUMP_TO,
                           "placeholder-text", _("Search or enter an address"),
                           NULL);

    g_object_connect (action,
                      "signal::activate",
                      _action_location_activate, browser,
                      "signal::focus-out",
                      _action_location_focus_out, browser,
                      "signal::reset-uri",
                      _action_location_reset_uri, browser,
                      "signal::submit-uri",
                      _action_location_submit_uri, browser,
                      "signal-after::secondary-icon-released",
                      _action_location_secondary_icon_released, browser,
                      NULL);

    gtk_action_group_add_action_with_accel (browser->action_group,
                                            action, "<Ctrl>L");
    g_object_unref (action);

// ZRL 暂时屏蔽地址栏旁搜索框功能
#if ENABLE_SEARCH_ACTION 
    action = g_object_new (MIDORI_TYPE_SEARCH_ACTION,
                           "name", "Search",
                           "label", _("_Web Search…"),
                           "stock-id", GTK_STOCK_FIND,
                           "tooltip", _("Run a web search"),
                           NULL);

    g_object_connect (action,
                      "signal::activate",
                      _action_search_activate, browser,
                      "signal::submit",
                      _action_search_submit, browser,
                      "signal::focus-out",
                      _action_search_focus_out, browser,
                      "signal::notify::current-item",
                      _action_search_notify_current_item, browser,
                      "signal::notify::default-item",
                      _action_search_notify_default_item, browser,
                      NULL);

    gtk_action_group_add_action_with_accel (browser->action_group,
                                             action, "<Ctrl>K");
    g_object_unref (action);
#endif

    action = g_object_new (MIDORI_TYPE_PANED_ACTION,
                           "name", "LocationSearch",
                           NULL);
    gtk_action_group_add_action (browser->action_group, action);
    g_object_unref (action);

    //add by zghhistory 20141211
    dummy_array = katze_array_new (KATZE_TYPE_ARRAY);
    katze_array_update (dummy_array);
    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
                           "name", "Historys",
                           "label", _("_History"),
                           "stock-id", STOCK_HISTORY,
                           "array", dummy_array /* updated, unique */,
                           NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_historys_populate_folder, browser,
                      "signal::activate-item-alt",
                      midori_history_activate_item_alt, browser,
                      "signal::activate-item",
                      midori_history_activate_item, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);
    g_object_unref (dummy_array);
    dummy_array = katze_array_new (KATZE_TYPE_ARRAY);
    katze_array_update (dummy_array);
    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
                           "name", "Bookmarks",
                           "label", _("_Bookmarks"),
                           "stock-id", STOCK_BOOKMARKS,
                           "tooltip", _("Show the saved bookmarks"),
                           "array", dummy_array /* updated, unique */,
                           NULL);
    g_object_connect (action,
                      "signal::populate-folder",
                      _action_bookmarks_populate_folder, browser,
                      "signal::activate-item-alt",
                      midori_bookmarkbar_activate_item_alt, browser,
                      "signal::activate-item",
                      midori_bookmarkbar_activate_item, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);
    g_object_unref (dummy_array);

    dummy_array = katze_array_new (KATZE_TYPE_ITEM);
    katze_array_update (dummy_array);
    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
                           "name", "Tools",
                           "label", _("_Tools"),
                           "stock-id", GTK_STOCK_PREFERENCES,
                           "array", dummy_array /* updated, unique */,
                           NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_tools_populate_popup, browser,
                      NULL);
    gtk_action_group_add_action (browser->action_group, action);
    g_object_unref (action);
    g_object_unref (dummy_array);

    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
                           "name", "Window",
                           "label", _("_Tabs"),
                           "stock-id", GTK_STOCK_INDEX,
                           "tooltip", _("Show a list of all open tabs"),
                           "array", browser->proxy_array,
                           NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_window_populate_popup, browser,
                      "signal::activate-item-alt",
                      _action_window_activate_item_alt, browser,
                      NULL);
    gtk_action_group_add_action_with_accel (browser->action_group, action, "");
    g_object_unref (action);

    action = g_object_new (KATZE_TYPE_ARRAY_ACTION,
                           "name", "CompactMenu",
                           "label", _("_Menu"),
                           "stock-id", GTK_STOCK_PROPERTIES,
                           "tooltip", _("Menu"),
                           "array", katze_array_new (KATZE_TYPE_ITEM),
                           NULL);
    g_object_connect (action,
                      "signal::populate-popup",
                      _action_compact_menu_populate_popup, browser,
                      NULL);
    gtk_action_group_add_action (browser->action_group, action);
    g_object_unref (action);
    
    midori_browser_set_action_icon_name (browser);//zgh 30150319    替换工具栏图标

    /* Create the menubar */
    browser->menubar = gtk_ui_manager_get_widget (ui_manager, "/menubar");
    gtk_box_pack_start (GTK_BOX (vbox), browser->menubar, FALSE, FALSE, 0);
    gtk_widget_hide (browser->menubar);
    _action_set_visible (browser, "Menubar", !midori_browser_has_native_menubar ());
    g_signal_connect (browser->menubar, "button-press-event",
        G_CALLBACK (midori_browser_menu_button_press_event_cb), browser);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (
        gtk_ui_manager_get_widget (ui_manager, "/menubar/File/WindowNew")), NULL);
    _action_set_visible (browser, "LastSession", FALSE);

    _action_set_visible (browser, "Bookmarks", browser->bookmarks != NULL);
    _action_set_visible (browser, "BookmarkAdd", browser->bookmarks != NULL);
    _action_set_visible (browser, "BookmarksImport", browser->bookmarks != NULL);
    _action_set_visible (browser, "BookmarksExport", browser->bookmarks != NULL);
    _action_set_visible (browser, "Bookmarkbar", browser->bookmarks != NULL);

    //隐私模式下，history为空，应该隐藏菜单栏中的历史记录菜单
    _action_set_visible (browser, "Historys", browser->history != NULL);
#if ENABLE_TRASH
    _action_set_visible (browser, "Trash", browser->trash != NULL);
#endif
    _action_set_visible (browser, "Forward", FALSE);    //zgh

    /* Create the navigationbar */
    browser->navigationbar = gtk_ui_manager_get_widget (
        ui_manager, "/toolbar_navigation");
    /* FIXME: Settings should be connected with screen changes */
    gtk_settings = gtk_widget_get_settings (GTK_WIDGET (browser));
    if (gtk_settings)
        g_signal_connect (gtk_settings, "notify::gtk-toolbar-style",
            G_CALLBACK (midori_browser_navigationbar_notify_style_cb), browser);
    gtk_toolbar_set_show_arrow (GTK_TOOLBAR (browser->navigationbar), TRUE);
    gtk_widget_hide (browser->navigationbar);
    g_signal_connect (browser->navigationbar, "popup-context-menu",
        G_CALLBACK (midori_browser_toolbar_popup_context_menu_cb), browser);
    gtk_box_pack_start (GTK_BOX (vbox), browser->navigationbar, FALSE, FALSE, 0);

    /* Bookmarkbar */
    browser->bookmarkbar = gtk_toolbar_new ();
    katze_widget_add_class (browser->bookmarkbar, "secondary-toolbar");
    gtk_widget_set_name (browser->bookmarkbar, "MidoriBookmarkbar");
    gtk_toolbar_set_icon_size (GTK_TOOLBAR (browser->bookmarkbar),
                               GTK_ICON_SIZE_MENU);
    gtk_toolbar_set_style (GTK_TOOLBAR (browser->bookmarkbar),
                           GTK_TOOLBAR_BOTH_HORIZ);
    
    gtk_box_pack_start (GTK_BOX (vbox), browser->bookmarkbar, FALSE, FALSE, 0);
    g_signal_connect (browser->bookmarkbar, "popup-context-menu",
        G_CALLBACK (midori_browser_toolbar_popup_context_menu_cb), browser);

    /* Create the panel */
    hpaned = gtk_hpaned_new ();
    g_signal_connect (hpaned, "notify::position",
                      G_CALLBACK (midori_panel_notify_position_cb),
                      browser);
    g_signal_connect (hpaned, "cycle-child-focus",
                      G_CALLBACK (midori_panel_cycle_child_focus_cb),
                      browser);
    gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);
    gtk_widget_show (hpaned);
    browser->panel = g_object_new (MIDORI_TYPE_PANEL,
                                   "action-group", browser->action_group,
                                   NULL);
    g_object_connect (browser->panel,
        "signal::notify::page",
        midori_panel_notify_page_cb, browser,
        "signal::notify::show-titles",
        midori_panel_notify_show_titles_cb, browser,
        "signal::notify::open-panels-in-windows",//201411217 zlf add
        midori_panel_notify_open_in_window_cb, browser,        //20141217 zlf add
        "signal::close",
        midori_panel_close_cb, browser,
        NULL);

    /* Notebook, containing all views */
    vpaned = gtk_vpaned_new ();
    gtk_paned_pack2 (GTK_PANED (hpaned), vpaned, TRUE, FALSE);
    gtk_widget_show (vpaned);
    browser->notebook = midori_notebook_new ();

    gtk_paned_pack1 (GTK_PANED (vpaned), browser->notebook, FALSE, FALSE);
    g_signal_connect (browser->notebook, "tab-switched",
                      G_CALLBACK (midori_browser_switched_tab_cb),
                      browser);
    g_signal_connect (browser->notebook, "notify::tab",
                      G_CALLBACK (midori_browser_notify_tab_cb), browser);
    g_signal_connect (browser->notebook, "tab-moved",
                      G_CALLBACK (midori_browser_tab_moved_cb),
                      browser);
    g_signal_connect (browser->notebook, "context-menu",
        G_CALLBACK (midori_browser_notebook_context_menu_cb),
                      browser);
    g_signal_connect (browser->notebook, "tab-context-menu",
        G_CALLBACK (midori_browser_notebook_tab_context_menu_cb), browser);
    g_signal_connect (browser->notebook, "tab-detached",
                      G_CALLBACK (midori_browser_notebook_create_window_cb), browser);
    g_signal_connect (browser->notebook, "new-tab",
                      G_CALLBACK (midori_browser_notebook_new_tab_cb), browser);
    gtk_widget_show (browser->notebook);

    /* Inspector container */
    browser->inspector = gtk_vbox_new (FALSE, 0);
    gtk_paned_pack2 (GTK_PANED (vpaned), browser->inspector, FALSE, FALSE);
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_can_focus (scrolled, TRUE);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                         GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (browser->inspector), scrolled, TRUE, TRUE, 0);
    browser->inspector_view = gtk_viewport_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), browser->inspector_view);

    /* Incremental findbar */
    browser->find = g_object_new (MIDORI_TYPE_FINDBAR, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), browser->find, FALSE, FALSE, 0);

    /* Statusbar */
    browser->statusbar = gtk_statusbar_new ();
    browser->statusbar_contents =
        gtk_statusbar_get_message_area (GTK_STATUSBAR (browser->statusbar));
    gtk_box_pack_start (GTK_BOX (vbox), browser->statusbar, FALSE, FALSE, 0);

    g_signal_connect (browser->statusbar, "button-press-event",
        G_CALLBACK (midori_browser_menu_button_press_event_cb), browser);

    g_object_unref (ui_manager);
}

static void
midori_browser_dispose (GObject* object)
{
    MidoriBrowser* browser = MIDORI_BROWSER (object);

    /* We are done, the session mustn't change anymore */
    katze_object_assign (browser->proxy_array, NULL);
    g_signal_handlers_disconnect_by_func (browser->settings,
                                          midori_browser_settings_notify,
                                          browser);
    midori_browser_set_bookmarks (browser, NULL);
    midori_browser_set_history (browser, NULL);

    G_OBJECT_CLASS (midori_browser_parent_class)->dispose (object);
}

static void
midori_browser_finalize (GObject* object)
{
    MidoriBrowser* browser = MIDORI_BROWSER (object);

    katze_assign (browser->statusbar_text, NULL);

    katze_object_assign (browser->settings, NULL);
    katze_object_assign (browser->trash, NULL);
    katze_object_assign (browser->search_engines, NULL);
    katze_object_assign (browser->history, NULL);
    katze_object_assign (browser->history_database, NULL);
    katze_object_assign (browser->dial, NULL);

//ZRL 2014.12.09
    katze_object_assign (browser->bookmarks, NULL);
    katze_object_assign (browser->forward_url,NULL);//add by luyue 2015/3/17
    browser->page_window = NULL; //luyue 2015/4/10
    g_idle_remove_by_data (browser);

    G_OBJECT_CLASS (midori_browser_parent_class)->finalize (object);
}

static void
_midori_browser_set_toolbar_style (MidoriBrowser*     browser,
                                   MidoriToolbarStyle toolbar_style)
{
    GtkToolbarStyle gtk_toolbar_style;
    GtkIconSize icon_size;
    GtkSettings* gtk_settings = gtk_widget_get_settings (GTK_WIDGET (browser));
    g_object_get (gtk_settings, "gtk-toolbar-icon-size", &icon_size, NULL);
    if (toolbar_style == MIDORI_TOOLBAR_DEFAULT && gtk_settings)
        g_object_get (gtk_settings, "gtk-toolbar-style", &gtk_toolbar_style, NULL);
    else
    {
        switch (toolbar_style)
        {
        case MIDORI_TOOLBAR_SMALL_ICONS:
            icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
        case MIDORI_TOOLBAR_ICONS:
            gtk_toolbar_style = GTK_TOOLBAR_ICONS;
            break;
        case MIDORI_TOOLBAR_TEXT:
            gtk_toolbar_style = GTK_TOOLBAR_TEXT;
            break;
        case MIDORI_TOOLBAR_BOTH:
            gtk_toolbar_style = GTK_TOOLBAR_BOTH;
            break;
        case MIDORI_TOOLBAR_BOTH_HORIZ:
        case MIDORI_TOOLBAR_DEFAULT:
            gtk_toolbar_style = GTK_TOOLBAR_BOTH_HORIZ;
        }
    }
    gtk_toolbar_set_style (GTK_TOOLBAR (browser->navigationbar),
                           gtk_toolbar_style);
    gtk_toolbar_set_icon_size (GTK_TOOLBAR (browser->navigationbar), MIDORI_TOOLBAR_SMALL_ICONS/*icon_size*/);
    midori_panel_set_toolbar_style (MIDORI_PANEL (browser->panel),
                                    gtk_toolbar_style);
}

static gboolean
midori_browser_toolbar_item_button_press_event_cb (GtkWidget*      toolitem,
                                                   GdkEventButton* event,
                                                   MidoriBrowser*  browser)
{
   if (MIDORI_EVENT_NEW_TAB (event))
   {
      /* check if the middle-click was performed over reload button */
      if (g_object_get_data (G_OBJECT (toolitem), "reload-middle-click"))
      {
         gtk_action_activate (_action_by_name (browser, "TabDuplicate"));
      }

      GtkWidget* parent = gtk_widget_get_parent (toolitem);
      GtkAction* action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (parent));
      g_object_set_data (G_OBJECT (action),
                         "midori-middle-click",
                         GINT_TO_POINTER (1));
      return _action_navigation_activate (action, browser);
   }
   else if (MIDORI_EVENT_CONTEXT_MENU (event))
   {
      if (g_object_get_data (G_OBJECT (toolitem), "history-back"))
      {
         //todo
      }
      else if (g_object_get_data (G_OBJECT (toolitem), "history-forward"))
      {
         //todo
      }
      else
      {
         midori_browser_toolbar_popup_context_menu_cb (
                GTK_IS_BIN (toolitem) && gtk_bin_get_child (GTK_BIN (toolitem)) ?
                gtk_widget_get_parent (toolitem) : toolitem,
                event->x, event->y, event->button, browser);
      }
      return TRUE;
   }
   return FALSE;
}

static void
_midori_browser_search_item_allocate_cb (GtkWidget*    widget,
                                         GdkRectangle* allocation,
                                         gpointer      user_data)
{
    MidoriBrowser* browser = MIDORI_BROWSER (user_data);
    MidoriWebSettings* settings = browser->settings;
    g_object_set (settings, "search-width", allocation->width, NULL);
}

static void
_midori_browser_set_toolbar_items (MidoriBrowser* browser,
                                   const gchar*   items)
{
    gchar** names;
    gchar** name;
    GtkAction* action;
    GtkWidget* toolitem;
    const char* token_location = g_intern_static_string ("Location");
    const char* token_search = g_intern_static_string ("Search");
    const char* token_dontcare = g_intern_static_string ("Dontcare");
    const char* token_current = token_dontcare;
    const char* token_last;

    gtk_container_foreach (GTK_CONTAINER (browser->navigationbar),
        (GtkCallback)gtk_widget_destroy, NULL);

    names = g_strsplit (items ? items : "", ",", 0);
    name = names;
    for (; *name; ++name)
    {
        action = _action_by_name (browser, *name);
        if (action && strstr (*name, "CompactMenu") == NULL)
        {
            token_last = token_current;

            /* Decide, what kind of token (item) we got now */
            if (name && !g_strcmp0 (*name, "Location"))
                token_current = token_location;
            else if (name && !g_strcmp0 (*name, "Search"))
                token_current = token_search;
            else
                token_current = token_dontcare;

            if ((token_current == token_location || token_current == token_search) &&
                 (token_last == token_location || token_last == token_search))
            {
                GtkWidget* toolitem_first = gtk_action_create_tool_item (
                    _action_by_name (browser, token_last));
                GtkWidget* toolitem_second = gtk_action_create_tool_item (
                    _action_by_name (browser, token_current));
                MidoriPanedAction* paned_action = MIDORI_PANED_ACTION (
                    _action_by_name (browser, "LocationSearch"));
                MidoriWebSettings* midori_settings = browser->settings;
                midori_paned_action_set_child1 (paned_action, toolitem_first, token_last,
                    token_last == token_search ? FALSE : TRUE, TRUE);
                midori_paned_action_set_child2 (paned_action, toolitem_second, token_current,
                    token_current == token_search ? FALSE : TRUE, TRUE);
                g_signal_connect (G_OBJECT (token_current == token_search ? toolitem_second : toolitem_first),
                    "size-allocate", G_CALLBACK (_midori_browser_search_item_allocate_cb), (gpointer) browser);

                gtk_widget_set_size_request (
                    token_last == token_search ? toolitem_first : toolitem_second,
                    katze_object_get_int ((gpointer) midori_settings,
                    "search-width"),
                    -1);

                toolitem = gtk_action_create_tool_item (GTK_ACTION (paned_action));
                token_current = token_dontcare;
            }
            else if (token_current == token_dontcare && token_last != token_dontcare)
            {
                /* There was a location or search item, but was not followed by
                   the other one, that form a couple */
                gtk_toolbar_insert (GTK_TOOLBAR (browser->navigationbar),
                    GTK_TOOL_ITEM (gtk_action_create_tool_item (
                    _action_by_name (browser, token_last))),
                    -1);

                toolitem = gtk_action_create_tool_item (action);
            }
            else if (token_current != token_dontcare && token_last == token_dontcare)
                continue;
            /* A "new tab" button is already part of the notebook */
            else if (!strcmp (gtk_action_get_name (action), "TabNew"))
                continue;
            else
                toolitem = gtk_action_create_tool_item (action);

            if (gtk_bin_get_child (GTK_BIN (toolitem)))
            {
                if (!g_strcmp0 (*name, "Back"))
                    g_object_set_data (G_OBJECT (gtk_bin_get_child (GTK_BIN (toolitem))),
                                       "history-back", (void*) 0xdeadbeef);
                else if (g_str_has_suffix (*name, "Forward"))
                    g_object_set_data (G_OBJECT (gtk_bin_get_child (GTK_BIN (toolitem))),
                                       "history-forward", (void*) 0xdeadbeef);
                else if (g_strcmp0 (*name, "Reload"))
                    g_object_set_data (G_OBJECT (gtk_bin_get_child (GTK_BIN (toolitem))),
                                                 "reload-middle-click", (void*) 0xdeadbeef);

                g_signal_connect (gtk_bin_get_child (GTK_BIN (toolitem)),
                                  "button-press-event",
                                  G_CALLBACK (midori_browser_toolbar_item_button_press_event_cb),
                                  browser);
            }
            else
            {
                gtk_tool_item_set_use_drag_window (GTK_TOOL_ITEM (toolitem), TRUE);
                g_signal_connect (toolitem,
                                  "button-press-event",
                                  G_CALLBACK (midori_browser_toolbar_item_button_press_event_cb),
                                  browser);
           }
            gtk_toolbar_insert (GTK_TOOLBAR (browser->navigationbar),
                                GTK_TOOL_ITEM (toolitem), -1);
        }
    }
    g_strfreev (names);

    /* There was a last item, which could have formed a couple, but
       there is no item left, we add that last item to toolbar as is */
    if (token_current != token_dontcare)
    {
        gtk_toolbar_insert (GTK_TOOLBAR (browser->navigationbar),
            GTK_TOOL_ITEM (gtk_action_create_tool_item (
            _action_by_name (browser, token_current))), -1);
    }

    if (!katze_object_get_boolean (browser->settings, "show-menubar"))
    {
        toolitem = gtk_action_create_tool_item (
            GTK_ACTION (_action_by_name (browser, "CompactMenu")));
        gtk_toolbar_insert (GTK_TOOLBAR (browser->navigationbar),
                            GTK_TOOL_ITEM (toolitem), -1);
        g_signal_connect (gtk_bin_get_child (GTK_BIN (toolitem)),
            "button-press-event",
            G_CALLBACK (midori_browser_toolbar_item_button_press_event_cb),
            browser);
    }
}

//add by luyue 2015/1/20
static void
_smart_zoom_function_realization (GtkWidget*     botton,
                                  MidoriBrowser* browser)
{
   bool smart_zoom_status = false;

   g_object_get(browser->settings, "smart-zoom", &smart_zoom_status, NULL);
   g_object_set(browser->settings, "smart-zoom", !smart_zoom_status, NULL);    
}

//add by luyue 2015/1/20
//状态栏中添加快捷启动按钮。
static void
_midori_browser_set_statusbar_button (MidoriBrowser* browser)
{
   bool smart_zoom_status = false;

   //智能缩放
   g_object_get(browser->settings, "smart-zoom", &smart_zoom_status, NULL);
   if (smart_zoom_status)
       {
      browser->smart_zoom_image = gtk_image_new_from_file(midori_paths_get_res_filename("DblClickZoom-enabled.png"));
       }
   else
      browser->smart_zoom_image = gtk_image_new_from_file(midori_paths_get_res_filename("DblClickZoom-disabled.png"));
   browser->smart_zoom_button = gtk_button_new();
   gtk_container_add(GTK_CONTAINER(browser->smart_zoom_button), browser->smart_zoom_image);
   if(smart_zoom_status)
      gtk_widget_set_tooltip_text(browser->smart_zoom_button,"双击缩放开关状态:开启\n单击按钮进行切换");
   else
      gtk_widget_set_tooltip_text(browser->smart_zoom_button,"双击缩放开关状态:关闭\n单击按钮进行切换");
   gtk_widget_show(browser->smart_zoom_image);
   gtk_widget_show(browser->smart_zoom_button);
   gtk_box_pack_end ((GtkBox*) browser->statusbar, browser->smart_zoom_button, FALSE, FALSE, (guint) 3);
   g_signal_connect(G_OBJECT(browser->smart_zoom_button),"clicked",G_CALLBACK(_smart_zoom_function_realization),browser);
}

static void
_midori_browser_update_settings (MidoriBrowser* browser)
{
    gboolean remember_last_window_size;
    MidoriWindowState last_window_state;
    guint inactivity_reset;
    gboolean compact_sidepanel;
    gboolean right_align_sidepanel, open_panels_in_windows;
    gint last_panel_position, last_panel_page;
    gboolean show_menubar, show_bookmarkbar;
    gboolean show_panel;
    MidoriToolbarStyle toolbar_style;
    gchar* toolbar_items;
    gboolean close_buttons_left, close_buttons_on_tabs;

    g_object_get (browser->settings,
                  "remember-last-window-size", &remember_last_window_size,
                  "last-window-width", &browser->last_window_width,
                  "last-window-height", &browser->last_window_height,
                  "last-window-state", &last_window_state,
                  "inactivity-reset", &inactivity_reset,
                  "compact-sidepanel", &compact_sidepanel,
                  "right-align-sidepanel", &right_align_sidepanel,
                  "open-panels-in-windows", &open_panels_in_windows,
                  "last-panel-position", &last_panel_position,
                  "last-panel-page", &last_panel_page,
                  "show-menubar", &show_menubar,
                  "show-navigationbar", &browser->show_navigationbar,
                  "show-bookmarkbar", &show_bookmarkbar,
                  "show-panel", &show_panel,
                  "show-statusbar", &browser->show_statusbar,
                  "toolbar-style", &toolbar_style,
                  "toolbar-items", &toolbar_items,
                  "close-buttons-left", &close_buttons_left,
                  "close-buttons-on-tabs", &close_buttons_on_tabs,
                  "maximum-history-age", &browser->maximum_history_age,
                  NULL);

    midori_notebook_set_close_buttons_visible (
        MIDORI_NOTEBOOK (browser->notebook), close_buttons_on_tabs);
    midori_notebook_set_close_buttons_left (
        MIDORI_NOTEBOOK (browser->notebook), close_buttons_left);
    midori_findbar_set_close_button_left (MIDORI_FINDBAR (browser->find),
        close_buttons_left);
    if (browser->dial != NULL)
        midori_speed_dial_set_close_buttons_left (browser->dial, close_buttons_left);

    midori_browser_set_inactivity_reset (browser, inactivity_reset);

    if (remember_last_window_size)
    {
        if (browser->last_window_width && browser->last_window_height)
            gtk_window_set_default_size (GTK_WINDOW (browser),
                browser->last_window_width, browser->last_window_height);
        else
            katze_window_set_sensible_default_size (GTK_WINDOW (browser));
        switch (last_window_state)
        {
            case MIDORI_WINDOW_MINIMIZED:
                gtk_window_iconify (GTK_WINDOW (browser));
                break;
            case MIDORI_WINDOW_MAXIMIZED:
                gtk_window_maximize (GTK_WINDOW (browser));
                break;
            case MIDORI_WINDOW_FULLSCREEN:
                gtk_window_fullscreen (GTK_WINDOW (browser));
                break;
            default:
                ;/* Do nothing. */
        }
    }

    _midori_browser_set_toolbar_style (browser, toolbar_style);
    _toggle_tabbar_smartly (browser, FALSE);
    _midori_browser_set_toolbar_items (browser, toolbar_items);
    //add by luyue 2015/1/20
    _midori_browser_set_statusbar_button (browser);

// ZRL 屏蔽搜索框功能
#if ENABLE_SEARCH_ACTION
    if (browser->search_engines)
    {
        const gchar* default_search = midori_settings_get_location_entry_search (
            MIDORI_SETTINGS (browser->settings));
        KatzeItem* item;

        if ((item = katze_array_get_nth_item (browser->search_engines,
                                              browser->last_web_search)))
            midori_search_action_set_current_item (MIDORI_SEARCH_ACTION (
                _action_by_name (browser, "Search")), item);

        if (default_search != NULL
         && (item = katze_array_find_uri (browser->search_engines, default_search)))
            midori_search_action_set_default_item (MIDORI_SEARCH_ACTION (
                _action_by_name (browser, "Search")), item);
    }
#endif

    g_object_set (browser->panel, "show-titles", !compact_sidepanel,
                  "right-aligned", right_align_sidepanel,
                  "open-panels-in-windows", open_panels_in_windows, NULL);

    /* The browser may not yet be visible, which means that we can't set the
       page. So we set it in midori_browser_size_allocate_cb */
    if (gtk_widget_get_visible (GTK_WIDGET (browser)))
        midori_panel_set_current_page (MIDORI_PANEL (browser->panel), last_panel_page);
    else
        g_object_set_data (G_OBJECT (browser), "last-page",
                           GINT_TO_POINTER (last_panel_page));

    _action_set_active (browser, "Menubar", show_menubar);
    _action_set_active (browser, "Navigationbar", browser->show_navigationbar);
    _action_set_active (browser, "Bookmarkbar", show_bookmarkbar
                                             && browser->bookmarks != NULL);
//    _action_set_active (browser, "Panel", show_panel);
    _action_set_active (browser, "Statusbar", browser->show_statusbar);

    g_free (toolbar_items);
}

static void
midori_browser_settings_notify (MidoriWebSettings* web_settings,
                                GParamSpec*        pspec,
                                MidoriBrowser*     browser)
{
    const gchar* name;
    GValue value = {0, };

    name = g_intern_string (pspec->name);
    g_value_init (&value, pspec->value_type);
    g_object_get_property (G_OBJECT (web_settings), name, &value);

    if (name == g_intern_string ("toolbar-style"))
        _midori_browser_set_toolbar_style (browser, g_value_get_enum (&value));
    else if (name == g_intern_string ("toolbar-items"))
        _midori_browser_set_toolbar_items (browser, g_value_get_string (&value));
    else if (name == g_intern_string ("compact-sidepanel"))
    {
        g_signal_handlers_block_by_func (browser->panel,
            midori_panel_notify_show_titles_cb, browser);
        g_object_set (browser->panel, "show-titles",
                      !g_value_get_boolean (&value), NULL);
        g_signal_handlers_unblock_by_func (browser->panel,
            midori_panel_notify_show_titles_cb, browser);
    }
    else if (name == g_intern_string ("open-panels-in-windows"))
        g_object_set (browser->panel, "open-panels-in-windows",
                      g_value_get_boolean (&value), NULL);
    else if (name == g_intern_string ("always-show-tabbar"))
        _toggle_tabbar_smartly (browser, FALSE);
    else if (name == g_intern_string ("show-menubar"))
    {
        _action_set_active (browser, "Menubar", g_value_get_boolean (&value));
    }
    else if (name == g_intern_string ("show-navigationbar"))
    {
        browser->show_navigationbar = g_value_get_boolean (&value);
        _action_set_active (browser, "Navigationbar", g_value_get_boolean (&value));
    }
    else if (name == g_intern_string ("show-bookmarkbar"))
    {
        _action_set_active (browser, "Bookmarkbar", g_value_get_boolean (&value));
    }
    else if (name == g_intern_string ("show-statusbar"))
    {
        browser->show_statusbar = g_value_get_boolean (&value);
        _action_set_active (browser, "Statusbar", g_value_get_boolean (&value));
    }
    else if (name == g_intern_string ("maximum-history-age"))
        browser->maximum_history_age = g_value_get_int (&value);
    else if (name == g_intern_string ("close-buttons-on-tabs"))
        midori_notebook_set_close_buttons_visible (
            MIDORI_NOTEBOOK (browser->notebook), g_value_get_boolean (&value));
    else if (name == g_intern_string ("close-buttons-left"))
    {
        midori_findbar_set_close_button_left (MIDORI_FINDBAR (browser->find),
                                              g_value_get_boolean (&value));
        midori_speed_dial_set_close_buttons_left (browser->dial,
            g_value_get_boolean (&value));
        midori_notebook_set_close_buttons_left (
            MIDORI_NOTEBOOK (browser->notebook), g_value_get_boolean (&value));
    }
    else if (name == g_intern_string ("inactivity-reset"))
        midori_browser_set_inactivity_reset (browser, g_value_get_uint (&value));
    else if (!g_object_class_find_property (G_OBJECT_GET_CLASS (web_settings),name))
        g_warning (_("Unexpected setting '%s'"), name);
    else if(name == g_intern_string("default-font-family"))
    {
        gchar *strval = NULL;
	g_object_get(browser->settings, "default-font-family", &strval, NULL);
	webkit_settings_set_default_font_family(browser->settings, strval);	
        g_free(strval);
    }
    else if(name == g_intern_string("serif-font-family"))
    {
	gchar *strval = NULL;
	g_object_get(browser->settings, "serif-font-family", &strval, NULL);
	webkit_settings_set_serif_font_family(browser->settings, strval);	
	g_free(strval);
    }
    else if(name == g_intern_string("sans-serif-font-family"))
    {
	gchar *strval = NULL;
	g_object_get(browser->settings, "sans-serif-font-family", &strval, NULL);
	webkit_settings_set_sans_serif_font_family(browser->settings, strval);
	g_free(strval);
    }
    else if(name == g_intern_string("monospace-font-family"))
    {
	gchar *strval = NULL;
	g_object_get(browser->settings, "monospace-font-family", &strval, NULL);
	webkit_settings_set_monospace_font_family(browser->settings, strval);
	g_free(strval);
    }
    else if(name == g_intern_string("default-monospace-font-size"))
    {
	gint fontSize = katze_object_get_int (browser->settings, "default-monospace-font-size");
	webkit_settings_set_default_monospace_font_size(browser->settings, fontSize);
    }
    else if(name == g_intern_string("default-font-size"))
    {
	gint fontSize = katze_object_get_int (browser->settings, "default-font-size");
	webkit_settings_set_default_font_size(browser->settings, fontSize);
    }
    else if(name == g_intern_string("zoom-level"))
    {
	gdouble dvalue = 0.0;
	g_object_get(browser->settings, "zoom-level", &dvalue, NULL);
        //add by luyue 2015/1/21
        GList* tabs = midori_browser_get_tabs (browser);
        for (; tabs; tabs = g_list_next (tabs))
           midori_view_set_zoom_level (tabs->data, dvalue);
        g_list_free (tabs);
    }
    else if(name == g_intern_string("zoom-text-and-images"))
    {
	bool bvalue = katze_object_get_boolean(browser->settings, "zoom-text-and-images");
                        
        //add by luyue 2015/1/20               
        GList* tabs = midori_browser_get_tabs (browser);
        for (; tabs; tabs = g_list_next (tabs))
           midori_view_set_zoomtext_state (tabs->data, browser->settings);
        g_list_free (tabs);
       
	webkit_settings_set_zoom_text_only(browser->settings, !bvalue);
    }
    //add by luyue 2015/1/20
    else if(name == g_intern_string("smart-zoom"))
    {
        bool smart_zoom_status = false;
        gtk_widget_destroy (browser->smart_zoom_image);
        g_object_get(browser->settings, "smart-zoom", &smart_zoom_status, NULL);
        if (smart_zoom_status)
           browser->smart_zoom_image = gtk_image_new_from_file(midori_paths_get_res_filename("DblClickZoom-enabled.png"));
        else
           browser->smart_zoom_image = gtk_image_new_from_file(midori_paths_get_res_filename("DblClickZoom-disabled.png"));
        gtk_container_add(GTK_CONTAINER(browser->smart_zoom_button), browser->smart_zoom_image);
        if(smart_zoom_status)
           gtk_widget_set_tooltip_text(browser->smart_zoom_button,"双击缩放开关状态:开启\n单击按钮进行切换");
        else
           gtk_widget_set_tooltip_text(browser->smart_zoom_button,"双击缩放开关状态:关闭\n单击按钮进行切换");
        gtk_widget_show(browser->smart_zoom_image);
        GList* tabs = midori_browser_get_tabs (browser);
        for (; tabs; tabs = g_list_next (tabs))
        {
           midori_view_set_doublezoom_state (tabs->data, browser->settings);
           midori_view_set_doublezoom_level (tabs->data, browser->settings);
        }
        g_list_free (tabs);
    }
    else if(name == g_intern_string("auto-load-images"))
    {
	bool bvalue = katze_object_get_boolean(browser->settings, "auto-load-images");
	webkit_settings_set_auto_load_images(browser->settings, bvalue);
    }
    else if(name == g_intern_string("enable-scripts"))
    {
	bool bvalue = katze_object_get_boolean(browser->settings, "enable-scripts");
	webkit_settings_set_enable_javascript(browser->settings, bvalue);
    }
    else if(name == g_intern_string("do-not-track"))
    {
	bool bvalue = katze_object_get_boolean(browser->settings, "do-not-track");
	webkit_settings_set_enable_do_not_track(browser->settings, bvalue);
    }
    //luyue add by 2015/1/21
    else if(name == g_intern_string("smart-zoom-level"))
    {
        GList* tabs = midori_browser_get_tabs (browser);
        for (; tabs; tabs = g_list_next (tabs))
           midori_view_set_doublezoom_level (tabs->data, browser->settings);
        g_list_free (tabs);
    }
    //add by luyue 2015/6/30
    else if(name == g_intern_string("secure-level"))
    {
        GList* tabs = midori_browser_get_tabs (browser);
        for (; tabs; tabs = g_list_next (tabs))
           midori_view_set_secure_level (tabs->data, browser->settings);
        g_list_free (tabs);
    }
    //add end
    g_value_unset (&value);
}

static void
midori_bookmarkbar_insert_item (GtkWidget* toolbar,
                                KatzeItem* item)
{
    MidoriBrowser* browser = midori_browser_get_for_widget (toolbar);
    GtkAction* action = _action_by_name (browser, "Bookmarks");
    GtkToolItem* toolitem = katze_array_action_create_tool_item_for (
        KATZE_ARRAY_ACTION (action), item);
    g_object_set_data (G_OBJECT (toolitem), "KatzeItem", item);

    if (!KATZE_IS_ITEM (item)) /* Separator */
        gtk_tool_item_set_use_drag_window (toolitem, TRUE);
    gtk_widget_show (GTK_WIDGET (toolitem));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
}

static void
midori_bookmarkbar_add_item_cb (KatzeArray*    bookmarks,
                                KatzeItem*     item,
                                MidoriBrowser* browser)
{
    if (gtk_widget_get_visible (browser->bookmarkbar))
        midori_bookmarkbar_populate (browser);
    else if (katze_item_get_meta_boolean (item, "toolbar"))
        _action_set_active (browser, "Bookmarkbar", TRUE);
    midori_browser_update_history (item, "bookmark", "created");
}

static void
midori_bookmarkbar_update_item_cb (KatzeArray*    bookmarks,
                                   KatzeItem*     item,
                                   MidoriBrowser* browser)
{
    if (gtk_widget_get_visible (browser->bookmarkbar))
        midori_bookmarkbar_populate (browser);
    midori_browser_update_history (item, "bookmark", "modify");
}

static void
midori_bookmarkbar_remove_item_cb (KatzeArray*    bookmarks,
                                   KatzeItem*     item,
                                   MidoriBrowser* browser)
{
    if (gtk_widget_get_visible (browser->bookmarkbar))
        midori_bookmarkbar_populate (browser);
    midori_browser_update_history (item, "bookmark", "delete");
}

static void
midori_bookmarkbar_populate (MidoriBrowser* browser)
{
    if (browser->bookmarkbar_populate)
        return;

    g_idle_add (midori_browser_idle, browser);
    browser->bookmarkbar_populate = TRUE;
}

static void
midori_bookmarkbar_populate_idle (MidoriBrowser* browser)
{
    KatzeArray* array;
    KatzeItem* item;

    midori_bookmarkbar_clear (browser->bookmarkbar);

    /* Use a dummy to ensure height of the toolbar */
    gtk_toolbar_insert (GTK_TOOLBAR (browser->bookmarkbar),
                        gtk_separator_tool_item_new (), -1);

    array = midori_bookmarks_db_query_recursive (browser->bookmarks,
        "id, parentid, title, uri, desc, app, toolbar, pos_panel, pos_bar", "toolbar = 1", NULL, FALSE);
    if (!array)
    {
        _action_set_sensitive (browser, "BookmarkAdd", FALSE);
        _action_set_sensitive (browser, "BookmarkFolderAdd", FALSE);
        return;
    }
    KATZE_ARRAY_FOREACH_ITEM (item, array)
    {
       //add by luyue 2015/4/30 start
       //书签栏中目录中的书签只在目录中出现
       gint64 parentid = katze_item_get_meta_integer (KATZE_ITEM (item), "parentid");
       if(parentid<0)
          midori_bookmarkbar_insert_item (browser->bookmarkbar, item);
      //add end
    }
    _action_set_sensitive (browser, "BookmarkAdd", TRUE);
    _action_set_sensitive (browser, "BookmarkFolderAdd", TRUE);
}

static void
midori_bookmarkbar_clear (GtkWidget* toolbar)
{
    GList* children = gtk_container_get_children (GTK_CONTAINER (toolbar));
    while (children != NULL)
    {
        gtk_widget_destroy (children->data);
        children = g_list_next (children);
    }
}

static void
midori_browser_show_bookmarkbar_notify_value_cb (MidoriWebSettings* settings,
                                                 GParamSpec*        pspec,
                                                 MidoriBrowser*     browser)
{
    if (!katze_object_get_boolean (browser->settings, "show-bookmarkbar"))
        midori_bookmarkbar_clear (browser->bookmarkbar);
    else
        midori_bookmarkbar_populate (browser);
}

static void
midori_browser_set_bookmarks (MidoriBrowser* browser,
                              MidoriBookmarksDb*    bookmarks)
{
    MidoriWebSettings* settings;

    if (browser->bookmarks != NULL)
    {
        g_signal_handlers_disconnect_by_func (browser->bookmarks,
            midori_bookmarkbar_add_item_cb, browser);
        g_signal_handlers_disconnect_by_func (browser->bookmarks,
            midori_bookmarkbar_update_item_cb, browser);
        g_signal_handlers_disconnect_by_func (browser->bookmarks,
            midori_bookmarkbar_remove_item_cb, browser);
    }

    g_object_set (G_OBJECT (_action_by_name (browser, "Bookmarks")),
        "array", KATZE_ARRAY (bookmarks),
        NULL);

    settings = midori_browser_get_settings (browser);
    g_signal_handlers_disconnect_by_func (settings,
        midori_browser_show_bookmarkbar_notify_value_cb, browser);
    katze_object_assign (browser->bookmarks, bookmarks);

    _action_set_visible (browser, "Bookmarks", bookmarks != NULL);
    if (bookmarks != NULL)
    {
        /* FIXME: Proxies aren't shown propely. Why? */
        GSList* proxies = gtk_action_get_proxies (
            _action_by_name (browser, "Bookmarks"));
        for (; proxies; proxies = g_slist_next (proxies))
            gtk_widget_show (proxies->data);
    }
    _action_set_visible (browser, "BookmarkAdd", bookmarks != NULL);
    _action_set_visible (browser, "BookmarksImport", bookmarks != NULL);
    _action_set_visible (browser, "BookmarksExport", bookmarks != NULL);
    _action_set_visible (browser, "Bookmarkbar", bookmarks != NULL);

    if (!bookmarks)
        return;

    if (katze_object_get_boolean (browser->settings, "show-bookmarkbar"))
        _action_set_active (browser, "Bookmarkbar", TRUE);
    g_object_ref (bookmarks);
    g_signal_connect (settings, "notify::show-bookmarkbar",
        G_CALLBACK (midori_browser_show_bookmarkbar_notify_value_cb), browser);
    g_object_notify (G_OBJECT (settings), "show-bookmarkbar");
    g_signal_connect_after (bookmarks, "add-item",
        G_CALLBACK (midori_bookmarkbar_add_item_cb), browser);
    g_signal_connect_after (bookmarks, "update-item",
        G_CALLBACK (midori_bookmarkbar_update_item_cb), browser);
    g_signal_connect_after (bookmarks, "remove-item",
        G_CALLBACK (midori_bookmarkbar_remove_item_cb), browser);
}

static void
midori_browser_set_property (GObject*      object,
                             guint         prop_id,
                             const GValue* value,
                             GParamSpec*   pspec)
{
    MidoriBrowser* browser = MIDORI_BROWSER (object);

    switch (prop_id)
    {
    case PROP_URI:
        midori_browser_set_current_uri (browser, g_value_get_string (value));
        break;
    case PROP_TAB:
        midori_browser_set_current_tab (browser, g_value_get_object (value));
        break;
    case PROP_STATUSBAR_TEXT:
        _midori_browser_set_statusbar_text (browser,
            MIDORI_VIEW (midori_browser_get_current_tab (browser)),
            g_value_get_string (value));
        break;
    case PROP_SETTINGS:
        g_signal_handlers_disconnect_by_func (browser->settings,
                                              midori_browser_settings_notify,
                                              browser);
        katze_object_assign (browser->settings, g_value_dup_object (value));
        if (!browser->settings)
            browser->settings = midori_web_settings_new ();
#ifdef APP_LEVEL_TIME
printf("函数(_midori_browser_update_settings) start time = %lld\n",g_get_real_time());
#endif
        _midori_browser_update_settings (browser);
#ifdef APP_LEVEL_TIME
printf("函数(_midori_browser_update_settings) end time = %lld\n",g_get_real_time());
#endif
        g_signal_connect (browser->settings, "notify",
            G_CALLBACK (midori_browser_settings_notify), browser);
        GList* tabs = midori_browser_get_tabs (browser);
        for (; tabs; tabs = g_list_next (tabs))
            midori_view_set_settings (tabs->data, browser->settings);
        g_list_free (tabs);
        break;
    case PROP_BOOKMARKS:
        midori_browser_set_bookmarks (browser, g_value_get_object (value));
        break;
    case PROP_TRASH:
        /* FIXME: Disconnect handlers */
#if ENABLE_TRASH
        katze_object_assign (browser->trash, g_value_dup_object (value));
        g_object_set (_action_by_name (browser, "Trash"),
                      "array", browser->trash, "reversed", TRUE,
                      NULL);
        _action_set_visible (browser, "Trash", browser->trash != NULL);
        if (browser->trash != NULL)
        {
            g_signal_connect_after (browser->trash, "clear",
                G_CALLBACK (midori_browser_trash_clear_cb), browser);
            midori_browser_trash_clear_cb (browser->trash, browser);
        }
#endif
        break;
    case PROP_SEARCH_ENGINES:
    {
        /* FIXME: Disconnect handlers */
        katze_object_assign (browser->search_engines, g_value_dup_object (value));
        midori_location_action_set_search_engines (MIDORI_LOCATION_ACTION (
            _action_by_name (browser, "Location")), browser->search_engines);
// ZRL 去除search action功能，即地址栏旁的搜索框
#if ENABLE_SEARCH_ACTION
        midori_search_action_set_search_engines (MIDORI_SEARCH_ACTION (
            _action_by_name (browser, "Search")), browser->search_engines);
        /* FIXME: Connect to updates */

        if (browser->search_engines)
        {
            const gchar* default_search = midori_settings_get_location_entry_search (
                MIDORI_SETTINGS (browser->settings));
            g_object_get (browser->settings, "last-web-search", &browser->last_web_search, NULL);
            item = katze_array_get_nth_item (browser->search_engines, browser->last_web_search);
            midori_search_action_set_current_item (MIDORI_SEARCH_ACTION (
                _action_by_name (browser, "Search")), item);

            if (default_search != NULL && (item = katze_array_find_uri (browser->search_engines, default_search)))
                midori_search_action_set_default_item (MIDORI_SEARCH_ACTION (
                    _action_by_name (browser, "Search")), item);
        }
#endif
        break;
    }
    case PROP_HISTORY:
        midori_browser_set_history (browser, g_value_get_object (value));
        break;
    case PROP_SPEED_DIAL:
        if (browser->dial != NULL)
            g_signal_handlers_disconnect_by_func (browser->dial,
                midori_browser_speed_dial_refresh_cb, browser);
        katze_object_assign (browser->dial, g_value_dup_object (value));
        if (browser->dial != NULL)
            g_signal_connect (browser->dial, "refresh",
                G_CALLBACK (midori_browser_speed_dial_refresh_cb), browser);
	if (browser->dial != NULL)
            g_signal_connect (browser->dial, "refresh1",
                G_CALLBACK (midori_browser_speed_dial_refresh1_cb), browser);
        break;
    case PROP_SHOW_TABS:
        browser->show_tabs = g_value_get_boolean (value);
        _toggle_tabbar_smartly (browser, FALSE);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
midori_browser_get_property (GObject*    object,
                             guint       prop_id,
                             GValue*     value,
                             GParamSpec* pspec)
{
    MidoriBrowser* browser = MIDORI_BROWSER (object);

    switch (prop_id)
    {
    case PROP_MENUBAR:
        g_value_set_object (value, browser->menubar);
        break;
    case PROP_NAVIGATIONBAR:
        g_value_set_object (value, browser->navigationbar);
        break;
    case PROP_NOTEBOOK:
        g_value_set_object (value, MIDORI_NOTEBOOK (browser->notebook)->notebook);
        break;
    case PROP_PANEL:
        g_value_set_object (value, browser->panel);
        break;
    case PROP_URI:
        g_value_set_string (value, midori_browser_get_current_uri (browser));
        break;
    case PROP_TAB:
        g_value_set_object (value, midori_browser_get_current_tab (browser));
        break;
    case PROP_LOAD_STATUS:
    {
        GtkWidget* view = midori_browser_get_current_tab (browser);
        if (view)
            g_value_set_enum (value,
                midori_view_get_load_status (MIDORI_VIEW (view)));
        else
            g_value_set_enum (value, MIDORI_LOAD_FINISHED);
        break;
    }
    case PROP_STATUSBAR:
        g_value_set_object (value, browser->statusbar_contents);
        break;
    case PROP_STATUSBAR_TEXT:
        g_value_set_string (value, browser->statusbar_text);
        break;
    case PROP_SETTINGS:
        g_value_set_object (value, browser->settings);
        break;
    case PROP_PROXY_ITEMS:
        g_value_set_object (value, browser->proxy_array);
        break;
    case PROP_BOOKMARKS:
        g_value_set_object (value, browser->bookmarks);
        break;
    case PROP_TRASH:
        g_value_set_object (value, browser->trash);
        break;
    case PROP_SEARCH_ENGINES:
        g_value_set_object (value, browser->search_engines);
        break;
    case PROP_HISTORY:
        g_value_set_object (value, browser->history);
        break;
    case PROP_SPEED_DIAL:
        g_value_set_object (value, browser->dial);
        break;
    case PROP_SHOW_TABS:
        g_value_set_boolean (value, browser->show_tabs);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/**
 * midori_browser_new:
 *
 * Creates a new browser widget.
 *
 * A browser is a window with a menubar, toolbars, a notebook, panels
 * and a statusbar. You should mostly treat it as an opaque widget.
 *
 * Return value: a new #MidoriBrowser
 **/
MidoriBrowser*
midori_browser_new (void)
{
    MidoriBrowser* browser = g_object_new (MIDORI_TYPE_BROWSER,
                                           NULL);
    return browser;
}

/**
 * midori_browser_add_tab:
 * @browser: a #MidoriBrowser
 * @widget: a view
 *
 * Appends a view in the form of a new tab and creates an
 * according item in the Window menu.
 *
 * Since: 0.4.9: Return type is void
 **/
void
midori_browser_add_tab (MidoriBrowser* browser,
                        GtkWidget*     view)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (GTK_IS_WIDGET (view));

    g_signal_emit (browser, signals[ADD_TAB], 0, view);
}

/**
 * midori_browser_page_num:
 * @browser: a #MidoriBrowser
 * @widget: a widget in the browser
 *
 * Retrieves the position of @widget in the browser.
 *
 * If there is no page present at all, -1 is returned.
 *
 * Return value: the index of the widget, or -1
 *
 * Since: 0.4.5
 **/
gint
midori_browser_page_num (MidoriBrowser* browser,
                         GtkWidget*     view)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), -1);
    g_return_val_if_fail (MIDORI_IS_VIEW (view), -1);

    return midori_notebook_get_tab_index (MIDORI_NOTEBOOK (browser->notebook), MIDORI_TAB (view));
}


/**
 * midori_browser_close_tab:
 * @browser: a #MidoriBrowser
 * @widget: a view
 *
 * Closes an existing view, removing it and
 * its associated menu item from the browser.
 **/
void
midori_browser_close_tab (MidoriBrowser* browser,
                           GtkWidget*     view)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (GTK_IS_WIDGET (view));

   g_signal_emit (browser, signals[REMOVE_TAB], 0, view);
}

//add by luyue 2015/5/16 start
//解决部分标签需要自动关闭的问题
void
midori_browser_close_tab1 (MidoriBrowser* browser,
                           GtkWidget*     view)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (GTK_IS_WIDGET (view));

   //当前窗口只有一个tab时，关闭整个窗口
   int tab_num;
   GList* tabs = midori_browser_get_tabs (browser);
   for (tab_num=0; tabs != NULL; tabs = g_list_next (tabs),tab_num++);
   g_list_free (tabs);
   if(tab_num >1)
      g_signal_emit (browser, signals[REMOVE_TAB], 0, view);
   else
   {
      midori_browser_destroy_cb (browser);
      gtk_widget_destroy (GTK_WIDGET (browser));
   }
}
//add end

/**
 * midori_browser_add_item
 * @browser: a #MidoriBrowser
 * @item: an item
 *
 * Return value: a #GtkWidget
 *
 * Since: 0.4.9: Return type is GtkWidget*
 **/
GtkWidget*
midori_browser_add_item (MidoriBrowser* browser,
                         KatzeItem*     item)
{
    const gchar* uri;
    GtkWidget* view;

    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);
    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    uri = katze_item_get_uri (item);
#ifdef APP_LEVEL_TIME
printf("view start create time = %lld\n",g_get_real_time());
#endif
    view = midori_view_new_with_item (item, browser->settings);
#ifdef APP_LEVEL_TIME
printf("view end create time = %lld\n",g_get_real_time());
#endif
    midori_browser_add_tab (browser, view);
    midori_view_set_uri (MIDORI_VIEW (view), uri);
    return view;
}

/**
 * midori_browser_add_uri:
 * @browser: a #MidoriBrowser
 * @uri: an URI
 *
 * Appends an uri in the form of a new view.
 *
 * Return value: a #GtkWidget
 *
 * Since: 0.4.9: Return type is GtkWidget*
 **/
GtkWidget*
midori_browser_add_uri (MidoriBrowser* browser,
                        const gchar*   uri)
{
    KatzeItem* item;

    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);
    g_return_val_if_fail (uri != NULL, NULL);

    item = katze_item_new ();
    item->uri = g_strdup (uri);
    return midori_browser_add_item (browser, item);
}

/**
 * midori_browser_activate_action:
 * @browser: a #MidoriBrowser
 * @name: action, setting=value expression or extension=true|false
 *
 * Activates the specified action. See also midori_browser_assert_action().
 **/
void
midori_browser_activate_action (MidoriBrowser* browser,
                                const gchar*   name)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (name != NULL);

    g_signal_emit (browser, signals[ACTIVATE_ACTION], 0, name);
}

void
midori_browser_set_action_visible (MidoriBrowser* browser,
                                   const gchar*   name,
                                   gboolean       visible)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (name != NULL);

    _action_set_visible (browser, name, visible);
    _action_set_sensitive (browser, name, visible);
}

/**
 * midori_browser_get_action_group:
 * @browser: a #MidoriBrowser
 *
 * Retrieves the action group holding all actions used
 * by the browser. It allows obtaining individual
 * actions and adding new actions.
 *
 * Return value: the action group of the browser
 *
 * Since: 0.1.4
 **/
GtkActionGroup*
midori_browser_get_action_group (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);

    return browser->action_group;
}

/**
 * midori_browser_set_current_uri:
 * @browser: a #MidoriBrowser
 * @uri: an URI
 *
 * Loads the specified URI in the current view.
 *
 * If the current view is opaque, and cannot load
 * new pages, it will automatically open a new tab.
 **/
void
midori_browser_set_current_uri (MidoriBrowser* browser,
                                const gchar*   uri)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (uri != NULL);

    midori_view_set_uri (MIDORI_VIEW (midori_browser_get_current_tab (browser)), uri);
}

/**
 * midori_browser_get_current_uri:
 * @browser: a #MidoriBrowser
 *
 * Determines the URI loaded in the current view.
 *
 * If there is no view present at all, %NULL is returned.
 *
 * Return value: the current URI, or %NULL
 **/
const gchar*
midori_browser_get_current_uri (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);

    return midori_view_get_display_uri (MIDORI_VIEW (
        midori_browser_get_current_tab (browser)));
}

/**
 * midori_browser_set_current_page:
 * @browser: a #MidoriBrowser
 * @n: the index of a page
 *
 * Switches to the page with the index @n.
 *
 * The widget will also grab the focus automatically.
 **/
void
midori_browser_set_current_page (MidoriBrowser* browser,
                                 gint           n)
{
    GtkWidget* view;

    g_return_if_fail (MIDORI_IS_BROWSER (browser));

    view = midori_browser_get_nth_tab (browser, n);
    g_return_if_fail (view != NULL);

    midori_browser_set_tab (browser, view);
}

/**
 * midori_browser_get_current_page:
 * @browser: a #MidoriBrowser
 *
 * Determines the currently selected page.
 *
 * If there is no page present at all, %NULL is returned.
 *
 * Return value: the selected page, or -1
 **/
gint
midori_browser_get_current_page (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), -1);
    return midori_notebook_get_index (MIDORI_NOTEBOOK (browser->notebook));
}

/**
 * midori_browser_set_current_item:
 * @browser: a #MidoriBrowser
 * @item: a #KatzeItem
 *
 * Switches to the page containing @item, see also midori_browser_set_current_page().
 *
 * The widget will also grab the focus automatically.
 *
 * Since: 0.4.8
 **/
void
midori_browser_set_current_item (MidoriBrowser* browser,
                                 KatzeItem*     item)
{
    guint i;
    guint n = katze_array_get_length (browser->proxy_array);

    for (i = 0; i < n; i++)
    {
        GtkWidget* view = midori_browser_get_nth_tab (browser, i);
        if (midori_view_get_proxy_item (MIDORI_VIEW (view)) == item)
            midori_browser_set_current_page (browser, i);
    }
}

/**
 * midori_browser_get_nth_tab:
 * @browser: a #MidoriBrowser
 * @page: the index of a tab
 *
 * Retrieves the tab at the position @page.
 *
 * If there is no page present at all, %NULL is returned.
 *
 * Return value: the selected page, or -1
 *
 * Since: 0.1.9
 **/
GtkWidget*
midori_browser_get_nth_tab (MidoriBrowser* browser,
                            gint           page)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);
    return (GtkWidget*)midori_notebook_get_nth_tab (MIDORI_NOTEBOOK (browser->notebook), page);
}

/**
 * midori_browser_set_tab:
 * @browser: a #MidoriBrowser
 * @view: a #GtkWidget
 *
 * Switches to the page containing @view.
 *
 * The widget will also grab the focus automatically.
 *
 * Since: 0.2.6
 **/
void
midori_browser_set_current_tab (MidoriBrowser* browser,
                                GtkWidget*     view)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_return_if_fail (GTK_IS_WIDGET (view));

    midori_notebook_set_tab (MIDORI_NOTEBOOK (browser->notebook), MIDORI_TAB (view));
    if (midori_tab_is_blank (MIDORI_TAB (view)))
        midori_browser_activate_action (browser, "Location");
    else
        gtk_widget_grab_focus (view);

    midori_browser_notify_tab_cb (browser->notebook, NULL, browser);
}

/**
 * midori_browser_get_tab:
 * @browser: a #MidoriBrowser
 *
 * Retrieves the currently selected tab.
 *
 * If there is no tab present at all, %NULL is returned.
 *
 * See also midori_browser_get_current_page().
 *
 * Return value: the selected tab, or %NULL
 *
 * Since: 0.2.6
 **/
GtkWidget*
midori_browser_get_current_tab (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);
    return (GtkWidget*)midori_notebook_get_tab (MIDORI_NOTEBOOK (browser->notebook));
}

/**
 * midori_browser_get_tabs:
 * @browser: a #MidoriBrowser
 *
 * Retrieves the tabs as a list.
 *
 * Return value: a newly allocated #GList of #MidoriView
 *
 * Since: 0.2.5
 **/
GList*
midori_browser_get_tabs (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);
    return gtk_container_get_children (GTK_CONTAINER (browser->notebook));
}

/**
 * midori_browser_get_proxy_array:
 * @browser: a #MidoriBrowser
 *
 * Retrieves a proxy array representing the respective proxy items.
 * The array is updated automatically.
 *
 * Return value: the proxy #KatzeArray
 **/
KatzeArray*
midori_browser_get_proxy_array (MidoriBrowser* browser)
{
    g_return_val_if_fail (MIDORI_IS_BROWSER (browser), NULL);
    return browser->proxy_array;
}

/**
 * midori_browser_get_for_widget:
 * @widget: a #GtkWidget
 *
 * Determines the browser appropriate for the specified widget.
 *
 * Return value: a #MidoriBrowser
 *
 * Since 0.1.7
 **/
MidoriBrowser*
midori_browser_get_for_widget (GtkWidget* widget)
{
    gpointer browser;

    g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

    browser = gtk_widget_get_toplevel (GTK_WIDGET (widget));
    if (!MIDORI_IS_BROWSER (browser))
    {
        if (!GTK_IS_WINDOW (browser))
            return NULL;

        browser = gtk_window_get_transient_for (GTK_WINDOW (browser));
        if (!MIDORI_IS_BROWSER (browser))
        {
            GList* top_levels = gtk_window_list_toplevels ();
            GList *iter;

            for (iter = top_levels; iter; iter = g_list_next (iter))
            {
                browser = iter->data;
                if (MIDORI_IS_BROWSER (browser) )
                {
                    g_list_free (top_levels);
                    return MIDORI_BROWSER (browser);
                }
            }
            g_list_free (top_levels);
            return NULL;
        }
    }
    return MIDORI_BROWSER (browser);
}

/**
 * midori_browser_quit:
 * @browser: a #MidoriBrowser
 *
 * Quits the browser, including any other browser windows.
 *
 * This function relys on the application implementing
 * the MidoriBrowser::quit signal. If the browser was added
 * to the MidoriApp, this is handled automatically.
 **/
void
midori_browser_quit (MidoriBrowser* browser)
{
    g_return_if_fail (MIDORI_IS_BROWSER (browser));
    g_signal_emit (browser, signals[QUIT], 0);
}

//20141217 zlf
static void
midori_panel_window_hide (GtkWidget*      window, 
                          MidoriBrowser*  browser)
{
    gtk_widget_hide(window);
    return; 
}

static void
midori_browser_show_panel_window(MidoriBrowser* browser, 
                                 gboolean       show)
{
    GtkWidget* vpaned = gtk_widget_get_parent (browser->notebook);
    static GtkWidget *panel_window = NULL;
    
    g_object_ref (browser->panel);
    g_object_ref (vpaned);

    if(!panel_window || !GTK_IS_WINDOW (panel_window))
    {
        panel_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_transient_for((GtkWindow *)panel_window,GTK_WINDOW(browser));
        g_signal_connect(G_OBJECT(panel_window), "delete-event", midori_panel_window_hide , browser);
        gtk_window_set_title(GTK_WINDOW(panel_window), "书签/历史 管理器");
        gtk_window_set_position(GTK_WINDOW(panel_window),GTK_WIN_POS_CENTER); 
        gtk_window_set_default_size(GTK_WINDOW(panel_window), 460, 360);
        gtk_container_add(GTK_CONTAINER(panel_window), browser->panel);        
    }
    else
    {
        GList* children = gtk_container_get_children (GTK_CONTAINER (panel_window));
        if (!g_list_length (children))
            gtk_container_add(GTK_CONTAINER(panel_window), browser->panel);
        if (show)
            gtk_window_present((GtkWindow *)panel_window);
    }
    if(show)
       gtk_widget_show_all(panel_window);

    browser->sari_panel_windows = panel_window;
    g_object_unref (browser->panel);
    g_object_unref (vpaned);
    return;
}

static void 
midori_browser_actiave_bookmark_in_window(GtkAction*     action,
                                          MidoriBrowser* browser)
{
    midori_browser_show_panel_window(browser, true);    //zgh 20150226 将panel显示与设置显示页调换顺序   下同
    midori_panel_open_in_window((MidoriPanel *)browser->panel, TRUE, PANEL_BOOKMARK);
}

static void 
midori_browser_actiave_history_in_window(GtkAction*     action,
                                         MidoriBrowser* browser)
{
    midori_browser_show_panel_window(browser, true);
    midori_panel_open_in_window((MidoriPanel *)browser->panel, TRUE, PANEL_HISTORY);
}

static void 
midori_browser_actiave_transfer_in_window(GtkAction*     action,
                                          MidoriBrowser* browser)
{
   system("/usr/local/libexec/cdosbrowser/cdosbrowser_download &");
   midori_panel_open_in_window((MidoriPanel *)browser->panel, TRUE, PANEL_TRANSFER);
    
   //zgh 工具栏上图标变化
   GtkActionGroup *action_group = midori_browser_get_action_group (browser);
   GtkAction *download_action = gtk_action_group_get_action(action_group, "DownloadDialog");
   g_object_set (download_action, 
                 "stock-id", NULL,
                 "icon_name", STOCK_DOWNLOAD,
                 NULL);
}

void midori_browser_clear_history(MidoriBrowser* browser)
{
        katze_array_clear (browser->history);
}

//added by wangyl 2015.7.9
void midori_browser_clear_bookmarks(MidoriBrowser* browser)
{
       g_signal_emit_by_name(browser->bookmarks,"clear");
}

void midori_browser_change_history_seting(MidoriBrowser* browser, gint *settings)
{
   GtkWidget* dialog;
   gint result;  

   switch(*settings)
   {
      case 0:
         ////0.this setting will clean the history, OK?
         break;
      case 1:
         dialog = gtk_message_dialog_new (GTK_WINDOW (browser),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                          _("This operation will remove all history first, Are you sure?"));
         result = gtk_dialog_run (GTK_DIALOG (dialog));
         gtk_widget_destroy (dialog);
         if (result == GTK_RESPONSE_YES)
            midori_browser_clear_history (browser);
         break;
      case 2:
         ////2.this setting will clean the history, OK?
         break;        
      default:
         break;
   }
}

void
midori_browser_open_new_tab_from_extension   (MidoriBrowser* browser, const gchar* uri, gboolean background)
{
    GtkWidget* new_view = midori_browser_add_uri (browser, uri);
    if (!background)
        midori_browser_set_current_tab (browser, new_view);
    else
        midori_browser_notify_new_tab (browser);
}
