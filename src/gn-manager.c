/* gn-manager.c
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

#define G_LOG_DOMAIN "gn-manager"

#include "config.h"

#include "gn-plain-note.h"
#include "gn-xml-note.h"
#include "gn-goa-provider.h"
#include "gn-memo-provider.h"
#include "gn-local-provider.h"
#include "gn-settings.h"
#include "gn-tag-store.h"
#include "gn-utils.h"
#include "gn-manager.h"
#include "gn-trace.h"

/**
 * SECTION: gn-manager
 * @title: GnManager
 * @short_description: A class to manage providers
 * @include: "gn-manager.h"
 */

/*
 * TODO:
 * 0. The search feature can have lots of improvement.
 *    * Current design assums items are not added/updated/removed midst search
 */

#define MAX_ITEMS_TO_LOAD 30

struct _GnManager
{
  GObject       parent_instance;

  GnSettings   *settings;

  ESourceRegistry *eds_registry;
  GoaClient *goa_client;

  GHashTable   *providers;
  GCancellable *provider_cancellable;

  GListStore   *list_of_notes_store;
  GListStore   *list_of_trash_store;
  GtkSliceListModel *notes_store;
  GtkSliceListModel *trash_store;
  GtkFilterListModel *search_store;
  GListStore *tag_store;

  GList       *delete_queue;

  /* Search */
  gchar *search_needle;

  gint providers_to_load;
};

G_DEFINE_TYPE (GnManager, gn_manager, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_PROVIDERS_LOADING,
  N_PROPS
};

enum {
  PROVIDER_ADDED,
  PROVIDER_REMOVED,
  DELETE_ITEMS,
  N_SIGNALS
};

static GParamSpec *properties[N_PROPS];
static guint signals[N_SIGNALS];

static gboolean
gn_manager_search_filter (gpointer item,
                          gpointer pp)
{
  gchar *search_string = *(gchar **)pp;

  return gn_item_match (GN_ITEM (item), search_string);
}

void
gn_manager_increment_pending_providers (GnManager *self)
{
  g_assert (GN_IS_MANAGER (self));
  g_assert (GN_IS_MAIN_THREAD ());

  self->providers_to_load++;

  /*
   * if providers_to_load is 1, it means we had 0 providers
   * to load previously.  So there is a change of state
   * from no providers loading -> some providers loading.
   * This is what PROP_PROVIDERS_LOADING property is about.
   */
  if (self->providers_to_load == 1)
    g_object_notify_by_pspec (G_OBJECT (self),
                              properties[PROP_PROVIDERS_LOADING]);
}

void
gn_manager_decrement_pending_providers (GnManager *self)
{
  g_assert (GN_IS_MANAGER (self));
  g_assert (GN_IS_MAIN_THREAD ());
  g_assert (self->providers_to_load > 0);

  self->providers_to_load--;

  /*
   * if providers_to_load is 0, it means we had 1 provider
   * to load previously.  So there is a change of state
   * from some providers loading -> no providers loading.
   */
  if (self->providers_to_load == 0)
    g_object_notify_by_pspec (G_OBJECT (self),
                              properties[PROP_PROVIDERS_LOADING]);
}

static void
gn_manager_save_item_cb (GObject      *object,
                         GAsyncResult *result,
                         gpointer      user_data)
{
  GnProvider *provider = (GnProvider *)object;
  g_autoptr(GError) error = NULL;

  g_assert (GN_IS_PROVIDER (provider));
  g_assert (G_IS_ASYNC_RESULT (result));

  if (!gn_provider_save_item_finish (provider, result, &error))
    g_warning ("Failed to save item: %s", error->message);
}

/* Load items from the provider to the store and queue */
static void
gn_manager_load_items (GnManager  *self,
                       GnProvider *provider)
{
  GListStore *items;

  items = gn_provider_get_notes (provider);
  if (items != NULL)
    g_list_store_append (self->list_of_notes_store, items);

  items = gn_provider_get_trash_notes (provider);
  if (items != NULL)
    g_list_store_append (self->list_of_trash_store, items);
}

