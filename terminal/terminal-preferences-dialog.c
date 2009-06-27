/* $Id$ */
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
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <terminal/terminal-dialogs.h>
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-gtk-extensions.h>
#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-shortcut-editor.h>
#include <terminal/terminal-stock.h>
#include <terminal/terminal-private.h>



/* Property identifiers */
enum
{
  PIXBUF_COLUMN,
  TEXT_COLUMN,
  INDEX_COLUMN,
  LAST_COLUMN,
};



static void terminal_preferences_dialog_finalize     (GObject                        *object);
static void terminal_preferences_dialog_response     (TerminalPreferencesDialog      *dialog,
                                                      gint                            response);
static void terminal_preferences_open_image_file     (GtkWidget                      *button,
                                                      GtkWidget                      *entry);
static void terminal_preferences_dialog_reset_compat (GtkWidget                      *button,
                                                      TerminalPreferencesDialog      *dialog);



G_DEFINE_TYPE (TerminalPreferencesDialog, terminal_preferences_dialog, GTK_TYPE_DIALOG)



static void
terminal_preferences_dialog_class_init (TerminalPreferencesDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_preferences_dialog_finalize;
}



/* Convert int -> boolean (i == 1) */
static gboolean
transform_is1 (const GValue *src_value,
               GValue       *dst_value,
               gpointer      user_data)
{
  g_value_set_boolean (dst_value, g_value_get_enum (src_value) == 1);
  return TRUE;
}



/* Convert int -> boolean (i == 1 || i == 2) */
static gboolean
transform_is1_or_is2 (const GValue *src_value,
                      GValue       *dst_value,
                      gpointer      user_data)
{
  gint i;

  i = g_value_get_enum (src_value);
  g_value_set_boolean (dst_value, (i == 1 || i == 2));

  return TRUE;
}



