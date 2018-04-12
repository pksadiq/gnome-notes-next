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

#include "gn-provider-item.h"
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

  gchar *uid;
  gchar *name;
  gchar *icon;
  gchar *domain;
  gchar *user_name;

  GFile *location;
  GFile *trash_location;

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

  g_clear_pointer (&self->uid, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->icon, g_free);
  g_clear_pointer (&self->domain, g_free);
  g_clear_pointer (&self->user_name, g_free);
  g_clear_object (&self->location);

  g_list_free_full (self->notes, g_object_unref);

  G_OBJECT_CLASS (gn_local_provider_parent_class)->finalize (object);

  GN_EXIT;
}

static gchar *
gn_local_provider_get_uid (GnProvider *provider)
{
  return g_strdup (GN_LOCAL_PROVIDER (provider)->uid);
}

static gchar *
gn_local_provider_get_name (GnProvider *provider)
{
  return g_strdup (GN_LOCAL_PROVIDER (provider)->name);
}

static gchar *
gn_local_provider_get_icon (GnProvider *provider)
{
  return g_strdup (GN_LOCAL_PROVIDER (provider)->icon);
}

static gchar *
gn_local_provider_get_domain (GnProvider *provider)
{
  return g_strdup (GN_LOCAL_PROVIDER (provider)->domain);
}

static gchar *
gn_local_provider_get_user_name (GnProvider *provider)
{
  return g_strdup (GN_LOCAL_PROVIDER (provider)->user_name);
}

static void
gn_local_provider_load_path (GnLocalProvider  *self,
                             GFile            *location,
                             GList           **items,
                             GCancellable     *cancellable,
                             GError          **error)
{
  g_autoptr(GFileEnumerator) enumerator = NULL;
  gpointer file_info_ptr;

  GN_ENTRY;

  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));
  g_assert (G_IS_FILE (location));

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
      GnProviderItem *provider_item;
      GnPlainNote *note;
      const gchar *name;

      name = g_file_info_get_name (file_info);

      if (!g_str_has_suffix (name, ".txt"))
        continue;

      file = g_file_get_child (location, name);
      g_file_load_contents (file, cancellable, &contents, NULL, NULL, NULL);

      note = gn_plain_note_new_from_data (contents);
      provider_item = gn_provider_item_new (GN_PROVIDER (self), GN_ITEM (note));
      *items = g_list_prepend (*items, provider_item);
    }
}

static gboolean
gn_local_provider_load_items (GnProvider    *provider,
                              GCancellable  *cancellable,
                              GError       **error)
{
  GnLocalProvider *self = (GnLocalProvider *)provider;

  GN_ENTRY;

  g_assert (GN_IS_LOCAL_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  gn_local_provider_load_path (self, self->location, &self->notes,
                               cancellable, error);
  if (*error != NULL)
    GN_RETURN (FALSE);

  gn_local_provider_load_path (self, self->trash_location,
                               &self->trash_notes,
                               cancellable, error);
  if (*error != NULL)
    GN_RETURN (FALSE);

  GN_RETURN (TRUE);
}

static GList *
gn_local_provider_get_notes (GnProvider *provider)
{
  GN_ENTRY;

  g_assert (GN_IS_PROVIDER (provider));

  GN_RETURN (GN_LOCAL_PROVIDER (provider)->notes);
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
  provider_class->get_notes = gn_local_provider_get_notes;

  provider_class->load_items = gn_local_provider_load_items;
}

static void
gn_local_provider_init (GnLocalProvider *self)
{
  self->uid = g_strdup ("local");

  if (self->location == NULL)
    {
      g_autofree gchar *path = NULL;

      path = g_build_filename (g_get_user_data_dir (), "gnome-notes", NULL);
      self->location = g_file_new_for_path (path);
      g_free (path);

      path = g_build_filename (g_get_user_data_dir (), "gnome-notes",
                               ".Trash", NULL);
      self->trash_location = g_file_new_for_path (path);
    }
}

GnLocalProvider *
gn_local_provider_new (void)
{
  return g_object_new (GN_TYPE_LOCAL_PROVIDER, NULL);
}
