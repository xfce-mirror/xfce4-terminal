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

/* The line that puts a pane into tmux. */

#ifndef TERMINAL_TMUX_H
#define TERMINAL_TMUX_H

#include <glib.h>

G_BEGIN_DECLS

/* The line the pane gets when the setting is empty.  The same text is the
 * default value of the setting, so the field in the preferences shows what
 * will actually run. */
#define TERMINAL_TMUX_DEFAULT_COMMAND \
  "TERM=tmux-256color tmux new-session -A -s main" \
  " \\; set -g allow-passthrough on" \
  " \\; set -g window-size latest"

gchar *
terminal_tmux_build_command (const gchar *command);

G_END_DECLS

#endif /* TERMINAL_TMUX_H */
