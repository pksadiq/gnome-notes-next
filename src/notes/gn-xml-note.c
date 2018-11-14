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
#include "gn-note-buffer.h"
#include "gn-macro.h"
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

#define COMMON_XML_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define BIJIBEN_XML_NS "http://projects.gnome.org/bijiben"
#define TOMBOY_XML_NS  "http://beatniksoftware.com/tomboy"

typedef enum
{
  NOTE_FORMAT_UNKNOWN,
  NOTE_FORMAT_TOMBOY_1,
  NOTE_FORMAT_TOMBOY_2,
  NOTE_FORMAT_TOMBOY_3,
  NOTE_FORMAT_BIJIBEN_1,
  NOTE_FORMAT_BIJIBEN_2,
  N_NOTE_FORMATS
} NoteFormat;

struct _GnXmlNote
{
  GnNote parent_instance;

  xmlTextReader *xml_reader;
  xmlBuffer *xml_buffer;
  xmlTextWriter *xml_writer;
  GString *raw_data;    /* The raw data used to parse, NULL if raw_xml set */
  GString *raw_xml;     /* full XML data to be saved to file */
  gchar   *raw_inner_xml; /* xml data of the <text> tag */
  gchar   *raw_content;   /* full xml data that will be saved as file */
  GString *text_content;
  GString *markup;
  gchar   *title;
  GHashTable *labels;

  /* TRUE: Bijiben XML.  FALSE: Tomboy XML */
  gboolean is_bijiben;
  NoteFormat note_format;
};

G_DEFINE_TYPE (GnXmlNote, gn_xml_note, GN_TYPE_NOTE)

/*
 * gn_xml_note_get_format:
 * @data: A string
 * @length: length of @data
 *
 * Get the format of the XML data passed.
 * @data is not validated, so partial data
 * can be given.
 *
 * Returns: A #NoteFormat
 */
NoteFormat
gn_xml_note_get_format (const gchar *data,
                        gsize        length)
{
  gchar *start, *end;
  g_autofree gchar *note_tag = NULL;
  gboolean is_bijiben, is_tomboy;

  g_return_val_if_fail (data != NULL, NOTE_FORMAT_UNKNOWN);

  /*
   * The length checked is pure artificial, just enough
   * to be safe from the code below which does nasty things
   * without any checks.
   */
  if (length < 100)
    return NOTE_FORMAT_UNKNOWN;

  start = strstr (data, "<note ");
  if (!start)
    return NOTE_FORMAT_UNKNOWN;

  end = strchr (start, '>');
  if (!end)
    return NOTE_FORMAT_UNKNOWN;

  note_tag = g_strndup (start, end - start);

  start = strstr (note_tag, " xmlns=\"");
  if (!start)
    return NOTE_FORMAT_UNKNOWN;
  start += strlen (" xmlns=\"");

  is_bijiben = is_tomboy = FALSE;

  if (g_str_has_prefix (start, BIJIBEN_XML_NS "\""))
    is_bijiben = TRUE;
  else if (g_str_has_prefix (start, TOMBOY_XML_NS "\""))
    is_tomboy = TRUE;
  else
    return NOTE_FORMAT_UNKNOWN;

  start = strstr (note_tag, " version=\"");
  if (!start)
    return NOTE_FORMAT_UNKNOWN;
  start += strlen (" version=\"");

  if (is_bijiben)
    {
      if (g_str_has_prefix (start, "2" "\""))
        return NOTE_FORMAT_BIJIBEN_2;
      else if (g_str_has_prefix (start, "1" "\""))
        return NOTE_FORMAT_BIJIBEN_1;
    }
  else
    {
      if (g_str_has_prefix (start, "0.3" "\""))
        return NOTE_FORMAT_TOMBOY_3;
      else if (g_str_has_prefix (start, "0.2" "\""))
        return NOTE_FORMAT_TOMBOY_2;
      else if (g_str_has_prefix (start, "0.1" "\""))
        return NOTE_FORMAT_TOMBOY_1;
    }

  return NOTE_FORMAT_UNKNOWN;
}

