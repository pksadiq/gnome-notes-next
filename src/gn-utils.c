/* gn-utils.c
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

#define G_LOG_DOMAIN "gn-application"

#include "config.h"

#include "gn-utils.h"
#include "gn-trace.h"

/*
 * gn_utils_append_tags_queue:
 *
 * Add closing tags for items in @tags_queue to the string
 * in @str.  Say for example, if @tags_queue has the head
 * element as "b", the string @str will be append with "</b>"
 * and "b" will be popped from the @tags_queue.
 */
static void
gn_utils_append_tags_queue (GString *str,
                            GQueue  *tags_queue)
{
  g_assert (str != NULL);

  if (tags_queue == NULL)
    return;

  while (tags_queue->length > 0)
    {
      g_autofree gchar *tag = g_queue_pop_head (tags_queue);

      g_string_append (str, "</");
      g_string_append (str, tag);
      g_string_append_c (str, '>');
    }
}

/*
 * gn_utils_handle_bijiben_tag:
 *
 * @tag_start should begin with '<'
 * @tag_end should be pointing to the char before '>'
 */
static void
gn_utils_handle_bijiben_tag (GString     *str,
                             GQueue      *tags_queue,
                             const gchar *tag_start,
                             const gchar *tag_end)
{
  const gchar *start;

  g_assert (str != NULL);
  g_assert (tag_start != NULL);
  g_assert (tag_end != NULL);

  start = tag_start;

  /* skip '<' */
  tag_start++;

  if (g_str_has_prefix (tag_start, "div"))
    {
      if (!g_str_has_prefix (tag_end, "><br />"))
        g_string_append (str, "\n");
    }
  else if (g_str_has_prefix (tag_start, "/div"))
    {
      /* Do nothing */
    }
  else if (g_str_has_prefix (tag_start, "br"))
    {
      g_string_append_c (str, '\n');
    }
  else if (g_str_has_prefix (tag_start, "strike"))
    {
      g_string_append (str, "<s>");
      g_queue_push_head (tags_queue, g_strdup ("s"));
    }
  else if (g_str_has_prefix (tag_start, "/strike"))
    {
      g_string_append (str, "</s>");
      if (tags_queue->length > 0)
        {
          gchar *tag = g_queue_peek_head (tags_queue);

          if (strcmp (tag, "s") == 0)
            {
              tag = g_queue_pop_head (tags_queue);
              g_free (tag);
            }
        }
    }
  else
    {
      /* tag_end doesn't cover the ending '>'.  So add one more */
      g_string_append_len (str, start, tag_end - start + 1);

      /* That is, if tag isn't a closing tag */
      if (*tag_start != '/')
        {
          g_queue_push_head (tags_queue,
                             g_strndup (tag_start, tag_end - tag_start));
        }
      else if (tags_queue->length > 0)
        {
          gchar *tag = g_queue_peek_head (tags_queue);

          /* Skip '/' */
          tag_start++;

          if (g_str_has_prefix (tag_start, tag))
            {
              tag = g_queue_pop_head (tags_queue);
              g_free (tag);
            }
        }
    }
}

/**
 * gn_utils_get_main_thread:
 *
 * Returns the thread-id of the main thread. Useful
 * to check if the current is the main UI thread or
 * not. This is used by GN_IS_MAIN_THREAD() macro.
 *
 * The first call of this function should be done in
 * the UI thread, the class_init() of #GApplication
 * is a good place to do.
 *
 * Returns: (transfer none): a #GThread
 */
GThread *
gn_utils_get_main_thread (void)
{
  static GThread *main_thread;

  if (main_thread == NULL)
    main_thread = g_thread_self ();

  return main_thread;
}

static void
gn_utils_append_unescaped (GString     *str,
                           const gchar *start,
                           const gchar *end)
{
  gchar c;

  if (g_str_has_prefix (start, "&lt;"))
    c = '<';
  else if (g_str_has_prefix (start, "&gt;"))
    c = '>';
  else if (g_str_has_prefix (start, "&amp;"))
    c = '&';
  else if (g_str_has_prefix (start, "&quote;"))
    c = '"';
  else
    c = '\0';

  g_string_append_c (str, c);
}

