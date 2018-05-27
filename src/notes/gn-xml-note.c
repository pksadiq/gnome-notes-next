/* gn-xml-note.c
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

#define G_LOG_DOMAIN "gn-xml-note"

#include "config.h"

#include <gtk/gtk.h>

#include "gn-utils.h"
#include "gn-xml-note.h"
#include "gn-trace.h"

/**
 * SECTION: gn-xml-note
 * @title: GnXmlNote
 * @short_description: Bijiben XML note class
 * @include: "gn-xml-note.h"
 *
 * This class implements XML notes compatible with Tomboy.
 * Please note that this class isn't for supporting some
 * generic XML DTD.  This do support only the XML tags
 * that are supported by Tomboy (and Bijiben).
 */

/*
 * The XML defined here is different from that XML format
 * understood by Bijiben in the past (till 3.28).
 *
 * Bijiben is said to support HTML5 XML strict schema in the XML
 * file.  Practically we didn't ever have a full HTML5 support.
 * So I decided to reduce the scope and simply use the old Tomboy
 * format, but without breaking the current Bijiben format.
 *
 * TODO: Create the DTD defining the Tomboy format.  Discuss this
 * with tomboy-ng developer.
 * So far the only missing feature in Tomboy XML: numbered lists
 */

struct _GnXmlNote
{
  GnNote parent_instance;

  gchar *raw_content;
  gchar *text_content;
  gchar *markup;
  gchar *title;

  /* TRUE: Bijiben XML.  FALSE: Tomboy XML */
  gboolean is_bijiben;
};

G_DEFINE_TYPE (GnXmlNote, gn_xml_note, GN_TYPE_NOTE)

static void
gn_xml_note_finalize (GObject *object)
{
  GnXmlNote *self = (GnXmlNote *)object;

  GN_ENTRY;

  g_free (self->title);
  g_free (self->raw_content);
  g_free (self->text_content);
  g_free (self->markup);

  G_OBJECT_CLASS (gn_xml_note_parent_class)->finalize (object);

  GN_EXIT;
}

static void
gn_xml_note_update_text_content (GnXmlNote *self)
{
  g_autofree gchar *content;
  g_assert (GN_IS_XML_NOTE (self));

  g_free (self->text_content);

  content = gn_utils_get_text_from_xml (self->raw_content);
  self->text_content = g_utf8_casefold (content, -1);
}

static gchar *
gn_xml_note_get_text_content (GnNote *note)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_NOTE (note));

  if (self->text_content == NULL)
    gn_xml_note_update_text_content (self);

  return g_strdup (self->text_content);
}

static void
gn_xml_note_set_text_content (GnNote      *note,
                              const gchar *content)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_XML_NOTE (self));

  g_free (self->text_content);
  self->text_content = g_strdup (content);
}

static void
gn_xml_note_update_markup (GnXmlNote *self)
{
  g_autofree gchar *content = NULL;

  g_assert (GN_IS_XML_NOTE (self));

  if (self->raw_content == NULL)
    return;

  if (self->is_bijiben)
    content = gn_utils_get_markup_from_bijiben (self->raw_content,
                                                GN_NOTE_MARKUP_LINES_MAX);

  g_free (self->markup);
  self->markup = g_steal_pointer (&content);
}

static gchar *
gn_xml_note_get_markup (GnNote *note)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_NOTE (note));

  if (self->markup == NULL)
    gn_xml_note_update_markup (self);

  return g_strdup (self->markup);
}

gboolean
gn_xml_note_match (GnItem      *item,
                   const gchar *needle)
{
  GnXmlNote *self = GN_XML_NOTE (item);
  g_autofree gchar *content = NULL;
  gboolean match;

  match = GN_ITEM_CLASS (gn_xml_note_parent_class)->match (item, needle);

  if (match)
    return TRUE;

  if (self->text_content == NULL)
    gn_xml_note_update_text_content (self);

  if (strstr (self->text_content, needle) != NULL)
    return TRUE;

  return FALSE;
}

static void
gn_xml_note_class_init (GnXmlNoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GnNoteClass *note_class = GN_NOTE_CLASS (klass);
  GnItemClass *item_class = GN_ITEM_CLASS (klass);

  object_class->finalize = gn_xml_note_finalize;

  note_class->get_raw_content = gn_xml_note_get_text_content;
  note_class->get_text_content = gn_xml_note_get_text_content;
  note_class->set_text_content = gn_xml_note_set_text_content;
  note_class->get_markup = gn_xml_note_get_markup;

  item_class->match = gn_xml_note_match;
}

static void
gn_xml_note_init (GnXmlNote *self)
{
}

/*
 * This is a stupid parser.  All we need to get is the
 * title, and the boundaries of note content.
 * GMarkupParser is an overkill here.
 */
static GnXmlNote *
gn_xml_note_create_from_data (const gchar *data)
{
  g_autofree gchar *title = NULL;
  g_autofree gchar *content = NULL;
  gchar *start, *end, *p;
  GnXmlNote *self;

  g_assert (data != NULL);

  start = strstr (data, "<title>");
  g_return_val_if_fail (start != NULL, NULL);

  /* Let's begin finding the title */
  start = start + strlen ("<title>");

  /* Find the start of </title> (That is, <title> close tag) */
  end = strchr (start, '<');
  g_return_val_if_fail (end != NULL, NULL);

  /*
   * This isn't the real title.  It may have (1), (2), etc. as
   * suffix when there is a conflict with other titles
   */
  title = g_strndup (start, end - start);

  self = g_object_new (GN_TYPE_XML_NOTE,
                       "title", title,
                       NULL);

  /* Skip < in </title> */
  end++;

  /* Now skip < in the following tag (blindly assuming it's <text). */
  start = strchr (end, '<');
  g_return_val_if_fail (start != NULL, self);
  start++;

  /* Skip to the next tag */
  start = strchr (start, '<');
  g_return_val_if_fail (start != NULL, self);
  start++;
  end = start;

  /* Jump over the tag found */
  start = strchr (start, '>');
  g_return_val_if_fail (start != NULL, self);
  start++;

  /*
   * The previously found tag shall either be "note-content"
   * (if note is a tomboy note), or "html" (if note is in old
   * bijiben format).  Handle the content accordingly.
   */
  if (g_str_has_prefix (end, "html"))
    {
      self->is_bijiben = TRUE;

      /* Skip until the end of <body> tag */
      start = strstr (start, "<body");
      g_return_val_if_fail (start != NULL, self);
      start = strchr (start, '>');
      g_return_val_if_fail (start != NULL, self);
      start++;

      end = strstr (start, "</body>");
      g_return_val_if_fail (start != NULL, self);
    }
  else /* "note-content" */
    {
      self->is_bijiben = FALSE;

      start = strchr (start, '>');
      g_return_val_if_fail (start != NULL, self);
      start++;
    }

  self->raw_content = g_strndup (start, end - start);

  return self;
}

/**
 * gn_xml_note_new_from_data:
 * @data (nullable): The raw note content
 *
 * Create a new XML note from the given data.
 *
 * Returns: (transfer full): a new #GnXmlNote
 * with given content.
 */
GnXmlNote *
gn_xml_note_new_from_data (const gchar *data)
{
  if (data == NULL)
    return g_object_new (GN_TYPE_XML_NOTE, NULL);

  return gn_xml_note_create_from_data (data);
}
