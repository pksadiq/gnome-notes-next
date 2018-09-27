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

  g_assert (GN_IS_MAIN_THREAD ());

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
