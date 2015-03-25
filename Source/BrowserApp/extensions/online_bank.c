//add by luyue 2015/1/5


#include "midori/midori.h"

GtkWidget *online_bank_button;

static int 
close_popup_window()
{
   popup_window=NULL;
   return false;
}

static void
online_bank_deactivated_cb (MidoriExtension* extension,
                            MidoriBrowser*   browser)
{
   if(online_bank_button)
      gtk_widget_destroy (online_bank_button);
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
online_bank_function_realization (GtkWidget* botton,MidoriBrowser* browser)
{
   char uri[256];
   sprintf(uri,"file://%s",midori_paths_get_res_filename("online_bank/bank.html"));
   
   if(popup_window)
        {
     gtk_widget_destroy(popup_window);
     popup_window = NULL;
        }
   popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(popup_window, _("secure online bank"));
  //add by luyue 2015/3/16
//   gtk_window_set_default_size(popup_window, 350, 500);
    gtk_widget_set_size_request (popup_window, 350, 500);
    gtk_window_set_resizable (GTK_WINDOW (popup_window), FALSE);
  //add end
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

static void online_bank_extension_browser_added_cb (MidoriApp*       app,
                                                    MidoriBrowser*   browser,
                                                    MidoriExtension* extension)
{
   GtkStatusbar* tmp = NULL;

   GtkWidget *online_bank_image = gtk_image_new_from_file(midori_paths_get_res_filename("online_bank/online_bank.png"));
   online_bank_button = gtk_button_new();
   gtk_container_add(GTK_CONTAINER(online_bank_button), online_bank_image);
   gtk_widget_set_tooltip_text(online_bank_button,_("Online bank"));
   gtk_widget_show(online_bank_image);
   gtk_widget_show(online_bank_button);
   g_object_get (browser, "statusbar", &tmp, NULL);
   gtk_box_pack_end ((GtkBox*) tmp, online_bank_button, FALSE, FALSE, (guint) 3);
   g_signal_connect(G_OBJECT(online_bank_button),"clicked",G_CALLBACK(online_bank_function_realization),browser);
}

static void 
online_bank_activated_cb (MidoriExtension* extension, 
                          MidoriApp*       app) 
{
   GList* browser_it = NULL;
   GList* browser_collection = midori_app_get_browsers (app);

   for (browser_it = browser_collection; browser_it != NULL; browser_it = browser_it->next) 
   {
      MidoriBrowser* browser = (MidoriBrowser*) browser_it->data;
      online_bank_extension_browser_added_cb(app,browser,extension);
   }
   g_signal_connect (app, "add-browser",
       G_CALLBACK (online_bank_extension_browser_added_cb), extension);
}

MidoriExtension* 
extension_init (void) 
{
   MidoriExtension* extension = g_object_new (MIDORI_TYPE_EXTENSION,
      "name", _("Online bank"),
      "description", _("Provide safe online banking links,prevent access to fishing website"),
      "version", "1.0" MIDORI_VERSION_SUFFIX,
      "authors", "luy_os@sari.ac.cn",
      NULL);

   g_signal_connect (extension, "activate",
       G_CALLBACK ( online_bank_activated_cb), NULL);
   g_signal_connect (extension, "deactivate",
       G_CALLBACK ( online_bank_deactivated_cb), NULL);

   return extension;
}
