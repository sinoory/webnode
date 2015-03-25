//add by luyue 2015/1/10

#include "midori/midori.h"

GtkWidget *read_mode_button;

static void
read_mode_deactivated_cb (MidoriExtension* extension,
                          MidoriBrowser*   browser)
{
   if(read_mode_button)
      gtk_widget_destroy (read_mode_button);
}

static void
read_mode_function_realization (GtkWidget* botton,MidoriBrowser* browser)
{
   GtkWidget* current_web_view;
   char *script=NULL;
   FILE *fp;
   unsigned int file_size;

  MidoriView* view = MIDORI_VIEW (midori_browser_get_current_tab (browser));
  if((fp=fopen(midori_paths_get_res_filename("read_mode/readability.js"),"r"))!=NULL)
     {
     fseek(fp,0,SEEK_END);
     file_size=ftell(fp);
     fseek(fp,0,SEEK_SET);
     script=(char *)malloc(file_size * sizeof(char)+1);
     fread(script,file_size,sizeof(char),fp);
     script[file_size*sizeof(char)]='\0';
     fclose(fp);
      }
  char *insert_js = "(function(){readConvertLinksToFootnotes=false;readStyle='style-newspaper';readSize='size-medium';readMargin='margin-wide';})();";
  current_web_view = midori_view_get_web_view (view);
  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW (current_web_view), insert_js, NULL, NULL, NULL);
  webkit_web_view_run_javascript(WEBKIT_WEB_VIEW (current_web_view), script, NULL, NULL, NULL);
  free(script);
  script=NULL;
}

static void read_mode_extension_browser_added_cb (MidoriApp*       app,
                                                  MidoriBrowser*   browser,
                                                  MidoriExtension* extension)
{
   GtkStatusbar* tmp = NULL;

   GtkWidget *read_mode_image = gtk_image_new_from_file(midori_paths_get_res_filename("read_mode/icon-19.png"));
   read_mode_button = gtk_button_new();
   gtk_container_add(GTK_CONTAINER(read_mode_button), read_mode_image);
   gtk_widget_set_tooltip_text(read_mode_button,_("Read mode"));
   gtk_widget_show(read_mode_image);
   gtk_widget_show(read_mode_button);
   g_object_get (browser, "statusbar", &tmp, NULL);
   gtk_box_pack_end ((GtkBox*) tmp, read_mode_button, FALSE, FALSE, (guint) 3);
   g_signal_connect(G_OBJECT(read_mode_button),"clicked",G_CALLBACK(read_mode_function_realization),browser);
}

static void 
read_mode_activated_cb (MidoriExtension* extension, 
                        MidoriApp*       app) 
{
   GList* browser_it = NULL;
   GList* browser_collection = midori_app_get_browsers (app);

   for (browser_it = browser_collection; browser_it != NULL; browser_it = browser_it->next) 
   {
      MidoriBrowser* browser = (MidoriBrowser*) browser_it->data;
      read_mode_extension_browser_added_cb(app,browser,extension);
   }
   g_signal_connect (app, "add-browser",
       G_CALLBACK (read_mode_extension_browser_added_cb), extension);
}

MidoriExtension* 
extension_init (void) 
{
   MidoriExtension* extension = g_object_new (MIDORI_TYPE_EXTENSION,
      "name", _("Read mode"),
      "description", _("Let article is more conducive to reading"),
      "version", "1.0" MIDORI_VERSION_SUFFIX,
      "authors", "luy_os@sari.ac.cn",
      NULL);

   g_signal_connect (extension, "activate",
       G_CALLBACK ( read_mode_activated_cb), NULL);
   g_signal_connect (extension, "deactivate",
       G_CALLBACK ( read_mode_deactivated_cb), NULL);

   return extension;
}
