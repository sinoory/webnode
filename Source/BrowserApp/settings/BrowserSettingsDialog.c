/*
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

#include "BrowserSettingsDialog.h"
#include "midori-panel.h"
#include "../midori/Certificate.h"
#include "panels/midori-extensions.h"
#include "../midori/password-manager.h"


//lxx add +, 20141210
#if 1
#define FontNum 118
#define FontSizeNum 5
#define PageZoomNum 16
//lxx add fontNum = 118, fontSizeNum = 5, pageZoomNum = 16
const gchar* font[]  =    {"sans-serif", "AR PL UKai CN", "AR PL UKai HK", "AR PL UKai TW",
                            "AR PL UKai TW MBE", "AR PL UMing CN", "AR PL UMing HK", "AR PL UMing TW",
                            "AR PL UMing TW MBE", "Droid Sans", "文泉驿微米黑", "文泉驿正黑",
                            "文泉驿点阵正黑", "文泉驿等宽微米黑", "文泉驿等宽正黑", "方正书宋_GBK",
                            "方正仿宋_GBK", "方正姚体_GBK", "方正宋体S-超大字符集", "方正小标宋_GBK",
                            "方正楷体_GBK", "方正细黑——_GBK", "方正行楷_GBK", "方正超粗黑_GBK",
                            "方正隶书_GBK", "方正魏碑_GBK", "方正黑体_GBK", "serif",
                            "monospace", "Abyssinica SIL", "Bitstream Charter", "Century Schoolbook L",
                            "Courier 10 pitch", "DejaVu Sans", "DejaVu Sans Mono", "DejaVu Serif",
                            "Dingbats", "Droid Sans Mono", "Droid Serif", "FreeMono",
                            "FreeSans", "FreeSerif", "Garuda", "kacstArt",
                            "kacstBook", "kacstDecorative", "kacstDigital", "kacstFarsi",
                            "kacstLetter", "kacstNaskh", "kacstOffice", "kacstOne",
                            "kacstPen", "kacstPoster", "kacstQurn", "kacstScreen",
                            "kacstTitle", "kacstTitleL", "Kedage", "Khmer OS",
                            "Khmer OS System", "Kinnari", "LKLUG", "Liberation Mono",
                            "Liberation Sans", "Liberation Sans Narrow", "Liberation Serif",
                            "Lohit Bengali", "Lohit Gujarati", "Lohit Hindi", "Lohit Punjabi",
                            "Lohit Tamil", "MT Extra", "Mallige", "Meera",
                            "Mukti Narrow", "NanumGothic", "NanumMyeongjo", "Nimbus Mono L",
                            "Nimbus Roman No9 L", "Nimbus Sans L", "Norasi", "OpenSymbol",
                            "Padauk", "Padauk Book", "Phetsarath OT", "Pothana2000", 
                            "Purisa", "Rachana", "Rekha", "Saab", 
                            "Sawasdee", "Standard Symbols L", "Symbol", "TakaoPGothic",
                            "Tibetan Machine Uni", "Tlwg Typist", "Tlwg Typo", "TlwgMono",
                            "TlwgTypewriter", "URW Bookman L", "URW Chancery L", "URW Gothic L",
                            "URW Palladio L", "Ubuntu", "Ubuntu Condensed", "Ubuntu Mono",
                            "Umpush", "Vemana2000", "Waree", "Webdings",
                            "Wingdings", "Wingdings 2", "Wingdings 3", "gargi",
                            "mry_KacstQurn", "ori1Uni", "方正宋体S-超大字符集(SIP)"  
           
                          };


const  guint32 font_size[] = {9, 12, 16, 20, 24 
                           
                             };


const gdouble zoom_factor[] = {0.25, 0.33, 0.5, 0.67, 0.75, 0.9, 1.0, 1.1, 1.25, 1.5, 1.75, 2.0,
                               2.5, 3.0, 4.0, 5.0  
                                 
                              };

const gdouble smart_zoom_factor[] = {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0};
#endif   

/* 创建一个新的纵向盒,它包含一个图像和一个标签,并返回这个盒。*/
GtkWidget *xpm_label_box( gchar *xpm_filename, gchar* label_text )
{
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *image;
/* 为图像和标签创建盒 */
	box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box), 2);
/* 创建一个图像 */
	image = gtk_image_new_from_file (xpm_filename);
/* 为按钮创建一个标签 */
	label = gtk_label_new (label_text);
	gtk_label_set_width_chars(GTK_LABEL(label), 8);
/* 把图像和标签组装到盒子里 */
	gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (image);
	gtk_widget_show (label);
	
	return box;
}

static void onStartupCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "load-on-startup", CurrentSelect,
             NULL);
}

static void openNewpageCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "open-new-pages-in", CurrentSelect,
             NULL);
}

static void currentPageToHomePageCallback(GtkButton *button, MidoriWebSettings *settings) 
{
    MidoriApp *app = midori_app_get_default();
    MidoriBrowser *browser = midori_app_get_browser(app);
		 GtkWidget* tab = midori_browser_get_current_tab(browser);
		 const gchar* uri = midori_tab_get_uri (tab);
    gtk_entry_set_text(GTK_ENTRY(settings->entry1_general), /*settings->current_uri*/uri);
}

static void homePageCallback(GtkEntry *entry, MidoriWebSettings *settings) 
{
    const gchar * entry_content =  gtk_entry_get_text(GTK_ENTRY(entry/*settings->entry1_general*/));

    g_object_set(settings, "homepage", entry_content, NULL);
}

static void muchTabWarningCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "much_tab_warning", bvalue,
             NULL);
} 

static void closeWindowWarningCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton);
    g_object_set(settings,
             "close_window_warning", bvalue,
             NULL);
}

