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



static void    terminal_preferences_dialog_finalize    (GObject                    *object);
static void    terminal_preferences_dialog_response    (GtkDialog                  *dialog,
                                                        gint                        response);
static void    terminal_preferences_dialog_configure   (TerminalPreferencesDialog  *dialog);
static void    terminal_preferences_dialog_new_section (GtkWidget                  **frame,
                                                        GtkWidget                  **vbox,
                                                        GtkWidget                  **grid,
                                                        GtkWidget                  **label,
                                                        gint                        *row,
                                                        const gchar                 *header);
static void    terminal_gtk_label_set_a11y_relation    (GtkLabel                   *label,
                                                        GtkWidget                  *widget);
PangoAttrList *terminal_pango_attr_list_bold           (void);



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



PangoAttrList*
terminal_pango_attr_list_bold (void)
{
  static PangoAttrList *attr_list = NULL;
  if (G_UNLIKELY (attr_list == NULL))
    {
      attr_list = pango_attr_list_new ();
      pango_attr_list_insert (attr_list, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
    }
  return attr_list;
}



void
terminal_preferences_dialog_new_section (GtkWidget   **frame,
                                         GtkWidget   **vbox,
                                         GtkWidget   **grid,
                                         GtkWidget   **label,
                                         gint         *row,
                                         const gchar  *header)
{
  *frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (*vbox), *frame, FALSE, TRUE, 0);
  gtk_widget_show (*frame);

  *label = gtk_label_new (_(header));
  /* For bold text */
  gtk_label_set_attributes (GTK_LABEL (*label), terminal_pango_attr_list_bold());
  gtk_frame_set_label_widget (GTK_FRAME (*frame), *label);
  gtk_widget_show (*label);

  /* init row */
  *row = 0;

  *grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (*grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (*grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (*grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (*grid), 12);
  gtk_container_add (GTK_CONTAINER (*frame), *grid);
  gtk_widget_show (*grid);
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



static void
terminal_preferences_dialog_init (TerminalPreferencesDialog *dialog)
{
  GtkWidget *notebook;
  GtkWidget *button;
  GtkWidget *image;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *box;
  GtkWidget *infobar;
  GtkWidget *grid;
  GtkWidget *entry;
  GtkWidget *combo;
  gint       row = 0;

  /* grab a reference on the preferences */
  dialog->preferences = terminal_preferences_get();

  /* configure the dialog properties */
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "org.xfce.terminal");
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
  /* TODO: configure button to show xfce-terminal's docs instead of the default thunar's docs */
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
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "Title");

  label = gtk_label_new_with_mnemonic (_("_Initial title:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "");
  /* TODO: bind object */
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 1, 1);
  gtk_widget_show (entry);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic (_("Dynamically-set title:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Replaces initial title"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Goes before initial title"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Goes after initial title"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Isn't displayed"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);
  /* TODO: add rest of the combo elemenets & bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);


  /* section: Command */
  terminal_preferences_dialog_new_section  (&frame, &vbox, &grid, &label, &row, "Command");

  button = gtk_check_button_new_with_mnemonic (_("_Run command as login shell"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Select this option to force Terminal to run your shell"
                                         " as a login shell when you open new terminals. See the"
                                         " documentation of your shell for details about differences"
                                         " between running it as interactive shell and running it as"
                                         " login shell."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Run a custom command instead of my shell"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Select this option to force Terminal to run a custom command"
                                         " instead of your shell when you open new terminals"));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic (_("Custom command:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "");
  /* TODO: bind object */
  gtk_widget_set_halign (entry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 1, 1);
  gtk_widget_show (entry);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Working directory:"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Select this option to make new temrinals (tabs or windows) use custom"
                                         " working directory. Otherwise, current working directory will be used."));
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "");
  /* TODO: bind object */
  gtk_widget_set_halign (entry, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 1, 1);
  gtk_widget_show (entry);


  /* section: Scrolling */
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "Scrolling");

  button = gtk_check_button_new_with_mnemonic (_("_Scroll on output"));
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
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);
  /* TODO: add rest of the combo elemenets & bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 2, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic (_("Scrollback:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (1, 1000000, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), 1000);
  /* TODO: Bind Object */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("_Unlimited Scrollback"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("This controls whether the terminal will have no limits on scrollback."));
  gtk_grid_attach (GTK_GRID (grid), button, 2, row, 1, 1);
  gtk_widget_show (button);


  /* section: Cursor */
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "Cursor");

  label = gtk_label_new_with_mnemonic (_("Scrollbar is:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Block"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("I-Beam"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Underline"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  /* TODO: bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);
  
  button = gtk_check_button_new_with_mnemonic (_("Cursor blinks"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("This option controls whether the cursor will be blinking or not."));
  gtk_widget_set_halign (button, GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), button, 2, row, 1, 1);
  gtk_widget_show (button);


  /* section: Clipboard */
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "Clipboard");

  button = gtk_check_button_new_with_mnemonic (_("Automatically copy selection to clipboard"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Show unsafe paste dialog"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Show a dialog that allows to edit text that is considered unsafe for pasting"));
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);



  /*
    Appearance
  */
  label = gtk_label_new (_("Appearance"));
  vbox = g_object_new (GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 12, "spacing", 18, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (label);
  gtk_widget_show (vbox);


  /* section: Font */
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "Font");

  button = gtk_check_button_new_with_mnemonic (_("_Use system font"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this to use system-wide monospace font."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_font_button_new ();
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 5, 1);
  gtk_widget_show (button);

  /* next row */
  row++;
  
  button = gtk_check_button_new_with_mnemonic (_("_Allow bold text"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this to allow applications running inside the terminal windows to use bold font."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 6, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic(_("Text blinks:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Never"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("When focused"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("When unfocused"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Always"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 3);
  /* TODO: bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 5, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic(_("Cell spacing:"));
  gtk_label_set_xalign(GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (1, 1000000, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), 1000);
  /* TODO: Bind Object */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic(_("x width"));
  gtk_label_set_xalign(GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 2, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (1, 1000000, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), 1000);
  /* TODO: Bind Object */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 3, row, 1, 1);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic(_("x height"));
  gtk_label_set_xalign(GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 4, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_button_new_with_mnemonic (_("Reset"));
  /* TODO: Bind Object */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 5, row, 1, 1);
  gtk_widget_show (button);


  /* section: Background */
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "Background");

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Node (use solid color)"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Background image"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Transparent Background"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  /* TODO: bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 0, row, 1, 1);
  gtk_widget_show (combo);


  /* section: Opening new windows */
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "Opening New Nindows");

  button = gtk_check_button_new_with_mnemonic (_("_Display menubar in new windows"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this option to show menubars in newly created terminal windows."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 5, 1);
  gtk_widget_show (button);

  /* Next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Display toolbar in new windows"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this option to show toolbar in newly created terminal windows."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 5, 1);
  gtk_widget_show (button);

  /* Next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Display borders around new windows"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this option to show window decorations around newly created terminal windows."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 5, 1);
  gtk_widget_show (button);

  /* Next row */
  row++;

  label = gtk_label_new_with_mnemonic(_("Default geometry"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (10, 4000, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), 80);
  /* TODO: Bind Object */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic(_("columns"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 2, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (10, 3000, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), 24);
  /* TODO: Bind Object */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 3, row, 1, 1);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic(_("rows"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 4, row, 1, 1);
  gtk_widget_show (label);


  /* section: Tabs */
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "Tabs");

  label = gtk_label_new_with_mnemonic(_("Reset tab activity indicator after"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (0, 1000, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), 2);
  /* TODO: Bind Object */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic(_("seconds"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 2, row, 1, 1);
  gtk_widget_show (label);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Use custom styleing to make tabs slim (restart required)"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 3, 1);
  gtk_widget_show (button);



  /*
    Colors
  */
  label = gtk_label_new (_("Colors"));
  vbox = g_object_new (GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 12, "spacing", 18, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (label);
  gtk_widget_show (vbox);


  /* section: General */
  terminal_preferences_dialog_new_section (&frame, &vbox, &grid, &label, &row, "General");

  label = gtk_label_new ("Note: Ctrl + Click for color editor.");
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 6, 1);
  gtk_widget_show (label);

  /* next row */
  row++;

  label = gtk_label_new ("Text Color:");
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_color_button_new ();
  /* TODO: Binding */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);

  label = gtk_label_new ("Background Color:");
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 2, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_color_button_new ();
  /* TODO: Binding */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 3, row, 1, 1);
  gtk_widget_show (button);

  label = gtk_label_new ("Tab activity color:");
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 4, row, 1, 1);
  gtk_widget_show (label);

  button = gtk_color_button_new ();
  /* TODO: Binding */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 5, row, 1, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Use system theme colors for text and background"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 6, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Vary the background color for each tab"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("The random color is based on the selected background color, keeping the same brightness."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 6, 1);
  gtk_widget_show (button);


  /* section: Custom colors */
  terminal_preferences_dialog_new_section(&frame, &vbox, &grid, &label, &row, "Custom Colors");

  button = gtk_check_button_new_with_mnemonic (_("_Cursor color:"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this option to set custom text and background colors for cursor."
                                         " If disabled the background and text colors will be reversed."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  /* TODO: Binding */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  /* TODO: Binding */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 2, row, 1, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Text selection color:"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this option to set custom text and background colors for cursor."
                                         " If disabled the background and text colors will be reversed."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  /* TODO: Binding */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  /* TODO: Binding */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 2, row, 1, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Bold selection color:"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this option to set a custom bold color. If disabled the text color will be used."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  /* TODO: Binding */
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);


  /* section: Palette */
  terminal_preferences_dialog_new_section(&frame, &vbox, &grid, &label, &row, "Palette");

  button = gtk_check_button_new_with_mnemonic (_("_Show bold text in bright colors:"));
  /* TODO: Bind object */
  gtk_widget_set_tooltip_text (button, _("Enable this option to allow escape sequences such as '\e[1;35m' to switch text "
                                         "bright colors in addition to bold. If disabled, text color will remain intact."));
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  /* section: Presets */
  terminal_preferences_dialog_new_section(&frame, &vbox, &grid, &label, &row, "Presets");

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Load Presets..."));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Black on White"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Dark Pastels"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  /* TODO: bind object */
  gtk_widget_set_halign (combo, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), combo, 0, row, 1, 1);
  gtk_widget_show (combo);



  /*
    Compatibility
  */
  label = gtk_label_new (_("Compatibility"));
  vbox = g_object_new (GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 12, "spacing", 18, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (label);
  gtk_widget_show (vbox);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = gtk_label_new (_("Compatibility"));
  gtk_label_set_attributes (GTK_LABEL (label), terminal_pango_attr_list_bold ());
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  infobar = gtk_info_bar_new ();
  gtk_container_set_border_width (GTK_CONTAINER (infobar), 1);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), _("These options may cause some applications to behave incorrectly.\n"
                                             "They are only here to allow you to work around certain applications\n"
                                             "and operating systems that expect different terminal behavior."));
  box = gtk_info_bar_get_content_area (GTK_INFO_BAR (infobar));
  gtk_container_add (GTK_CONTAINER (box), label);
  gtk_info_bar_set_message_type (GTK_INFO_BAR (infobar), GTK_MESSAGE_WARNING);
  gtk_widget_show (label);
  gtk_widget_show (infobar);
  gtk_container_add (GTK_CONTAINER (frame), infobar);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  /* init row */
  row = 0;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  label = gtk_label_new_with_mnemonic (_("Dynamically-set title:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Auto-detect"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("ASCII Del"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Escapre sequence"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Control-H"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Erase TTY"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  /* TODO: add rest of the combo elemenets & bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic (_("Delete key generates:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Auto-detect"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("ASCII Del"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Escapre sequence"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Control-H"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Erase TTY"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  /* TODO: add rest of the combo elemenets & bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic (_("Ambiguous-width characters:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Narrow"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Wide"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  /* TODO: add rest of the combo elemenets & bind object */
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* next row */
  row++;

  button = gtk_button_new_with_mnemonic(_("Reset compatibility options to defaults"));
  /* TODO: Bind */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);



  /*
    Advanced
  */
  label = gtk_label_new (_("Advanced"));
  vbox = g_object_new (GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 12, "spacing", 18, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (label);
  gtk_widget_show (vbox);


  /* section: Double Click */
  terminal_preferences_dialog_new_section(&frame, &vbox, &grid, &label, &row, "Double Click");

  label = gtk_label_new_with_mnemonic(_("Consider the following"
                                        "characters part of a word"
                                        "when double clicking:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  /* next row */
  row++;

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "");
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 0, row, 1, 1);
  gtk_widget_show (entry);

  button = gtk_button_new_with_mnemonic (_("Reset"));
  /* TODO: Bind */
  gtk_grid_attach(GTK_GRID (grid), button, 1, row, 1, 1);
  gtk_widget_show (button);


  /* section: Encoding */
  terminal_preferences_dialog_new_section(&frame, &vbox, &grid, &label, &row, "Encoding");

  label = gtk_label_new_with_mnemonic(_("Default character encoding"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Default (UTF-8)"));
  /* TODO: add rest of the combo elemenets & bind object */
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  
  /* section: Shortcuts */
  terminal_preferences_dialog_new_section(&frame, &vbox, &grid, &label, &row, "Shortcuts");

  button = gtk_check_button_new_with_mnemonic (_("_Disable all menu access keys (such as Alt + f)"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Disable menu shortcut key (F10 by default)"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Disable help window shortcut key (F1 by default)"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  gtk_widget_show (button);


  /* section: Misc */
  terminal_preferences_dialog_new_section(&frame, &vbox, &grid, &label, &row, "Misc");

  button = gtk_check_button_new_with_mnemonic (_("_Use middle mouse click to close tabs"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Use middle mouse click to open urls"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Auto-hide mouse pointer"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Open new tab to the right of the active tab"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Audible bell"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  button = gtk_check_button_new_with_mnemonic (_("_Visual bell"));
  /* TODO: Bind object */
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 2, 1);
  gtk_widget_show (button);

  /* next row */
  row++;

  label = gtk_label_new_with_mnemonic(_("Right click action:"));
  gtk_label_set_xalign (GTK_LABEL(label), 0.0f);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Context Menu"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Paste clipboard"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("Paste selection"));
  /* TODO: add rest of the combo elemenets & bind object */
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  gtk_widget_set_hexpand (combo, TRUE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);


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

  grid = xfce_shortcuts_editor_new (4,
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
      xfce_dialog_show_help (GTK_WINDOW (dialog), "xfce4-terminal",
                             "preferences", NULL);
    }
  else
    {
      /* close the preferences dialog */
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}



static void
terminal_preferences_dialog_configure (TerminalPreferencesDialog *dialog)
{
  /* TODO: */
}



GtkWidget*
terminal_preferences_dialog_new (GtkWindow *parent)
{
  return  g_object_new (TERMINAL_TYPE_PREFERENCES_DIALOG, NULL);
}
