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

#include "gn-enums.h"
#include "gn-note.h"
#include "gn-note-buffer.h"
#include "gn-manager.h"
#include "gn-text-view.h"
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

  GnSettings *settings;

  GnItem        *item;
  GListModel    *model;
  GtkTextBuffer *note_buffer;

  GtkWidget *editor_view;

  GtkWidget *bold_button;
  GtkWidget *italic_button;
  GtkWidget *strikethrough_button;
  GtkWidget *underline_button;

  GtkWidget *editor_menu;
  GtkWidget *detach_button;

  guint save_timeout_id;
};

G_DEFINE_TYPE (GnEditor, gn_editor, GTK_TYPE_GRID)

static void
gn_editor_selection_changed_cb (GnEditor *self)
{
  g_assert (GN_IS_EDITOR (self));

  if (gtk_text_buffer_get_has_selection (self->note_buffer))
    {
      if (gn_item_get_features (self->item) & GN_FEATURE_FORMAT)
        gtk_widget_set_sensitive (self->bold_button, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (self->bold_button, FALSE);
    }
}

static void
gn_editor_format_clicked (GnEditor  *self,
                          GtkWidget *widget)
{
  const gchar *tag_name;
  g_assert (GN_IS_EDITOR (self));
  g_assert (GTK_IS_WIDGET (widget));

  if (widget == self->bold_button)
    tag_name = "bold";
  else if (widget == self->italic_button)
    tag_name = "italic";
  else if (widget == self->underline_button)
    tag_name = "underline";
  else if (widget == self->strikethrough_button)
    tag_name = "strikethrough";
  else
    g_return_if_reached ();

  gn_note_buffer_apply_tag (GN_NOTE_BUFFER (self->note_buffer), tag_name);
}

static void
gn_editor_remove_format_clicked (GnEditor *self)
{
  g_assert (GN_IS_EDITOR (self));

  gn_note_buffer_remove_all_tags (GN_NOTE_BUFFER (self->note_buffer));
}

static void
gn_editor_undo (GnEditor *self)
{
  g_assert (GN_IS_EDITOR (self));
}

static void
gn_editor_redo (GnEditor *self)
{
  g_assert (GN_IS_EDITOR (self));
}

static gboolean
gn_editor_save_note (gpointer user_data)
{
  GnEditor *self = (GnEditor *)user_data;
  GnManager *manager;

  GN_ENTRY;

  g_assert (GN_IS_EDITOR (self));

  self->save_timeout_id = 0;
  manager = gn_manager_get_default ();

  gn_note_set_content_from_buffer (GN_NOTE (self->item), self->note_buffer);
  gtk_text_buffer_set_modified (self->note_buffer, FALSE);
  gn_manager_save_item (manager, self->item);

  GN_RETURN (G_SOURCE_REMOVE);
}

static void
gn_editor_buffer_modified_cb (GnEditor      *self,
                              GtkTextBuffer *buffer)
{
  g_assert (GN_IS_EDITOR (self));
  g_assert (GTK_IS_TEXT_BUFFER (buffer));

  if (!gtk_text_buffer_get_modified (buffer) ||
      self->item == NULL)
    return;

  if (self->save_timeout_id == 0)
    self->save_timeout_id = g_timeout_add (SAVE_TIMEOUT,
                                           gn_editor_save_note, self);
}

static void
gn_editor_dispose (GObject *object)
{
  GnEditor *self = (GnEditor *)object;

  if (self->save_timeout_id != 0)
    {
      g_source_remove (self->save_timeout_id);
      self->save_timeout_id = 0;
      gn_editor_save_note (self);
    }

  G_OBJECT_CLASS (gn_editor_parent_class)->dispose (object);
}

static void
gn_editor_class_init (GnEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = gn_editor_dispose;

  g_type_ensure (GN_TYPE_TEXT_VIEW);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-editor.ui");

  gtk_widget_class_bind_template_child (widget_class, GnEditor, editor_view);
  gtk_widget_class_bind_template_child (widget_class, GnEditor, bold_button);
  gtk_widget_class_bind_template_child (widget_class, GnEditor, italic_button);
  gtk_widget_class_bind_template_child (widget_class, GnEditor, strikethrough_button);
  gtk_widget_class_bind_template_child (widget_class, GnEditor, underline_button);
  gtk_widget_class_bind_template_child (widget_class, GnEditor, editor_menu);
  gtk_widget_class_bind_template_child (widget_class, GnEditor, detach_button);

  gtk_widget_class_bind_template_callback (widget_class, gn_editor_format_clicked);
  gtk_widget_class_bind_template_callback (widget_class, gn_editor_remove_format_clicked);
  gtk_widget_class_bind_template_callback (widget_class, gn_editor_undo);
  gtk_widget_class_bind_template_callback (widget_class, gn_editor_redo);
}

static void
gn_editor_init (GnEditor *self)
{
  GnManager *manager;
  GtkTextTag *font_tag;
  GtkTextTagTable *tag_table;
  GtkTextIter start, end;

  gtk_widget_init_template (GTK_WIDGET (self));

  manager = gn_manager_get_default ();
  self->note_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->editor_view));
  self->settings = gn_manager_get_settings (manager);

  g_signal_connect_object (self->note_buffer, "notify::has-selection",
                           G_CALLBACK (gn_editor_selection_changed_cb),
                           self, G_CONNECT_SWAPPED);
  gn_editor_selection_changed_cb (self);

  g_signal_connect_object (self->note_buffer, "modified-changed",
                           G_CALLBACK (gn_editor_buffer_modified_cb),
                           self, G_CONNECT_SWAPPED);

  tag_table = gtk_text_buffer_get_tag_table (self->note_buffer);
  font_tag = gtk_text_tag_table_lookup (tag_table, "font");

  g_object_bind_property (self->settings, "font",
                          font_tag, "font",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  gtk_text_buffer_get_bounds (self->note_buffer, &start, &end);
  gtk_text_buffer_apply_tag (self->note_buffer, font_tag, &start, &end);

}

