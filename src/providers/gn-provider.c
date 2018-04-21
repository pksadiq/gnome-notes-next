/* gn-provider.c
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

#define G_LOG_DOMAIN "gn-provider"

#include "config.h"

#include "gn-provider.h"
#include "gn-provider-item.h"
#include "gn-trace.h"

/**
 * SECTION: gn-provider
 * @title: GnProvider
 * @short_description: Base class for providers
 * @include: "gn-provider.h"
 */

typedef struct
{
  gboolean loaded;
} GnProviderPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GnProvider, gn_provider, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_UID,
  PROP_NAME,
  PROP_ICON,
  PROP_DOMAIN,
  PROP_USER_NAME,
  N_PROPS
};

enum {
  ITEM_ADDED,
  ITEM_DELETED,
  ITEM_TRASHED,
  ITEM_RESTORED,
  ITEM_UPDATED,
  N_SIGNALS
};

static GParamSpec *properties[N_PROPS];
static guint signals[N_SIGNALS];

static void
gn_provider_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GnProvider *self = (GnProvider *)object;

  switch (prop_id)
    {
    case PROP_UID:
      g_value_set_string (value, gn_provider_get_uid (self));
      break;

    case PROP_NAME:
      g_value_set_string (value, gn_provider_get_name (self));
      break;

    case PROP_ICON:
      g_value_set_string (value, gn_provider_get_icon (self));
      break;

    case PROP_DOMAIN:
      g_value_set_string (value, gn_provider_get_domain (self));
      break;

    case PROP_USER_NAME:
      g_value_set_string (value, gn_provider_get_user_name (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_provider_finalize (GObject *object)
{
  GN_ENTRY;

  G_OBJECT_CLASS (gn_provider_parent_class)->finalize (object);

  GN_EXIT;
}

static GList *
gn_provider_real_get_notes (GnProvider *self)
{
  g_assert (GN_IS_PROVIDER (self));

  /* Derived classes should implement this, if supported */
  return NULL;
}

static GList *
gn_provider_real_get_trash_notes (GnProvider *self)
{
  g_assert (GN_IS_PROVIDER (self));

  /* Derived classes should implement this, if supported */
  return NULL;
}

static GList *
gn_provider_real_get_notebooks (GnProvider *self)
{
  g_assert (GN_IS_PROVIDER (self));

  /* Derived classes should implement this, if supported */
  return NULL;
}

static gboolean
gn_provider_real_load_items (GnProvider    *self,
                             GCancellable  *cancellable,
                             GError       **error)
{
  g_set_error_literal (error,
                       G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       "Loading items synchronously not supported");

  return FALSE;
}

static void
gn_provider_real_load_items_async (GnProvider          *self,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  g_task_report_new_error (self, callback, user_data,
                           gn_provider_real_load_items_async,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Loading items asynchronously not supported");
}

static gboolean
gn_provider_real_load_items_finish (GnProvider    *self,
                                    GAsyncResult  *result,
                                    GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
gn_provider_real_save_item_async (GnProvider          *self,
                                  GnProviderItem      *provider_item,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  g_task_report_new_error (self, callback, user_data,
                           gn_provider_real_load_items_async,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Saving item asynchronously not supported");
}

static gboolean
gn_provider_real_save_item_finish (GnProvider    *self,
                                   GAsyncResult  *result,
                                   GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
gn_provider_class_init (GnProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gn_provider_get_property;
  object_class->finalize = gn_provider_finalize;

  klass->get_notes = gn_provider_real_get_notes;
  klass->get_trash_notes = gn_provider_real_get_trash_notes;
  klass->get_notebooks = gn_provider_real_get_notebooks;

  klass->load_items = gn_provider_real_load_items;
  klass->load_items_async = gn_provider_real_load_items_async;
  klass->load_items_finish = gn_provider_real_load_items_finish;
  klass->save_item_async = gn_provider_real_save_item_async;
  klass->save_item_finish = gn_provider_real_save_item_finish;

  properties[PROP_UID] =
    g_param_spec_string ("uid",
                         "Uid",
                         "A unique id for the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "The name of the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_ICON] =
    g_param_spec_string ("icon",
                         "Icon",
                         "The icon name for the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_DOMAIN] =
    g_param_spec_string ("domain",
                         "Domain",
                         "The domain name of the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_USER_NAME] =
    g_param_spec_string ("user-name",
                         "User Name",
                         "The user name used for the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  /**
   * GnProvider::item-added:
   * @self: a #GnProvider
   * @item: a #GnProviderItem
   *
   * item-added signal is emitted when a new item is added
   * to the provider.
   */
  signals [ITEM_ADDED] =
    g_signal_new ("item-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GN_TYPE_PROVIDER_ITEM);

  /**
   * GnProvider::item-deleted:
   * @self: a #GnProvider
   * @item: a #GnProviderItem
   *
   * item-deleted signal is emitted when a new item is deleted
   * from the provider.
   */
  signals [ITEM_DELETED] =
    g_signal_new ("item-deleted",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GN_TYPE_PROVIDER_ITEM);

  /**
   * GnProvider::item-trashed:
   * @self: a #GnProvider
   * @item: a #GnProviderItem
   *
   * item-trashed signal is emitted when an item is trashed
   * in the provider.
   */
  signals [ITEM_TRASHED] =
    g_signal_new ("item-trashed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GN_TYPE_PROVIDER_ITEM);

  /**
   * GnProvider::item-restored:
   * @self: a #GnProvider
   * @item: a #GnProviderItem
   *
   * item-restored signal is emitted when an item is restored
   * from the trash in the provider.
   */
  signals [ITEM_RESTORED] =
    g_signal_new ("item-restored",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GN_TYPE_PROVIDER_ITEM);

  /**
   * GnProvider::item-updated:
   * @self: a #GnProvider
   * @item: a #GnProviderItem
   *
   * item-updated signal is emitted when an item is updated
   * in the provider.
   */
  signals [ITEM_UPDATED] =
    g_signal_new ("item-updated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GN_TYPE_PROVIDER_ITEM);

}

static void
gn_provider_init (GnProvider *self)
{
}

/**
 * gn_provider_get_uid:
 * @self: a #GnProvider
 *
 * Get the unique id of the provider
 *
 * Returns: (transfer full) (nullable): the uid
 * of the provider. Free with g_free().
 */
gchar *
gn_provider_get_uid (GnProvider *self)
{
  gchar *uid;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), NULL);

  uid = GN_PROVIDER_GET_CLASS (self)->get_uid (self);

  GN_RETURN (uid);
}

/**
 * gn_provider_get_name:
 * @self: a #GnProvider
 *
 * Get the name of the provider
 *
 * Returns: (transfer full) (nullable): the name
 * of the provider. Free with g_free().
 */
gchar *
gn_provider_get_name (GnProvider *self)
{
  gchar *name;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), NULL);

  name = GN_PROVIDER_GET_CLASS (self)->get_name (self);

  GN_RETURN (name);
}

/**
 * gn_item_get_icon:
 * @self: a #GnProvider
 *
 * Get the icon name of the provider
 *
 * Returns: (transfer full) (nullable): the icon
 * of the provider. Free with g_free().
 */
gchar *
gn_provider_get_icon (GnProvider *self)
{
  gchar *icon;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), NULL);

  icon = GN_PROVIDER_GET_CLASS (self)->get_icon (self);

  GN_RETURN (icon);
}

/**
 * gn_provider_get_domain:
 * @self: a #GnProvider
 *
 * Get the domain name of the provider
 *
 * Returns: (transfer full) (nullable): the domain
 * name of the provider. Free with g_free().
 */
gchar *
gn_provider_get_domain (GnProvider *self)
{
  gchar *domain;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), NULL);

  domain = GN_PROVIDER_GET_CLASS (self)->get_domain (self);

  GN_RETURN (domain);
}

/**
 * gn_provider_get_user_name:
 * @self: a #GnProvider
 *
 * Get the user name used to connect to the provider.
 *
 * Returns: (transfer full) (nullable): the user name
 * connected to the provider. Free with g_free().
 */
gchar *
gn_provider_get_user_name (GnProvider *self)
{
  gchar *user_name;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), NULL);

  user_name = GN_PROVIDER_GET_CLASS (self)->get_user_name (self);

  GN_RETURN (user_name);
}

/**
 * gn_provider_load_items:
 * @self: a #GnProvider
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @error: A #GError
 *
 * Synchronously load all items (Notes and Notebooks)
 * from the provider.
 *
 * Returns: %TRUE if provider items loaded successfully.
 * %FALSE otherwise.
 */
gboolean
gn_provider_load_items (GnProvider    *self,
                        GCancellable  *cancellable,
                        GError       **error)
{
  gboolean ret;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), FALSE);
  g_return_val_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable), FALSE);

  ret = GN_PROVIDER_GET_CLASS (self)->load_items (self, cancellable, error);

  GN_RETURN (ret);
}

