//add by luyue 2015/7/15


#include "midori/midori.h"

GtkWidget *weibo_button;

static void
weibo_deactivated_cb (MidoriExtension* extension,
                      MidoriBrowser*   browser)
{
   if(weibo_button)
      gtk_widget_destroy (weibo_button);
}

static void
weibo_function_realization (GtkWidget* botton,MidoriBrowser* browser)
{
   gchar *destUri = "http://www.weibo.com";
   midori_browser_open_new_tab_from_extension(browser, destUri, false);
}

static void weibo_extension_browser_added_cb (MidoriApp*       app,
                                              MidoriBrowser*   browser,
                                              MidoriExtension* extension)
{
   GtkStatusbar* tmp = NULL;

   GtkWidget *weibo_image = gtk_image_new_from_file(midori_paths_get_res_filename("weibo/weibo.png"));
   weibo_button = gtk_button_new();
   gtk_container_add(GTK_CONTAINER(weibo_button), weibo_image);
   gtk_widget_set_tooltip_text(weibo_button,"新浪微博");
   gtk_widget_show(weibo_image);
   gtk_widget_show(weibo_button);
   g_object_get (browser, "statusbar", &tmp, NULL);
   gtk_box_pack_end ((GtkBox*) tmp, weibo_button, FALSE, FALSE, (guint) 3);
   g_signal_connect(G_OBJECT(weibo_button),"clicked",G_CALLBACK(weibo_function_realization),browser);
}

static void 
weibo_activated_cb (MidoriExtension* extension, 
                    MidoriApp*       app) 
{
   GList* browser_it = NULL;
   GList* browser_collection = midori_app_get_browsers (app);

   for (browser_it = browser_collection; browser_it != NULL; browser_it = browser_it->next) 
   {
      MidoriBrowser* browser = (MidoriBrowser*) browser_it->data;
      weibo_extension_browser_added_cb(app,browser,extension);
   }
   g_signal_connect (app, "add-browser",
       G_CALLBACK (weibo_extension_browser_added_cb), extension);
}

MidoriExtension* 
extension_init (void) 
{
   MidoriExtension* extension = g_object_new (MIDORI_TYPE_EXTENSION,
      "name", "新浪微博",
      "description", "快速进入新浪微博主页",
      "version", "1.0" MIDORI_VERSION_SUFFIX,
      "authors", "luy_os@sari.ac.cn",
      NULL);

   g_signal_connect (extension, "activate",
       G_CALLBACK ( weibo_activated_cb), NULL);
   g_signal_connect (extension, "deactivate",
       G_CALLBACK ( weibo_deactivated_cb), NULL);

   return extension;
}
