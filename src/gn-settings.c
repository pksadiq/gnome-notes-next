/* gn-settings.c
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

#define G_LOG_DOMAIN "gn-settings"

#include "config.h"

#include "gn-settings.h"
#include "gn-trace.h"

/**
 * SECTION: gn-settings
 * @title: GnSettings
 * @short_description:
 * @include: "gn-settings.h"
 */

struct _GnSettings
{
  GSettings parent_instance;

  gchar *color;
  GdkRGBA rgba;

  gchar *font_name;
  gchar *provider;
  gboolean use_system_font;
};

G_DEFINE_TYPE (GnSettings, gn_settings, G_TYPE_SETTINGS)

enum {
  PROP_0,
  PROP_FONT,
  PROP_USE_SYSTEM_FONT,
  PROP_COLOR,
  PROP_PROVIDER,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
gn_settings_set_font (GnSettings  *self,
                      const gchar *font_name)
{
  g_assert (GN_IS_SETTINGS (self));
  g_assert (font_name != NULL);

  g_clear_pointer (&self->font_name, g_free);
  self->font_name = g_strdup (font_name);

  g_settings_set_string (G_SETTINGS (self), "font", font_name);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FONT]);
}

static void
gn_settings_set_use_system_font (GnSettings *self,
                                 gboolean    use_system_font)
{
  g_assert (GN_IS_SETTINGS (self));

  use_system_font = !!use_system_font;

  if (self->use_system_font == use_system_font)
    return;

  self->use_system_font = use_system_font;

  if (use_system_font)
    {
      g_autoptr(GSettings) desktop_settings = NULL;
      g_autofree gchar *font_name = NULL;

      desktop_settings = g_settings_new ("org.gnome.desktop.interface");
      font_name = g_settings_get_string (desktop_settings, "document-font-name");

      gn_settings_set_font (self, font_name);
    }

  g_settings_set_boolean (G_SETTINGS (self),
                          "use-system-font", self->use_system_font);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_USE_SYSTEM_FONT]);
}