static void
gn_xml_note_update_buffer (GnXmlNote     *self,
                           GtkTextBuffer *buffer,
                           gchar         *start,
                           gchar         *end)
{
  GtkTextIter iter;

  g_assert (GTK_IS_TEXT_BUFFER (buffer));
  g_assert (start != NULL);
  g_assert (end != NULL);

  if (start == end)
    return;

  gtk_text_buffer_get_end_iter (buffer, &iter);

  gtk_text_buffer_insert (buffer, &iter, start, end - start);
}

static void
gn_xml_note_apply_tag_at_mark (GtkTextBuffer *buffer,
                               GtkTextMark   *mark,
                               const gchar   *tag_name)
{
  GtkTextIter start, end;

  gtk_text_buffer_get_end_iter (buffer, &end);
  gtk_text_buffer_get_iter_at_mark (buffer, &start, mark);
  gtk_text_buffer_apply_tag_by_name (buffer, tag_name, &start, &end);
  gtk_text_buffer_delete_mark (buffer, mark);
}

static void
gn_xml_note_set_content_to_buffer (GnNote       *note,
                                   GnNoteBuffer *buffer)
{
  GtkTextMark *mark_bold, *mark_italic;
  GtkTextMark *mark_underline, *mark_strike;
  GtkTextBuffer *text_buffer;
  gchar *start, *end;
  const gchar *title;
  GtkTextIter end_iter;
  gchar c;

  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_XML_NOTE (self));

  text_buffer = GTK_TEXT_BUFFER (buffer);

  title = gn_item_get_title (GN_ITEM (self));
  gtk_text_buffer_set_text (text_buffer, title, -1);

  if (title && self->raw_inner_xml && *self->raw_inner_xml)
    {
      gtk_text_buffer_get_end_iter (text_buffer, &end_iter);
      gtk_text_buffer_insert (text_buffer, &end_iter, "\n", 1);
    }

  start = end = self->raw_inner_xml;

  if (start == NULL)
    return;

  mark_bold = gtk_text_mark_new ("b", TRUE);
  mark_italic = gtk_text_mark_new ("i", TRUE);
  mark_underline = gtk_text_mark_new ("u", TRUE);
  mark_strike = gtk_text_mark_new ("s", TRUE);

  while ((c = *end))
    {
      if (c == '<')
        {
          gn_xml_note_update_buffer (self, text_buffer, start, end);
          gtk_text_buffer_get_end_iter (text_buffer, &end_iter);

          /* Skip '<' */
          end++;

          if (g_str_has_prefix (end, "b>"))
            {
              if (!gtk_text_buffer_get_mark (text_buffer, "b"))
                gtk_text_buffer_add_mark (text_buffer, mark_bold, &end_iter);
            }
          else if (g_str_has_prefix (end, "i>"))
            {
              if (!gtk_text_buffer_get_mark (text_buffer, "i"))
                gtk_text_buffer_add_mark (text_buffer, mark_italic, &end_iter);
            }
          else if (g_str_has_prefix (end, "u>"))
            {
              if (!gtk_text_buffer_get_mark (text_buffer, "u"))
                gtk_text_buffer_add_mark (text_buffer, mark_underline, &end_iter);
            }
          else if (g_str_has_prefix (end, "s>"))
            {
              if (!gtk_text_buffer_get_mark (text_buffer, "s"))
                gtk_text_buffer_add_mark (text_buffer, mark_strike, &end_iter);
            }
          else if (g_str_has_prefix (end, "/b>"))
            {
              gn_xml_note_apply_tag_at_mark (text_buffer, mark_bold, "b");
            }
          else if (g_str_has_prefix (end, "/i>"))
            {
              gn_xml_note_apply_tag_at_mark (text_buffer, mark_italic, "i");
            }
          else if (g_str_has_prefix (end, "/u>"))
            {
              gn_xml_note_apply_tag_at_mark (text_buffer, mark_underline, "u");
            }
          else if (g_str_has_prefix (end, "/s>"))
            {
              gn_xml_note_apply_tag_at_mark (text_buffer, mark_strike, "s");
            }

          end = strchr (end, '>');
          g_return_if_fail (end != NULL);

          end++;
          start = end;
        }
      else if (c == '&')
        {
          gchar *str = "";

          gn_xml_note_update_buffer (self, text_buffer, start, end);
          gtk_text_buffer_get_end_iter (text_buffer, &end_iter);

          if (g_str_has_prefix (end, "&lt;"))
            str = "<";
          else if (g_str_has_prefix (end, "&gt;"))
            str = ">";
          else if (g_str_has_prefix (end, "&amp;"))
            str = "&";
          else if (g_str_has_prefix (end, "&quote;"))
            str = "\"";
          else
            g_warn_if_reached ();

          gtk_text_buffer_insert (text_buffer, &end_iter, str, 1);

          end = strchr (end, ';');
          g_return_if_fail (end != NULL);

          end++;
          start = end;
        }
      else
        end++;
    }

  gn_xml_note_update_buffer (self, text_buffer, start, end);
  gtk_text_buffer_set_modified (text_buffer, FALSE);
}

