/* $Id$ */
/*-
 * Copyright (c) 2004 os-cillation e.K.
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
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TERMINAL_WINDOW_H__
#define __TERMINAL_WINDOW_H__

#include <terminal/terminal-widget.h>

G_BEGIN_DECLS;

#define TERMINAL_TYPE_WINDOW            (terminal_window_get_type ())
#define TERMINAL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_WINDOW, TerminalWindow))
#define TERMINAL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW, TerminalWindowClass))
#define TERMINAL_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_WINDOW))
#define TERMINAL_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW))
#define TERMINAL_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_WINDOW, TerminalWindowClass))

typedef struct _TerminalWindowClass TerminalWindowClass;
typedef struct _TerminalWindow      TerminalWindow;

struct _TerminalWindowClass
{
  GtkWindowClass __parent__;

  /* signals */
  void (*new_window) (TerminalWindow *window,
                      const gchar    *working_directory);
};

GType      terminal_window_get_type             (void) G_GNUC_CONST;

GtkWidget *terminal_window_new                  (TerminalVisibility  menubar,
                                                 TerminalVisibility  borders,
                                                 TerminalVisibility  toolbars);

void       terminal_window_add                  (TerminalWindow     *window,
                                                 TerminalWidget     *widget);

void       terminal_window_remove               (TerminalWindow     *window,
                                                 TerminalWidget     *widget);

void       terminal_window_set_startup_id       (TerminalWindow     *window,
                                                 const gchar        *startup_id);

GList     *terminal_window_get_restart_command  (TerminalWindow     *window);

G_END_DECLS;

#endif /* !__TERMINAL_WINDOW_H__ */
