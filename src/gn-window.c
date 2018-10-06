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

#include "config.h"

#include <glib/gi18n.h>

#include "gn-application.h"
#include "gn-manager.h"
#include "gn-enums.h"
#include "gn-utils.h"
#include "gn-note.h"
#include "gn-provider.h"
#include "gn-settings.h"
#include "gn-settings-dialog.h"
#include "gn-editor.h"
#include "gn-main-view.h"
#include "gn-action-bar.h"
#include "gn-window.h"
#include "gn-trace.h"

struct _GnWindow
{
  GtkApplicationWindow parent_instance;

  GtkWidget *header_bar;
  GtkWidget *secondary_header_bar;
  GtkWidget *new_button;
  GtkWidget *undo_revealer;

  GtkWidget *select_button;
  GtkWidget *main_action_bar;

  GtkWidget *search_bar;
  GtkWidget *search_entry;
  GtkWidget *search_view;
  GtkWidget *main_view;
  GtkWidget *notes_view;
  GtkWidget *notebook_view;
  GtkWidget *editor_view;
  GtkWidget *trash_view;

  GQueue    *view_stack;
  GtkWidget *current_view;
  GnViewMode current_view_mode;

  guint      undo_timeout_id;
};

G_DEFINE_TYPE (GnWindow, gn_window, GTK_TYPE_APPLICATION_WINDOW)

static void
gn_window_cancel_delete (GnWindow *self)
{
  GnManager *manager;

  g_assert (GN_IS_WINDOW (self));

  manager = gn_manager_get_default ();
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->undo_revealer), FALSE);
  gn_manager_dequeue_delete (manager);
  g_clear_handle_id (&self->undo_timeout_id, g_source_remove);
}

static void
gn_window_continue_delete (GnWindow *self)
{
  GnManager *manager;

  g_assert (GN_IS_WINDOW (self));

  manager = gn_manager_get_default ();
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->undo_revealer), FALSE);
  g_clear_handle_id (&self->undo_timeout_id, g_source_remove);
  gn_manager_trash_queue_items (manager);
}

static int
gn_window_undo_timeout_cb (gpointer user_data)
{
  GnWindow *self = user_data;

  g_assert (GN_IS_WINDOW (self));

  self->undo_timeout_id = 0;
  gn_window_continue_delete (self);

  return G_SOURCE_REMOVE;
}

static void
gn_window_show_undo_revealer (GnWindow *self)
{
  g_assert (GN_IS_WINDOW (self));

  gtk_revealer_set_reveal_child (GTK_REVEALER (self->undo_revealer), TRUE);
  self->undo_timeout_id = g_timeout_add_seconds (10, gn_window_undo_timeout_cb, self);
}

static void
gn_window_set_title (GnWindow    *self,
                     const gchar *title,
                     const gchar *subtitle)
{
  GtkHeaderBar *header_bar;

  g_assert (GN_IS_WINDOW (self));

  header_bar = GTK_HEADER_BAR (self->secondary_header_bar);

  gtk_header_bar_set_title (header_bar, title);
  gtk_header_bar_set_subtitle (header_bar, subtitle);
}

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
                         "list");

  gn_main_view_set_model (GN_MAIN_VIEW (self->notes_view),
                          G_LIST_MODEL (store));

  store = gn_manager_get_trash_notes_store (gn_manager_get_default ());

  gn_main_view_set_model (GN_MAIN_VIEW (self->trash_view),
                          G_LIST_MODEL (store));

  store = gn_manager_get_search_store (gn_manager_get_default ());

  gn_main_view_set_model (GN_MAIN_VIEW (self->search_view),
                          G_LIST_MODEL (store));
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

  if (self->current_view == self->notes_view)
    gn_manager_load_more_notes (gn_manager_get_default ());
  else if (self->current_view == self->trash_view)
    gn_manager_load_more_trash_notes (gn_manager_get_default ());
}

static void
gn_window_search_changed (GnWindow       *self,
                          GtkSearchEntry *search_entry)
{
  const gchar *needle;
  GnManager *manager;
  GtkWidget *last_view;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_SEARCH_ENTRY (search_entry));

  manager = gn_manager_get_default ();
  needle = gtk_entry_get_text (GTK_ENTRY (search_entry));
  gn_manager_search (manager, &needle);

  if (needle[0] != '\0')
    {
      gn_window_set_view (self, self->search_view, GN_VIEW_MODE_NORMAL);
    }
  else
    {
      last_view = g_queue_pop_head (self->view_stack);
      gn_window_set_view (self, last_view, GN_VIEW_MODE_NORMAL);
    }
}

