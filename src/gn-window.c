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
  GtkWidget *nav_button_stack;
  GtkWidget *search_button;
  GtkWidget *view_button_stack;
  GtkWidget *menu_button;
  GtkWidget *main_menu;
  GtkWidget *editor_menu;
  GtkWidget *new_editor_button;
  GtkWidget *undo_revealer;

  GtkWidget *select_button;
  GtkWidget *main_action_bar;

  GtkWidget *search_bar;
  GtkWidget *search_entry;
  GtkWidget *search_view;
  GtkWidget *main_view;
  GtkWidget *notes_view;
  GtkWidget *trash_view;
  GtkWidget *editor_view;

  GQueue    *view_stack;
  GtkWidget *current_view;
  gboolean   back_button_pressed;

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

  header_bar = GTK_HEADER_BAR (self->header_bar);

  gtk_header_bar_set_title (header_bar, title);
  gtk_header_bar_set_subtitle (header_bar, subtitle);
}

static void
gn_window_view_button_toggled (GnWindow  *self,
                               GtkWidget *widget)
{
  GtkStack *btn_stack;
  const gchar *view;
  const gchar *other_view;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_WIDGET (widget));

  btn_stack = GTK_STACK (self->view_button_stack);
  view = gtk_stack_get_visible_child_name (btn_stack);
  other_view = gn_utils_get_other_view_type (view);

  gtk_stack_set_visible_child_name (btn_stack, other_view);

  gn_main_view_set_view (GN_MAIN_VIEW (self->current_view),
                         view);
}

static void
gn_window_provider_added_cb (GnWindow   *self,
                             GnProvider *provider,
                             GnManager  *manager)
{
  GListModel *store;

  g_assert (GN_IS_MAIN_THREAD ());
  g_assert (GN_IS_WINDOW (self));
  g_assert (provider == NULL || GN_IS_PROVIDER (provider));

  store = gn_manager_get_notes_store (gn_manager_get_default ());

  gn_main_view_set_view (GN_MAIN_VIEW (self->notes_view),
                         "list");

  gn_main_view_set_model (GN_MAIN_VIEW (self->notes_view),
                          G_LIST_MODEL (store));

  store = gn_manager_get_trash_notes_store (gn_manager_get_default ());

  gn_main_view_set_view (GN_MAIN_VIEW (self->trash_view),
                         "list");

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
  GtkWidget *current_view;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_SCROLLED_WINDOW (scrolled_window));

  /* We load more items only if bottom edge is hit */
  if (pos != GTK_POS_BOTTOM)
    return;

  current_view = gtk_stack_get_visible_child (GTK_STACK (self->main_view));

  if (current_view == self->notes_view)
    gn_manager_load_more_notes (gn_manager_get_default ());
}

static gboolean
gn_window_key_press_cb (GtkEventController *controller,
                        guint               keyval,
                        guint               keycode,
                        GdkModifierType     state,
                        GnWindow           *self)
{
  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_EVENT_CONTROLLER (controller));

  if (self->current_view == self->editor_view)
    return FALSE;

  return gtk_search_bar_handle_event (GTK_SEARCH_BAR (self->search_bar),
                                      gtk_get_current_event ());
}

static void
gn_window_search_changed (GnWindow       *self,
                          GtkSearchEntry *search_entry)
{
  const gchar *needle;
  GnManager *manager;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_SEARCH_ENTRY (search_entry));

  manager = gn_manager_get_default ();
  needle = gtk_entry_get_text (GTK_ENTRY (search_entry));
  gn_manager_search (manager, &needle);

  if (needle[0] != '\0')
    gtk_stack_set_visible_child (GTK_STACK (self->main_view),
                                 self->search_view);
  else
    gtk_stack_set_visible_child (GTK_STACK (self->main_view),
                                 self->notes_view);
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
  gtk_stack_set_visible_child (GTK_STACK (self->main_view),
                               self->editor_view);
}

