/* gn-memo-provider.h
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
#include <libedataserver/libedataserver.h>
#include <libecal/libecal.h>

#include "gn-provider.h"

G_BEGIN_DECLS

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ESource, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ECalClient, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ECalComponent, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ECalComponentId, e_cal_component_free_id)

#define GN_TYPE_MEMO_PROVIDER (gn_memo_provider_get_type ())

G_DECLARE_FINAL_TYPE (GnMemoProvider, gn_memo_provider, GN, MEMO_PROVIDER, GnProvider)

GnMemoProvider *gn_memo_provider_new (ESource *source);

G_END_DECLS
