/* gn-application.h
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

#include "gn-settings.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GN_APPLICATION_DEFAULT() (GN_APPLICATION (g_application_get_default ()))
#define GN_TYPE_APPLICATION (gn_application_get_type ())

G_DECLARE_FINAL_TYPE (GnApplication, gn_application, GN, APPLICATION, GtkApplication)

GnApplication *gn_application_new (void);

G_END_DECLS