static void showStatusBarCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "show-statusbar", bvalue,
             NULL);
} 

static void showBookmarkbarCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "show_bookmarkbar", bvalue,
             NULL);
} 

static void showMenubarCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "show_menubar", bvalue,
             NULL);
} 

static void defaultFontCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "default-font-family", font[CurrentSelect],
             NULL);
}

static void serifFontCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "serif-font-family", font[CurrentSelect],
             NULL);
}

static void sansSerifFontCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "sans-serif-font-family", font[CurrentSelect],
             NULL);

		gchar *strval = NULL;
		g_object_get(settings, "sans-serif-font-family", &strval, NULL);
}

static void monospaceFontCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "monospace-font-family", font[CurrentSelect],
             NULL);
}

static gint getFontFamilyComboboxIndex(MidoriWebSettings *settings, const char *fontFamily)
{
	int index = 0;
	gchar *strval;
	g_object_get(settings, fontFamily, &strval, NULL);
	for(index = 0; index < FontNum; index++) 
	{
		if(!g_strcmp0(font[index], strval)) 
		{
			g_free(strval);
			return index;
	      }
	}
	g_warning("undefined string: default-font-family  %s\n", strval);
	g_free(strval);
	return -1;
}

static void defaultFontsizeCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{ 
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "default-font-size", font_size[CurrentSelect],
             NULL);
}

static void defaultMonospaceFontsizeCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{ 
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "default-monospace-font-size", font_size[CurrentSelect],
             NULL);
}

gint getFontSizeComboboxIndex(MidoriWebSettings *settings, const char *fontSize)
{
	int index = 0;
	int size;
	g_object_get(settings, fontSize, &size, NULL);
	for(index = 0; index < FontSizeNum; index++) 
	{
      if(font_size[index] == size) 
		{
			return index;   
		}
	}
	
	return -1;
}

gint getZoomLevelComboboxIndex(MidoriWebSettings *settings)
{
	int index = 0;
	gdouble dvalue;
	g_object_get(settings, "zoom-level", &dvalue, NULL);
	for(index = 0; index < PageZoomNum; index++) 
	{
      if(zoom_factor[index] == dvalue)
		{
			return index;   
		}
	}
	
	return -1;
}

gint getSmartZoomLevelComboboxIndex(MidoriWebSettings *settings)
{
	int index = 0;
	gdouble dvalue;
	g_object_get(settings, "smart-zoom-level", &dvalue, NULL);
	for(index = 0; index < 11; index++) 
	{
      if(smart_zoom_factor[index] == dvalue)
		{
			return index;   
		}
	}
	
	return -1;
}

static void zoomTextOnlyCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 

    g_object_set(settings,
             "zoom-text-and-images", !bvalue,
             NULL);
}

//add by luyue 2015/3/18
static void dangerUrlCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton);
    g_object_set(settings,
                 "danger-url", !bvalue,
                 NULL);
}
//add end

//add by luyue 2015/6/8 start
static void phishCheckCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton);
    g_object_set(settings,
                 "phish-check", !bvalue,
                 NULL);
}
//add end

//add by luyue 2015/3/26
static void rememberWebPasswordCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton);
    if(1 == !bvalue)
       gtk_widget_set_sensitive(GTK_WIDGET(settings->remember_password_button),false);
    else if(0 == !bvalue)
       gtk_widget_set_sensitive(GTK_WIDGET(settings->remember_password_button),true);
    g_object_set(settings,
             "remember-web-password", !bvalue,
             NULL);
}
//add end

static void smartZoomCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 

	 if(0 == bvalue)
	  	gtk_widget_set_sensitive(GTK_WIDGET(settings->smart_zoom_combo_box_content),false);
	 else if(1 == bvalue)
		gtk_widget_set_sensitive(GTK_WIDGET(settings->smart_zoom_combo_box_content),true);

    g_object_set(settings,
             "smart-zoom", bvalue,
             NULL);
}

static void pageZoomCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget);
    g_object_set(settings,
             "zoom-level", zoom_factor[CurrentSelect],
             NULL);
}

static void smartZoomLevelCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget);
    g_object_set(settings,
             "smart-zoom-level", smart_zoom_factor[CurrentSelect],
             NULL);
}

static void showImageCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue;
    if(gtk_toggle_button_get_active(togglebutton)) {
      if((void *)settings->radiobutton1_content == (void *)togglebutton) {
        bvalue = TRUE;
      }
      else if((void *)settings->radiobutton2_content == (void *)togglebutton) {
        bvalue = FALSE;
      }
      else {
        return;
      }
      g_object_set(settings,
               "auto_load_images", bvalue,
               NULL);
    }
} 

static void runJavascriptCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 

    g_object_set(settings, "enable_scripts", bvalue, NULL);
}

static void JavascriptCanOpenWindowsAutomaticallyCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 

	 webkit_settings_set_javascript_can_open_windows_automatically(settings, bvalue);

    g_object_set(settings, "javascript-can-open-windows-automatically", bvalue, NULL);
}

static void pageContentCacheCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "enable-page-cache", bvalue,
             NULL);

	webkit_settings_set_enable_page_cache(settings, bvalue);
} 

static void historySettingCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    gint CurrentSelect = 0;//the value will be set 
    gint old_value = 0; //last time value
    
    g_object_get(settings,"history-setting", &old_value, NULL);//get last time value????
    CurrentSelect = gtk_combo_box_get_active(widget); //get value that will be set

    if(old_value == CurrentSelect)return;
    g_object_set(settings, "history-setting", CurrentSelect, NULL);

    MidoriApp *app = midori_app_get_default();
    MidoriBrowser *browser = midori_app_get_browser(app);
    midori_browser_change_history_seting(browser, &CurrentSelect);
}

