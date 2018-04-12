/* gn-list-view.c
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

#define G_LOG_DOMAIN "gn-list-view"

#include "config.h"

#include "gn-list-view-item.h"
#include "gn-list-view.h"
#include "gn-trace.h"

/**
 * SECTION: gn-list-view
 * @title: GnListView
 * @short_description: Container whose child are shown as List
 * @include: "gn-list-view.h"
 */

struct _GnListView
{
  GtkListBox parent_instance;
};

G_DEFINE_TYPE (GnListView, gn_list_view, GTK_TYPE_LIST_BOX)

static void
gn_list_view_select_all (GtkListBox *box)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (box));

  for (GList *child = children; child != NULL; child = child->next)
    {
      gn_list_view_item_set_selected ((GN_LIST_VIEW_ITEM (child->data)),
                                      TRUE);
    }
}

static void
gn_list_view_class_init (GnListViewClass *klass)
{
  GtkListBoxClass *list_box_class = GTK_LIST_BOX_CLASS (klass);

  list_box_class->select_all = gn_list_view_select_all;
  list_box_class->unselect_all = gn_list_view_unselect_all;
}

static void
gn_list_view_init (GnListView *self)
{
}

GnListView *
gn_list_view_new (void)
{
  return g_object_new (GN_TYPE_LIST_VIEW,
                       NULL);
}

void
gn_list_view_unselect_all (GtkListBox *box)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (box));

  for (GList *child = children; child != NULL; child = child->next)
    {
      gn_list_view_item_set_selected ((GN_LIST_VIEW_ITEM (child->data)),
                                      FALSE);
    }
}
