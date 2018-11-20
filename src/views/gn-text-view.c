/* gn-text-view.c
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

#define G_LOG_DOMAIN "gn-text-view"

#include "config.h"

#include "gn-note-buffer.h"
#include "gn-text-view.h"
#include "gn-trace.h"

/**
 * SECTION: gn-text-view
 * @title: GnTextView
 * @short_description: The Note editor view
 * @include: "gn-text-view.h"
 */

#define MAX_UNDO_LEVEL 100

struct _GnTextView
{
  GtkTextView parent_instance;

  GnNoteBuffer *buffer;

  GQueue *undo_queue;
  GList  *current_undo;
  guint   can_undo : 1;
  guint   can_redo : 1;

  guint   undo_freeze_count;
};

G_DEFINE_TYPE (GnTextView, gn_text_view, GTK_TYPE_TEXT_VIEW)


typedef enum ActionType {
  ACTION_TYPE_TEXT_ADD,
  ACTION_TYPE_TEXT_REMOVE,
  ACTION_TYPE_TAG_ADD,
  ACTION_TYPE_TAG_REMOVE,
} ActionType;

typedef struct _Action
{
  ActionType type;

  union {
    gchar *text;
    GtkTextTag *tag;
    GtkTextBuffer *buffer;
  };

  gint start;
  gint end;

  guint can_merge : 1;
} Action;

enum {
  PROP_0,
  PROP_CAN_UNDO,
  PROP_CAN_REDO,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
gn_text_view_undo_action_free (Action *action)
{
  if (action == NULL)
    return;

  if (action->type == ACTION_TYPE_TEXT_ADD)
    g_free (action->text);
  else if (action->type == ACTION_TYPE_TEXT_REMOVE)
    g_object_unref (action->buffer);

  g_slice_free (Action, action);
}

static void
gn_text_view_update_can_undo_redo (GnTextView *self)
{
  gboolean last_undo, last_redo;

  g_assert (GN_IS_TEXT_VIEW (self));

  last_undo = self->can_undo;
  last_redo = self->can_redo;

  if (g_queue_is_empty (self->undo_queue))
    {
      self->can_undo = FALSE;
      self->can_redo = FALSE;

      goto emit;
    }

  if (self->current_undo == NULL)
    {
      self->can_undo = TRUE;
      self->can_redo = FALSE;
    }
  else
    {
      self->can_redo = TRUE;

      if (self->current_undo->next != NULL)
        self->can_undo = TRUE;
      else
        self->can_undo = FALSE;
    }

 emit:
  if (last_undo != self->can_undo)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_UNDO]);

  if (last_redo != self->can_redo)
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_REDO]);
}

static gboolean
gn_text_view_action_can_merge (GnTextView *self,
                               Action     *action)
{
  Action *last_action;

  g_assert (GN_IS_TEXT_VIEW (self));
  g_assert (action != NULL);

  if (g_queue_is_empty (self->undo_queue))
    return FALSE;

  last_action = g_queue_peek_head (self->undo_queue);

  if (!last_action->can_merge ||
      !action->can_merge ||
      last_action->type != action->type ||
      ABS (action->start - action->end) > 1) /* if more than 1 char changed */
    return FALSE;

  if (action->type == ACTION_TYPE_TEXT_ADD &&
      (last_action->end != action->start ||
       g_ascii_isspace(*action->text))) /* If begins with space */
    return FALSE;

  if (action->type == ACTION_TYPE_TEXT_REMOVE)
    {
      GtkTextIter start;
      gunichar c;

      if (last_action->start != action->end)
        return FALSE;

      gtk_text_buffer_get_start_iter (last_action->buffer, &start);
      c = gtk_text_iter_get_char (&start);

      if (c == ' ')
        return FALSE;
    }

  return TRUE;
}

static void
gn_text_view_merge_text_add (Action *last_action,
                             Action *action)
{
  gchar *text;

  g_assert (last_action->start - last_action->end != 0);
  g_assert (action->start - action->end != 0);

  text = g_strconcat (last_action->text, action->text, NULL);
  g_free (last_action->text);
  last_action->text = text;

  last_action->end = action->end;
}

static void
gn_text_view_merge_text_remove (Action *last_action,
                                Action *action)
{
  GtkTextIter iter, start, end;

  g_assert (last_action->start - last_action->end != 0);
  g_assert (action->start - action->end != 0);

  gtk_text_buffer_get_bounds (action->buffer, &start, &end);
  gtk_text_buffer_get_start_iter (last_action->buffer, &iter);
  gtk_text_buffer_insert_range (last_action->buffer,
                                &iter, &start, &end);

  last_action->start = action->start;
}

