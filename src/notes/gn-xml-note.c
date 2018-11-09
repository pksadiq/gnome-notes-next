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

struct _GnXmlNote
{
  GnNote parent_instance;

  xmlTextReader *xml_reader;
  xmlDoc  *xml_doc;
  xmlBuffer *xml_buffer;
  xmlTextWriter *xml_writer;
  gchar   *raw_inner_xml; /* xml data of the <text> tag */
  gchar   *raw_content;   /* full xml data that will be saved as file */
  GString *text_content;
  GString *markup;
  gchar   *title;
  GHashTable *labels;

  /* TRUE: Bijiben XML.  FALSE: Tomboy XML */
  gboolean is_bijiben;
};

G_DEFINE_TYPE (GnXmlNote, gn_xml_note, GN_TYPE_NOTE)

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

static GtkTextBuffer *
gn_xml_note_get_buffer (GnNote *note)
{
  GtkTextMark *mark_bold, *mark_italic;
  GtkTextMark *mark_underline, *mark_strike;
  GtkTextBuffer *buffer;
  gchar *start, *end;
  GtkTextIter end_iter;
  gboolean last_is_div = FALSE;
  gchar c;

  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_XML_NOTE (self));

  start = end = self->raw_inner_xml;
  buffer = GTK_TEXT_BUFFER (gn_note_buffer_new ());

  if (start == NULL)
    return buffer;

  mark_bold = gtk_text_mark_new ("b", TRUE);
  mark_italic = gtk_text_mark_new ("i", TRUE);
  mark_underline = gtk_text_mark_new ("u", TRUE);
  mark_strike = gtk_text_mark_new ("s", TRUE);

  /* Skip up to the content */
  end = strstr (end, "<body");
  end = strchr (end, '>');
  end++;
  start = end;

  while ((c = *end))
    {
      if (c == '<')
        {
          gn_xml_note_update_buffer (self, buffer, start, end);
          gtk_text_buffer_get_end_iter (buffer, &end_iter);

          /* Skip '<' */
          end++;

          if (!g_str_has_prefix (end, "div"))
            last_is_div = FALSE;

          if (g_str_has_prefix (end, "div"))
            {
              /*
               * "div" tag may be nested.  Only the first level
               * should be considered as a paragraph.  The rest
               * can safely be ignored.
               */
              if (!last_is_div)
                gtk_text_buffer_insert (buffer, &end_iter, "\n", 1);

              last_is_div = TRUE;
            }
          else if (g_str_has_prefix (end, "b>"))
            {
              if (!gtk_text_buffer_get_mark (buffer, "b"))
                gtk_text_buffer_add_mark (buffer, mark_bold, &end_iter);
            }
          else if (g_str_has_prefix (end, "i>"))
            {
              if (!gtk_text_buffer_get_mark (buffer, "i"))
                gtk_text_buffer_add_mark (buffer, mark_italic, &end_iter);
            }
          else if (g_str_has_prefix (end, "u>"))
            {
              if (!gtk_text_buffer_get_mark (buffer, "u"))
                gtk_text_buffer_add_mark (buffer, mark_underline, &end_iter);
            }
          else if (g_str_has_prefix (end, "strike"))
            {
              if (!gtk_text_buffer_get_mark (buffer, "s"))
                gtk_text_buffer_add_mark (buffer, mark_strike, &end_iter);
            }
          else if (g_str_has_prefix (end, "/div") ||
                   g_str_has_prefix (end, "br "))
            {
              /* Do nothing */
            }
          else if (g_str_has_prefix (end, "/b>"))
            {
              gn_xml_note_apply_tag_at_mark (buffer, mark_bold, "bold");
            }
          else if (g_str_has_prefix (end, "/i>"))
            {
              gn_xml_note_apply_tag_at_mark (buffer, mark_italic, "italic");
            }
          else if (g_str_has_prefix (end, "/u>"))
            {
              gn_xml_note_apply_tag_at_mark (buffer, mark_underline, "underline");
            }
          else if (g_str_has_prefix (end, "/strike"))
            {
              gn_xml_note_apply_tag_at_mark (buffer, mark_strike, "strike");
            }
          else if (g_str_has_prefix (end, "/body"))
            {
              start = end;
              break;
            }

          end = strchr (end, '>');
          end++;
          start = end;
        }
      else if (c == '&')
        {
          gchar *str = "";

          last_is_div = FALSE;
          gn_xml_note_update_buffer (self, buffer, start, end);
          gtk_text_buffer_get_end_iter (buffer, &end_iter);

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

          gtk_text_buffer_insert (buffer, &end_iter, str, 1);

          end = strchr (end, ';');
          end++;
          start = end;
        }
      else
        {
          last_is_div = FALSE;
          end++;
        }
    }

  gn_xml_note_update_buffer (self, buffer, start, end);
  gtk_text_buffer_set_modified (buffer, FALSE);

  return buffer;
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

