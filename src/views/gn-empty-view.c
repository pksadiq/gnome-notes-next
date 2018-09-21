/* gn-empty-view.c
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

#define G_LOG_DOMAIN "gn-empty-view"

#include "config.h"

#include <glib/gi18n.h>

#include "gn-empty-view.h"
#include "gn-trace.h"

/**
 * SECTION: gn-empty-view
 * @title: GnEmptyView
 * @short_description: View for empty states
 * @include: "gn-empty-view.h"
 */

struct _GnEmptyView
{
  GtkBox parent_instance;

  GtkWidget *primary_label;
};

G_DEFINE_TYPE (GnEmptyView, gn_empty_view, GTK_TYPE_BOX)

static void
gn_empty_view_class_init (GnEmptyViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-empty-view.ui");

  gtk_widget_class_bind_template_child (widget_class, GnEmptyView, primary_label);
}

static void
gn_empty_view_init (GnEmptyView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

void
gn_empty_view_set_view (GnEmptyView *self,
                        GnView       view)
{
  const gchar *primary_label;

  g_return_if_fail (GN_IS_EMPTY_VIEW (self));

  switch (view)
    {
    case GN_VIEW_NOTES:
    case GN_VIEW_NOTEBOOK_NOTES:
    case GN_VIEW_TRASH:
      primary_label =  _("No Notes");
      break;

    case GN_VIEW_NOTEBOOKS:
      primary_label =  _("No Notebooks");
      break;

    case GN_VIEW_SEARCH:
      primary_label =  _("No search results");
      break;

    case GN_VIEW_EDITOR:
      return;
      break;

    default:
      g_return_if_reached ();
    }

  g_assert_nonnull (self->primary_label);
  g_assert (GTK_IS_LABEL (self->primary_label));
  gtk_label_set_label (GTK_LABEL (self->primary_label),
                       primary_label);
}