static void
gn_utils_append_string (GString     *str,
                        const gchar *start,
                        const gchar *end)
{
  if (start != end)
    {
      g_string_append_len (str, start, end - start);
    }
}

/**
 * gn_utils_get_markup_from_xml:
 * @xml: A valid XML string.
 * @max_line: Maximum number of lines to parse.
 *
 * Make a Bijiben XML note text compatible with pango
 * markup.  Right now, this does support only a basic subset
 * of pango markup (the subset that is supported by Bijiben).
 *
 * Unknown/Unsupported xml tags are appended as such.
 * It is assumed that only tags present in Bijiben note
 * format will be present in @xml.
 *
 * The primary use of this function is to replace <div> and
 * <br /> tags heavily used in old Bijiben format to simple
 * newlines.
 *
 * Also, the tags like <ul>, <ol>, and <li> are replaced with
 * number or bullet because it isn't supported by Pango markup
 * parser.
 *
 * The returned string will be a valid XML. If the tags are
 * incomplete at the end of @max_line, pending close tags
 * will be added, if required.
 *
 * Returns: (transfer full): A pango compatible markup string
 */
gchar *
gn_utils_get_markup_from_bijiben (const gchar *xml,
                                  gint         max_line)
{
  GQueue *tags_queue;
  GString *str;
  const gchar *start, *end;
  const gchar *tag_end;
  gint line = 0;
  gchar c;

  g_return_val_if_fail (xml != NULL, NULL);

  /* Some random size */
  str = g_string_sized_new (10);

  tags_queue = g_queue_new ();
  start = end = xml;

  g_string_append (str, "<b>");
  g_queue_push_head (tags_queue, g_strdup ("b"));

  while ((c = *end) && line < max_line)
    {
      if (c == '<')
        {
          gn_utils_append_string (str, start, end);
          tag_end = strchr (end, '>');
          gn_utils_handle_bijiben_tag (str, tags_queue, end, tag_end);

          /* skip '<' */
          end++;

          if (g_str_has_prefix (end, "div"))
            {
              /* The first line is assumed to be title.  So let's
               do some fancy things. */
              if (line == 0)
                {
                  gn_utils_append_tags_queue (str, tags_queue);
                  g_string_append_c (str, '\n');
                }

              line++;
            }

          /* Skip '>' */
          tag_end++;

          end = tag_end;
          start = end;
        }
      else
        end++;
    }

  gn_utils_append_string (str, start, end);
  gn_utils_append_tags_queue (str, tags_queue);
  g_queue_free (tags_queue);

  return g_string_free (str, FALSE);
}

/**
 * gn_utils_get_text_from_xml:
 * @xml: An XML string.
 *
 * Remove all tags from @xml. Also, unescape &lt;
 * &gt; &amp; and &quote;
 *
 * Returns: (transfer full): A string.  Free with g_free()
 */
gchar *
gn_utils_get_text_from_xml (const gchar *xml)
{
  GString *str;
  const gchar *start, *end;
  gchar c;

  if (xml == NULL)
    return g_strdup ("");

  /* Some random size */
  str = g_string_sized_new (10);
  start = end = xml;

  while ((c = *end))
    {
      if (c == '<')
        {
          gn_utils_append_string (str, start, end);

          /* Skip the tag */
          start = strchr (end, '>');
          start++;
          end = start;
        }
      else if (c == '&')
        {
          gn_utils_append_string (str, start, end);

          start = end;
          end = strchr (end, ';');

          gn_utils_append_unescaped (str, start, end);

          /* Skip ';' */
          end++;
          start = end;
        }
      else
        end++;
    }

  gn_utils_append_string (str, start, end);

  return g_string_free (str, FALSE);
}