static void clearBrowseRecordCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "clear-browse-record", bvalue,
             NULL);
} 


static void clearDownloadRecordCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "clear-download-record", bvalue,
             NULL);
} 


static void clearCookieAndOthersCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "clear-cookie-and-others", bvalue,
             NULL);
}

//add by luyue 2015/4/11 start
static void clearOpenTabsCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton);
    g_object_set(settings,
             "clear-open-tabs", bvalue,
             NULL);
}
//add end

static void clearCachedImagesAndFilesCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "clear-cached-images-and-files", bvalue,
             NULL);
}

static void clearPasswordsCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "clear-passwords", bvalue,
             NULL);
}

static void doNotTrackCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "do-not-track", bvalue,
             NULL);
}

static void cookieSettingCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
   int iCurrentSelect = gtk_combo_box_get_active(widget); 
   WebKitCookieAcceptPolicy cookiePolicy = WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS;
   switch(iCurrentSelect)
   {
      case 0:
         cookiePolicy = WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS;
	 break;
      case 1:
	 cookiePolicy = WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY;
	 break;
      case 2:
	 cookiePolicy = WEBKIT_COOKIE_POLICY_ACCEPT_NEVER;
         break;
      default:
	 g_warning("error cookieSetting callback\n");
   }			
   g_object_set(settings, "cookie-setting", iCurrentSelect, NULL);	
   WebKitCookieManager* cookiemanager = webkit_web_context_get_cookie_manager(webkit_web_context_get_default());
   webkit_cookie_manager_set_accept_policy(cookiemanager,cookiePolicy);
}

static void trackLocationCallback(GtkComboBox *widget, MidoriWebSettings *settings)
{
    int CurrentSelect = gtk_combo_box_get_active(widget); 
    g_object_set(settings,
             "track-location", CurrentSelect,
             NULL);
}

static void showPasswordManagerCallback(GtkButton* savedPasswordButton)
{
    password_manager_display();
}

//clear data callback 
static void clearDataCallback(GtkButton *button, MidoriWebSettings *settings) 
{
	 MidoriApp *app = midori_app_get_default();
    MidoriBrowser *browser = midori_app_get_browser(app);

    //clear open tabs
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->checkbutton9_privacy))) {
       GList* tabs = midori_browser_get_tabs (browser);
       for (; tabs != NULL; tabs = g_list_next (tabs))
          midori_browser_close_tab (browser, tabs->data);
       g_list_free (tabs);
    }

    //clear browse record.
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->checkbutton4_privacy))) {
		midori_browser_clear_history(browser);
    }

    //clear download record.
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->checkbutton5_privacy))) {
		//TODO clear download record.
		//zgh 清除下载数据
		midori_browser_clear_download_data (browser, GTK_WIDGET(button));
    }

    //clear cookie and others
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->checkbutton6_privacy))) {
        WebKitCookieManager* cookiemanager = webkit_web_context_get_cookie_manager(webkit_web_context_get_default());
        if(cookiemanager){
            webkit_cookie_manager_delete_all_cookies(cookiemanager);
        }
    }

    //clear cached images and files
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->checkbutton7_privacy))) {
        webkit_web_context_clear_cache(webkit_web_context_get_default());
    }

    //clear passwords
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->checkbutton8_privacy))) {
	//TODO clear password
        clear_password_all();
    }     
}

//luyue add
static void certificateManagerCallback(GtkToggleButton *button, MidoriWebSettings *settings)
{
   display_certificatemanager(); 
}

static void certificateRevocationCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(togglebutton); 
    g_object_set(settings,
             "certificate-revocation", bvalue,
             NULL);
}
/*
static void mediaAccessCallback(GtkToggleButton *togglebutton, MidoriWebSettings *settings)
{
    int printf("error mediaAccessCallback\n"); 
        return;
      }
      g_object_set(settings,
               "media-access", ivalue,
               NULL);
    }
}
*/

static bool isCdosbrowserDefault()
{
   gchar result[20] = {0};
   system("xdg-settings get default-web-browser > /tmp/.default-browser");
   FILE *fp = fopen("/tmp/.default-browser","r");
   fgets(result, 20, fp);
   fclose(fp);	
   if(0 == strncmp(result, "cdosbrowser.desktop", 19))
      return 1;
   return 0;
}

static void setCdosDefaultBrowserCallback(GtkButton *button, MidoriWebSettings *settings)
{
   system("xdg-settings set default-web-browser cdosbrowser.desktop");
   gtk_widget_hide(GTK_WIDGET(button));
   gtk_label_set_text (GTK_LABEL(settings->label1_advanced), "目前的默认浏览器是本浏览器."); 
}

static void networkSettingCallback(GtkButton *button, MidoriWebSettings *settings)
{
   system("/usr/bin/cinnamon-settings network");
}

static void do_reset_browser(MidoriWebSettings *settings)
{
//删除config文件
   g_object_set(settings, "reset-browser-settings", true, NULL);

//删除历史记录的
   MidoriApp *app = midori_app_get_default();
   MidoriBrowser *browser = midori_app_get_browser(app);
   midori_browser_clear_history(browser);
//删除书签的
//TODO

//删除各种临时数据和缓存数据
//clear Cookie　and others
   WebKitCookieManager* cookiemanager = webkit_web_context_get_cookie_manager(webkit_web_context_get_default());
   if(cookiemanager)
      webkit_cookie_manager_delete_all_cookies(cookiemanager);

//clear cached images and files
   webkit_web_context_clear_cache(webkit_web_context_get_default());
}

