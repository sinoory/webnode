#include "password-manager.h"
#include "midori-app.h"
#include "midori-web-extension-proxy.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <glib.h>

struct _PasswordManagerPrivate{
  GtkWindow* p_auth_data_manager_window;
  GtkListStore* p_auth_data_list_store;
  GtkTreeViewColumn* hostname_view_column;
  GtkTreeViewColumn* username_view_column;
  gboolean p_bShowPassword;
};

GtkCellRenderer* window_renderer;
GtkTreeViewColumn* password_view_column;
GtkTreeView* p_item_list;

const SecretSchema *
ephy_form_auth_data_get_password_schema (void)
{
  static const SecretSchema schema = {
    "org.epiphany.FormPassword", SECRET_SCHEMA_NONE,
    {
      { URI_KEY, SECRET_SCHEMA_ATTRIBUTE_STRING },
      { FORM_USERNAME_KEY, SECRET_SCHEMA_ATTRIBUTE_STRING },
      { FORM_PASSWORD_KEY, SECRET_SCHEMA_ATTRIBUTE_STRING },
      { USERNAME_KEY, SECRET_SCHEMA_ATTRIBUTE_STRING },
      { "NULL", 0 },
    }
  };
  return &schema;
}

G_DEFINE_TYPE (PasswordManager, password_manager, G_TYPE_OBJECT)

static char *
midori_string_get_host_name (const char *url)
{
  SoupURI *uri;
  char *ret;
  
  if (url == NULL ||
      g_str_has_prefix (url, "file://") ||
      g_str_has_prefix (url, "about:") ||
      g_str_has_prefix (url, "ephy-about:"))
    return NULL;

  uri = soup_uri_new (url);
  /* If uri is NULL it's very possible that we just got
   * something without a scheme, let's try to prepend
   * 'http://' */
  if (uri == NULL) {
      char *effective_url = g_strconcat ("http://", url, NULL);
      uri = soup_uri_new (effective_url);
      g_free (effective_url);
  }

  if (uri == NULL) return NULL;

  ret = g_strdup (uri->host);
  soup_uri_free (uri);

  return ret;
}

static void
screcet_service_search_finished (SecretService *service,
                                 GAsyncResult *result,
                                 PasswordManagerPrivate* priv)
{
  GList *results, *p;
  GError *error = NULL;

  results = secret_service_search_finish (service, result, &error);
  if (error != NULL) {
    g_warning ("Error caching form data: %s", error->message);
    g_error_free (error);
    return;
  }
  
  for (p = results; p; p = p->next) {
    SecretItem *item = (SecretItem *)p->data;
    SecretValue* value = NULL;
    GHashTable *attributes;
    char *host;
   
    attributes = secret_item_get_attributes (item);
    host = midori_string_get_host_name (g_hash_table_lookup (attributes, URI_KEY));
    
    secret_item_load_secret_sync(item, NULL, NULL);
    value = secret_item_get_secret(item);
    const char* password = secret_value_get(value, NULL);
    /*ephy_form_auth_data_cache_add (cache, host,
                                   g_hash_table_lookup (attributes, FORM_USERNAME_KEY),
                                   g_hash_table_lookup (attributes, FORM_PASSWORD_KEY),
                                   g_hash_table_lookup (attributes, USERNAME_KEY));
    */
    GtkTreeIter iter;
    gtk_list_store_append(priv->p_auth_data_list_store, &iter);
    const char* invisiable = "**********";
    gtk_list_store_set(priv->p_auth_data_list_store, &iter, 0, host, 
								1, g_hash_table_lookup (attributes, FORM_USERNAME_KEY), 
								2, g_hash_table_lookup (attributes, FORM_PASSWORD_KEY),
								3, password,
								4, g_hash_table_lookup (attributes, USERNAME_KEY), 
                                                                5, invisiable,-1 );

    g_free (host);
    g_hash_table_unref (attributes);
  }

  g_list_free_full (results, g_object_unref);
}


