/*-
 * Copyright 2012 Nick Schermer <nick@xfce.org>
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

#ifndef __TERMINAL_SEARCH_DIALOG_H__
#define __TERMINAL_SEARCH_DIALOG_H__

#include <vte/vte.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_SEARCH_DIALOG            (terminal_search_dialog_get_type ())
#define TERMINAL_SEARCH_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_SEARCH_DIALOG, TerminalSearchDialog))
#define TERMINAL_SEARCH_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_SEARCH_DIALOG, TerminalSearchDialogClass))
#define TERMINAL_IS_SEARCH_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_SEARCH_DIALOG))
#define TERMINAL_IS_SEARCH_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_SEARCH_DIALOG))
#define TERMINAL_SEARCH_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_SEARCH_DIALOG, TerminalSearchDialogClass))

typedef struct _TerminalSearchDialog      TerminalSearchDialog;
typedef struct _TerminalSearchDialogClass TerminalSearchDialogClass;

enum
{
  TERMINAL_RESPONSE_SEARCH_NEXT,
  TERMINAL_RESPONSE_SEARCH_PREV
};

GType      terminal_search_dialog_get_type        (void) G_GNUC_CONST;

GtkWidget *terminal_search_dialog_new             (GtkWindow             *parent);

gboolean   terminal_search_dialog_get_wrap_around (TerminalSearchDialog  *dialog);

GRegex    *terminal_search_dialog_get_regex       (TerminalSearchDialog  *dialog,
                                                   GError               **error);

void       terminal_search_dialog_present         (TerminalSearchDialog  *dialog);

G_END_DECLS

#endif /* !__TERMINAL_SEARCH_DIALOG_H__ */
