/* gn-tag-row.c
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

#define G_LOG_DOMAIN "gn-tag-row"

#include "config.h"

#include "gn-manager.h"
#include "gn-item-thumbnail.h"
#include "gn-tag-row.h"
#include "gn-trace.h"

/**
 * SECTION: gn-tag-row
 * @title: GnTagRow
 * @short_description: A #GtkListBoxRow representing a note tag
 * @include: "gn-tag-row.h"
 */

struct _GnTagRow
{
  GtkListBoxRow parent_instance;

  GnTag *tag;

  GtkWidget *color_button;
  GtkWidget *tag_name;
};

G_DEFINE_TYPE (GnTagRow, gn_tag_row, GTK_TYPE_LIST_BOX_ROW)

static void
gn_tag_row_class_init (GnTagRowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-tag-row.ui");

  gtk_widget_class_bind_template_child (widget_class, GnTagRow, color_button);
  gtk_widget_class_bind_template_child (widget_class, GnTagRow, tag_name);
}

static void
gn_tag_row_init (GnTagRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
gn_tag_row_new (gpointer item,
                gpointer user_data)
{
  GnTagRow *self;
  GnTag *tag = item;

  g_return_val_if_fail (GN_IS_TAG (tag), NULL);

  self = g_object_new (GN_TYPE_TAG_ROW, NULL);
  self->tag = tag;

  gtk_label_set_label (GTK_LABEL (self->tag_name),
                       gn_tag_get_name (tag));

  return GTK_WIDGET (self);
}