static void
gn_xml_note_update_raw_xml (GnXmlNote *self)
{
  xmlTextWriter *writer;
  GnItem *item;
  gchar *str;
  GdkRGBA rgba;
  gint64 unix_time;

  g_assert (GN_IS_XML_NOTE (self));

  self->xml_buffer = xml_buffer_new ();
  self->xml_writer = xml_writer_new (self->xml_buffer);
  writer = self->xml_writer;
  item = GN_ITEM (self);

  xml_writer_start_doc (writer);
  xml_writer_start_tag (writer, "note");
  xml_writer_write_attribute (writer, NULL, "version", "1");
  xml_writer_write_attribute (writer, "xmlns", "link",
                              BIJIBEN_XML_NS "/link");
  xml_writer_write_attribute (writer, "xmlns", "size",
                              BIJIBEN_XML_NS "/size");
  xml_writer_write_attribute (writer, NULL, "xmlns",
                              BIJIBEN_XML_NS);

  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "title",
                      gn_item_get_title (GN_ITEM (self)));

  unix_time = gn_item_get_modification_time (item);
  str = gn_utils_unix_time_to_iso (unix_time);
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "last-change-date", str);
  g_free (str);

  unix_time = gn_item_get_meta_modification_time (item);
  str = gn_utils_unix_time_to_iso (unix_time);
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "last-metadata-change-date", str);
  g_free (str);

  unix_time = gn_item_get_creation_time (item);
  str = gn_utils_unix_time_to_iso (unix_time);
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "create-date", str);
  g_free (str);

  /* XXX: Just for backward compatibility */
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "cursor-position", "0");
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "selection-bound-position", "0");
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "width", "0");
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "height", "0");
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "x", "0");
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "y", "0");

  if (gn_item_get_rgba (item, &rgba))
    {
      str = gdk_rgba_to_string (&rgba);
      xml_writer_write_raw (writer, "\n");
      xml_writer_add_tag (writer, "color", str);
    }

  xml_writer_write_raw (writer, "\n");
  xml_writer_start_tag (writer, "tags");

  if (g_hash_table_size (self->labels) > 0)
    {
      GList *labels = g_hash_table_get_keys (self->labels);

      for (GList *node = labels; node != NULL; node = node->next)
        {
          g_autofree gchar *label = g_strconcat ("system:notebook:",
                                                 node->data, NULL);
          xml_writer_write_raw (writer, "\n");
          xml_writer_add_tag (writer, "tag", label);
        }
    }

  xml_writer_end_tag (writer);
  xml_writer_write_raw (writer, "\n");
  xml_writer_add_tag (writer, "open-on-startup", "False");

  xml_writer_write_raw (writer, "\n");
  xml_writer_start_tag (writer, "text");
  xml_writer_write_attribute (writer, "xml", "space", "preserve");

  /*
   * XXX: Much of these are simply to keep backward compatibility.
   * We have no use of these tags anymore.
   */
  xml_writer_start_tag (writer, "html");
  xml_writer_write_attribute (writer, NULL, "xmlns",
                              "http://www.w3.org/1999/xhtml");

  xml_writer_start_tag (writer, "head");
  xml_writer_write_raw (writer,
                        "<link rel=\"stylesheet\" href=\"Default.css\" "
                        "type=\"text/css\"/><script language=\"javascript\""
                        " src=\"bijiben.js\"></script>");
  xml_writer_end_tag (writer);

  xml_writer_start_tag (writer, "body");
  xml_writer_write_attribute (writer, NULL, "contenteditable", "true");
  xml_writer_write_attribute (writer, NULL, "id", "editable");
  xml_writer_write_attribute (writer, NULL, "style", "color: white;");

  xml_writer_write_string (writer, gn_item_get_title (item));
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
  gunichar c;

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
    xml_writer_write_raw (self->xml_writer, "<br/>\n");

  do
    {
      GSList *tags;
      GSList *node;

      c = gtk_text_iter_get_char (&iter);

      if (c == '\n')
        g_string_append (raw_content, "<br />");

      /* First, we have to handle tags that are closed */
      tags = gtk_text_iter_get_toggled_tags (&iter, FALSE);
      for (node = tags; node != NULL; node = node->next)
        {
          GtkTextTag *tag = node->data;
          const gchar *tag_name;

          tag_name = gn_note_buffer_get_name_for_tag (GN_NOTE_BUFFER (buffer),
                                                      tag);
          gn_xml_note_close_tag (self, raw_content, tag_name, tags_queue);
        }

      g_slist_free (tags);

      /* Now, let's handle open tags */
      tags = gtk_text_iter_get_toggled_tags (&iter, TRUE);
      for (node = tags; node != NULL; node = node->next)
        {
          GtkTextTag *tag = node->data;
          const gchar *tag_name;

          tag_name = gn_note_buffer_get_name_for_tag (GN_NOTE_BUFFER (buffer),
                                                      tag);
          g_string_append_printf (raw_content, "<%s>", tag_name);
          g_queue_push_head (tags_queue, (gchar *)g_intern_string (tag_name));
        }

      g_slist_free (tags);

      g_string_append_unichar (raw_content, c);
    } while (gtk_text_iter_forward_char (&iter));

  for (GList *node = tags_queue->head; node != NULL; node = node->next)
    g_string_append_printf (raw_content, "</%s>", (gchar *)node->data);

  g_queue_free (tags_queue);

  if (raw_content && raw_content->str)
    xml_writer_write_raw (self->xml_writer, raw_content->str);

 end:
  xml_writer_end_doc (self->xml_writer);

  self->raw_content = g_strdup ((gchar *)self->xml_buffer->content);
  g_clear_pointer (&self->raw_inner_xml, g_free);

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
  xml_doc_free (self->xml_doc);

  G_OBJECT_CLASS (gn_xml_note_parent_class)->finalize (object);

  GN_EXIT;
}

