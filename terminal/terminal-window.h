/*-
 * Copyright (c) 2004-2008 os-cillation e.K.
 *
 * Written by Benedikt Meurer <benny@xfce.org>.
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

#ifndef TERMINAL_WINDOW_H
#define TERMINAL_WINDOW_H

#include <terminal/terminal-screen.h>
#include <terminal/terminal-options.h>
#include <terminal/terminal-preferences.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_WINDOW            (terminal_window_get_type ())
#define TERMINAL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_WINDOW, TerminalWindow))
#define TERMINAL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW, TerminalWindowClass))
#define TERMINAL_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_WINDOW))
#define TERMINAL_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW))
#define TERMINAL_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_WINDOW, TerminalWindowClass))

typedef struct _TerminalWindowPrivate TerminalWindowPrivate;

typedef struct
{
  GtkWindowClass parent_class;
} TerminalWindowClass;

typedef struct
{
  GtkWindow              parent_instance;

  TerminalWindowPrivate *priv;

  /* if this is a TerminalWindowDropdown */
  guint                  drop_down : 1;

  GtkActionGroup        *action_group;

  GtkWidget             *vbox;
  GtkWidget             *menubar;
  GtkWidget             *toolbar;
  GtkWidget             *notebook;

  gchar                 *font;
  TerminalZoomLevel      zoom;

  GtkAction             *action_fullscreen;

  TerminalVisibility     scrollbar_visibility;
} TerminalWindow;

GType           terminal_window_get_type               (void) G_GNUC_CONST;

GtkWidget      *terminal_window_new                    (const gchar        *role,
                                                        gboolean            fullscreen,
                                                        TerminalVisibility  menubar,
                                                        TerminalVisibility  borders,
                                                        TerminalVisibility  toolbar);

void            terminal_window_add                    (TerminalWindow     *window,
                                                        TerminalScreen     *screen);

TerminalScreen *terminal_window_get_active             (TerminalWindow     *window);

void            terminal_window_notebook_show_tabs     (TerminalWindow     *window);

GSList         *terminal_window_get_restart_command    (TerminalWindow     *window);

void            terminal_window_set_grid_size          (TerminalWindow     *window,
                                                        glong               width,
                                                        glong               height);

gboolean        terminal_window_has_children           (TerminalWindow     *window);

GObject        *terminal_window_get_preferences        (TerminalWindow     *window);

GtkWidget      *terminal_window_get_preferences_dialog (TerminalWindow     *window);

G_END_DECLS

#endif /* !TERMINAL_WINDOW_H */
