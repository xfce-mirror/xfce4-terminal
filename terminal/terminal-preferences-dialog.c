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

#include "pango/pango-attributes.h"
#include "terminal-window.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4kbd-private-3/libxfce4kbd-private/xfce-shortcuts-editor.h>
#include <libxfce4kbd-private-3/libxfce4kbd-private/xfce-shortcuts-editor-dialog.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include <terminal/terminal-util.h>
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-encoding-action.h>
#include <terminal/terminal-private.h>



static void    terminal_preferences_dialog_finalize  (GObject                    *object);
static void    terminal_preferences_dialog_response  (GtkDialog                  *dialog,
                                                      gint                        response);
static void    terminal_preferences_dialog_configure (TerminalPreferencesDialog  *dialog);
static void    terminal_gtk_label_set_a11y_relation  (GtkLabel                   *label,
                                                      GtkWidget                  *widget);
PangoAttrList *terminal_pango_attr_list_bold         (void);
GdkScreen*     terminal_util_parse_parent            (gpointer                    parent,
                                                      GtkWindow                 **window_return);
static void    terminal_setup_display_cb             (gpointer                    data);
static void    terminal_dialogs_show_error           (gpointer                    parent,
                                                      const GError               *error,
                                                      const gchar                *format, ...);



struct _TerminalPreferencesDialogClass
{
  XfceTitledDialogClass __parent__;
};

struct _TerminalPreferencesDialog
{
  XfceTitledDialog     __parent__;
  TerminalPreferences *preferences;
};



G_DEFINE_TYPE (TerminalPreferencesDialog, terminal_preferences_dialog, XFCE_TYPE_TITLED_DIALOG)



static void
terminal_preferences_dialog_class_init (TerminalPreferencesDialogClass *klass)
{
  GtkDialogClass *gtkdialog_class;
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_preferences_dialog_finalize;

  gtkdialog_class = GTK_DIALOG_CLASS (klass);
  gtkdialog_class->response = terminal_preferences_dialog_response;
}



void
terminal_gtk_label_set_a11y_relation (GtkLabel  *label,
                                      GtkWidget *widget)
{
  AtkRelationSet *relations;
  AtkRelation    *relation;
  AtkObject      *object;

  terminal_return_if_fail (GTK_IS_WIDGET (widget));
  terminal_return_if_fail (GTK_IS_LABEL (label));

  object = gtk_widget_get_accessible (widget);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (GTK_WIDGET (label)));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));
}



