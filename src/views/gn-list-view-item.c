/* gn-list-view-item.c
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

#define G_LOG_DOMAIN "gn-list-view-item"

#include "config.h"

#include "gn-note.h"
#include "gn-item-thumbnail.h"
#include "gn-provider-item.h"
#include "gn-manager.h"
#include "gn-settings.h"
#include "gn-list-view-item.h"
#include "gn-trace.h"

/**
 * SECTION: gn-list-view-item
 * @title: GnListViewItem
 * @short_description: A widget to show the note or notebook
 * @include: "gn-list-view-item.h"
 */

struct _GnListViewItem
{
  GtkListBoxRow parent_instance;

  GnProviderItem *item;

  GtkWidget *color_box;
  GtkWidget *title_label;
  GtkWidget *check_box;

  gboolean selected;
};

G_DEFINE_TYPE (GnListViewItem, gn_list_view_item, GTK_TYPE_LIST_BOX_ROW)

static void
gn_list_view_item_toggled (GnListViewItem  *self,
                           GtkToggleButton *button)
{
  gboolean is_selected;

  g_assert (GN_IS_LIST_VIEW_ITEM (self));
  g_assert (GTK_IS_TOGGLE_BUTTON (button));

  is_selected = gtk_toggle_button_get_active (button);
  gn_list_view_item_set_selected (self, is_selected);
}

static void
gn_list_view_item_class_init (GnListViewItemClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_ensure (GN_TYPE_ITEM_THUMBNAIL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-list-view-item.ui");

  gtk_widget_class_bind_template_child (widget_class, GnListViewItem, color_box);
  gtk_widget_class_bind_template_child (widget_class, GnListViewItem, check_box);
  gtk_widget_class_bind_template_child (widget_class, GnListViewItem, title_label);

  gtk_widget_class_bind_template_callback (widget_class, gn_list_view_item_toggled);
}

static void
gn_list_view_item_init (GnListViewItem *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
gn_list_view_item_new (gpointer data,
                       gpointer user_data)
{
  GnListViewItem *self;
  GnProviderItem *provider_item = data;
  GObject *object = user_data;
  const gchar *title;
  GnItem *item;
  GdkRGBA rgba;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_PROVIDER_ITEM (provider_item), NULL);

  self = g_object_new (GN_TYPE_LIST_VIEW_ITEM, NULL);
  item = gn_provider_item_get_item (provider_item);
  title = gn_item_get_title (item);
  self->item = provider_item;

  g_object_bind_property (object, "selection-mode",
                          self->check_box, "visible",
                          G_BINDING_SYNC_CREATE);

  if (!gn_item_get_rgba (item, &rgba))
    gn_settings_get_rgba (gn_manager_get_settings (gn_manager_get_default ()),
                          &rgba);

  gtk_label_set_label (GTK_LABEL (self->title_label), title);

  g_object_set (G_OBJECT (self->color_box),
                "rgba", &rgba,
                NULL);

  GN_RETURN (GTK_WIDGET (self));
}

void
gn_list_view_item_set_selected (GnListViewItem *self,
                                gboolean        is_selected)
{
  GtkListBox *box;

  g_return_if_fail (GN_IS_LIST_VIEW_ITEM (self));

  box = GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self)));

  if (is_selected)
    gtk_list_box_select_row (box, GTK_LIST_BOX_ROW (self));
  else
    gtk_list_box_unselect_row (box, GTK_LIST_BOX_ROW (self));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->check_box),
                                is_selected);
}

gboolean
gn_list_view_item_get_selected (GnListViewItem *self)
{
  return self->selected;
}

void
gn_list_view_item_toggle_selection (GnListViewItem *self)
{
  GtkToggleButton *button;
  gboolean selection;

  g_return_if_fail (GN_IS_LIST_VIEW_ITEM (self));

  button = GTK_TOGGLE_BUTTON (self->check_box);
  selection = gtk_toggle_button_get_active (button);
  selection = !selection;

  gn_list_view_item_set_selected (self, selection);
}

GnProviderItem *
gn_list_view_item_get_item (GnListViewItem *self)
{
  g_return_val_if_fail (GN_IS_LIST_VIEW_ITEM (self), NULL);

  return self->item;
}
