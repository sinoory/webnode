//add by luyue 2014/12/29


#include "midori/midori.h"

GtkWidget *weather_inquiry_button;

static int 
close_popup_window()
{
   popup_window=NULL;
   return false;
}

static void
weather_inquiry_deactivated_cb (MidoriExtension* extension,
                                MidoriBrowser*   browser)
{
   if(weather_inquiry_button)
      gtk_widget_destroy (weather_inquiry_button);
}

static void
weather_inquiry_function_realization (GtkWidget* botton,MidoriBrowser* browser)
{
   char uri[256];
   sprintf(uri,"file://%s",midori_paths_get_res_filename("weather_inquiry/pop.html"));
   
   if(popup_window)
      gtk_widget_destroy(popup_window);
   popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(popup_window, _("Weather inquiry"));
   //add by luyue 2015/3/16 start
//   gtk_window_set_default_size(popup_window, 500, 230);
   gtk_widget_set_size_request (popup_window, 500, 230);
   gtk_window_set_resizable(GTK_WINDOW(popup_window), FALSE);
//add end
   gtk_window_set_type_hint(popup_window,GDK_WINDOW_TYPE_HINT_DIALOG);

   //定制settings，关闭web security policy
   WebKitSettings *localSettings = webkit_settings_new();
   webkit_settings_set_enable_javascript(localSettings, true);
   webkit_settings_set_enable_web_security(localSettings, false);
   GtkWidget* webview = webkit_web_view_new_with_settings(localSettings);
   gtk_container_add(GTK_CONTAINER(popup_window), webview);
   gtk_widget_show(webview);
   gtk_widget_show(popup_window);
   webkit_web_view_load_uri(webview, uri);
   g_object_connect (webview, "signal::decide-policy",web_view_navigation_decision_cb, browser);
   g_signal_connect(G_OBJECT(popup_window),"delete_event", G_CALLBACK(close_popup_window),NULL);
}

static void weather_inquiry_extension_browser_added_cb (MidoriApp*       app,
                                                        MidoriBrowser*   browser,
                                                        MidoriExtension* extension)
{
   GtkStatusbar* tmp = NULL;

   GtkWidget *weather_inquiry_image = gtk_image_new_from_file(midori_paths_get_res_filename("weather_inquiry/icon_16.png"));
   weather_inquiry_button = gtk_button_new();
   gtk_container_add(GTK_CONTAINER(weather_inquiry_button), weather_inquiry_image);
   gtk_widget_set_tooltip_text(weather_inquiry_button,_("Weather inquiry"));
   gtk_widget_show(weather_inquiry_image);
   gtk_widget_show(weather_inquiry_button);
   g_object_get (browser, "statusbar", &tmp, NULL);
   gtk_box_pack_end ((GtkBox*) tmp, weather_inquiry_button, FALSE, FALSE, (guint) 3);
   g_signal_connect(G_OBJECT(weather_inquiry_button),"clicked",G_CALLBACK(weather_inquiry_function_realization),browser);
}

static void 
weather_inquiry_activated_cb (MidoriExtension* extension, 
                              MidoriApp*       app) 
{
   GList* browser_it = NULL;
   GList* browser_collection = midori_app_get_browsers (app);

   for (browser_it = browser_collection; browser_it != NULL; browser_it = browser_it->next) 
   {
      MidoriBrowser* browser = (MidoriBrowser*) browser_it->data;
      weather_inquiry_extension_browser_added_cb(app,browser,extension);
   }
   g_signal_connect (app, "add-browser",
       G_CALLBACK (weather_inquiry_extension_browser_added_cb), extension);
}

MidoriExtension* 
extension_init (void) 
{
   MidoriExtension* extension = g_object_new (MIDORI_TYPE_EXTENSION,
      "name", _("Weather inquiry"),
      "description", _("Quick view real-time weather"),
      "version", "1.0.9" MIDORI_VERSION_SUFFIX,
      "authors", "luy_os@sari.ac.cn",
      NULL);

   g_signal_connect (extension, "activate",
       G_CALLBACK ( weather_inquiry_activated_cb), NULL);
   g_signal_connect (extension, "deactivate",
       G_CALLBACK ( weather_inquiry_deactivated_cb), NULL);

   return extension;
}
