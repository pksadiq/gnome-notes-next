/* gn-settings-dialog.c
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

#define G_LOG_DOMAIN "gn-settings-dialog"

#include "config.h"

#include "gn-manager.h"
#include "gn-settings.h"
#include "gn-provider-row.h"
#include "gn-settings-dialog.h"
#include "gn-trace.h"

/**
 * SECTION: gn-settings-dialog
 * @title: GnSettingsDialog
 * @short_description:
 * @include: "gn-settings-dialog.h"
 */

struct _GnSettingsDialog
{
  GtkDialog parent_instance;

  GnSettings *settings;

  GtkWidget *font_button;
  GtkWidget *color_button;
  GtkWidget *system_font_switch;
  GtkWidget *provider_list;
};

G_DEFINE_TYPE (GnSettingsDialog, gn_settings_dialog, GTK_TYPE_DIALOG)

static void
gn_settings_dialog_row_activated (GnSettingsDialog *self,
                                  GtkListBoxRow    *row,
                                  GtkListBox       *list_box)
{
  GnManager *manager;
  GnSettings *settings;
  GnProvider *provider;
  g_autofree gchar *provider_uid = NULL;

  g_assert (GN_IS_SETTINGS_DIALOG (self));
  g_assert (GN_IS_PROVIDER_ROW (row));

  manager = gn_manager_get_default ();
  settings = gn_manager_get_settings (manager);
  provider = gn_provider_row_get_provider (GN_PROVIDER_ROW (row));
  provider_uid = gn_provider_get_uid (provider);
  gn_settings_set_provider_name (settings, provider_uid);

  gtk_container_foreach (GTK_CONTAINER (self->provider_list),
                         (GtkCallback)gn_provider_row_unset_selection, NULL);
  gn_provider_row_set_selection (GN_PROVIDER_ROW (row));
}

void
gn_settings_dialog_add_providers (GnSettingsDialog *self)
{
  GnManager *manager;
  GnSettings *settings;
  g_autoptr(GList) providers = NULL;
  GnProvider *default_provider;

  g_assert (GN_IS_SETTINGS_DIALOG (self));

  manager = gn_manager_get_default ();
  providers = gn_manager_get_providers (manager);
  default_provider = gn_manager_get_default_provider (manager, TRUE);

  for (GList *node = providers; node != NULL; node = node->next)
    {
      GtkWidget *row = gn_provider_row_new (node->data);

      if (node->data == default_provider)
        gn_provider_row_set_selection (GN_PROVIDER_ROW (row));

      gtk_container_add (GTK_CONTAINER (self->provider_list), row);
    }
}

static void
gn_settings_dialog_class_init (GnSettingsDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-settings-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, GnSettingsDialog, font_button);
  gtk_widget_class_bind_template_child (widget_class, GnSettingsDialog, color_button);
  gtk_widget_class_bind_template_child (widget_class, GnSettingsDialog, system_font_switch);
  gtk_widget_class_bind_template_child (widget_class, GnSettingsDialog, provider_list);

  gtk_widget_class_bind_template_callback (widget_class, gn_settings_dialog_row_activated);
}

static void
gn_settings_dialog_init (GnSettingsDialog *self)
{
  GnManager *manager;

  manager = gn_manager_get_default ();
  self->settings = gn_manager_get_settings (manager);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (self->settings, "font",
                          self->font_button, "font",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  g_object_bind_property (self->settings, "use-system-font",
                          self->system_font_switch, "active",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  g_object_bind_property (self->settings, "color",
                          self->color_button, "rgba",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  gn_settings_dialog_add_providers (self);
}

GtkWidget *
gn_settings_dialog_new (GtkWindow *window)
{
  return g_object_new (GN_TYPE_SETTINGS_DIALOG,
                       "transient-for", window,
                       "use-header-bar", TRUE,
                       NULL);
}
