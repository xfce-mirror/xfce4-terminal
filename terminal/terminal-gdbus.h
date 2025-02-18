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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TERMINAL_GDBUS_H
#define TERMINAL_GDBUS_H

#include "terminal-app.h"

G_BEGIN_DECLS

gboolean  terminal_gdbus_register_service  (TerminalApp  *app,
                                            GError      **error);
gboolean  terminal_gdbus_invoke_launch     (gint          argc,
                                            gchar       **argv,
                                            GError      **error);

G_END_DECLS

#endif /* !TERMINAL_GDBUS_H */
