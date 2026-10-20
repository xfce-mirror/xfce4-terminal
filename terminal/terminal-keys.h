/*-
 * Copyright (c) 2026 als-creator
 *
 * Layout independent accelerators for xfce4-terminal.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __TERMINAL_KEYS_H__
#define __TERMINAL_KEYS_H__

#include <gdk/gdk.h>
#include <glib.h>

G_BEGIN_DECLS

gboolean
terminal_keys_normalize_layout_accel (GdkEventKey *event,
                                      guint32 *keyval,
                                      guint *mods);

gboolean
terminal_keys_accel_matches (const gchar *accel,
                             guint32 keyval,
                             guint mods);

G_END_DECLS

#endif /* __TERMINAL_KEYS_H__ */
