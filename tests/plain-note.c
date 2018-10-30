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

  gn_item_set_title (item, "test title");
  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "test title");

  gdk_rgba_parse (&rgba, "#123");
  gn_item_set_rgba (item, &rgba);
  has_color = gn_item_get_rgba (item, &new_rgba);
  g_assert_true (has_color);
  g_assert (gdk_rgba_equal (&rgba, &new_rgba));

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

  plain_note = gn_plain_note_new_from_data (NULL);
  g_assert (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);

  uid = gn_item_get_uid (item);
  g_assert_null (uid);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "");

  has_color = gn_item_get_rgba (item, &rgba);
  g_assert_false (has_color);

  g_assert_true (gn_item_is_new (item));

  test_plain_note_with_change (plain_note);
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

  plain_note = gn_plain_note_new_from_data ("Some Randomly long test 😊");
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
  g_autofree gchar *content = NULL;

  plain_note = gn_plain_note_new_from_data ("Some Randomly\nlong test 😊");
  g_assert (GN_IS_PLAIN_NOTE (plain_note));

  item = GN_ITEM (plain_note);
  note = GN_NOTE (plain_note);

  uid = gn_item_get_uid (item);
  g_assert_null (uid);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "Some Randomly");

  content = gn_note_get_raw_content (note);
  g_assert_cmpstr (content, ==, "long test 😊");

  test_plain_note_with_change (plain_note);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/note/plain/empty", test_plain_note_empty);
  g_test_add_func ("/note/plain/title", test_plain_note_title);
  g_test_add_func ("/note/plain/content", test_plain_note_content);

  return g_test_run ();
}
