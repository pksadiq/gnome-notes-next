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

  GString *raw_data;    /* The raw data used to parse, NULL if raw_xml set */
  GString *raw_xml;     /* full XML data to be saved to file */
  gchar   *content_xml; /* pointer to the beginning of content */
  GString *text_content;
  GString *markup;
  gchar   *title;
  GList   *tags;        /* List of GnTag */
  GHashTable *labels;

  NoteFormat note_format;
  guint      parse_complete : 1;
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
gn_xml_note_parse (GnXmlNote  *self,
                   GnTagStore *tag_store)
{
  xmlTextReader *xml_reader;

  g_assert (GN_IS_XML_NOTE (self));

  if (self->parse_complete)
    return;

  /* Only new bijiben format should be parsed */
  g_return_if_fail (self->note_format == NOTE_FORMAT_BIJIBEN_2);
  g_return_if_fail (self->raw_xml != NULL);

  self->parse_complete = 1;

  xml_reader = xml_reader_new (self->raw_xml->str,
                               self->raw_xml->len);

  while (xml_reader_read (xml_reader) == 1)
    if (xml_reader_get_node_type (xml_reader) == XML_ELEMENT_NODE)
      {
        const gchar *tag;
        g_autofree gchar *content = NULL;

        tag = xml_reader_get_name (xml_reader);
        g_return_if_fail (tag != NULL);

        if (g_str_equal (tag, "title"))
          {
            content = xml_reader_dup_string (xml_reader);
            gn_item_set_title (GN_ITEM (self), content);
          }
        else if (g_str_equal (tag, "create-date"))
          {
            g_autoptr(GDateTime) date_time = NULL;
            gint64 creation_time;

            content = xml_reader_dup_string (xml_reader);
            if (content == NULL)
              continue;

            date_time = g_date_time_new_from_iso8601 (content, NULL);
            creation_time = g_date_time_to_unix (date_time);
            g_object_set (G_OBJECT (self), "creation-time",
                          creation_time, NULL);
          }
        else if (g_str_equal (tag, "last-change-date"))
          {
            g_autoptr(GDateTime) date_time = NULL;
            gint64 modification_time;

            content = xml_reader_dup_string (xml_reader);
            if (content == NULL)
              continue;

            date_time = g_date_time_new_from_iso8601 (content, NULL);
            modification_time = g_date_time_to_unix (date_time);
            g_object_set (G_OBJECT (self), "modification-time",
                          modification_time, NULL);
          }
        else if (g_str_equal (tag, "last-metadata-change-date"))
          {
            g_autoptr(GDateTime) date_time = NULL;
            gint64 modification_time;

            content = xml_reader_dup_string (xml_reader);
            if (content == NULL)
              continue;

            date_time = g_date_time_new_from_iso8601 (content, NULL);
            modification_time = g_date_time_to_unix (date_time);
            g_object_set (G_OBJECT (self), "meta-modification-time",
                          modification_time, NULL);
          }
        else if (g_str_equal (tag, "color"))
          {
            GdkRGBA rgba;

            content = xml_reader_dup_string (xml_reader);
            if (content == NULL)
              continue;

            if (!gdk_rgba_parse (&rgba, content))
              {
                g_warning ("Failed to parse color: %s", content);
                continue;
              }

            gn_item_set_rgba (GN_ITEM (self), &rgba);
          }
        else if (g_str_equal (tag, "tag"))
          {
            content = xml_reader_dup_string (xml_reader);

            if (content == NULL)
              continue;

            g_hash_table_add (self->labels, g_strdup (content));
            g_assert (tag_store != NULL);

            if (tag_store != NULL)
              {
                GnTag *tag;

                tag = gn_tag_store_insert (tag_store,
                                           g_intern_string (content), NULL);
                self->tags = g_list_prepend (self->tags, tag);
              }
          }
      }

  xml_reader_free (xml_reader);
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
                               const gchar   *mark_name,
                               const gchar   *tag_name)
{
  GtkTextMark *mark;
  GtkTextIter start, end;

  mark = gtk_text_buffer_get_mark (buffer, mark_name);
  g_return_if_fail (mark != NULL);

  gtk_text_buffer_get_end_iter (buffer, &end);
  gtk_text_buffer_get_iter_at_mark (buffer, &start, mark);
  gtk_text_buffer_apply_tag_by_name (buffer, tag_name, &start, &end);
  gtk_text_buffer_delete_mark (buffer, mark);
}

