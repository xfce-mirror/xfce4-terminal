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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TERMINAL_APP_H__
#define __TERMINAL_APP_H__

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

#include <terminal/terminal-options.h>

G_BEGIN_DECLS

#define TERMINAL_ERROR (terminal_error_quark ())
GQuark terminal_error_quark (void) G_GNUC_CONST;

enum _TerminalError
{
  /* problem with the runtime linker */
  TERMINAL_ERROR_LINKER_FAILURE,
  /* different user id in service */
  TERMINAL_ERROR_USER_MISMATCH,
  /* different display in service */
  TERMINAL_ERROR_DISPLAY_MISMATCH,
  /* parsing the options failed */
  TERMINAL_ERROR_OPTIONS,
  /* general failure */
  TERMINAL_ERROR_FAILED,
};


#define TERMINAL_TYPE_APP         (terminal_app_get_type ())
#define TERMINAL_APP(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_APP, TerminalApp))
#define TERMINAL_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_APP, TerminalAppClass))
#define TERMINAL_IS_APP(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_APP))

typedef struct _TerminalAppClass TerminalAppClass;
typedef struct _TerminalApp      TerminalApp;
typedef enum   _TerminalError    TerminalError;

GType        terminal_app_get_type            (void) G_GNUC_CONST;

gboolean     terminal_app_process             (TerminalApp        *app,
                                               gchar             **argv,
                                               gint                argc,
                                               GError            **error);

G_END_DECLS

#endif /* !__TERMINAL_APP_H__ */
