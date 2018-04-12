/* gn-window.c
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

#define G_LOG_DOMAIN "gn-window"

#include <glib/gi18n.h>

#include "gn-application.h"
#include "gn-manager.h"
#include "gn-enums.h"
#include "gn-utils.h"
#include "gn-note.h"
#include "gn-provider-item.h"
#include "gn-settings.h"
#include "gn-main-view.h"
#include "gn-window.h"
#include "gn-trace.h"

struct _GnWindow
{
  GtkApplicationWindow parent_instance;

  GtkWidget *header_bar;
  GtkWidget *header_title_stack;
  GtkWidget *view_button_stack;
  GtkWidget *grid_button;
  GtkWidget *list_button;
  GtkWidget *navigate_button_stack;
  GtkWidget *new_button;
  GtkWidget *back_button;

  GtkWidget *select_button_stack;
  GtkWidget *select_button;
  GtkWidget *cancel_button;

  GtkWidget *main_stack;
  GtkWidget *notes_stack;
  GtkWidget *notes_view;
  GtkWidget *trash_stack;

  GQueue    *view_stack;
  GnView     current_view;
  GnViewMode current_view_mode;

  gint       width;
  gint       height;
  gint       pos_x;
  gint       pos_y;
  gboolean   is_maximized;
};

G_DEFINE_TYPE (GnWindow, gn_window, GTK_TYPE_APPLICATION_WINDOW)

static void
gn_window_provider_added_cb (GnWindow   *self,
                             GnProvider *provider,
                             GnManager  *manager)
{
  GListStore *store;

  g_assert (GN_IS_MAIN_THREAD ());
  g_assert (GN_IS_WINDOW (self));
  g_assert (GN_IS_PROVIDER (provider));

  store = gn_manager_get_notes_store (gn_manager_get_default ());

  gn_main_view_set_view (GN_MAIN_VIEW (self->notes_view),
                         GN_VIEW_TYPE_LIST);
  gn_main_view_set_model (GN_MAIN_VIEW (self->notes_view),
                          G_LIST_MODEL (store));

  /* Change view if we have at least one item in store */
  if (g_list_model_get_item (G_LIST_MODEL (store), 0) != NULL)
    gtk_stack_set_visible_child_name (GTK_STACK (self->notes_stack),
                                      "note-view");
}

static void
gn_window_load_more_items (GnWindow          *self,
                           GtkPositionType    pos,
                           GtkScrolledWindow *scrolled_window)
{
  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_SCROLLED_WINDOW (scrolled_window));

  /* We load more items only if bottom edge is hit */
  if (pos != GTK_POS_BOTTOM)
    return;

  if (self->current_view == GN_VIEW_NOTES)
    gn_manager_load_more_notes (gn_manager_get_default ());
}

static void
gn_window_show_previous_view (GnWindow  *self,
                              GtkWidget *widget)
{
  GnView last_view;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_WIDGET (widget));

  last_view = GPOINTER_TO_INT (g_queue_pop_head (self->view_stack));
  gn_window_set_view (self, last_view, GN_VIEW_MODE_NORMAL);
}

static void
gn_window_view_button_toggled (GnWindow  *self,
                               GtkWidget *widget)
{
  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_WIDGET (widget));

  if (widget == self->grid_button)
    {
      gn_main_view_set_view (GN_MAIN_VIEW (self->notes_view),
                             GN_VIEW_TYPE_GRID);
      gtk_stack_set_visible_child (GTK_STACK (self->view_button_stack),
                                   self->list_button);
    }
  else
    {
      gn_main_view_set_view (GN_MAIN_VIEW (self->notes_view),
                             GN_VIEW_TYPE_LIST);
      gtk_stack_set_visible_child (GTK_STACK (self->view_button_stack),
                                   self->grid_button);
    }
}

static void
gn_window_selection_mode_toggled (GnWindow  *self,
                                  GtkWidget *widget)
{
  GtkStyleContext *style_context;
  gboolean selection_mode;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_BUTTON (widget));

  if (widget == self->select_button)
    self->current_view_mode = GN_VIEW_MODE_SELECTION;
  else
    self->current_view_mode = GN_VIEW_MODE_NORMAL;

  selection_mode = self->current_view_mode == GN_VIEW_MODE_SELECTION;
  style_context = gtk_widget_get_style_context (self->header_bar);

  if (self->current_view == GN_VIEW_NOTES)
    {
      gn_main_view_set_selection_mode (GN_MAIN_VIEW (self->notes_view),
                                       selection_mode);
    }

  if (selection_mode)
    {
      gtk_style_context_add_class (style_context, "selection-mode");
      gtk_stack_set_visible_child_name (GTK_STACK (self->header_title_stack),
                                        "title");
      gtk_header_bar_set_title (GTK_HEADER_BAR (self->header_bar),
                                _("Click on items to select them"));
    }
  else
    {
      gtk_style_context_remove_class (style_context, "selection-mode");
      gtk_stack_set_visible_child_name (GTK_STACK (self->header_title_stack),
                                        "main");
    }

  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self->header_bar),
                                        !selection_mode);
  /*
   * widget can either be select_button or cancel_button. As their
   * 'visible' property is bind to inverse of each other, hiding
   * one of them reveals the other.
   */
  gtk_widget_hide (widget);
}

