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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TERMINAL_SCREEN_H
#define TERMINAL_SCREEN_H

#include <gtk/gtk.h>
#include <terminal/terminal-private.h>
#include <terminal/terminal-options.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_SCREEN            (terminal_screen_get_type ())
#define TERMINAL_SCREEN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_SCREEN, TerminalScreen))
#define TERMINAL_SCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_SCREEN, TerminalScreenClass))
#define TERMINAL_IS_SCREEN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_SCREEN))
#define TERMINAL_IS_SCREEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_SCREEN))
#define TERMINAL_SCREEN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_SCREEN, TerminalScreenClass))

typedef struct _TerminalScreenClass TerminalScreenClass;
typedef struct _TerminalScreen      TerminalScreen;

GType           terminal_screen_get_type                  (void) G_GNUC_CONST;

TerminalScreen *terminal_screen_new                       (TerminalTabAttr *attr,
                                                           glong            columns,
                                                           glong            rows);

void            terminal_screen_launch_child              (TerminalScreen *screen);

const gchar    *terminal_screen_get_custom_title          (TerminalScreen *screen);
void            terminal_screen_set_custom_title          (TerminalScreen *screen,
                                                           const gchar    *title);

void            terminal_screen_get_size                  (TerminalScreen *screen,
                                                           glong          *width_chars,
                                                           glong          *height_chars);
void            terminal_screen_set_size                  (TerminalScreen *screen,
                                                           glong           width_chars,
                                                           glong           height_chars);

void            terminal_screen_get_geometry              (TerminalScreen *screen,
                                                           glong          *char_width,
                                                           glong          *char_height,
                                                           gint           *xpad,
                                                           gint           *ypad);

void            terminal_screen_set_window_geometry_hints (TerminalScreen *screen,
                                                           GtkWindow      *window);

void            terminal_screen_force_resize_window       (TerminalScreen *screen,
                                                           GtkWindow      *window,
                                                           glong           force_columns,
                                                           glong           force_rows);

gchar          *terminal_screen_get_title                 (TerminalScreen *screen);

const gchar    *terminal_screen_get_working_directory     (TerminalScreen *screen);
void            terminal_screen_set_working_directory     (TerminalScreen *screen,
                                                           const gchar    *directory);

gboolean        terminal_screen_has_selection             (TerminalScreen *screen);

void            terminal_screen_copy_clipboard            (TerminalScreen *screen);
void            terminal_screen_copy_clipboard_html       (TerminalScreen *screen);
void            terminal_screen_paste_clipboard           (TerminalScreen *screen);
void            terminal_screen_paste_primary             (TerminalScreen *screen);

void            terminal_screen_select_all                (TerminalScreen *screen);

void            terminal_screen_reset                     (TerminalScreen *screen,
                                                           gboolean        clear);

GSList         *terminal_screen_get_restart_command       (TerminalScreen *screen);

void            terminal_screen_reset_activity            (TerminalScreen *screen);

GtkWidget      *terminal_screen_get_tab_label             (TerminalScreen *screen);

void            terminal_screen_focus                     (TerminalScreen *screen);

const gchar    *terminal_screen_get_encoding              (TerminalScreen *screen);
void            terminal_screen_set_encoding              (TerminalScreen *screen,
                                                           const gchar    *charset);

void            terminal_screen_search_set_gregex         (TerminalScreen *screen,
                                                           VteRegex       *regex,
                                                           gboolean        wrap_around);
gboolean        terminal_screen_search_has_gregex         (TerminalScreen *screen);

void            terminal_screen_search_find_next          (TerminalScreen *screen);
void            terminal_screen_search_find_previous      (TerminalScreen *screen);

void            terminal_screen_update_scrolling_bar      (TerminalScreen *screen);

void            terminal_screen_update_font               (TerminalScreen *screen);

gboolean        terminal_screen_get_input_enabled         (TerminalScreen *screen);
void            terminal_screen_set_input_enabled         (TerminalScreen *screen,
                                                           gboolean        enabled);

gboolean        terminal_screen_get_scroll_on_output      (TerminalScreen *screen);
void            terminal_screen_set_scroll_on_output      (TerminalScreen *screen,
                                                           gboolean        enabled);

void            terminal_screen_save_contents             (TerminalScreen *screen,
                                                           GOutputStream  *stream,
                                                           GError         *error);

gboolean        terminal_screen_has_foreground_process    (TerminalScreen *screen);

void            terminal_screen_feed_text                 (TerminalScreen *screen,
                                                           const char     *text);

const gchar    *terminal_screen_get_custom_fg_color       (TerminalScreen *screen);

const gchar    *terminal_screen_get_custom_bg_color       (TerminalScreen *screen);

const gchar    *terminal_screen_get_custom_title_color    (TerminalScreen *screen);
void            terminal_screen_set_custom_title_color    (TerminalScreen *screen,
                                                           const gchar    *color);
void            terminal_screen_send_signal               (TerminalScreen *screen,
                                                           int             signum);
void            terminal_screen_widget_append_accels      (TerminalScreen *screen,
                                                           GtkAccelGroup  *accel_group);

G_END_DECLS

#endif /* !TERMINAL_SCREEN_H */
