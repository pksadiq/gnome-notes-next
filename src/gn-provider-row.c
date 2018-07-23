/* gn-provider-row.c
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

#define G_LOG_DOMAIN "gn-provider-row"

#include "config.h"

#include "gn-manager.h"
#include "gn-item-thumbnail.h"
#include "gn-provider-row.h"
#include "gn-trace.h"

/**
 * SECTION: gn-provider-row
 * @title: GnProviderRow
 * @short_description:
 * @include: "gn-provider-row.h"
 */

struct _GnProviderRow
{
  GtkListBoxRow parent_instance;

  GnProvider *provider;

  GtkWidget *icon_box;
  GtkWidget *title;
  GtkWidget *subtitle;
  GtkWidget *check_box_stack;
  /* GtkWidget *active; */
  /* GtkWidget *inactive; */

  gboolean selected;
};

G_DEFINE_TYPE (GnProviderRow, gn_provider_row, GTK_TYPE_LIST_BOX_ROW)

static GtkWidget *
gn_provider_row_get_icon (GnProviderRow *self,
                          GnProvider    *provider)
{
  GnManager *manager;
  GnSettings *settings;
  GIcon *icon;
  GtkWidget *icon_image;

  g_assert (GN_IS_PROVIDER (provider));

  manager = gn_manager_get_default ();
  settings = gn_manager_get_settings (manager);
  icon = gn_provider_get_icon (provider, NULL);

  if (icon == NULL)
    {
      GdkRGBA rgba;

      if (!gn_provider_get_rgba (provider, &rgba))
        gn_settings_get_rgba (settings, &rgba);

      icon_image = GTK_WIDGET (gn_item_thumbnail_new ("", &rgba));
      gtk_widget_set_size_request (icon_image, 32, 32);
    }
  else
    {
      icon_image = gtk_image_new_from_gicon (icon);
      gtk_image_set_pixel_size (GTK_IMAGE (icon_image), 32);
    }

  return icon_image;
}

static void
gn_provider_row_class_init (GnProviderRowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-provider-row.ui");

  gtk_widget_class_bind_template_child (widget_class, GnProviderRow, icon_box);
  gtk_widget_class_bind_template_child (widget_class, GnProviderRow, title);
  gtk_widget_class_bind_template_child (widget_class, GnProviderRow, subtitle);
  gtk_widget_class_bind_template_child (widget_class, GnProviderRow, check_box_stack);
  /* gtk_widget_class_bind_template_child (widget_class, GnProviderRow, active); */
  /* gtk_widget_class_bind_template_child (widget_class, GnProviderRow, inactive); */
}

static void
gn_provider_row_init (GnProviderRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
gn_provider_row_new (GnProvider *provider)
{
  GnProviderRow *self;
  GtkWidget *icon;

  g_return_val_if_fail (GN_IS_PROVIDER (provider), NULL);

  self = g_object_new (GN_TYPE_PROVIDER_ROW, NULL);
  icon = gn_provider_row_get_icon (self, provider);
  self->provider = provider;

  gtk_container_add (GTK_CONTAINER (self->icon_box), icon);
  gtk_label_set_label (GTK_LABEL (self->title),
                       gn_provider_get_name (provider));
  gtk_label_set_label (GTK_LABEL (self->subtitle),
                       gn_provider_get_location_name (provider));

  return GTK_WIDGET (self);
}

GnProvider *
gn_provider_row_get_provider (GnProviderRow *self)
{
  g_return_val_if_fail (GN_IS_PROVIDER_ROW (self), NULL);

  return self->provider;
}

void
gn_provider_row_set_selection (GnProviderRow *self)
{
  GtkStack *stack;

  g_return_if_fail (GN_IS_PROVIDER_ROW (self));

  stack = GTK_STACK (self->check_box_stack);
  gtk_stack_set_visible_child_name (stack, "selected");
}

void
gn_provider_row_unset_selection (GnProviderRow *self,
                                 gpointer       user_data)
{
  GtkStack *stack;

  g_return_if_fail (GN_IS_PROVIDER_ROW (self));

  stack = GTK_STACK (self->check_box_stack);
  gtk_stack_set_visible_child_name (stack, "empty");
}

