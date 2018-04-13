/* gn-action-bar.c
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

#define G_LOG_DOMAIN "gn-action-bar"

#include "config.h"

#include "gn-action-bar.h"
#include "gn-trace.h"

/**
 * SECTION: gn-action-bar
 * @title: GnActionBar
 * @short_description:
 * @include: "gn-action-bar.h"
 */

struct _GnActionBar
{
  GtkActionBar parent_instance;

  GtkWidget *actions_stack;
};

G_DEFINE_TYPE (GnActionBar, gn_action_bar, GTK_TYPE_ACTION_BAR)

static void
gn_action_bar_class_init (GnActionBarClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-action-bar.ui");

  gtk_widget_class_bind_template_child (widget_class, GnActionBar, actions_stack);
}

static void
gn_action_bar_init (GnActionBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GnActionBar *
gn_action_bar_new (void)
{
  return g_object_new (GN_TYPE_ACTION_BAR,
                       NULL);
}
