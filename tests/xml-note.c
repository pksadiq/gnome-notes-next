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
test_xml_note_update_from_file (const gchar *xml_file_name)
{
  g_autoptr(GError) error = NULL;
  g_autofree gchar *file_name_prefix = NULL;
  gchar *file_name;
  gchar *end;

  g_assert_true (g_str_has_suffix (xml_file_name, ".xml"));
  test_xml_note_free ();

  file_name_prefix = g_strdup (xml_file_name);
  end = strrchr (file_name_prefix, '.');
  *end = '\0';

  g_file_get_contents (xml_file_name, &test_note.file_content, NULL, &error);
  g_assert_no_error (error);

  file_name = g_strconcat (file_name_prefix, ".title", NULL);
  g_file_get_contents (file_name, &test_note.title, NULL, &error);
  g_free (file_name);
  g_assert_no_error (error);

  file_name = g_strconcat (file_name_prefix, ".content", NULL);
  g_file_get_contents (file_name, &test_note.text_content, NULL, &error);
  g_free (file_name);
  g_assert_no_error (error);
}

static void
test_xml_note_parse (gconstpointer user_data)
{
  GnXmlNote *xml_note;
  GnNote *note;
  GnItem *item;
  const gchar *title;
  gchar *content;

  test_xml_note_update_from_file (user_data);

  xml_note = gn_xml_note_new_from_data (test_note.file_content);
  g_assert_true (GN_IS_XML_NOTE (xml_note));
  item = GN_ITEM (xml_note);
  note = GN_NOTE (xml_note);

  title = gn_item_get_title (item);
  g_assert_cmpstr (title, ==, test_note.title);

  content = gn_note_get_text_content (note);
  g_assert_cmpstr (content, ==, test_note.text_content);
  g_free (content);
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

  path = g_test_build_filename (G_TEST_DIST, "xml-note", NULL);
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
                                 g_test_build_filename (G_TEST_DIST, "xml-note",
                                                        file_name, NULL),
                                 test_xml_note_parse,
                                 g_free);
    }

  g_assert_cmpint (errno, ==, 0);

  return g_test_run ();
}
