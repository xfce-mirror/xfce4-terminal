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

#ifndef __TERMINAL_OPTIONS_H__
#define __TERMINAL_OPTIONS_H__

#include <glib.h>

G_BEGIN_DECLS;

typedef struct _TerminalOptions    TerminalOptions;
typedef struct _TerminalTabAttr    TerminalTabAttr;
typedef struct _TerminalWindowAttr TerminalWindowAttr;

typedef enum /*< enum,prefix=TERMINAL_VISIBILITY >*/
{
  TERMINAL_VISIBILITY_DEFAULT,
  TERMINAL_VISIBILITY_SHOW,
  TERMINAL_VISIBILITY_HIDE,
} TerminalVisibility;

struct _TerminalOptions
{
  gchar    *session_id;
  gboolean  show_help;
  gboolean  show_version;
  gboolean  disable_server;
};

struct _TerminalTabAttr
{
  gchar **command;
  gchar  *directory;
  gchar  *title;
};

struct _TerminalWindowAttr
{
  GList               *tabs;
  gchar               *display;
  gchar               *geometry;
  gchar               *role;
  gchar               *startup_id;
  TerminalVisibility   menubar;
  TerminalVisibility   borders;
  TerminalVisibility   toolbars;
};

gboolean  terminal_options_parse    (gint                 argc,
                                     gchar              **argv,
                                     GList              **attrs_return,
                                     TerminalOptions    **options_return,
                                     GError             **error);

void                terminal_options_free     (TerminalOptions     *options);

void                terminal_tab_attr_free    (TerminalTabAttr     *attr);

TerminalWindowAttr *terminal_window_attr_new  (void);
void                terminal_window_attr_free (TerminalWindowAttr  *attr);

G_END_DECLS;

#endif /* !__TERMINAL_OPTIONS_H__ */
