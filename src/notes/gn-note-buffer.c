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

  GtkTextTag *tag_font;
  GtkTextTag *tag_bold;
  GtkTextTag *tag_italic;
  GtkTextTag *tag_underline;
  GtkTextTag *tag_strike;
};

G_DEFINE_TYPE (GnNoteBuffer, gn_note_buffer, GTK_TYPE_TEXT_BUFFER)

static void
gn_note_buffer_exclude_title (GnNoteBuffer *self,
                              GtkTextIter  *start,
                              GtkTextIter  *end)
{
  g_assert (GN_IS_NOTE_BUFFER (self));
  g_assert (start != NULL && end != NULL);

  if (gtk_text_iter_get_line (start) > 0)
    return;

  if (gtk_text_iter_get_line (end) > 0)
    gtk_text_iter_forward_line (start);
  else
    *start = *end;
}

static void
gn_note_buffer_toggle_tag (GnNoteBuffer *buffer,
                           GtkTextTag   *tag,
                           GtkTextIter  *start,
                           GtkTextIter  *end)
{
  gboolean has_tag;

  g_assert (GN_IS_NOTE_BUFFER (buffer));
  g_assert (GTK_IS_TEXT_TAG (tag));
  g_assert (start != NULL && end != NULL);

  /*
   * If tag isn't present in either start or end,
   * we can safely apply tag.
   * FIXME: This is really a lame way of checking tag.
   * The selection starts and end with some tag doesn't
   * mean that every character in the selection has
   * the tag applied.
   */
  has_tag = gtk_text_iter_has_tag (start, tag) &&
    (gtk_text_iter_has_tag (end, tag) ||
     gtk_text_iter_ends_tag (end, tag));

  if (has_tag)
    gtk_text_buffer_remove_tag (GTK_TEXT_BUFFER (buffer), tag,
                                start, end);
  else
    gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER (buffer), tag,
                               start, end);
}

static void
gn_note_buffer_insert_text (GtkTextBuffer *buffer,
                              GtkTextIter   *pos,
                              const gchar   *text,
                              gint           text_len)
{
  GnNoteBuffer *self = (GnNoteBuffer *)buffer;
  GtkTextIter end, start;
  gboolean is_title;
  gboolean reset_font;

  /*
   * TODO: Pasting title to content area may keep the boldness property,
   * which is not what we want. Check this behavior after Gtk4 port.
   */

  GN_ENTRY;

  is_title = gtk_text_iter_get_line (pos) == 0;

  /*
   * If @pos doesn't have the tag tag_font, that means the newly
   * inserted text is either appended or prepended to the buffer.
   * If that's the case, we have to reapply default font.
   */
  reset_font = !gtk_text_iter_has_tag (pos, self->tag_font);

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
      gtk_text_buffer_apply_tag_by_name (buffer, "font", &start, &end);
    }

  if (reset_font && !is_title)
    {
      gtk_text_buffer_get_end_iter (buffer, &end);
      start = end;

      if (gtk_text_iter_backward_to_tag_toggle (&start, self->tag_font))
        gtk_text_buffer_apply_tag_by_name (buffer, "font", &start, &end);
    }

  GN_EXIT;
}

static void
gn_note_buffer_constructed (GObject *object)
{
  GnNoteBuffer *self = (GnNoteBuffer *)object;
  GtkTextBuffer *buffer = (GtkTextBuffer *)object;
  GtkTextTag *tag;

  G_OBJECT_CLASS (gn_note_buffer_parent_class)->constructed (object);

  gtk_text_buffer_create_tag (buffer, "title",
                              "weight", PANGO_WEIGHT_BOLD,
                              "pixels-below-lines", TITLE_SPACING,
                              "scale", TITLE_SCALE,
                              NULL);

  tag = gtk_text_buffer_create_tag (buffer, "font", NULL);
  self->tag_font = tag;

  tag = gtk_text_buffer_create_tag (buffer, "bold",
                                    "weight", PANGO_WEIGHT_BOLD,
                                    NULL);
  self->tag_bold = tag;

  tag = gtk_text_buffer_create_tag (buffer, "italic",
                                    "style", PANGO_STYLE_ITALIC,
                                    NULL);
  self->tag_italic = tag;

  tag = gtk_text_buffer_create_tag (buffer, "strike",
                                    "strikethrough", TRUE,
                                    NULL);
  self->tag_strike = tag;

  tag = gtk_text_buffer_create_tag (buffer, "underline",
                                    "underline", PANGO_UNDERLINE_SINGLE,
                                    NULL);
  self->tag_underline = tag;
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

/**
 * gn_note_buffer_apply_tag:
 * @self: A #GnNoteBuffer
 * @tag_name: A valid tag name
 *
 * @tag_name can be one of "bold", "italic",
 * "underline", or "strikethrough".
 *
 * The tag is applied to the selected text in
 * @self.
 */
void
gn_note_buffer_apply_tag (GnNoteBuffer *self,
                          const gchar  *tag_name)
{
  GtkTextIter start, end;
  GtkTextTag *tag;

  g_return_if_fail (GN_IS_NOTE_BUFFER (self));
  g_return_if_fail (gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (self)));
  g_return_if_fail (tag_name != NULL);

  gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (self), &start, &end);
  gn_note_buffer_exclude_title (self, &start, &end);

  if (strcmp (tag_name, "bold") == 0)
    tag = self->tag_bold;
  else if (strcmp (tag_name, "italic") == 0)
    tag = self->tag_italic;
  else if (strcmp (tag_name, "underline") == 0)
    tag = self->tag_underline;
  else if (strcmp (tag_name, "strikethrough") == 0)
    tag = self->tag_strike;
  else
    g_return_if_reached ();

  gn_note_buffer_toggle_tag (self, tag, &start, &end);
}

void
gn_note_buffer_remove_all_tags (GnNoteBuffer *self)
{
  GtkTextIter start, end;

  g_return_if_fail (GN_IS_NOTE_BUFFER (self));

  gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (self), &start, &end);
  gn_note_buffer_exclude_title (self, &start, &end);

  if (gtk_text_iter_equal (&start, &end))
    return;

  gtk_text_buffer_remove_all_tags (GTK_TEXT_BUFFER (self), &start, &end);
  gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (self), TRUE);
}

const gchar *
gn_note_buffer_get_name_for_tag (GnNoteBuffer *self,
                                 GtkTextTag   *tag)
{
  g_return_val_if_fail (GN_IS_NOTE_BUFFER (self), NULL);
  g_return_val_if_fail (tag != NULL, NULL);

  if (tag == self->tag_bold)
    return "b";
  else if (tag == self->tag_italic)
    return "i";
  else if (tag == self->tag_underline)
    return "u";
  else if (tag == self->tag_strike)
    return "strike";
  else
    g_return_val_if_reached ("span");
}
