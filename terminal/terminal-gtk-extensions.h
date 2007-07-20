/* $Id$ */
/*-
 * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
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

#ifndef __TERMINAL_GTK_EXTENSIONS_H__
#define __TERMINAL_GTK_EXTENSIONS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

void terminal_gtk_widget_set_tooltip (GtkWidget   *widget,
                                      const gchar *format,
                                      ...) G_GNUC_INTERNAL G_GNUC_PRINTF (2, 3);

G_END_DECLS;

#endif /* !__TERMINAL_GTK_EXTENSIONS_H__ */