static gboolean
gn_text_view_merge_action (GnTextView *self,
                           Action     *action)
{
  Action *last_action;

  g_assert (GN_IS_TEXT_VIEW (self));
  g_assert (action != NULL);

  if (g_queue_is_empty (self->undo_queue))
    return FALSE;

  if (self->current_undo != NULL)
    return FALSE;

  last_action = g_queue_peek_head (self->undo_queue);

  /* Force to not merge for changes more than 1 character */
  if (ABS (action->start - action->end) > 1)
    action->can_merge = FALSE;

  if (!gn_text_view_action_can_merge (self, action))
    {
      last_action->can_merge = FALSE;
      return FALSE;
    }

  if (action->type == ACTION_TYPE_TEXT_ADD)
    gn_text_view_merge_text_add (last_action, action);
  if (action->type == ACTION_TYPE_TEXT_REMOVE)
    gn_text_view_merge_text_remove (last_action, action);

  gn_text_view_undo_action_free (action);

  return TRUE;
}

static void
gn_text_view_add_undo_action (GnTextView *self,
                              Action     *action)
{
  g_assert (GN_IS_TEXT_VIEW (self));
  g_assert (action != NULL);

  if (gn_text_view_merge_action (self, action))
    return;

  if (self->current_undo != NULL)
    {
      self->current_undo = self->current_undo->next;

      while (self->undo_queue->head != self->current_undo)
        {
          Action *action;

          action = g_queue_pop_head (self->undo_queue);
          gn_text_view_undo_action_free (action);
        }
    }

  self->current_undo = NULL;
  g_queue_push_head (self->undo_queue, action);

  gn_text_view_update_can_undo_redo (self);
}

static void
gn_text_view_insert_text_cb (GnTextView  *self,
                             GtkTextIter *location,
                             gchar       *text,
                             gint         len)
{
  Action *action;

  g_assert (GN_IS_TEXT_VIEW (self));

  action = g_slice_new (Action);
  action->type = ACTION_TYPE_TEXT_ADD;
  action->text = g_memdup (text, len + 1);
  action->start = gtk_text_iter_get_offset (location);
  action->end = action->start + len;
  action->can_merge = TRUE;

  gn_text_view_add_undo_action (self, action);
}

static void
gn_text_view_delete_range_cb (GnTextView  *self,
                              GtkTextIter *start,
                              GtkTextIter *end)
{
  GtkTextBuffer *buffer;
  GtkTextTagTable *tag_table;
  Action *action;
  GtkTextIter start_iter;

  g_assert (GN_IS_TEXT_VIEW (self));

  tag_table = gtk_text_buffer_get_tag_table (GTK_TEXT_BUFFER (self->buffer));
  buffer = gtk_text_buffer_new (tag_table);
  action = g_slice_new (Action);
  action->type = ACTION_TYPE_TEXT_REMOVE;
  action->buffer = buffer;
  action->start = gtk_text_iter_get_offset (start);
  action->end = gtk_text_iter_get_offset (end);
  action->can_merge = TRUE;

  gtk_text_buffer_get_start_iter (buffer, &start_iter);
  gtk_text_buffer_insert_range (buffer, &start_iter, start, end);

  gn_text_view_add_undo_action (self, action);
}

static void
gn_text_view_apply_tag_cb (GtkTextBuffer *buffer,
                           GtkTextTag    *tag,
                           GtkTextIter   *start,
                           GtkTextIter   *end,
                           GnTextView    *self)
{
  Action *action;

  g_assert (GTK_IS_TEXT_BUFFER (buffer));
  g_assert (GTK_IS_TEXT_TAG (tag));

  action = g_slice_new (Action);
  action->type = ACTION_TYPE_TAG_ADD;
  action->tag = tag;
  action->start = gtk_text_iter_get_offset (start);
  action->end = gtk_text_iter_get_offset (end);
  action->can_merge = FALSE;

  gn_text_view_add_undo_action (self, action);
}

static void
gn_text_view_remove_tag_cb (GtkTextBuffer *buffer,
                            GtkTextTag    *tag,
                            GtkTextIter   *start,
                            GtkTextIter   *end,
                            GnTextView    *self)
{
  Action *action;

  g_assert (GTK_IS_TEXT_BUFFER (buffer));
  g_assert (GTK_IS_TEXT_TAG (tag));

  action = g_slice_new (Action);
  action->type = ACTION_TYPE_TAG_REMOVE;
  action->tag = tag;
  action->start = gtk_text_iter_get_offset (start);
  action->end = gtk_text_iter_get_offset (end);
  action->can_merge = FALSE;

  gn_text_view_add_undo_action (self, action);
}

