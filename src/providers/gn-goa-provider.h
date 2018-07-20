/* gn-goa-provider.h
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

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>

#include "gn-provider.h"

G_BEGIN_DECLS

#define GN_TYPE_GOA_PROVIDER (gn_goa_provider_get_type ())

G_DECLARE_FINAL_TYPE (GnGoaProvider, gn_goa_provider, GN, GOA_PROVIDER, GnProvider)

GnGoaProvider *gn_goa_provider_new (GoaObject *object);

G_END_DECLS
