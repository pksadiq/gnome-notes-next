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


#include <glib/gi18n.h>
#include <stdio.h>

#include "gn-utils.h"
#include "gn-trace.h"

/*
 * Not an efficient way.  But we require
 * only simple uses.
 */
static GQueue *
gn_utils_copy_string_queue (GQueue *queue)
{
  GQueue *new_queue;

  new_queue = g_queue_copy (queue);

  for (GList *list = new_queue->head; list != NULL; list = list->next)
    list->data = g_strdup (list->data);

  return new_queue;
}

/*
 * gn_utils_append_tags_queue:
 *
 * Add closing tags for items in @tags_queue to the string
 * in @str.  Say for example, if @tags_queue has the head
 * element as "b", the string @str will be append with "</b>"
 * and "b" will be popped from the @tags_queue.
 */
static void
gn_utils_append_tags_queue (GString  *str,
                            GQueue   *tags_queue,
                            gboolean  open_tag)
{
  g_assert (str != NULL);

  if (tags_queue == NULL)
    return;

  while (tags_queue->length > 0)
    {
      g_autofree gchar *tag = g_queue_pop_head (tags_queue);

      if (open_tag)
              g_string_append (str, "<");
      else
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
                             const gchar *tag_end,
                             gboolean     last_is_div)
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
      if (!last_is_div)
        g_string_append (str, "\n");
    }
  else if (g_str_has_prefix (tag_start, "/div") ||
           g_str_has_prefix (tag_start, "br"))
    {
      /* Do nothing */
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
 * gn_utils_get_markup_from_bijiben:
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
  gboolean last_is_div = FALSE;
  gchar c;

  g_return_val_if_fail (xml != NULL, NULL);

  /* Some random size */
  str = g_string_sized_new (10);

  tags_queue = g_queue_new ();
  start = end = xml;

  end = strstr (xml, "<body");
  end = strchr (end, '>');
  end++;
  start = end;

  g_string_append (str, "<b>");
  g_queue_push_head (tags_queue, g_strdup ("b"));

  while ((c = *end) && line < max_line)
    {
      if (c == '\n')
        {
          if (line == 0)
            {
              GQueue *queue = NULL;

              if (tags_queue->length > 1)
                queue = gn_utils_copy_string_queue (tags_queue);

              gn_utils_append_tags_queue (str, tags_queue, FALSE);

              /*
               * Sometimes the title may have extra tags applied.
               * So we have to readd all the tags we have removed
               * to not break the markup, except the last tag
               * which is "b", which we added ourself.
               */
              if (queue)
                {
                  gchar *tag;

                  tag = g_queue_pop_tail (queue);
                  g_free (tag);

                  g_queue_reverse (queue);
                  gn_utils_append_tags_queue (str, queue, TRUE);
                }

              g_string_append_c (str, '\n');
            }

          end++;
          gn_utils_append_string (str, start, end);

          line++;
          start = end;
        }
      else if (c == '<')
        {
          gn_utils_append_string (str, start, end);
          tag_end = strchr (end, '>');

          if (g_str_has_prefix (end, "</body"))
            {
              start = end;
              break;
            }

          gn_utils_handle_bijiben_tag (str, tags_queue, end, tag_end,
                                       last_is_div);

          /* skip '<' */
          end++;

          if (g_str_has_prefix (end, "div"))
            {
              /* The first line is assumed to be title.  So let's
               do some fancy things. */
              if (line == 0)
                {
                  GQueue *queue = NULL;

                  if (tags_queue->length > 1)
                    queue = gn_utils_copy_string_queue (tags_queue);

                  gn_utils_append_tags_queue (str, tags_queue, FALSE);

                  if (queue)
                    {
                      gchar *tag;

                      tag = g_queue_pop_tail (queue);
                      g_free (tag);
                      g_queue_reverse (queue);
                      gn_utils_append_tags_queue (str, queue, TRUE);
                    }

                  g_string_append_c (str, '\n');
                }

              if (!last_is_div)
                line++;
              last_is_div = TRUE;
            }
          else
            last_is_div = FALSE;

          /* Skip '>' */
          tag_end++;

          end = tag_end;
          start = end;
        }
      else
        {
          last_is_div = FALSE;
          end++;
        }
    }

  gn_utils_append_string (str, start, end);
  gn_utils_append_tags_queue (str, tags_queue, FALSE);
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
          g_return_val_if_fail (start != NULL, g_string_free (str, FALSE));

          start++;
          end = start;
        }
      else if (c == '&')
        {
          gn_utils_append_string (str, start, end);

          start = end;
          end = strchr (end, ';');
          g_return_val_if_fail (end != NULL, g_string_free (str, FALSE));

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

/**
 * gn_utils_get_other_view_type:
 * @view: (nullable): The current view
 *
 * Get the alternate view for the given @view.
 * @view can either be “list” or “grid”.
 *
 * If @view is “list”, the function returns “grid”.
 * Else, “list” is returned.
 *
 * Returns: (transfer none): A string.
 */
const gchar *
gn_utils_get_other_view_type (const gchar *view)
{
  if (view && g_str_equal (view, "list"))
    return "grid";
  else
    return "list";
}

/**
 * gn_utils_get_item_position:
 * @model: A #GListModel
 * @item: A #GObject or derived object.
 * @position: (out): A location to store position
 *
 * Get the position of @item in @model.
 * This is an O(n) operation.
 *
 * Returns: %TRUE if @item was found in @model.
 */
gboolean
gn_utils_get_item_position (GListModel *model,
                            gpointer    item,
                            guint      *position)
{
  gpointer object;
  guint i = 0;

  g_return_val_if_fail (G_IS_LIST_MODEL (model), FALSE);
  g_return_val_if_fail (item != NULL, FALSE);

  /* This maybe slow. But, for now let's do. */
  while ((object = g_list_model_get_item (model, i)))
    {
      if (object == (gpointer) item)
        {
          *position = i;
          g_object_unref (object);
          return TRUE;
        }
      g_object_unref (object);
      i++;
    }

  return FALSE;
}

/**
 * gn_utils_unix_time_to_iso:
 * @unix_time: seconds since Epoch
 *
 * Get ISO8601 string representing the given @unix_time.
 * @unix_time is assumed to be in UTC.
 *
 * Returns: A new string.  Free with g_free().
 */
gchar *
gn_utils_unix_time_to_iso (gint64 unix_time)
{
  g_autoptr (GDateTime) date_time = NULL;

  date_time = g_date_time_new_from_unix_utc (unix_time);

  return g_date_time_format (date_time, "%FT%TZ");
}

/**
 * gn_utils_get_human_time:
 * @unix_time: seconds since Epoch
 *
 * Get a human readable representation of the time
 * @unix_time.
 *
 * The time returned isn't always in the same format.
 * Say if @unix_time represents the current day,
 * a string with time like "07:30" is returned.
 * Else if @unix_time represents a preceding day from
 * the same week the week name is returned, and so on.
 *
 * Returns: A new string.  Free with g_free().
 */
gchar *
gn_utils_get_human_time (gint64 unix_time)
{
  g_autoptr(GDateTime) now = NULL;
  g_autoptr(GDateTime) utc_time = NULL;
  g_autoptr(GDateTime) local_time = NULL;
  gint year_now, month_now, day_now;
  gint year, month, day;

  g_return_val_if_fail (unix_time >= 0, g_strdup (_("Unknown")));

  now = g_date_time_new_now_local ();
  utc_time = g_date_time_new_from_unix_utc (unix_time);
  local_time = g_date_time_to_local (utc_time);

  g_date_time_get_ymd (now, &year_now, &month_now, &day_now);
  g_date_time_get_ymd (local_time, &year, &month, &day);

  if (year == year_now &&
      month == month_now)
    {
      /* Time in the format HH:MM */
      if (day == day_now)
        return g_date_time_format (local_time, "%R");

      if (day_now - day == 1)
        return g_strdup (_("Yesterday"));

      /* Localized day name */
      if (day_now - day <= 7)
        return g_date_time_format (local_time, "%A");

      return g_strdup (_("This month"));
    }

  /* Localized month name */
  if (year == year_now)
    return g_date_time_format (local_time, "%B");

  /* Year */
  return g_date_time_format (local_time, "%Y");
}