PangoAttrList*
terminal_pango_attr_list_bold (void)
{
  static PangoAttrList *attr_list = NULL;
  if (G_UNLIKELY (attr_list == NULL))
    {
      // Code
      attr_list = pango_attr_list_new ();
      pango_attr_list_insert (attr_list, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
    }
  return attr_list;
}



static void
terminal_preferences_dialog_init (TerminalPreferencesDialog *dialog)
{
  GtkWidget *notebook;
  GtkWidget *button;
  GtkWidget *image;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *grid;
  GtkWidget *textbox;
  GtkWidget *combo;
  gint       row = 0;

  /* grab a reference on the preferences */
  dialog->preferences = terminal_preferences_get();

  /* configure the dialog properties */
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "org.xfce.xfce-terminal");
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Terminal Preferences"));

  xfce_titled_dialog_create_action_area (XFCE_TITLED_DIALOG (dialog));

  /* add the "Close" button */
  button = gtk_button_new_with_mnemonic (_("_Close"));
  image = gtk_image_new_from_icon_name ("window-close", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  xfce_titled_dialog_add_action_widget (XFCE_TITLED_DIALOG (dialog), button, GTK_RESPONSE_CLOSE);

  gtk_widget_show (button);

  /* add the "Help" button */
  button = gtk_button_new_with_mnemonic (_("_Help"));
  image = gtk_image_new_from_icon_name ("help-browser", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  xfce_titled_dialog_add_action_widget (XFCE_TITLED_DIALOG (dialog), button, GTK_RESPONSE_HELP);

  gtk_widget_show (button);

  notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);


  /* 
    General
  */
  label = gtk_label_new (_("General"));
  vbox = g_object_new (GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 12, "spacing", 18, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (label);
  gtk_widget_show (vbox);

  /* section: Title */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = gtk_label_new (_("Title"));
  gtk_label_set_attributes (GTK_LABEL (label), terminal_pango_attr_list_bold ());
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  label = gtk_label_new_with_mnemonic (_("_Initial title:"));
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  textbox = gtk_text_view_new ();
  /* TODO: bind object */
  gtk_widget_set_hexpand (textbox, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 1, row, 1, 1);
  gtk_widget_show (textbox);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic (_("Dynamically-set title:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Goes after initial title"));
  /* TODO: add rest of the combo elemenets & bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* section: Command */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = gtk_label_new_with_mnemonic (_("Command"));
  gtk_label_set_attributes (GTK_LABEL (label), terminal_pango_attr_list_bold ());
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  /* new grid */
  row = 0;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  button = gtk_check_button_new_with_mnemonic (_("_Run command as login shell"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Select this option to force Terminal to run your shell"
                                         " as a login shell when you open new terminals. See the"
                                         " documentation of your shell for details about differences"
                                         " between running it as interactive shell and running it as"
                                         " login shell."));
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Run a custom command instead of my shell"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Select this option to force Terminal to run a custom command"
                                         " instead of your shell when you open new terminals"));
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic (_("_Custom command:"));
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  textbox = gtk_text_view_new ();
  /* TODO: Bind object */
  gtk_widget_set_hexpand (textbox, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 1, row, 1, 1);
  gtk_widget_show (textbox);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Working directory:"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Select this option to make new temrinals (tabs or windows) use custom"
                                         " working directory. Otherwise, current working directory will be used."));
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  textbox = gtk_text_view_new ();
  /* TODO: Bind object */ gtk_widget_set_hexpand (textbox, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 2, row, 1, 1);
  gtk_widget_show (textbox);

  /* section: Scrolling */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = gtk_label_new_with_mnemonic (_("Scrolling"));
  gtk_label_set_attributes (GTK_LABEL (label), terminal_pango_attr_list_bold ());
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  /* new grid */
  row = 0;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  button = gtk_check_button_new_with_mnemonic (_("Scroll on output"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("This option controls whether the terminal will scroll down automatically"
                                         " whenever new output is generated by the commands running inside the terminal."));
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("_Enable overlay scrolling (Requires restart)"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("This controls whether scrollbar is drawn as an overlay (auto-hide) or not."));
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("_Scroll on keystroke"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enables you to press any key on the keyboard to scroll down the terminal"
                                         " to the command prompt."));
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 2, row, 1, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic (_("Scrollbar is:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Disabled"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("On the left side"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("On the right side"));
  /* TODO: bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* next row */
  row++;

  /* TODO: Scrollback row */

  /* section: Cursor */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = gtk_label_new_with_mnemonic (_("Cursor"));
  gtk_label_set_attributes (GTK_LABEL (label), terminal_pango_attr_list_bold ());
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  /* new grid */
  row = 0;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  label = gtk_label_new_with_mnemonic (_("Scrollbar is:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Block"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("I-Beam"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Underline"));
  /* TODO: bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);
  
  button = gtk_check_button_new_with_mnemonic (_("Cursor blinks"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("This option controls whether the cursor will be blinking or not."));
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  /* section: Clipboard */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = gtk_label_new_with_mnemonic (_("Clipboard"));
  gtk_label_set_attributes (GTK_LABEL (label), terminal_pango_attr_list_bold ());
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  /* new grid */
  row = 0;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  button = gtk_check_button_new_with_mnemonic (_("Automatically copy selection to clipboard"));
  /* TODO: Bind object */
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Show unsafe paste dialog"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Show a dialog that allows to edit text that is considered unsafe for pasting"));
  gtk_widget_set_hexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  /*
   Shortcuts
  */
  label = gtk_label_new (_("Shortcuts"));
  vbox = g_object_new (GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 12, "spacing", 18, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (label);
  gtk_widget_show (vbox);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_IN, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  grid = xfce_shortcuts_editor_new (13,
                                    _("Window"), terminal_window_get_action_entries (), TERMINAL_WINDOW_ACTION_N
                                    );
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);
}



static void
terminal_preferences_dialog_finalize (GObject *object)
{
  TerminalPreferencesDialog *dialog = TERMINAL_PREFERENCES_DIALOG (object);

  /* release our reference on the preferences */
  g_object_unref (G_OBJECT (dialog->preferences));

  (*G_OBJECT_CLASS (terminal_preferences_dialog_parent_class)->finalize) (object);
}



static void
terminal_preferences_dialog_response (GtkDialog *dialog,
                                    gint       response)
{
  if (G_UNLIKELY (response == GTK_RESPONSE_HELP))
    {
      /* open the preferences section of the user manual */
      xfce_dialog_show_help (GTK_WINDOW (dialog), "thunar",
                             "preferences", NULL);
    }
  else
    {
      /* close the preferences dialog */
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}



void
terminal_setup_display_cb (gpointer data)
{
  g_setenv ("DISPLAY", (char *) data, TRUE);
}



GdkScreen*
terminal_util_parse_parent (gpointer    parent,
                            GtkWindow **window_return)
{
  GdkScreen *screen;
  GtkWidget *window = NULL;

  terminal_return_val_if_fail (parent == NULL || GDK_IS_SCREEN (parent) || GTK_IS_WIDGET (parent), NULL);

  /* determine the proper parent */
  if (parent == NULL)
    {
      /* just use the default screen then */
      screen = gdk_screen_get_default ();
    }
  else if (GDK_IS_SCREEN (parent))
    {
      /* yep, that's a screen */
      screen = GDK_SCREEN (parent);
    }
  else
    {
      /* parent is a widget, so let's determine the toplevel window */
      window = gtk_widget_get_toplevel (GTK_WIDGET (parent));
      if (window != NULL
          && gtk_widget_is_toplevel (window))
        {
          /* make sure the toplevel window is shown */
          gtk_widget_show_now (window);
        }
      else
        {
          /* no toplevel, not usable then */
          window = NULL;
        }

      /* determine the screen for the widget */
      screen = gtk_widget_get_screen (GTK_WIDGET (parent));
    }

  /* check if we should return the window */
  if (G_LIKELY (window_return != NULL))
    *window_return = (GtkWindow *) window;

  return screen;
}



void
terminal_dialogs_show_error (gpointer      parent,
                           const GError *error,
                           const gchar  *format,
                           ...)
{
  GtkWidget *dialog;
  GtkWindow *window;
  GdkScreen *screen;
  va_list    args;
  gchar     *primary_text;
  GList     *children;
  GList     *lp;

  terminal_return_if_fail (parent == NULL || GDK_IS_SCREEN (parent) || GTK_IS_WIDGET (parent));

  /* do not display error dialog for already handled errors */
  if (error && error->code == G_IO_ERROR_FAILED_HANDLED)
    return;

  /* parse the parent pointer */
  screen = terminal_util_parse_parent (parent, &window);

  /* determine the primary error text */
  va_start (args, format);
  primary_text = g_strdup_vprintf (format, args);
  va_end (args);

  /* allocate the error dialog */
  dialog = gtk_message_dialog_new (window,
                                   GTK_DIALOG_DESTROY_WITH_PARENT
                                   | GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                   "%s.", primary_text);

  /* move the dialog to the appropriate screen */
  if (G_UNLIKELY (window == NULL && screen != NULL))
    gtk_window_set_screen (GTK_WINDOW (dialog), screen);

  /* set secondary text if an error is provided */
  if (G_LIKELY (error != NULL))
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s.", error->message);

  children = gtk_container_get_children (
    GTK_CONTAINER (gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (dialog))));

  /* enable wrap for labels */
  for (lp = children; lp != NULL; lp = lp->next)
    if (GTK_IS_LABEL (lp->data))
      gtk_label_set_line_wrap_mode (GTK_LABEL (lp->data), PANGO_WRAP_WORD_CHAR);

  /* display the dialog */
  gtk_dialog_run (GTK_DIALOG (dialog));

  /* cleanup */
  gtk_widget_destroy (dialog);
  g_free (primary_text);
  g_list_free (children);
}



static void
terminal_preferences_dialog_configure (TerminalPreferencesDialog *dialog)
{
  GError    *err = NULL;
  gchar     *argv[3];
  GdkScreen *screen;
  char      *display = NULL;

  terminal_return_if_fail (THUNAR_IS_PREFERENCES_DIALOG (dialog));

  /* prepare the argument vector */
  argv[0] = (gchar *) "thunar-volman";
  argv[1] = (gchar *) "--configure";
  argv[2] = NULL;

  screen = gtk_widget_get_screen (GTK_WIDGET (dialog));

  if (screen != NULL)
    display = g_strdup (gdk_display_get_name (gdk_screen_get_display (screen)));

  /* invoke the configuration interface of thunar-volman */
  if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, terminal_setup_display_cb, display, NULL, &err))
    {
      /* tell the user that we failed to come up with the thunar-volman configuration dialog */
      terminal_dialogs_show_error (dialog, err, _("Failed to display the volume management settings"));
      g_error_free (err);
    }

  g_free (display);
}



