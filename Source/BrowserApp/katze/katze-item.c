/*

*/
 
#include "katze-item.h"
#include "katze-utils.h"
#include "midori/cdosbrowser-core.h"

#include <glib/gi18n.h>

/**
 * SECTION:katze-item
 * @short_description: A useful item
 * @see_also: #KatzeArray
 *
 * #KatzeItem is a particularly useful item that provides
 * several commonly needed properties.
 */

G_DEFINE_TYPE (KatzeItem, katze_item, G_TYPE_OBJECT);

enum
{
    PROP_0,

    PROP_NAME,
    PROP_TEXT,
    PROP_URI,
    PROP_ICON,
    PROP_TOKEN,
    PROP_ADDED,
    PROP_PARENT
};

enum {
    META_DATA_CHANGED,

    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
katze_item_finalize (GObject* object);

static void
katze_item_set_property (GObject*      object,
                         guint         prop_id,
                         const GValue* value,
                         GParamSpec*   pspec);

static void
katze_item_get_property (GObject*    object,
                         guint       prop_id,
                         GValue*     value,
                         GParamSpec* pspec);

static void
katze_item_class_init (KatzeItemClass* class)
{
    GObjectClass* gobject_class;
    GParamFlags flags;

    /**
     * KatzeItem::meta-data-changed:
     * @item: the object on which the signal is emitted
     * @key: the key that changed
     *
     * Emitted when a meta data value was changed.
     *
     * Since 0.2.2 details according to keys are supported.
     *
     * Since: 0.1.9
     */
    signals[META_DATA_CHANGED] = g_signal_new (
        "meta-data-changed",
        G_TYPE_FROM_CLASS (class),
        (GSignalFlags)(G_SIGNAL_RUN_LAST |G_SIGNAL_DETAILED),
        0,
        0,
        NULL,
        g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1,
        G_TYPE_STRING);

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = katze_item_finalize;
    gobject_class->set_property = katze_item_set_property;
    gobject_class->get_property = katze_item_get_property;

    flags = G_PARAM_READWRITE | G_PARAM_CONSTRUCT;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string (
                                     "name",
                                     "Name",
                                     "The name of the item",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_TEXT,
                                     g_param_spec_string (
                                     "text",
                                     "Text",
                                     "The descriptive text of the item",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_URI,
                                     g_param_spec_string (
                                     "uri",
                                     "URI",
                                     "The URI of the item",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_ICON,
                                     g_param_spec_string (
                                     "icon",
                                     "Icon",
                                     "The icon of the item",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_TOKEN,
                                     g_param_spec_string (
                                     "token",
                                     "Token",
                                     "The token of the item",
                                     NULL,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_ADDED,
                                     g_param_spec_int64 (
                                     "added",
                                     "Added",
                                     "When the item was added",
                                     G_MININT64,
                                     G_MAXINT64,
                                     0,
                                     flags));

    /**
    * KatzeItem:parent:
    *
    * The parent of the item.
    *
    * Since: 0.1.2
    */
    g_object_class_install_property (gobject_class,
                                     PROP_PARENT,
                                     g_param_spec_object (
                                     "parent",
                                     "Parent",
                                     "The parent of the item",
                                     G_TYPE_OBJECT,
                                     flags));

    class->copy = NULL;
}



static void
katze_item_init (KatzeItem* item)
{
    item->metadata = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            g_free, g_free);
}

static void
katze_item_finalize (GObject* object)
{
    KatzeItem* item = KATZE_ITEM (object);

    g_free (item->name);
    g_free (item->text);
    g_free (item->uri);
    g_free (item->token);

    g_hash_table_unref (item->metadata);

    G_OBJECT_CLASS (katze_item_parent_class)->finalize (object);
}

static void
katze_item_set_property (GObject*      object,
                         guint         prop_id,
                         const GValue* value,
                         GParamSpec*   pspec)
{
    KatzeItem* item = KATZE_ITEM (object);

    switch (prop_id)
    {
    case PROP_NAME:
        katze_assign (item->name, g_value_dup_string (value));
        break;
    case PROP_TEXT:
        katze_assign (item->text, g_value_dup_string (value));
        break;
    case PROP_URI:
        katze_assign (item->uri, g_value_dup_string (value));
        break;
    case PROP_ICON:
        katze_item_set_icon (item, g_value_get_string (value));
        break;
    case PROP_TOKEN:
        katze_assign (item->token, g_value_dup_string (value));
        break;
    case PROP_ADDED:
        item->added = g_value_get_int64 (value);
        break;
    case PROP_PARENT:
        katze_item_set_parent (item, g_value_get_object (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
katze_item_get_property (GObject*    object,
                         guint       prop_id,
                         GValue*     value,
                         GParamSpec* pspec)
{
    KatzeItem* item = KATZE_ITEM (object);

    switch (prop_id)
    {
    case PROP_NAME:
        g_value_set_string (value, item->name);
        break;
    case PROP_TEXT:
        g_value_set_string (value, item->text);
        break;
    case PROP_URI:
        g_value_set_string (value, item->uri);
        break;
    case PROP_ICON:
        g_value_set_string (value, katze_item_get_icon (item));
        break;
    case PROP_TOKEN:
        g_value_set_string (value, item->token);
        break;
    case PROP_ADDED:
        g_value_set_int64 (value, item->added);
        break;
    case PROP_PARENT:
        g_value_set_object (value, item->parent);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/**
 * katze_item_new:
 *
 * Creates a new #KatzeItem.
 *
 * Return value: (transfer full): a new #KatzeItem
 **/
KatzeItem*
katze_item_new (void)
{
    KatzeItem* item = g_object_new (KATZE_TYPE_ITEM, NULL);

    return item;
}

/**
 * katze_item_get_name:
 * @item: a #KatzeItem
 *
 * Retrieves the name of @item.
 *
 * Return value: the name of the item
 **/
const gchar*
katze_item_get_name (KatzeItem* item)
{
  g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

  if (item->name != NULL && strlen(item->name)>128)   //make sure name not too long
  {
    unsigned char *p;
    p = item->name;
    int size_num = 0;
    int size_longth = 0;
    while (size_num <=84)
    {
      if(*p>=0xA1)
      {
        size_num = size_num + 2;
        size_longth+=3;
	p+=3;
      }
      else
      {
        size_num = size_num + 1;
        size_longth+=1;
	p+=1;
      }
    }
    gchar* tmp_name=g_malloc(size_longth+1);
    memset(tmp_name,0,size_longth+1);
    strncpy(tmp_name,item->name,size_longth);
    katze_item_set_name(item,tmp_name);
    g_free(tmp_name);                                       //change end
  }
  return item->name;
}

/**
 * katze_item_set_name:
 * @item: a #KatzeItem
 * @name: a string
 *
 * Sets the name of @item.
 **/
void
katze_item_set_name (KatzeItem*   item,
                     const gchar* name)
{
    g_return_if_fail (KATZE_IS_ITEM (item));

    if (!g_strcmp0 (item->name, name))
        return;

    katze_assign (item->name, g_strdup (name));
    //if (item->parent)                                           //delete by liuyibiao 20150928
    //    katze_array_update ((KatzeArray*)item->parent);
    g_object_notify (G_OBJECT (item), "name");
}

/**
 * katze_item_get_text:
 * @item: a #KatzeItem
 *
 * Retrieves the descriptive text of @item.
 *
 * Return value: the text of the item
 **/
const gchar*
katze_item_get_text (KatzeItem* item)
{
    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    return item->text;
}

/**
 * katze_item_set_text:
 * @item: a #KatzeItem
 * @text: a string
 *
 * Sets the descriptive text of @item.
 **/
void
katze_item_set_text (KatzeItem*   item,
                     const gchar* text)
{
    g_return_if_fail (KATZE_IS_ITEM (item));

    katze_assign (item->text, g_strdup (text));
    g_object_notify (G_OBJECT (item), "text");
}

/**
 * katze_item_get_uri:
 * @item: a #KatzeItem
 *
 * Retrieves the URI of @item.
 *
 * Return value: the URI of the item
 **/
const gchar*
katze_item_get_uri (KatzeItem* item)
{
    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    return item->uri;
}

/**
 * katze_item_set_uri:
 * @item: a #KatzeItem
 * @uri: a string
 *
 * Sets the URI of @item.
 **/
void
katze_item_set_uri (KatzeItem*   item,
                    const gchar* uri)
{
    g_return_if_fail (KATZE_IS_ITEM (item));

    if (!g_strcmp0 (item->uri, uri))
        return;

    katze_assign (item->uri, g_strdup (uri));
    g_object_notify (G_OBJECT (item), "uri");
}

/**
 * katze_item_get_icon:
 * @item: a #KatzeItem
 *
 * Retrieves the icon of @item.
 *
 * Return value: the icon of the item
 **/
const gchar*
katze_item_get_icon (KatzeItem* item)
{
    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    return katze_item_get_meta_string (item, "icon");
}

/**
 * katze_item_set_icon:
 * @item: a #KatzeItem
 * @icon: a string
 *
 * Sets the icon of @item.
 **/
void
katze_item_set_icon (KatzeItem*   item,
                     const gchar* icon)
{
    g_return_if_fail (KATZE_IS_ITEM (item));

    if (!g_strcmp0 (katze_item_get_meta_string (item, "icon"), icon))
        return;

    katze_item_set_meta_string (item, "icon", icon);
    if (item->parent)
        katze_array_update ((KatzeArray*)item->parent);
    g_object_notify (G_OBJECT (item), "icon");
}

/**
 * katze_item_get_pixbuf:
 * @item: a #KatzeItem
 * @widget: a #GtkWidget, or %NULL
 *
 * Creates a #GdkPixbuf fit to display @item.
 *
 * Return value: (transfer full): the icon of the item, or %NULL
 *
 * Since: 0.4.6
 **/
GdkPixbuf*
katze_item_get_pixbuf (KatzeItem* item,
                       GtkWidget*  widget)
{
    GdkPixbuf* pixbuf;

    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    if (widget && KATZE_ITEM_IS_FOLDER (item))
        return gtk_widget_render_icon (widget, GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_MENU, NULL);
    if ((pixbuf = midori_paths_get_icon (item->uri, widget)))
        return pixbuf;
    return NULL;
}

static void
katze_item_image_destroyed_cb (GtkWidget* image,
                               KatzeItem* item);
#ifndef HAVE_WEBKIT2
static void
katze_item_icon_loaded_cb (WebKitFaviconDatabase* database,
                           const gchar*           frame_uri,
                           GtkWidget*             image)
{
    KatzeItem* item = g_object_get_data (G_OBJECT (image), "KatzeItem");
    GdkPixbuf* pixbuf;
    if (!g_strcmp0 (frame_uri, item->uri)
      && (pixbuf = midori_paths_get_icon (frame_uri, image)))
    {
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
        g_object_unref (pixbuf);
        /* This signal fires extremely often (WebKit bug?)
           we must throttle it (disconnect) once we have an icon */
        katze_item_image_destroyed_cb (image, g_object_ref (item));
    }
}
#endif

static void
katze_item_image_destroyed_cb (GtkWidget* image,
                               KatzeItem* item)
{
#ifndef HAVE_WEBKIT2
    g_signal_handlers_disconnect_by_func (webkit_get_favicon_database (),
        katze_item_icon_loaded_cb, image);
#endif
    g_object_unref (item);
}

/**
 * katze_item_get_image:
 * @item: a #KatzeItem
 * @widget: a #GtkWidget, or %NULL
 *
 * Creates a #GtkImage fit to display @item.
 *
 * Return value: (transfer floating): the icon of the item
 *
 * Since: 0.4.4
 * Since 0.4.8 a @widget was added and the image is set visible.
 **/
GtkWidget*
katze_item_get_image (KatzeItem* item,
                      GtkWidget* widget)
{
    GtkWidget* image;
    GdkPixbuf* pixbuf;

    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    pixbuf = katze_item_get_pixbuf (item, widget);
    image = gtk_image_new_from_pixbuf (pixbuf);
    gtk_widget_show (image);
    if (pixbuf != NULL)
        g_object_unref (pixbuf);
    if (KATZE_ITEM_IS_FOLDER (item))
        return image;
    g_object_set_data (G_OBJECT (image), "KatzeItem", g_object_ref (item));
    g_signal_connect (image, "destroy",
        G_CALLBACK (katze_item_image_destroyed_cb), item);
#ifndef HAVE_WEBKIT2
    g_signal_connect (webkit_get_favicon_database (), "icon-loaded",
        G_CALLBACK (katze_item_icon_loaded_cb), image);
#endif
    return image;
}

/**
 * katze_item_get_token:
 * @item: a #KatzeItem
 *
 * Retrieves the token of @item.
 *
 * Return value: the token of the item
 **/
const gchar*
katze_item_get_token (KatzeItem* item)
{
    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    return item->token;
}

/**
 * katze_item_set_token:
 * @item: a #KatzeItem
 * @token: a string
 *
 * Sets the token of @item.
 **/
void
katze_item_set_token (KatzeItem*   item,
                      const gchar* token)
{
    g_return_if_fail (KATZE_IS_ITEM (item));

    katze_assign (item->token, g_strdup (token));
    g_object_notify (G_OBJECT (item), "token");
}

/**
 * katze_item_get_added:
 * @item: a #KatzeItem
 *
 * Determines when @item was added.
 *
 * Return value: a timestamp
 **/
gint64
katze_item_get_added (KatzeItem* item)
{
    g_return_val_if_fail (KATZE_IS_ITEM (item), 0);

    return item->added;
}

/**
 * katze_item_set_added:
 * @item: a #KatzeItem
 * @added: a timestamp
 *
 * Sets when @item was added.
 **/
void
katze_item_set_added (KatzeItem* item,
                      gint64     added)
{
    g_return_if_fail (KATZE_IS_ITEM (item));

    item->added = added;
    g_object_notify (G_OBJECT (item), "added");
}

/**
 * katze_item_get_meta_keys:
 * @item: a #KatzeItem
 *
 * Retrieves a list of all meta keys.
 *
 * Return value: (element-type utf8) (transfer container): a newly allocated #GList of constant strings
 *
 * Since: 0.1.8
 **/
GList*
katze_item_get_meta_keys (KatzeItem* item)
{
    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    return g_hash_table_get_keys (item->metadata);
}

static void
katze_item_set_meta_data_value (KatzeItem*   item,
                                const gchar* key,
                                gchar*       value)
{
    /* FIXME: Make the default namespace configurable */
    if (g_str_has_prefix (key, "midori:"))
        g_hash_table_insert (item->metadata, g_strdup (&key[7]), value);
    else
        g_hash_table_insert (item->metadata, g_strdup (key), value);
    g_signal_emit (item, signals[META_DATA_CHANGED], g_quark_from_string (key), key);
}

/**
 * katze_item_get_meta_string:
 * @item: a #KatzeItem
 * @key: the name of an integer value
 *
 * Retrieves a string value by the specified key from the
 * meta data of the item.
 *
 * Specify "namespace:key" or "key" to use the default namespace.
 *
 * Return value: a string, or %NULL
 *
 * Since: 0.1.8
 *
 * Since 0.4.4 "" is treated like %NULL.
 **/
const gchar*
katze_item_get_meta_string (KatzeItem*   item,
                            const gchar* key)
{
    const gchar* value;

    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);
    g_return_val_if_fail (key != NULL, NULL);

    if (g_str_has_prefix (key, "midori:"))
        key = &key[7];
    value = g_hash_table_lookup (item->metadata, key);
    return value && *value ? value : NULL;
}

/**
 * katze_item_set_meta_string:
 * @item: a #KatzeItem
 * @key: the name of a string value
 * @value: the value as a string
 *
 * Saves the specified string value in the meta data of
 * the item under the specified key.
 *
 * Specify "namespace:key" or "key" to use the default namespace.
 *
 * Since: 0.1.8
 **/
void
katze_item_set_meta_string (KatzeItem*   item,
                            const gchar* key,
                            const gchar* value)
{
    g_return_if_fail (KATZE_IS_ITEM (item));
    g_return_if_fail (key != NULL);

    katze_item_set_meta_data_value (item, key, g_strdup (value));
}

/**
 * katze_item_get_meta_integer:
 * @item: a #KatzeItem
 * @key: the name of an integer value
 *
 * Retrieves an integer value by the specified key from the
 * meta data of the item.
 *
 * If the key is present but not representable as an
 * integer, -1 is returned.
 *
 * Return value: an integer value, or -1
 *
 * Since: 0.1.8
 **/
gint64
katze_item_get_meta_integer (KatzeItem*   item,
                             const gchar* key)
{
    gpointer value;

    g_return_val_if_fail (KATZE_IS_ITEM (item), -1);
    g_return_val_if_fail (key != NULL, -1);

    if (g_str_has_prefix (key, "midori:"))
        key = &key[7];
    if (g_hash_table_lookup_extended (item->metadata, key, NULL, &value))
        return value ? g_ascii_strtoll (value, NULL, 0) : -1;
    return -1;
}

/**
 * katze_item_get_meta_boolean:
 * @item: a #KatzeItem
 * @key: the name of a boolean value
 *
 * The Value should be set with katze_item_set_meta_integer().
 * If the value is set and not 0, %TRUE will be returned.
 *
 * Since: 0.2.7
 **/
gboolean
katze_item_get_meta_boolean  (KatzeItem*   item,
                              const gchar* key)
{
    const gchar* value;

    g_return_val_if_fail (KATZE_IS_ITEM (item), FALSE);
    g_return_val_if_fail (key != NULL, FALSE);

    value = katze_item_get_meta_string (item, key);
    if (value == NULL || value[0] == '0')
        return FALSE;
    else
        return TRUE;
}

/**
 * katze_item_set_meta_integer:
 * @item: a #KatzeItem
 * @key: the name of an integer value
 *
 * Saves the specified integer value in the meta data of
 * the item under the specified key.
 *
 * A value of -1 is intepreted as unset.
 *
 * Since: 0.1.8
 **/
void
katze_item_set_meta_integer (KatzeItem*   item,
                             const gchar* key,
                             gint64       value)
{
    g_return_if_fail (KATZE_IS_ITEM (item));
    g_return_if_fail (key != NULL);

    if (value == -1)
        katze_item_set_meta_data_value (item, key, NULL);
    else
    {
        katze_item_set_meta_data_value (item, key,
        #ifdef G_GINT64_FORMAT
            g_strdup_printf ("%" G_GINT64_FORMAT, value));
        #else
            g_strdup_printf ("%li", value));
        #endif
    }
}

/**
 * katze_item_get_parent:
 * @item: a #KatzeItem
 *
 * Determines the parent of @item.
 *
 * Since 0.1.2 you can monitor the "parent" property.
 *
 * Return value: (type GObject) (transfer none): the parent of the item
 **/
gpointer
katze_item_get_parent (KatzeItem* item)
{
    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    return item->parent;
}

/**
 * katze_item_set_parent:
 * @item: a #KatzeItem
 * @parent: the new parent
 *
 * Sets the parent of @item.
 *
 * This is intended for item container implementations. Notably
 * the new parent will not be notified of the change.
 *
 * Since 0.1.2 you can monitor the "parent" property, so unsetting
 * the parent is actually safe if the parent supports it.
 **/
void
katze_item_set_parent (KatzeItem* item,
                       gpointer   parent)
{
    g_return_if_fail (KATZE_IS_ITEM (item));
    g_return_if_fail (!parent || G_IS_OBJECT (parent));

    if (parent)
        g_object_ref (parent);
    katze_object_assign (item->parent, parent);
    g_object_notify (G_OBJECT (item), "parent");
}

/**
 * katze_item_copy:
 * @item: a #KatzeItem
 *
 * Creates an exact copy of @item.
 *
 * Note that subclass specific features will only
 * be preserved if the class implements it.
 *
 * Since 0.4.3 meta data is copied.
 *
 * Return value: (transfer full): a new #KatzeItem
 *
 * Since: 0.1.3
 **/
KatzeItem*
katze_item_copy (KatzeItem* item)
{
    KatzeItem* copy;
    GHashTableIter iter;
    const gchar* key;
    const gchar* value;
    KatzeItemClass* class;

    g_return_val_if_fail (KATZE_IS_ITEM (item), NULL);

    copy = g_object_new (G_OBJECT_TYPE (item),
        "name", item->name,
        "text", item->text,
        "uri", item->uri,
        "token", item->token,
        "added", item->added,
        "parent", item->parent,
        NULL);

    g_hash_table_iter_init (&iter, item->metadata);
    while (g_hash_table_iter_next (&iter, (void*)&key, (void*)&value))
    {
        if (g_str_has_prefix (key, "midori:"))
            key = &key[7];
        g_hash_table_insert (copy->metadata, g_strdup (key), g_strdup (value));
    }

    class = KATZE_ITEM_GET_CLASS (item);
    return class->copy ? class->copy (copy) : copy;
}
const gint32
katze_item_get_kind (KatzeItem* item)
{
    g_return_val_if_fail (KATZE_IS_ITEM (item), 0);

    return item->kind;
}


void
katze_item_set_kind (KatzeItem* item,
                      gint32     kind)
{
    g_return_if_fail (KATZE_IS_ITEM (item));

    item->kind = kind;
}