static void
gn_xml_note_close_tag (GnXmlNote   *self,
                       GString     *raw_content,
                       const gchar *tag_name,
                       GQueue      *tags_queue)
{
  GList *last_tag;
  GList *node;

  g_assert (GN_IS_XML_NOTE (self));
  g_assert (raw_content != NULL);
  g_assert (tag_name != NULL);
  g_assert (tags_queue != NULL);

  last_tag = g_queue_find (tags_queue, g_intern_string (tag_name));

  if (last_tag == NULL)
    return;

  /*
   * First, we have to close the tags in the reverse order it is opened.
   * Eg: If the @tags_queue has "i" as head and "s" as next, and "b"
   * next element, and if the @tag represents "b", we have to close "i"
   * tag first, then "s".  And then close "b" tag.  That is, the result
   * should be the following:
   * <b><s><i>some text</i></s></b> and not <b><s><i>some text</b>...
   */
  for (node = tags_queue->head; node != last_tag; node = node->next)
    g_string_append_printf (raw_content, "</%s>", (gchar *)node->data);

  /* From the previous example: we are now closing </b> */
  g_string_append_printf (raw_content, "</%s>", tag_name);

    /*
     * To make the XML valid, we have to open the closed tags that aren't
     * supposed to be closed.  Again, from the previous example: We
     * have closed "s" and "i" tags.  We have to open them in reverse
     * order. This results in: <b><s><i>some text</i></s></b><s><i>
     */
  for (node = node->prev; node != NULL; node = node->prev)
    g_string_append_printf (raw_content, "<%s>", (gchar *)node->data);

  g_queue_delete_link (tags_queue, last_tag);
}

void
gn_xml_note_append_escaped (GString     *xml,
                            const gchar *content)
{
  g_autofree gchar *escaped = NULL;

  g_return_if_fail (xml != NULL);

  if (!content || !*content)
    return;

  escaped = g_markup_escape_text (content, -1);
  g_string_append_printf (xml, "%s", escaped);
}

void
gn_xml_note_add_tag (GString     *xml,
                     const gchar *tag,
                     const gchar *content)
{
  g_autofree gchar *escaped = NULL;

  g_return_if_fail (xml != NULL);
  g_return_if_fail (tag != NULL);
  g_return_if_fail (*tag);
  g_return_if_fail (content != NULL);
  g_return_if_fail (*content);

  escaped = g_markup_escape_text (content, -1);
  g_string_append_printf (xml, "<%s>%s</%s>\n", tag, escaped, tag);
}

void
gn_xml_note_add_time_tag (GString     *xml,
                          const gchar *tag,
                          gint64       unix_time)
{
  g_autofree gchar *iso_time = NULL;

  g_return_if_fail (xml != NULL);
  g_return_if_fail (tag != NULL);
  g_return_if_fail (*tag);

  iso_time = gn_utils_unix_time_to_iso (unix_time);
  g_string_append_printf (xml, "<%s>%s</%s>\n", tag, iso_time, tag);
}

