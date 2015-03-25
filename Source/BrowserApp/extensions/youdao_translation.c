//add by luyue 2014/12/29

#include "midori/midori.h"



GtkWidget *fanyi_button;

static void
youdao_translation_deactivated_cb (MidoriExtension* extension,
                                   MidoriBrowser*   browser)
{
   if(fanyi_button)
      gtk_widget_destroy (fanyi_button);
}

static void
youdao_translation_function_realization (GtkWidget* botton,MidoriBrowser* browser)
{
   gchar *script=NULL;
   FILE *fp;
   int file_size;
   GtkWidget* current_web_view;

  MidoriView* view = MIDORI_VIEW (midori_browser_get_current_tab (browser));
  //对www.baidu.com/s进行特别处理，使得翻译在该模式下无效
  //add start
  gchar* uri = midori_tab_get_uri (MIDORI_TAB (view)); 
  if(strncmp(uri,"https://www.baidu.com",strlen("https://www.baidu.com"))==0 ||
     strncmp(uri,"http://www.baidu.com",strlen("http://www.baidu.com"))==0)
     return;
  //add end
  if((fp=fopen(midori_paths_get_res_filename("fanyi/seed.js"),"r"))!=NULL)
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
     g_free(script);
     script=NULL;
  }
}

static void youdao_translation_extension_browser_added_cb (MidoriApp*       app,
                                                           MidoriBrowser*   browser,
                                                           MidoriExtension* extension)
{
   GtkStatusbar* tmp = NULL;

   GtkWidget *fanyi_image = gtk_image_new_from_file(midori_paths_get_res_filename("fanyi/fanyi.png"));
   fanyi_button = gtk_button_new();
   gtk_container_add(GTK_CONTAINER(fanyi_button), fanyi_image);
   gtk_widget_set_tooltip_text(fanyi_button,_("english translation"));
   gtk_widget_show(fanyi_image);
   gtk_widget_show(fanyi_button);
   g_object_get (browser, "statusbar", &tmp, NULL);
   gtk_box_pack_end ((GtkBox*) tmp, fanyi_button, FALSE, FALSE, (guint) 3);
   g_signal_connect(G_OBJECT(fanyi_button),"clicked",G_CALLBACK(youdao_translation_function_realization),browser);
}

static void 
youdao_translation_activated_cb (MidoriExtension* extension, 
                                      MidoriApp*       app) 
{
   GList* browser_it = NULL;
   GList* browser_collection = midori_app_get_browsers (app);

   for (browser_it = browser_collection; browser_it != NULL; browser_it = browser_it->next) 
   {
      MidoriBrowser* browser = (MidoriBrowser*) browser_it->data;
      youdao_translation_extension_browser_added_cb(app,browser,extension);
   }
   g_signal_connect (app, "add-browser",
       G_CALLBACK (youdao_translation_extension_browser_added_cb), extension);
}

MidoriExtension* 
extension_init (void) 
{
   MidoriExtension* extension = g_object_new (MIDORI_TYPE_EXTENSION,
      "name", _("Youdao translation"),
      "description", _("Translate English web pages into Chinese"),
      "version", "2.0" MIDORI_VERSION_SUFFIX,
      "authors", "luy_os@sari.ac.cn",
      NULL);

   g_signal_connect (extension, "activate",
       G_CALLBACK ( youdao_translation_activated_cb), NULL);
   g_signal_connect (extension, "deactivate",
       G_CALLBACK ( youdao_translation_deactivated_cb), NULL);

   return extension;
}
