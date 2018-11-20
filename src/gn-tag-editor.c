/* gn-tag-editor.c
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

#define G_LOG_DOMAIN "gn-tag-editor"

#include "config.h"

#include "gn-tag-store.h"
#include "gn-tag-row.h"
#include "gn-tag-editor.h"
#include "gn-trace.h"

/**
 * SECTION: gn-tag-editor
 * @title: GnTagEditor
 * @short_description: A dialog to select/edit note tags
 * @include: "gn-tag-editor.h"
 */

struct _GnTagEditor
{
  GtkDialog parent_instance;

  GtkWidget *tag_entry;
  GtkWidget *add_button;
  GtkWidget *tags_list;

  GListModel *model;
};

G_DEFINE_TYPE (GnTagEditor, gn_tag_editor, GTK_TYPE_DIALOG)

static void
gn_tag_editor_set_header (GtkListBoxRow *row,
                          GtkListBoxRow *before,
                          gpointer       user_data)
{
  if (before == NULL)
    gtk_list_box_row_set_header (row, NULL);
  else if (gtk_list_box_row_get_header (row) == NULL)
    gtk_list_box_row_set_header (row, gtk_separator_new (0));
}

static void
gn_tag_editor_text_changed (GnTagEditor *self,
                            GParamSpec  *pspec,
                            GtkStack    *main_view)
{
  g_assert (GN_IS_TAG_EDITOR (self));

  if (gtk_entry_get_text_length (GTK_ENTRY (self->tag_entry)) > 0)
    gtk_widget_set_sensitive (self->add_button, TRUE);
  else
    gtk_widget_set_sensitive (self->add_button, FALSE);
}

static void
gn_tag_editor_class_init (GnTagEditorClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-tag-editor.ui");

  gtk_widget_class_bind_template_child (widget_class, GnTagEditor, tag_entry);
  gtk_widget_class_bind_template_child (widget_class, GnTagEditor, add_button);
  gtk_widget_class_bind_template_child (widget_class, GnTagEditor, tags_list);

  gtk_widget_class_bind_template_callback (widget_class, gn_tag_editor_text_changed);
}

static void
gn_tag_editor_init (GnTagEditor *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_list_box_set_header_func (GTK_LIST_BOX (self->tags_list),
                                gn_tag_editor_set_header,
                                self, NULL);
}

GtkWidget *
gn_tag_editor_new (GtkWindow *window)
{
  return g_object_new (GN_TYPE_TAG_EDITOR,
                       "transient-for", window,
                       "use-header-bar", TRUE,
                       NULL);
}

void
gn_tag_editor_set_model (GnTagEditor *self,
                         GListModel  *model)
{
  g_return_if_fail (GN_IS_TAG_EDITOR (self));
  g_return_if_fail (G_IS_LIST_MODEL (model));

  if (!g_set_object (&self->model, model))
    return;

  gtk_list_box_bind_model (GTK_LIST_BOX (self->tags_list), model,
                           gn_tag_row_new,
                           self, NULL);
}