static void
gn_xml_note_update_raw_xml (GnXmlNote *self)
{
  GnItem *item;
  gchar *str;
  GdkRGBA rgba;

  g_assert (GN_IS_XML_NOTE (self));

  item = GN_ITEM (self);

  if (self->raw_xml)
    g_string_free (self->raw_xml, TRUE);

  self->raw_xml = g_string_new (COMMON_XML_HEAD "\n" "<note version=\"1\" "
                                "xmlns:link=\"" BIJIBEN_XML_NS "/link\" "
                                "xmlns:size=\"" BIJIBEN_XML_NS "/size\" "
                                "xmlns=\"" BIJIBEN_XML_NS "\">\n");

  gn_xml_note_add_tag (self->raw_xml, "title",
                       gn_item_get_title (item));

  gn_xml_note_add_time_tag (self->raw_xml, "last-change-date",
                            gn_item_get_modification_time (item));
  gn_xml_note_add_time_tag (self->raw_xml, "last-metadata-change-date",
                            gn_item_get_meta_modification_time (item));
  gn_xml_note_add_time_tag (self->raw_xml, "create-date",
                            gn_item_get_creation_time (item));

  if (gn_item_get_rgba (item, &rgba))
    {
      str = gdk_rgba_to_string (&rgba);
      gn_xml_note_add_tag (self->raw_xml, "color", str);
      g_free (str);
    }

  if (g_hash_table_size (self->labels) > 0)
    {
      GList *labels = g_hash_table_get_keys (self->labels);

      g_string_append (self->raw_xml, "<tags>");
      g_string_append_c (self->raw_xml, '\n');

      for (GList *node = labels; node != NULL; node = node->next)
        gn_xml_note_add_tag (self->raw_xml, "tag", node->data);
    }

  g_string_append (self->raw_xml, "<text xml:space=\"preserve\">"
                   "<note-content>");
  gn_xml_note_append_escaped (self->raw_xml,
                              gn_item_get_title (item));
}

static void
gn_xml_note_set_content_from_buffer (GnNote        *note,
                                     GtkTextBuffer *buffer)
{
  GnXmlNote *self = GN_XML_NOTE (note);
  GQueue *tags_queue;
  GString *raw_content;
  g_autofree gchar *content = NULL;
  GtkTextIter start, end, iter;

  g_assert (GN_IS_XML_NOTE (self));
  g_assert (GTK_IS_TEXT_BUFFER (buffer));

  tags_queue = g_queue_new ();
  raw_content = g_string_sized_new (gtk_text_buffer_get_char_count (buffer));

  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_iter_at_line_index (buffer, &end, 0, G_MAXINT);
  content = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  gn_item_set_title (GN_ITEM (note), content);

  gn_xml_note_update_raw_xml (self);

  gtk_text_iter_forward_char (&end);
  iter = start = end;

  /*
   * We moved one character past the last character in the title.
   * And if we are still at line 0 (first line), it means we have
   * no more content
   */
  if (gtk_text_iter_get_line (&iter) == 0)
    goto end;
  else
    g_string_append_c (self->raw_xml, '\n');

  while (gtk_text_iter_forward_to_tag_toggle (&iter, NULL))
    {
      GSList *tags;
      const gchar *tag_name;

      if (!gtk_text_iter_equal (&iter, &start))
        {
          g_autofree gchar *content = NULL;
          g_autofree gchar *markup = NULL;

          content = gtk_text_buffer_get_text (buffer, &start, &iter, FALSE);
          markup = g_markup_escape_text (content, -1);
          g_string_append (raw_content, markup);

          start = iter;
        }

      /* First, we have to handle tags that are closed */
      tags = gtk_text_iter_get_toggled_tags (&iter, FALSE);
      for (GSList *node = tags; node != NULL; node = node->next)
        {
          GtkTextTag *tag = node->data;

          tag_name = gn_note_buffer_get_name_for_tag (GN_NOTE_BUFFER (buffer),
                                                      tag);
          gn_xml_note_close_tag (self, raw_content, tag_name, tags_queue);
        }
      g_slist_free (tags);

      /* Now, let's handle open tags */
      tags = gtk_text_iter_get_toggled_tags (&iter, TRUE);
      for (GSList *node = tags; node != NULL; node = node->next)
        {
          GtkTextTag *tag = node->data;

          tag_name = gn_note_buffer_get_name_for_tag (GN_NOTE_BUFFER (buffer),
                                                      tag);
          g_string_append_printf (raw_content, "<%s>", tag_name);
          g_queue_push_head (tags_queue, (gchar *)g_intern_string (tag_name));
        }

      g_slist_free (tags);
    }

  for (GList *node = tags_queue->head; node != NULL; node = node->next)
    g_string_append_printf (raw_content, "</%s>", (gchar *)node->data);

  g_queue_free (tags_queue);

  gtk_text_buffer_get_end_iter (buffer, &end);

  if (!gtk_text_iter_equal (&iter, &end))
    {
      g_autofree gchar *content = NULL;
      g_autofree gchar *markup = NULL;

      content = gtk_text_buffer_get_text (buffer, &iter, &end, FALSE);
      markup = g_markup_escape_text (content, -1);
      g_string_append (raw_content, markup);
    }


  if (raw_content && raw_content->str)
    g_string_append (self->raw_xml, raw_content->str);

 end:
  g_string_append (self->raw_xml, "</note-content></text></note>\n");

  self->raw_content = g_strdup (self->raw_xml->str);
  g_clear_pointer (&self->raw_inner_xml, g_free);
  if (raw_content->str)
    self->raw_inner_xml = g_strconcat (raw_content->str, NULL);

  if (self->text_content)
    g_string_free (self->text_content, TRUE);
  self->text_content = NULL;
  if (self->markup)
    g_string_free (self->markup, TRUE);
  self->markup = NULL;
}