/* undo/redo actions */
static void   gn_text_view_text_remove (GnTextView *self,
                                       Action     *action);
static void
gn_text_view_text_add (GnTextView *self,
                       Action     *action)
{
  GtkTextIter start;

  g_assert (GN_IS_TEXT_VIEW (self));
  g_assert (action != NULL);

  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (self->buffer),
                                      &start, action->start);

  if (action->type == ACTION_TYPE_TEXT_ADD)
    gtk_text_buffer_insert (GTK_TEXT_BUFFER (self->buffer),
                            &start, action->text,
                            ABS (action->end - action->start));
  else  /* ACTION_TYPE_TEXT_REMOVE */
    {
      GtkTextIter start_iter, end_iter;

      gtk_text_buffer_get_bounds (action->buffer, &start_iter, &end_iter);
      gtk_text_buffer_insert_range (GTK_TEXT_BUFFER (self->buffer),
                                    &start, &start_iter, &end_iter);
    }
}

static void
gn_text_view_text_remove (GnTextView *self,
                          Action     *action)
{
  GtkTextMark *mark;
  GtkTextIter start, end;

  g_assert (GN_IS_TEXT_VIEW (self));
  g_assert (action != NULL);

  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (self->buffer),
                                      &start, action->start);
  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (self->buffer),
                                      &end, action->end);

  gtk_text_buffer_delete (GTK_TEXT_BUFFER (self->buffer),
                          &start, &end);

  gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (self->buffer), &start);

  mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (self->buffer));
  gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (self), mark);
}

static void
gn_text_view_remove_tag (GnTextView *self,
                         Action     *action)
{
  GtkTextIter start, end;

  g_assert (GN_IS_TEXT_VIEW (self));
  g_assert (action != NULL);

  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (self->buffer),
                                      &start, action->start);
  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (self->buffer),
                                      &end, action->end);

  gtk_text_buffer_remove_tag (GTK_TEXT_BUFFER (self->buffer),
                              action->tag, &start, &end);
}

static void
gn_text_view_add_tag (GnTextView *self,
                      Action     *action)
{
  GtkTextIter start, end;

  g_assert (GN_IS_TEXT_VIEW (self));
  g_assert (action != NULL);

  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (self->buffer),
                                      &start, action->start);
  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (self->buffer),
                                      &end, action->end);

  gtk_text_buffer_apply_tag (GTK_TEXT_BUFFER (self->buffer),
                             action->tag, &start, &end);
}

