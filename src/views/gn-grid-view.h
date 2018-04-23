/* gn-grid-view.h
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

#define GN_TYPE_GRID_VIEW (gn_grid_view_get_type ())

G_DECLARE_FINAL_TYPE (GnGridView, gn_grid_view, GN, GRID_VIEW, GtkFlowBox)

GnGridView *gn_grid_view_new          (void);
void        gn_grid_view_unselect_all (GtkFlowBox *box);
GList      *gn_grid_view_get_selected_items (GnGridView *self);

G_END_DECLS
