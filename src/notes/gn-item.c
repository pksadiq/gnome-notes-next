/* gn-item.c
 *
 * Copyright 2018 Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "gn-item"

#include "config.h"

#include "gn-item.h"
#include "gn-trace.h"

/**
 * SECTION: gn-item
 * @title: GnItem
 * @short_description: The base class for Notes and Notebooks
 *
 * #GnItem as such is not very useful, but used for derived classes
 * like #GnNote and #GnNotebook
 */

typedef struct
{
  gchar *uid;
  gchar *title;

  GdkRGBA *rgba;

  /* The last modified time of the item */
  gint64 modification_time;

  /* The creation time of the item */
  gint64 creation_time;
  gboolean modified;
} GnItemPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GnItem, gn_item, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_UID,
  PROP_TITLE,
  PROP_RGBA,
  PROP_CREATION_TIME,
  PROP_MODIFICATION_TIME,
  PROP_MODIFIED,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
gn_item_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  GnItem *self = (GnItem *)object;
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_UID:
      g_value_set_string (value, priv->uid);
      break;

    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_RGBA:
      g_value_set_boxed (value, priv->rgba);
      break;

    case PROP_CREATION_TIME:
      g_value_set_int64 (value, priv->creation_time);
      break;

    case PROP_MODIFICATION_TIME:
      g_value_set_int64 (value, priv->modification_time);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_item_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  GnItem *self = (GnItem *)object;
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_UID:
      gn_item_set_uid (self, g_value_get_string (value));
      break;

    case PROP_TITLE:
      gn_item_set_title (self, g_value_get_string (value));
      break;

    case PROP_RGBA:
      gn_item_set_rgba (self, g_value_get_boxed (value));
      break;

    case PROP_CREATION_TIME:
      priv->creation_time = g_value_get_int64 (value);
      break;

    case PROP_MODIFICATION_TIME:
      priv->modification_time = g_value_get_int64 (value);
      break;

    case PROP_MODIFIED:
      priv->modified = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_item_finalize (GObject *object)
{
  GnItem *self = (GnItem *)object;
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  GN_ENTRY;

  g_clear_pointer (&priv->uid, g_free);
  g_clear_pointer (&priv->title, g_free);

  G_OBJECT_CLASS (gn_item_parent_class)->finalize (object);

  GN_EXIT;
}

static void
gn_item_set_modified (GnItem *self)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_assert (GN_IS_ITEM (self));

  /*
   * Irrespective of the previous state, we always need to notify
   * ::modified.
   */
  priv->modified = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODIFIED]);
}

static gboolean
gn_item_real_is_modified (GnItem *self)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_assert (GN_IS_ITEM (self));

  return priv->modified;
}

static void
gn_item_real_unset_modified (GnItem *self)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_assert (GN_IS_ITEM (self));

  if (priv->modified == FALSE)
    return;

  priv->modified = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODIFIED]);
}

static gboolean
gn_item_real_match (GnItem      *self,
                    const gchar *needle)
{
  g_autofree gchar *title = NULL;

  g_assert (GN_IS_ITEM (self));

  title = g_utf8_casefold (gn_item_get_title (self), -1);
  if (strstr (title, needle) != NULL)
    return TRUE;

  return FALSE;
}

static GnFeature
gn_item_real_get_features (GnItem *self)
{
  g_assert (GN_IS_ITEM (self));

  return GN_FEATURE_NONE;
}

