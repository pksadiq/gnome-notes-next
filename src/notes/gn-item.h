/* gn-item.h
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
#include <gdk/gdk.h>

#include "gn-enums.h"

G_BEGIN_DECLS

#define GN_TYPE_ITEM (gn_item_get_type ())

G_DECLARE_DERIVABLE_TYPE (GnItem, gn_item, GN, ITEM, GObject)

struct _GnItemClass
{
  GObjectClass parent_class;

  gboolean (*is_modified)    (GnItem *self);
  void     (*unset_modified) (GnItem *self);
  gboolean (*match)          (GnItem *self,
                              const gchar *needle);
  GnFeature (*get_features)  (GnItem *self);
};

const gchar *gn_item_get_uid               (GnItem        *self);
void         gn_item_set_uid               (GnItem        *self,
                                            const gchar   *uid);

const gchar *gn_item_get_title             (GnItem        *self);
void         gn_item_set_title             (GnItem        *self,
                                            const gchar   *title);

gboolean     gn_item_get_rgba              (GnItem        *self,
                                            GdkRGBA       *color);
void         gn_item_set_rgba              (GnItem        *self,
                                            const GdkRGBA *color);

gint64       gn_item_get_creation_time     (GnItem        *self);
gint64       gn_item_get_modification_time (GnItem        *self);
gint64       gn_item_get_meta_modification_time (GnItem        *self);

gboolean     gn_item_is_modified           (GnItem        *self);
void         gn_item_unset_modified        (GnItem        *self);

gboolean     gn_item_is_new                (GnItem        *self);

gint         gn_item_compare               (gconstpointer a,
                                            gconstpointer b,
                                            gpointer      user_data);;
gboolean     gn_item_match                 (GnItem       *self,
                                            const gchar  *needle);
GnFeature    gn_item_get_features          (GnItem       *self);

G_END_DECLS
