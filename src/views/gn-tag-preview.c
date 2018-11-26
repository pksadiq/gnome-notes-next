/* gn-tag-preview.c
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

#define G_LOG_DOMAIN "gn-tag-preview"

#include "config.h"

#include "gn-tag-preview.h"
#include "gn-trace.h"

/**
 * SECTION: gn-tag-preview
 * @title: GnTagPreview
 * @short_description: A widget to show the note or notebook
 * @include: "gn-tag-preview.h"
 */

#define INTENSITY(rgb) ((rgb->red) * 0.30 + (rgb->green) * 0.59 + (rgb->blue) * 0.11)

struct _GnTagPreview
{
  GtkLabel parent_instance;

  GnTag *tag;
  GdkRGBA *rgba;
};

G_DEFINE_TYPE (GnTagPreview, gn_tag_preview, GTK_TYPE_LABEL)

static void
gn_tag_preview_update (GnTagPreview *self)
{
  GtkStyleContext *context;
  GdkRGBA rgba;

  g_assert (GN_IS_TAG_PREVIEW (self));
  g_assert (GN_IS_TAG (self->tag));

  gtk_label_set_label (GTK_LABEL (self),
                       gn_tag_get_name (self->tag));

  if (self->rgba == NULL)
    self->rgba = g_slice_new (GdkRGBA);
  else if (gdk_rgba_equal (self->rgba, &rgba))
    return;

  if (gn_tag_get_rgba (self->tag, &rgba))
    *self->rgba = rgba;
  else
    gdk_rgba_parse (self->rgba, "#1C71D8"); /* FIXME: save color to settings? */

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  if (INTENSITY (self->rgba) > 0.5)
    {
      gtk_style_context_add_class (context, "dark");
      gtk_style_context_remove_class (context, "light");
    }
  else
    {
      gtk_style_context_add_class (context, "light");
      gtk_style_context_remove_class (context, "dark");
    }
}

static void
gn_tag_preview_snapshot (GtkWidget *widget,
                         GtkSnapshot *snapshot)
{
  GnTagPreview *self = (GnTagPreview *)widget;
  GskRoundedRect rounded_rect;
  graphene_size_t border = {4, 4}; /* border radius */
  gint width, height;
  gint alloc_width, alloc_height;
  gint diff_width, diff_height;

  width = gtk_widget_get_width (widget);
  height = gtk_widget_get_height (widget);
  alloc_width = gtk_widget_get_allocated_width (widget);
  alloc_height = gtk_widget_get_allocated_height (widget);

  diff_width = alloc_width - width;
  diff_height = alloc_height - height;

  gtk_snapshot_offset (snapshot, -diff_width / 2, -diff_height / 2);
  gsk_rounded_rect_init (&rounded_rect,
                         &GRAPHENE_RECT_INIT (0, 0, alloc_width, alloc_height),
                         &border, &border, &border, &border);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
  gtk_snapshot_append_color (snapshot, self->rgba,
                             &GRAPHENE_RECT_INIT (0, 0, alloc_width, alloc_height));
  gtk_snapshot_pop (snapshot);
  gtk_snapshot_offset (snapshot, diff_width / 2, diff_height / 2);

  GTK_WIDGET_CLASS (gn_tag_preview_parent_class)->snapshot (widget, snapshot);
}

static void
gn_tag_preview_class_init (GnTagPreviewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->snapshot = gn_tag_preview_snapshot;
}

static void
gn_tag_preview_init (GnTagPreview *self)
{
}

GtkWidget *
gn_tag_preview_new (GnTag *tag)
{
  GnTagPreview *self;

  g_return_val_if_fail (GN_IS_TAG (tag), NULL);

  self = g_object_new (GN_TYPE_TAG_PREVIEW,
                      NULL);
  self->tag = tag;
  gn_tag_preview_update (self);

  return GTK_WIDGET (self);
}

