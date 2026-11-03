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

#include <gdk/gdk.h>
#include <glib.h>

G_BEGIN_DECLS

/* Put back the modifier that the layout switch ate out of the state of the
 * key press, and keep track of the keys that are down. */
void
terminal_keys_key_press (GdkEventKey *event);

/* Keep track of the keys that are up again. */
void
terminal_keys_key_release (GdkEventKey *event);

/* The modifier keys may have been pressed while another window had the
 * focus: ask for them when the focus comes in. */
void
terminal_keys_focus_in (void);

/* The modifier keys may be released while another window has the focus:
 * forget them when the focus goes out. */
void
terminal_keys_focus_out (void);

G_END_DECLS

#endif /* __TERMINAL_KEYS_H__ */
