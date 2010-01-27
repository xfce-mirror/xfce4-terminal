/*-
 * Copyright (c) 2004-2005 os-cillation e.K.
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

#ifndef __TERMINAL_TOOLBARS_VIEW_H__
#define __TERMINAL_TOOLBARS_VIEW_H__

#include <exo/exo.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_TOOLBARS_VIEW             (terminal_toolbars_view_get_type ())
#define TERMINAL_TOOLBARS_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_TOOLBARS_VIEW, TerminalToolbarsView))
#define TERMINAL_TOOLBARS_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_TOOLBARS_VIEW, TerminalToolbarsViewClass))
#define TERMINAL_IS_TOOLBARS_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_TOOLBARS_VIEW))
#define TERMINAL_IS_TOOLBARS_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_TOOLBARS_VIEW))
#define TERMINAL_TOOLBARS_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_TOOLBARS_VIEW, TerminalToolbarsViewClass))

typedef struct _TerminalToolbarsViewClass TerminalToolbarsViewClass;
typedef struct _TerminalToolbarsView      TerminalToolbarsView;

struct _TerminalToolbarsViewClass
{
  ExoToolbarsViewClass __parent__;
};

struct _TerminalToolbarsView
{
  ExoToolbarsView __parent__;
  GtkWidget      *editor_dialog;
};


GType      terminal_toolbars_view_get_type  (void) G_GNUC_CONST;

void       terminal_toolbars_view_edit      (TerminalToolbarsView *toolbar);

G_END_DECLS

#endif /* !__TERMINAL_TOOLBAR_H__ */
