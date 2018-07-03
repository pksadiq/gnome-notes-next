/* gn-provider.h
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
#include <gio/gio.h>

#include "gn-item.h"

G_BEGIN_DECLS

#define GN_TYPE_PROVIDER (gn_provider_get_type ())

G_DECLARE_DERIVABLE_TYPE (GnProvider, gn_provider, GN, PROVIDER, GObject)

struct _GnProviderClass
{
  GObjectClass parent_class;

  gboolean     (*load_items)           (GnProvider          *self,
                                        GCancellable        *cancellable,
                                        GError             **error);
  void         (*load_items_async)     (GnProvider           *self,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*load_items_finish)    (GnProvider           *self,
                                        GAsyncResult         *result,
                                        GError              **error);

  void         (*save_item_async)      (GnProvider           *self,
                                        GnItem               *item,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*save_item_finish)     (GnProvider           *self,
                                        GAsyncResult         *result,
                                        GError              **error);

  gboolean     (*trash_item)           (GnProvider           *self,
                                        GnItem               *item,
                                        GCancellable         *cancellable,
                                        GError              **error);
  void         (*trash_item_async)     (GnProvider           *self,
                                        GnItem              *item,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*trash_item_finish)    (GnProvider           *self,
                                        GAsyncResult         *result,
                                        GError              **error);

  void         (*restore_item_async)   (GnProvider           *self,
                                        GnItem               *item,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*restore_item_finish)  (GnProvider           *self,
                                        GAsyncResult         *result,
                                        GError              **error);

  void         (*delete_item_async)    (GnProvider           *self,
                                        GnItem               *item,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*delete_item_finish)   (GnProvider           *self,
                                        GAsyncResult         *result,
                                        GError              **error);

  GList       *(*get_notes)            (GnProvider           *self);
  GList       *(*get_trash_notes)      (GnProvider           *self);
  GList       *(*get_notebooks)        (GnProvider           *self);

  gchar       *(*get_uid)              (GnProvider           *self);
  const gchar *(*get_name)             (GnProvider           *self);
  gchar       *(*get_icon)             (GnProvider           *self);
  gboolean     (*get_rgba)             (GnProvider           *self,
                                        GdkRGBA              *rgba);
  gchar       *(*get_domain)           (GnProvider           *self);
  gchar       *(*get_user_name)        (GnProvider           *self);
  const gchar *(*get_location_name)    (GnProvider           *self);

  void         (*ready)                (GnProvider           *self);
};

gchar       *gn_provider_get_uid              (GnProvider           *self);
const gchar *gn_provider_get_name             (GnProvider           *self);
gchar       *gn_provider_get_icon             (GnProvider           *self);
gboolean     gn_provider_get_rgba             (GnProvider           *self,
                                               GdkRGBA              *rgba);
gchar       *gn_provider_get_domain           (GnProvider           *self);
gchar       *gn_provider_get_user_name        (GnProvider           *self);
const gchar *gn_provider_get_location_name    (GnProvider           *self);

gboolean     gn_provider_load_items           (GnProvider           *self,
                                               GCancellable         *cancellable,
                                               GError              **error);
void         gn_provider_load_items_async     (GnProvider           *self,
                                               GCancellable         *cancellable,
                                               GAsyncReadyCallback   callback,
                                               gpointer              user_data);
gboolean     gn_provider_load_items_finish    (GnProvider           *self,
                                               GAsyncResult         *result,
                                               GError              **error);

void         gn_provider_save_item_async      (GnProvider           *self,
                                               GnItem               *item,
                                               GCancellable         *cancellable,
                                               GAsyncReadyCallback   callback,
                                               gpointer              user_data);
gboolean     gn_provider_save_item_finish     (GnProvider           *self,
                                               GAsyncResult         *result,
                                               GError              **error);

gboolean     gn_provider_trash_item           (GnProvider           *self,
                                               GnItem               *item,
                                               GCancellable         *cancellable,
                                               GError              **error);
void         gn_provider_trash_item_async     (GnProvider           *self,
                                               GnItem               *item,
                                               GCancellable         *cancellable,
                                               GAsyncReadyCallback   callback,
                                               gpointer              user_data);
gboolean     gn_provider_trash_item_finish    (GnProvider           *self,
                                               GAsyncResult         *result,
                                               GError              **error);

void         gn_provider_restore_item_async   (GnProvider           *self,
                                               GnItem               *item,
                                               GCancellable         *cancellable,
                                               GAsyncReadyCallback   callback,
                                               gpointer              user_data);
gboolean     gn_provider_restore_item_finish  (GnProvider           *self,
                                               GAsyncResult         *result,
                                               GError              **error);

void         gn_provider_delete_item_async    (GnProvider           *self,
                                               GnItem               *item,
                                               GCancellable         *cancellable,
                                               GAsyncReadyCallback   callback,
                                               gpointer              user_data);
gboolean     gn_provider_delete_item_finish   (GnProvider           *self,
                                               GAsyncResult         *result,
                                               GError              **error);

GList       *gn_provider_get_notes            (GnProvider           *self);
GList       *gn_provider_get_trash_notes      (GnProvider           *self);
GList       *gn_provider_get_notebooks        (GnProvider           *self);

G_END_DECLS
