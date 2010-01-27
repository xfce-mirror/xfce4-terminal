/*-
 * Copyright (c) 2004-2006 os-cillation e.K.
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
 *
 * The geometry handling code was taken from gnome-terminal. The geometry hacks
 * where initially written by Owen Taylor.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <terminal/terminal-toolbars-model.h>



static void     terminal_toolbars_model_finalize      (GObject               *object);
static void     terminal_toolbars_model_queue_sync    (TerminalToolbarsModel *model);
static gboolean terminal_toolbars_model_sync          (gpointer               user_data);
static void     terminal_toolbars_model_sync_destroy  (gpointer               user_data);



static const gchar *actions[] =
{
  "close-tab",
  "close-window",
  "new-tab",
  "new-window",
  "copy",
  "paste",
  "preferences",
  "show-menubar",
  "show-borders",
  "fullscreen",
  "prev-tab",
  "next-tab",
  "reset",
  "reset-and-clear",
  "contents",
  "report-bug",
  "about",
};



G_DEFINE_TYPE (TerminalToolbarsModel, terminal_toolbars_model, EXO_TYPE_TOOLBARS_MODEL)



static void
terminal_toolbars_model_class_init (TerminalToolbarsModelClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_toolbars_model_finalize;
}



static void
terminal_toolbars_model_init (TerminalToolbarsModel *model)
{
  gchar *file;

  exo_toolbars_model_set_actions (EXO_TOOLBARS_MODEL (model),
                                  (gchar **) actions,
                                  G_N_ELEMENTS (actions));

  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "Terminal/Terminal-toolbars.ui");
  xfce_resource_pop_path (XFCE_RESOURCE_DATA);

  if (G_LIKELY (file != NULL))
    {
      exo_toolbars_model_load_from_file (EXO_TOOLBARS_MODEL (model), file, NULL);
      g_free (file);
    }
  else
    {
      g_warning ("Unable to locate Terminal/Terminal-toolbars.ui, "
                 "the toolbars may not work correctly");
    }

  g_signal_connect (G_OBJECT (model), "item-added",
                    G_CALLBACK (terminal_toolbars_model_queue_sync), NULL);
  g_signal_connect (G_OBJECT (model), "item-removed",
                    G_CALLBACK (terminal_toolbars_model_queue_sync), NULL);
  g_signal_connect (G_OBJECT (model), "toolbar-added",
                    G_CALLBACK (terminal_toolbars_model_queue_sync), NULL);
  g_signal_connect (G_OBJECT (model), "toolbar-changed",
                    G_CALLBACK (terminal_toolbars_model_queue_sync), NULL);
  g_signal_connect (G_OBJECT (model), "toolbar-removed",
                    G_CALLBACK (terminal_toolbars_model_queue_sync), NULL);
}



static void
terminal_toolbars_model_finalize (GObject *object)
{
  TerminalToolbarsModel *model = TERMINAL_TOOLBARS_MODEL (object);

  if (G_UNLIKELY (model->sync_id != 0))
    g_source_remove (model->sync_id);

  (*G_OBJECT_CLASS (terminal_toolbars_model_parent_class)->finalize) (object);
}



static void
terminal_toolbars_model_queue_sync (TerminalToolbarsModel *model)
{
  if (G_LIKELY (model->sync_id == 0))
    {
      model->sync_id = g_timeout_add_seconds_full (G_PRIORITY_LOW, 1, terminal_toolbars_model_sync,
                                                   model, terminal_toolbars_model_sync_destroy);
    }
}



static gboolean
terminal_toolbars_model_sync (gpointer user_data)
{
  TerminalToolbarsModel *model = TERMINAL_TOOLBARS_MODEL (user_data);
  gchar                 *file;

  file = xfce_resource_save_location (XFCE_RESOURCE_DATA, "Terminal/Terminal-toolbars.ui", TRUE);
  exo_toolbars_model_save_to_file (EXO_TOOLBARS_MODEL (model), file, NULL);
  g_free (file);

  return FALSE;
}



static void
terminal_toolbars_model_sync_destroy (gpointer user_data)
{
  TERMINAL_TOOLBARS_MODEL (user_data)->sync_id = 0;
}



/**
 * terminal_toolbars_model_get_default:
 *
 * Return value :
 **/
ExoToolbarsModel*
terminal_toolbars_model_get_default (void)
{
  static ExoToolbarsModel *model = NULL;

  if (G_UNLIKELY (model == NULL))
    {
      model = g_object_new (TERMINAL_TYPE_TOOLBARS_MODEL, NULL);
      g_object_add_weak_pointer (G_OBJECT (model), (gpointer) &model);
    }
  else
    {
      g_object_ref (G_OBJECT (model));
    }

  return model;
}



