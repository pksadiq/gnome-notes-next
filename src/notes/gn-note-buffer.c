/* gn-note-buffer.c
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

#define G_LOG_DOMAIN "gn-note-buffer"

#include "config.h"

#include "gn-note-buffer.h"
#include "gn-trace.h"

/**
 * SECTION: gn-note-buffer
 * @title: GnNoteBuffer
 * @short_description: A TextBuffer to handle notes
 * @include: "gn-note-buffer.h"
 */

#define TITLE_SPACING 12
#define TITLE_SCALE   1.2

struct _GnNoteBuffer
{
  GtkTextBuffer parent_instance;
};

G_DEFINE_TYPE (GnNoteBuffer, gn_note_buffer, GTK_TYPE_TEXT_BUFFER)

static void
gn_note_buffer_insert_text (GtkTextBuffer *buffer,
                              GtkTextIter   *pos,
                              const gchar   *text,
                              gint           text_len)
{
  GtkTextIter end, start;
  gboolean is_title;

  /*
   * TODO: Pasting title to content area may keep the boldness property,
   * which is not what we want. Check this behavior after Gtk4 port.
   */

  GN_ENTRY;

  is_title = gtk_text_iter_get_line (pos) == 0;

  if (is_title)
    {
      gtk_text_buffer_get_iter_at_line_index (buffer, &end, 0, G_MAXINT);
      gtk_text_buffer_remove_tag_by_name (buffer, "title", pos, &end);
    }

  GTK_TEXT_BUFFER_CLASS (gn_note_buffer_parent_class)->insert_text (buffer,
                                                                      pos,
                                                                      text,
                                                                      text_len);
  if (is_title)
    {
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_iter_at_line_index (buffer, &end, 0, G_MAXINT);
      gtk_text_buffer_apply_tag_by_name (buffer, "title", &start, &end);
    }

  GN_EXIT;
}

static void
gn_note_buffer_constructed (GObject *object)
{
  GnNoteBuffer *self = (GnNoteBuffer *)object;

  gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (self), "title",
                              "weight", PANGO_WEIGHT_BOLD,
                              "pixels-below-lines", TITLE_SPACING,
                              "scale", TITLE_SCALE,
                              NULL);
}

static void
gn_note_buffer_class_init (GnNoteBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkTextBufferClass *text_buffer_class = GTK_TEXT_BUFFER_CLASS (klass);

  object_class->constructed = gn_note_buffer_constructed;

  text_buffer_class->insert_text = gn_note_buffer_insert_text;
}

static void
gn_note_buffer_init (GnNoteBuffer *self)
{
}

GnNoteBuffer *
gn_note_buffer_new (void)
{
  return g_object_new (GN_TYPE_NOTE_BUFFER,
                       NULL);
}
