/* $Id: terminal-app.h,v 1.3 2004/09/17 10:16:45 bmeurer Exp $ */
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

#ifndef __TERMINAL_APP_H__
#define __TERMINAL_APP_H__

#include <exo/exo.h>
#include <terminal/terminal-options.h>

G_BEGIN_DECLS;

#define TERMINAL_APP_METHOD_LAUNCH "Launch"
#define TERMINAL_APP_INTERFACE     "com.os_cillation.Terminal"
#define TERMINAL_APP_SERVICE       "com.os_cillation.Terminal"
#define TERMINAL_APP_PATH          "/com/os_cillation/Terminal"

#define TERMINAL_TYPE_APP         (terminal_app_get_type ())
#define TERMINAL_APP(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_APP, TerminalApp))
#define TERMINAL_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_APP, TerminalAppClass))
#define TERMINAL_IS_APP(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_APP))

typedef struct _TerminalAppClass TerminalAppClass;
typedef struct _TerminalApp      TerminalApp;

struct _TerminalAppClass
{
  GObjectClass  __parent__;
};

GType        terminal_app_get_type            (void) G_GNUC_CONST;

TerminalApp *terminal_app_new                 (void);

gboolean     terminal_app_start_server        (TerminalApp      *app,
                                               GError          **error);

gboolean     terminal_app_launch_with_options (TerminalApp      *app,
                                               TerminalOptions  *options,
                                               GError          **error);

gboolean     terminal_app_try_existing        (TerminalOptions  *options);

G_END_DECLS;

#endif /* !__TERMINAL_APP_H__ */
