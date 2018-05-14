/* gn-grid-view-item.c
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

#define G_LOG_DOMAIN "gn-grid-view-item"

#include "config.h"

#include "gn-note.h"
#include "gn-item-thumbnail.h"
#include "gn-manager.h"
#include "gn-settings.h"
#include "gn-grid-view-item.h"
#include "gn-trace.h"

/**
 * SECTION: gn-grid-view-item
 * @title: GnGridViewItem
 * @short_description: A widget to show the note or notebook
 * @include: "gn-grid-view-item.h"
 */

struct _GnGridViewItem
{
  GtkFlowBoxChild parent_instance;

  GnItem *item;

  GtkWidget *preview_label;
  GtkWidget *title_label;
  GtkWidget *check_box;

  gboolean selected;
};

G_DEFINE_TYPE (GnGridViewItem, gn_grid_view_item, GTK_TYPE_FLOW_BOX_CHILD)

static void
gn_grid_view_item_toggled (GnGridViewItem  *self,
                           GtkToggleButton *button)
{
  gboolean is_selected;

  g_assert (GN_IS_GRID_VIEW_ITEM (self));
  g_assert (GTK_IS_TOGGLE_BUTTON (button));

  is_selected = gtk_toggle_button_get_active (button);
  gn_grid_view_item_set_selected (self, is_selected);
}

static void
gn_grid_view_item_class_init (GnGridViewItemClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_ensure (GN_TYPE_ITEM_THUMBNAIL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-grid-view-item.ui");

  gtk_widget_class_bind_template_child (widget_class, GnGridViewItem, preview_label);
  gtk_widget_class_bind_template_child (widget_class, GnGridViewItem, title_label);
  gtk_widget_class_bind_template_child (widget_class, GnGridViewItem, check_box);

  gtk_widget_class_bind_template_callback (widget_class, gn_grid_view_item_toggled);
}

static void
gn_grid_view_item_init (GnGridViewItem *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
gn_grid_view_item_new (gpointer data,
                       gpointer user_data)
{
  GnGridViewItem *self;
  GnItem *item = data;
  GObject *object = user_data;
  g_autofree gchar *markup = NULL;
  const gchar *title;
  GdkRGBA rgba;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_ITEM (item), NULL);

  self = g_object_new (GN_TYPE_GRID_VIEW_ITEM, NULL);
  title = gn_item_get_title (item);
  markup = gn_note_get_markup (GN_NOTE (item));
  self->item = item;

  g_object_bind_property (object, "selection-mode",
                          self->check_box, "visible",
                          G_BINDING_SYNC_CREATE);

  if (!gn_item_get_rgba (item, &rgba))
    gn_settings_get_rgba (gn_manager_get_settings (gn_manager_get_default ()),
                          &rgba);

  gtk_label_set_label (GTK_LABEL (self->title_label), title);

  g_object_set (G_OBJECT (self->preview_label),
                "rgba", &rgba,
                "label", markup,
                NULL);

  GN_RETURN (GTK_WIDGET (self));
}

void
gn_grid_view_item_set_selected (GnGridViewItem *self,
                                gboolean        is_selected)
{
  GtkFlowBox *box;

  g_return_if_fail (GN_IS_GRID_VIEW_ITEM (self));

  box = GTK_FLOW_BOX (gtk_widget_get_parent (GTK_WIDGET (self)));

  if (is_selected)
    gtk_flow_box_select_child (box, GTK_FLOW_BOX_CHILD (self));
  else
    gtk_flow_box_unselect_child (box, GTK_FLOW_BOX_CHILD (self));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->check_box),
                                is_selected);
}

gboolean
gn_grid_view_item_get_selected (GnGridViewItem *self)
{
  return self->selected;
}

void
gn_grid_view_item_toggle_selection (GnGridViewItem *self)
{
  GtkToggleButton *button;
  gboolean selection;

  g_return_if_fail (GN_IS_GRID_VIEW_ITEM (self));

  button = GTK_TOGGLE_BUTTON (self->check_box);
  selection = gtk_toggle_button_get_active (button);
  selection = !selection;

  gn_grid_view_item_set_selected (self, selection);
}

GnItem *
gn_grid_view_item_get_item (GnGridViewItem *self)
{
  g_return_val_if_fail (GN_IS_GRID_VIEW_ITEM (self), NULL);

  return self->item;
}

