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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TERMINAL_ENCODING_ACTION_H__
#define __TERMINAL_ENCODING_ACTION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _TerminalEncodingActionClass TerminalEncodingActionClass;
typedef struct _TerminalEncodingAction      TerminalEncodingAction;

#define TERMINAL_TYPE_ENCODING_ACTION            (terminal_encoding_action_get_type ())
#define TERMINAL_ENCODING_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_ENCODING_ACTION, TerminalEncodingAction))
#define TERMINAL_ENCODING_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_ENCODING_ACTION, TerminalEncodingActionClass))
#define TERMINAL_IS_ENCODING_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_ENCODING_ACTION))
#define TERMINAL_IS_ENCODING_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_ENCODING_ACTION))
#define TERMINAL_ENCODING_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_ENCODING_ACTION, TerminalEncodingActionClass))

GType         terminal_encoding_action_get_type    (void) G_GNUC_CONST;

GtkAction    *terminal_encoding_action_new         (const gchar *name,
                                                    const gchar *label) G_GNUC_MALLOC;

void          terminal_encoding_action_set_charset (GtkAction   *gtkaction,
                                                    const gchar *charset);

enum
{
  ENCODING_COLUMN_TITLE,
  ENCODING_COLUMN_IS_CHARSET,
  ENCODING_COLUMN_VALUE,
  N_ENCODING_COLUMNS
};

GtkTreeModel *terminal_encoding_model_new          (const gchar *current,
                                                    GtkTreeIter *current_iter);

G_END_DECLS

#endif /* !__TERMINAL_ENCODING_ACTION_H__ */
