/* plain-note.c
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

#include "gn-plain-note.h"

static void
test_plain_note_with_change (GnPlainNote *plain_note)
{
  GnItem *item;
  GdkRGBA rgba;
  GdkRGBA new_rgba;
  const gchar *uid;
  const gchar *title;
  gboolean has_color;

  g_assert (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);

  gn_item_set_uid (item, "test-uid");
  uid = gn_item_get_uid (item);
  g_assert_cmpstr (uid, ==, "test-uid");

  g_assert_true (gn_item_is_modified (item));
  gn_item_unset_modified (item);
  g_assert_false (gn_item_is_modified (item));

  gn_item_set_title (item, "test title");
  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "test title");

  g_assert_true (gn_item_is_modified (item));
  gn_item_unset_modified (item);
  g_assert_false (gn_item_is_modified (item));

  /*
   * FIXME: plain notes won’t have color feature.
   * This exists just to satisfy evolution memos
   * which have color feature, and are plain notes.
   * This feature may better fit in provider class.
   */
  gdk_rgba_parse (&rgba, "#123");
  gn_item_set_rgba (item, &rgba);
  has_color = gn_item_get_rgba (item, &new_rgba);
  g_assert_true (has_color);
  g_assert (gdk_rgba_equal (&rgba, &new_rgba));

  g_assert_true (gn_item_is_modified (item));
  gn_item_unset_modified (item);
  g_assert_false (gn_item_is_modified (item));

  /* Setting the same color shouldn’t change anything */
  gn_item_set_rgba (item, &rgba);
  has_color = gn_item_get_rgba (item, &new_rgba);
  g_assert_true (has_color);
  g_assert (gdk_rgba_equal (&rgba, &new_rgba));
  g_assert_false (gn_item_is_modified (item));
}

static void
test_plain_note_empty (void)
{
  g_autoptr(GnPlainNote) plain_note = NULL;
  GnItem *item;
  GdkRGBA rgba;
  const gchar *uid;
  const gchar *title;
  gboolean has_color;

  plain_note = gn_plain_note_new_from_data (NULL, 0);
  g_assert (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);

  uid = gn_item_get_uid (item);
  g_assert_null (uid);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "");

  has_color = gn_item_get_rgba (item, &rgba);
  g_assert_false (has_color);

  g_assert_true (gn_item_is_new (item));
  g_assert_false (gn_item_is_modified (item));
  g_assert_true (gn_item_get_features (item) == GN_FEATURE_NONE);

  test_plain_note_with_change (plain_note);
}

static void
test_plain_note_new (void)
{
  GnPlainNote *plain_note;
  GnNote *note;
  GnItem *item;
  const gchar *title;
  gchar *content;

  plain_note = gn_plain_note_new_from_data (NULL, 0);
  g_assert_true (GN_IS_PLAIN_NOTE (plain_note));
  item = GN_ITEM (plain_note);
  note = GN_NOTE (plain_note);

  g_assert (GN_IS_ITEM (item));
  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "");

  content = gn_note_get_raw_content (note);
  g_assert_null (content);
  content = gn_note_get_text_content (note);
  g_assert_null (content);
  g_object_unref (plain_note);

  plain_note = gn_plain_note_new_from_data ("", 0);
  g_assert_true (GN_IS_PLAIN_NOTE (plain_note));
  item = GN_ITEM (plain_note);
  note = GN_NOTE (plain_note);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "");

  content = gn_note_get_raw_content (note);
  g_assert_null (content);
  content = gn_note_get_text_content (note);
  g_assert_null (content);
  g_object_unref (plain_note);
}

static void
test_plain_note_title (void)
{
  g_autoptr(GnPlainNote) plain_note = NULL;
  GnNote *note;
  GnItem *item;
  const gchar *uid;
  const gchar *title;
  g_autofree gchar *content = NULL;

  plain_note = gn_plain_note_new_from_data ("Some Randomly long test 😊", -1);
  g_assert (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);
  note = GN_NOTE (plain_note);

  uid = gn_item_get_uid (item);
  g_assert_null (uid);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "Some Randomly long test 😊");

  content = gn_note_get_raw_content (note);
  g_assert_null (content);

  test_plain_note_with_change (plain_note);
}

static void
test_plain_note_content (void)
{
  g_autoptr(GnPlainNote) plain_note = NULL;
  GnNote *note;
  GnItem *item;
  const gchar *uid;
  const gchar *title;
  gchar *content;

  plain_note = gn_plain_note_new_from_data ("Some Randomly\nlong test 😊", -1);
  g_assert (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);
  note = GN_NOTE (plain_note);

  uid = gn_item_get_uid (item);
  g_assert_null (uid);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "Some Randomly");

  content = gn_note_get_raw_content (note);
  g_assert_cmpstr (content, ==, "long test 😊");
  g_free (content);

  content = gn_note_get_text_content (note);
  g_assert_cmpstr (content, ==, "long test 😊");
  g_free (content);

  test_plain_note_with_change (plain_note);
}

