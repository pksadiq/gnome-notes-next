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
  GHashTable *tags_list;
  GtkTextBuffer *buffer;
  gchar *start, *end;
  GtkTextIter start_iter, end_iter;
  gboolean last_is_div = FALSE;
  gchar c;

  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_XML_NOTE (self));

  start = end= self->raw_content;
  buffer = GTK_TEXT_BUFFER (gn_note_buffer_new ());

  if (start == NULL)
    return buffer;

  mark_bold = gtk_text_mark_new ("b", TRUE);
  mark_italic = gtk_text_mark_new ("i", TRUE);
  mark_underline = gtk_text_mark_new ("u", TRUE);
  mark_strike = gtk_text_mark_new ("s", TRUE);

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

static int
gn_xml_note_tag_compare (gconstpointer tag1,
                         gconstpointer tag2)
{
  return tag1 != tag2;
}

static void
gn_xml_note_close_tag (GnXmlNote    *self,
                       GnNoteBuffer *note_buffer,
                       GString      *raw_content,
                       GtkTextTag   *tag,
                       GQueue       *tags_queue)
{
  GList *last_tag;
  GList *node;
  const gchar *tag_name;

  g_assert (GN_IS_XML_NOTE (self));
  g_assert (GN_IS_NOTE_BUFFER (note_buffer));
  g_assert (raw_content != NULL);
  g_assert (tag != NULL);

  last_tag = g_queue_find (tags_queue, tag);

  g_assert (last_tag != NULL);

  /*
   * First, we have to close the tags in the reverse order it is opened.
   * Eg: If the @tags_queue has "i" as head and "s" as next, and "b"
   * next element, and if the @tag represents "b", we have to close "i"
   * tag first, then "s".  And then close "b" tag.  That is, the result
   * should be the following:
   * <b><s><i>some text</i></s></b> and not <b><s><i>some text</b>...
   */
  for (node = tags_queue->head; node != last_tag; node = node->next)
    {
      tag_name = gn_note_buffer_get_name_for_tag (note_buffer, node->data);
      g_string_append_printf (raw_content, "</%s>", tag_name);
    }

    /* From the previous example: we are now closing </b> */
    tag_name = gn_note_buffer_get_name_for_tag (note_buffer, last_tag->data);
    g_string_append_printf (raw_content, "</%s>", tag_name);

    /*
     * To make the XML valid, we have to open the closed tags that aren't
     * supposed to be closed.  Again, from the previous example: We
     * have closed "s" and "i" tags.  We have to open them in reverse
     * order. This results in: <b><s><i>some text</i></s></b><s><i>
     */
  for (node = node->prev; node && node != tags_queue->head; node = node->prev)
    {
      tag_name = gn_note_buffer_get_name_for_tag (note_buffer, node->data);
      g_string_append_printf (raw_content, "<%s>", tag_name);
    }

  g_queue_delete_link (tags_queue, last_tag);
}

