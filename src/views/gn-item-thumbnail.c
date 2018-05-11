/* gn-item-thumbnail.c
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

#define G_LOG_DOMAIN "gn-item-thumbnail"

#include "config.h"

#include "gn-item-thumbnail.h"
#include "gn-trace.h"

/**
 * SECTION: gn-item-thumbnail
 * @title: GnItemThumbnail
 * @short_description: The thumbnail for the item
 * @include: "gn-item-thumbnail.h"
 *
 * Use this as an overlay child of a #GtkOverlay and set
 * the size of overlay based on the requirement.
 */

#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)

struct _GnItemThumbnail
{
  GtkLabel parent_instance;

  GdkRGBA *rgba;
};

G_DEFINE_TYPE (GnItemThumbnail, gn_item_thumbnail, GTK_TYPE_LABEL)

enum {
  PROP_0,
  PROP_RGBA,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static gboolean
gn_item_thumbnail_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  GnItemThumbnail *self = (GnItemThumbnail *)widget;
  GtkStyleContext *context;
  GdkRGBA *rgba;
  gint width, height;

  g_assert (GN_IS_ITEM_THUMBNAIL (self));

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);
  context = gtk_widget_get_style_context (widget);
  rgba = self->rgba;

  if (INTENSITY (rgba->red, rgba->green, rgba->blue) > 0.5)
    {
      gtk_style_context_add_class (context, "dark");
      gtk_style_context_remove_class (context, "light");
    }
  else
    {
      gtk_style_context_add_class (context, "light");
      gtk_style_context_remove_class (context, "dark");
    }

  cairo_rectangle (cr, 0, 0, width, height);
  gdk_cairo_set_source_rgba (cr, self->rgba);
  cairo_fill_preserve (cr);

  /* return GTK_WIDGET_CLASS (gn_item_thumbnail_parent_class)->draw (widget, cr); */
}

static void
gn_item_thumbnail_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GnItemThumbnail *self = (GnItemThumbnail *)object;

  switch (prop_id)
    {
    case PROP_RGBA:
      g_value_set_boxed (value, self->rgba);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_item_thumbnail_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GnItemThumbnail *self = (GnItemThumbnail *)object;

  switch (prop_id)
    {
    case PROP_RGBA:
      gdk_rgba_free (self->rgba);
      self->rgba = g_value_dup_boxed (value);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_item_thumbnail_finalize (GObject *object)
{
  GnItemThumbnail *self = (GnItemThumbnail *)object;

  GN_ENTRY;

  gdk_rgba_free (self->rgba);

  G_OBJECT_CLASS (gn_item_thumbnail_parent_class)->finalize (object);

  GN_EXIT;
}

static void
gn_item_thumbnail_class_init (GnItemThumbnailClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = gn_item_thumbnail_get_property;
  object_class->set_property = gn_item_thumbnail_set_property;
  object_class->finalize = gn_item_thumbnail_finalize;

  /* widget_class->draw = gn_item_thumbnail_draw; */

  properties[PROP_RGBA] =
    g_param_spec_boxed ("rgba",
                        "Rgba",
                        "The GdkRGBA background color of the item",
                        GDK_TYPE_RGBA,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-item-thumbnail.ui");

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gn_item_thumbnail_init (GnItemThumbnail *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GnItemThumbnail *
gn_item_thumbnail_new (const gchar *markup,
                       GdkRGBA     *rgba)
{
  return g_object_new (GN_TYPE_ITEM_THUMBNAIL,
                       "label", markup,
                       "rgba", rgba,
                       NULL);
}
