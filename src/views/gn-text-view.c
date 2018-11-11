/* gn-text-view.c
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

#define G_LOG_DOMAIN "gn-text-view"

#include "config.h"

#include "gn-note-buffer.h"
#include "gn-text-view.h"
#include "gn-trace.h"

/**
 * SECTION: gn-text-view
 * @title: GnTextView
 * @short_description: The Note editor view
 * @include: "gn-text-view.h"
 */

#define MAX_UNDO_LEVEL 100

struct _GnTextView
{
  GtkTextView parent_instance;

  GnNoteBuffer *buffer;

  GQueue *undo_queue;
  GList  *current_undo;
  guint   can_undo : 1;
  guint   can_redo : 1;
};

G_DEFINE_TYPE (GnTextView, gn_text_view, GTK_TYPE_TEXT_VIEW)

enum {
  PROP_0,
  PROP_CAN_UNDO,
  PROP_CAN_REDO,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
gn_text_view_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GnTextView *self = (GnTextView *)object;

  switch (prop_id)
    {
    case PROP_CAN_UNDO:
      g_value_set_boolean (value, self->can_undo);
      break;

    case PROP_CAN_REDO:
      g_value_set_boolean (value, self->can_redo);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_text_view_constructed (GObject *object)
{
  GnTextView *self = GN_TEXT_VIEW (object);

  self->buffer = gn_note_buffer_new ();
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (self),
                            GTK_TEXT_BUFFER (self->buffer));

  G_OBJECT_CLASS (gn_text_view_parent_class)->constructed (object);
}

static void
gn_text_view_finalize (GObject *object)
{
  GnTextView *self = GN_TEXT_VIEW (object);

  g_object_unref (self->buffer);

  G_OBJECT_CLASS (gn_text_view_parent_class)->finalize (object);
}

static void
gn_text_view_class_init (GnTextViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gn_text_view_get_property;
  object_class->constructed = gn_text_view_constructed;
  object_class->finalize = gn_text_view_finalize;

  properties[PROP_CAN_UNDO] =
    g_param_spec_boolean ("can-undo",
                          "Can undo",
                          "If undo can be done or not",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_CAN_REDO] =
    g_param_spec_boolean ("can-redo",
                          "Can redo",
                          "If redo can be done or not",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gn_text_view_init (GnTextView *self)
{
}

GtkWidget *
gn_text_view_new (void)
{
  return g_object_new (GN_TYPE_TEXT_VIEW,
                       NULL);
}
