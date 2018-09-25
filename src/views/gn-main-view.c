/* gn-main-view.c
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

#define G_LOG_DOMAIN "gn-main-view"

#include "config.h"

#include "gn-grid-view-item.h"
#include "gn-list-view-item.h"
#include "gn-grid-view.h"
#include "gn-list-view.h"
#include "gn-main-view.h"
#include "gn-trace.h"

/**
 * SECTION: gn-main-view
 * @title: GnMainView
 * @short_description:
 * @include: "gn-main-view.h"
 */

struct _GnMainView
{
  GtkGrid parent_instance;

  GListModel *model;

  GtkWidget *view_stack;
  GtkWidget *grid_view;
  GtkWidget *list_view;
  GtkWidget *empty_view;

  GnViewType current_view;
  gboolean selection_mode;
};

G_DEFINE_TYPE (GnMainView, gn_main_view, GTK_TYPE_GRID)

enum {
  PROP_0,
  PROP_SELECTION_MODE,
  PROP_MODEL,
  N_PROPS
};

enum {
  ITEM_ACTIVATED,
  N_SIGNALS
};

static GParamSpec *properties[N_PROPS];
static guint signals[N_SIGNALS];

static void
gn_main_view_model_changed (GListModel *model,
                            guint       position,
                            guint       removed,
                            guint       added,
                            GnMainView *self)
{
  g_assert (GN_IS_MAIN_VIEW (self));
  g_assert (G_IS_LIST_MODEL (model));

  /* TODO: Refactor */
  if (g_list_model_get_item (model, 0) == NULL)
    gtk_stack_set_visible_child (GTK_STACK (self->view_stack),
                                 self->empty_view);
  else if (self->current_view == GN_VIEW_TYPE_GRID)
    gtk_stack_set_visible_child (GTK_STACK (self->view_stack),
                                 self->grid_view);
  else
    gtk_stack_set_visible_child (GTK_STACK (self->view_stack),
                                 self->list_view);
}

static void
gn_main_view_grid_item_activated (GnMainView      *self,
                                  GtkFlowBoxChild *child,
                                  GtkFlowBox      *box)
{
  GnGridViewItem *item = GN_GRID_VIEW_ITEM (child);

  g_assert (GN_IS_MAIN_VIEW (self));
  g_assert (GTK_IS_FLOW_BOX (box));
  g_assert (GTK_IS_FLOW_BOX_CHILD (child));

  if (gtk_flow_box_get_selection_mode (box) != GTK_SELECTION_MULTIPLE)
    g_signal_emit (self, signals[ITEM_ACTIVATED], 0,
                   gn_grid_view_item_get_item (item));
  else
    gn_grid_view_item_toggle_selection (item);
}

static void
gn_main_view_list_item_activated (GnMainView    *self,
                                  GtkListBoxRow *row,
                                  GtkListBox    *box)
{
  GnListViewItem *item = GN_LIST_VIEW_ITEM (row);

  g_assert (GN_IS_MAIN_VIEW (self));
  g_assert (GTK_IS_LIST_BOX (box));
  g_assert (GTK_IS_LIST_BOX_ROW (row));

  if (gtk_list_box_get_selection_mode (box) != GTK_SELECTION_MULTIPLE)
    g_signal_emit (self, signals[ITEM_ACTIVATED], 0,
                   gn_list_view_item_get_item (item));
  else
    gn_list_view_item_toggle_selection (item);
}