static void auth_data_list_init(PasswordManagerPrivate* priv)
{
  GHashTable *attributes;

  attributes = secret_attributes_build (EPHY_FORM_PASSWORD_SCHEMA, NULL);
  secret_service_search (NULL,
                         EPHY_FORM_PASSWORD_SCHEMA,
                         attributes,
                         SECRET_SEARCH_UNLOCK | SECRET_SEARCH_ALL,
                         NULL,
                         (GAsyncReadyCallback)screcet_service_search_finished,
                         priv);
  g_hash_table_unref (attributes);
}

static void password_manager_class_init(PasswordManagerClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  g_type_class_add_private(object_class, sizeof(PasswordManagerPrivate));
}

static void password_manager_init(PasswordManager* manager)
{
  manager->priv = G_TYPE_INSTANCE_GET_PRIVATE(manager, PASSWORD_MANAGER_TYPE, PasswordManagerPrivate);
  manager->priv->p_auth_data_list_store = gtk_list_store_new(6,G_TYPE_STRING, G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, G_TYPE_STRING);
  auth_data_list_init(manager->priv);
}

static gpointer
password_manager_create_instance(gpointer data)
{
  return g_object_new(PASSWORD_MANAGER_TYPE, NULL);
}

PasswordManager*
password_manager_get_default(void)
{
  static GOnce once_init = G_ONCE_INIT;
  return PASSWORD_MANAGER(g_once (&once_init, password_manager_create_instance, NULL));
}

static void
auth_data_deleted_cb(SecretService *service,
                    GAsyncResult *result,
                    gpointer data)
{
  secret_service_clear_finish (service, result, NULL);
  password_manager_display();
}

//add by luyue 2015/4/11 start
static void
auth_data_clear_cb(SecretService *service,
                   GAsyncResult *result,
                   gpointer data)
{
  secret_service_clear_finish (service, result, NULL);
}
//add end

static GHashTable *
midori_form_auth_data_get_secret_attributes_table (const char *uri,
                                                 const char *field_username,
                                                 const char *field_password,
                                                 const char *username)
{
  if (field_username)
    return secret_attributes_build (EPHY_FORM_PASSWORD_SCHEMA,
                                    URI_KEY, uri,
                                    FORM_USERNAME_KEY, field_username,
                                    FORM_PASSWORD_KEY, field_password,
                                    username ? USERNAME_KEY : NULL, username,
                                    NULL);
  else
    return secret_attributes_build (EPHY_FORM_PASSWORD_SCHEMA,
                                    URI_KEY, uri,
                                    FORM_PASSWORD_KEY, field_password,
                                    username ? USERNAME_KEY : NULL, username,
                                    NULL);
}

static void delete_secret_service_item (const char *uri,
					const char *form_username,
					const char *form_password,
					const char *username)
{
  SoupURI *key;
  char *key_str;

  g_return_if_fail(uri);
  g_return_if_fail(form_password);
  
  GHashTable *attributes;

  key = soup_uri_new (NULL);
  soup_uri_set_scheme(key, SOUP_URI_SCHEME_HTTP);
  char path[256];
  sprintf(path, "//%s/", uri);
  soup_uri_set_path(key,path);
  g_return_if_fail (key);
  key_str = soup_uri_to_string (key, FALSE);

  attributes = midori_form_auth_data_get_secret_attributes_table (key_str,
                                                                form_username,
                                                                form_password,
                                                                username);
  //attributes = secret_attributes_build (EPHY_FORM_PASSWORD_SCHEMA, NULL);
  secret_service_clear_sync (NULL,
			EPHY_FORM_PASSWORD_SCHEMA,
                        attributes,
                        NULL,
                        //(GAsyncReadyCallback)auth_data_deleted_cb,
                        NULL);

  /*MidoriApp* app = (MidoriApp*)midori_app_get_default();
  ephy_web_extension_proxy_form_auth_data_item_delete(app->web_extensions,
                                                      key_str,
                                                      form_username,
                                                      form_password,
                                                      username);*/
}

