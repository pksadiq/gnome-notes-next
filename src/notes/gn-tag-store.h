/* gn-tag-store.h
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

#include <gdk/gdk.h>

G_BEGIN_DECLS

typedef struct _GnTagStore GnTagStore;

#define GN_TYPE_TAG (gn_tag_get_type ())

G_DECLARE_FINAL_TYPE (GnTag, gn_tag, GN, TAG, GObject)

GnTagStore  *gn_tag_store_new           (void);
void         gn_tag_store_free          (GnTagStore  *self);
GListModel  *gn_tag_store_get_model     (GnTagStore  *self);

GnTag       *gn_tag_store_insert        (GnTagStore  *self,
                                         const gchar *name,
                                         GdkRGBA     *rgba);

const gchar *gn_tag_get_name            (GnTag       *tag);
gboolean     gn_tag_get_rgba            (GnTag       *tag,
                                         GdkRGBA     *rgba);
gint         gn_tag_compare             (gconstpointer a,
                                         gconstpointer b,
                                         gpointer      user_data);

G_END_DECLS
