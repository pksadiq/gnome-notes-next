/* gn-application.c
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

#define G_LOG_DOMAIN "gn-application"

#include "config.h"

#include <glib/gi18n.h>

#include "gn-settings.h"
#include "gn-settings-dialog.h"
#include "gn-manager.h"
#include "gn-utils.h"
#include "gn-window.h"
#include "gn-application.h"
#include "gn-trace.h"

/**
 * SECTION: gn-application
 * @title: gn-application
 * @short_description: Base Application class
 * @include: "gn-application.h"
 */

struct _GnApplication
{
  GtkApplication  parent_instance;

  GtkCssProvider *css_provider;
  GnSettings     *settings;
};

G_DEFINE_TYPE (GnApplication, gn_application, GTK_TYPE_APPLICATION)

static GOptionEntry cmd_options[] = {
  {
    "quit", 'q', G_OPTION_FLAG_NONE,
    G_OPTION_ARG_NONE, NULL,
    N_("Quit all running instances of the application"), NULL
  },
  {
    "version", 0, G_OPTION_FLAG_NONE,
    G_OPTION_ARG_NONE, NULL,
    N_("Show release version"), NULL
  },
  { NULL }
};

static void
gn_application_show_trash (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
  GList *windows;
  GnWindow *window = NULL;

  windows = gtk_application_get_windows (user_data);

  for (GList *node = windows; node != NULL; node = node->next)
    {
      window = GN_WINDOW (node->data);

      if (gn_window_get_mode (window) != GN_VIEW_MODE_DETACHED)
          break;
    }

  g_return_if_fail (window != NULL);

  gn_window_set_view (window, GN_VIEW_TRASH, GN_VIEW_MODE_NORMAL);
  gtk_window_present (GTK_WINDOW (window));
}

static void
gn_application_show_settings (GSimpleAction *action,
                              GVariant      *parameter,
                              gpointer       user_data)
{
  GtkWindow *window;
  GtkWidget *dialog;

  window = gtk_application_get_active_window (user_data);
  dialog = gn_settings_dialog_new (window);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
gn_application_show_about (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
  GtkApplication *application = GTK_APPLICATION (user_data);
  const gchar *authors[] = {
    "Mohammed Sadiq",
    NULL
  };

  const gchar *artists[] = {
    "William Jon McCann <jmccann@redhat.com>",
    NULL
  };

  gtk_show_about_dialog (gtk_application_get_active_window (application),
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
gn_application_quit (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  g_application_quit (G_APPLICATION (user_data));
}

static const GActionEntry application_entries[] = {
  { "trash", gn_application_show_trash },
  { "settings", gn_application_show_settings },
  { "about", gn_application_show_about },
  { "quit",  gn_application_quit       },
};

static void
gn_application_finalize (GObject *object)
{
  GnApplication *self = (GnApplication *)object;

  GN_ENTRY;

  g_clear_object (&self->settings);

  GN_EXIT;
}

static gint
gn_application_handle_local_options (GApplication *application,
                                     GVariantDict *options)
{
  if (g_variant_dict_contains (options, "version"))
    {
      g_print ("%s %s\n", PACKAGE, PACKAGE_VERSION);
      return 0;
    }

  return -1;
}

static void
gn_application_startup (GApplication *application)
{
  GnApplication *self = (GnApplication *)application;

  G_APPLICATION_CLASS (gn_application_parent_class)->startup (application);

  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   application_entries,
                                   G_N_ELEMENTS (application_entries),
                                   application);
  if (self->css_provider == NULL)
    {
      g_autoptr(GFile) file = NULL;

      self->css_provider = gtk_css_provider_new ();
      file = g_file_new_for_uri ("resource:///org/sadiqpk/notes/css/style.css");
      gtk_css_provider_load_from_file (self->css_provider, file);

      gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                                  GTK_STYLE_PROVIDER (self->css_provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
}

static gint
gn_application_command_line (GApplication            *application,
                             GApplicationCommandLine *command_line)
{
  GVariantDict *options;

  options = g_application_command_line_get_options_dict (command_line);

  if (g_variant_dict_contains (options, "quit"))
    {
      g_application_quit (application);
      return 0;
    }

  g_application_activate (application);

  return 0;
}

static void
gn_application_activate (GApplication *application)
{
  GtkWindow *window;

  window = gtk_application_get_active_window (GTK_APPLICATION (application));

  if (window == NULL)
    window = GTK_WINDOW (gn_window_new (GN_APPLICATION (application)));

  gtk_window_present (window);
}

static void
gn_application_shutdown (GApplication *application)
{
  g_object_run_dispose (G_OBJECT (gn_manager_get_default ()));

  G_APPLICATION_CLASS (gn_application_parent_class)->shutdown (application);
}

static void
gn_application_class_init (GnApplicationClass *klass)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_assert (GN_IS_MAIN_THREAD ());

  object_class->finalize = gn_application_finalize;

  application_class->handle_local_options = gn_application_handle_local_options;
  application_class->startup = gn_application_startup;
  application_class->command_line = gn_application_command_line;
  application_class->activate = gn_application_activate;
  application_class->shutdown = gn_application_shutdown;
}

static void
gn_application_init (GnApplication *self)
{
  g_application_add_main_option_entries (G_APPLICATION (self), cmd_options);

  g_set_application_name (_("GNOME Notes"));
  gtk_window_set_default_icon_name (PACKAGE_ID);
}

GnApplication *
gn_application_new (void)
{
  return g_object_new (GN_TYPE_APPLICATION,
                       "application-id", PACKAGE_ID,
                       "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                       NULL);
}
