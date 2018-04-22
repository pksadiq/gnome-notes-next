/* gn-editor.c
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

#define G_LOG_DOMAIN "gn-editor"

#include "config.h"

#include "gn-note.h"
#include "gn-note-buffer.h"
#include "gn-provider-item.h"
#include "gn-manager.h"
#include "gn-editor.h"
#include "gn-trace.h"

/**
 * SECTION: gn-editor
 * @title: GnEditor
 * @short_description: The Note editor view
 * @include: "gn-editor.h"
 */

#define SAVE_TIMEOUT 2000       /* milliseconds */

struct _GnEditor
{
  GtkGrid parent_instance;

  GnProviderItem *provider_item;
  GtkTextBuffer  *note_buffer;

  GtkWidget *editor_view;
  GtkWidget *cut_button;
};

G_DEFINE_TYPE (GnEditor, gn_editor, GTK_TYPE_GRID)

static void
gn_editor_copy_or_cut (GnEditor  *self,
                       GtkWidget *button)
{
  GtkClipboard *clipboard;

  g_assert (GN_IS_EDITOR (self));
  g_assert (GTK_IS_BUTTON (button));

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (self),
                                        GDK_SELECTION_CLIPBOARD);

  if (button == self->cut_button)
    gtk_text_buffer_cut_clipboard (self->note_buffer, clipboard, TRUE);
  else /* copy_button */
    gtk_text_buffer_copy_clipboard (self->note_buffer, clipboard);

}

static void
gn_editor_paste (GnEditor *self)
{
  GtkClipboard *clipboard;

  g_assert (GN_IS_EDITOR (self));

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (self),
                                        GDK_SELECTION_CLIPBOARD);
  gtk_text_buffer_paste_clipboard (self->note_buffer, clipboard,
                                   NULL, TRUE);
}

static gboolean
gn_editor_save_note (gpointer user_data)
{
  GnEditor *self = (GnEditor *)user_data;
  GnManager *manager;
  GnNote *note;

  GN_ENTRY;

  g_assert (GN_IS_EDITOR (self));

  manager = gn_manager_get_default ();
  note = GN_NOTE (gn_provider_item_get_item (self->provider_item));

  gn_note_set_content_from_buffer (note, self->note_buffer);
  gtk_text_buffer_set_modified (self->note_buffer, FALSE);
  gn_manager_save_item (manager, self->provider_item);

  GN_RETURN (G_SOURCE_REMOVE);
}

static void
gn_editor_buffer_modified_cb (GnEditor      *self,
                              GtkTextBuffer *buffer)
{
  g_assert (GN_IS_EDITOR (self));
  g_assert (GTK_IS_TEXT_BUFFER (buffer));

  if (!gtk_text_buffer_get_modified (buffer))
    return;

  g_timeout_add (SAVE_TIMEOUT, gn_editor_save_note, self);
}

static void
gn_editor_class_init (GnEditorClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-editor.ui");

  gtk_widget_class_bind_template_child (widget_class, GnEditor, editor_view);
  gtk_widget_class_bind_template_child (widget_class, GnEditor, cut_button);

  gtk_widget_class_bind_template_callback (widget_class, gn_editor_copy_or_cut);
  gtk_widget_class_bind_template_callback (widget_class, gn_editor_paste);
}

static void
gn_editor_init (GnEditor *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
gn_editor_new (void)
{
  return g_object_new (GN_TYPE_EDITOR,
                       NULL);
}

void
gn_editor_set_item (GnEditor       *self,
                    GnProviderItem *provider_item)
{
  g_autoptr(GtkTextBuffer) buffer = NULL;
  GnItem *item;

  GN_ENTRY;

  g_return_if_fail (GN_IS_EDITOR (self));
  g_return_if_fail (GN_IS_PROVIDER_ITEM (provider_item));

  if (self->provider_item != NULL)
    GN_EXIT;

  item = gn_provider_item_get_item (provider_item);
  buffer = gn_note_get_buffer (GN_NOTE (item));

  self->provider_item = provider_item;
  self->note_buffer = buffer;

  gtk_text_view_set_buffer (GTK_TEXT_VIEW (self->editor_view), buffer);
  g_object_bind_property (buffer, "has-selection",
                          self->cut_button, "sensitive",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
  g_signal_connect_object (buffer, "modified-changed",
                           G_CALLBACK (gn_editor_buffer_modified_cb),
                           self, G_CONNECT_SWAPPED);

  GN_EXIT;
}
