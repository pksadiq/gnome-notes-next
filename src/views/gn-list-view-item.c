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

#include "gn-utils.h"
#include "gn-tag-store.h"
#include "gn-note.h"
#include "gn-item-thumbnail.h"
#include "gn-manager.h"
#include "gn-settings.h"
#include "gn-tag-preview.h"
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

  GnItem *item;

  GtkWidget *title_label;
  GtkWidget *time_label;
  GtkWidget *preview_label;
  GtkWidget *tags_box;
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

  gtk_widget_class_bind_template_child (widget_class, GnListViewItem, title_label);
  gtk_widget_class_bind_template_child (widget_class, GnListViewItem, time_label);
  gtk_widget_class_bind_template_child (widget_class, GnListViewItem, preview_label);
  gtk_widget_class_bind_template_child (widget_class, GnListViewItem, tags_box);
  gtk_widget_class_bind_template_child (widget_class, GnListViewItem, check_box);

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
  GnItem *item = data;
  GObject *object = user_data;
  g_autofree gchar *markup = NULL;
  g_autofree gchar *title_markup = NULL;
  g_autofree gchar *time_label = NULL;
  GList *tags;
  GdkRGBA rgba;
  gint64 modification_time;

  GN_ENTRY;

  g_return_val_if_fail (GN_IS_ITEM (item), NULL);

  self = g_object_new (GN_TYPE_LIST_VIEW_ITEM, NULL);
  self->item = item;
  markup = gn_note_get_markup (GN_NOTE (item));
  modification_time = gn_item_get_modification_time (item);
  time_label = gn_utils_get_human_time (modification_time);
  tags = gn_note_get_tags (GN_NOTE (item));

  /* A space is appended to title so as to keep height on empty title */
  title_markup = g_strconcat ("<span font='Cantarell' size='large'>",
                              gn_item_get_title (item), " </span>",
                              NULL);
  g_object_set (self->title_label, "label", title_markup, NULL);
  gtk_label_set_label (GTK_LABEL (self->time_label), time_label);

  g_object_bind_property (object, "selection-mode",
                          self->check_box, "visible",
                          G_BINDING_SYNC_CREATE);

  if (!gn_item_get_rgba (item, &rgba))
    gn_settings_get_rgba (gn_manager_get_settings (gn_manager_get_default ()),
                          &rgba);

  for (GList *node = tags; node != NULL; node = node->next)
    {
      GtkWidget *tag;

      tag = gn_tag_preview_new (node->data);
      gtk_container_add (GTK_CONTAINER (self->tags_box), tag);
    }

  g_object_set (G_OBJECT (self->preview_label),
                "rgba", &rgba,
                "label", markup,
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

GnItem *
gn_list_view_item_get_item (GnListViewItem *self)
{
  g_return_val_if_fail (GN_IS_LIST_VIEW_ITEM (self), NULL);

  return self->item;
}