/**
 * gn_provider_load_items_async:
 * @self: a #GnProvider
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @callback: a #GAsyncReadyCallback, or %NULL
 * @user_data: closure data for @callback
 *
 * Asynchronously load all items (Notes and Notebooks)
 * from the provider.
 *
 * @callback should complete the operation by calling
 * gn_provider_load_items_finish().
 */
void
gn_provider_load_items_async (GnProvider          *self,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  GnProviderPrivate *priv = gn_provider_get_instance_private (self);

  GN_ENTRY;

  g_return_if_fail (GN_IS_PROVIDER (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (priv->loaded == FALSE);

  priv->loaded = TRUE;

  GN_PROVIDER_GET_CLASS (self)->load_items_async (self, cancellable,
                                                  callback, user_data);

  GN_EXIT;
}

/**
 * gn_provider_load_items_finish:
 * @self: a #GnProvider
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError or %NULL
 *
 * Completes an asynchronous loading of items initiated
 * with gn_provider_load_items_async().
 *
 * Returns: %TRUE if items loaded successfully. %FALSE otherwise.
 */
gboolean
gn_provider_load_items_finish (GnProvider    *self,
                               GAsyncResult  *result,
                               GError       **error)
{
  gboolean ret;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  ret = GN_PROVIDER_GET_CLASS (self)->load_items_finish (self, result, error);

  GN_RETURN (ret);
}

/**
 * gn_provider_save_item_async:
 * @self: a #GnProvider
 * @provider_item: a #GnProviderItem
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @callback: a #GAsyncReadyCallback, or %NULL
 * @user_data: closure data for @callback
 *
 * Asynchronously save the @provider_item. If the item
 * isn't saved at all, a new item (ie, a file, or database
 * entry, or whatever) is created. Else, the old item is
 * updated with the new data.
 *
 * @callback should complete the operation by calling
 * gn_provider_save_item_finish().
 */
void
gn_provider_save_item_async (GnProvider          *self,
                             GnProviderItem      *provider_item,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
  GN_ENTRY;

  g_return_if_fail (GN_IS_PROVIDER (self));
  g_return_if_fail (GN_IS_PROVIDER_ITEM (provider_item));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  GN_PROVIDER_GET_CLASS (self)->save_item_async (self, provider_item,
                                                 cancellable, callback,
                                                 user_data);
  GN_EXIT;
}

/**
 * gn_provider_save_item_finish:
 * @self: a #GnProvider
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError or %NULL
 *
 * Completes saving an item initiated with
 * gn_provider_save_item_async().
 *
 * Returns: %TRUE if items was saved successfully. %FALSE otherwise.
 */
gboolean
gn_provider_save_item_finish (GnProvider    *self,
                              GAsyncResult  *result,
                              GError       **error)
{
  gboolean ret;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  ret = GN_PROVIDER_GET_CLASS (self)->save_item_finish (self, result, error);

  GN_RETURN (ret);
}

/**
 * gn_provider_get_notes:
 * @self: a #GnProvider
 *
 * Get the list of notes loaded by the provider. This
 * should be called only after gn_provider_load_items()
 * or gn_provider_load_items_async() is called.
 *
 * Returns: (transfer none) (nullable): A #GList of
 * #GnProviderItem or %NULL if empty.
 */
GList *
gn_provider_get_notes (GnProvider *self)
{
  GList *notes;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), NULL);

  notes = GN_PROVIDER_GET_CLASS (self)->get_notes (self);

  GN_RETURN (notes);
}

/**
 * gn_provider_get_trash_notes:
 * @self: a #GnProvider
 *
 * Get the list of trashed notes loaded by the provider. This
 * should be called only after gn_provider_load_items()
 * or gn_provider_load_items_async() is called.
 *
 * Returns: (transfer none) (nullable): A #GList of
 * #GnProviderItem or %NULL if empty.
 */
GList *
gn_provider_get_trash_notes (GnProvider *self)
{
  GList *notes;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER (self), NULL);

  notes = GN_PROVIDER_GET_CLASS (self)->get_trash_notes (self);

  GN_RETURN (notes);
}