static void
gn_item_class_init (GnItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gn_item_get_property;
  object_class->set_property = gn_item_set_property;
  object_class->finalize = gn_item_finalize;

  klass->is_modified = gn_item_real_is_modified;
  klass->unset_modified = gn_item_real_unset_modified;
  klass->match = gn_item_real_match;
  klass->get_features = gn_item_real_get_features;

  properties[PROP_UID] =
    g_param_spec_string ("uid",
                         "UID",
                         "A Unique Identifier of the item",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * GnItem:title:
   *
   * The name of the Item. May be %NULL if title is empty.
   */
  properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The name of the Item",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);


  /**
   * GnItem:rgba:
   *
   * The #GdkRGBA color of the item. Set to %NULL if the item doesn't
   * support color.
   *
   */
  properties[PROP_RGBA] =
    g_param_spec_boxed ("rgba",
                        "RGBA Color",
                        "The RGBA color of the item",
                        GDK_TYPE_RGBA,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * GnItem:modification-time:
   *
   * The date and time (in seconds since UNIX epoch) the item was last
   * modified. A value of 0 denotes that the item doesn't support this
   * feature, or the note is item is not yet saved. To check if the
   * item is saved use gn_item_is_new().
   */
  properties[PROP_MODIFICATION_TIME] =
    g_param_spec_int64 ("modification-time",
                        "Modification Time",
                        "The Unix time in seconds the item was last modified",
                        0, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * GnItem:creation-time:
   *
   * The date and time (in seconds since UNIX epoch) the item was
   * created. A value of 0 denotes that the item doesn't support this
   * feature.
   *
   * The creation time can be the time the file was created (if the
   * backend is a file), or the time at which the data was saved to
   * database, etc.
   */
  properties[PROP_CREATION_TIME] =
    g_param_spec_int64 ("creation-time",
                        "Creation Time",
                        "The Unix time in seconds the item was created",
                        0, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * GnItem:modified:
   *
   * The modification state of the item. %TRUE if the item
   * has modified since last save.
   *
   * As derived classes can have more properties than this base class,
   * the implementation should do gn_item_unset_modified() after the
   * item have saved. Also on save, uid should be set.
   */
  properties[PROP_MODIFIED] =
    g_param_spec_boolean ("modified",
                          "Modified",
                          "The modified state of the item",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gn_item_init (GnItem *self)
{
}

/**
 * gn_item_get_uid:
 * @self: a #GnItem
 *
 * Get the unique id of the item.
 *
 * Returns: (transfer none) (nullable): the uid of the item
 */
const gchar *
gn_item_get_uid (GnItem *self)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_return_val_if_fail (GN_IS_ITEM (self), NULL);

  return priv->uid;
}

/**
 * gn_item_set_uid:
 * @self: a #GnItem
 * @uid: (nullable): a text to set as uid
 *
 * Set A unique identifier of the item. This can be a URL, URN,
 * Primary key of a database table or any kind of unique string
 * that can be used as an identifier.
 *
 * the uid of a saved item (ie, saved to file, goa, etc.) should
 * not be %NULL.
 */
void
gn_item_set_uid (GnItem      *self,
                 const gchar *uid)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_return_if_fail (GN_IS_ITEM (self));

  if (g_strcmp0 (priv->uid, uid) == 0)
    return;

  g_free (priv->uid);
  priv->uid = g_strdup (uid);

  gn_item_set_modified (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_UID]);
}

/**
 * gn_item_get_title:
 * @self: a #GnItem
 *
 * Get the title/name of the item
 *
 * Returns: (transfer none): the title of the item.
 */
const gchar *
gn_item_get_title (GnItem *self)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_return_val_if_fail (GN_IS_ITEM (self), NULL);

  return priv->title ? priv->title : "";
}

/**
 * gn_item_set_title:
 * @self: a #GnItem
 * @title: (nullable): a text to set as title
 *
 * Set the title of the item. Can be %NULL.
 */
void
gn_item_set_title (GnItem      *self,
                   const gchar *title)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  GN_ENTRY;

  g_return_if_fail (GN_IS_ITEM (self));

  /* If the new and old titles are same, don't bother changing */
  if (g_strcmp0 (priv->title, title) == 0)
    return;

  g_free (priv->title);
  priv->title = g_strdup (title);

  gn_item_set_modified (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);

  GN_EXIT;
}

/**
 * gn_item_get_rgba:
 * @self: a #GnItem
 * @rgba (out): a location for #GdkRGBA
 *
 * Get the #GdkRGBA of the item
 *
 * Returns: %TRUE if the item has a color. %FALSE otherwise
 */
gboolean
gn_item_get_rgba (GnItem  *self,
                  GdkRGBA *rgba)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_return_val_if_fail (GN_IS_ITEM (self), FALSE);
  g_return_val_if_fail (rgba != NULL, FALSE);

  if (priv->rgba == NULL)
    return FALSE;

  *rgba = *priv->rgba;

  return TRUE;
}

/**
 * gn_item_set_rgba:
 * @self: a #GnItem
 * @rgba: a #GdkRGBA
 *
 * Set the #GdkRGBA of the item
 */
