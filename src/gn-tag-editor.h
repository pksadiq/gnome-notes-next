/* gn-tag-editor.h
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
#include <glib-object.h>

G_BEGIN_DECLS

#define GN_TYPE_TAG_EDITOR (gn_tag_editor_get_type ())

G_DECLARE_FINAL_TYPE (GnTagEditor, gn_tag_editor, GN, TAG_EDITOR, GtkDialog)

GtkWidget  *gn_tag_editor_new       (GtkWindow   *window);
void        gn_tag_editor_set_model (GnTagEditor *self,
                                     GListModel  *model);

G_END_DECLS
