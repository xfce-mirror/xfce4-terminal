/*-
 * Copyright (C) 2012 Nick Schermer <nick@xfce.org>
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

#ifndef __TERMINAL_PREFERENCES_DROPDOWN_DROPDOWN_DIALOG_H__
#define __TERMINAL_PREFERENCES_DROPDOWN_DROPDOWN_DIALOG_H__

#include <terminal/terminal-preferences.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_PREFERENCES_DROPDOWN_DIALOG            (terminal_preferences_dropdown_dialog_get_type ())
#define TERMINAL_PREFERENCES_DROPDOWN_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_PREFERENCES_DROPDOWN_DIALOG, TerminalPreferencesDropdownDialog))
#define TERMINAL_PREFERENCES_DROPDOWN_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_PREFERENCES_DROPDOWN_DIALOG, TerminalPreferencesDropdownDialogClass))
#define TERMINAL_IS_PREFERENCES_DROPDOWN_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_PREFERENCES_DROPDOWN_DIALOG))
#define TERMINAL_IS_PREFERENCES_DROPDOWN_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), TERMINAL_TYPE_PREFERENCES_DROPDOWN_DIALOG))
#define TERMINAL_PREFERENCES_DROPDOWN_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_PREFERENCES_DROPDOWN_DIALOG, TerminalPreferencesDropdownDialogClass))

typedef struct _TerminalPreferencesDropdownDialogClass TerminalPreferencesDropdownDialogClass;
typedef struct _TerminalPreferencesDropdownDialog      TerminalPreferencesDropdownDialog;

GType      terminal_preferences_dropdown_dialog_get_type (void) G_GNUC_CONST;

GtkWidget *terminal_preferences_dropdown_dialog_new      (void);

G_END_DECLS

#endif /* !__TERMINAL_PREFERENCES_DROPDOWN_DROPDOWN_DIALOG_H__ */
