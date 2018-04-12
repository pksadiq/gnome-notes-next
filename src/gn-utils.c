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