static void
gn_window_open_new_note (GnWindow *self)
{
  GnItem *item;
  GnProvider *provider;
  GnManager *manager;
  GtkWidget *editor, *child;

  g_assert (GN_IS_WINDOW (self));

  manager = gn_manager_get_default ();
  item = gn_manager_new_note (manager);
  provider = g_object_get_data (G_OBJECT (item), "provider");

  editor = gn_editor_new ();
  gn_editor_set_item (GN_EDITOR (editor), item);

  child = gtk_bin_get_child (GTK_BIN (self->editor_view));
  if (child != NULL)
    gtk_container_remove (GTK_CONTAINER (self->editor_view), child);

  gn_window_set_title (self, _("Untitled"),
                       gn_provider_get_name (provider));
  gtk_container_add (GTK_CONTAINER (self->editor_view), editor);
  gn_window_set_view (self, self->editor_view, GN_VIEW_MODE_NORMAL);
}

static void
gn_window_show_trash (GnWindow  *self,
                      GtkWidget *widget)
{
  g_assert (GN_IS_WINDOW (self));

  gn_window_set_view (self, self->trash_view, GN_VIEW_MODE_NORMAL);
}

static void
gn_window_show_settings (GnWindow  *self,
                         GtkWidget *widget)
{
  GtkWidget *dialog;

  g_assert (GN_IS_WINDOW (self));

  dialog = gn_settings_dialog_new (GTK_WINDOW (self));

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
gn_window_show_about (GnWindow  *self,
                      GtkWidget *widget)
{
  const gchar *authors[] = {
    "Mohammed Sadiq https://www.sadiqpk.org",
    NULL
  };

  const gchar *artists[] = {
    "William Jon McCann <jmccann@redhat.com>",
    NULL
  };

  g_assert (GN_IS_WINDOW (self));

  gtk_show_about_dialog (GTK_WINDOW (self),
                         "program-name", _("GNOME Notes"),
                         "comments", _("Simple Notes for GNOME"),
                         "website", "https://www.sadiqpk.org",
                         "version", PACKAGE_VERSION,
                         "copyright", "Copyright \xC2\xA9 2018 Mohammed Sadiq",
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "authors", authors,
                         "artists", artists,
                         "logo-icon-name", PACKAGE_ID,
                         "translator-credits", _("translator-credits"),
                         NULL);
}

static void
gn_window_show_previous_view (GnWindow  *self,
                              GtkWidget *widget)
{
  GtkWidget *last_view;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_WIDGET (widget));

  last_view = g_queue_pop_head (self->view_stack);
  gn_window_set_view (self, last_view, GN_VIEW_MODE_NORMAL);
}

static void
gn_window_main_view_changed (GnWindow   *self,
                             GParamSpec *pspec,
                             GtkStack   *main_view)
{
  GtkWidget *child;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_STACK (main_view));

  child = gtk_stack_get_visible_child (main_view);

  if (child == self->notes_view ||
      child == self->notebook_view)
    {
      g_warning ("note or notebook");
      /* If the current view is notes/notebook, reset navigation history */
      g_queue_clear (self->view_stack);
      self->current_view = child;
    }
}

static void
gn_window_item_activated (GnWindow   *self,
                          GnItem     *item,
                          GnMainView *main_view)
{
  GtkWidget *editor;
  GnProvider *provider;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GN_IS_ITEM (item));
  g_assert (GN_IS_MAIN_VIEW (main_view));

  provider = g_object_get_data (G_OBJECT (item), "provider");

  if (GN_IS_NOTE (item))
    {
      GtkWidget *child;

      editor = gn_editor_new ();
      gn_editor_set_item (GN_EDITOR (editor), item);

      child = gtk_bin_get_child (GTK_BIN (self->editor_view));
      if (child != NULL)
        gtk_container_remove (GTK_CONTAINER (self->editor_view), child);

      gn_window_set_title (self, gn_item_get_title (item),
                           gn_provider_get_name (provider));

      gtk_container_add (GTK_CONTAINER (self->editor_view), editor);
      gn_window_set_view (self, self->editor_view, GN_VIEW_MODE_NORMAL);
    }
}

static void
gn_window_set_view_type (GnWindow    *self,
                         const gchar *view_type)
{
  g_assert (GN_IS_WINDOW (self));

  gn_main_view_set_view (GN_MAIN_VIEW (self->current_view), view_type);
}

static void
gn_window_selection_mode_toggled (GnWindow  *self,
                                  GtkWidget *widget)
{
  GtkStyleContext *style_context;
  GtkWidget *current_view;
  GtkWidget *title_bar;
  GtkToggleButton *select_button;
  gboolean selection_mode;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_BUTTON (widget));

  select_button = GTK_TOGGLE_BUTTON (self->select_button);
  selection_mode = gtk_toggle_button_get_active (select_button);

  title_bar = gtk_window_get_titlebar (GTK_WINDOW (self));
  style_context = gtk_widget_get_style_context (title_bar);

  current_view = self->current_view;
  gn_main_view_set_selection_mode (GN_MAIN_VIEW (current_view),
                                   selection_mode);

  if (selection_mode)
    {
      gtk_widget_show (self->main_action_bar);
      gtk_style_context_add_class (style_context, "selection-mode");
      gn_window_set_title (self, _("Click on items to select them"),
                           NULL);
    }
  else
    {
      gtk_style_context_remove_class (style_context, "selection-mode");
      gtk_widget_hide (self->main_action_bar);
      gn_window_set_title (self, NULL, NULL);
    }

  gtk_header_bar_set_show_title_buttons (GTK_HEADER_BAR (self->secondary_header_bar),
                                         !selection_mode);
}