GtkWidget *
gn_editor_new (void)
{
  return g_object_new (GN_TYPE_EDITOR,
                       NULL);
}

void
gn_editor_set_item (GnEditor   *self,
                    GListModel *model,
                    GnItem     *item)
{
  GN_ENTRY;

  g_return_if_fail (GN_IS_EDITOR (self));
  g_return_if_fail (!model || G_IS_LIST_MODEL (model));
  g_return_if_fail (!item || GN_IS_ITEM (item));

  if (self->item == item)
    GN_EXIT;

  if (self->save_timeout_id != 0)
    g_source_remove (self->save_timeout_id);

  if (self->item)
    gn_editor_save_note (self);

  self->item = item;
  self->model = model;

  g_object_freeze_notify (G_OBJECT (self->note_buffer));

  if (item == NULL)
    gtk_text_buffer_set_text (GTK_TEXT_BUFFER (self->note_buffer), "", 0);
  else
    gn_note_set_content_to_buffer (GN_NOTE (item),
                                   GN_NOTE_BUFFER (self->note_buffer));

  g_object_thaw_notify (G_OBJECT (self->note_buffer));

  GN_EXIT;
}

GnNote *
gn_editor_get_note (GnEditor *self)
{
  g_return_val_if_fail (GN_IS_EDITOR (self), NULL);

  return GN_NOTE (self->item);
}

GListModel *
gn_editor_get_model (GnEditor *self)
{
  g_return_val_if_fail (GN_IS_EDITOR (self), NULL);

  return self->model;
}

GtkWidget *
gn_editor_get_menu (GnEditor *self)
{
  g_return_val_if_fail (GN_IS_EDITOR (self), NULL);

  return self->editor_menu;
}

void
gn_editor_set_detached (GnEditor *self,
                        gboolean  detached)
{
  g_return_if_fail (GN_IS_EDITOR (self));

  if (detached)
    gtk_widget_hide (self->detach_button);
  else
    gtk_widget_show (self->detach_button);
}