static void
terminal_preferences_dialog_init (TerminalPreferencesDialog *dialog)
{
  GtkListStore      *store;
  GtkTreeIter        iter;
  GdkPixbuf         *icon;
  GtkWidget         *button;
  GtkWidget         *entry;
  GtkWidget         *hbox;
  GtkWidget         *swin;
  GtkWidget         *box;
  GtkWidget         *frame;
  GtkWidget         *vbox;
  GtkWidget         *table;
  GtkWidget         *combo;
  GtkWidget         *label;
  GtkWidget         *image;
  GtkWidget         *ibox;
  GtkWidget         *align;
  GtkWidget         *editor;
  AtkRelationSet    *relations;
  AtkRelation       *relation;
  AtkObject         *object;
  GSList            *group;
  gchar             *name;
  gint               idx;
  gint               n;

  dialog->preferences = terminal_preferences_get ();

  /* initialize the dialog window */
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Terminal Preferences"));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (terminal_preferences_dialog_response), NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  swin = gtk_scrolled_window_new (NULL, NULL);
  g_object_set (G_OBJECT (swin),
                "hscrollbar-policy", GTK_POLICY_NEVER,
                "shadow-type", GTK_SHADOW_IN,
                "vscrollbar-policy", GTK_POLICY_NEVER,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), swin, FALSE, FALSE, 0);
  gtk_widget_show (swin);

  store = gtk_list_store_new (LAST_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);

  dialog->icon_bar = exo_icon_bar_new_with_model (GTK_TREE_MODEL (store));
  exo_icon_bar_set_pixbuf_column (EXO_ICON_BAR (dialog->icon_bar), PIXBUF_COLUMN);
  exo_icon_bar_set_text_column (EXO_ICON_BAR (dialog->icon_bar), TEXT_COLUMN);
  gtk_container_add (GTK_CONTAINER (swin), dialog->icon_bar);
  gtk_widget_show (dialog->icon_bar);

  dialog->notebook = gtk_notebook_new ();
  g_object_set (G_OBJECT (dialog->notebook),
                "show-border", FALSE,
                "show-tabs", FALSE,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), dialog->notebook, TRUE, TRUE, 0);
  gtk_widget_show (dialog->notebook);

  
  /*
     General
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  idx = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
  gtk_widget_show (box);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Title</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);
  
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = g_object_new (GTK_TYPE_LABEL,
                        "wrap", TRUE,
                        "label", _("The command running inside the terminal may dynamically set a new title."),
                        "xalign", 0.0,
                        NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Initial title:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "title-initial", G_OBJECT (entry), "text");
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_show (entry);

  /* set Atk label relation for the entry */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), entry);

  label = gtk_label_new_with_mnemonic (_("_Dynamically-set title:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (hbox);
  
  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Replaces initial title"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Goes before initial title"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Goes after initial title"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Isn't displayed"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "title-mode", G_OBJECT (combo), "active");
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* set Atk label relation for the combo */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Command</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  button = gtk_check_button_new_with_mnemonic (_("_Run command as login shell"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "command-login-shell", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Select this option to force Terminal to run your shell as a login shell "
                                     "when you open new terminals. See the documentation of your shell for "
                                     "details about differences between running it as interactive shell and "
                                     "running it as login shell."));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("_Update utmp/wtmp records when command is launched"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "command-update-records", G_OBJECT (button), "active");
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Scrolling</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  gtk_widget_show (table);

  button = gtk_check_button_new_with_mnemonic (_("Scroll single _line using Shift-Up/-Down keys"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-single-line", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Enable this option to be able to scroll by a single line "
                                     "using the up/down arrow keys together with the Shift key."));
  gtk_table_attach (GTK_TABLE (table), button, 0, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  button = gtk_check_button_new_with_mnemonic (_("Sc_roll on output"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-on-output", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("This option controls whether the terminal will scroll "
                                     "down automatically whenever new output is generated by "
                                     "the commands running inside the terminal."));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Scroll on _keystroke"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-on-keystroke", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Enables you to press any key on the "
                                     "keyboard to scroll down the terminal "
                                     "window to the command prompt."));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic (_("_Scrollbar is:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Disabled"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("On the left side"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("On the right side"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-bar", G_OBJECT (combo), "active");
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* set Atk label relation for the combo */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);

  label = gtk_label_new_with_mnemonic (_("Scr_ollback:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (48.0, 5 * 1024.0 * 1024.0, 1.0);
  gtk_entry_set_activates_default (GTK_ENTRY (button), TRUE);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-lines",
                          G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (button))), "value");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Specifies the number of lines that you can "
                                     "scroll back using the scrollbar."));
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  gtk_widget_show (button);

  /* set Atk label relation for the spin button */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), button);

  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 TERMINAL_STOCK_GENERAL,
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("General"),
                      INDEX_COLUMN, idx,
                      -1);
  g_object_unref (G_OBJECT (icon));


  /*
     Appearance
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  idx = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
  gtk_widget_show (box);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Font</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  button = g_object_new (GTK_TYPE_FONT_BUTTON, "title", _("Choose Terminal Font"), "use-font", TRUE, NULL);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "font-name", G_OBJECT (button), "font-name");
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

#if TERMINAL_HAS_ANTI_ALIAS_SETTING
  button = gtk_check_button_new_with_mnemonic (_("Enable anti-aliasing for the terminal font"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "font-anti-alias", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Enable this option if you want Terminal to use anti-aliasing when "
                                     "rendering text in terminal windows. Disabling this option can "
                                     "impressively speed up terminal rendering performance and reduce "
                                     "the overall system load on slow systems."));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
#endif

  button = gtk_check_button_new_with_mnemonic (_("Allow bold text"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "font-allow-bold", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Enable this option to allow applications running inside the "
                                     "terminal windows to use bold text."));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Background</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("None (use solid color)"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Background image"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Transparent background"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "background-mode", G_OBJECT (combo), "active");
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, TRUE, 0);
  gtk_widget_show (combo);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  gtk_widget_show (table);

  exo_binding_new_full (G_OBJECT (dialog->preferences), "background-mode", G_OBJECT (table), "visible",
                        transform_is1, NULL, NULL);

  label = gtk_label_new_with_mnemonic (_("_File:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "background-image-file", G_OBJECT (entry), "text");
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_show (entry);

  button = gtk_button_new ();
  image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (terminal_preferences_open_image_file), entry);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_widget_show (image);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic (_("_Style:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Tiled"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Centered"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Scaled"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Stretched"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "background-image-style", G_OBJECT (combo), "active");
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  ibox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), ibox, FALSE, TRUE, 0);
  gtk_widget_show (ibox);

  exo_binding_new_full (G_OBJECT (dialog->preferences), "background-mode", G_OBJECT (ibox), "visible",
                        transform_is1_or_is2, NULL, NULL);

  label = gtk_label_new (_("Shade transparent or image background:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (ibox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (ibox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  label = g_object_new (GTK_TYPE_ALIGNMENT, "width-request", 12, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<small><i>None</i></small>"), "use-markup", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  button = gtk_hscale_new_with_range (0.0, 1.0, 0.05);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "background-darkness",
                          G_OBJECT (gtk_range_get_adjustment (GTK_RANGE (button))), "value");
  gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<small><i>Maximum</i></small>"), "use-markup", TRUE, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Opening New Windows</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);
  
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  button = gtk_check_button_new_with_mnemonic (_("Display _menubar in new windows"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "misc-menubar-default", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Enable this option to show the menubar in newly "
                                     "created terminal windows."));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Display _toolbars in new windows"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "misc-toolbars-default", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Enable this option to show the toolbars in newly "
                                     "created terminal windows."));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Display _borders around new windows"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "misc-borders-default", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button,
                                   _("Enable this option to show window decorations around newly "
                                     "created terminal windows."));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 TERMINAL_STOCK_APPEARANCE,
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("Appearance"),
                      INDEX_COLUMN, idx,
                      -1);
  g_object_unref (icon);


  /*
     Colors
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  idx = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
  gtk_widget_show (box);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Foreground and Background</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", _("_Text and cursor color:"),
                        "use-underline", TRUE,
                        "xalign", 0.0,
                        NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (align);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (align), hbox);
  gtk_widget_show (hbox);

  button = g_object_new (GTK_TYPE_COLOR_BUTTON, "title", _("Choose terminal text color"), NULL);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-foreground", G_OBJECT (button), "color");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  gtk_widget_show (button);

  /* set Atk name/description and label relation for the button */
  object = gtk_widget_get_accessible (button);
  atk_object_set_name (object, _("Color Selector"));
  atk_object_set_description (object, _("Open a dialog to specify the color"));
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  button = g_object_new (GTK_TYPE_COLOR_BUTTON, "title", _("Choose terminal cursor color"), NULL);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-cursor", G_OBJECT (button), "color");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  gtk_widget_show (button);

  /* set Atk name/description and label relation for the button */
  object = gtk_widget_get_accessible (button);
  atk_object_set_name (object, _("Color Selector"));
  atk_object_set_description (object, _("Open a dialog to specify the color"));
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", _("_Background color:"),
                        "use-underline", TRUE,
                        "xalign", 0.0,
                        NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (align);

  button = g_object_new (GTK_TYPE_COLOR_BUTTON,
                         "title", _("Choose terminal background color"),
                         NULL);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-background", G_OBJECT (button), "color");
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  gtk_widget_show (button);

  /* set Atk name/description and label relation for the button */
  object = gtk_widget_get_accessible (button);
  atk_object_set_name (object, _("Color Selector"));
  atk_object_set_description (object, _("Open a dialog to specify the color"));
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Text Selection</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  button = gtk_radio_button_new_with_mnemonic (NULL, _("Use _default color"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-selection-use-default", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button, _("Use the default text selection background color"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_mnemonic (group, _("Use _custom color"));
  exo_mutual_binding_new_with_negation (G_OBJECT (dialog->preferences), "color-selection-use-default", G_OBJECT (button), "active");
  terminal_gtk_widget_set_tooltip (button, _("Use a custom text selection background color"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (align);

  button = g_object_new (GTK_TYPE_COLOR_BUTTON, "title", _("Choose terminal text selection background color"), NULL);
  exo_binding_new_with_negation (G_OBJECT (dialog->preferences), "color-selection-use-default", G_OBJECT (button), "sensitive");
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-selection", G_OBJECT (button), "color");
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_widget_show (button);

  /* set Atk name/description and label relation for the button */
  object = gtk_widget_get_accessible (button);
  atk_object_set_name (object, _("Color Selector"));
  atk_object_set_description (object, _("Open a dialog to specify the color"));
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Palette</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", _("Terminal applications have this color palette available to them:"),
                        "wrap", TRUE,
                        "xalign", 0.0f,
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  table = gtk_table_new (3, 8, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 0);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  gtk_widget_show (table);

  /* add the color buttons in 2 rows, with 8 buttons per row */
  for (n = 0; n < 16; ++n)
    {
      /* setup and add the button */
      button = gtk_color_button_new ();
      gtk_table_attach (GTK_TABLE (table), button, (n % 8), (n % 8) + 1, (n / 8), (n / 8) + 1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (button);

      /* setup a tooltip */
      terminal_gtk_widget_set_tooltip (button, _("Palette entry %d"), (n + 1));

      /* sync with the appropriate preference */
      name = g_strdup_printf ("color-palette%d", (n + 1));
      exo_mutual_binding_new (G_OBJECT (dialog->preferences), name, G_OBJECT (button), "color");
      g_free (name);
    }

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Tab activity</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  table = gtk_table_new (2, 1, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", _("T_ab activity color:"),
                        "use-underline", TRUE,
                        "xalign", 0.0,
                        NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (align);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (align), hbox);
  gtk_widget_show (hbox);

  button = g_object_new (GTK_TYPE_COLOR_BUTTON, "title", _("Choose tab activity color"), NULL);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "tab-activity-color", G_OBJECT (button), "color");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  gtk_widget_show (button);

  /* set Atk name/description and label relation for the button */
  object = gtk_widget_get_accessible (button);
  atk_object_set_name (object, _("Color Selector"));
  atk_object_set_description (object, _("Open a dialog to specify the color"));
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 TERMINAL_STOCK_COLORS,
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("Colors"),
                      INDEX_COLUMN, idx,
                      -1);
  g_object_unref (G_OBJECT (icon));


  /*
     Shortcuts
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  idx = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
  gtk_widget_show (box);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Shortcut keys</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  ibox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ibox),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (ibox),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), ibox, TRUE, TRUE, 0);
  gtk_widget_show (ibox);

  editor = terminal_shortcut_editor_new ();
  gtk_container_add (GTK_CONTAINER (ibox), editor);
  gtk_widget_show (editor);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Menubar access</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  button = gtk_check_button_new_with_mnemonic (_("Disable all me_nu access keys (such as Alt+f)"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "shortcuts-no-mnemonics", G_OBJECT (button), "active");
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Disable m_enu shortcut key (F10 by default)"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "shortcuts-no-menukey", G_OBJECT (button), "active");
  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 TERMINAL_STOCK_SHORTCUTS,
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("Shortcuts"),
                      INDEX_COLUMN, idx,
                      -1);
  g_object_unref (icon);


  /*
     Advanced
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  idx = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
  gtk_widget_show (box);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Compatibility</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  label = gtk_label_new (_("These options may cause some applications to behave incorrectly. They are only "
                           "here to allow you to work around certain applications and operating systems "
                           "that expect different terminal behavior."));
  g_object_set (G_OBJECT (label), "wrap", TRUE, "xalign", 0.0, NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 3, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Backspace key generates:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Auto-detect"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("ASCII DEL"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Escape sequence"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Control-H"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "binding-backspace", G_OBJECT (combo), "active");
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* set Atk label relation for the combo */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);

  label = gtk_label_new_with_mnemonic (_("_Delete key generates:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Auto-detect"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("ASCII DEL"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Escape sequence"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Control-H"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "binding-delete", G_OBJECT (combo), "active");
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_widget_show (combo);

  /* set Atk label relation for the combo */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), combo);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<tt>$TERM</tt> setting:"), "use-markup", TRUE, "xalign", 0.0f, NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "term", G_OBJECT (entry), "text");
  terminal_gtk_widget_set_tooltip (entry,
                                   _("This specifies the value the $TERM environment variable is set "
                                     "to, when a new terminal tab or terminal window is opened. The default "
                                     "should be ok for most systems. If you have problems with colors in "
                                     "some applications, try xterm-color here."));
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_show (entry);

  /* set Atk label relation for the entry */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), entry);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 2, 4, 5,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Reset compatibility options to defaults"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (terminal_preferences_dialog_reset_compat), dialog);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Double click</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Consider the following characters part of a word when double clicking:"));
  g_object_set (G_OBJECT (label), "wrap", TRUE, "xalign", 0.0, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "word-chars", G_OBJECT (entry), "text");
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_show (entry);

  /* set Atk label relation for the entry */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), entry);

