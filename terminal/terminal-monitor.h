/* $Id: terminal-window.h 99 2004-12-21 19:40:47Z bmeurer $ */
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
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TERMINAL_MONITOR_H__
#define __TERMINAL_MONITOR_H__

#include <glib-object.h>

G_BEGIN_DECLS;

#define TERMINAL_TYPE_MONITOR             (terminal_monitor_get_type ())
#define TERMINAL_MONITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_MONITOR, TerminalMonitor))
#define TERMINAL_MONITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_MONITOR, TerminalMonitorClass))
#define TERMINAL_IS_MONITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_MONITOR))
#define TERMINAL_IS_MONITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_MONITOR))
#define TERMINAL_MONITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_MONITOR, TerminalMonitorClass))

typedef struct _TerminalMonitorClass TerminalMonitorClass;
typedef struct _TerminalMonitor      TerminalMonitor;

GType            terminal_monitor_get_type          (void) G_GNUC_CONST;

TerminalMonitor *terminal_monitor_get               (void);

void             terminal_monitor_add               (TerminalMonitor *monitor,
                                                     const gchar     *path,
                                                     GClosure        *closure);

void             terminal_monitor_remove_by_closure (TerminalMonitor *monitor,
                                                     GClosure        *closure);

G_END_DECLS;

#endif /* !__TERMINAL_MONITOR_H__ */