static void resetNetworkSettingCallback(GtkButton *button, MidoriWebSettings *settings)
{
   GtkWidget *dialog;
   dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
                                    "该操作将重置您的主页、将您的常规设置、\n隐私和安全等设置恢复为原始默认设置，还\n将删除您的书签、历史记录以及临时数据和\n缓存数据（例如Cookie）。");
   gtk_window_set_title (GTK_WINDOW (dialog), "确实要重置浏览器吗？");
   gint result = gtk_dialog_run(GTK_DIALOG (dialog));
   if(result == GTK_RESPONSE_CANCEL)
      gtk_widget_destroy(dialog);
   else if(result == GTK_RESPONSE_OK)
   {
      do_reset_browser(settings);
      gtk_widget_destroy(dialog);
   }
}

static void alterDownloadSaveCatalogCallback(GtkButton *button, MidoriWebSettings *settings)
{
   GtkWidget *dialog;  
   GtkWidget *entry;
   //创建文件选择对话框
   dialog = gtk_file_chooser_dialog_new ("保存目录",
				         NULL,/*gtk_widget_get_toplevel(GTK_WINDOW(button)),*/
					 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,//GTK_FILE_CHOOSER_ACTION_SAVE, 
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,/**/
					 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					 NULL);
   entry = GTK_ENTRY(settings->entry1_advanced);
   //文件选择类型过滤

   //设置当前文件夹
   gchar local_path[50] = {0};
   gchar *path = getenv("HOME");
   strcat(local_path, path);
   strcat(local_path, "/下载");
   gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog), local_path);
   gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_CENTER_ON_PARENT);
   //等待用户的动作
   if(gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
   {
      //取得用户所选文件的路径
      gchar *filename;
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      //把文件路径显示到输入条上
      gtk_entry_set_text(GTK_ENTRY(entry),filename);
   }
   gtk_widget_destroy(dialog);
}

static void downloadPathCallback(GtkEntry *entry, MidoriWebSettings *settings) 
{
   const gchar *entry_content =  gtk_entry_get_text(GTK_ENTRY(entry));
   g_object_set(settings, "download-folder", entry_content, NULL);
}

static void askEverytimeBeforeDownloadCallback(GtkToggleButton *button, MidoriWebSettings *settings)
{
    bool bvalue = gtk_toggle_button_get_active(button); 
    g_object_set(settings,
                 "ask_every_time_before_download_file", bvalue,
                 NULL);
}

