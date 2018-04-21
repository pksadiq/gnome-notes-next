#define G_LOG_DOMAIN "gn-plain-note"

#include "config.h"

#include <gtk/gtk.h>

#include "gn-plain-note.h"
#include "gn-trace.h"

/**
 * SECTION: gn-plain-note
 * @title: GnPlainNote
 * @short_description:
 * @include: "gn-plain-note.h"
 */

struct _GnPlainNote
{
  GnNote parent_instance;

  gchar *content;
};

G_DEFINE_TYPE (GnPlainNote, gn_plain_note, GN_TYPE_NOTE)

static void
gn_plain_note_set_content_from_buffer (GnNote        *note,
                                       GtkTextBuffer *buffer)
{
  GnPlainNote *self = GN_PLAIN_NOTE (note);
  g_autofree gchar *full_content = NULL;
  gchar *title = NULL, *content = NULL;
  g_auto(GStrv) split_data;

  g_object_get (G_OBJECT (buffer), "text", &full_content, NULL);

  /* We shall have at most 2 parts: title and content */
  split_data = g_strsplit (full_content, "\n", 2);

  if (split_data[0] != NULL)
    {
      title = split_data[0];

      if (split_data[1] != NULL)
        content = split_data[1];
    }

  g_free (self->content);
  self->content = g_strdup (content);
  gn_item_set_title (GN_ITEM (note), title);
}

static void
gn_plain_note_finalize (GObject *object)
{
  GnPlainNote *self = (GnPlainNote *)object;

  GN_ENTRY;

  g_clear_pointer (&self->content, g_free);

  G_OBJECT_CLASS (gn_plain_note_parent_class)->finalize (object);

  GN_EXIT;
}

static gchar *
gn_plain_note_get_text_content (GnNote *note)
{
  GnPlainNote *self = GN_PLAIN_NOTE (note);

  g_assert (GN_IS_NOTE (note));

  return g_strdup (self->content);
}

static gchar *
gn_plain_note_get_markup (GnNote *note)
{
  GnPlainNote *self = GN_PLAIN_NOTE (note);
  g_autofree gchar *title = NULL;
  g_autofree gchar *content = NULL;

  g_assert (GN_IS_NOTE (note));

  title = g_markup_escape_text (gn_item_get_title (GN_ITEM (note)), -1);

  if (self->content != NULL)
    content = g_markup_escape_text (self->content, -1);

  return g_strconcat ("<b>", title, "</b>\n\n",
                      content, NULL);
}

static void
gn_plain_note_class_init (GnPlainNoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GnNoteClass *note_class = GN_NOTE_CLASS (klass);

  object_class->finalize = gn_plain_note_finalize;

  note_class->get_raw_content = gn_plain_note_get_text_content;
  note_class->get_text_content = gn_plain_note_get_text_content;
  note_class->get_markup = gn_plain_note_get_markup;

  note_class->set_content_from_buffer = gn_plain_note_set_content_from_buffer;
}

static void
gn_plain_note_init (GnPlainNote *self)
{
}


static GnPlainNote *
gn_plain_note_create_from_data (const gchar *data)
{
  g_auto(GStrv) split_data;
  gchar *title = NULL, *content = NULL;
  GnPlainNote *note;

  g_assert (data != NULL);

  /* We shall have at most 2 parts: title and content */
  split_data = g_strsplit (data, "\n", 2);

  if (split_data[0] != NULL)
    {
      title = split_data[0];

      if (split_data[1] != NULL)
        content = split_data[1];
    }

  note = g_object_new (GN_TYPE_PLAIN_NOTE,
                       "title", title,
                       NULL);

  note->content = g_strdup (content);

  return note;
}

/**
 * gn_plain_note_new_from_data:
 * @data (nullable): The raw note content
 *
 * Create a new plain note from the given data.
 *
 * Returns: (transfer full): a new #GnPlainNote
 * with given content.
 */
GnPlainNote *
gn_plain_note_new_from_data (const gchar *data)
{
  if (data == NULL)
    return g_object_new (GN_TYPE_PLAIN_NOTE, NULL);

  return gn_plain_note_create_from_data (data);
}