static void
gn_xml_note_set_content_to_buffer (GnNote       *note,
                                   GnNoteBuffer *buffer)
{
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

  if (g_str_has_prefix (self->content_xml, "</note-content>"))
    return;

  start = end = self->content_xml;
  gtk_text_buffer_get_end_iter (text_buffer, &end_iter);
  gtk_text_buffer_insert (text_buffer, &end_iter, "\n", 1);

  while ((c = *end))
    {
      if (c == '<')
        {
          const gchar *tag = NULL;
          gboolean is_close_tag = FALSE;

          gn_xml_note_update_buffer (self, text_buffer, start, end);
          gtk_text_buffer_get_end_iter (text_buffer, &end_iter);

          /* Skip '<' */
          end++;

          if (*end == '/')
            {
              is_close_tag = TRUE;
              end++;
            }

          if (g_str_has_prefix (end, "b"))
            tag = g_intern_static_string ("b");
          else if (g_str_has_prefix (end, "i"))
            tag = g_intern_static_string ("i");
          else if (g_str_has_prefix (end, "s"))
            tag = g_intern_static_string ("s");
          else if (g_str_has_prefix (end, "u"))
            tag = g_intern_static_string ("u");
          else if (g_str_has_prefix (end, "note-content>"))
            break;

          if (tag)
            {
              if (is_close_tag)
                gn_xml_note_apply_tag_at_mark (text_buffer, tag, tag);
              else if (!gtk_text_buffer_get_mark (text_buffer, tag))
                gtk_text_buffer_create_mark (text_buffer, tag, &end_iter, TRUE);
            }
          else
            g_warn_if_reached ();

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
          else if (g_str_has_prefix (end, "&apos;"))
            str = "'";
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
   * Eg: If the @tags_queue has "i" as head, then "s" and then "b", and
   * if the @tag_name represents "b", we have to close "i"  tag first,
   * then "s", and then close "b" tag.  That is, the result
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

  /* Remove the tag we closed from the queue */
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

  self->raw_xml = g_string_new (COMMON_XML_HEAD "\n" "<note version=\"2\" "
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

      g_string_append (self->raw_xml, "<tags>\n");

      for (GList *node = labels; node != NULL; node = node->next)
        gn_xml_note_add_tag (self->raw_xml, "tag", node->data);

      g_string_append (self->raw_xml, "</tags>\n");
    }

  g_string_append (self->raw_xml, "<text xml:space=\"preserve\">"
                   "<note-content>");
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
  gboolean has_content;
  gint content_start;

  g_assert (GN_IS_XML_NOTE (self));
  g_assert (GTK_IS_TEXT_BUFFER (buffer));

  tags_queue = g_queue_new ();
  raw_content = g_string_sized_new (gtk_text_buffer_get_char_count (buffer));

  if (gn_item_get_creation_time (GN_ITEM (self)) == 0)
    g_object_set (self, "creation-time", time (NULL), NULL);
  if (gn_item_get_meta_modification_time (GN_ITEM (self)) == 0)
    g_object_set (self, "meta-modification-time", time (NULL), NULL);

  g_object_set (self, "modification-time", time (NULL), NULL);

  gtk_text_buffer_get_start_iter (buffer, &start);
  end = start;
  has_content = gtk_text_iter_forward_to_line_end (&end);
  content = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  gn_item_set_title (GN_ITEM (note), content);

  gn_xml_note_update_raw_xml (self);
  /* Point to end of <note-content> */
  content_start = self->raw_xml->len;

  if (!has_content)
    goto end;

  gtk_text_iter_set_line (&end, 1);
  iter = start = end;

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
  self->content_xml = self->raw_xml->str + content_start;

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
  g_hash_table_destroy (self->labels);
  if (self->text_content)
    g_string_free (self->text_content, TRUE);
  if (self->markup)
    g_string_free (self->markup, TRUE);

  G_OBJECT_CLASS (gn_xml_note_parent_class)->finalize (object);

  GN_EXIT;
}

static void
gn_xml_note_update_text_content (GnXmlNote *self)
{
  g_autofree gchar *content = NULL;
  g_autofree gchar *casefold_content = NULL;

  g_assert (GN_IS_XML_NOTE (self));

  if (self->text_content)
    g_string_free (self->text_content, TRUE);
  self->text_content = NULL;

  content = gn_utils_get_text_from_xml (self->raw_xml->str);
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

  /* TODO: only for notes being imported */
  if (self->raw_xml == NULL)
    {
      g_autofree gchar *content = NULL;

      gn_xml_note_update_raw_xml (self);
      content = gn_note_get_text_content (GN_NOTE (self));

      if (content)
        {
          g_string_append_c (self->raw_xml, '\n');
          gn_xml_note_append_escaped (self->raw_xml, content);
        }

      g_string_append (self->raw_xml, "</note-content></text></note>");
    }

  return g_strdup (self->raw_xml->str);
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
gn_xml_add_pending (GString     *string,
                    const gchar *start,
                    const gchar *end)
{
  if (!(end > start))
    return;

  g_string_append_len (string, start, end - start);
}

static void
gn_xml_note_update_markup (GnXmlNote *self)
{
  GQueue *tags_queue;
  gchar *tag_start, *start, *tag_end;

  g_assert (GN_IS_XML_NOTE (self));

  if (self->markup)
    g_string_free (self->markup, TRUE);

  /* Exit early if empty note contentn */
  if (g_str_has_prefix (self->content_xml, "</note-content>"))
    {
      self->markup = g_string_new ("");
      return;
    }

  tags_queue = g_queue_new ();
  start = tag_start = self->content_xml;
  self->markup = g_string_new ("<markup>"
                               "<span font='Cantarell'>");

  while ((tag_start = strchr (tag_start, '<')))
    {
      g_autofree gchar *tag = NULL;
      const gchar *interned_tag;
      gboolean is_close_tag = FALSE;

      gn_xml_add_pending (self->markup, start, tag_start);

      /* Skip '<' */
      tag_start++;

      if (g_str_has_prefix (tag_start, "/"))
        {
          is_close_tag = TRUE;
          tag_start++;
        }

      tag_end = strchr (tag_start, '>');
      if (G_UNLIKELY (!tag_end))
        break;

      tag = g_strndup (tag_start, tag_end - tag_start);
      interned_tag = g_intern_string (tag);

      if (interned_tag == g_intern_static_string ("note-content"))
        break;

      if (interned_tag == g_intern_static_string ("b") ||
          interned_tag == g_intern_static_string ("i") ||
          interned_tag == g_intern_static_string ("s") ||
          interned_tag == g_intern_static_string ("u"))
        {
          if (is_close_tag)
            gn_xml_note_close_tag (self, self->markup, interned_tag, tags_queue);
          else
            {
              g_queue_push_head (tags_queue, (gchar *)interned_tag);
              g_string_append_printf (self->markup, "<%s>", interned_tag);
            }
        }
      tag_start = start = tag_end + 1;
    }

  for (GList *node = tags_queue->head; node != NULL; node = node->next)
    g_string_append_printf (self->markup, "</%s>", (gchar *)node->data);

  g_string_append (self->markup, "</span></markup>");
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

static GList *
gn_xml_note_get_tags (GnNote *note)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  return g_hash_table_get_keys (self->labels);
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
  note_class->get_tags = gn_xml_note_get_tags;
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
  self->note_format = NOTE_FORMAT_BIJIBEN_2;
}

/*
 * This is a stupid parser.  All we need to get is the
 * title, and the boundaries of note content.
 * GMarkupParser is an overkill here.
 */
static GnXmlNote *
gn_xml_note_create_from_data (const gchar *data,
                              gsize        length,
                              GnTagStore  *tag_store)
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

  if (note_format == NOTE_FORMAT_BIJIBEN_2)
    {
      self->raw_xml = g_string_new_len (data, length);
      self->content_xml = strstr (self->raw_xml->str, "<note-content>");

      if (self->content_xml == NULL)
        return NULL;

      self->content_xml = self->content_xml + strlen ("<note-content>");
      gn_xml_note_parse (self, tag_store);
    }
  else
    self->raw_data = g_string_new_len (data, length);

  return g_steal_pointer (&self);
}

/**
 * gn_xml_note_new_from_data:
 * @data (nullable): The raw note content
 * @length: The length of @data, or -1
 * @tag_store: A #GnTagStore
 *
 * Create a new XML note from the given data.
 *
 * Returns: (transfer full): a new #GnXmlNote
 * with given content.
 */
GnXmlNote *
gn_xml_note_new_from_data (const gchar *data,
                           gsize        length,
                           GnTagStore  *tag_store)
{
  if (data == NULL)
    return g_object_new (GN_TYPE_XML_NOTE, NULL);

  return gn_xml_note_create_from_data (data, length, tag_store);
}
