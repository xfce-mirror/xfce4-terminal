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
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifndef __TERMINAL_TAB_HEADER_H__
#define __TERMINAL_TAB_HEADER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define TERMINAL_TYPE_TAB_HEADER            (terminal_tab_header_get_type ())
#define TERMINAL_TAB_HEADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_TAB_HEADER, TerminalTabHeader))
#define TERMINAL_TAB_HEADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_TAB_HEADER, TerminalTabHeaderClass))
#define TERMINAL_IS_TAB_HEADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_TAB_HEADER))
#define TERMINAL_IS_TAB_HEADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_TAB_HEADER))
#define TERMINAL_TAB_HEADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_TAB_HEADER, TerminalTabHeaderClass))

typedef struct _TerminalTabHeaderClass TerminalTabHeaderClass;
typedef struct _TerminalTabHeader      TerminalTabHeader;

struct _TerminalTabHeaderClass
{
  GtkHBoxClass __parent__;

  /* signals */
  void (*close) (TerminalTabHeader *header);
};

GType      terminal_tab_header_get_type (void) G_GNUC_CONST;

GtkWidget *terminal_tab_header_new      (void);

G_END_DECLS;

#endif /* !__TERMINAL_TAB_HEADER_H__ */