/* start */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Tab activity indicator</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Reset tab activity indicator after"));
  g_object_set (G_OBJECT (label), "wrap", TRUE, "xalign", 0.0, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (0.0, 30.0, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(button), TRUE);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "tab-activity-timeout", G_OBJECT (button), "value");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
  gtk_widget_show (button);

  /* set Atk label relation for the button */
  terminal_gtk_label_set_a11y_relation (GTK_LABEL (label), button);

  label = gtk_label_new (_("seconds"));
  g_object_set (G_OBJECT (label), "wrap", TRUE, "xalign", 0.0, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

/*end */
  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 TERMINAL_STOCK_ADVANCED,
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("Advanced"),
                      INDEX_COLUMN, idx,
                      -1);
  g_object_unref (icon);

  exo_mutual_binding_new (G_OBJECT (dialog->notebook), "page", G_OBJECT (dialog->icon_bar), "active");

  g_object_unref (G_OBJECT (store));
}



static void
terminal_preferences_dialog_finalize (GObject *object)
{
  TerminalPreferencesDialog *dialog = TERMINAL_PREFERENCES_DIALOG (object);

  /* release the preferences */
  g_object_unref (G_OBJECT (dialog->preferences));

  (*G_OBJECT_CLASS (terminal_preferences_dialog_parent_class)->finalize) (object);
}



