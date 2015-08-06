//add by luyue 2015/7/14


#include "midori/midori.h"

GtkWidget *gupiao_button;

static int 
close_popup_window()
{
   popup_window=NULL;
   return false;
}

static void
gupiao_deactivated_cb (MidoriExtension* extension,
                       MidoriBrowser*   browser)
{
   if(gupiao_button)
      gtk_widget_destroy (gupiao_button);
}

static GtkWidget*
webkit_web_view_create_web_view_cb (GtkWidget*      web_view,
                                    WebKitNavigationAction* navigationAction,
                                    MidoriBrowser*     browser)
{
    WebKitURIRequest *naviationRequest = webkit_navigation_action_get_request(navigationAction);
    gchar *destUri = webkit_uri_request_get_uri(naviationRequest);
    midori_browser_open_new_tab_from_extension(browser, destUri, false);
    if(popup_window)
    {              
       gtk_widget_destroy(popup_window);
       popup_window = NULL;
    }
    return NULL;
}

static void
gupiao_function_realization (GtkWidget* botton,MidoriBrowser* browser)
{
   char uri[256];
   sprintf(uri,"file://%s",midori_paths_get_res_filename("gupiao/index.html"));
   
   if(popup_window)
   {
     gtk_widget_destroy(popup_window);
     popup_window = NULL;
   }
   popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(popup_window,"股票小助手");
   gtk_widget_set_size_request (popup_window, 600, 450);
   gtk_window_set_resizable (GTK_WINDOW (popup_window), FALSE);
   gtk_window_set_resizable(GTK_WINDOW(popup_window), FALSE);
   gtk_window_set_type_hint(popup_window,GDK_WINDOW_TYPE_HINT_DIALOG);
   GtkWidget* webview = webkit_web_view_new();
   gtk_container_add(GTK_CONTAINER(popup_window), webview);
   gtk_widget_show(webview);
   gtk_widget_show(popup_window);
   webkit_web_view_load_uri(webview, uri);
   g_object_connect (webview, "signal::create",webkit_web_view_create_web_view_cb, browser);
   g_signal_connect(G_OBJECT(popup_window),"delete_event", G_CALLBACK(close_popup_window),NULL);
}

static void gupiao_extension_browser_added_cb (MidoriApp*       app,
                                               MidoriBrowser*   browser,
                                               MidoriExtension* extension)
{
   GtkStatusbar* tmp = NULL;

   GtkWidget *gupiao_image = gtk_image_new_from_file(midori_paths_get_res_filename("gupiao/logo16.png"));
   gupiao_button = gtk_button_new();
   gtk_container_add(GTK_CONTAINER(gupiao_button), gupiao_image);
   gtk_widget_set_tooltip_text(gupiao_button,"股票小助手");
   gtk_widget_show(gupiao_image);
   gtk_widget_show(gupiao_button);
   g_object_get (browser, "statusbar", &tmp, NULL);
   gtk_box_pack_end ((GtkBox*) tmp, gupiao_button, FALSE, FALSE, (guint) 3);
   g_signal_connect(G_OBJECT(gupiao_button),"clicked",G_CALLBACK(gupiao_function_realization),browser);
}

static void 
gupiao_activated_cb (MidoriExtension* extension, 
                     MidoriApp*       app) 
{
   GList* browser_it = NULL;
   GList* browser_collection = midori_app_get_browsers (app);

   for (browser_it = browser_collection; browser_it != NULL; browser_it = browser_it->next) 
   {
      MidoriBrowser* browser = (MidoriBrowser*) browser_it->data;
      gupiao_extension_browser_added_cb(app,browser,extension);
   }
   g_signal_connect (app, "add-browser",
       G_CALLBACK (gupiao_extension_browser_added_cb), extension);
}

MidoriExtension* 
extension_init (void) 
{
   MidoriExtension* extension = g_object_new (MIDORI_TYPE_EXTENSION,
      "name", "股票小助手",
      "description", "办公室一族股民的利器",
      "version", "1.0" MIDORI_VERSION_SUFFIX,
      "authors", "luy_os@sari.ac.cn",
      NULL);

   g_signal_connect (extension, "activate",
       G_CALLBACK ( gupiao_activated_cb), NULL);
   g_signal_connect (extension, "deactivate",
       G_CALLBACK ( gupiao_deactivated_cb), NULL);

   return extension;
}
