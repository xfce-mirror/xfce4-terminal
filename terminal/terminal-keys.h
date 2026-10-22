/*-
 * Copyright (c) 2026 als-creator
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* The modifiers that the layout group switch swallows. */

#ifndef __TERMINAL_KEYS_H__
#define __TERMINAL_KEYS_H__

#include <glib.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

/* The modifier keys that are held down right now: the layout switch may have
 * eaten one of them out of the state of the event. The answer comes from the
 * X server; the bits are %GDK_SHIFT_MASK, %GDK_CONTROL_MASK and
 * %GDK_MOD1_MASK. */
guint terminal_keys_modifiers_held (void);

/* The modifiers of a key press that the layout switch swallowed: they are
 * held down but missing from the state of the event. */
guint terminal_keys_swallowed_modifiers (GdkEventKey *event,
                                         guint        mods_held);

G_END_DECLS

#endif /* __TERMINAL_KEYS_H__ */
