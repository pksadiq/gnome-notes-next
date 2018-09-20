/* settings.c
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

#include "gn-settings.h"

static void
test_settings_fonts (void)
{
  g_autoptr(GSettings) desktop_settings = NULL;
  g_autoptr(GnSettings) settings = NULL;
  g_autofree gchar *system_font = NULL;
  const gchar *font;
  gboolean use_system_font;

  settings = gn_settings_new ("org.sadiqpk.notes");
  desktop_settings = g_settings_new ("org.gnome.desktop.interface");
  system_font = g_settings_get_string (desktop_settings,
                                       "document-font-name");
  g_assert (GN_IS_SETTINGS (settings));
  g_assert (G_IS_SETTINGS (desktop_settings));

  /* Unset any font set */
  gn_settings_set_font_name (settings, "");

  font = "Possibly invalid Font";
  g_assert_true (gn_settings_set_font_name (settings, font));
  g_object_get (G_OBJECT (settings),
                "use-system-font", &use_system_font, NULL);
  g_assert_false (use_system_font);
  font = gn_settings_get_font_name (settings);
  g_assert_cmpstr (font, ==, "Possibly invalid Font");

  g_object_set (G_OBJECT (settings), "use-system-font", FALSE, NULL);
  g_object_get (G_OBJECT (settings),
                "use-system-font", &use_system_font, NULL);
  g_assert_false (use_system_font);
  font = gn_settings_get_font_name (settings);
  g_assert_cmpstr (font, ==, "Possibly invalid Font");

  g_object_set (G_OBJECT (settings), "use-system-font", TRUE, NULL);
  g_object_get (G_OBJECT (settings),
                "use-system-font", &use_system_font, NULL);
  g_assert_true (use_system_font);
  font = gn_settings_get_font_name (settings);
  g_assert_cmpstr (font, ==, system_font);

  font = "New Font";
  g_object_set (G_OBJECT (settings), "font", font, NULL);
  font = gn_settings_get_font_name (settings);
  g_object_get (G_OBJECT (settings),
                "use-system-font", &use_system_font, NULL);
  g_assert_false (use_system_font);
  g_assert_cmpstr (font, ==, "New Font");

  /* Save the settings, create a new object, and check again */
  g_object_unref (settings);
  settings = gn_settings_new ("org.sadiqpk.notes");
  g_assert (GN_IS_SETTINGS (settings));

  font = gn_settings_get_font_name (settings);
  g_assert_cmpstr (font, ==, "New Font");

  g_object_get (G_OBJECT (settings),
                "use-system-font", &use_system_font, NULL);
  g_assert_false (use_system_font);
}

static void
test_settings_geometry (void)
{
  g_autoptr(GnSettings) settings = NULL;
  GdkRectangle geometry = {100, 200, 300, 400};
  GdkRectangle reset = {0, 0, 0, 0};
  GdkRectangle out;
  gboolean is_maximized;

  settings = gn_settings_new ("org.sadiqpk.notes");
  g_assert (GN_IS_SETTINGS (settings));

  gn_settings_set_window_maximized (settings, 0);
  g_assert_false (gn_settings_get_window_maximized (settings));

  gn_settings_set_window_maximized (settings, 1);
  g_assert_true (gn_settings_get_window_maximized (settings));

  /*
   * gbooleans are typedef to gint.  So test if non boolean
   * values are clamped.
   */
  gn_settings_set_window_maximized (settings, 100);
  is_maximized = gn_settings_get_window_maximized (settings);
  g_assert_cmpint (is_maximized, ==, 1);

  gn_settings_set_window_geometry (settings, &geometry);
  gn_settings_get_window_geometry (settings, &out);
  g_assert_cmpint (out.x, ==, geometry.x);
  g_assert_cmpint (out.y, ==, geometry.y);
  g_assert_cmpint (out.width, ==, geometry.width);
  g_assert_cmpint (out.height, ==, geometry.height);
  out = reset;

  /* Save the settings, create a new object, and check again */
  g_object_unref (settings);
  settings = gn_settings_new ("org.sadiqpk.notes");
  g_assert (GN_IS_SETTINGS (settings));

  is_maximized = gn_settings_get_window_maximized (settings);
  g_assert_cmpint (is_maximized, ==, 1);

  gn_settings_get_window_geometry (settings, &out);
  g_assert_cmpint (out.x, ==, geometry.x);
  g_assert_cmpint (out.y, ==, geometry.y);
  g_assert_cmpint (out.width, ==, geometry.width);
  g_assert_cmpint (out.height, ==, geometry.height);
}

static void
test_settings_first_run (void)
{
  g_autoptr(GnSettings) settings = NULL;

  settings = gn_settings_new ("org.sadiqpk.notes");
  g_assert (GN_IS_SETTINGS (settings));

  g_assert_true (gn_settings_get_is_first_run (settings));

  /* Save the settings, create a new object, and check again */
  g_object_unref (settings);
  settings = gn_settings_new ("org.sadiqpk.notes");
  g_assert (GN_IS_SETTINGS (settings));

  g_assert_false (gn_settings_get_is_first_run (settings));
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  /*
   * "/first_run/settings" should be the first test as it is supposed
   * to be changing only on the first run.  Changing this order will
   * result in test failure.
   */
  g_test_add_func ("/settings/first_run", test_settings_first_run);
  g_test_add_func ("/settings/geometry", test_settings_geometry);
  g_test_add_func ("/settings/fonts", test_settings_fonts);

  return g_test_run ();
}
