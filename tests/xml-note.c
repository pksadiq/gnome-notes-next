/* xml-note.c
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

#include <errno.h>

#include "gn-xml-note.h"

struct Note
{
  gchar *file_content;
  gchar *title;
  gchar *text_content;
  gchar *raw_content;
  gchar *markup;
  GnFeature features;
  GdkRGBA rgba;
  gint64 creation_time;
  gint64 modification_time;
} test_note;

static void
test_xml_note_free (void)
{
  g_clear_pointer (&test_note.file_content, g_free);
  g_clear_pointer (&test_note.title, g_free);
  g_clear_pointer (&test_note.text_content, g_free);
  g_clear_pointer (&test_note.raw_content, g_free);
  g_clear_pointer (&test_note.markup, g_free);
}

static void
test_xml_note_empty (void)
{
  GnXmlNote *xml_note;
  GnNote *note;
  GnItem *item;
  const gchar *title;
  gchar *content;

  xml_note = gn_xml_note_new_from_data (NULL);
  g_assert_true (GN_IS_XML_NOTE (xml_note));
  item = GN_ITEM (xml_note);
  note = GN_NOTE (xml_note);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, "");

  content = gn_note_get_raw_content (note);
  g_assert_null (content);
  content = gn_note_get_text_content (note);
  g_assert_null (content);
  g_object_unref (xml_note);
}

static void
test_xml_note_update_content_from_file (const gchar *xml_file_name)
{
  g_auto(GStrv) split_content = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *file_name_prefix = NULL;
  g_autofree gchar *content = NULL;
  GDateTime *date_time;
  gchar *file_name;
  gchar *start, *end, *str;

  g_assert_true (g_str_has_suffix (xml_file_name, ".xml"));
  test_xml_note_free ();

  file_name_prefix = g_strdup (xml_file_name);
  end = strrchr (file_name_prefix, '.');
  *end = '\0';

  g_file_get_contents (xml_file_name, &test_note.file_content, NULL, &error);
  g_assert_no_error (error);

  start = strstr (test_note.file_content, "<last-change-date>");
  g_assert_true (start != NULL);
  start = start + strlen ("<last-change-date>");

  end = strstr (start, "</last-change-date>");
  g_assert_true (end != NULL);

  str = g_strndup (start, end - start);
  date_time = g_date_time_new_from_iso8601 (str, NULL);
  g_free (str);
  g_assert_true (date_time != NULL);

  test_note.modification_time = g_date_time_to_unix (date_time);
  g_date_time_unref (date_time);
  g_assert_cmpint (test_note.modification_time, >, 0);

  start = strstr (test_note.file_content, "<color>");
  if (start != NULL)
    {
      start = start + strlen ("<color>");

      end = strstr (start, "</color>");
      g_assert_true (end != NULL);

      str = g_strndup (start, end - start);
      g_assert_true (gdk_rgba_parse (&test_note.rgba, str));
    }

  file_name = g_strconcat (file_name_prefix, ".content", NULL);
  g_file_get_contents (file_name, &content, NULL, &error);
  g_free (file_name);
  g_assert_no_error (error);

  /* We shall have at most 2 parts: title and content */
  split_content = g_strsplit (content, "\n", 2);

  test_note.title = g_strdup (split_content[0]);
  if (test_note.title)
    test_note.text_content = g_strdup (split_content[1]);
}

static void
test_xml_note_parse (gconstpointer user_data)
{
  g_autoptr(GnXmlNote) xml_note = NULL;
  GnNote *note;
  GnItem *item;
  const gchar *title;
  gchar *content;
  GdkRGBA rgba;
  guint64 modification_time;

  test_xml_note_update_content_from_file (user_data);

  xml_note = gn_xml_note_new_from_data (test_note.file_content);
  g_assert_true (GN_IS_XML_NOTE (xml_note));
  item = GN_ITEM (xml_note);
  note = GN_NOTE (xml_note);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, test_note.title);

  content = gn_note_get_text_content (note);
  g_assert_cmpstr (content, ==, test_note.text_content);
  g_free (content);

  modification_time = gn_item_get_modification_time (item);
  g_assert_cmpint (modification_time, >, 0);
  g_assert_cmpint (modification_time, ==, test_note.modification_time);

  if (gn_item_get_rgba (item, &rgba))
    g_assert_true (gdk_rgba_equal (&test_note.rgba, &rgba));
}

static void
test_xml_note_update_markup_from_file (const gchar *markup_file_name)
{
  g_autoptr(GError) error = NULL;
  g_autofree gchar *file_name_prefix = NULL;
  g_autofree gchar *file_name = NULL;
  gchar *end;

  g_assert_true (g_str_has_suffix (markup_file_name, ".markup"));
  test_xml_note_free ();

  file_name_prefix = g_strdup (markup_file_name);
  end = strrchr (file_name_prefix, '.');
  *end = '\0';

  g_file_get_contents (markup_file_name, &test_note.markup, NULL, &error);
  g_assert_no_error (error);

  file_name = g_strconcat (file_name_prefix, ".xml", NULL);
  g_file_get_contents (file_name, &test_note.file_content, NULL, &error);
  g_assert_no_error (error);
}

static void
test_xml_note_markup (gconstpointer user_data)
{
  g_autoptr(GnXmlNote) xml_note = NULL;
  GnNote *note;
  g_autofree gchar *markup = NULL;

  test_xml_note_update_markup_from_file (user_data);

  xml_note = gn_xml_note_new_from_data (test_note.file_content);
  g_assert_true (GN_IS_XML_NOTE (xml_note));
  note = GN_NOTE (xml_note);

  markup = gn_note_get_markup (note);
  g_assert_cmpstr (markup, ==, test_note.markup);
}

int
main (int   argc,
      char *argv[])
{
  g_autoptr(GDir) dir = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *path = NULL;
  const gchar *file_name;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/note/xml/empty", test_xml_note_empty);

  path = g_test_build_filename (G_TEST_DIST, "xml-notes", NULL);
  dir = g_dir_open (path, 0, &error);
  g_assert_no_error (error);

  errno = 0;

  while ((file_name = g_dir_read_name (dir)) != NULL)
    {
      g_autofree gchar *test_path = NULL;

      if (!g_str_has_suffix (file_name, ".xml"))
        continue;

      test_path = g_strdup_printf ("/note/xml/parse/%s", file_name);
      g_test_add_data_func_full (test_path,
                                 g_build_filename (path, file_name, NULL),
                                 test_xml_note_parse,
                                 g_free);
    }

  g_assert_cmpint (errno, ==, 0);
  g_dir_rewind (dir);

  while ((file_name = g_dir_read_name (dir)) != NULL)
    {
      g_autofree gchar *test_path = NULL;

      if (!g_str_has_suffix (file_name, ".markup"))
        continue;

      test_path = g_strdup_printf ("/note/xml/markup/%s", file_name);
      g_test_add_data_func_full (test_path,
                                 g_build_filename (path, file_name, NULL),
                                 test_xml_note_markup,
                                 g_free);
    }

  g_assert_cmpint (errno, ==, 0);

  return g_test_run ();
}