static void
gn_window_show_view (GnWindow *self,
                     GnView    view)
{
  switch (view)
    {
    case GN_VIEW_TRASH:
      gtk_stack_set_visible_child (GTK_STACK (self->main_stack),
                                   self->trash_stack);
      gtk_stack_set_visible_child (GTK_STACK (self->navigate_button_stack),
                                   self->back_button);
      gtk_stack_set_visible_child_name (GTK_STACK (self->header_title_stack),
                                        "title");
      break;

    case GN_VIEW_NOTES:
      gtk_stack_set_visible_child (GTK_STACK (self->main_stack),
                                   self->notes_stack);
      gtk_stack_set_visible_child (GTK_STACK (self->navigate_button_stack),
                                   self->new_button);
      gtk_stack_set_visible_child_name (GTK_STACK (self->header_title_stack),
                                        "main");
    }
}

static void
gn_window_size_allocate_cb (GnWindow *self)
{
  GtkWindow *window = GTK_WINDOW (self);
  GnSettings *settings;
  gboolean is_maximized;
  gint width, height, x, y;

  g_assert (GN_IS_WINDOW (self));

  settings = gn_manager_get_settings (gn_manager_get_default ());
  is_maximized = gtk_window_is_maximized (window);
  gn_settings_set_window_maximized (settings, is_maximized);

  if (is_maximized)
    return;

  gtk_window_get_size (window, &width, &height);
  gtk_window_get_position (window, &x, &y);

  gn_settings_set_window_geometry (settings, width, height, x, y);
}

static void
gn_window_destroy_cb (GnWindow *self)
{
  GnSettings *settings;

  g_assert (GN_IS_WINDOW (self));

  settings = gn_manager_get_settings (gn_manager_get_default ());
  gn_settings_save_window_state (settings);
}

static void
gn_window_constructed (GObject *object)
{
  GnWindow *self = GN_WINDOW (object);
  GtkWindow *window = GTK_WINDOW (self);
  GnSettings *settings;
  gboolean is_maximized;
  gint width, height;
  gint x, y;

  settings = gn_manager_get_settings (gn_manager_get_default ());
  is_maximized = gn_settings_get_window_maximized (settings);
  gn_settings_get_window_geometry (settings, &width, &height, &x, &y);

  gtk_window_set_default_size (window, width, height);

  if (is_maximized)
    gtk_window_maximize (window);
  else if (x >= 0)
    gtk_window_move (window, x, y);

  G_OBJECT_CLASS (gn_window_parent_class)->constructed (object);
}

static void
gn_window_class_init (GnWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gn_window_constructed;

  g_type_ensure (GN_TYPE_MAIN_VIEW);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-window.ui");

  gtk_widget_class_bind_template_child (widget_class, GnWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, header_title_stack);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, main_stack);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, notes_stack);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, notes_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, trash_stack);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, view_button_stack);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, grid_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, list_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, navigate_button_stack);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, new_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, back_button);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, select_button_stack);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, select_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, cancel_button);

  gtk_widget_class_bind_template_callback (widget_class, gn_window_destroy_cb);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_size_allocate_cb);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_show_previous_view);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_view_button_toggled);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_selection_mode_toggled);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_load_more_items);
}

static void
gn_window_init (GnWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->view_stack = g_queue_new ();
  g_signal_connect_object (gn_manager_get_default (),
                           "provider-added",
                           G_CALLBACK (gn_window_provider_added_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

GnWindow *
gn_window_new (GnApplication *application)
{
  g_assert (GTK_IS_APPLICATION (application));

  return g_object_new (GN_TYPE_WINDOW,
                       "application", application,
                       NULL);
}

GnViewMode
gn_window_get_mode (GnWindow *self)
{
  g_return_val_if_fail (GN_IS_WINDOW (self), 0);

  return self->current_view_mode;
}

void
gn_window_set_view (GnWindow   *self,
                    GnView      view,
                    GnViewMode  mode)
{
  g_return_if_fail (GN_IS_WINDOW (self));

  if (view != self->current_view)
    {
      g_queue_push_head (self->view_stack,
                         GINT_TO_POINTER (self->current_view));

      if (view == GN_VIEW_NOTES ||
          view == GN_VIEW_NOTEBOOKS)
        g_queue_clear (self->view_stack);

      self->current_view = view;
      gn_window_show_view (self, view);
    }
}
