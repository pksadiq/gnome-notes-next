/* gn-goa-provider.c
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

#define G_LOG_DOMAIN "gn-goa-provider"

#include "config.h"

#include "gn-item.h"
#include "gn-plain-note.h"
#include "gn-goa-provider.h"
#include "gn-utils.h"
#include "gn-trace.h"

/**
 * SECTION: gn-goa-provider
 * @title: GnGoaProvider
 * @short_description: GNOME online accounts provider
 * @include: "gn-goa-provider.h"
 */

struct _GnGoaProvider
{
  GnProvider parent_instance;

  gchar *uid;
  gchar *name;
  gchar *icon;

  gchar *domain_name;
  gchar *user_name;

  GoaObject *goa_object;
  GVolume          *volume;
  GMount           *mount;
  GFile *note_dir;

  GList *notes;
};

G_DEFINE_TYPE (GnGoaProvider, gn_goa_provider, GN_TYPE_PROVIDER)

static void
gn_goa_provider_finalize (GObject *object)
{
  GnGoaProvider *self = (GnGoaProvider *)object;

  GN_ENTRY;

  g_clear_object (&self->goa_object);

  g_clear_pointer (&self->uid, g_free);
  g_clear_pointer (&self->name, g_free);

  G_OBJECT_CLASS (gn_goa_provider_parent_class)->finalize (object);

  GN_EXIT;
}

static gchar *
gn_goa_provider_get_uid (GnProvider *provider)
{
  return g_strdup (GN_GOA_PROVIDER (provider)->uid);
}

static gchar *
gn_goa_provider_get_name (GnProvider *provider)
{
  return g_strdup (GN_GOA_PROVIDER (provider)->name);
}

static GList *
gn_goa_provider_get_notes (GnProvider *provider)
{
  return GN_GOA_PROVIDER (provider)->notes;
}

