/* gn-local-provider.c
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

#define G_LOG_DOMAIN "gn-local-provider"

#include "config.h"

#include <glib/gi18n.h>

#include "gn-xml-note.h"
#include "gn-plain-note.h"
#include "gn-local-provider.h"
#include "gn-trace.h"

/**
 * SECTION: gn-local-provider
 * @title: GnLocalProvider
 * @short_description: The local provider class
 * @include: "gn-local-provider.h"
 *
 * The local provider store notes as files. The content is also
 * cached in the tracker database (if available), so that users
 * can search in notes faster.
 */

/*
 * The notes are saved in the user config dir (usually in
 * ~/.local/share/gnome-notes for non flatpaked application) in the
 * format xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.eee where xxx.. is
 * some unique id for the file.
 *
 * The notebook names are stored in the file notebooks.xml in the same
 * directory. The unique ids of the notes in each notebook is also
 * added here.
 */

struct _GnLocalProvider
{
  GnProvider parent_instance;

  gchar *domain;
  gchar *user_name;

  gchar *location;
  gchar *trash_location;

  GList *notes;
  GList *trash_notes;
};

G_DEFINE_TYPE (GnLocalProvider, gn_local_provider, GN_TYPE_PROVIDER)

static void
gn_local_provider_dispose (GObject *object)
{
  GN_ENTRY;

  G_OBJECT_CLASS (gn_local_provider_parent_class)->dispose (object);

  GN_EXIT;
}

static void
gn_local_provider_finalize (GObject *object)
{
  GnLocalProvider *self = (GnLocalProvider *)object;

  GN_ENTRY;

  g_clear_pointer (&self->domain, g_free);
  g_clear_pointer (&self->user_name, g_free);
  g_clear_pointer (&self->location, g_free);

  g_list_free_full (self->notes, g_object_unref);

  G_OBJECT_CLASS (gn_local_provider_parent_class)->finalize (object);

  GN_EXIT;
}

static gchar *
gn_local_provider_get_uid (GnProvider *provider)
{
  return g_strdup ("local");
}

static const gchar *
gn_local_provider_get_name (GnProvider *provider)
{
  return _("Local");
}

static GIcon *
gn_local_provider_get_icon (GnProvider  *provider,
                            GError     **error)
{
  return g_icon_new_for_string ("user-home", error);
}

static gchar *
gn_local_provider_get_domain (GnProvider *provider)
{
  return g_strdup (GN_LOCAL_PROVIDER (provider)->domain);
}

static gchar *
gn_local_provider_get_user_name (GnProvider *provider)
{
  GnLocalProvider *self = GN_LOCAL_PROVIDER (provider);

  return self->user_name ? g_strdup (self->user_name) : g_strdup ("");
}

static const gchar *
gn_local_provider_get_location_name (GnProvider *provider)
{
  return _("On This Computer");
}

static void
gn_local_provider_load_path (GnLocalProvider  *self,
                             const gchar      *path,
                             GList           **items,
                             GCancellable     *cancellable,
                             GError          **error)
{
  g_autoptr(GFileEnumerator) enumerator = NULL;
  g_autoptr(GFile) location = NULL;
  gpointer file_info_ptr;

  GN_ENTRY;

  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));
  g_assert (path != NULL);

  location = g_file_new_for_path (path);
  enumerator = g_file_enumerate_children (location,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME","
                                          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                          G_FILE_QUERY_INFO_NONE,
                                          cancellable,
                                          error);
  if (*error != NULL)
    GN_EXIT;

  while ((file_info_ptr = g_file_enumerator_next_file (enumerator, cancellable, NULL)))
    {
      g_autoptr(GFileInfo) file_info = file_info_ptr;
      g_autoptr(GFile) file = NULL;
      g_autofree gchar *contents = NULL;
      g_autofree gchar *file_name = NULL;
      GnXmlNote *note;
      const gchar *name;
      gchar *end;

      name = g_file_info_get_name (file_info);

      if (!g_str_has_suffix (name, ".note"))
        continue;

      file_name = g_strdup (name);
      end = g_strrstr (file_name, ".");
      *end = '\0';
      file = g_file_get_child (location, name);
      g_file_load_contents (file, cancellable, &contents, NULL, NULL, NULL);

      note = gn_xml_note_new_from_data (contents);

      if (note == NULL)
        continue;

      gn_item_set_uid (GN_ITEM (note), file_name);
      g_object_set_data (G_OBJECT (note), "provider", GN_PROVIDER (self));
      g_object_set_data_full (G_OBJECT (note), "file", g_steal_pointer (&file),
                              g_object_unref);
      *items = g_list_prepend (*items, note);
    }
}