static void
gn_xml_note_finalize (GObject *object)
{
  GnXmlNote *self = (GnXmlNote *)object;

  GN_ENTRY;

  g_free (self->title);
  g_free (self->raw_content);
  g_hash_table_destroy (self->labels);
  if (self->text_content)
    g_string_free (self->text_content, TRUE);
  if (self->markup)
    g_string_free (self->markup, TRUE);

  xml_reader_free (self->xml_reader);

  G_OBJECT_CLASS (gn_xml_note_parent_class)->finalize (object);

  GN_EXIT;
}

static void
gn_xml_note_update_text_content (GnXmlNote *self)
{
  g_autofree gchar *content = NULL;
  g_autofree gchar *casefold_content = NULL;

  gchar *content_start;

  g_assert (GN_IS_XML_NOTE (self));

  if (self->text_content)
    g_string_free (self->text_content, TRUE);
  self->text_content = NULL;

  if (self->raw_content == NULL)
    return;

  content_start = strchr (self->raw_content, '\n');
  if (content_start == NULL)
    return;

  /* Skip '\n' */
  content_start++;

  content = gn_utils_get_text_from_xml (content_start);
  casefold_content = g_utf8_casefold (content, -1);
  self->text_content = g_string_new (casefold_content);
}

static gchar *
gn_xml_note_get_text_content (GnNote *note)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_NOTE (note));

  if (self->text_content == NULL)
    gn_xml_note_update_text_content (self);

  if (self->text_content == NULL ||
      self->text_content->len == 0)
    return NULL;

  /* FIXME: Check tests */
  if (*self->text_content->str == '\0')
    return NULL;

  return g_strdup (self->text_content->str);
}

static gchar *
gn_xml_note_get_raw_content (GnNote *note)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_NOTE (note));


  if (self->raw_content == NULL)
    {
      g_autofree gchar *content = NULL;

      if (self->raw_inner_xml == NULL ||
          *self->raw_inner_xml == '\0')
        return NULL;

      gn_xml_note_update_raw_xml (self);
      content = gn_note_get_text_content (GN_NOTE (self));

      if (content)
        {
          xml_writer_write_raw (self->xml_writer, "\n");
          xml_writer_write_string (self->xml_writer, content);
        }

      xml_writer_end_doc (self->xml_writer);

      self->raw_content = g_strdup ((gchar *)self->xml_buffer->content);
      g_clear_pointer (&self->raw_inner_xml, g_free);
    }

  return g_strdup (self->raw_content);
}