GtkWidget * browser_settings_window_new(MidoriWebSettings *settings)
{

   GtkWidget *window;
   GtkWidget *button;
   GtkWidget *notebook;
   GtkWidget *label;
   GtkWidget *widget;
   GtkGrid *grid;
   GSList *group;
   int i;
 
   window = gtk_window_new (GTK_WINDOW_TOPLEVEL/*GTK_WINDOW_POPUP*/);
   gtk_window_set_resizable (GTK_WINDOW(window), FALSE);
   gtk_window_set_title (GTK_WINDOW (window), "设置");
   gtk_container_set_border_width (GTK_CONTAINER (window), 0);
   gtk_window_set_position(GTK_WINDOW (window),GTK_WIN_POS_CENTER);
   /* 创建一个新的笔记本,将标签页放在顶部 */
   notebook = gtk_notebook_new ();
   gtk_container_set_border_width (GTK_CONTAINER (notebook), 0);
   gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
   gtk_container_add (GTK_CONTAINER (window), notebook);
   /* 在笔记本后面追加几个页面 */
   //general
   grid = (GtkGrid*)gtk_grid_new();//创建网格
   gtk_grid_set_row_spacing (grid, 8);
   gtk_grid_set_column_spacing (grid, 5);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 0, 1, 1);	
   label = gtk_label_new("当浏览器启动时：");
   gtk_grid_attach( grid, label, 1, 1, 1, 1);
   widget = gtk_combo_box_text_new();
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "显示空白页");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "显示我的主页");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "显示上次打开的窗口和标签页");
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(onStartupCallback), settings);
   gint ivalue = katze_object_get_int (settings, "load-on-startup");
   switch(ivalue) {
      case 0://blank page
         gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
	 break;
      case 1://homepage
	 gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 1);
	 break;
      case 2://last opened pages
	 gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 2);
	 break;
      default:
	 break;
   }
   gtk_grid_attach( grid, widget, 2, 1, 2, 1);
   widget = gtk_label_new("主页：");
   gtk_grid_attach( grid, widget, 1, 3, 1, 1);
   button = gtk_entry_new();
   settings->entry1_general = GTK_WIDGET(button);
   gchar *strval = katze_object_get_string(settings, "homepage");
   gtk_entry_set_text(GTK_ENTRY(button), strval);
   g_free (strval);
   g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(homePageCallback), settings);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_RIGHT, 2, 1);
   widget = gtk_button_new_with_label("使用当前页面");
   g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(currentPageToHomePageCallback), settings);
   gtk_grid_attach_next_to(grid, widget, button, GTK_POS_RIGHT, 1, 1);
   label = gtk_label_new("    ");
   gtk_grid_attach_next_to(grid, label, widget, GTK_POS_RIGHT, 1, 1);
   widget = gtk_label_new("标签：");
   gtk_grid_attach(grid, widget, 1, 4, 1, 1);
   button = gtk_combo_box_text_new();
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), "始终以新标签页打开网页");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), "始终以新窗口打开网页");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), "始终在当前标签页打开网页");
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_RIGHT, 2, 1);
   g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(openNewpageCallback), settings);
   ivalue = katze_object_get_int(settings, "open-new-pages-in");
   switch(ivalue) {
      case 0:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(button), 0);
	 break;
      case 1:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(button), 1);
         break;
      case 2:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(button), 2);
	 break;
      default:
	 break;
   }
   widget = gtk_check_button_new_with_label("打开多个标签可能致使浏览器速度缓慢时警告我");
   bool bvalue = katze_object_get_boolean(settings, "much_tab_warning");
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
   g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(muchTabWarningCallback), settings);
   gtk_grid_attach_next_to(grid, widget, button, GTK_POS_BOTTOM, 1, 1);        
   button = gtk_check_button_new_with_label("关闭多个标签页时警告我");
   bvalue = katze_object_get_boolean(settings, "close_window_warning");
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(closeWindowWarningCallback), settings);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_BOTTOM, 1, 1);
   widget = gtk_label_new("外观：");
   gtk_grid_attach(grid,widget,1,7,1,1);
   button = gtk_check_button_new_with_label("显示状态栏");
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(showStatusBarCallback), settings);
   bvalue = katze_object_get_boolean(settings, "show-statusbar");
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE); 
   gtk_grid_attach(grid,button,2,8,1,1);
   widget = gtk_check_button_new_with_label("显示书签栏");
   g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(showBookmarkbarCallback), settings);
   bvalue = katze_object_get_boolean(settings, "show_bookmarkbar");
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
   gtk_grid_attach_next_to(grid, widget, button, GTK_POS_BOTTOM, 1, 1);
   button = gtk_check_button_new_with_label("显示菜单栏");
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(showMenubarCallback), settings);
   bvalue = katze_object_get_boolean(settings, "show_menubar");
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_BOTTOM, 1, 1);
   gchar *general_pic = midori_paths_get_res_filename("settings-icons/general.png");
   label = xpm_label_box( general_pic, "常 规" );
   g_free(general_pic);
   gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET(grid), GTK_WIDGET(label));
   grid = (GtkGrid*)gtk_grid_new();//创建网格
   gtk_grid_set_row_spacing (grid, 8);
   gtk_grid_set_column_spacing (grid, 5);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 0, 1, 1);
   widget = gtk_label_new("字体：");
   gtk_grid_attach(grid, widget, 1, 1, 1, 1);
   widget = gtk_label_new("默认字体：");
   gtk_grid_attach(grid, widget, 2, 2, 1, 1);
   button = gtk_combo_box_text_new();
   for(i = 0; i < FontNum; i++)
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), font[i]);
   gint index = getFontFamilyComboboxIndex(settings, "default-font-family");
   gtk_combo_box_set_active(GTK_COMBO_BOX(button), index);
   g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(defaultFontCallback), settings);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_RIGHT, 1, 1);
   //size of default font
   widget = gtk_label_new("字号：");
   gtk_grid_attach_next_to(grid, widget, button, GTK_POS_RIGHT, 1, 1);
   button = gtk_combo_box_text_new();
   for(i = 0; i < FontSizeNum; i++)
   {
      char ic[10] = {0};
      sprintf(ic, "%d", font_size[i]);
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), ic);
   }
   index = getFontSizeComboboxIndex(settings, "default-font-size");
   gtk_combo_box_set_active(GTK_COMBO_BOX(button), index);
   g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(defaultFontsizeCallback), settings);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_RIGHT, 1, 1);
   widget = gtk_label_new("Serif：");  
   gtk_grid_attach(grid, widget, 2, 3, 1, 1);
   button = gtk_combo_box_text_new();
   for(i = 0; i < FontNum; i++)
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), font[i]);
   index = getFontFamilyComboboxIndex(settings, "serif-font-family");
   gtk_combo_box_set_active(GTK_COMBO_BOX(button), index);
   g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(serifFontCallback), settings);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_RIGHT, 1, 1);

   //Sans-serif font
   widget = gtk_label_new("Sans-serif：");
   gtk_grid_attach(grid, widget, 2, 4, 1, 1);
   button = gtk_combo_box_text_new();
   for(i = 0; i < FontNum; i++)
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), font[i]);
   index = getFontFamilyComboboxIndex(settings, "sans-serif-font-family");
   gtk_combo_box_set_active(GTK_COMBO_BOX(button), index);
   g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(sansSerifFontCallback), settings);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_RIGHT, 1, 1);

   //Monospace font
   widget = gtk_label_new("Monospace：");
   gtk_grid_attach(grid, widget, 2, 5, 1, 1);
   button = gtk_combo_box_text_new();
   for(i = 0; i < FontNum; i++)
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), font[i]);
   index = getFontFamilyComboboxIndex(settings, "monospace-font-family");
   gtk_combo_box_set_active(GTK_COMBO_BOX(button), index);
   g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(monospaceFontCallback), settings);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_RIGHT, 1, 1);

   //size of monospace font
   widget = gtk_label_new("字号：");
   gtk_grid_attach_next_to(grid, widget, button, GTK_POS_RIGHT, 1, 1);
   button = gtk_combo_box_text_new();
   for(i = 0; i < FontSizeNum; i++)
   {
      char ic[10] = {0};
      sprintf(ic, "%d", font_size[i]);
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button), ic);
   }
   index = getFontSizeComboboxIndex(settings, "default-monospace-font-size");
   gtk_combo_box_set_active(GTK_COMBO_BOX(button), index);
   g_signal_connect(G_OBJECT(button), "changed", G_CALLBACK(defaultMonospaceFontsizeCallback), settings);
   gtk_grid_attach_next_to(grid, button, widget, GTK_POS_RIGHT, 1, 1);
   gchar *content_pic = midori_paths_get_res_filename("settings-icons/fonts.png");
   label = xpm_label_box( content_pic, "字体" );
   g_free(content_pic);
   gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET(grid), label);

   //content
   grid = (GtkGrid*)gtk_grid_new();//创建网格
   gtk_grid_set_row_spacing (grid, 8);
   gtk_grid_set_column_spacing (grid, 5);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 0, 1, 1);
   widget = gtk_label_new("网页缩放：");
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_RIGHT);	
   gtk_grid_attach(grid, widget, 1, 4, 1, 1);
   widget = gtk_label_new("缩放比例：");
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_grid_attach(grid, widget, 2, 5, 1, 1);
   widget = gtk_combo_box_text_new();
   for(i = 0; i < PageZoomNum; i++)
   {
      char ic[10] = {0};
      sprintf(ic, "%2.0f%%", zoom_factor[i] * 100);
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), ic);
   }
   index = getZoomLevelComboboxIndex(settings);
   gtk_combo_box_set_active(GTK_COMBO_BOX(widget), index);
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(pageZoomCallback), settings);
   gtk_grid_attach(grid, widget, 3, 5, 1, 1);
   button = gtk_check_button_new_with_label("仅对文本缩放");
   bvalue = 1;
   g_object_get(settings, "zoom-text-and-images", &bvalue, NULL);
   if(!bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(zoomTextOnlyCallback), settings);
   gtk_grid_attach(grid,button,2,6,2,1);
   button = gtk_check_button_new_with_label("开启双击缩放功能");
   g_object_get(settings, "smart-zoom", &bvalue, NULL);
   if(bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(smartZoomCallback), settings);
   gtk_grid_attach(grid,button,2,7,3,1);
   widget = gtk_label_new("缩放比例：");
   gtk_label_set_justify (GTK_LABEL(widget), GTK_JUSTIFY_LEFT);
   gtk_grid_attach(grid, widget, 2, 8, 1, 1);
   widget = gtk_combo_box_text_new();
   settings->smart_zoom_combo_box_content = GTK_WIDGET(widget);
   for(i = 0; i < 11; i++)
   {
      char ic[10] = {0};
      if(smart_zoom_factor[i] == 1.0)
      {
         gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "自动");
	 continue;
      }
      sprintf(ic, "%2.0f%%", smart_zoom_factor[i] * 100);
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), ic);
   }
   index = getSmartZoomLevelComboboxIndex(settings);
   gtk_combo_box_set_active(GTK_COMBO_BOX(widget), index);
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(smartZoomLevelCallback), settings);
   gtk_grid_attach(grid, widget, 3, 8, 1, 1);	
   g_object_get(settings, "smart-zoom", &bvalue, NULL);
   if(0 == bvalue)
      gtk_widget_set_sensitive(GTK_WIDGET(widget),FALSE);
   widget = gtk_label_new("图片：");
   gtk_grid_attach(grid, widget, 1, 9, 1, 1);
   button = gtk_radio_button_new_with_label (NULL, "显示所有图片（推荐）");
   settings->radiobutton1_content = GTK_WIDGET(button);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(showImageCallback), settings);
   gtk_grid_attach(grid, button, 2, 10, 2, 1);
   group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
   button = gtk_radio_button_new_with_label (group, "不显示任何图片");
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(showImageCallback), settings);
   settings->radiobutton2_content = GTK_WIDGET(button);
   g_object_get(settings, "auto_load_images", &bvalue, NULL);   
   if(TRUE == bvalue) 
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->radiobutton1_content), TRUE);     
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->radiobutton2_content), TRUE); 
   gtk_grid_attach(grid, button, 2, 11, 2, 1);
   widget = gtk_label_new("JavaScript：");
   gtk_grid_attach(grid, widget, 1, 12, 1, 1);
      button = gtk_check_button_new_with_label("允许所有网站运行JavaScript");
   bvalue = 1;
   g_object_get(settings, "enable_scripts", &bvalue, NULL);
   if(bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(runJavascriptCallback), settings);
   gtk_grid_attach(grid,button,2,13,2,1);
   button = gtk_check_button_new_with_label("允许所有网站显示弹出式窗口");
   bvalue = 1;
   g_object_get(settings, "javascript-can-open-windows-automatically", &bvalue, NULL);
   if(bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(JavascriptCanOpenWindowsAutomaticallyCallback), settings);
   gtk_grid_attach(grid,button,2,14,2,1);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 15, 1, 1);
   gchar *font_pic = midori_paths_get_res_filename("settings-icons/content.png");
   label = xpm_label_box( font_pic, "内 容" );
   g_free(font_pic);
   gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET(grid), label);

   //privacy
   grid = (GtkGrid*)gtk_grid_new();//创建网格
   gtk_grid_set_row_spacing (grid, 8);
   gtk_grid_set_column_spacing (grid, 5);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 0, 1, 1);
   widget = gtk_label_new("缓存：");
   gtk_grid_attach(grid, widget, 1, 1, 1, 1);
   button = gtk_check_button_new_with_label("开启网页内容缓存");
   g_object_get(settings, "enable-page-cache", &bvalue, NULL);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE); 
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(pageContentCacheCallback), settings);
   gtk_grid_attach(grid, button, 2, 2, 1, 1);
   widget = gtk_label_new("清除数据：");
   gtk_grid_attach(grid, widget, 1, 3, 1, 1);
   button = gtk_check_button_new_with_label("历史记录");
   settings->checkbutton4_privacy = GTK_WIDGET(button);
   g_object_get(settings, "clear-browse-record", &bvalue, NULL);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE); 
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(clearBrowseRecordCallback), settings);
   gtk_grid_attach(grid, button, 2, 4, 1, 1);
   button = gtk_check_button_new_with_label("下载记录");
   settings->checkbutton5_privacy = GTK_WIDGET(button);
   g_object_get(settings, "clear-download-record", &bvalue, NULL);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE); 
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(clearDownloadRecordCallback), settings);
   gtk_grid_attach(grid, button, 3, 4, 1, 1);
   button = gtk_check_button_new_with_label("网页缓存");
   settings->checkbutton7_privacy = GTK_WIDGET(button);
   g_object_get(settings, "clear-cached-images-and-files", &bvalue, NULL);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE); 
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(clearCachedImagesAndFilesCallback), settings);
   gtk_grid_attach(grid, button, 2, 5, 1, 1);
   button = gtk_check_button_new_with_label("密码");
   settings->checkbutton8_privacy = GTK_WIDGET(button);
   g_object_get(settings, "clear-passwords", &bvalue, NULL);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE); 
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(clearPasswordsCallback), settings);
   gtk_grid_attach(grid, button, 4, 4, 1, 1);
   button = gtk_check_button_new_with_label("隐私信息和网站数据");
   settings->checkbutton6_privacy = GTK_WIDGET(button);
   g_object_get(settings, "clear-cookie-and-others", &bvalue, NULL);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE); 
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(clearCookieAndOthersCallback), settings);
   gtk_grid_attach(grid, button, 3, 5, 2, 1);

   //luyue add by 2015/4/11 start
   button = gtk_check_button_new_with_label("已打开的标签");
   settings->checkbutton9_privacy = GTK_WIDGET(button);
   g_object_get(settings, "clear-open-tabs", &bvalue, NULL);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(clearOpenTabsCallback), settings);
   gtk_grid_attach(grid, button, 2, 6, 1, 1);
   button = gtk_button_new_with_label("　　清除　　");
   g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(clearDataCallback), settings);
   gtk_grid_attach(grid, button, 3, 8, 1, 1);
   widget = gtk_label_new("历史记录：");
   gtk_grid_attach(grid, widget, 1, 9, 1, 1);
   widget = gtk_combo_box_text_new();
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "记录浏览历史");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "不记录浏览历史");
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(historySettingCallback), settings);
   ivalue = katze_object_get_int(settings, "history-setting");
   switch(ivalue) {
      case 0:
         gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
	 break;
      case 1:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 1);
         break;
      case 2:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 2);
	 break;
      default:
	 break;
   }
   gtk_grid_attach(grid, widget, 2, 9, 1, 1);
   widget = gtk_label_new("Cookie：");
   gtk_grid_attach(grid, widget, 1, 11, 1, 1);
   widget = gtk_combo_box_text_new();
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "允许设置本地数据（推荐）");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "仅将本地数据保留到您退出浏览器为止");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "阻止网站设置任何数据");
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(cookieSettingCallback), settings);
   ivalue = katze_object_get_int(settings, "cookie-setting");
   switch(ivalue) {
      case 0:
         gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
	 break;
      case 1:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 1);
         break;
      case 2:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 2);
	 break;
      default:
	 break;
   }
   gtk_grid_attach(grid, widget, 2, 11, 2, 1);
   widget = gtk_label_new("位置：");
   gtk_grid_attach(grid, widget, 1, 12, 1, 1);
   widget = gtk_combo_box_text_new();
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "不允许任何网站跟踪您所在的位置");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "允许所有网站跟踪您所在的位置");
   gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "当网站要跟踪您所在的位置时询问您（推荐）");
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(trackLocationCallback), settings);
   ivalue = katze_object_get_int(settings, "track-location");
   switch(ivalue) {
      case 0:
         gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
	 break;
      case 1:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 1);
         break;
      case 2:
	 gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 2);
	 break;
      default:
	 break;
   }
   gtk_grid_attach(grid, widget, 2, 12, 2, 1);
   widget = gtk_label_new("防追踪：");
   gtk_grid_attach(grid, widget, 1, 13, 1, 1);
   button = gtk_check_button_new_with_label("随浏览流量一起发送“请勿跟踪”请求");
   g_object_get(settings, "do-not-track", &bvalue, NULL);
   if(TRUE == bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE); 
   else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(doNotTrackCallback), settings);
   gtk_grid_attach(grid, button, 2, 13, 2, 1);
   gchar *privacy_pic = midori_paths_get_res_filename("settings-icons/privacy.png");
   label = xpm_label_box( privacy_pic, "隐 私" );
   g_free(privacy_pic);
   gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET(grid), label);

   //security
   grid = (GtkGrid*)gtk_grid_new();//创建网格
   gtk_grid_set_row_spacing (grid, 8);
   gtk_grid_set_column_spacing (grid, 5);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 0, 1, 1);
   widget = gtk_label_new("密码：");
   gtk_grid_attach(grid, widget, 1, 1, 1, 1);
   button = gtk_check_button_new_with_label("记住网站密码");
   gtk_grid_attach(grid, button, 2, 2, 1, 1);
   g_object_get(settings, "remember-web-password", &bvalue, NULL);
   if(!bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(rememberWebPasswordCallback), settings);
   settings->remember_password_button = gtk_button_new_with_label("已保存密码信息");
   gtk_grid_attach(grid, settings->remember_password_button, 3, 2, 1, 1);
   g_object_get(settings, "remember-web-password", &bvalue, NULL);
   if(1 == bvalue)
      gtk_widget_set_sensitive(GTK_WIDGET(settings->remember_password_button),FALSE);
   g_signal_connect(G_OBJECT(settings->remember_password_button), "clicked", G_CALLBACK(showPasswordManagerCallback), NULL);
   widget = gtk_label_new("HTTPS/SSL:");
   gtk_grid_attach(grid, widget, 1, 3, 1, 1);
   button = gtk_button_new_with_label("管理证书");
   g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(certificateManagerCallback), settings);
   gtk_grid_attach(grid, button, 2, 4, 1, 1);
   //add by luyue 2015/3/18
   widget = gtk_label_new("高危网址检测：");
   gtk_grid_attach(grid, widget, 1, 5, 1, 1);
   button = gtk_check_button_new_with_label("打开高危网址检测功能");
   g_object_get(settings, "danger-url", &bvalue, NULL);
   if(!bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(dangerUrlCallback), settings);
   gtk_grid_attach(grid,button,2,6,2,1);
   label = gtk_label_new("警告:该功能会导致部分网页加载速度变慢");
   gtk_grid_attach(grid,label,2,7,2,1);
   //add end
   //add by luyue 2015/6/8 start
   widget = gtk_label_new("钓鱼网站检测：");
   gtk_grid_attach(grid, widget, 1, 8, 1, 1);
   button = gtk_check_button_new_with_label("打开钓鱼网站检测功能");
   g_object_get(settings, "phish-check", &bvalue, NULL);
   if(!bvalue)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(phishCheckCallback), settings);
   gtk_grid_attach(grid,button,2,9,2,1);
   //add end
   gchar *security_pic = midori_paths_get_res_filename("settings-icons/security.png");
   label = xpm_label_box( security_pic, "安 全" );
   g_free(security_pic);
   gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET(grid), label);

   //advanced
   grid = (GtkGrid*)gtk_grid_new();//创建网格
   gtk_grid_set_row_spacing (grid, 8);
   gtk_grid_set_column_spacing (grid, 5);
   label = gtk_label_new("    ");
   gtk_grid_attach( grid, label, 0, 0, 1, 1);
   widget = gtk_label_new("默认浏览器：");
   gtk_grid_attach(grid, widget, 1, 1, 1, 1);

   //check cdosbrowser default
   if( isCdosbrowserDefault() )
   {
      label = gtk_label_new("目前的默认浏览器是本浏览器.");
      gtk_grid_attach(grid, label, 2, 2, 2, 1);
   }
   else
   {
      button = gtk_button_new_with_label("将本浏览器设置为默认浏览器");
      g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(setCdosDefaultBrowserCallback), settings);
      gtk_grid_attach(grid, button, 2, 2, 2, 1);
      label = gtk_label_new("本浏览器目前不是默认浏览器.");
      settings->label1_advanced = label;
      gtk_grid_attach(grid, label, 2, 3, 2, 1);
   }
   widget = gtk_label_new("网络：");
   gtk_grid_attach(grid, widget, 1, 4, 1, 1);
   button = gtk_button_new_with_label("网络设置");
   g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(networkSettingCallback), settings);
   gtk_grid_attach(grid, button, 2, 5, 1, 1);

   widget = gtk_label_new("重置浏览器：");
   gtk_grid_attach(grid, widget, 1, 6, 1, 1);

   widget = gtk_label_new("将浏览器设置恢复为默认设置");
   gtk_grid_attach(grid, widget, 2, 7, 2, 1);

   button = gtk_button_new_with_label("重置浏览器设置");
   g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(resetNetworkSettingCallback), settings);
   gtk_grid_attach(grid, button, 2, 8, 2, 1);

   widget = gtk_label_new("下载：");
   gtk_grid_attach(grid, widget, 1, 9, 1, 1);

   widget = gtk_label_new("下载内容保存位置：");
   gtk_grid_attach(grid, widget, 2, 10, 1, 1);

   widget = gtk_entry_new();
   settings->entry1_advanced = GTK_WIDGET(widget);
   gtk_entry_set_icon_from_icon_name(GTK_ENTRY(widget), GTK_ENTRY_ICON_PRIMARY, "folder");
   gchar *path = katze_object_get_string(settings, "download-folder");
   gtk_entry_set_text(GTK_ENTRY(widget), path);
   g_free (path);
   g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(downloadPathCallback), settings);
   gtk_grid_attach(grid, widget, 3, 10, 2, 1);
   button = gtk_button_new_with_label("更改...");
   g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(alterDownloadSaveCatalogCallback), settings);
   gtk_grid_attach(grid, button, 5, 10, 1, 1);
   button = gtk_check_button_new_with_label("每次下载时询问下载位置");
   bvalue = katze_object_get_boolean(settings, "ask_every_time_before_download_file");
   if(TRUE == bvalue) 
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);     
   else 
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE); 
   g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(askEverytimeBeforeDownloadCallback), settings);
   gtk_grid_attach(grid, button, 2, 11, 3, 1);
   gchar *advanced_pic = midori_paths_get_res_filename("settings-icons/advanced.png");
   label = xpm_label_box( advanced_pic, "高 级" );
   g_free(advanced_pic);
   gtk_notebook_append_page (GTK_NOTEBOOK (notebook), GTK_WIDGET(grid), label);

   //expand
   gchar *expand_pic = midori_paths_get_res_filename("settings-icons/expand.png");
   label = xpm_label_box( expand_pic, "扩 展" );
   g_free(expand_pic);
   //add by luyue 2014/12/29
   MidoriApp *app = midori_app_get_default();
   KatzeArray* array = katze_object_get_object (app, "extensions");
   midori_extension_load_from_folder (app, NULL, FALSE);
   g_object_set_data (G_OBJECT (app), "extensions", NULL);
   if (!katze_array_get_nth_item (array, 0))
   {
      g_object_unref (array);
      return NULL;
   }
   g_object_unref (array); 
   GtkWidget* scrolled = gtk_scrolled_window_new (NULL, NULL);
   GtkWidget* addon = g_object_new (MIDORI_TYPE_EXTENSIONS, "app", app, NULL);
   GList* children = gtk_container_get_children (GTK_CONTAINER (addon));
   gtk_widget_reparent (g_list_nth_data (children, 0), scrolled);
   g_list_free (children);
   gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled, label);
   gtk_widget_show_all(window);
   return window;
}

