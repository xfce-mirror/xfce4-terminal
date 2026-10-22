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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "terminal-keys.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#endif

/* Whether the physical key of @keyval is held down, according to the answer of
 * the X server (the bit array of XQueryKeymap). */
static gboolean
terminal_keys_key_down (GdkKeymap   *keymap,
                        const guchar *keys,
                        guint32      keyval)
{
  GdkKeymapKey *entries = NULL;
  gint n_entries = 0;
  gint j;
  gboolean down = FALSE;

  if (!gdk_keymap_get_entries_for_keyval (keymap, keyval, &entries, &n_entries)
      || entries == NULL)
    return FALSE;

  for (j = 0; j < n_entries; j++)
    if ((keys[entries[j].keycode >> 3] & (1 << (entries[j].keycode & 7))) != 0)
      {
        down = TRUE;
        break;
      }

  g_free (entries);

  return down;
}

/* Whether the keysym is one of the modifier keys themselves: a lone modifier
 * press is no accelerator, so there is nothing to restore for it. */
static gboolean
terminal_keys_keyval_is_modifier (guint32 keyval)
{
  switch (keyval)
    {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
    case GDK_KEY_Hyper_L:
    case GDK_KEY_Hyper_R:
    case GDK_KEY_Caps_Lock:
    case GDK_KEY_Shift_Lock:
    case GDK_KEY_Num_Lock:
    case GDK_KEY_ISO_Level3_Shift:
    case GDK_KEY_ISO_Level3_Latch:
    case GDK_KEY_ISO_Level3_Lock:
    case GDK_KEY_Mode_switch:
    /* with Ctrl+Shift as the layout switch the key press of shift itself
     * arrives as a group switch keysym, not as Shift_L/Shift_R */
    case GDK_KEY_ISO_Next_Group:
    case GDK_KEY_ISO_Prev_Group:
    case GDK_KEY_ISO_First_Group:
    case GDK_KEY_ISO_Last_Group:
    case GDK_KEY_ISO_Group_Latch:
    case GDK_KEY_ISO_Group_Lock:
      return TRUE;
    default:
      return FALSE;
    }
}

/**
 * terminal_keys_modifiers_held:
 *
 * The modifier keys that are held down right now.
 *
 * When the layout switch fires it eats one of the modifiers of its chord:
 * with Ctrl+Shift as the switch (grp:ctrl_shift_toggle) the control or the
 * shift is consumed, with Alt+Shift (grp:alt_shift_toggle) the alt or the
 * shift is. The key press that follows arrives without that modifier in the
 * state, and the accelerator that needs it never matches.
 *
 * The switch sits below GDK, but the X server still knows the real state of
 * the physical keys, so ask it directly. Wayland has no such query here, the
 * fallback keeps the old behaviour.
 *
 * Returns the modifiers that are held down: %GDK_SHIFT_MASK,
 * %GDK_CONTROL_MASK and %GDK_MOD1_MASK in any combination.
 */
guint
terminal_keys_modifiers_held (void)
{
#ifdef GDK_WINDOWING_X11
  GdkDisplay *display;
  Display *xdisplay;
  GdkKeymap *keymap;
  guchar keys[32];
  guint held = 0;

  display = gdk_display_get_default ();
  if (display == NULL || !GDK_IS_X11_DISPLAY (display))
    return 0;

  keymap = gdk_keymap_get_for_display (display);
  xdisplay = gdk_x11_display_get_xdisplay (display);
  if (keymap == NULL || xdisplay == NULL)
    return 0;

  XQueryKeymap (xdisplay, (gchar *) keys);

  if (terminal_keys_key_down (keymap, keys, GDK_KEY_Shift_L)
      || terminal_keys_key_down (keymap, keys, GDK_KEY_Shift_R))
    held |= GDK_SHIFT_MASK;

  if (terminal_keys_key_down (keymap, keys, GDK_KEY_Control_L)
      || terminal_keys_key_down (keymap, keys, GDK_KEY_Control_R))
    held |= GDK_CONTROL_MASK;

  if (terminal_keys_key_down (keymap, keys, GDK_KEY_Alt_L)
      || terminal_keys_key_down (keymap, keys, GDK_KEY_Alt_R))
    held |= GDK_MOD1_MASK;

  return held;
#else
  return 0;
#endif
}

/**
 * terminal_keys_swallowed_modifiers:
 * @event: a key press event
 * @mods_held: the modifiers that are held down, see
 *              terminal_keys_modifiers_held()
 *
 * The modifiers of @event that the layout switch swallowed: they are held
 * down but missing from the state of the event.
 *
 * Which one is swallowed depends on the order the user pressed the keys in:
 * with Ctrl+Shift as the switch the control is eaten when the shift comes
 * second and the shift when the control comes second, with Alt+Shift the alt
 * or the shift the same way.
 *
 * A lone modifier press is no accelerator, so there is nothing to restore for
 * it: the press of the switch itself arrives as a group switch keysym and must
 * not be touched.
 *
 * Returns the modifiers that were swallowed.
 */
guint
terminal_keys_swallowed_modifiers (GdkEventKey *event,
                                   guint        mods_held)
{
  g_return_val_if_fail (event != NULL, 0);

  if (terminal_keys_keyval_is_modifier (event->keyval))
    return 0;

  return mods_held & ~event->state;
}