static void ask_for_show_password_cb(PasswordManagerPrivate* priv)
{
  PasswordManager* manager = password_manager_get_default();
  if(!manager->priv->p_bShowPassword){
    manager->priv->p_bShowPassword = true;
    gtk_tree_view_column_set_attributes(GTK_TREE_VIEW_COLUMN(password_view_column), window_renderer, "text", 3, NULL);
  }else{
    manager->priv->p_bShowPassword = false;
    gtk_tree_view_column_set_attributes(GTK_TREE_VIEW_COLUMN(password_view_column), window_renderer, "text", 5, NULL);
  }
  password_manager_display();
}

static void delete_auth_data_cb(PasswordManagerPrivate* priv)
{
  char* uri = NULL;
  char* form_username = NULL;
  char* form_password = NULL;
  char* username = NULL;
  GtkTreeModel* model;
  GtkTreeIter selected_item;
  GtkListStore* store;
  //mm = (GtkTreeModel*)gtk_tree_view_get_model((GtkTreeView*)(priv->p_item_list));
  //gtk_tree_view_get_model(priv->p_item_list);
  //store = (GtkListStore*)gtk_tree_view_get_model(priv->p_item_list);
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(p_item_list)));
  //store = GTK_LIST_STORE(mm);
  //GtkTreeSelection* selection = (GtkTreeSelection*)gtk_tree_view_get_selection((GtkTreeView*)(priv->p_item_list));
  GtkTreeSelection* selection = (GtkTreeSelection*)gtk_tree_view_get_selection((GtkTreeView*)(p_item_list));
  if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &model, &selected_item))
  {
    gtk_tree_model_get(model, &selected_item, 0, &uri,1, &form_username, 2, &form_password, 4, &username, -1);
    gtk_list_store_remove(store, &selected_item);
    delete_secret_service_item(uri, form_username, form_password, username);
  }
}

static void delete_all_auth_data_cb(PasswordManagerPrivate* priv)
{
  GHashTable *attributes;
  GtkListStore* store;

  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(p_item_list)));
  gtk_list_store_clear(store);
  attributes = secret_attributes_build (EPHY_FORM_PASSWORD_SCHEMA, NULL);
  secret_service_clear (NULL,
			EPHY_FORM_PASSWORD_SCHEMA,
                        attributes,
                        NULL,
                        (GAsyncReadyCallback)auth_data_deleted_cb,
                        NULL);
}

//add by luyue 2015/4/10
void clear_password_all()
{
  GHashTable *attributes;
  GtkListStore* store;

  PasswordManager* passwordManager = password_manager_get_default();
  p_item_list = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(passwordManager->priv->p_auth_data_list_store)));
  store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(p_item_list)));
  gtk_list_store_clear(store);
  attributes = secret_attributes_build (EPHY_FORM_PASSWORD_SCHEMA, NULL);
  secret_service_clear (NULL,
                        EPHY_FORM_PASSWORD_SCHEMA,
                        attributes,
                        NULL,
                        (GAsyncReadyCallback)auth_data_clear_cb,
                        NULL);
}
//add end

static int close_password_manager()
{
  PasswordManager* manager = password_manager_get_default();
  manager->priv->p_auth_data_manager_window = NULL;
  return false;
}

