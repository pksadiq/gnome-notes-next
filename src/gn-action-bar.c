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

#include "gn-window.h"
#include "gn-action-bar.h"
#include "gn-trace.h"

/**
 * SECTION: gn-action-bar
 * @title: GnActionBar
 * @short_description:
 * @include: "gn-action-bar.h"
 */

/*
 * FIXME: I don't feel the design of this class to be good. Because
 * This class knows about #GnWindow class, and #GnWindow class knows
 * about this class. It should better be only one way. That is,
 * #GnWindow knows about this class. Should we try hard to fix that?
 * Or just don't care?
 */

struct _GnActionBar
{
  GtkActionBar parent_instance;

  GtkWidget *actions_start;
};

G_DEFINE_TYPE (GnActionBar, gn_action_bar, GTK_TYPE_ACTION_BAR)

static void
gn_action_bar_delete_selected_items (GnActionBar *self,
                                     GtkWidget   *widget)
{
  GtkWidget *parent;

  g_assert (GN_IS_ACTION_BAR (self));
  g_assert (GTK_IS_WIDGET (widget));

  parent = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_WINDOW);
  g_assert (parent != NULL);

  g_print ("deleted\n");
  gn_window_trash_selected_items (GN_WINDOW (parent));
}

static void
gn_action_bar_class_init (GnActionBarClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-action-bar.ui");

  gtk_widget_class_bind_template_child (widget_class, GnActionBar, actions_start);

  gtk_widget_class_bind_template_callback (widget_class, gn_action_bar_delete_selected_items);
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
