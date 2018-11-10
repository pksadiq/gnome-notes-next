/* gn-text-view.c
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

#define G_LOG_DOMAIN "gn-text-view"

#include "config.h"

#include "gn-text-view.h"
#include "gn-trace.h"

/**
 * SECTION: gn-text-view
 * @title: GnTextView
 * @short_description: The Note editor view
 * @include: "gn-text-view.h"
 */

struct _GnTextView
{
  GtkTextView parent_instance;
};

G_DEFINE_TYPE (GnTextView, gn_text_view, GTK_TYPE_TEXT_VIEW)

static void
gn_text_view_class_init (GnTextViewClass *klass)
{
}

static void
gn_text_view_init (GnTextView *self)
{
}

GtkWidget *
gn_text_view_new (void)
{
  return g_object_new (GN_TYPE_TEXT_VIEW,
                       NULL);
}
