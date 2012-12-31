/*-
 * Copyright (c) 2004-2007 os-cillation e.K.
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

#ifndef __TERMINAL_PREFERENCES_DIALOG_H__
#define __TERMINAL_PREFERENCES_DIALOG_H__

#include <terminal/terminal-preferences.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_PREFERENCES_DIALOG            (terminal_preferences_dialog_get_type ())
#define TERMINAL_PREFERENCES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_PREFERENCES_DIALOG, TerminalPreferencesDialog))
#define TERMINAL_PREFERENCES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_PREFERENCES_DIALOG, TerminalPreferencesDialogClass))
#define TERMINAL_IS_PREFERENCES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_PREFERENCES_DIALOG))
#define TERMINAL_IS_PREFERENCES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), TERMINAL_TYPE_PREFERENCES_DIALOG))
#define TERMINAL_PREFERENCES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_PREFERENCES_DIALOG, TerminalPreferencesDialogClass))

typedef struct _TerminalPreferencesDialogClass TerminalPreferencesDialogClass;
typedef struct _TerminalPreferencesDialog      TerminalPreferencesDialog;

GType      terminal_preferences_dialog_get_type (void) G_GNUC_CONST;

GtkWidget *terminal_preferences_dialog_new      (gboolean show_drop_down);

G_END_DECLS

#endif /* !__TERMINAL_PREFERENCES_DIALOG_H__ */