static void
gn_xml_note_update_text_content (GnXmlNote *self)
{
  g_autofree gchar *content = NULL;
  gchar *content_start;

  g_assert (GN_IS_XML_NOTE (self));

  if (self->text_content)
    g_string_free (self->text_content, TRUE);
  self->text_content = NULL;

  if (self->raw_content == NULL)
    return;

  if (self->is_bijiben)
    content_start = strstr (self->raw_content, "<div");
  else
    {
      content_start = strchr (self->raw_content, '\n');
      if (content_start)
        content_start++;
    }


  if (content_start == NULL)
    return;

  /* content = gn_utils_get_text_from_xml (content_start); */
  /* self->text_content = g_utf8_casefold (content, -1); */
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
          xml_writer_write_raw (self->xml_writer, "<br/>");
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

static gchar *
gn_xml_note_get_markup (GnNote *note)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_NOTE (note));


  if (self->markup &&
      self->markup->str)
    return g_strdup (self->markup->str);

  return g_strdup ("");
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

  note_class->get_buffer = gn_xml_note_get_buffer;
  note_class->set_content_from_buffer = gn_xml_note_set_content_from_buffer;

  item_class->match = gn_xml_note_match;
  item_class->get_features = gn_xml_note_get_features;
}

static void
gn_xml_note_init (GnXmlNote *self)
{
  self->text_content = g_string_new ("");
  self->markup = g_string_new ("");
  self->labels = g_hash_table_new_full (g_str_hash, g_str_equal,
                                        g_free, NULL);
  self->is_bijiben = TRUE;
}

