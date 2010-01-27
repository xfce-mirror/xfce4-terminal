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

#ifndef __TERMINAL_SCREEN_H__
#define __TERMINAL_SCREEN_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_SCREEN            (terminal_screen_get_type ())
#define TERMINAL_SCREEN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_SCREEN, TerminalScreen))
#define TERMINAL_SCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_SCREEN, TerminalScreenClass))
#define TERMINAL_IS_SCREEN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_SCREEN))
#define TERMINAL_IS_SCREEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_SCREEN))
#define TERMINAL_SCREEN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_SCREEN, TerminalScreenClass))

typedef struct _TerminalScreenClass TerminalScreenClass;
typedef struct _TerminalScreen      TerminalScreen;

GType        terminal_screen_get_type                     (void) G_GNUC_CONST;

void         terminal_screen_launch_child                 (TerminalScreen *screen);

void         terminal_screen_set_custom_command           (TerminalScreen *screen,
                                                           gchar         **command);
void         terminal_screen_set_custom_title             (TerminalScreen *screen,
                                                           const gchar    *title);

void         terminal_screen_get_size                     (TerminalScreen *screen,
                                                           gint           *width_chars,
                                                           gint           *height_chars);
void         terminal_screen_set_size                     (TerminalScreen *screen,
                                                           gint            width_chars,
                                                           gint            height_chars);

void         terminal_screen_set_window_geometry_hints    (TerminalScreen *screen,
                                                           GtkWindow      *window);

void         terminal_screen_force_resize_window          (TerminalScreen *screen,
                                                           GtkWindow      *window,
                                                           gint            force_columns,
                                                           gint            force_rows);

gchar       *terminal_screen_get_title                    (TerminalScreen *screen);

const gchar *terminal_screen_get_working_directory        (TerminalScreen *screen);
void         terminal_screen_set_working_directory        (TerminalScreen *screen,
                                                           const gchar    *directory);

void         terminal_screen_set_hold                     (TerminalScreen *screen,
                                                           gboolean        hold);

gboolean     terminal_screen_has_selection                (TerminalScreen *screen);

void         terminal_screen_copy_clipboard               (TerminalScreen *screen);
void         terminal_screen_paste_clipboard              (TerminalScreen *screen);
void         terminal_screen_paste_primary                (TerminalScreen *screen);

void         terminal_screen_reset                        (TerminalScreen *screen,
                                                           gboolean        clear);

void         terminal_screen_im_append_menuitems          (TerminalScreen *screen,
                                                           GtkMenuShell   *menushell);

GSList      *terminal_screen_get_restart_command          (TerminalScreen *screen);

void         terminal_screen_reset_activity               (TerminalScreen *screen);

GtkWidget   *terminal_screen_get_tab_label                (TerminalScreen *screen);

void         terminal_screen_focus                        (TerminalScreen *screen);

G_END_DECLS

#endif /* !__TERMINAL_SCREEN_H__ */
