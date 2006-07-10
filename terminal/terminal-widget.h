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

#ifndef __TERMINAL_WIDGET_H__
#define __TERMINAL_WIDGET_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define TERMINAL_TYPE_WIDGET      (terminal_widget_get_type ())
#define TERMINAL_WIDGET(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_WIDGET, TerminalWidget))
#define TERMINAL_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WIDGET, TerminalWidgetClass))
#define TERMINAL_IS_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_WIDGET))
#define TERMINAL_IS_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_WIDGET))
#define TERMINAL_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_WIDGET, TerminalWidgetClass))

typedef struct _TerminalWidgetClass TerminalWidgetClass;
typedef struct _TerminalWidget      TerminalWidget;

struct _TerminalWidgetClass
{
  GtkHBoxClass __parent__;

  /* signals */
  void (*context_menu) (TerminalWidget *widget, GdkEvent *event);
  void (*selection_changed) (TerminalWidget *widget);
  void (*title_changed) (TerminalWidget *widget);
};

GType      terminal_widget_get_type             (void) G_GNUC_CONST;

GtkWidget *terminal_widget_new                  (void);

void       terminal_widget_launch_child         (TerminalWidget *widget);

void       terminal_widget_set_custom_command   (TerminalWidget *widget,
                                                 gchar         **command);

void       terminal_widget_get_size             (TerminalWidget *widget,
                                                 gint           *width_chars,
                                                 gint           *height_chars);
void       terminal_widget_set_size             (TerminalWidget *widget,
                                                 gint            width_chars,
                                                 gint            height_chars);

void       terminal_widget_force_resize_window        (TerminalWidget *widget,
                                                       GtkWindow      *window,
                                                       gint            force_columns,
                                                       gint            force_rows);
void       terminal_widget_set_window_geometry_hints  (TerminalWidget *widget,
                                                       GtkWindow      *window);

GtkWidget *terminal_widget_get_tab_box                (TerminalWidget *widget);

gchar     *terminal_widget_get_title                  (TerminalWidget *widget);

gboolean   terminal_widget_has_selection              (TerminalWidget *widget);

void       terminal_widget_copy_clipboard             (TerminalWidget *widget);
void       terminal_widget_paste_clipboard            (TerminalWidget *widget);

void       terminal_widget_reset                      (TerminalWidget *widget,
                                                       gboolean        clear);

void       terminal_widget_im_append_menuitems        (TerminalWidget *widget,
                                                       GtkMenuShell   *menushell);

G_END_DECLS;

#endif /* !__TERMINAL_WIDGET_H__ */
