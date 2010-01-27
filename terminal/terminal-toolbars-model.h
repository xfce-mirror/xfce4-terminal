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
 *
 * The geometry handling code was taken from gnome-terminal. The geometry hacks
 * where initially written by Owen Taylor.
 */

#ifndef __TERMINAL_TOOLBARS_MODEL_H__
#define __TERMINAL_TOOLBARS_MODEL_H__

#include <exo/exo.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_TOOLBARS_MODEL            (terminal_toolbars_model_get_type ())
#define TERMINAL_TOOLBARS_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_TOOLBARS_MODEL, TerminalToolbarsModel))
#define TERMINAL_TOOLBARS_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_TOOLBARS_MODEL, TerminalToolbarsModelClass))
#define TERMINAL_IS_TOOLBARS_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_TOOLBARS_MODEL))
#define TERMINAL_IS_TOOLBARS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_TOOLBARS_MODEL))
#define TERMINAL_TOOLBARS_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_TOOLBARS_MODEL, TerminalToolbarsModelClass))

typedef struct _TerminalToolbarsModelClass TerminalToolbarsModelClass;
typedef struct _TerminalToolbarsModel      TerminalToolbarsModel;

struct _TerminalToolbarsModelClass
{
  ExoToolbarsModelClass __parent__;
};

struct _TerminalToolbarsModel
{
  ExoToolbarsModel __parent__;

  /*< private >*/
  guint            sync_id;
};


GType             terminal_toolbars_model_get_type    (void) G_GNUC_CONST;

ExoToolbarsModel *terminal_toolbars_model_get_default (void);

G_END_DECLS

#endif /* !__TERMINAL_TOOLBARS_MODEL_H__ */
