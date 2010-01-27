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
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <terminal/terminal-private.h>
#include <terminal/terminal-toolbars-model.h>
#include <terminal/terminal-toolbars-view.h>



static void terminal_toolbars_view_finalize   (GObject                   *object);
static void terminal_toolbars_view_edit_done  (ExoToolbarsEditorDialog   *dialog,
                                               TerminalToolbarsView      *toolbar);



G_DEFINE_TYPE (TerminalToolbarsView, terminal_toolbars_view, EXO_TYPE_TOOLBARS_VIEW)



static void
terminal_toolbars_view_class_init (TerminalToolbarsViewClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_toolbars_view_finalize;
}



static void
terminal_toolbars_view_init (TerminalToolbarsView *toolbar)
{
  ExoToolbarsModel *model;

  model = terminal_toolbars_model_get_default ();
  exo_toolbars_view_set_model (EXO_TOOLBARS_VIEW (toolbar), model);
  g_object_unref (G_OBJECT (model));

  g_signal_connect (G_OBJECT (toolbar), "customize",
                    G_CALLBACK (terminal_toolbars_view_edit), NULL);
}



static void
terminal_toolbars_view_finalize (GObject *object)
{
  TerminalToolbarsView *toolbar = TERMINAL_TOOLBARS_VIEW (object);

  if (G_UNLIKELY (toolbar->editor_dialog != NULL))
    gtk_widget_destroy (toolbar->editor_dialog);

  (*G_OBJECT_CLASS (terminal_toolbars_view_parent_class)->finalize) (object);
}



static void
terminal_toolbars_view_edit_done (ExoToolbarsEditorDialog *dialog,
                                  TerminalToolbarsView    *toolbar)
{
  exo_toolbars_view_set_editing (EXO_TOOLBARS_VIEW (toolbar), FALSE);
}



/**
 * terminal_toolbars_view_edit:
 * @toolbar : A #TerminalToolbarsView.
 **/
void
terminal_toolbars_view_edit (TerminalToolbarsView *toolbar)
{
  ExoToolbarsModel  *model;
  GtkUIManager      *ui_manager;
  GtkWidget         *toplevel;

  terminal_return_if_fail (TERMINAL_IS_TOOLBARS_VIEW (toolbar));

  exo_toolbars_view_set_editing (EXO_TOOLBARS_VIEW (toolbar), TRUE);

  if (toolbar->editor_dialog == NULL)
    {
      model = exo_toolbars_view_get_model (EXO_TOOLBARS_VIEW (toolbar));
      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (toolbar));
      ui_manager = exo_toolbars_view_get_ui_manager (EXO_TOOLBARS_VIEW (toolbar));

      toolbar->editor_dialog = exo_toolbars_editor_dialog_new_with_model (ui_manager, model);
      gtk_window_set_destroy_with_parent (GTK_WINDOW (toolbar->editor_dialog), TRUE);
      gtk_window_set_title (GTK_WINDOW (toolbar->editor_dialog), _("Toolbar Editor"));
      gtk_window_set_transient_for (GTK_WINDOW (toolbar->editor_dialog), GTK_WINDOW (toplevel));
      g_signal_connect (G_OBJECT (toolbar->editor_dialog), "destroy",
                        G_CALLBACK (terminal_toolbars_view_edit_done), toolbar);
      g_object_add_weak_pointer (G_OBJECT (toolbar->editor_dialog),
                                 (gpointer) &toolbar->editor_dialog);
      gtk_widget_show (toolbar->editor_dialog);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (toolbar->editor_dialog));
    }
}