static void
gn_window_show_previous_view (GnWindow  *self,
                              GtkWidget *widget)
{
  GtkWidget *last_view;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_BUTTON (widget));
  g_assert (!g_queue_is_empty (self->view_stack));

  last_view = g_queue_pop_head (self->view_stack);

  /*
   * This helps us to check if the main view was changed by
   * the user pressing the back button.
   */
  self->back_button_pressed = TRUE;
  gtk_stack_set_visible_child (GTK_STACK (self->main_view),
                               last_view);
  self->back_button_pressed = FALSE;
}

static void
gn_window_show_trash (GnWindow *self)
{
  g_assert (GN_IS_WINDOW (self));

  gtk_stack_set_visible_child (GTK_STACK (self->main_view),
                               self->trash_view);
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
gn_window_main_view_changed (GnWindow   *self,
                             GParamSpec *pspec,
                             GtkStack   *main_view)
{
  GtkWidget *child;
  GtkStack *nav_stack;
  GtkStack *btn_stack;
  const gchar *view;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GTK_IS_STACK (main_view));

  child = gtk_stack_get_visible_child (main_view);
  nav_stack = GTK_STACK (self->nav_button_stack);
  btn_stack = GTK_STACK (self->view_button_stack);
  view = gtk_stack_get_visible_child_name (btn_stack);
  view = gn_utils_get_other_view_type (view);

  if (child == self->notes_view)
    {
      gtk_stack_set_visible_child_name (nav_stack, "new");
      g_queue_clear (self->view_stack);
    }
  else
    {
      gtk_stack_set_visible_child_name (nav_stack, "back");
      if (!self->back_button_pressed &&
          g_queue_peek_head (self->view_stack) != self->current_view)
        g_queue_push_head (self->view_stack, self->current_view);
    }

  self->current_view = child;

  if (child == self->editor_view)
    {
      gtk_widget_hide (self->view_button_stack);
      gtk_widget_hide (self->search_button);
      gtk_widget_hide (self->select_button);
      gtk_menu_button_set_popover (GTK_MENU_BUTTON (self->menu_button),
                                   self->editor_menu);
      gtk_button_set_icon_name (GTK_BUTTON (self->menu_button),
                                "view-more-symbolic");
    }
  else
    {
      gtk_widget_show (self->view_button_stack);
      gtk_widget_show (self->search_button);
      gtk_widget_show (self->select_button);
      gn_main_view_set_view (GN_MAIN_VIEW (child), view);
      gtk_menu_button_set_popover (GTK_MENU_BUTTON (self->menu_button),
                                   self->main_menu);
      gtk_button_set_icon_name (GTK_BUTTON (self->menu_button),
                                "open-menu-symbolic");
    }
}

static GnWindow *
gn_window_get_note_window (GnWindow *self,
                           GnNote   *note)
{
  GtkApplication *application;
  GtkWidget *child;
  GList *windows;

  g_assert (GN_IS_WINDOW (self));
  g_assert (GN_IS_NOTE (note));

  application = GTK_APPLICATION (g_application_get_default());
  windows = gtk_application_get_windows (application);

  for (GList *node = windows; node != NULL; node = node->next)
    {
      GnWindow *window = GN_WINDOW (node->data);

      child = gtk_bin_get_child (GTK_BIN (window->editor_view));

      if (child != NULL &&
          note == gn_editor_get_note (GN_EDITOR (child)))
        return window;
    }

  return NULL;
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
      GnWindow *window;
      GtkWidget *child;

      window = gn_window_get_note_window (self, GN_NOTE (item));

      if (window != NULL)
        {
          gtk_stack_set_visible_child (GTK_STACK (window->main_view),
                                       window->editor_view);
          gtk_window_present (GTK_WINDOW (window));
          return;
        }

      editor = gn_editor_new ();
      gn_editor_set_item (GN_EDITOR (editor), item);

      child = gtk_bin_get_child (GTK_BIN (self->editor_view));
      if (child != NULL)
        gtk_container_remove (GTK_CONTAINER (self->editor_view), child);

      gn_window_set_title (self, gn_item_get_title (item),
                           gn_provider_get_name (provider));

      gtk_container_add (GTK_CONTAINER (self->editor_view), editor);
      gtk_stack_set_visible_child (GTK_STACK (self->main_view),
                                   self->editor_view);
    }
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

  current_view = gtk_stack_get_visible_child (GTK_STACK (self->main_view));
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

  gtk_header_bar_set_show_title_buttons (GTK_HEADER_BAR (self->header_bar),
                                         !selection_mode);
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
  gtk_widget_class_bind_template_child (widget_class, GnWindow, nav_button_stack);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, search_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, view_button_stack);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, menu_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, main_menu);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, editor_menu);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, new_editor_button);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, search_bar);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, search_entry);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, search_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, main_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, notes_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, trash_view);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, editor_view);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, undo_revealer);

  gtk_widget_class_bind_template_child (widget_class, GnWindow, select_button);
  gtk_widget_class_bind_template_child (widget_class, GnWindow, main_action_bar);

  gtk_widget_class_bind_template_callback (widget_class, gn_window_continue_delete);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_cancel_delete);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_search_changed);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_show_previous_view);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_open_new_note);
  gtk_widget_class_bind_template_callback (widget_class, gn_window_view_button_toggled);
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
}

