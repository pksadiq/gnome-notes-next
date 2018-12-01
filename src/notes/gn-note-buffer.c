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

  gint freeze_count;
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

  gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (buffer), TRUE);
}

static void
gn_note_buffer_insert_text (GtkTextBuffer *buffer,
                              GtkTextIter   *pos,
                              const gchar   *text,
                              gint           text_len)
{
  GnNoteBuffer *self = (GnNoteBuffer *)buffer;
  GtkTextIter end, start;
  gint start_offset = G_MAXINT;
  gboolean is_title = FALSE, is_buffer_end = FALSE;

  /*
   * TODO: Pasting title to content area may keep the boldness property,
   * which is not what we want. Check this behavior after Gtk4 port.
   */

  GN_ENTRY;

  if (self->freeze_count == 0)
    {
      is_title = gtk_text_iter_get_line (pos) == 0;
      start_offset = gtk_text_iter_get_offset (pos);
      is_buffer_end = gtk_text_iter_is_end (pos);

      if (is_title)
        {
          gtk_text_buffer_get_iter_at_line (buffer, &end, 1);
          gtk_text_buffer_remove_tag_by_name (buffer, "title", pos, &end);
        }
    }

  GTK_TEXT_BUFFER_CLASS (gn_note_buffer_parent_class)->insert_text (buffer,
                                                                      pos,
                                                                      text,
                                                                      text_len);

  if (self->freeze_count != 0)
    GN_EXIT;

  gtk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);
  end = *pos;

  if (start_offset == 0 || is_buffer_end)
    gtk_text_buffer_apply_tag (buffer, self->tag_font, &start, &end);

  if (is_title)
    {
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_iter_at_line (buffer, &end, 1);
      gtk_text_buffer_apply_tag_by_name (buffer, "title", &start, &end);
    }

  GN_EXIT;
}

static
void
gn_note_buffer_delete_range (GtkTextBuffer *buffer,
                             GtkTextIter   *start,
                             GtkTextIter   *end)
{
  GtkTextIter start_iter, end_iter;

  GTK_TEXT_BUFFER_CLASS (gn_note_buffer_parent_class)->delete_range (buffer, start, end);

  if (gtk_text_iter_get_line (start) == 0)
    {
      gtk_text_buffer_get_start_iter (buffer, &start_iter);
      gtk_text_buffer_get_iter_at_line (buffer, &end_iter, 1);
      gtk_text_buffer_apply_tag_by_name (buffer, "title", &start_iter, &end_iter);
      gtk_text_buffer_apply_tag_by_name (buffer, "font", &start_iter, &end_iter);
    }
}

static void
gn_note_buffer_real_apply_tag (GtkTextBuffer     *buffer,
                               GtkTextTag        *tag,
                               const GtkTextIter *start,
                               const GtkTextIter *end)
{
  GnNoteBuffer *self = (GnNoteBuffer *)buffer;

  GTK_TEXT_BUFFER_CLASS (gn_note_buffer_parent_class)->apply_tag (buffer, tag,
                                                                  start, end);

  /* We don't need anything other this tag handled by text-view undo */
  if (self->freeze_count > 0 ||
      (tag != self->tag_bold &&
       tag != self->tag_italic &&
       tag != self->tag_underline &&
       tag != self->tag_strike))
    g_signal_stop_emission_by_name (buffer, "apply-tag");
}

static void
gn_note_buffer_real_remove_tag (GtkTextBuffer     *buffer,
                                GtkTextTag        *tag,
                                const GtkTextIter *start,
                                const GtkTextIter *end)
{
  GnNoteBuffer *self = (GnNoteBuffer *)buffer;

  GTK_TEXT_BUFFER_CLASS (gn_note_buffer_parent_class)->remove_tag (buffer, tag,
                                                                   start, end);

  /* We don't need anything other this tag handled by text-view undo */
  if (self->freeze_count > 0 ||
      (tag != self->tag_bold &&
       tag != self->tag_italic &&
       tag != self->tag_underline &&
       tag != self->tag_strike))
    g_signal_stop_emission_by_name (buffer, "remove-tag");

}

static void
gn_note_buffer_constructed (GObject *object)
{
  GnNoteBuffer *self = (GnNoteBuffer *)object;
  GtkTextBuffer *buffer = (GtkTextBuffer *)object;
  GtkTextTag *tag;

  G_OBJECT_CLASS (gn_note_buffer_parent_class)->constructed (object);

  /*
   * The order of the tags defined here shouldn’t be changed
   * because when xml is generated (in GnXmlNote), the priority
   * of tags are taken as the order of tags, and the priority
   * of tags depend on the order they are defined.
   */
  tag = gtk_text_buffer_create_tag (buffer, "font", NULL);
  self->tag_font = tag;

  tag = gtk_text_buffer_create_tag (buffer, "title",
                                    "weight", PANGO_WEIGHT_BOLD,
                                    "pixels-below-lines", TITLE_SPACING,
                                    "scale", TITLE_SCALE,
                                    NULL);

  gtk_text_tag_set_priority (tag, 1);

  tag = gtk_text_buffer_create_tag (buffer, "b",
                                    "weight", PANGO_WEIGHT_BOLD,
                                    NULL);
  self->tag_bold = tag;

  tag = gtk_text_buffer_create_tag (buffer, "i",
                                    "style", PANGO_STYLE_ITALIC,
                                    NULL);
  self->tag_italic = tag;

  tag = gtk_text_buffer_create_tag (buffer, "s",
                                    "strikethrough", TRUE,
                                    NULL);
  self->tag_strike = tag;

  tag = gtk_text_buffer_create_tag (buffer, "u",
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
  text_buffer_class->delete_range = gn_note_buffer_delete_range;
  text_buffer_class->apply_tag = gn_note_buffer_real_apply_tag;
  text_buffer_class->remove_tag = gn_note_buffer_real_remove_tag;
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

  if (gtk_text_iter_equal (&start, &end))
    return;

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
    return "s";
  else if (tag == self->tag_font)
    return "";
  else
    g_return_val_if_reached ("span");
}

void
gn_note_buffer_freeze (GnNoteBuffer *self)
{
  g_return_if_fail (GN_IS_NOTE_BUFFER (self));

  self->freeze_count++;
}

void
gn_note_buffer_thaw (GnNoteBuffer *self)
{
  g_return_if_fail (GN_IS_NOTE_BUFFER (self));
  g_return_if_fail (self->freeze_count > 0);

  self->freeze_count--;
}