static void
test_plain_note_buffer (void)
{
  g_autoptr(GnPlainNote) plain_note = NULL;
  g_autoptr(GtkTextBuffer) buffer = NULL;
  GnNote *note;
  GnItem *item;
  const gchar *title;
  g_autofree gchar *content = NULL;

  plain_note = gn_plain_note_new_from_data (NULL, 0);
  g_assert_true (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);
  note = GN_NOTE (plain_note);
  buffer = gtk_text_buffer_new (NULL);

  gn_note_set_content_from_buffer (note, buffer);
  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "");
  content = gn_note_get_raw_content (note);
  g_assert_null (content);

  gtk_text_buffer_set_text (buffer, "Title \t only", -1);
  gn_note_set_content_from_buffer (note, buffer);
  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "Title \t only");
  content = gn_note_get_raw_content (note);
  g_assert_null (content);

  gtk_text_buffer_set_text (buffer, "Title\nand content", -1);
  gn_note_set_content_from_buffer (note, buffer);
  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "Title");
  content = gn_note_get_raw_content (note);
  g_assert_cmpstr (content, ==, "and content");
}

static void
test_plain_note_markup (void)
{
  g_autoptr(GnPlainNote) plain_note = NULL;
  GnNote *note;
  GnItem *item;
  gchar *markup;

  plain_note = gn_plain_note_new_from_data (NULL, 0);
  g_assert_true (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);
  note = GN_NOTE (plain_note);

  markup = gn_note_get_markup (note);
  g_assert_null (markup);

  gn_item_set_title (item, "<html> tag & no content");
  markup = gn_note_get_markup (note);
  g_assert_cmpstr (markup, ==, "<b>&lt;html&gt; tag &amp; no content</b>");
  g_free (markup);

  gn_note_set_text_content (note, "\" It doesn't have <tag> \"");
  markup = gn_note_get_markup (note);
  g_assert_cmpstr (markup, ==,
                   "<b>&lt;html&gt; tag &amp; no content</b>\n\n"
                   "&quot; It doesn&apos;t have &lt;tag&gt; &quot;");
  g_free (markup);

  gn_item_set_title (item, "");
  markup = gn_note_get_markup (note);
  g_assert_cmpstr (markup, ==,
                   "\n\n&quot; It doesn&apos;t have &lt;tag&gt; &quot;");
  g_free (markup);
}

static void
test_plain_note_search (void)
{
  g_autoptr(GnPlainNote) plain_note = NULL;
  GnNote *note;
  GnItem *item;

  plain_note = gn_plain_note_new_from_data ("Some Randomly\nlong test 😊", -1);
  g_assert_true (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);
  note = GN_NOTE (plain_note);

  g_assert_true (gn_item_match (item, "Some"));
  g_assert_true (gn_item_match (item, "some"));
  g_assert_true (gn_item_match (item, "soME"));
  g_assert_true (gn_item_match (item, "long test"));
  g_assert_false (gn_item_match (item, "invalid"));

  gn_item_set_title (item, "ഒരു തലക്കെട്ടു");
  g_assert_true (gn_item_match (item, "തല"));

  gn_item_set_title (item, "Русский");
  g_assert_true (gn_item_match (item, "руссКИЙ"));

  gn_note_set_text_content (note, "ß ഉള്ളടക്കം");
  g_assert_true (gn_item_match (item, "руссКИЙ"));
  g_assert_true (gn_item_match (item, "ss"));
  g_assert_true (gn_item_match (item, "ഉള"));
  g_assert_false (gn_item_match (item, "ഉള്ളി"));
}

static void
test_plain_note_time (void)
{
  g_autoptr(GnPlainNote) plain_note = NULL;
  GnItem *item;

  plain_note = gn_plain_note_new_from_data (NULL, 0);
  g_assert_true (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);

  g_assert_cmpint (gn_item_get_creation_time (item), ==, 0);
  g_assert_cmpint (gn_item_get_modification_time (item), ==, 0);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/note/plain/empty", test_plain_note_empty);
  g_test_add_func ("/note/plain/new", test_plain_note_new);
  g_test_add_func ("/note/plain/title", test_plain_note_title);
  g_test_add_func ("/note/plain/content", test_plain_note_content);
  g_test_add_func ("/note/plain/buffer", test_plain_note_buffer);
  g_test_add_func ("/note/plain/markup", test_plain_note_markup);
  g_test_add_func ("/note/plain/search", test_plain_note_search);
  g_test_add_func ("/note/plain/time", test_plain_note_time);

  return g_test_run ();
}
