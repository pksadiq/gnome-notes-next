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

#include "gn-provider-item.h"
#include "gn-plain-note.h"
#include "gn-local-provider.h"
#include "gn-settings.h"
#include "gn-utils.h"
#include "gn-manager.h"
#include "gn-trace.h"

/**
 * SECTION: gn-manager
 * @title: GnManager
 * @short_description: A singleton class to manage providers
 * @include: "gn-manager.h"
 */

#define MAX_ITEMS_TO_LOAD 30

struct _GnManager
{
  GObject       parent_instance;

  GnSettings   *settings;

  GHashTable   *providers;
  GCancellable *provider_cancellable;

  GQueue       *notes_queue;
  GQueue       *notebooks_queue;
  GQueue       *trash_notes_queue;
  GListStore   *notes_store;
  GListStore   *notebooks_store;
  GListStore   *trash_notes_store;
};

G_DEFINE_TYPE (GnManager, gn_manager, G_TYPE_OBJECT)

enum {
  PROVIDER_ADDED,
  PROVIDER_REMOVED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

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

static void
gn_manager_load_more_items (GnManager   *self,
                            GListStore **store,
                            GQueue     **queue)
{
  GnProviderItem *provider_item;
  int i = 0;

  g_assert (GN_IS_MANAGER (self));
  g_assert (G_IS_LIST_STORE (*store));

  while ((provider_item = g_queue_pop_head (*queue)))
    {
      g_list_store_append (*store, provider_item);

      i++;
      if (i >= MAX_ITEMS_TO_LOAD)
        break;
    }
}

static void
gn_manager_load_local_providers (GTask        *task,
                                 gpointer      source_object,
                                 gpointer      task_data,
                                 GCancellable *cancellable)
{
  GnManager *self = source_object;
  GnProvider *provider;
  g_autoptr(GError) error = NULL;
  GList *provider_items;
  gboolean success;

  g_assert (G_IS_TASK (task));
  g_assert (GN_IS_MANAGER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  provider = GN_PROVIDER (gn_local_provider_new ());

  g_hash_table_insert (self->providers,
                       gn_provider_get_uid (provider),
                       provider);
  success = gn_provider_load_items (provider,
                                    self->provider_cancellable,
                                    &error);
  if (success)
    {
      provider_items = gn_provider_get_notes (provider);

      for (GList *item = provider_items; item != NULL; item = item->next)
        {
          g_queue_insert_sorted (self->notes_queue, item->data,
                                 gn_provider_item_compare, NULL);
        }

      provider_items = gn_provider_get_trash_notes (provider);

      for (GList *item = provider_items; item != NULL; item = item->next)
        {
          g_queue_insert_sorted (self->trash_notes_queue, item->data,
                                 gn_provider_item_compare, NULL);
        }
    }

  gn_manager_load_more_items (self, &self->notes_store,
                              &self->notes_queue);
  gn_manager_load_more_items (self, &self->trash_notes_store,
                              &self->trash_notes_queue);
}

static void
gn_manager_load_local_providers_cb (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  GnManager *self = (GnManager *)object;
  GList *providers;

  g_assert (GN_IS_MANAGER (self));
  g_assert (G_IS_ASYNC_RESULT (result));

  providers = g_hash_table_get_values (self->providers);

  for (GList *provider = providers; provider != NULL; provider = provider->next)
    {
      g_signal_emit (self, signals[PROVIDER_ADDED], 0, provider->data);
    }
}

static void
gn_manager_load_providers (GnManager *self)
{
  g_autoptr(GTask) task = NULL;

  GN_ENTRY;

  g_assert (GN_IS_MANAGER (self));

  task = g_task_new (self, self->provider_cancellable,
                     gn_manager_load_local_providers_cb, NULL);
  g_task_set_source_tag (task, gn_manager_load_providers);
  g_task_run_in_thread (task, gn_manager_load_local_providers);

  GN_EXIT;
}

static void
gn_manager_dispose (GObject *object)
{
  GnManager *self = (GnManager *)object;

  GN_ENTRY;

  g_cancellable_cancel (self->provider_cancellable);
  g_clear_object (&self->provider_cancellable);

  g_clear_pointer (&self->notes_queue, g_queue_free);
  g_clear_pointer (&self->notes_queue, g_queue_free);
  g_clear_pointer (&self->notes_queue, g_queue_free);
  g_clear_object (&self->notes_store);
  g_clear_object (&self->notebooks_store);
  g_clear_object (&self->trash_notes_store);
  g_clear_pointer (&self->providers, g_hash_table_unref);

  G_OBJECT_CLASS (gn_manager_parent_class)->dispose (object);

  GN_EXIT;
}

static void
gn_manager_class_init (GnManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gn_manager_dispose;

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
}

static void
gn_manager_init (GnManager *self)
{
  self->settings = gn_settings_new ("org.sadiqpk.notes");

  self->providers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, NULL);
  self->notes_queue = g_queue_new ();
  self->notes_store = g_list_store_new (GN_TYPE_PROVIDER_ITEM);
  self->trash_notes_queue = g_queue_new ();
  self->trash_notes_store = g_list_store_new (GN_TYPE_PROVIDER_ITEM);
  self->provider_cancellable = g_cancellable_new ();

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
 *
 * Get the default provider to which new notes will
 * be saved.
 *
 * If the default provider is not available, the
 * local provider is returned.
 *
 * Returns: (transfer none): A #GnProvider
 */
GnProvider *
gn_manager_get_default_provider (GnManager *self)
{
  GnProvider *provider;
  const gchar *name;

  g_return_val_if_fail (GN_IS_MANAGER (self), NULL);

  name = gn_settings_get_provider_name (self->settings);
  provider = g_hash_table_lookup (self->providers, name);

  if (provider == NULL)
    g_hash_table_lookup (self->providers, "local");

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
GListStore *
gn_manager_get_notes_store (GnManager *self)
{
  return self->notes_store;
}

/**
 * gn_manager_get_trash_notes_store:
 * @self: A #GnManager
 *
 * Get a sorted list of trashed notes loaded from providers.
 *
 * Returns: (transfer none): a #GListStore
 */
GListStore *
gn_manager_get_trash_notes_store (GnManager *self)
{
  return self->trash_notes_store;
}

/**
 * gn_manager_load_more_notes:
 * @self: A #GnManager
 *
 * Load more notes to the store from the queue, if any.
 */
void
gn_manager_load_more_notes (GnManager *self)
{
  /* FIXME: use a GMutex instead? */
  g_return_if_fail (GN_IS_MAIN_THREAD ());

  if (g_queue_is_empty (self->notes_queue))
    return;

  gn_manager_load_more_items (self, &self->notes_store,
                              &self->notes_queue);
}

/**
 * gn_manager_load_more_trash_notes:
 * @self: A #GnManager
 *
 * Load more trash notes to the store from the queue, if any.
 */
void
gn_manager_load_more_trash_notes (GnManager *self)
{
  /* FIXME: use a GMutex instead? */
  g_return_if_fail (GN_IS_MAIN_THREAD ());

  if (g_queue_is_empty (self->trash_notes_queue))
    return;

  gn_manager_load_more_items (self, &self->trash_notes_store,
                              &self->trash_notes_queue);
}

/**
 * gn_manager_new_note:
 * @self: A #GnManager
 *
 * Create a New empty note for the default provider.
 * The default provider name can be retried by
 * gn_settings_get_provider_name().
 *
 * The format of the note is decided based on
 * the default provider.
 *
 * Returns: (transfer full): a #GnProviderItem
 */
GnProviderItem *
gn_manager_new_note (GnManager *self)
{
  GnProvider *provider;
  GnNote *note;

  provider = gn_manager_get_default_provider (self);

  /* TODO: set note type based on provider */
  note = GN_NOTE (gn_plain_note_new_from_data (NULL));

  return gn_provider_item_new (provider, GN_ITEM (note));
}

void
gn_manager_save_item (GnManager      *self,
                      GnProviderItem *provider_item)
{
  GnProvider *provider;
  GnItem *item;

  g_return_if_fail (GN_IS_MANAGER (self));
  g_return_if_fail (GN_IS_PROVIDER_ITEM (provider_item));

  provider = gn_provider_item_get_provider (provider_item);
  item = gn_provider_item_get_item (provider_item);

  gn_provider_save_item_async (provider, provider_item,
                               self->provider_cancellable,
                               gn_manager_save_item_cb, NULL);
}
