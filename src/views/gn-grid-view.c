/* gn-grid-view.c
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

#define G_LOG_DOMAIN "gn-grid-view"

#include "config.h"

#include "gn-grid-view-item.h"
#include "gn-grid-view.h"
#include "gn-trace.h"

/**
 * SECTION: gn-grid-view
 * @title: GnGridView
 * @short_description: Container whose child are shown as Grid
 * @include: "gn-grid-view.h"
 */

struct _GnGridView
{
  GtkFlowBox parent_instance;
};

G_DEFINE_TYPE (GnGridView, gn_grid_view, GTK_TYPE_FLOW_BOX)

static void
gn_grid_view_select_all (GtkFlowBox *box)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (box));

  for (GList *child = children; child != NULL; child = child->next)
    {
      gn_grid_view_item_set_selected ((GN_GRID_VIEW_ITEM (child->data)),
                                      TRUE);
    }
}

static void
gn_grid_view_class_init (GnGridViewClass *klass)
{
  GtkFlowBoxClass *flow_box_class = GTK_FLOW_BOX_CLASS (klass);

  flow_box_class->select_all = gn_grid_view_select_all;
  flow_box_class->unselect_all = gn_grid_view_unselect_all;
}

static void
gn_grid_view_init (GnGridView *self)
{
}

GnGridView *
gn_grid_view_new (void)
{
  return g_object_new (GN_TYPE_GRID_VIEW,
                       NULL);
}

void
gn_grid_view_unselect_all (GtkFlowBox *box)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (box));

  for (GList *child = children; child != NULL; child = child->next)
    {
      gn_grid_view_item_set_selected ((GN_GRID_VIEW_ITEM (child->data)),
                                      FALSE);
    }
}