void password_manager_display(void)
{
  PasswordManager* passwordManager = password_manager_get_default();
  //GtkWindow* auth_data_manager_window = passwordManager->priv->p_auth_data_manager_window;
  if(passwordManager->priv->p_auth_data_manager_window != NULL){
    gtk_window_present(GTK_WINDOW(passwordManager->priv->p_auth_data_manager_window));
      return;
  }
  
  GtkGrid* grid = (GtkGrid*)gtk_grid_new();
  GtkTreeViewColumn* view_column;
  GtkCellRenderer* p_window_renderer;
  GtkVBox* vbox = (GtkVBox*)gtk_vbox_new(false, 1);
  GtkVBox* hbox = (GtkVBox*)gtk_hbox_new(false, 1);

  passwordManager->priv->p_auth_data_manager_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  passwordManager->priv->p_bShowPassword = false;
  const gchar* title = "用户密码管理器";
  gtk_window_set_title(passwordManager->priv->p_auth_data_manager_window, title);
  gtk_window_set_default_size(passwordManager->priv->p_auth_data_manager_window, 400, 300);
  p_item_list = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(passwordManager->priv->p_auth_data_list_store)));
  //passwordManager->priv->window_renderer=gtk_cell_renderer_text_new();
  window_renderer=gtk_cell_renderer_text_new();
  passwordManager->priv->hostname_view_column=gtk_tree_view_column_new_with_attributes("域  名",window_renderer,"text",0,NULL);
  gtk_tree_view_column_set_sizing(passwordManager->priv->hostname_view_column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(p_item_list), passwordManager->priv->hostname_view_column);
  gtk_tree_view_column_set_resizable(passwordManager->priv->hostname_view_column, true);

  passwordManager->priv->username_view_column=gtk_tree_view_column_new_with_attributes("用户名",window_renderer,"text",4,NULL);
  gtk_tree_view_column_set_sizing(passwordManager->priv->username_view_column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(p_item_list), passwordManager->priv->username_view_column);
  gtk_tree_view_column_set_resizable(passwordManager->priv->username_view_column, true);
  
  password_view_column=gtk_tree_view_column_new_with_attributes("密  码",window_renderer,"text",5,NULL);
  gtk_tree_view_column_set_sizing(password_view_column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(p_item_list), password_view_column);
  gtk_tree_view_column_set_resizable(password_view_column, true);

  gtk_box_pack_start ((GtkBox*)vbox,(GtkWidget*)p_item_list, true, true, (guint)0);
  gtk_tree_view_expand_all((GtkTreeView *)p_item_list);
  gtk_container_set_border_width (GTK_CONTAINER (p_item_list), 1);
  gtk_widget_show(GTK_WIDGET(p_item_list));
  
  GtkButton* button = GTK_BUTTON(gtk_button_new_with_label("删    除"));
  gtk_box_pack_start ((GtkBox*)hbox,(GtkWidget*)button, false, false, (guint)0);
  g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(delete_auth_data_cb), passwordManager->priv);
  
  button = GTK_BUTTON(gtk_button_new_with_label("删除全部"));
  gtk_box_pack_start ((GtkBox*)hbox,(GtkWidget*)button, false, false, (guint)0);
  g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(delete_all_auth_data_cb), passwordManager->priv);

/*  button = GTK_BUTTON(gtk_button_new_with_label("显示/隐藏"));
  gtk_box_pack_start ((GtkBox*)hbox,(GtkWidget*)button, false, false, (guint)0);
  g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(ask_for_show_password_cb), passwordManager->priv);*/
 
/*  button = GTK_BUTTON(gtk_button_new_with_label("关    闭"));
  gtk_box_pack_end ((GtkBox*)hbox,(GtkWidget*)button, false, false, (guint)0);
//  g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(gtk_window_close), passwordManager->priv->p_auth_data_manager_window);
  g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(close_password_manager), NULL);*/

  gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);

  gtk_box_pack_start ((GtkBox*)vbox,(GtkWidget*)hbox, false, false, (guint)0);
  gtk_container_add (GTK_CONTAINER (passwordManager->priv->p_auth_data_manager_window),GTK_WIDGET(vbox));

  gtk_container_set_border_width (GTK_CONTAINER (passwordManager->priv->p_auth_data_manager_window), 0);
  gtk_widget_show_all(GTK_WIDGET(passwordManager->priv->p_auth_data_manager_window));
  g_signal_connect(G_OBJECT(passwordManager->priv->p_auth_data_manager_window),"delete_event",G_CALLBACK(close_password_manager),NULL);
  
  gtk_window_present(GTK_WINDOW(passwordManager->priv->p_auth_data_manager_window));
}