static void
gn_goa_provider_check_note_dir (GTask        *task,
                                gpointer      source_object,
                                gpointer      task_data,
                                GCancellable *cancellable)
{
  GnGoaProvider *self = source_object;
  g_autoptr(GFile) root = NULL;
  g_autoptr(GError) error = NULL;

  GN_ENTRY;

  g_assert (G_IS_TASK (task));
  g_assert (GN_IS_GOA_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  root = g_mount_get_root (self->mount);
  self->note_dir = g_file_get_child (root, "Notes");

  /* FIXME: g_file_query_exists() doesn't work. */
  g_file_make_directory (self->note_dir, cancellable, &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
    g_clear_error (&error);

  if (error)
    g_task_return_error (task, g_steal_pointer (&error));
  else
    g_task_return_boolean (task, TRUE);

  GN_EXIT;
}

static void
gn_goa_provider_volume_mount_cb (GObject      *object,
                                 GAsyncResult *result,
                                 gpointer      user_data)
{
  GTask *task = user_data;
  GnGoaProvider *self;
  g_autoptr(GError) error = NULL;

  g_assert (G_IS_TASK (task));

  if (!g_volume_mount_finish (G_VOLUME (object), result, &error))
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  self = g_task_get_source_object (task);
  self->mount = g_volume_get_mount (self->volume);

  g_task_run_in_thread (task, gn_goa_provider_check_note_dir);
}

/*
 * This is pretty much a copy from gn-local-provider. May be we
 * can merge the code someday.
 */
static void
gn_goa_provider_load_items (GTask        *task,
                            gpointer      source_object,
                            gpointer      task_data,
                            GCancellable *cancellable)
{
  GnGoaProvider *self = source_object;
  g_autoptr(GFileEnumerator) enumerator = NULL;
  g_autoptr(GError) error = NULL;
  gpointer file_info_ptr;

  GN_ENTRY;

  g_assert (G_IS_TASK (task));
  g_assert (GN_IS_GOA_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  enumerator = g_file_enumerate_children (self->note_dir,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME","
                                          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                          G_FILE_QUERY_INFO_NONE,
                                          cancellable,
                                          &error);
  if (error != NULL)
    g_task_return_error (task, g_steal_pointer (&error));

  while ((file_info_ptr = g_file_enumerator_next_file (enumerator, cancellable, NULL)))
    {
      g_autoptr(GFileInfo) file_info = file_info_ptr;
      g_autoptr(GFile) file = NULL;
      g_autofree gchar *contents = NULL;
      g_autofree gchar *file_name = NULL;
      GnPlainNote *note;
      const gchar *name;
      gchar *end;

      name = g_file_info_get_name (file_info);

      if (!g_str_has_suffix (name, ".txt"))
        continue;

      file_name = g_strdup (name);
      end = g_strrstr (file_name, ".");
      *end = '\0';
      file = g_file_get_child (self->note_dir, name);
      g_file_load_contents (file, cancellable, &contents, NULL, NULL, NULL);

      note = gn_plain_note_new_from_data (contents);

      if (note == NULL)
        continue;

      gn_item_set_uid (GN_ITEM (note), file_name);
      g_object_set_data (G_OBJECT (note), "provider", GN_PROVIDER (self));
      g_object_set_data_full (G_OBJECT (note), "file", g_steal_pointer (&file),
                              g_object_unref);
      self->notes = g_list_prepend (self->notes, note);
    }

  g_task_return_boolean (task, TRUE);

  GN_EXIT;
}

static void
gn_goa_provider_load_items_async (GnProvider          *provider,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GnGoaProvider *self = (GnGoaProvider *)provider;
  g_autoptr(GTask) task = NULL;

  g_assert (GN_IS_GOA_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, gn_goa_provider_load_items_async);
  g_task_run_in_thread (task, gn_goa_provider_load_items);
}

static void
gn_goa_provider_class_init (GnGoaProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GnProviderClass *provider_class = GN_PROVIDER_CLASS (klass);

  object_class->finalize = gn_goa_provider_finalize;

  provider_class->get_uid = gn_goa_provider_get_uid;
  provider_class->get_name = gn_goa_provider_get_name;
  provider_class->get_notes = gn_goa_provider_get_notes;

  provider_class->load_items_async = gn_goa_provider_load_items_async;
}

static void
gn_goa_provider_init (GnGoaProvider *self)
{
}

GnGoaProvider *
gn_goa_provider_new (GoaObject *object)
{
  GnGoaProvider *self;
  GoaAccount *account;
  GoaFiles *goa_files;

  g_return_val_if_fail (GOA_IS_OBJECT (object), NULL);

  goa_files = goa_object_peek_files (object);
  g_return_val_if_fail (goa_files != NULL, NULL);

  account = goa_object_peek_account (object);

  self = g_object_new (GN_TYPE_GOA_PROVIDER, NULL);
  self->goa_object = g_object_ref (object);
  self->uid = goa_account_dup_id (account);
  self->name = g_strconcat ("Goa: ",
                            goa_account_get_provider_name (account),
                            NULL);

  return self;
}

void
gn_goa_provider_connect_async (GnGoaProvider       *self,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;
  g_autoptr(GVolumeMonitor) monitor = NULL;
  GoaFiles *goa_files;
  const gchar *uri;

  g_return_if_fail (GN_IS_GOA_PROVIDER (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  monitor = g_volume_monitor_get ();
  goa_files = goa_object_peek_files (self->goa_object);
  uri = goa_files_get_uri (goa_files);
  self->volume = g_volume_monitor_get_volume_for_uuid (monitor, uri);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, gn_goa_provider_connect_async);

  g_volume_mount (self->volume,
                  G_MOUNT_MOUNT_NONE,
                  NULL,
                  cancellable,
                  gn_goa_provider_volume_mount_cb,
                  g_steal_pointer (&task));

}

gboolean
gn_goa_provider_connect_finish (GnGoaProvider  *self,
                                GAsyncResult   *result,
                                GError        **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}
