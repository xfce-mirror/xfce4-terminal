/* $Id: terminal-options.h,v 1.3 2004/09/17 10:16:45 bmeurer Exp $ */
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

#ifndef __TERMINAL_OPTIONS_H__
#define __TERMINAL_OPTIONS_H__

G_BEGIN_DECLS;

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

typedef enum /*< flags,prefix=TERMINAL_OPTIONS >*/
{
  TERMINAL_OPTIONS_MASK_COMMAND           = 1 << 0,
  TERMINAL_OPTIONS_MASK_TITLE             = 1 << 1,
  TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY = 1 << 2,
  TERMINAL_OPTIONS_MASK_DISPLAY           = 1 << 3,
  TERMINAL_OPTIONS_MASK_SCREEN            = 1 << 4,
  TERMINAL_OPTIONS_MASK_FLAGS             = 1 << 5,
  TERMINAL_OPTIONS_MASK_GEOMETRY          = 1 << 6,
} TerminalOptionsMask;

typedef enum /*< flags,prefix=TERMINAL_FLAGS >*/
{
  TERMINAL_FLAGS_OPENTAB        = 1 << 0,
  TERMINAL_FLAGS_COMPACTMODE    = 1 << 1,
  TERMINAL_FLAGS_FULLSCREEN     = 1 << 2,
  TERMINAL_FLAGS_DISABLESERVER  = 1 << 3,
  TERMINAL_FLAGS_SHOWVERSION    = 1 << 4,
  TERMINAL_FLAGS_SHOWHELP       = 1 << 5,
} TerminalFlags;

typedef struct _TerminalOptions
{
  TerminalOptionsMask   mask;
  gchar               **command;
  gint                  command_len;
  gchar                *title;
  gchar                *working_directory;
  TerminalFlags         flags;
  gchar                *geometry;
} TerminalOptions;

TerminalOptions *terminal_options_from_args     (gint             argc,
                                                 gchar          **argv,
                                                 GError         **error);

TerminalOptions *terminal_options_from_message  (DBusMessage     *message);

void             terminal_options_free          (TerminalOptions *options);

G_END_DECLS;

#endif /* !__TERMINAL_OPTIONS_H__ */