static void
gn_settings_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GnSettings *self = (GnSettings *)object;
  GdkRGBA rgba;

  switch (prop_id)
    {
    case PROP_FONT:
      g_value_set_string (value, self->font_name);
      break;

    case PROP_USE_SYSTEM_FONT:
      g_value_set_boolean (value, self->use_system_font);
      break;

    case PROP_COLOR:
      gn_settings_get_rgba (self, &rgba);
      g_value_set_boxed (value, &rgba);
      break;

    case PROP_PROVIDER:
      g_value_set_string (value, self->provider);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_settings_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GnSettings *self = (GnSettings *)object;

  switch (prop_id)
    {
    case PROP_FONT:
      gn_settings_set_font_name (self, g_value_get_string (value));
      break;

    case PROP_USE_SYSTEM_FONT:
      g_message ("SET use system font: %d", g_value_get_boolean (value));
      gn_settings_set_use_system_font (self, g_value_get_boolean (value));
      break;

    case PROP_COLOR:
      gn_settings_set_rgba (self, g_value_get_boxed (value));
      break;

    case PROP_PROVIDER:
      gn_settings_set_provider_name (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_settings_constructed (GObject *object)
{
  GnSettings *self = (GnSettings *)object;
  GSettings *settings = G_SETTINGS (self);

  G_OBJECT_CLASS (gn_settings_parent_class)->constructed (object);

  g_settings_delay (settings);

  self->color = g_settings_get_string (settings, "color");
  if (!gdk_rgba_parse (&self->rgba, self->color))
    g_critical ("Color %s is an invalid color", self->color);

  self->provider = g_settings_get_string (settings, "provider");
  self->font_name = g_settings_get_string (settings, "font");
  self->use_system_font = g_settings_get_boolean (settings, "use-system-font");
}

static void
gn_settings_dispose (GObject *object)
{
  GSettings *settings = (GSettings *)object;

  g_settings_set_boolean (settings, "first-run", FALSE);
  g_settings_apply (settings);

  G_OBJECT_CLASS (gn_settings_parent_class)->dispose (object);
}

static void
gn_settings_finalize (GObject *object)
{
  GnSettings *self = (GnSettings *)object;

  GN_ENTRY;

  g_free (self->font_name);
  g_free (self->color);
  g_free (self->provider);

  G_OBJECT_CLASS (gn_settings_parent_class)->finalize (object);

  GN_EXIT;
}

static void
gn_settings_class_init (GnSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gn_settings_get_property;
  object_class->set_property = gn_settings_set_property;
  object_class->constructed = gn_settings_constructed;
  object_class->dispose = gn_settings_dispose;
  object_class->finalize = gn_settings_finalize;

  properties[PROP_FONT] =
    g_param_spec_string ("font",
                         "Font name",
                         "The default font name for notes",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                         G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_USE_SYSTEM_FONT] =
    g_param_spec_boolean ("use-system-font",
                          "Use System Font",
                          "Use default system font as the font for notes",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_COLOR] =
    g_param_spec_boxed ("color",
                        "Color",
                        "The default color of new items",
                        GDK_TYPE_RGBA,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                        G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_PROVIDER] =
    g_param_spec_string ("provider",
                         "Provider",
                         "The default provider name",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                         G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gn_settings_init (GnSettings *self)
{
}

/**
 * gn_settings_new:
 * @schema_id: An application id as string
 *
 * Create a new settings for the given application
 * id @schema_id.  @schema_ids are usually of the form
 * “org.example.AppName”.
 *
 * Returns: (transfer full): A #GnSettings.
 * Free with g_object_unref().
 */
GnSettings *
gn_settings_new (const gchar *schema_id)
{
  g_assert (schema_id != NULL);

  return g_object_new (GN_TYPE_SETTINGS,
                       "schema-id", schema_id,
                       NULL);
}

/**
 * gn_settings_get_is_first_run:
 * @self: A #GnSettings
 *
 * Get if the application has ever launched after install.
 * Updating the application to a new version won’t reset
 * this flag.
 *
 * Returns: %TRUE for the first launch of application.
 * %FALSE otherwise.
 */
gboolean
gn_settings_get_is_first_run (GnSettings *self)
{
  g_return_val_if_fail (GN_IS_SETTINGS (self), FALSE);

  return g_settings_get_boolean (G_SETTINGS (self), "first-run");
}

/**
 * gn_settings_get_window_maximized:
 * @self: A #GnSettings
 *
 * Get the window maximized state as saved in @self.
 *
 * Returns: %TRUE if maximized.  %FALSE otherwise.
 */
gboolean
gn_settings_get_window_maximized (GnSettings *self)
{
  g_return_val_if_fail (GN_IS_SETTINGS (self), TRUE);

  return g_settings_get_boolean (G_SETTINGS (self), "window-maximized");
}

/**
 * gn_settings_set_window_maximized:
 * @self: A #GnSettings
 * @maximized: The window state to save
 *
 * Set the window maximized state in @self.
 */
void
gn_settings_set_window_maximized (GnSettings *self,
                                  gboolean    maximized)
{
  g_return_if_fail (GN_IS_SETTINGS (self));

  g_settings_set_boolean (G_SETTINGS (self), "window-maximized", !!maximized);
}

/**
 * gn_settings_get_window_geometry:
 * @self: A #GnSettings
 * @geometry: (out): A #GdkRectangle
 *
 * Get the window geometry as saved in @self.
 */
void
gn_settings_get_window_geometry (GnSettings   *self,
                                 GdkRectangle *geometry)
{
  GdkRectangle size;
  GSettings *settings;

  g_return_if_fail (GN_IS_SETTINGS (self));
  g_return_if_fail (geometry != NULL);

  settings = G_SETTINGS (self);
  g_settings_get (settings, "window-size", "(ii)", &size.width, &size.height);
  g_settings_get (settings, "window-position", "(ii)", &size.x, &size.y);

  *geometry = size;
}

/**
 * gn_settings_set_window_geometry:
 * @self: A #GnSettings
 * @geometry: A #GdkRectangle
 *
 * Set the window geometry in @self.
 */
void
gn_settings_set_window_geometry (GnSettings   *self,
                                 GdkRectangle *geometry)
{
  GSettings *settings;

  g_return_if_fail (GN_IS_SETTINGS (self));
  g_return_if_fail (geometry != NULL);

  settings = G_SETTINGS (self);

  g_settings_set (settings, "window-size", "(ii)", geometry->width, geometry->height);
  g_settings_set (settings, "window-position", "(ii)", geometry->x, geometry->y);
}

/**
 * gn_settings_get_rgba:
 * @self: A #GnSettings
 * @rgba: (out): A #GdkRGBA
 *
 * Get the default color for notes
 */
void
gn_settings_get_rgba (GnSettings *self,
                      GdkRGBA    *rgba)
{
  g_return_if_fail (GN_IS_SETTINGS (self));
  g_return_if_fail (rgba != NULL);

  *rgba = self->rgba;
}

/**
 * gn_settings_set_rgba:
 * @self: A #GnSettings
 * @rgba: A #GdkRGBA
 *
 * Set the default color for notes
 */
void
gn_settings_set_rgba (GnSettings    *self,
                      const GdkRGBA *rgba)
{
  g_return_if_fail (GN_IS_SETTINGS (self));
  g_return_if_fail (rgba != NULL);

  if (gdk_rgba_equal (&self->rgba, rgba))
    return;

  g_free (self->color);
  self->color = gdk_rgba_to_string (rgba);

  self->rgba = *rgba;

  g_settings_set_string (G_SETTINGS (self), "color", self->color);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COLOR]);
}

/**
 * gn_settings_get_provider_name:
 * @self: A #GnSettings
 *
 * Get the default provider name for saving notes.
 * It is user's responsibility to verify that the
 * provider returned is available and usable.
 *
 * Returns: (transfer none): The default font name string
 */
const gchar *
gn_settings_get_provider_name (GnSettings *self)
{
  g_return_val_if_fail (GN_IS_SETTINGS (self), NULL);

  return self->provider;
}

/**
 * gn_settings_set_provider_name:
 * @self: A #GnSettings
 * @name: The default provider name to set as
 * retrieved by gn_provider_get_name()
 *
 * Set the default provider name for saving notes.
 *
 * Returns: %TRUE if @name was set as default.  If
 * @name was already set as the default provider name,
 * %FALSE is returned.
 */
gboolean
gn_settings_set_provider_name (GnSettings  *self,
                               const gchar *name)
{
  GSettings *settings;

  g_return_val_if_fail (GN_IS_SETTINGS (self), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (g_strcmp0 (self->provider, name) == 0)
    return FALSE;

  settings = G_SETTINGS (self);
  g_free (self->provider);
  self->provider = g_strdup (name);
  g_settings_set_string (settings, "provider", name);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PROVIDER]);

  return TRUE;
}

/**
 * gn_settings_get_font_name:
 * @self: A #GnSettings
 *
 * Get the default font name for notes editor
 *
 * Returns: (transfer none): The default font name string
 */
const gchar *
gn_settings_get_font_name (GnSettings *self)
{
  g_return_val_if_fail (GN_IS_SETTINGS (self), NULL);

  return self->font_name;
}

/**
 * gn_settings_set_font_name:
 * @self: A #GnSettings
 * @name: The default font name to set
 *
 * Set the default font to be used for notes editor.
 *
 * Returns: %TRUE if @name was set as default.  If
 * @name was already set as the default font,
 * %FALSE is returned.
 */
gboolean
gn_settings_set_font_name (GnSettings  *self,
                           const gchar *name)
{
  g_return_val_if_fail (GN_IS_SETTINGS (self), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (g_strcmp0 (self->font_name, name) == 0)
    return FALSE;

  gn_settings_set_use_system_font (self, FALSE);
  gn_settings_set_font (self, name);

  return TRUE;
}
