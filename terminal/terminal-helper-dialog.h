/* $Id$ */
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
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifndef __TERMINAL_HELPER_CHOOSER_H__
#define __TERMINAL_HELPER_CHOOSER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

#define TERMINAL_TYPE_HELPER_CHOOSER            (terminal_helper_chooser_get_type ())
#define TERMINAL_HELPER_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_HELPER_CHOOSER, TerminalHelperChooser))
#define TERMINAL_HELPER_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_HELPER_CHOOSER, TerminalHelperChooserClass))
#define TERMINAL_IS_HELPER_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_HELPER_CHOOSER))
#define TERMINAL_IS_HELPER_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_HELPER_CHOOSER))
#define TERMINAL_HELPER_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_HELPER_CHOOSER, TerminalHelperChooserClass))

typedef struct _TerminalHelperChooserClass TerminalHelperChooserClass;
typedef struct _TerminalHelperChooser      TerminalHelperChooser;

GType        terminal_helper_chooser_get_type           (void) G_GNUC_CONST;
const gchar *terminal_helper_chooser_get_active         (TerminalHelperChooser  *chooser);
void         terminal_helper_chooser_set_active         (TerminalHelperChooser  *chooser,
                                                         const gchar            *active);
GSList      *terminal_helper_chooser_get_helpers        (TerminalHelperChooser  *chooser);
void         terminal_helper_chooser_set_helpers        (TerminalHelperChooser  *chooser,
                                                         GSList                 *helpers);
const gchar *terminal_helper_chooser_get_browse_message (TerminalHelperChooser  *chooser);
void         terminal_helper_chooser_set_browse_message (TerminalHelperChooser  *chooser,
                                                         const gchar            *message);
const gchar *terminal_helper_chooser_get_browse_stock   (TerminalHelperChooser  *chooser);
void         terminal_helper_chooser_set_browse_stock   (TerminalHelperChooser  *chooser,
                                                         const gchar            *stock);
const gchar *terminal_helper_chooser_get_browse_title   (TerminalHelperChooser  *chooser);
void         terminal_helper_chooser_set_browse_title   (TerminalHelperChooser  *chooser,
                                                         const gchar            *title);


#define TERMINAL_TYPE_HELPER_DIALOG             (terminal_helper_dialog_get_type ())
#define TERMINAL_HELPER_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_HELPER_DIALOG, TerminalHelperDialog))
#define TERMINAL_HELPER_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_HELPER_DIALOG, TerminalHelperDialogClass))
#define TERMINAL_IS_HELPER_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_HELPER_DIALOG))
#define TERMINAL_IS_HELPER_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_HELPER_DIALOG))
#define TERMINAL_HELPER_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_HELPER_DIALOG, TerminalHelperDialogClass))

typedef struct _TerminalHelperDialogClass TerminalHelperDialogClass;
typedef struct _TerminalHelperDialog      TerminalHelperDialog;

GType      terminal_helper_dialog_get_type  (void) G_GNUC_CONST;
GtkWidget *terminal_helper_dialog_new       (void);

G_END_DECLS;

#endif /* !__TERMINAL_HELPER_CHOOSER_H__ */

