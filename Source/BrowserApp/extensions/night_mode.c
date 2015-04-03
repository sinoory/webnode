//add by luyue 2015/1/6

#include "midori/midori.h"



GtkWidget *night_mode_button;
GtkWidget *night_mode_image;
gboolean g_night_mode = false;
gchar *night_mode_remove = "jQuery(\"#__nightingale_view_cover\").remove();f=false;";

static void
night_mode_tabs_view_notify_uri_cb (MidoriView*      view,
                                    GParamSpec*      pspec,
                                    MidoriExtension* extension)
{
   GtkWidget* current_web_view;
   gchar *script=NULL;
   FILE *fp;
   int file_size;

   const gchar* uri = midori_view_get_display_uri (view);
   if (!*uri)
      return;
   if (g_night_mode)
   {
      if (!midori_uri_is_blank (uri))
      {
         gchar* hostname = midori_uri_parse_hostname (uri, NULL);
         if (hostname)
         {
            if((fp=fopen(midori_paths_get_res_filename("night_mode/nightingale_view_content.js"),"r"))!=NULL)
            {
               fseek(fp,0,SEEK_END);
               file_size=ftell(fp);
               fseek(fp,0,SEEK_SET);
               script=(char *)malloc(file_size * sizeof(char)+1);
               fread(script,file_size,sizeof(char),fp);
               script[file_size*sizeof(char)]='\0';
               fclose(fp);
               current_web_view = midori_view_get_web_view (view);
               webkit_web_view_run_javascript(WEBKIT_WEB_VIEW (current_web_view), script, NULL, NULL, NULL);
               free(script);
               script=NULL;
            }         
            g_free (hostname);
         }
      }
   }
   else
   {
      current_web_view = midori_view_get_web_view (MIDORI_VIEW (view));
      webkit_web_view_run_javascript(WEBKIT_WEB_VIEW (current_web_view), night_mode_remove, NULL, NULL, NULL);
    }
}

static void
night_mode_extension_browser_add_tab_cb (MidoriBrowser*   browser,
                                         GtkWidget*       view,
                                         MidoriExtension* extension)
{
   night_mode_tabs_view_notify_uri_cb (MIDORI_VIEW (view), NULL, extension);
   g_signal_connect (view, "notify::icon",
      G_CALLBACK (night_mode_tabs_view_notify_uri_cb), extension);
}

static void
night_mode_function_realization (GtkWidget*     botton,
                                 MidoriBrowser* browser)
{
   gtk_widget_destroy (night_mode_image);
   if(!g_night_mode)
        {
      night_mode_image = gtk_image_new_from_file(midori_paths_get_res_filename("night_mode/19-2.png"));
      gtk_container_add(GTK_CONTAINER(night_mode_button), night_mode_image);
      gtk_widget_show(night_mode_image);
      g_night_mode = true;
        }
   else
        {
      night_mode_image = gtk_image_new_from_file(midori_paths_get_res_filename("night_mode/19.png"));
      gtk_container_add(GTK_CONTAINER(night_mode_button), night_mode_image);
      gtk_widget_show(night_mode_image);
      g_night_mode = false;
        }
   GList* children;
   children = midori_browser_get_tabs (MIDORI_BROWSER (browser));
   for (; children; children = g_list_next (children))
      night_mode_extension_browser_add_tab_cb (browser, children->data, NULL);
   g_list_free (children);
   if(g_night_mode)   
      g_signal_connect (browser, "add-tab",G_CALLBACK (night_mode_extension_browser_add_tab_cb), NULL);
   else
      g_signal_handlers_disconnect_by_func (browser, night_mode_extension_browser_add_tab_cb, NULL);
}

static void
night_mode_deactivated_cb (MidoriExtension* extension,
                           MidoriBrowser*   browser)
{
   if(night_mode_button)
      gtk_widget_destroy (night_mode_button);
   g_signal_handlers_disconnect_by_func (extension, night_mode_deactivated_cb, browser);
   if(g_night_mode)
        {
      GList* children = midori_browser_get_tabs (MIDORI_BROWSER (browser));
      for (; children; children = g_list_next (children))
                 {
         GtkWidget* current_web_view = midori_view_get_web_view (MIDORI_VIEW (children->data));
         webkit_web_view_run_javascript(WEBKIT_WEB_VIEW (current_web_view), night_mode_remove, NULL, NULL, NULL);
                }
      g_list_free (children);
      g_signal_handlers_disconnect_by_func (browser, night_mode_extension_browser_add_tab_cb, NULL);
        }            
}

static void 
night_mode_extension_add_browser_cb (MidoriApp*       app,
                                     MidoriBrowser*   browser,
                                     MidoriExtension* extension)
{
   GtkStatusbar* tmp = NULL;
   night_mode_image = gtk_image_new_from_file(midori_paths_get_res_filename("night_mode/19.png"));
   night_mode_button = gtk_button_new();
   gtk_container_add(GTK_CONTAINER(night_mode_button), night_mode_image);
   gtk_widget_set_tooltip_text(night_mode_button,
              "夜间模式\n使用方法:\n点击按钮选择夜间模式或非夜间模式\n在夜间模式下:\n使用alt键加=(或+)键增加暗度\n使用alt键加-键减小暗度\n使用alt键+Backspace键恢复默认");
   gtk_widget_show(night_mode_image);
   gtk_widget_show(night_mode_button);
   g_object_get (browser, "statusbar", &tmp, NULL);
   gtk_box_pack_end ((GtkBox*) tmp, night_mode_button, FALSE, FALSE, (guint) 3);
   g_signal_connect(G_OBJECT(night_mode_button),"clicked",G_CALLBACK(night_mode_function_realization),browser);
   g_signal_connect (extension, "deactivate",G_CALLBACK ( night_mode_deactivated_cb), browser);              
}

static void 
night_mode_activated_cb (MidoriExtension* extension, 
                         MidoriApp*       app) 
{
   g_night_mode = false; 
   GList* browser_it = NULL;
   GList* browser_collection = midori_app_get_browsers (app);

   for (browser_it = browser_collection; browser_it != NULL; browser_it = browser_it->next) 
        {
      MidoriBrowser* browser = (MidoriBrowser*) browser_it->data;
      night_mode_extension_add_browser_cb(app,browser,extension);
        }
   g_signal_connect (app, "add-browser",
       G_CALLBACK (night_mode_extension_add_browser_cb), extension);
}

MidoriExtension* 
extension_init (void) 
{
   MidoriExtension* extension = g_object_new (MIDORI_TYPE_EXTENSION,
      "name",_("Night mode"),
      "description", _("Protect the eyesight and preventing the myopia,adjust the brightness and night mode"),
      "version", "1.0" MIDORI_VERSION_SUFFIX,
      "authors", "luy_os@sari.ac.cn",
      NULL);
   g_signal_connect (extension, "activate",
       G_CALLBACK ( night_mode_activated_cb), NULL);
   return extension;
}