static void
gn_main_view_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GnMainView *self = (GnMainView *)object;

  switch (prop_id)
    {
    case PROP_SELECTION_MODE:
      g_value_set_boolean (value, self->selection_mode);
      break;

    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_main_view_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GnMainView *self = (GnMainView *)object;

  switch (prop_id)
    {
    case PROP_SELECTION_MODE:
      gn_main_view_set_selection_mode (self, g_value_get_boolean (value));
      break;

    case PROP_MODEL:
      gn_main_view_set_model (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_main_view_class_init (GnMainViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = gn_main_view_get_property;
  object_class->set_property = gn_main_view_set_property;

  g_type_ensure (GN_TYPE_GRID_VIEW);
  g_type_ensure (GN_TYPE_LIST_VIEW);

  properties[PROP_SELECTION_MODE] =
    g_param_spec_boolean ("selection-mode",
                          "Selection Mode",
                          "If mode is selection mode or not",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_MODEL] =
    g_param_spec_object ("model",
                         "Model",
                         "The GListModel for the view",
                         G_TYPE_LIST_MODEL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  /**
   * GnMainView::item-activated:
   * @self: a #GnMainView
   * @item: a #GnItem
   *
   * item-activated signal is emitted when the user activates
   * an item by click, tap, or by some key.
   */
  signals [ITEM_ACTIVATED] =
    g_signal_new ("item-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GN_TYPE_ITEM);
  g_signal_set_va_marshaller (signals [ITEM_ACTIVATED],
                              G_TYPE_FROM_CLASS (klass),
                              g_cclosure_marshal_VOID__OBJECTv);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/sadiqpk/notes/"
                                               "ui/gn-main-view.ui");

  gtk_widget_class_bind_template_child (widget_class, GnMainView, view_stack);
  gtk_widget_class_bind_template_child (widget_class, GnMainView, grid_view);
  gtk_widget_class_bind_template_child (widget_class, GnMainView, list_view);
  gtk_widget_class_bind_template_child (widget_class, GnMainView, empty_view);

  gtk_widget_class_bind_template_callback (widget_class, gn_main_view_grid_item_activated);
  gtk_widget_class_bind_template_callback (widget_class, gn_main_view_list_item_activated);
}

static void
gn_main_view_init (GnMainView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
gn_main_view_set_child_selection_mode (GnMainView       *self,
                                       GtkSelectionMode  selection_mode)
{
  if (selection_mode == GTK_SELECTION_NONE)
    {
      gn_grid_view_unselect_all (GTK_FLOW_BOX (self->grid_view));
      gn_list_view_unselect_all (GTK_LIST_BOX (self->list_view));
    }

  gtk_flow_box_set_selection_mode (GTK_FLOW_BOX (self->grid_view),
                                   selection_mode);
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (self->list_view),
                                   selection_mode);
}

GnMainView *
gn_main_view_new (void)
{
  return g_object_new (GN_TYPE_MAIN_VIEW,
                       NULL);
}

/**
 * gn_main_view_get_selection_mode:
 * @self: A #GnMainView
 *
 * Get if selection mode is active
 *
 * Returns: %TRUE if selection mode is active. %FALSE otherwise
 */
gboolean
gn_main_view_get_selection_mode (GnMainView *self)
{
  g_return_val_if_fail (GN_IS_MAIN_VIEW (self), FALSE);

  return self->selection_mode;
}

/**
 * gn_main_view_set_selection_mode:
 * @self: A #GnMainView
 * @selection_mode: a gboolean to set mode
 *
 * Set selection mode
 */
void
gn_main_view_set_selection_mode (GnMainView *self,
                                 gboolean    selection_mode)
{
  GN_ENTRY;

  g_return_if_fail (GN_IS_MAIN_VIEW (self));

  if (self->selection_mode == selection_mode)
    return;

  self->selection_mode = selection_mode;

  if (selection_mode)
    gn_main_view_set_child_selection_mode (self, GTK_SELECTION_MULTIPLE);
  else
    gn_main_view_set_child_selection_mode (self, GTK_SELECTION_NONE);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SELECTION_MODE]);

  GN_EXIT;
}

/**
 * gn_main_view_get_selected_items:
 * @self: A #GnMainView
 *
 * Get selected items
 *
 * Returns: (transfer container): a #GList of #GnProviderItem or NULL
 */
GList *
gn_main_view_get_selected_items (GnMainView *self)
{
  g_return_val_if_fail (GN_IS_MAIN_VIEW (self), NULL);

  if (!self->selection_mode)
    return NULL;

  if (self->current_view == GN_VIEW_TYPE_GRID)
    return gn_grid_view_get_selected_items (GN_GRID_VIEW (self->grid_view));
  else
    return gn_list_view_get_selected_items (GN_LIST_VIEW (self->list_view));
}

/**
 * gn_main_view_set_model:
 * @self: A #GnMainView
 * @model: a #GListModel
 *
 * The data model to be used for the view. The function
 * returns %FALSE doing nothing if the same @model is being used.
 *
 * Returns: %TRUE of model is set to @model. %FALSE otherwise
 */
gboolean
gn_main_view_set_model (GnMainView *self,
                        GListModel *model)
{
  g_return_val_if_fail (GN_IS_MAIN_VIEW (self), FALSE);

  if (g_set_object (&self->model, model))
    {
      gtk_flow_box_bind_model (GTK_FLOW_BOX (self->grid_view),
                               model,
                               gn_grid_view_item_new,
                               G_OBJECT (self), NULL);

      gtk_list_box_bind_model (GTK_LIST_BOX (self->list_view),
                               model,
                               gn_list_view_item_new,
                               G_OBJECT (self), NULL);

      g_signal_connect_object (self->model, "items-changed",
                               G_CALLBACK (gn_main_view_model_changed),
                               self, G_CONNECT_AFTER);

      gn_main_view_model_changed (model, 0, 0, 0, self);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);
      return TRUE;
    }

  return FALSE;
}

/**
 * gn_main_view_set_view:
 * @self: A #GnMainView
 * @view_type: a #GnViewType
 *
 * Set Current view type.
 */
void
gn_main_view_set_view (GnMainView *self,
                       GnViewType  view_type)
{
  g_return_if_fail (GN_IS_MAIN_VIEW (self));

  if (self->current_view == view_type)
    return;

  self->current_view = view_type;

  if (view_type == GN_VIEW_TYPE_GRID)
    gtk_stack_set_visible_child (GTK_STACK (self->view_stack),
                                 self->grid_view);
  else
    gtk_stack_set_visible_child (GTK_STACK (self->view_stack),
                                 self->list_view);
}
