/* gn-enums.h
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

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  GN_VIEW_NOTES,
  GN_VIEW_EDITOR,
  GN_VIEW_NOTEBOOKS,
  GN_VIEW_NOTEBOOK_NOTES, /* Notes view after opening a Notebook */
  GN_VIEW_TRASH,
  GN_VIEW_SEARCH
} GnView;

typedef enum
{
  GN_VIEW_MODE_NORMAL,
  GN_VIEW_MODE_EMPTY,
  GN_VIEW_MODE_LOADING,
  GN_VIEW_MODE_DETACHED,
  GN_VIEW_MODE_SELECTION
} GnViewMode;

typedef enum
{
  GN_VIEW_TYPE_GRID,
  GN_VIEW_TYPE_LIST
} GnViewType;

typedef enum
{
  GN_FEATURE_NONE              = 0,
  GN_FEATURE_COLOR             = 1 <<  0,
  /* If note supports bold, italic, underline, strikethrough */
  GN_FEATURE_FORMAT            = 1 << 1,
  GN_FEATURE_TRASH             = 1 << 2,
  GN_FEATURE_NOTEBOOK          = 1 << 3,
  /* If provider supports a Note to be a part of atmost one notebook */
  GN_FEATURE_ISOLATED_NOTEBOOK = 1 << 4,
  GN_FEATURE_CREATION_DATE     = 1 << 5,
  GN_FEATURE_MODIFICATION_DATE = 1 << 6,
} GnFeature;

G_END_DECLS
