/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
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

#ifndef __TERMINAL_DIALOGS_H__
#define __TERMINAL_DIALOGS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

void terminal_util_show_about_dialog  (GtkWindow    *parent);

void terminal_util_set_style_thinkess (GtkWidget    *widget,
                                       gint          thinkness);

void terminal_util_activate_window    (GtkWindow    *window);

G_END_DECLS

#endif /* !__TERMINAL_DIALOGS_H__ */
