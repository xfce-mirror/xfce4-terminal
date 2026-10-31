/*-
 * Copyright (c) 2026 als-creator
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
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* The line that puts a pane into tmux: one field in the preferences,
 * one line out.  Whatever the user wrote is used as it is, and an
 * empty field falls back to the default line.  It is kept apart from
 * the window so that it can be tested without a display. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include "terminal-tmux.h"

/* The default line, and what each part is for:
 *
 *   TERM=tmux-256color          programs inside tmux are told that 256 colours
 *                               are there
 *   tmux new-session            a new session
 *   -A                          attach to the session instead of making a
 *                               second one when it is already there
 *   -s main                     the name of that session
 *   \; set -g allow-passthrough on
 *                               programs may pass 24-bit colour through tmux
 *                               (neovim, imgcat, fzf with icons), instead of
 *                               having it rounded to the nearest of 256
 *   \; set -g window-size latest
 *                               the pane takes the size of the last-sized
 *                               client window, so resizing the terminal resizes
 *                               tmux and leaves no scrollbars behind
 *
 * "\\;" is the shell quoting of tmux's own command separator: the shell has to
 * hand both commands to tmux instead of running the second one itself.
 *
 * Returns a newly allocated string. */
gchar *
terminal_tmux_build_command (const gchar *command)
{
  if (command != NULL && *command != '\0')
    return g_strdup (command);

  return g_strdup (TERMINAL_TMUX_DEFAULT_COMMAND);
}
