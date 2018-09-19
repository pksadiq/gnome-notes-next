/* gn-settings.h
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GN_TYPE_SETTINGS (gn_settings_get_type ())

G_DECLARE_FINAL_TYPE (GnSettings, gn_settings, GN, SETTINGS, GSettings)

GnSettings *gn_settings_new (const gchar *schema_id);

gboolean    gn_settings_get_window_maximized (GnSettings *self);
void        gn_settings_set_window_maximized (GnSettings *self,
                                              gboolean    maximized);
void        gn_settings_get_window_geometry  (GnSettings   *self,
                                              GdkRectangle *geometry);
void        gn_settings_set_window_geometry  (GnSettings   *self,
                                              GdkRectangle *geometry);

void        gn_settings_get_rgba             (GnSettings    *self,
                                              GdkRGBA       *rgba);
void        gn_settings_set_rgba             (GnSettings    *self,
                                              const GdkRGBA *rgba);
const gchar *gn_settings_get_provider_name   (GnSettings    *self);
gboolean     gn_settings_set_provider_name   (GnSettings    *self,
                                              const gchar   *name);
const gchar *gn_settings_get_font_name       (GnSettings    *self);
gboolean     gn_settings_set_font_name       (GnSettings    *self,
                                              const gchar   *name);

G_END_DECLS