static void
gn_local_provider_load_notes (GTask        *task,
                              gpointer      source_object,
                              gpointer      task_data,
                              GCancellable *cancellable)
{
  GnLocalProvider *self = source_object;
  GError *error = NULL;

  g_assert (G_IS_TASK (task));
  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  gn_local_provider_load_path (self, self->location, &self->notes,
                               cancellable, &error);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  gn_local_provider_load_path (self, self->trash_location,
                               &self->trash_notes,
                               cancellable, &error);
  if (error)
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
}

static void
gn_local_provider_load_items_async (GnProvider          *provider,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
{
  GnLocalProvider *self = (GnLocalProvider *)provider;
  g_autoptr(GTask) task = NULL;

  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, gn_local_provider_load_items_async);
  g_task_run_in_thread (task, gn_local_provider_load_notes);
}

static void
gn_local_provider_save_note (GnLocalProvider *self,
                             GnItem          *item,
                             GTask           *task,
                             GCancellable    *cancellable)
{
  GFile *file;
  g_autofree gchar *content = NULL;
  gchar *full_content;
  g_autoptr(GError) error = NULL;

  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (GN_IS_ITEM (item));
  g_assert (G_IS_TASK (task));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  file = g_object_get_data (G_OBJECT (item), "file");
  content = gn_note_get_raw_content (GN_NOTE (item));

  if (content)
    full_content = content;
  else
    full_content = "";

  if (file == NULL)
    {
      g_autofree gchar *uuid = NULL;
      g_autofree gchar *file_name = NULL;

      uuid = g_uuid_string_random ();
      file_name = g_strconcat (uuid, gn_note_get_extension (GN_NOTE (item)), NULL);
      file = g_file_new_build_filename (g_get_user_data_dir (),
                                        "gnome-notes", file_name, NULL);
      g_object_set_data_full (G_OBJECT (item), "file", file,
                              g_object_unref);
    }

  g_file_replace_contents (file, full_content, strlen (full_content),
                           NULL, FALSE, 0, NULL, NULL, &error);

  if (error == NULL)
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, g_steal_pointer (&error));
}

static void
gn_local_provider_real_save_item (GTask        *task,
                                  gpointer      source_object,
                                  gpointer      task_data,
                                  GCancellable *cancellable)
{
  GnLocalProvider *self = source_object;
  GnItem *item = task_data;

  GN_ENTRY;

  g_assert (G_IS_TASK (task));
  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (GN_IS_ITEM (item));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  if (GN_IS_NOTE (item))
    gn_local_provider_save_note (self, item, task, cancellable);
  GN_EXIT;
}

static void
gn_local_provider_save_item_async (GnProvider          *provider,
                                   GnItem              *item,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  GnLocalProvider *self = (GnLocalProvider *)provider;
  g_autoptr(GTask) task = NULL;

  GN_ENTRY;

  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (GN_IS_ITEM (item));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, gn_local_provider_save_item_async);
  g_task_set_task_data (task, g_object_ref (item), g_object_unref);
  g_task_run_in_thread (task, gn_local_provider_real_save_item);

  GN_EXIT;
}