void
gn_item_set_rgba (GnItem        *self,
                  const GdkRGBA *rgba)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_return_if_fail (GN_IS_ITEM (self));
  g_return_if_fail (rgba != NULL);

  if (priv->rgba != NULL &&
      gdk_rgba_equal (priv->rgba, rgba))
    return;

  gdk_rgba_free (priv->rgba);
  priv->rgba = gdk_rgba_copy (rgba);

  gn_item_set_modified (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_RGBA]);
}

/**
 * gn_item_get_creation_time:
 * @self: a #GnItem
 *
 * Get the creation time of the item. The time returned is the
 * seconds since UNIX epoch.
 *
 * A value of zero means the item isn't yet saved or the item
 * doesn't support creation-time property. gn_item_is_new()
 * can be used to check if item is saved.
 *
 * Returns: a signed integer representing creation time
 */
gint64
gn_item_get_creation_time (GnItem *self)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_return_val_if_fail (GN_IS_ITEM (self), 0);

  return priv->creation_time;
}

/**
 * gn_item_get_modification_time:
 * @self: a #GnItem
 *
 * Get the modification time of the item. The time returned is
 * the seconds since UNIX epoch.
 *
 * A value of zero means the item isn't yet saved or the item
 * doesn't support modification-time property. gn_item_is_new()
 * can be used to check if item is saved.
 *
 * Returns: a signed integer representing modification time
 */
gint64
gn_item_get_modification_time (GnItem *self)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_return_val_if_fail (GN_IS_ITEM (self), 0);

  return priv->modification_time;
}

/**
 * gn_item_is_modified:
 * @self: a #GnItem
 *
 * Get if the item is modified
 *
 * Returns: %TRUE if the item has been modified since
 * last save. %FALSE otherwise.
 */
gboolean
gn_item_is_modified (GnItem *self)
{
  gboolean ret;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_ITEM (self), FALSE);

  ret = GN_ITEM_GET_CLASS (self)->is_modified (self);

  GN_RETURN (ret);
}

/**
 * gn_item_unset_modified:
 * @self: a #GnItem
 *
 * Unmark the item as modified.
 */
void
gn_item_unset_modified (GnItem *self)
{
  GN_ENTRY;

  g_return_if_fail (GN_IS_ITEM (self));

  GN_ITEM_GET_CLASS (self)->unset_modified (self);

  GN_EXIT;
}

/**
 * gn_item_is_new:
 * @self: a #GnItem
 *
 * Get if the item is new (not yet saved).
 *
 * Returns: a boolean
 */
gboolean
gn_item_is_new (GnItem *self)
{
  GnItemPrivate *priv = gn_item_get_instance_private (self);

  g_return_val_if_fail (GN_IS_ITEM (self), FALSE);

  /* If uid is NULL, that means the item isn't saved */
  return priv->uid == NULL;
}

/**
 * gn_item_compare:
 * @a: A #GnItem
 * @b: A #GnItem
 *
 * Compare two #GnItem
 *
 * Returns: an integer less than, equal to, or greater
 * than 0, if title of @a is <, == or > than title of @b.
 */
gint
gn_item_compare (gconstpointer a,
                 gconstpointer b,
                 gpointer      user_data)
{
  GnItemPrivate *item_a = gn_item_get_instance_private ((GnItem *)a);
  GnItemPrivate *item_b = gn_item_get_instance_private ((GnItem *)b);
  g_autofree gchar *title_a = NULL;
  g_autofree gchar *title_b = NULL;

  if (item_a == item_b)
    return 0;

  title_a = g_utf8_casefold (item_a->title, -1);
  title_b = g_utf8_casefold (item_b->title, -1);

  return g_strcmp0 (title_a, title_b);
}

gboolean
gn_item_match (GnItem      *self,
               const gchar *needle)
{
  gboolean ret;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_ITEM (self), FALSE);
  g_return_val_if_fail (needle != NULL, FALSE);

  ret = GN_ITEM_GET_CLASS (self)->match (self, needle);

  GN_RETURN (ret);
}

/**
 * gn_item_get_features:
 * @self: a #GnItem
 *
 * Get the features supported by @self.
 *
 * Returns: A #GnFeature
 */
GnFeature
gn_item_get_features (GnItem *self)
{
  GnFeature features;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_ITEM (self), 0);

  features = GN_ITEM_GET_CLASS (self)->get_features (self);

  GN_RETURN (features);
}
