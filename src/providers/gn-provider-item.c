/* gn-provider-item.c
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

#define G_LOG_DOMAIN "gn-provider-item"

#include "config.h"

#include "gn-item.h"
#include "gn-provider.h"
#include "gn-provider-item.h"
#include "gn-trace.h"

/**
 * SECTION: gn-provider-item
 * @title: GnProviderItem
 * @short_description:
 * @include: "gn-provider-item.h"
 */

struct _GnProviderItem
{
  GObject parent_instance;

  /* unowned */
  GnProvider *provider;
  GnItem *item;
};

G_DEFINE_TYPE (GnProviderItem, gn_provider_item, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_ITEM,
  PROP_PROVIDER,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
gn_provider_item_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GnProviderItem *self = (GnProviderItem *)object;

  switch (prop_id)
    {
    case PROP_ITEM:
      g_value_set_object (value, self->item);
      break;

    case PROP_PROVIDER:
      g_value_set_object (value, self->provider);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_provider_item_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GnProviderItem *self = (GnProviderItem *)object;

  switch (prop_id)
    {
    case PROP_ITEM:
      self->item = g_value_get_object (value);
      break;

    case PROP_PROVIDER:
      self->provider = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gn_provider_item_class_init (GnProviderItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = gn_provider_item_get_property;
  object_class->set_property = gn_provider_item_set_property;

  properties[PROP_ITEM] =
    g_param_spec_object ("item",
                         "Item",
                         "A Note/Notebook item",
                         GN_TYPE_ITEM,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_PROVIDER] =
    g_param_spec_object ("provider",
                         "Provider",
                         "A #GnProvider",
                         GN_TYPE_PROVIDER,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gn_provider_item_init (GnProviderItem *self)
{
}

GnProviderItem *
gn_provider_item_new (GnProvider *provider,
                      GnItem     *item)
{
  g_return_val_if_fail (GN_IS_ITEM (item), NULL);
  g_return_val_if_fail (GN_IS_PROVIDER (provider), NULL);

  return g_object_new (GN_TYPE_PROVIDER_ITEM,
                       "item", item,
                       "provider", provider,
                       NULL);
}

GnItem *
gn_provider_item_get_item (GnProviderItem *self)
{
  g_return_val_if_fail (GN_IS_PROVIDER_ITEM (self), NULL);

  return self->item;
}

GnProvider *
gn_provider_item_get_provider (GnProviderItem *self)
{
  g_return_val_if_fail (GN_IS_PROVIDER_ITEM (self), NULL);

  return self->provider;
}

/**
 * gn_provider_item_compare:
 * @a: A #GnProviderItem
 * @b: A #GnProviderItem
 *
 * Compare two #GnProviderItem
 *
 * Returns: an integer less than, equal to, or greater
 * than 0, if title of @a is <, == or > than title of @b.
 */
gint
gn_provider_item_compare (gconstpointer a,
                          gconstpointer b,
                          gpointer      user_data)
{
  const GnProviderItem *item_a = a;
  const GnProviderItem *item_b = b;

  if (item_a == item_b)
    return 0;

  return g_strcmp0 (gn_item_get_title (item_a->item),
                    gn_item_get_title (item_b->item));
}
