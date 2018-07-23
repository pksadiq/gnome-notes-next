/* gn-provider-row.h
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

#include "gn-provider.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GN_TYPE_PROVIDER_ROW (gn_provider_row_get_type ())

G_DECLARE_FINAL_TYPE (GnProviderRow, gn_provider_row, GN, PROVIDER_ROW, GtkListBoxRow)

GtkWidget  *gn_provider_row_new             (GnProvider    *provider);
void        gn_provider_row_set_selection   (GnProviderRow *self);
void        gn_provider_row_unset_selection (GnProviderRow *self,
                                             gpointer       user_data);
GnProvider *gn_provider_row_get_provider    (GnProviderRow *self);

G_END_DECLS
