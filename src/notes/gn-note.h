/* gn-note.h
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

#pragma once

#include <gtk/gtk.h>

#include "gn-item.h"
#include "gn-note-buffer.h"

G_BEGIN_DECLS

#define GN_NOTE_MARKUP_LINES_MAX 20
#define GN_TYPE_NOTE (gn_note_get_type ())

G_DECLARE_DERIVABLE_TYPE (GnNote, gn_note, GN, NOTE, GnItem)

struct _GnNoteClass
{
  GnItemClass parent_class;

  gchar *(*get_text_content)        (GnNote        *self);
  void   (*set_text_content)        (GnNote        *self,
                                     const gchar   *content);

  gchar *(*get_raw_content)         (GnNote        *self);
  gchar *(*get_markup)              (GnNote        *self);
  void   (*set_content_from_buffer) (GnNote        *self,
                                     GtkTextBuffer *buffer);
  void   (*set_content_to_buffer)   (GnNote        *self,
                                     GnNoteBuffer  *buffer);
  const gchar   *(*get_extension)   (GnNote        *self);
};

gchar *gn_note_get_text_content        (GnNote        *self);
void   gn_note_set_text_content        (GnNote        *self,
                                        const gchar   *content);

gchar *gn_note_get_raw_content         (GnNote        *self);
gchar *gn_note_get_markup              (GnNote        *self);

void   gn_note_set_content_from_buffer (GnNote        *self,
                                        GtkTextBuffer *buffer);
void   gn_note_set_content_to_buffer   (GnNote        *self,
                                        GnNoteBuffer  *buffer);
const gchar   *gn_note_get_extension   (GnNote        *self);

G_END_DECLS
