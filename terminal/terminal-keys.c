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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "terminal-keys.h"

/**
 * terminal_keys_normalize_layout_accel:
 * @event: a key press event
 * @keyval: (out): the latin keysym of the physical key
 * @mods: (out): the modifiers of the key press
 *
 * Guake style layout normalization.
 *
 * Under a non-latin layout a key press such as Ctrl+Shift+O produces a cyrillic
 * keysym. The accelerators of the window are stored as latin keysyms, so they
 * never match and they look dead. This maps the key press back to the latin
 * symbol of the same physical key, exactly like Guake does it in
 * Keybindings.activate().
 *
 * Returns %TRUE if the key press could be normalized, %FALSE if it is not
 * layout dependent and should be handled by the accelerators as they are.
 */
gboolean
terminal_keys_normalize_layout_accel (GdkEventKey *event,
                                      guint32 *keyval,
                                      guint *mods)
{
  GdkKeymap *keymap;
  GdkKeymapKey *entries;
  guint *keyvals;
  gint n_entries;
  gint i;
  guint keycode;
  guint32 symbol;

  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (keyval != NULL, FALSE);
  g_return_val_if_fail (mods != NULL, FALSE);

  /* nothing to do for latin keys: the key press already carries the latin
   * symbol, and keys without a latin symbol (arrows, function keys, tab) do
   * not depend on the layout at all */
  if (event->keyval <= 0x7e)
    return FALSE;

  keymap = gdk_keymap_get_for_display (gdk_display_get_default ());
  if (keymap == NULL)
    return FALSE;

  /* X11 reports the real keycode, but the Wayland backend and synthetic events
   * (XTest, xdotool) may leave it unset; then find the key through the keysym
   * instead, which works on both backends because GDK answers keymap queries
   * through xkbcommon there */
  keycode = event->hardware_keycode;
  if (keycode == 0)
    {
      GdkKeymapKey *sym_entries;
      gint n_sym_entries;

      sym_entries = NULL;
      n_sym_entries = 0;

      if (gdk_keymap_get_entries_for_keyval (keymap, event->keyval, &sym_entries, &n_sym_entries)
          && sym_entries != NULL && n_sym_entries > 0)
        keycode = sym_entries[0].keycode;

      g_free (sym_entries);

      if (keycode == 0)
        return FALSE;
    }

  entries = NULL;
  keyvals = NULL;
  n_entries = 0;
  if (!gdk_keymap_get_entries_for_keycode (keymap, keycode, &entries, &keyvals, &n_entries))
    return FALSE;

  /* take the first latin symbol of the physical key */
  symbol = 0;
  for (i = 0; i < n_entries; i++)
    if (keyvals[i] > 0 && keyvals[i] <= 0x7e)
      {
        symbol = keyvals[i];
        break;
      }

  g_free (entries);
  g_free (keyvals);

  if (symbol == 0)
    return FALSE;

  /* the key press already carries the latin symbol of this physical key */
  if (symbol == event->keyval)
    return FALSE;

  /* with shift held the latin symbol is lower case, left tab stays a tab */
  if (event->state & GDK_SHIFT_MASK)
    {
      if (symbol == GDK_KEY_ISO_Left_Tab)
        symbol = GDK_KEY_Tab;
      else
        symbol = gdk_keyval_to_lower (symbol);
    }

  *keyval = symbol;
  *mods = event->state & gtk_accelerator_get_default_mod_mask ();

  return TRUE;
}



/**
 * terminal_keys_accel_matches:
 * @accel: an accelerator string, e.g. "<control><shift>o"
 * @keyval: a keysym
 * @mods: the modifiers of a key press
 *
 * Checks whether @keyval with @mods activates @accel.
 *
 * Returns %TRUE if the accelerator matches.
 */
gboolean
terminal_keys_accel_matches (const gchar *accel,
                             guint32 keyval,
                             guint mods)
{
  guint entry_key = 0;
  GdkModifierType entry_mods = 0;
  guint mask;

  if (accel == NULL || *accel == '\0')
    return FALSE;

  mask = gtk_accelerator_get_default_mod_mask ();

  /* gtk_accelerator_parse() returns void in newer gtk releases */
  gtk_accelerator_parse (accel, &entry_key, &entry_mods);

  return entry_key == keyval
         && entry_key != 0
         && (entry_mods & mask) == (mods & mask);
}