static void
gn_manager_items_loaded_cb (GObject      *object,
                            GAsyncResult *result,
                            gpointer      user_data)
{
  GnProvider *provider = (GnProvider *)object;
  GnManager *self = user_data;
  g_autoptr(GError) error = NULL;

  g_assert (GN_IS_PROVIDER (provider));
  g_assert (GN_IS_MANAGER (self));

  if (gn_provider_load_items_finish (provider, result, &error))
    gn_manager_load_items (self, provider);
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    g_warning ("Failed to load items: %s", error->message);

  g_signal_emit (self, signals[PROVIDER_ADDED], 0, provider);
  gn_manager_decrement_pending_providers (self);
}

static void
gn_manager_load_and_save_provider (GnManager *self,
                                   GnProvider *provider)
{
  g_assert (GN_IS_MANAGER (self));
  g_assert (GN_IS_PROVIDER (provider));

  g_hash_table_insert (self->providers,
                       gn_provider_get_uid (provider),
                       provider);
  gn_manager_increment_pending_providers (self);
  gn_provider_load_items_async (provider,
                                self->provider_cancellable,
                                gn_manager_items_loaded_cb, self);
}

static void
gn_manager_load_memo_providers (GnManager *self)
{
  GnProvider *provider;
  GList *sources;

  g_assert (GN_IS_MANAGER (self));

  sources = e_source_registry_list_sources (self->eds_registry,
                                            E_SOURCE_EXTENSION_MEMO_LIST);

  for (GList *node = sources; node != NULL; node = node->next)
    {
      provider = gn_memo_provider_new (node->data);
      gn_manager_load_and_save_provider (self, provider);
    }

  g_list_free_full (sources, g_object_unref);
}

static void
gn_manager_load_goa_providers (GnManager *self)
{
  GList *accounts;
  GoaAccount *account;
  GnProvider *provider;

  g_assert (GN_IS_MANAGER (self));

  accounts = goa_client_get_accounts (self->goa_client);

  for (GList *account = accounts; account != NULL; account = account->next)
    {
      GoaObject *object = account->data;
      GoaAccount *account = goa_object_peek_account (object);

      if (goa_object_peek_files (object)  == NULL ||
          goa_account_get_files_disabled (account))
        continue;

      provider = gn_goa_provider_new (object);
      gn_manager_load_and_save_provider (self, provider);
    }

  g_list_free_full (accounts, g_object_unref);
}

static void
gn_manager_load_providers (GnManager *self)
{
  GnProvider *provider;
  g_autoptr(GTask) task = NULL;
  g_autoptr(GError) error = NULL;

  GN_ENTRY;

  g_assert (GN_IS_MANAGER (self));

  provider = gn_local_provider_new ();
  self->tag_store = gn_provider_get_tags (provider);
  gn_manager_load_and_save_provider (self, provider);

  /* TODO: Enable once the new design is ready */
  /* self->eds_registry = e_source_registry_new_sync (self->provider_cancellable, */
  /*                                                  &error); */

  /* if (error) */
  /*   { */
  /*     g_warning ("Error loading Evolution-Data-Server backend: %s", */
  /*                error->message); */
  /*     g_clear_error (&error); */
  /*   } */
  /* else */
  /*   gn_manager_load_memo_providers (self); */

  /* self->goa_client = goa_client_new_sync (self->provider_cancellable, &error); */

  /* if (error) */
  /*   g_warning ("Error loading GNOME Online accounts: %s", error->message); */
  /* else */
  /*   gn_manager_load_goa_providers (self); */

  GN_EXIT;
}

