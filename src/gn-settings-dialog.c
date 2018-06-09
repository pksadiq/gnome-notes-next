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
};

G_DEFINE_TYPE (GnSettingsDialog, gn_settings_dialog, GTK_TYPE_DIALOG)

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
}

GtkWidget *
gn_settings_dialog_new (GtkWindow *window)
{
  return g_object_new (GN_TYPE_SETTINGS_DIALOG,
                       "transient-for", window,
                       "use-header-bar", TRUE,
                       NULL);
}