GtkWidget*
terminal_preferences_dialog_new (GtkWindow *parent)
{
  GtkWidget    *dialog;
  gint          root_x, root_y;
  gint          window_width, window_height;
  gint          dialog_width, dialog_height;
  GdkMonitor *  monitor;
  GdkRectangle  geometry;
  GdkScreen    *screen;

  dialog = g_object_new (TERMINAL_TYPE_PREFERENCES_DIALOG, NULL);

  /* fake gtk_window_set_transient_for() for centering the window on
   * the parent window. See bug #3586. */
  if (G_LIKELY (parent != NULL))
    {
      screen = gtk_window_get_screen (parent);
      gtk_window_set_screen (GTK_WINDOW (dialog), screen);
      gtk_widget_realize (dialog);

      /* get the size and position on the windows */
      gtk_window_get_position (parent, &root_x, &root_y);
      gtk_window_get_size (parent, &window_width, &window_height);
      gtk_window_get_size (GTK_WINDOW (dialog), &dialog_width, &dialog_height);

      /* get the monitor geometry of the monitor with the parent window */
      monitor = gdk_display_get_monitor_at_point (gdk_display_get_default (), root_x, root_y);
      gdk_monitor_get_geometry (monitor, &geometry);

      /* center the dialog on the window and clamp on the monitor */
      root_x += (window_width - dialog_width) / 2;
      root_x = CLAMP (root_x, geometry.x, geometry.x + geometry.width - dialog_width);
      root_y += (window_height - dialog_height) / 2;
      root_y = CLAMP (root_y, geometry.y, geometry.y + geometry.height - dialog_height);

      /* move the dialog */
      gtk_window_move (GTK_WINDOW (dialog), root_x, root_y);
    }

  return dialog;
}