static void
gn_manager_dispose (GObject *object)
{
  GnManager *self = (GnManager *)object;

  GN_ENTRY;

  g_cancellable_cancel (self->provider_cancellable);
  g_clear_object (&self->provider_cancellable);

  g_clear_object (&self->settings);

  g_clear_object (&self->notes_store);
  g_clear_pointer (&self->providers, g_hash_table_unref);

  G_OBJECT_CLASS (gn_manager_parent_class)->dispose (object);

  GN_EXIT;
}

static void
gn_manager_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GnManager *self = (GnManager *)object;
  gboolean providers_loading = self->providers_to_load > 0;

  switch (prop_id)
    {
    case PROP_PROVIDERS_LOADING:
      g_value_set_boolean (value, providers_loading);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_manager_class_init (GnManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gn_manager_dispose;
  object_class->get_property = gn_manager_get_property;

  properties[PROP_PROVIDERS_LOADING] =
    g_param_spec_boolean ("providers-loading",
                          "Providers loading",
                          "TRUE if providers are being loaded",
                          TRUE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  /**
   * GnManager::provider-added:
   * @self: a #GnManager
   * @provider: a #GnProvider
   *
   * The "provider-added" signal is emitted when a new provider
   * is added and ready for use.
   */
  signals [PROVIDER_ADDED] =
    g_signal_new ("provider-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GN_TYPE_PROVIDER);
  g_signal_set_va_marshaller (signals [PROVIDER_ADDED],
                              G_TYPE_FROM_CLASS (klass),
                              g_cclosure_marshal_VOID__OBJECTv);

  /**
   * GnManager::provider-removed:
   * @self: a #GnManager
   * @provider: a #GnProvider
   *
   * The "provider-removed" signal is emitted when a new provider
   * is removed and ready for use.
   */
  signals [PROVIDER_REMOVED] =
    g_signal_new ("provider-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GN_TYPE_PROVIDER);
  g_signal_set_va_marshaller (signals [PROVIDER_REMOVED],
                              G_TYPE_FROM_CLASS (klass),
                              g_cclosure_marshal_VOID__OBJECTv);

  /**
   * GnManager::delete-items:
   * @self: a #GnManager
   * @count: Number of items to delete
   *
   * The "delete-items" signal is emitted when the items are
   * queued for deletion.
   */
  signals [DELETE_ITEMS] =
    g_signal_new ("delete-items",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
gn_manager_init (GnManager *self)
{
  GtkFlattenListModel *model;

  self->settings = gn_settings_new (PACKAGE_ID);

  self->providers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, NULL);
  self->list_of_notes_store = g_list_store_new (G_TYPE_LIST_MODEL);
  self->list_of_trash_store = g_list_store_new (G_TYPE_LIST_MODEL);
  model = gtk_flatten_list_model_new (GN_TYPE_ITEM,
                                      G_LIST_MODEL (self->list_of_notes_store));
  self->notes_store = gtk_slice_list_model_new (G_LIST_MODEL (model),
                                                0, MAX_ITEMS_TO_LOAD);
  g_object_unref (model);
  model = gtk_flatten_list_model_new (GN_TYPE_ITEM,
                                      G_LIST_MODEL (self->list_of_trash_store));
  self->trash_store = gtk_slice_list_model_new (G_LIST_MODEL (model),
                                                0, MAX_ITEMS_TO_LOAD);
  g_object_unref (model);
  self->provider_cancellable = g_cancellable_new ();

  self->search_store = gtk_filter_list_model_new (G_LIST_MODEL (self->notes_store),
                                                  gn_manager_search_filter,
                                                  &self->search_needle, NULL);

  gn_manager_load_providers (self);
}

GnManager *
gn_manager_get_default (void)
{
  static GnManager *self;

  if (self == NULL)
    self = g_object_new (GN_TYPE_MANAGER,
                         NULL);

  return self;
}

/**
 * gn_manager_get_settings:
 * self: A #GnManager
 *
 * Get the default #GnSettings.
 *
 * Returns: (transfer none): A #GnSettings
 */
GnSettings *
gn_manager_get_settings (GnManager *self)
{
  return self->settings;
}

/**
 * gn_manager_get_default_provider:
 * self: A #GnManager
 * @show_disconnected: Search in disconnect providers
 *
 * Get the default provider to which new notes will
 * be saved.
 *
 * If @show_disconnected is %TRUE, return the default
 * provider even if the provider isn't yet connected.
 *
 * If @show_disconnected is %FALSE, or if the default
 * provider isn't available (eg.: The user has deleted
 * the default provider), return the local provider.
 *
 * Returns: (transfer none): A #GnProvider
 */
GnProvider *
gn_manager_get_default_provider (GnManager *self,
                                 gboolean   show_disconnected)
{
  GnProvider *provider;
  const gchar *name;

  g_return_val_if_fail (GN_IS_MANAGER (self), NULL);

  name = gn_settings_get_provider_name (self->settings);
  provider = g_hash_table_lookup (self->providers, name);

  if (provider == NULL ||
      !(show_disconnected || gn_provider_has_loaded (provider)))
    provider = g_hash_table_lookup (self->providers, "local");

  return provider;
}

/**
 * gn_manager_get_notes_store:
 * @self: A #GnManager
 *
 * Get a sorted list of notes loaded from providers.
 *
 * Returns: (transfer none): a #GListStore
 */
GListModel *
gn_manager_get_notes_store (GnManager *self)
{
  return G_LIST_MODEL (self->notes_store);
}

/**
 * gn_manager_get_tag_store:
 * @self: A #GnManager
 *
 * Get a sorted list of tags/labels for local notes.
 *
 * Returns: (transfer none): a #GListStore
 */
GListModel *
gn_manager_get_tag_store (GnManager *self)
{
  return G_LIST_MODEL (self->tag_store);
}

/**
 * gn_manager_get_trash_notes_store:
 * @self: A #GnManager
 *
 * Get a sorted list of trashed notes loaded from providers.
 *
 * Returns: (transfer none): a #GListStore
 */
GListModel *
gn_manager_get_trash_notes_store (GnManager *self)
{
  return G_LIST_MODEL (self->trash_store);
}

/**
 * gn_manager_get_search_store:
 * @self: A #GnManager
 *
 * Get a sorted list of items for search results.
 *
 * Returns: (transfer none): a #GListStore
 */
GListModel *
gn_manager_get_search_store (GnManager *self)
{
  return G_LIST_MODEL (self->search_store);
}

/**
 * gn_manager_new_note:
 * @self: A #GnManager
 *
 * Create a New empty note for the default provider.
 * The default provider name can be retrieved by
 * gn_settings_get_provider_name().
 *
 * The format of the note is decided based on
 * the default provider.
 *
 * Returns: (transfer full): a #GnItem
 */
GnItem *
gn_manager_new_note (GnManager *self)
{
  GnProvider *provider;
  GnItem *item;

  provider = gn_manager_get_default_provider (self, FALSE);

  if (GN_IS_LOCAL_PROVIDER (provider))
    item = GN_ITEM (gn_xml_note_new_from_data (NULL, 0, NULL));
  else
    item = GN_ITEM (gn_plain_note_new_from_data (NULL, 0));

  g_object_set_data (G_OBJECT (item), "provider", provider);

  return item;
}

void
gn_manager_save_item (GnManager *self,
                      GnItem    *item)
{
  GnProvider *provider;

  g_return_if_fail (GN_IS_MANAGER (self));
  g_return_if_fail (GN_IS_ITEM (item));

  provider = g_object_get_data (G_OBJECT (item), "provider");

  gn_provider_save_item_async (provider, item, NULL,
                               gn_manager_save_item_cb, NULL);
}

/**
 * gn_manager_queue_for_delete:
 * @self: A #GnManager
 * @note_store: A #GListStore
 * @items: A #GList of #GnItem
 *
 * Queue a #GList of #GnItems from @note_store.
 * If the item is a note, it should be present in @note_store.
 * If it's a notebook, the function shall take care of that.
 * The queued items will be removed from store.
 *
 * The reference on @items is taken by the manager.
 * Don't free it yourself.
 */
void
gn_manager_queue_for_delete (GnManager  *self,
                             GListModel *note_store,
                             GList      *items)
{
  guint position;
  guint count = 0;

  g_return_if_fail (GN_IS_MANAGER (self));
  g_return_if_fail (G_IS_LIST_MODEL (note_store));

  self->delete_queue = items;

  /* FIXME: The story is very different when notebooks come into scene */
  for (GList *node = items; node != NULL; node = node->next)
    {
      GListStore *notes_store;
      GnProvider *provider;

      provider = g_object_get_data (G_OBJECT (node->data), "provider");
      notes_store = gn_provider_get_notes (provider);

      if (gn_utils_get_item_position (G_LIST_MODEL (notes_store),
                                      node->data, &position))
        {
          g_list_store_remove (notes_store, position);
          count++;
        }
    }

  if (count > 0)
    g_signal_emit (self, signals[DELETE_ITEMS], 0, count);
}

/**
 * gn_manager_dequeue_delete:
 * @self: A #GnManager
 *
 * Dequeue the items marked for deletion. The items
 * Will be restored back to the corresponding store.
 *
 * Returns: %TRUE if dequeue succeeded. %FALSE otherwise
 * (ie, The queue was empty and was already deleted).
 */
gboolean
gn_manager_dequeue_delete (GnManager *self)
{
  g_return_val_if_fail (GN_IS_MANAGER (self), FALSE);

  if (self->delete_queue == NULL)
    return FALSE;

  for (GList *node = self->delete_queue; node != NULL; node = node->next)
    {
      GListStore *notes_store;
      GnProvider *provider;

      provider = g_object_get_data (G_OBJECT (node->data), "provider");
      notes_store = gn_provider_get_notes (provider);

      g_list_store_insert_sorted (notes_store, node->data,
                                  gn_item_compare, NULL);
    }

  g_clear_pointer (&self->delete_queue, g_list_free);
  return TRUE;
}

/**
 * gn_manager_trash_queue_items:
 * @self: A #GnManager
 *
 * Trash the items queued for deletion with
 * gn_manager_queue_for_delete().
 */
void
gn_manager_trash_queue_items (GnManager *self)
{
  g_return_if_fail (GN_IS_MANAGER (self));

  for (GList *node = self->delete_queue; node != NULL; node = node->next)
    {
      g_autoptr(GError) error = NULL;
      GnProvider *provider;

      provider = g_object_get_data (G_OBJECT (node->data), "provider");
      gn_provider_trash_item (provider, node->data, NULL, &error);

      if (error != NULL)
        g_warning ("Error deleting item: %s", error->message);
    }

  g_clear_pointer (&self->delete_queue, g_list_free);
}

/**
 * gn_manager_search:
 * @self: A #GnManager
 * @terms: A %NULL terminated array of strings
 *
 * Asynchronously search for items that matches
 * @terms.  The seach store shall be then updated
 * in the main thread.  The store can be retrieved
 * with gn_manager_get_search_store().
 *
 * Currently only text search is supported.  That
 * is, the first string from @terms is searched.
 * The rest is ignored.
 */
void
gn_manager_search (GnManager    *self,
                   const gchar **terms)
{
  g_return_if_fail (GN_IS_MANAGER (self));

  g_free (self->search_needle);
  self->search_needle = g_strdup (terms[0]);
  gtk_filter_list_model_refilter (self->search_store);
}

/**
 * gn_manager_get_providers:
 * @self: A #GnManager
 *
 * Get the list of providers in #GnManager
 *
 * Returns: (transfer container): A #GList of #GnProvider
 * Free with g_list_free().
 */
GList *
gn_manager_get_providers (GnManager *self)
{
  g_return_val_if_fail (GN_IS_MANAGER (self), NULL);

  return g_hash_table_get_values (self->providers);
}
