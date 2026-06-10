/*-
 * Copyright (C) 2012 Nick Schermer <nick@xfce.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TERMINAL_WINDOW_DROPDOWN_DROPDOWN_H
#define TERMINAL_WINDOW_DROPDOWN_DROPDOWN_H

#include "terminal-options.h"
#include "terminal-screen.h"
#include "terminal-window.h"

G_BEGIN_DECLS

#define TERMINAL_TYPE_WINDOW_DROPDOWN (terminal_window_dropdown_get_type ())
G_DECLARE_FINAL_TYPE (TerminalWindowDropdown, terminal_window_dropdown, TERMINAL, WINDOW_DROPDOWN, TerminalWindow)

GtkWidget *
terminal_window_dropdown_new (const gchar *role,
                              const gchar *icon,
                              gboolean fullscreen,
                              TerminalVisibility menubar,
                              TerminalVisibility toolbar);

void
terminal_window_dropdown_toggle (TerminalWindowDropdown *dropdown,
                                 const gchar *startup_id,
                                 gboolean force_show);

void
terminal_window_dropdown_get_size (TerminalWindowDropdown *dropdown,
                                   TerminalScreen *screen,
                                   glong *grid_width,
                                   glong *grid_height);

void
terminal_window_dropdown_update_geometry (TerminalWindowDropdown *dropdown);
void
terminal_window_dropdown_ignore_next_focus_out_event (TerminalWindowDropdown *dropdown);

G_END_DECLS

#endif /* !TERMINAL_WINDOW_DROPDOWN_H */
