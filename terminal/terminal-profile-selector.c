/*-
 * Copyright 2022 Amrit Borah <Elessar1802@xfce.org>
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

#include "terminal-preferences.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

#include <terminal/terminal-preferences.h>
#include <terminal/terminal-profile-selector.h>



static void  terminal_profile_selector_finalize             (GObject                   *object);
static void  terminal_profile_selector_add_new_profile      (TerminalProfileSelector   *dialog);
static void  terminal_profile_selector_remove_profile       (TerminalProfileSelector   *dialog);
static void  terminal_profile_selector_activate_profile     (TerminalProfileSelector   *dialog);
static void  terminal_profile_selector_populate_store       (TerminalProfileSelector   *dialog,
                                                             GtkListStore               *store);



enum
{
  COLUMN_PROFILE_NAME,
  COLUMN_PROFILE_ICON_NAME,
  N_COLUMN,
};

struct _TerminalProfileSelectorClass
{
  GtkBinClass          __parent__;
};

struct _TerminalProfileSelector
{
  GtkBin               __parent__;

  TerminalPreferences *preferences;

  gint                 n_presets;
  GtkListStore        *store;
  GtkTreeView         *view;
};



G_DEFINE_TYPE (TerminalProfileSelector, terminal_profile_selector, GTK_TYPE_BIN)



static void
terminal_profile_selector_class_init (TerminalProfileSelectorClass *klass)
{
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_profile_selector_finalize;
}



static void
terminal_profile_selector_init (TerminalProfileSelector *widget)
{
  GtkTreeViewColumn *column;
  GtkListStore      *store;
  GtkWidget         *button;
  GtkWidget         *vbox;
  GtkWidget         *hbox;
  GtkWidget         *view;

  widget->preferences = terminal_preferences_get ();

  store = gtk_list_store_new (N_COLUMN, G_TYPE_STRING, G_TYPE_STRING);
  terminal_profile_selector_populate_store (widget, store);
  widget->store = store;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  widget->view = GTK_TREE_VIEW (view);
  gtk_widget_set_margin_start (view, 12);
  gtk_widget_set_margin_end (view, 12);
  column = gtk_tree_view_column_new_with_attributes ("Profiles",
                                                     gtk_cell_renderer_text_new (),
                                                     "text", COLUMN_PROFILE_NAME,
                                                     NULL);
  gtk_tree_view_column_set_min_width (column, 150);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
  column = gtk_tree_view_column_new_with_attributes ("Enabled",
                                                     gtk_cell_renderer_pixbuf_new (),
                                                     "icon-name", COLUMN_PROFILE_ICON_NAME,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
  gtk_box_pack_start (GTK_BOX (vbox), view, TRUE, TRUE, 6);
  gtk_widget_show (view);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start (hbox, 12);
  gtk_widget_set_margin_end (hbox, 12);
  button = gtk_button_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (terminal_profile_selector_add_new_profile), widget);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_icon_name ("list-remove", GTK_ICON_SIZE_BUTTON);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (terminal_profile_selector_remove_profile), widget);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_icon_name ("object-select", GTK_ICON_SIZE_BUTTON);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (terminal_profile_selector_activate_profile), widget);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 6);
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);

  gtk_container_add (GTK_CONTAINER (GTK_BIN (widget)), vbox);
}



static void
terminal_profile_selector_finalize (GObject *object)
{
  TerminalProfileSelector *widget = TERMINAL_PROFILE_SELECTOR (object);
}



static void
terminal_profile_selector_add_new_profile (TerminalProfileSelector *dialog)
{
  GtkTreeIter  iter;
  gchar       *profile_name = NULL;
  GtkWidget   *entry_dialog;
  GtkWidget   *entry;
  gint         response;

  /* create input box */
  entry = gtk_entry_new ();
  gtk_widget_set_margin_start (entry, 15);
  gtk_widget_set_margin_end (entry, 15);
  entry_dialog = xfce_titled_dialog_new ();
  gtk_window_set_default_size (GTK_WINDOW (entry_dialog), 300, 50);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (entry_dialog))), entry, TRUE, TRUE, 10);
  gtk_widget_show (entry);
  xfce_titled_dialog_create_action_area (XFCE_TITLED_DIALOG (entry_dialog));
  xfce_titled_dialog_add_button (XFCE_TITLED_DIALOG (entry_dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
  xfce_titled_dialog_add_button (XFCE_TITLED_DIALOG (entry_dialog), _("Accept"), GTK_RESPONSE_ACCEPT);
  response = gtk_dialog_run (GTK_DIALOG (entry_dialog));
  switch (response)
    {
    case GTK_RESPONSE_CANCEL:
      gtk_widget_destroy (entry_dialog);
      break;
    default:
      if (gtk_entry_get_text_length (GTK_ENTRY (entry)) > 0)
        profile_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
      gtk_widget_destroy (entry_dialog);
    }

  if (profile_name == NULL)
    return;
  gtk_list_store_append (dialog->store, &iter);
  gtk_list_store_set (dialog->store, &iter, COLUMN_PROFILE_NAME, profile_name, -1);
  terminal_preferences_add_profile (dialog->preferences, profile_name);
  g_free (profile_name);
}



static void
terminal_profile_selector_remove_profile (TerminalProfileSelector *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  gchar            *name;
  gchar            *icon_name;

  model = GTK_TREE_MODEL (dialog->store);
  selection = gtk_tree_view_get_selection (dialog->view);
  if (gtk_tree_selection_count_selected_rows (selection) != 1)
    return;
  gtk_tree_selection_get_selected (selection, &model, &iter);
  gtk_tree_model_get (model, &iter, COLUMN_PROFILE_NAME, &name, COLUMN_PROFILE_ICON_NAME, &icon_name, -1);
  if (g_strcmp0 (icon_name, "object-select") == 0)
    return;
  gtk_list_store_remove (dialog->store, &iter);
  terminal_preferences_remove_profile (dialog->preferences, name);
}



static gboolean
terminal_profile_selector_tree_model_foreach (GtkTreeModel *model,
                                                GtkTreePath  *path,
                                                GtkTreeIter  *iter,
                                                gpointer      data)
{
  gtk_list_store_set (GTK_LIST_STORE (data), iter, COLUMN_PROFILE_ICON_NAME, NULL, -1);

  return FALSE;
}



static void
terminal_profile_selector_activate_profile (TerminalProfileSelector *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  gchar            *profile_name;

  model = GTK_TREE_MODEL (dialog->store);
  selection = gtk_tree_view_get_selection (dialog->view);
  gtk_tree_selection_get_selected (selection, &model, &iter);

  /* remove previous selection */
  gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) terminal_profile_selector_tree_model_foreach, dialog->store);
  /* set selection */
  gtk_list_store_set (dialog->store, &iter, COLUMN_PROFILE_ICON_NAME, g_strdup ("object-select"), -1);
  gtk_tree_model_get (GTK_TREE_MODEL (dialog->store), &iter, COLUMN_PROFILE_NAME, &profile_name, -1);
  
  terminal_preferences_switch_profile (dialog->preferences, profile_name);
}



static void
terminal_profile_selector_populate_store (TerminalProfileSelector *widget,
                                            GtkListStore            *store)
{
  GtkTreeIter  iter;
  gchar       *def = terminal_preferences_get_default_profile (widget->preferences);
  gchar      **str = terminal_preferences_get_profiles (widget->preferences);
  gint         n   = terminal_preferences_get_n_profiles (widget->preferences);

  for (gint i = 0; i < n; i++)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_PROFILE_NAME, str[i],
                          COLUMN_PROFILE_ICON_NAME, g_strcmp0 (str[i], def) == 0 ? "object-select" : NULL, -1);
    }
}



GtkWidget *
terminal_profile_selector_new (void)
{
  static TerminalProfileSelector *widget = NULL;

  if (G_LIKELY (widget == NULL))
    {
      widget = g_object_new (TERMINAL_TYPE_PROFILE_SELECTOR, NULL);
      g_object_add_weak_pointer (G_OBJECT (widget), (gpointer) &widget);
    }
  else
      g_object_ref (widget);

  return GTK_WIDGET (widget);
}
