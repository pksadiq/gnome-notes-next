/* gn-utils.h
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

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GN_IS_MAIN_THREAD() (g_thread_self () == gn_utils_get_main_thread ())

GThread    *gn_utils_get_main_thread    (void);
gchar      *gn_utils_get_markup_from_bijiben (const gchar *xml,
                                              gint         max_line);
gchar      *gn_utils_get_text_from_xml       (const gchar *xml);
const gchar *gn_utils_get_other_view_type    (const gchar *view);
gboolean     gn_utils_get_item_position      (GListModel *model,
                                              gpointer    item,
                                              guint      *position);
gchar       *gn_utils_unix_time_to_iso       (gint64      unix_time);
gchar       *gn_utils_get_human_time         (gint64      unix_time);

G_END_DECLS