static gboolean
gn_local_provider_save_item_finish (GnProvider   *self,
                                    GAsyncResult *result,
                                    GError       **error)
{
  GnItem *item;
  gboolean ret;

  ret = g_task_propagate_boolean (G_TASK (result), error);

  if (!ret)
    return ret;

  item = g_task_get_task_data (G_TASK (result));
  g_signal_emit_by_name (self, "item-added", item);

  if (gn_item_is_new (item))
    {
      GFile *file;
      g_autofree gchar *file_name = NULL;
      gchar *end;

      if (GN_IS_NOTE (item))
        {
          file = g_object_get_data (G_OBJECT (item), "file");
          g_assert (file != NULL);
          file_name = g_file_get_basename (file);
          end = g_strrstr (file_name, ".");

          /* strip the extension to get the uid */
          if (end != NULL)
            *end = '\0';

          gn_item_set_uid (item, file_name);
        }
    }

  return ret;
}

static gboolean
gn_local_provider_trash_item (GnProvider    *provider,
                              GnItem        *item,
                              GCancellable  *cancellable,
                              GError       **error)
{
  GnLocalProvider *self = (GnLocalProvider *)provider;
  g_autofree gchar *base_name = NULL;
  g_autofree gchar *trash_file_name = NULL;
  GFile *file;
  GFile *trash_file;
  gboolean success;

  GN_ENTRY;

  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (GN_IS_ITEM (item));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  file = g_object_get_data (G_OBJECT (item), "file");
  base_name = g_file_get_basename (file);
  trash_file_name = g_build_filename (self->trash_location, base_name, NULL);
  trash_file = g_file_new_for_path (trash_file_name);

  success = g_file_move (file, trash_file, G_FILE_COPY_NONE,
                         NULL, NULL, NULL, error);

  if (!success)
    GN_RETURN (success);

  g_object_set_data_full (G_OBJECT (item), "file", trash_file_name,
                          g_object_unref);
  self->notes = g_list_remove (self->notes, item);
  self->trash_notes = g_list_prepend (self->trash_notes, item);
  g_signal_emit_by_name (provider, "item-trashed", item);

  GN_RETURN (success);
}

static GList *
gn_local_provider_get_notes (GnProvider *provider)
{
  GN_ENTRY;

  g_assert (GN_IS_PROVIDER (provider));

  GN_RETURN (GN_LOCAL_PROVIDER (provider)->notes);
}

static GList *
gn_local_provider_get_trash_notes (GnProvider *provider)
{
  GN_ENTRY;

  g_assert (GN_IS_PROVIDER (provider));

  GN_RETURN (GN_LOCAL_PROVIDER (provider)->trash_notes);
}

static void
gn_local_provider_class_init (GnLocalProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GnProviderClass *provider_class = GN_PROVIDER_CLASS (klass);

  object_class->dispose = gn_local_provider_dispose;
  object_class->finalize = gn_local_provider_finalize;

  provider_class->get_uid = gn_local_provider_get_uid;
  provider_class->get_name = gn_local_provider_get_name;
  provider_class->get_icon = gn_local_provider_get_icon;
  provider_class->get_domain = gn_local_provider_get_domain;
  provider_class->get_user_name = gn_local_provider_get_user_name;
  provider_class->get_location_name = gn_local_provider_get_location_name;
  provider_class->get_notes = gn_local_provider_get_notes;
  provider_class->get_trash_notes = gn_local_provider_get_trash_notes;

  provider_class->load_items_async = gn_local_provider_load_items_async;
  provider_class->save_item_async = gn_local_provider_save_item_async;
  provider_class->save_item_finish = gn_local_provider_save_item_finish;
  provider_class->trash_item = gn_local_provider_trash_item;
}

static void
gn_local_provider_init (GnLocalProvider *self)
{
  if (self->location == NULL)
    {
      self->location = g_build_filename (g_get_user_data_dir (),
                                         "gnome-notes", NULL);
      self->trash_location = g_build_filename (self->location,
                                               ".Trash", NULL);
      g_mkdir_with_parents (self->location, 0755);
      g_mkdir_with_parents (self->trash_location, 0755);
    }
}

GnProvider *
gn_local_provider_new (void)
{
  GnLocalProvider *self;

  self = g_object_new (GN_TYPE_LOCAL_PROVIDER, NULL);

  return GN_PROVIDER (self);
}
