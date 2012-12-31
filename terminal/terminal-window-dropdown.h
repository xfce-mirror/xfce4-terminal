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

#ifndef __TERMINAL_WINDOW_DROPDOWN_DROPDOWN_H__
#define __TERMINAL_WINDOW_DROPDOWN_DROPDOWN_H__

#include <terminal/terminal-screen.h>
#include <terminal/terminal-options.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_WINDOW_DROPDOWN            (terminal_window_dropdown_get_type ())
#define TERMINAL_WINDOW_DROPDOWN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_WINDOW_DROPDOWN, TerminalWindowDropdown))
#define TERMINAL_WINDOW_DROPDOWN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW_DROPDOWN, TerminalWindowDropdownClass))
#define TERMINAL_IS_WINDOW_DROPDOWN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_WINDOW_DROPDOWN))
#define TERMINAL_IS_WINDOW_DROPDOWN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW_DROPDOWN))
#define TERMINAL_WINDOW_DROPDOWN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_WINDOW_DROPDOWN, TerminalWindowDropdownClass))

typedef struct _TerminalWindowDropdownClass TerminalWindowDropdownClass;
typedef struct _TerminalWindowDropdown      TerminalWindowDropdown;

GType           terminal_window_dropdown_get_type             (void) G_GNUC_CONST;

GtkWidget      *terminal_window_dropdown_new                  (const gchar            *role,
                                                               gboolean                fullscreen,
                                                               TerminalVisibility      menubar,
                                                               TerminalVisibility      toolbar);

void            terminal_window_dropdown_toggle               (TerminalWindowDropdown *dropdown,
                                                               const gchar            *startup_id,
                                                               gboolean                force_show);

void            terminal_window_dropdown_get_size             (TerminalWindowDropdown *dropdown,
                                                               TerminalScreen         *screen,
                                                               glong                  *grid_width,
                                                               glong                  *grid_height);

G_END_DECLS

#endif /* !__TERMINAL_WINDOW_DROPDOWN_H__ */