static void
gn_xml_note_set_text_content (GnNote      *note,
                              const gchar *content)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_XML_NOTE (self));

  /* g_free (self->text_content); */
  /* self->text_content = g_strdup (content); */
}

static void
gn_xml_note_update_markup (GnXmlNote *self)
{
  g_assert (GN_IS_XML_NOTE (self));

  if (self->markup)
    g_string_free (self->markup, TRUE);

  self->markup = g_string_new ("<markup>");

  if (self->raw_inner_xml)
    g_string_append_printf (self->markup, "<span font='Cantarell'>"
                            "%s</span>\n\n",
                            self->raw_inner_xml);
  g_string_append (self->markup, "</markup>");
}

static gchar *
gn_xml_note_get_markup (GnNote *note)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_NOTE (note));


  if (self->markup == NULL)
    gn_xml_note_update_markup (self);

  return g_strdup (self->markup->str);
}

static const gchar *
gn_xml_note_get_extension (GnNote *note)
{
  return ".note";
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

  if (strstr (self->text_content->str, needle) != NULL)
    return TRUE;

  return FALSE;
}

static GnFeature
gn_xml_note_get_features (GnItem *item)
{
  return GN_FEATURE_COLOR | GN_FEATURE_FORMAT |
    GN_FEATURE_CREATION_DATE | GN_FEATURE_MODIFICATION_DATE;
}

static void
gn_xml_note_class_init (GnXmlNoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GnNoteClass *note_class = GN_NOTE_CLASS (klass);
  GnItemClass *item_class = GN_ITEM_CLASS (klass);

  object_class->finalize = gn_xml_note_finalize;

  note_class->get_raw_content = gn_xml_note_get_raw_content;
  note_class->get_text_content = gn_xml_note_get_text_content;
  note_class->set_text_content = gn_xml_note_set_text_content;
  note_class->get_markup = gn_xml_note_get_markup;
  note_class->get_extension = gn_xml_note_get_extension;

  note_class->set_content_to_buffer = gn_xml_note_set_content_to_buffer;
  note_class->set_content_from_buffer = gn_xml_note_set_content_from_buffer;

  item_class->match = gn_xml_note_match;
  item_class->get_features = gn_xml_note_get_features;
}

static void
gn_xml_note_init (GnXmlNote *self)
{
  self->text_content = g_string_new ("");
  self->labels = g_hash_table_new_full (g_str_hash, g_str_equal,
                                        g_free, NULL);
  self->is_bijiben = TRUE;
}

