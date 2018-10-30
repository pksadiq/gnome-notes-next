/* gn-plain-note.c
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

#define G_LOG_DOMAIN "gn-plain-note"

#include "config.h"

#include <gtk/gtk.h>

#include "gn-plain-note.h"
#include "gn-trace.h"

/**
 * SECTION: gn-plain-note
 * @title: GnPlainNote
 * @short_description:
 * @include: "gn-plain-note.h"
 */

struct _GnPlainNote
{
  GnNote parent_instance;

  gchar *content;
};

G_DEFINE_TYPE (GnPlainNote, gn_plain_note, GN_TYPE_NOTE)

static void
gn_plain_note_set_content_from_buffer (GnNote        *note,
                                       GtkTextBuffer *buffer)
{
  GnPlainNote *self = GN_PLAIN_NOTE (note);
  g_autofree gchar *full_content = NULL;
  gchar *title = NULL, *content = NULL;
  g_auto(GStrv) split_data;

  g_object_get (G_OBJECT (buffer), "text", &full_content, NULL);

  /* We shall have at most 2 parts: title and content */
  split_data = g_strsplit (full_content, "\n", 2);

  if (split_data[0] != NULL)
    {
      title = split_data[0];

      if (split_data[1] != NULL)
        content = split_data[1];
    }

  g_free (self->content);
  self->content = g_strdup (content);
  gn_item_set_title (GN_ITEM (note), title);
}

static void
gn_plain_note_finalize (GObject *object)
{
  GnPlainNote *self = (GnPlainNote *)object;

  GN_ENTRY;

  g_clear_pointer (&self->content, g_free);

  G_OBJECT_CLASS (gn_plain_note_parent_class)->finalize (object);

  GN_EXIT;
}

static gchar *
gn_plain_note_get_text_content (GnNote *note)
{
  GnPlainNote *self = GN_PLAIN_NOTE (note);

  g_assert (GN_IS_NOTE (note));

  return g_strdup (self->content);
}

static void
gn_plain_note_set_text_content (GnNote      *note,
                                const gchar *content)
{
  GnPlainNote *self = GN_PLAIN_NOTE (note);

  g_assert (GN_IS_PLAIN_NOTE (self));

  g_free (self->content);
  self->content = g_strdup (content);
}

static gchar *
gn_plain_note_get_markup (GnNote *note)
{
  GnPlainNote *self = GN_PLAIN_NOTE (note);
  const gchar *title = NULL;
  GString *markup;
  g_autofree gchar *title_markup = NULL;
  g_autofree gchar *content = NULL;

  g_assert (GN_IS_NOTE (note));

  title = gn_item_get_title (GN_ITEM (note));

  if (title[0] == '\0' && self->content == NULL)
    return NULL;

  markup = g_string_new_len (NULL, 10);

  if (title[0] != '\0')
    {
      title_markup = g_markup_escape_text (title, -1);
      g_string_append_printf (markup, "<b>%s</b>", title_markup);
    }

  if (self->content != NULL)
    {
      content = g_markup_escape_text (self->content, -1);
      g_string_append_printf (markup,"\n\n%s", content);
    }

  return g_string_free (markup, FALSE);
}

gboolean
gn_plain_note_match (GnItem      *item,
                     const gchar *needle)
{
  GnPlainNote *self = GN_PLAIN_NOTE (item);
  g_autofree gchar *content = NULL;
  gboolean match;

  match = GN_ITEM_CLASS (gn_plain_note_parent_class)->match (item, needle);

  if (match)
    return TRUE;

  content = g_utf8_casefold (self->content, -1);

  if (strstr (content, needle) != NULL)
    return TRUE;

  return FALSE;
}

static void
gn_plain_note_class_init (GnPlainNoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GnNoteClass *note_class = GN_NOTE_CLASS (klass);
  GnItemClass *item_class = GN_ITEM_CLASS (klass);

  object_class->finalize = gn_plain_note_finalize;

  note_class->get_raw_content = gn_plain_note_get_text_content;
  note_class->get_text_content = gn_plain_note_get_text_content;
  note_class->set_text_content = gn_plain_note_set_text_content;
  note_class->get_markup = gn_plain_note_get_markup;

  note_class->set_content_from_buffer = gn_plain_note_set_content_from_buffer;

  item_class->match = gn_plain_note_match;
}

static void
gn_plain_note_init (GnPlainNote *self)
{
}


static GnPlainNote *
gn_plain_note_create_from_data (const gchar *data)
{
  g_auto(GStrv) split_data = NULL;
  gchar *title = NULL, *content = NULL;
  GnPlainNote *note;

  g_assert (data != NULL);

  /* We shall have at most 2 parts: title and content */
  split_data = g_strsplit (data, "\n", 2);

  if (split_data[0] != NULL)
    {
      title = split_data[0];

      if (split_data[1] != NULL)
        content = split_data[1];
    }

  note = g_object_new (GN_TYPE_PLAIN_NOTE,
                       "title", title,
                       NULL);

  note->content = g_strdup (content);

  return note;
}

/**
 * gn_plain_note_new_from_data:
 * @data (nullable): The raw note content
 *
 * Create a new plain note from the given data.
 *
 * Returns: (transfer full): a new #GnPlainNote
 * with given content.
 */
GnPlainNote *
gn_plain_note_new_from_data (const gchar *data)
{
  if (data == NULL)
    return g_object_new (GN_TYPE_PLAIN_NOTE, NULL);

  return gn_plain_note_create_from_data (data);
}
