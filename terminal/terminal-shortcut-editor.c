/* $Id: terminal-shortcut-editor.c,v 1.2 2004/09/18 22:06:16 bmeurer Exp $ */
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
 *
 * The modifier check was taken from egg/eggcellrendererkeys.c.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkkeysyms.h>

#include <terminal/terminal-preferences.h>
#include <terminal/terminal-shortcut-editor.h>



#define TERMINAL_RESPONSE_CLEAR   1



enum
{
  COLUMN_TITLE,
  COLUMN_ACCEL,
  COLUMN_PROPERTY,
  LAST_COLUMN,
};



static void     terminal_shortcut_editor_class_init   (TerminalShortcutEditorClass  *klass);
static void     terminal_shortcut_editor_init         (TerminalShortcutEditor       *editor);
static void     terminal_shortcut_editor_finalize     (GObject                      *object);
static void     terminal_shortcut_editor_activate     (TerminalShortcutEditor       *editor,
                                                       GtkTreePath                  *path,
                                                       GtkTreeViewColumn            *column);
static gboolean terminal_shortcut_editor_compose      (GtkWidget                    *dialog,
                                                       GdkEventKey                  *event,
                                                       TerminalShortcutEditor       *editor);
static void     terminal_shortcut_editor_notify       (TerminalPreferences          *preferences,
                                                       GParamSpec                   *pspec,
                                                       TerminalShortcutEditor       *editor);



typedef struct
{
  gchar *title;
  gchar *accels[32];
} ToplevelMenu;



struct _TerminalShortcutEditor
{
  GtkTreeView          __parent__;
  TerminalPreferences *preferences;
};



static GObjectClass *parent_class;

static ToplevelMenu toplevel_menus[] =
{
  {
    N_ ("File"),
    {
      "accel-new-tab",
      "accel-new-window",
      "accel-close-tab",
      "accel-close-window",
      NULL,
    },
  },
  {
    N_ ("Edit"),
    {
      "accel-copy",
      "accel-paste",
      "accel-preferences",
      NULL,
    },
  },
  {
    N_ ("View"),
    {
      "accel-compact-mode",
      "accel-fullscreen",
      NULL,
    },
  },
  {
    N_ ("Terminal"),
    {
      "accel-prev-tab",
      "accel-next-tab",
      /*"accel-set-title", FIXME */
      "accel-reset",
      "accel-reset-and-clear",
      NULL,
    },
  },
};



G_DEFINE_TYPE (TerminalShortcutEditor, terminal_shortcut_editor, GTK_TYPE_TREE_VIEW);



static void
terminal_shortcut_editor_class_init (TerminalShortcutEditorClass *klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_shortcut_editor_finalize;
}



static void
terminal_shortcut_editor_init (TerminalShortcutEditor *editor)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;
  ToplevelMenu      *menu;
  GtkTreeStore      *store;
  GtkTreeIter        parent;
  GtkTreeIter        child;
  GParamSpec        *pspec;
  gchar             *signal;
  gchar             *accel;
  gint               n;

  editor->preferences = terminal_preferences_get ();

  store = gtk_tree_store_new (LAST_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  for (menu = toplevel_menus; menu < toplevel_menus + G_N_ELEMENTS (toplevel_menus); ++menu)
    {
      gtk_tree_store_append (store, &parent, NULL);
      gtk_tree_store_set (store, &parent,
                          COLUMN_TITLE, _(menu->title),
                          -1);

      for (n = 0; menu->accels[n] != NULL; ++n)
        {
          pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (editor->preferences), menu->accels[n]);

          signal = g_strconcat ("notify::", menu->accels[n], NULL);
          g_signal_connect (G_OBJECT (editor->preferences), signal,
                            G_CALLBACK (terminal_shortcut_editor_notify), editor);
          g_free (signal);

          g_object_get (G_OBJECT (editor->preferences), pspec->name, &accel, NULL);

          gtk_tree_store_append (store, &child, &parent);
          gtk_tree_store_set (store, &child,
                              COLUMN_TITLE, g_param_spec_get_nick (pspec),
                              COLUMN_ACCEL, accel,
                              COLUMN_PROPERTY, pspec->name,
                              -1);

          if (accel != NULL)
            g_free (accel);
        }
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (editor), GTK_TREE_MODEL (store));
  gtk_tree_view_expand_all (GTK_TREE_VIEW (editor));
  g_object_unref (G_OBJECT (store));

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Action"),
                                                     renderer,
                                                     "text", COLUMN_TITLE,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (editor), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Shortcut key"),
                                                     renderer,
                                                     "text", COLUMN_ACCEL,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (editor), column);

  g_signal_connect (G_OBJECT (editor), "row-activated",
                    G_CALLBACK (terminal_shortcut_editor_activate), NULL);
}



static void
terminal_shortcut_editor_finalize (GObject *object)
{
  TerminalShortcutEditor *editor = TERMINAL_SHORTCUT_EDITOR (object);

  g_signal_handlers_disconnect_by_func (G_OBJECT (editor->preferences),
                                        G_CALLBACK (terminal_shortcut_editor_notify),
                                        editor);
  g_object_unref (G_OBJECT (editor->preferences));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}