static void
gn_xml_note_parse_xml (GnXmlNote *self)
{
  g_assert (GN_IS_XML_NOTE (self));

  while (xml_reader_read (self->xml_reader) == 1)
    if (xml_reader_get_node_type (self->xml_reader) == XML_ELEMENT_NODE)
      {
        const gchar *tag;
        g_autofree gchar *content = NULL;

        tag = xml_reader_get_name (self->xml_reader);
        g_return_if_fail (tag != NULL);

        if (g_strcmp0 (tag, "title") == 0)
          {
            content = xml_reader_dup_string (self->xml_reader);
            gn_item_set_title (GN_ITEM (self), content);
          }
        else if (g_strcmp0 (tag, "create-date") == 0)
          {
            GDateTime *date_time;
            gint64 creation_time;

            content = xml_reader_dup_string (self->xml_reader);
            if (content == NULL)
              continue;

            date_time = g_date_time_new_from_iso8601 (content, NULL);
            creation_time = g_date_time_to_unix (date_time);
            g_object_set (G_OBJECT (self), "creation-time",
                          creation_time, NULL);
          }
        else if (g_strcmp0 (tag, "last-change-date") == 0)
          {
            GDateTime *date_time;
            gint64 modification_time;

            content = xml_reader_dup_string (self->xml_reader);
            if (content == NULL)
              continue;

            date_time = g_date_time_new_from_iso8601 (content, NULL);
            modification_time = g_date_time_to_unix (date_time);
            g_object_set (G_OBJECT (self), "modification-time",
                          modification_time, NULL);
          }
        else if (g_strcmp0 (tag, "last-metadata-change-date") == 0)
          {
            GDateTime *date_time;
            gint64 modification_time;

            content = xml_reader_dup_string (self->xml_reader);
            if (content == NULL)
              continue;

            date_time = g_date_time_new_from_iso8601 (content, NULL);
            modification_time = g_date_time_to_unix (date_time);
            g_object_set (G_OBJECT (self), "meta-modification-time",
                          modification_time, NULL);
          }
        else if (g_strcmp0 (tag, "color") == 0)
          {
            GdkRGBA rgba;

            content = xml_reader_dup_string (self->xml_reader);
            if (content == NULL)
              continue;

            if (!gdk_rgba_parse (&rgba, content))
              {
                g_warning ("Failed to parse color: %s", content);
                continue;
              }

            gn_item_set_rgba (GN_ITEM (self), &rgba);
          }
        else if (g_strcmp0 (tag, "tag") == 0)
          {
            content = xml_reader_dup_string (self->xml_reader);

            if (content == NULL ||
                g_str_has_prefix (content, "system:template"))
              continue;

            if (g_str_has_prefix (content, "system:notebook:"))
              {
                gchar *label = content + strlen ("system:notebook:");

                g_hash_table_add (self->labels, g_strdup (label));
              }
          }
        else if (self->is_bijiben && g_strcmp0 (tag, "note-content") == 0)
          {
            g_autofree gchar *inner_xml = NULL;
            gchar *content;

            inner_xml = xml_reader_dup_inner_xml (self->xml_reader);
            g_return_if_fail (inner_xml != NULL);

            /* Skip the title */
            content = strchr (inner_xml, '\n');

            if (content)
              content++;

            self->raw_inner_xml = g_strdup (content);
          }
      }
}

/*
 * This is a stupid parser.  All we need to get is the
 * title, and the boundaries of note content.
 * GMarkupParser is an overkill here.
 */
static GnXmlNote *
gn_xml_note_create_from_data (const gchar *data,
                              gsize        length)
{
  g_autoptr(GnXmlNote) self = NULL;
  NoteFormat note_format;

  g_assert (data != NULL);

  if (length == -1)
    length = strlen (data);

  note_format = gn_xml_note_get_format (data, length);

  g_return_val_if_fail (note_format != NOTE_FORMAT_UNKNOWN, NULL);

  self = g_object_new (GN_TYPE_XML_NOTE, NULL);
  self->note_format = note_format;

  if (note_format == NOTE_FORMAT_BIJIBEN_2 ||
      note_format == NOTE_FORMAT_BIJIBEN_1)
    self->is_bijiben = TRUE;
  else
    self->is_bijiben = FALSE;

  self->xml_reader = xml_reader_new (data, length);

  g_return_val_if_fail (self->xml_reader != NULL, NULL);

  if (self->is_bijiben)
    self->raw_xml = g_string_new_len (data, length);
  else
    self->raw_data = g_string_new_len (data, length);

  if (self->is_bijiben)
    gn_xml_note_parse_xml (self);

  return g_steal_pointer (&self);
}

/**
 * gn_xml_note_new_from_data:
 * @data (nullable): The raw note content
 * @length: The length of @data, or -1
 *
 * Create a new XML note from the given data.
 *
 * Returns: (transfer full): a new #GnXmlNote
 * with given content.
 */
GnXmlNote *
gn_xml_note_new_from_data (const gchar *data,
                           gsize        length)
{
  if (data == NULL)
    return g_object_new (GN_TYPE_XML_NOTE, NULL);

  return gn_xml_note_create_from_data (data, length);
}