static void
terminal_preferences_dialog_response (TerminalPreferencesDialog *dialog,
                                      gint                       response)
{
  /* check if we should open the user manual */
  if (G_UNLIKELY (response == GTK_RESPONSE_HELP))
    {
      /* open the "Preferences" section of the user manual */
      terminal_dialogs_show_help (GTK_WIDGET (dialog), "preferences", NULL);
    }
  else
    {
      /* close the preferences dialog */
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}



static void
terminal_preferences_open_image_file (GtkWidget *button,
                                      GtkWidget *entry)
{
  GtkFileFilter *filter;
  GtkWidget     *toplevel;
  GtkWidget     *dialog;
  gchar         *filename;

  /* determine the toplevel window */
  toplevel = gtk_widget_get_toplevel (entry);
  if (toplevel == NULL || !GTK_WIDGET_TOPLEVEL (toplevel))
    return;

  /* allocate the file chooser window */
  dialog = gtk_file_chooser_dialog_new (_("Select background image file"),
                                        GTK_WINDOW (toplevel),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, GTK_RESPONSE_CANCEL, -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  exo_gtk_file_chooser_add_thumbnail_preview (GTK_FILE_CHOOSER (dialog));
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

  /* add "All Files" filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All Files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  /* add "Image Files" filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Image Files"));
  gtk_file_filter_add_pixbuf_formats (filter);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  /* preselect the previous filename */
  filename = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  if (G_LIKELY (filename != NULL))
    {
      if (*filename != '\0')
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), filename);
      g_free (filename);
    }

  /* run the chooser dialog */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      gtk_entry_set_text (GTK_ENTRY (entry), filename);
      g_free (filename);
    }

  /* destroy the dialog */
  gtk_widget_destroy (dialog);
}