static void
gn_text_view_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GnTextView *self = (GnTextView *)object;

  switch (prop_id)
    {
    case PROP_CAN_UNDO:
      g_value_set_boolean (value, self->can_undo);
      break;

    case PROP_CAN_REDO:
      g_value_set_boolean (value, self->can_redo);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_text_view_constructed (GObject *object)
{
  GnTextView *self = GN_TEXT_VIEW (object);

  self->undo_queue = g_queue_new ();
  self->buffer = gn_note_buffer_new ();
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (self),
                            GTK_TEXT_BUFFER (self->buffer));

  g_signal_connect_object (self->buffer, "insert-text",
                           G_CALLBACK (gn_text_view_insert_text_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->buffer, "delete-range",
                           G_CALLBACK (gn_text_view_delete_range_cb),
                           self, G_CONNECT_SWAPPED);

  g_signal_connect_object (self->buffer, "apply-tag",
                           G_CALLBACK (gn_text_view_apply_tag_cb),
                           self, G_CONNECT_AFTER);
  g_signal_connect_object (self->buffer, "remove-tag",
                           G_CALLBACK (gn_text_view_remove_tag_cb),
                           self, G_CONNECT_AFTER);

  G_OBJECT_CLASS (gn_text_view_parent_class)->constructed (object);
}

static void
gn_text_view_finalize (GObject *object)
{
  GnTextView *self = GN_TEXT_VIEW (object);

  g_object_unref (self->buffer);

  G_OBJECT_CLASS (gn_text_view_parent_class)->finalize (object);
}

static void
gn_text_view_class_init (GnTextViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gn_text_view_get_property;
  object_class->constructed = gn_text_view_constructed;
  object_class->finalize = gn_text_view_finalize;

  properties[PROP_CAN_UNDO] =
    g_param_spec_boolean ("can-undo",
                          "Can undo",
                          "If undo can be done or not",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_CAN_REDO] =
    g_param_spec_boolean ("can-redo",
                          "Can redo",
                          "If redo can be done or not",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gn_text_view_init (GnTextView *self)
{
}

GtkWidget *
gn_text_view_new (void)
{
  return g_object_new (GN_TYPE_TEXT_VIEW,
                       NULL);
}

static gboolean
gn_text_view_may_be_overwrite (GnTextView *self,
                               GList      *item,
                               GList      *next_item)
{
  Action *action, *next_action;

  if (item == NULL || next_item == NULL)
    return FALSE;

  action = item->data;
  next_action = next_item->data;

  /* We care only in place one character changes */
  if (action->start != next_action->start ||
      ABS (action->start - action->end) > 1 ||
      ABS (next_action->start - next_action->end) > 1)
    return FALSE;

  if (action->type == ACTION_TYPE_TEXT_ADD &&
      next_action->type == ACTION_TYPE_TEXT_REMOVE)
    return TRUE;

  if (action->type == ACTION_TYPE_TEXT_REMOVE &&
      next_action->type == ACTION_TYPE_TEXT_ADD)
    return TRUE;

  return FALSE;
}


void
gn_text_view_freeze_undo_redo (GnTextView *self)
{
  self->undo_freeze_count++;

  /* We have freezed everything once already */
  if (self->undo_freeze_count > 1)
    return;

  g_signal_handlers_block_by_func (self->buffer,
                                   gn_text_view_insert_text_cb, self);
  g_signal_handlers_block_by_func (self->buffer,
                                   gn_text_view_delete_range_cb, self);
  g_signal_handlers_block_by_func (self->buffer,
                                   gn_text_view_apply_tag_cb, self);
  g_signal_handlers_block_by_func (self->buffer,
                                   gn_text_view_remove_tag_cb, self);
}

void
gn_text_view_thaw_undo_redo (GnTextView *self)
{
  self->undo_freeze_count--;

  if (self->undo_freeze_count > 0)
    return;

  g_signal_handlers_unblock_by_func (self->buffer,
                                     gn_text_view_insert_text_cb, self);
  g_signal_handlers_unblock_by_func (self->buffer,
                                     gn_text_view_delete_range_cb, self);
  g_signal_handlers_unblock_by_func (self->buffer,
                                     gn_text_view_apply_tag_cb, self);
  g_signal_handlers_unblock_by_func (self->buffer,
                                     gn_text_view_remove_tag_cb, self);
}

void
gn_text_view_undo (GnTextView *self)
{
  Action *action;

  g_return_if_fail (GN_IS_TEXT_VIEW (self));
  g_return_if_fail (!g_queue_is_empty (self->undo_queue));

  if (self->current_undo == NULL)
    self->current_undo = self->undo_queue->head;
  else
    self->current_undo = self->current_undo->next;

  action = self->current_undo->data;

  gn_text_view_freeze_undo_redo (self);

  if (action->type == ACTION_TYPE_TEXT_ADD)
    {
      gn_text_view_text_remove (self, action);
      if (gn_text_view_may_be_overwrite (self, self->current_undo,
                                         self->current_undo->next))
        {
          self->current_undo = self->current_undo->next;
          gn_text_view_text_add (self, self->current_undo->data);
        }
    }
  if (action->type == ACTION_TYPE_TEXT_REMOVE)
    gn_text_view_text_add (self, action);
  else if (action->type == ACTION_TYPE_TAG_ADD)
    gn_text_view_remove_tag (self, action);
  else if (action->type == ACTION_TYPE_TAG_REMOVE)
    gn_text_view_add_tag (self, action);

  gn_text_view_thaw_undo_redo (self);

  gn_text_view_update_can_undo_redo (self);
}

void
gn_text_view_redo (GnTextView *self)
{
  Action *action;

  g_return_if_fail (GN_IS_TEXT_VIEW (self));
  g_return_if_fail (!g_queue_is_empty (self->undo_queue));
  g_return_if_fail (self->current_undo != NULL);

  action = self->current_undo->data;

  gn_text_view_freeze_undo_redo (self);

  if (action->type == ACTION_TYPE_TEXT_ADD)
    gn_text_view_text_add (self, action);
  if (action->type == ACTION_TYPE_TEXT_REMOVE)
    {
      gn_text_view_text_remove (self, action);

      if (gn_text_view_may_be_overwrite (self, self->current_undo,
                                         self->current_undo->prev))
        {
          self->current_undo = self->current_undo->prev;
          gn_text_view_text_add (self, self->current_undo->data);
        }
    }
  else if (action->type == ACTION_TYPE_TAG_ADD)
    gn_text_view_add_tag (self, action);
  else if (action->type == ACTION_TYPE_TAG_REMOVE)
    gn_text_view_remove_tag (self, action);

  gn_text_view_thaw_undo_redo (self);
  self->current_undo = self->current_undo->prev;

  gn_text_view_update_can_undo_redo (self);
}