static void
gn_xml_note_update_values (GnXmlNote   *self,
                           const gchar *str)
{
  gchar *value;
  const gchar *start, *end;
  GTimeVal time;
  GdkRGBA rgba;

  g_assert (GN_IS_XML_NOTE (self));
  g_assert (str != NULL);

  start = strstr (str, "<last-change-date>");

  if (!start)
    return;

  /* Find the end of the tag */
  start = strchr (start, '>');
  start++;
  end = strchr (start, '<');

  g_assert (end != NULL);
  g_assert (end > start);
  value = g_strndup (start, end - start);
  if (g_time_val_from_iso8601 (value, &time))
    g_object_set (G_OBJECT (self), "modification-time",
                  (gint64)time.tv_sec, NULL);
  g_free (value);

  start = strstr (end, "<create-date>");

  if (!end)
    return;

  start = strchr (start, '>');
  start++;
  end = strchr (start, '<');

  value = g_strndup (start, end - start);
  if (g_time_val_from_iso8601 (value, &time))
    g_object_set (G_OBJECT (self), "creation-time",
                  (gint64)time.tv_sec, NULL);
  g_free (value);

  start = strstr (end, "<color>");

  if (!start)
    return;

  start = strchr (start, '>');
  start++;
  end = strchr (start, '<');

  value = g_strndup (start, end - start);
  if (gdk_rgba_parse (&rgba, value))
    gn_item_set_rgba (GN_ITEM (self), &rgba);
  g_free (value);
}

static void
gn_xml_note_parse_as_bijiben (GnXmlNote     *self,
                              xmlTextReader *xml_reader)
{
  GQueue *tags_queue;

  g_assert (GN_IS_XML_NOTE (self));

  tags_queue = g_queue_new ();

  /*
   * The text until the first <div> tag is the title
   * of the note.  We don't need that, so simply skip.
   */
  while (xml_reader_read (xml_reader) == 1)
    {
      const gchar *tag;
      int type;

      type = xml_reader_get_node_type (xml_reader);
      tag = xml_reader_get_name (xml_reader);

      if (tag == NULL ||
          g_str_equal (tag, "div") ||
          g_str_equal (tag, "br"))
        break;

      continue;
      /*
       * TODO: allow the code below when we break bijiben
       * compatibility
       */
      /*
       * If the value of the tag contain a '\n', the title
       * ends there, the rest of the value is the part of
       * the content.
       */
      if (type == XML_TEXT_NODE)
        {
          const gchar *content;

          content = xml_reader_get_value (xml_reader);
          if (content)
            content = strchr (content, '\n');

          if (content == NULL)
            continue;

          /* Skip '\n' */
          content++;
          g_string_append (self->text_content, content);
          g_string_append (self->markup, content);
          break;
        }
    }

  while (xml_reader_read (xml_reader) == 1)
    {
      const gchar *tag;
      const gchar *content;
      int type;

      type = xml_reader_get_node_type (xml_reader);
      tag = xml_reader_get_name (xml_reader);

      if (tag == NULL)
        continue;

      switch (type)
        {
        case XML_TEXT_NODE:
          content = xml_reader_get_value (xml_reader);

          if (content)
            {
              g_autofree gchar *markup = NULL;

              g_string_append (self->text_content, content);
              markup = g_markup_escape_text (content, -1);
              g_string_append (self->markup, markup);
            }
          break;

          /* FIXME: Simplify */
        case XML_ELEMENT_NODE:
          if ((content = xml_reader_get_name (xml_reader)))
            {
              if (g_str_equal (content, "br"))
                g_string_append_c (self->markup, '\n');
              else if (!g_str_equal (content, "span") &&
                       !g_str_equal (content, "div") &&
                       !g_str_equal (content, "body") &&
                       !g_str_equal (content, "html"))
                {
                  g_queue_push_head (tags_queue, (gchar *)g_intern_string (tag));
                  g_string_append_printf (self->markup, "<%s>", content);
                }
            }
          break;

        case XML_ELEMENT_DECL:
          if ((content = xml_reader_get_name (xml_reader)))
            if (!g_str_equal (content, "span") &&
                !g_str_equal (content, "div") &&
                !g_str_equal (content, "body") &&
                !g_str_equal (content, "html"))
              gn_xml_note_close_tag (self, self->markup, content, tags_queue);
          break;

        default:
          break;
        }
    }
}

