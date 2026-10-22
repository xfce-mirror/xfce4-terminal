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

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "terminal-keys.h"

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

/* The modifiers that the layout switch can eat. */
#define TERMINAL_KEYS_MODIFIERS (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)

/* The modifier keys that are held down, as far as we know.
 *
 * The layout switch eats one of the modifiers of its chord, so the state of
 * the events does not say everything that is true: the key is down, but its
 * bit is missing from the state. The keys are tracked here instead, so that
 * the state of an event can be completed without asking the X server on every
 * key press.
 */
static guint terminal_keys_held = 0;

/* Whether the physical key of @keyval is held down, according to the answer of
 * the X server (the bit array of XQueryKeymap). */
static gboolean
terminal_keys_key_down (GdkKeymap *keymap,
                        const guchar *keys,
                        guint32 keyval)
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

/* The modifier keys that are held down right now, according to the X server.
 *
 * The layout switch sits below GDK, but the X server still knows the real
 * state of the physical keys, so ask it directly.
 *
 * Returns the modifiers that are held down: %GDK_SHIFT_MASK,
 * %GDK_CONTROL_MASK and %GDK_MOD1_MASK in any combination.
 */
static guint
terminal_keys_modifiers_held (void)
{
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
}

/* The modifier of a modifier key, or 0 for a key that is no modifier. */
static guint
terminal_keys_keyval_modifier (guint32 keyval)
{
  switch (keyval)
    {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
      return GDK_SHIFT_MASK;
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
      return GDK_CONTROL_MASK;
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
      return GDK_MOD1_MASK;
    default:
      return 0;
    }
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
    case GDK_KEY_ISO_Next_Group_Lock:
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

/* Whether the key press is the one where the layout switch fired: XKB reports
 * the switch with a group switch keysym, and the modifier it ate comes with
 * the same keysym. */
static gboolean
terminal_keys_keyval_is_group_switch (guint32 keyval)
{
  switch (keyval)
    {
    case GDK_KEY_ISO_Next_Group:
    case GDK_KEY_ISO_Next_Group_Lock:
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
 * terminal_keys_key_press:
 * @event: a key press event
 *
 * Put back the modifier that the layout switch ate and keep track of the keys
 * that are down.
 *
 * When the layout switch fires it eats one of the modifiers of its chord:
 * with Ctrl+Shift as the switch (grp:ctrl_shift_toggle) the control or the
 * shift is consumed, with Alt+Shift (grp:alt_shift_toggle) the alt or the
 * shift is. The key press that follows arrives without that modifier in the
 * state, and the accelerator that needs it never matches.
 *
 * Which one is eaten depends on the order the user pressed the keys in, so
 * the eaten modifier is not known here: the keys that are down are tracked
 * instead, and the modifier that is missing from the state of the event is
 * put back.
 */
void
terminal_keys_key_press (GdkEventKey *event)
{
  guint modifier;
  guint swallowed;

  g_return_if_fail (event != NULL);

  if (terminal_keys_keyval_is_group_switch (event->keyval))
    {
      /* The switch eats the modifier of the key that was pressed last, so the
       * event does not say which one it is: ask the server. */
      terminal_keys_held = terminal_keys_modifiers_held ();
    }
  else
    {
      modifier = terminal_keys_keyval_modifier (event->keyval);
      if (modifier != 0)
        {
          terminal_keys_held |= modifier;
        }
      else if ((event->state & TERMINAL_KEYS_MODIFIERS) & ~terminal_keys_held)
        {
          /* The event carries a modifier that the tracked keys do not have:
           * it was pressed before the window got the focus. Ask the server. */
          terminal_keys_held = terminal_keys_modifiers_held ();
        }
    }

  swallowed = terminal_keys_held & ~event->state & TERMINAL_KEYS_MODIFIERS;

  /* A lone modifier press is no accelerator, so there is nothing to restore
   * for it: the press of the switch itself arrives as a group switch keysym
   * and must not be touched. */
  if (!terminal_keys_keyval_is_modifier (event->keyval))
    event->state |= swallowed;
}

/**
 * terminal_keys_key_release:
 * @event: a key release event
 *
 * Keep track of the keys that are up again.
 */
void
terminal_keys_key_release (GdkEventKey *event)
{
  guint modifier;

  g_return_if_fail (event != NULL);

  if (terminal_keys_keyval_is_group_switch (event->keyval))
    {
      /* The key of the switch is released and the event does not say which
       * modifier it was: ask the server. */
      terminal_keys_held = terminal_keys_modifiers_held ();
      return;
    }

  modifier = terminal_keys_keyval_modifier (event->keyval);
  if (modifier != 0)
    terminal_keys_held &= ~modifier;
}

/**
 * terminal_keys_focus_in:
 *
 * Ask the server which modifier keys are down: they may have been pressed
 * while another window had the focus.
 */
void
terminal_keys_focus_in (void)
{
  terminal_keys_held = terminal_keys_modifiers_held ();
}

/**
 * terminal_keys_focus_out:
 *
 * Forget the modifier keys: they may be released while another window has the
 * focus, and the events would not say so.
 */
void
terminal_keys_focus_out (void)
{
  terminal_keys_held = 0;
}

#else /* !GDK_WINDOWING_X11 */

/* On Wayland the compositor tells the state of the modifiers in the events and
 * the layout switch does not eat any of them: there is nothing to put back. */

void
terminal_keys_key_press (GdkEventKey *event)
{
}

void
terminal_keys_key_release (GdkEventKey *event)
{
}

void
terminal_keys_focus_in (void)
{
}

void
terminal_keys_focus_out (void)
{
}

#endif /* !GDK_WINDOWING_X11 */