static void
gn_xml_note_set_content_from_buffer (GnNote        *note,
                                     GtkTextBuffer *buffer)
{
  GnXmlNote *self = GN_XML_NOTE (note);
  GQueue *tags_queue;
  GString *raw_content;
  g_autofree gchar *content = NULL;
  GTimeVal time_val = {0, 0};
  GdkRGBA rgba;
  g_autofree gchar *color = NULL;
  gchar *time_str;
  GtkTextIter start, end, iter;
  gunichar c;

  g_assert (GN_IS_XML_NOTE (self));
  g_assert (GTK_IS_TEXT_BUFFER (buffer));

  tags_queue = g_queue_new ();
  raw_content = g_string_sized_new (gtk_text_buffer_get_char_count (buffer));

  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_iter_at_line_index (buffer, &end, 0, G_MAXINT);
  content = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

  /*
   * FIXME: This is really bad.  May be we should use single quotes.
   * Or better, may be we should use libxml or e_xml_document from eds.
   * We don't require most of the tags used here.  But for compatibility
   * with older Bijiben, we keep those as such.  May be we can remove
   * them after 10 years from now (2018 May).
   */
  g_string_append (raw_content, COMMON_XML_HEAD "\n<note version=\"1\" "
                   "xmlns:link=\"" BIJIBEN_XML_NS "/link\" "
                   "xmlns:size=\"" BIJIBEN_XML_NS "/size\" "
                   "xmlns=\""BIJIBEN_XML_NS"\">\n");
  g_string_append_printf (raw_content, "<title>%s</title>\n", content);
  g_string_append (raw_content,
                   "<text xml:space=\"preserve\"><html "
                   "xmlns=\"http://www.w3.org/1999/xhtml\">"
                   "<head><link rel=\"stylesheet\" href=\"Default.css\" "
                   "type=\"text/css\"/><script language=\"javascript\" "
                   "src=\"bijiben.js\"/></head><body contenteditable=\"true\" "
                   "id=\"editable\" style=\"color: black;\">");

  /* Temporarily disable "title" tag */
  gtk_text_buffer_remove_tag_by_name (buffer, "title", &start, &end);
  iter = start;

  do
    {
      GSList *tags;
      GSList *node;
      GtkTextTag *last_tag;

      c = gtk_text_iter_get_char (&iter);
      last_tag = g_queue_peek_head (tags_queue);

      if (c == '\n')
        g_string_append (raw_content, "<br />");

      /* First, we have to handle tags that are closed */
      tags = gtk_text_iter_get_toggled_tags (&iter, FALSE);
      while ((node = g_slist_find_custom (tags, last_tag, gn_xml_note_tag_compare)))
        {
          GtkTextTag *tag = node->data;
          gn_xml_note_close_tag (self, GN_NOTE_BUFFER (buffer),
                                 raw_content, tag, tags_queue);
          tags = g_slist_delete_link (tags, node);
        }

      g_assert (tags == NULL);

      /* Now, let's handle open tags */
      tags = gtk_text_iter_get_toggled_tags (&iter, TRUE);
      for (node = tags; node != NULL; node = node->next)
        {
          GtkTextTag *tag = node->data;
          const gchar *tag_name;

          tag_name = gn_note_buffer_get_name_for_tag (GN_NOTE_BUFFER (buffer),
                                                      tag);
          g_string_append_printf (raw_content, "<%s>", tag_name);
          g_queue_push_head (tags_queue, tag);
        }

      g_string_append_unichar (raw_content, c);
    } while (gtk_text_iter_forward_char (&iter));

  for (GList *node = tags_queue->head; node != NULL; node = node->next)
    {
      const gchar *tag_name;

      tag_name = gn_note_buffer_get_name_for_tag (GN_NOTE_BUFFER (buffer),
                                                  node->data);
      g_string_append_printf (raw_content, "</%s>", tag_name);
    }

  g_queue_free (tags_queue);

  g_string_append (raw_content, "</body></html></text>");

  time_val.tv_sec = gn_item_get_modification_time (GN_ITEM (self));
  time_str = g_time_val_to_iso8601 (&time_val);
  g_string_append_printf (raw_content,
                          "<last-change-date>%s</last-change-date>",
                          time_str);
  g_free (time_str);

  time_val.tv_sec = gn_item_get_modification_time (GN_ITEM (self));
  time_str = g_time_val_to_iso8601 (&time_val);
  g_string_append_printf (raw_content,
                          "<last-metadata-change-date>%s"
                          "</last-metadata-change-date>",
                          time_str);
  g_free (time_str);

  time_val.tv_sec = gn_item_get_creation_time (GN_ITEM (self));
  time_str = g_time_val_to_iso8601 (&time_val);
  g_string_append_printf (raw_content,
                          "<create-date>%s</create-date>",
                          time_str);
  g_free (time_str);

  g_string_append (raw_content, "<cursor-position>0</cursor-position>"
                   "<selection-bound-position>0</selection-bound-position>"
                   "<width>0</width>"
                   "<height>0</height>"
                   "<x>0</x>"
                   "<y>0</y>");

  gn_item_get_rgba (GN_ITEM (self), &rgba);
  color = gdk_rgba_to_string (&rgba);
  g_string_append_printf (raw_content, "<color>%s</color>", color);

  g_string_append (raw_content, "<tags/>"
                   "<open-on-startup>False</open-on-startup>"
                   "</note>");

  /* Add the removed "title" tag */
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_iter_at_line_index (buffer, &end, 0, G_MAXINT);
  gtk_text_buffer_apply_tag_by_name (buffer, "title", &start, &end);

  g_free (self->raw_content);
  self->raw_content = g_string_free (raw_content, FALSE);
}

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

static gchar *
gn_xml_note_get_raw_content (GnNote *note)
{
  GnXmlNote *self = GN_XML_NOTE (note);

  g_assert (GN_IS_NOTE (note));

  return g_strdup (self->raw_content);
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

  if (strstr (self->text_content, needle) != NULL)
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
}


/* static void */
/* gn_xml_note_parse_tomboy (GnXmlNote   *self, */
/*                           const gchar *xml) */
/* { */
/*   gchar *start, *end; */

/*   g_assert (GN_IS_XML_NOTE (self)); */
/*   g_assert (xml != NULL); */

/*   /\* Skip "note-content" tag *\/ */
/*   start = strchr (xml, '>'); */
/*   start++; */
/*   end = strchr (start, '\n'); */

/*   self->title = g_strndup (start, end - start); */
/* } */

/* static void */
/* gn_xml_note_parse_bijiben (GnXmlNote   *self, */
/*                            const gchar *xml) */
/* { */
/*   GQueue *tags; */
/*   gchar *start, *end; */

/*   g_assert (GN_IS_XML_NOTE (self)); */
/*   g_assert (xml != NULL); */

/*   tags = g_queue_new (); */
/*   /\* Skip "body" tag *\/ */
/*   start = strchr (xml, '>'); */
/*   start++; */
/*   end = strchr (start, '<'); */

/*   self->title = g_strndup (start, end - start); */
/* } */

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