static void
terminal_preferences_dialog_reset_compat (GtkWidget                 *button,
                                          TerminalPreferencesDialog *dialog)
{
  GParamSpec *spec;
  GValue      value = {0, };

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences),
                                       "binding-backspace");
  if (G_LIKELY (spec != NULL))
    {
      g_value_init (&value, spec->value_type);
      g_param_value_set_default (spec, &value); 
      g_object_set_property (G_OBJECT (dialog->preferences), "binding-backspace", &value);
      g_value_unset (&value);
    }

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences),
                                       "binding-delete");
  if (G_LIKELY (spec != NULL))
    {
      g_value_init (&value, spec->value_type);
      g_param_value_set_default (spec, &value); 
      g_object_set_property (G_OBJECT (dialog->preferences), "binding-delete", &value);
      g_value_unset (&value);
    }

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences),
                                       "term");
  if (G_LIKELY (spec != NULL))
    {
      g_value_init (&value, spec->value_type);
      g_param_value_set_default (spec, &value); 
      g_object_set_property (G_OBJECT (dialog->preferences), "term", &value);
      g_value_unset (&value);
    }
}



/**
 * terminal_preferences_dialog_new:
 * @parent      : A #GtkWindow or %NULL.
 *
 * Return value :
 **/
GtkWidget*
terminal_preferences_dialog_new (GtkWindow *parent)
{
  GtkWidget *dialog;
  
  dialog = g_object_new (TERMINAL_TYPE_PREFERENCES_DIALOG, NULL);
  if (parent != NULL)
    gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

  return dialog;
}