GnWindow *
gn_window_new (GnApplication *application)
{
  GnWindow *self;

  g_assert (GTK_IS_APPLICATION (application));

  self = g_object_new (GN_TYPE_WINDOW,
                       "application", application,
                       NULL);
  gn_window_set_as_main (self);

  return self;
}

GnWindow *
gn_window_new_with_editor (GnApplication *application,
                           GtkWidget     *editor)
{
  GnWindow *self;

  g_assert (GTK_IS_APPLICATION (application));

  self = g_object_new (GN_TYPE_WINDOW,
                       "application", application,
                       NULL);
  gtk_container_add (GTK_CONTAINER (self->editor_view), editor);
  g_object_unref (editor);

  gtk_widget_hide (self->nav_button_stack);
  gtk_stack_set_visible_child (GTK_STACK (self->main_view),
                               self->editor_view);

  return self;
}

void
gn_window_trash_selected_items (GnWindow *self)
{
  GtkWidget *current_view;
  GnManager *manager;
  GListModel *store;
  GList *items;

  g_return_if_fail (GN_IS_WINDOW (self));

  current_view = gtk_stack_get_visible_child (GTK_STACK (self->main_view));
  manager = gn_manager_get_default ();
  items = gn_main_view_get_selected_items (GN_MAIN_VIEW (current_view));

  if (current_view == self->notes_view)
    store = gn_manager_get_notes_store (manager);
  else
    g_assert_not_reached ();

  gn_manager_queue_for_delete (manager, store, items);
  gn_window_show_undo_revealer (self);
}

GtkWidget *
gn_window_steal_editor (GnWindow *self)
{
  GtkWidget *editor;

  g_return_val_if_fail (GN_IS_WINDOW (self), NULL);

  editor = gtk_bin_get_child (GTK_BIN (self->editor_view));
  g_return_val_if_fail (editor != NULL, NULL);
  g_object_ref (editor);

  gtk_stack_set_visible_child (GTK_STACK (self->main_view),
                               self->notes_view);
  gtk_container_remove (GTK_CONTAINER (self->editor_view), editor);

  return editor;
}

void
gn_window_set_as_main (GnWindow *self)
{
  GtkEventController *controller;

  g_assert (GN_IS_WINDOW (self));

  controller = gtk_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (gn_window_key_press_cb),
                    self);
  gtk_widget_add_controller (GTK_WIDGET (self), controller);

  g_signal_connect_object (gn_manager_get_default (),
                           "provider-added",
                           G_CALLBACK (gn_window_provider_added_cb),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_widget_show (self->nav_button_stack);
  gn_window_provider_added_cb (self, NULL, gn_manager_get_default ());
}
