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

#ifndef __TERMINAL_SHORTCUT_EDITOR_H__
#define __TERMINAL_SHORTCUT_EDITOR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_SHORTCUT_EDITOR             (terminal_shortcut_editor_get_type ())
#define TERMINAL_SHORTCUT_EDITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_SHORTCUT_EDITOR, TerminalShortcutEditor))
#define TERMINAL_SHORTCUT_EDITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_SHORTCUT_EDITOR, TerminalShortcutEditorClass))
#define TERMINAL_IS_SHORTCUT_EDITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_SHORTCUT_EDITOR))
#define TERMINAL_IS_SHORTCUT_EDITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((obj), TERMINAL_TYPE_SHORTCUT_EDITOR))
#define TERMINAL_SHORTCUT_EDITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_SHORTCUT_EDITOR, TerminalShortcutEditorClass))

typedef struct _TerminalShortcutEditorClass TerminalShortcutEditorClass;
typedef struct _TerminalShortcutEditor      TerminalShortcutEditor;

struct _TerminalShortcutEditorClass
{
  GtkTreeViewClass __parent__;
};

GType      terminal_shortcut_editor_get_type  (void) G_GNUC_CONST;

G_END_DECLS

#endif /* !__TERMINAL_SHORTCUT_EDITOR_H__ */