static void
gn_window_update_header_bar (GnWindow  *self,
                             GtkWidget *view)
{
  if (view == self->editor_view)
    {
    }
  else if (view == self->trash_view)
    {
      gn_window_set_title (self, _("Trash"), NULL);
    }
  else  /* notebooks or notes */
    {
      gn_window_set_title (self, NULL, NULL);
    }
  /* TODO: handle notes inside notebooks view */
}

static void
gn_window_show_view (GnWindow  *self,
                     GtkWidget *view)
{
  gtk_stack_set_visible_child (GTK_STACK (self->main_view), view);
  gn_window_update_header_bar (self, view);
  gn_window_set_view_type (self, "list");
}

static void
gn_window_constructed (GObject *object)
{
  GnWindow *self = GN_WINDOW (object);
  GtkWindow *window = GTK_WINDOW (self);
  GnManager *manager;
  GnSettings *settings;
  GdkRectangle geometry;

  manager = gn_manager_get_default ();
  settings = gn_manager_get_settings (manager);

  gn_settings_get_window_geometry (settings, &geometry);
  gtk_window_set_default_size (window, geometry.width, geometry.height);
  gtk_window_move (window, geometry.x, geometry.y);

  if (gn_settings_get_window_maximized (settings))
    gtk_window_maximize (window);

  G_OBJECT_CLASS (gn_window_parent_class)->constructed (object);
}

static void
gn_window_unmap (GtkWidget *widget)
{
  GtkWindow *window = (GtkWindow *)widget;
  GnSettings *settings;
  GdkRectangle geometry;
  gboolean is_maximized;

  settings = gn_manager_get_settings (gn_manager_get_default ());
  is_maximized = gtk_window_is_maximized (window);
  gn_settings_set_window_maximized (settings, is_maximized);

  if (is_maximized)
    return;

  gtk_window_get_size (window, &geometry.width, &geometry.height);
  gtk_window_get_position (window, &geometry.x, &geometry.y);
  gn_settings_set_window_geometry (settings, &geometry);

  GTK_WIDGET_CLASS (gn_window_parent_class)->unmap (widget);
}

static void
gn_window_class_init (GnWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = gn_window_constructed;

  widget_class->unmap = gn_window_unmap;

  g_type_ensure (GN_TYPE_MAIN_VIEW);
  g_type_ensure (GN_TYPE_ACTION_BAR);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-window.ui");

  gtk_widget_class_bind_template_child (widget_class, GnWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, secondary_header_bar);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, search_bar);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, search_entry);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, search_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, main_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, notes_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, notebook_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, editor_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, trash_view);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, new_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, undo_revealer);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, select_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, main_action_bar);

  gtk_widget_class_bind_template_callback (widget_class, gn_window_continue_delete);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_cancel_delete);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_search_changed);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_open_new_note);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_show_previous_view);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_selection_mode_toggled);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_load_more_items);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_main_view_changed);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_item_activated);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_show_trash);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_show_settings);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_show_about);
}

static void
gn_window_init (GnWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->view_stack = g_queue_new ();
  self->current_view = self->notes_view;
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
                    GtkWidget  *view,
                    GnViewMode  mode)
{
  GtkWidget *child;

  g_return_if_fail (GN_IS_WINDOW (self));
  g_return_if_fail (GTK_IS_WIDGET (view));

  if (view == self->current_view)
    return;

  g_queue_push_head (self->view_stack, self->current_view);

  if (self->current_view == self->editor_view)
    {
      child = gtk_bin_get_child (GTK_BIN (self->editor_view));
      if (child != NULL)
        gtk_container_remove (GTK_CONTAINER (self->editor_view), child);
    }

  self->current_view = view;
  gn_window_show_view (self, view);
}

void
gn_window_trash_selected_items (GnWindow *self)
{
  GtkWidget *current_view;
  GnManager *manager;
  GListStore *store;
  GList *items;

  g_return_if_fail (GN_IS_WINDOW (self));

  current_view = self->current_view;
  manager = gn_manager_get_default ();
  items = gn_main_view_get_selected_items (GN_MAIN_VIEW (current_view));

  if (current_view == self->notes_view)
    store = gn_manager_get_notes_store (manager);
  else if (current_view == self->trash_view)
    store = gn_manager_get_trash_notes_store (manager);
  else
    g_assert_not_reached ();

  gn_manager_queue_for_delete (manager, store, items);
  gn_window_show_undo_revealer (self);
}