static void
gn_xml_note_parse_as_tomboy (GnXmlNote     *self,
                             xmlTextReader *xml_reader)
{
  g_assert (GN_IS_XML_NOTE (self));

  while (xml_reader_read (xml_reader) == 1)
    {
      const gchar *tag;
      const gchar *content;
      int type;

      type = xml_reader_get_node_type (xml_reader);
      tag = xml_reader_get_name (xml_reader);

      if (tag == NULL)
        continue;

      switch (type)
        {
        case XML_TEXT_NODE:
          content = xml_reader_get_value (xml_reader);
          /* The first line is the note title, skip that */
          content = strchr (content, '\n');
          if (content)
            content++;

          if (content && *content)
            {
              g_string_append (self->text_content, content);
              /*
               * Remove that last \n
               *
               * XXX: It shouldn't be a problem to end the content with \n
               * Remove this?
               */
              self->text_content->len--;
              self->text_content->str[self->text_content->len] = '\0';
            }
        }
    }
}

static void
gn_xml_note_parse_xml (GnXmlNote *self)
{
  g_assert (GN_IS_XML_NOTE (self));

  g_string_append (self->markup, "<markup>");

  while (xml_reader_read (self->xml_reader) == 1)
    if (xml_reader_get_node_type (self->xml_reader) == XML_ELEMENT_NODE)
      {
        const gchar *tag;
        g_autofree gchar *content = NULL;

        tag = xml_reader_get_name (self->xml_reader);
        g_return_if_fail (tag != NULL);

        if (g_strcmp0 (tag, "title") == 0)
          {
            g_autofree gchar *markup = NULL;

            content = xml_reader_dup_string (self->xml_reader);
            if (content == NULL)
              continue;

            gn_item_set_title (GN_ITEM (self), content);
            markup = g_markup_printf_escaped ("<b>%s</b>\n", content);
            g_string_append (self->markup, markup);
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
        else if (g_strcmp0 (tag, "text") == 0)
          {
            gchar *inner_xml;
            xmlTextReader *xml_reader;

            inner_xml = xml_reader_dup_inner_xml (self->xml_reader);
            g_return_if_fail (inner_xml != NULL);

            xml_reader = xml_reader_new (inner_xml, strlen (inner_xml));
            g_return_if_fail (xml_reader != NULL);

            self->raw_inner_xml = inner_xml;

            if (self->is_bijiben)
              gn_xml_note_parse_as_bijiben (self, xml_reader);
            else
              gn_xml_note_parse_as_tomboy (self, xml_reader);

            xml_reader_free (xml_reader);
          }
      }

  g_string_append (self->markup, "</markup>");
}

/*
 * This is a stupid parser.  All we need to get is the
 * title, and the boundaries of note content.
 * GMarkupParser is an overkill here.
 */
static GnXmlNote *
gn_xml_note_create_from_data (const gchar *data)
{
  xmlNode *root_node;
  g_autoptr(GnXmlNote) self = NULL;

  g_assert (data != NULL);

  self = g_object_new (GN_TYPE_XML_NOTE, NULL);

  self->xml_reader = xml_reader_new (data, strlen (data));

  g_return_val_if_fail (self->xml_reader != NULL, NULL);

  /*
   * TODO: Profile this.  May be we can avoid xml_doc_new()
   * because we only use xml_reader_new()
   */
  self->xml_doc = xml_doc_new (data, strlen (data));

  g_return_val_if_fail (self->xml_doc != NULL, NULL);

  root_node = xml_doc_get_root_element (self->xml_doc);
  g_return_val_if_fail (root_node != NULL, NULL);

  /* self->raw_xml_data = g_strdup (data); */

  if (g_strcmp0 ((gchar *)root_node->ns->href, TOMBOY_XML_NS) == 0)
    self->is_bijiben = FALSE;

  gn_xml_note_parse_xml (self);

  return g_steal_pointer (&self);
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
