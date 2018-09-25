/* gn-main-view.h
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

#include "gn-enums.h"

G_BEGIN_DECLS

#define GN_TYPE_MAIN_VIEW (gn_main_view_get_type ())

G_DECLARE_FINAL_TYPE (GnMainView, gn_main_view, GN, MAIN_VIEW, GtkStack)

GnMainView *gn_main_view_new                (void);
gboolean    gn_main_view_get_selection_mode (GnMainView *self);
void        gn_main_view_set_selection_mode (GnMainView *self,
                                             gboolean    selection_mode);
GList      *gn_main_view_get_selected_items (GnMainView *self);
gboolean    gn_main_view_set_model          (GnMainView *self,
                                             GListModel *model);
void        gn_main_view_set_view           (GnMainView *self,
                                             GnViewType  view_type);

G_END_DECLS