static gboolean
is_modifier (guint keycode)
{
  XModifierKeymap *keymap;
  gboolean         result = FALSE;
  gint             n;

  keymap = XGetModifierMapping (gdk_display);
  for (n = 0; n < keymap->max_keypermod * 8; ++n)
    if (keycode == keymap->modifiermap[n])
      {
        result = TRUE;
        break;
      }

  XFreeModifiermap (keymap);

  return result;
}



static void
terminal_shortcut_editor_activate (TerminalShortcutEditor *editor,
                                   GtkTreePath            *path,
                                   GtkTreeViewColumn      *column)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GtkWidget    *toplevel;
  GtkWidget    *dialog;
  GtkWidget    *hbox;
  GtkWidget    *image;
  GtkWidget    *label;
  GdkPixbuf    *icon;
  gchar        *property;
  gchar        *title;
  gchar        *text;
  gint          response;

  if (gtk_tree_path_get_depth (path) <= 1)
    return;

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (editor));
  if (G_UNLIKELY (toplevel == NULL))
    return;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor));
  if (!gtk_tree_model_get_iter (model, &iter, path))
    return;

  dialog = gtk_dialog_new_with_buttons (_("Compose shortcut"),
                                        GTK_WINDOW (toplevel),
                                        GTK_DIALOG_DESTROY_WITH_PARENT
                                        | GTK_DIALOG_MODAL,
                                        GTK_STOCK_CLEAR, TERMINAL_RESPONSE_CLEAR,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        NULL);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  icon = xfce_themed_icon_load ("terminal-compose-key", 48);
  if (G_LIKELY (icon != NULL))
    {
      image = gtk_image_new_from_pixbuf (icon);
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
      gtk_widget_show (image);
      g_object_unref (G_OBJECT (icon));
    }

  gtk_tree_model_get (model, &iter, COLUMN_TITLE, &title, -1);
  text = g_strdup_printf ("<i>%s</i>\n<b>%s</b>",
                          _("Compose shortcut for:"),
                          title);
  g_free (title);

  label = g_object_new (GTK_TYPE_LABEL,
                        "justify", GTK_JUSTIFY_CENTER,
                        "label", text,
                        "use-markup", TRUE,
                        NULL);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  gtk_widget_show_now (dialog);

  if (gdk_keyboard_grab (dialog->window, FALSE,
                         GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
    {
      xfce_err (_("Failed to grab keyboard."));
      goto done;
    }

  gtk_tree_model_get (model, &iter, COLUMN_PROPERTY, &property, -1);
  g_object_set_data_full (G_OBJECT (dialog), "property-name", property, g_free);

  g_signal_connect (G_OBJECT (dialog), "key-press-event",
                    G_CALLBACK (terminal_shortcut_editor_compose), editor);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == TERMINAL_RESPONSE_CLEAR)
    g_object_set (G_OBJECT (editor->preferences), property, NULL, NULL);

  gdk_keyboard_ungrab (GDK_CURRENT_TIME);

done:
  gtk_widget_destroy (dialog);
  g_free (text);
}



static gboolean
terminal_shortcut_editor_compose (GtkWidget              *dialog,
                                  GdkEventKey            *event,
                                  TerminalShortcutEditor *editor)
{
  GdkModifierType consumed_modifiers = 0;
  guint           keyval;
  gchar          *accelerator;
  gchar          *property;

  if (is_modifier (event->hardware_keycode))
    return TRUE;

  gdk_keymap_translate_keyboard_state (gdk_keymap_get_default (),
                                       event->hardware_keycode,
                                       event->state,
                                       event->group,
                                       NULL, NULL, NULL,
                                       &consumed_modifiers);

  keyval = gdk_keyval_to_lower (event->keyval);
  if (keyval == GDK_ISO_Left_Tab)
    keyval = GDK_Tab;

  if (keyval != event->keyval && (consumed_modifiers & GDK_SHIFT_MASK))
    consumed_modifiers &= ~GDK_SHIFT_MASK;

  accelerator = gtk_accelerator_name (keyval, event->state & ~consumed_modifiers);
  property = g_object_get_data (G_OBJECT (dialog), "property-name");
  g_object_set (G_OBJECT (editor->preferences), property, accelerator, NULL);
  g_free (accelerator);

  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  return TRUE;
}



static void
terminal_shortcut_editor_notify (TerminalPreferences    *preferences,
                                 GParamSpec             *pspec,
                                 TerminalShortcutEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   parent;
  GtkTreeIter   child;
  gchar        *property;
  gchar        *accel;

  g_object_get (G_OBJECT (preferences), pspec->name, &accel, NULL);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (editor));
  if (gtk_tree_model_get_iter_first (model, &parent))
    {
      do 
        {
          if (gtk_tree_model_iter_children (model, &child, &parent))
            {
              do
                {
                  gtk_tree_model_get (model, &child,
                                      COLUMN_PROPERTY, &property,
                                      -1);
                  if (exo_str_is_equal (property, pspec->name))
                    gtk_tree_store_set (GTK_TREE_STORE (model), &child, COLUMN_ACCEL, accel, -1);
                  g_free (property);
                }
              while (gtk_tree_model_iter_next (model, &child));
            }
        }
      while (gtk_tree_model_iter_next (model, &parent));
    }
  
  g_free (accel);
}



/**
 * terminal_shortcut_editor_new:
 *
 * Return value :
 **/
GtkWidget*
terminal_shortcut_editor_new (void)
{
  return g_object_new (TERMINAL_TYPE_SHORTCUT_EDITOR, NULL);
}


