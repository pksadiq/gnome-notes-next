/* note-buffer.c
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

#include <glib.h>

#include "notes/gn-item.h"
#include "notes/gn-note.h"
#include "notes/gn-plain-note.h"
#include "notes/gn-note-buffer.h"

void
test_note_buffer_empty (void)
{
  g_autoptr(GnNoteBuffer) buffer = NULL;
  GtkTextIter start, end;

  buffer = gn_note_buffer_new ();
  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer),
                              &start, &end);

  g_assert_true (gtk_text_iter_equal (&start, &end));
}

void
test_note_buffer_plain (void)
{
  g_autoptr(GnPlainNote) plain_note = NULL;
  GnNote *note;
  g_autofree gchar *content = NULL;
  g_autoptr(GtkTextBuffer) buffer = NULL;
  GtkTextTagTable *tag_table;
  GtkTextTag *title_tag;
  GtkTextIter start, end;

  plain_note = gn_plain_note_new_from_data ("Test\nContent");
  note = GN_NOTE (plain_note);
  buffer = gn_note_get_buffer (note);

  g_object_get (G_OBJECT (buffer), "text", &content, NULL);
  g_assert_cmpstr (content, ==, "Test\nContent");
  g_clear_pointer (&content, g_free);

  tag_table = gtk_text_buffer_get_tag_table (buffer);
  title_tag = gtk_text_tag_table_lookup (tag_table, "title");
  g_assert_nonnull (title_tag);

  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_iter_at_line_index (buffer, &end, 0, G_MAXINT);
  g_assert_true (gtk_text_iter_starts_tag (&start, title_tag));
  g_assert_true (gtk_text_iter_ends_tag (&end, title_tag));

  content = gtk_text_iter_get_text (&start, &end);
  g_assert_cmpstr (content, ==, "Test");
  g_clear_pointer (&content, g_free);

  gtk_text_buffer_set_text (buffer, "Random title 🐐", -1);

  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_iter_at_line_index (buffer, &end, 0, G_MAXINT);
  g_assert_true (gtk_text_iter_starts_tag (&start, title_tag));
  g_assert_true (gtk_text_iter_ends_tag (&end, title_tag));

  content = gtk_text_iter_get_text (&start, &end);
  g_assert_cmpstr (content, ==, "Random title 🐐");
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/note/buffer/empty", test_note_buffer_empty);
  g_test_add_func ("/note/buffer/plain", test_note_buffer_plain);

  return g_test_run ();
}
