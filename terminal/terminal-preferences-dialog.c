/* $Id$ */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-shortcut-editor.h>



enum
{
  PIXBUF_COLUMN,
  TEXT_COLUMN,
  INDEX_COLUMN,
  LAST_COLUMN,
};



static void     terminal_preferences_dialog_class_init              (TerminalPreferencesDialogClass *klass);
static void     terminal_preferences_dialog_init                    (TerminalPreferencesDialog      *dialog);
static void     terminal_preferences_dialog_finalize                (GObject                        *object);
static void     terminal_preferences_dialog_response                (TerminalPreferencesDialog      *dialog,
                                                                     gint                            response);
static void     terminal_preferences_open_image_file                (GtkWidget                      *button,
                                                                     GtkWidget                      *entry);
static void     terminal_preferences_dialog_reset_compat            (GtkWidget                      *button,
                                                                     TerminalPreferencesDialog      *dialog);



static GObjectClass *parent_class;



G_DEFINE_TYPE (TerminalPreferencesDialog, terminal_preferences_dialog, GTK_TYPE_DIALOG);



static void
terminal_preferences_dialog_class_init (TerminalPreferencesDialogClass *klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

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
  GParamSpec        *pspec;
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
  gint               index;

  dialog->preferences = terminal_preferences_get ();

  dialog->tooltips = gtk_tooltips_new ();
  g_object_ref (G_OBJECT (dialog->tooltips));
  gtk_object_sink (GTK_OBJECT (dialog->tooltips));

  g_object_set (G_OBJECT (dialog),
                "title", _("Terminal Preferences"),
                "destroy-with-parent", TRUE,
                "has-separator", FALSE,
                "resizable", FALSE,
                NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_CLOSE);
  gtk_widget_show (button);

  button = gtk_button_new_from_stock (GTK_STOCK_HELP);
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_HELP);
  gtk_widget_show (button);

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
  index = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
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
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Initial title:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "title-initial", G_OBJECT (entry), "text");
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (entry);

  /* set Atk label relation for the entry */
  object = gtk_widget_get_accessible (entry);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  label = gtk_label_new_with_mnemonic (_("_Dynamically-set title:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (hbox);
  
  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Replaces initial title"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Goes before initial title"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Goes after initial title"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Isn't displayed"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "title-mode", G_OBJECT (combo), "active");
  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (g_object_notify), "active");
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
  gtk_widget_show (combo);

  /* set Atk label relation for the combo */
  object = gtk_widget_get_accessible (combo);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

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

  /* FIXME ?! */
#if 0
  button = gtk_check_button_new_with_mnemonic (_("Terminal _bell"));
  proxy = terminal_preferences_get_proxy (dialog->preferences, "misc-bell");
  exo_property_proxy_add (proxy, G_OBJECT (button), "active", NULL, NULL, NULL);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 2, 3,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (button);
#endif

  /* FIXME ?! */
#if 0
  button = gtk_check_button_new_with_mnemonic (_("Cursor blin_ks"));
  proxy = terminal_preferences_get_proxy (dialog->preferences, "misc-cursor-blinks");
  exo_property_proxy_add (proxy, G_OBJECT (button), "active", NULL, NULL, NULL);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 3, 4,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (button);
#endif

  button = gtk_check_button_new_with_mnemonic (_("_Run command as login shell"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "command-login-shell", G_OBJECT (button), "active");
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

  button = gtk_check_button_new_with_mnemonic (_("Sc_roll on output"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-on-output", G_OBJECT (button), "active");
  gtk_table_attach (GTK_TABLE (table), button, 0, 2, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Scroll on key_stroke"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-on-keystroke", G_OBJECT (button), "active");
  gtk_tooltips_set_tip (dialog->tooltips, button,
                        _("Enables you to press any key on the "
                          "keyboard to scroll down the terminal "
                          "window to the command prompt."),
                        NULL);
  gtk_table_attach (GTK_TABLE (table), button, 0, 2, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic (_("_Scrollbar is:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Disabled"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("On the left side"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("On the right side"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-bar", G_OBJECT (combo), "active");
  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (g_object_notify), "active");
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  /* set Atk label relation for the combo */
  object = gtk_widget_get_accessible (combo);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  label = gtk_label_new_with_mnemonic (_("Scr_ollback:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  button = gtk_spin_button_new_with_range (48.0, 5 * 1024.0 * 1024.0, 1.0);
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "scrolling-lines",
                          G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (button))), "value");
  gtk_tooltips_set_tip (dialog->tooltips, button,
                        _("Specifies the number of lines that you can "
                          "scroll back using the scrollbar."),
                        NULL);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  /* set Atk label relation for the spin button */
  object = gtk_widget_get_accessible (button);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 "terminal-general",
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("General"),
                      INDEX_COLUMN, index,
                      -1);
  g_object_unref (G_OBJECT (icon));


  /*
     Appearance
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  index = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
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

  button = gtk_check_button_new_with_mnemonic (_("Enable anti-aliasing for the terminal font"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "font-anti-alias", G_OBJECT (button), "active");
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
  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (g_object_notify), "active");
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
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  entry = gtk_entry_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "background-image-file", G_OBJECT (entry), "text");
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
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
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Tiled"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Centered"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Scaled"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Stretched"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "background-image-style", G_OBJECT (combo), "active");
  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (g_object_notify), "active");
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
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
  gtk_tooltips_set_tip (dialog->tooltips, button,
                        _("Enable to show the menubar in newly "
                          "created terminal windows."), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Display _toolbars in new windows"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "misc-toolbars-default", G_OBJECT (button), "active");
  gtk_tooltips_set_tip (dialog->tooltips, button,
                        _("Enable to show the toolbars in newly "
                          "created terminal windows."), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Display _borders around new windows"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "misc-borders-default", G_OBJECT (button), "active");
  gtk_tooltips_set_tip (dialog->tooltips, button,
                        _("Enable to show window decorations around newly "
                          "created terminal windows."), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 "terminal-appearance",
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("Appearance"),
                      INDEX_COLUMN, index,
                      -1);
  g_object_unref (icon);


  /*
     Colors
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  index = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
  gtk_widget_show (box);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<b>Foreground and Background</b>"), "use-markup", TRUE, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  button = gtk_check_button_new_with_mnemonic (_("Use colors from s_ystem theme"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-system-theme", G_OBJECT (button), "active");
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  gtk_widget_show (table);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", _("_Text color:"),
                        "use-underline", TRUE,
                        "xalign", 0.0,
                        NULL);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (align);

  button = g_object_new (GTK_TYPE_COLOR_BUTTON,
                         "title", _("Choose terminal text color"),
                         NULL);
  exo_binding_new_with_negation (G_OBJECT (dialog->preferences), "color-system-theme", G_OBJECT (button), "sensitive");
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-foreground", G_OBJECT (button), "color");
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_widget_show (button);

  /* set Atk name/description and label relation for the button */
  g_object_set (G_OBJECT (label), "mnemonic-widget", G_OBJECT (button), NULL);
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
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (align);

  button = g_object_new (GTK_TYPE_COLOR_BUTTON,
                         "title", _("Choose terminal background color"),
                         NULL);
  exo_binding_new_with_negation (G_OBJECT (dialog->preferences), "color-system-theme", G_OBJECT (button), "sensitive");
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-background", G_OBJECT (button), "color");
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_widget_show (button);

  /* set Atk name/description and label relation for the button */
  g_object_set (G_OBJECT (label), "mnemonic-widget", G_OBJECT (button), NULL);
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
                        NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  table = gtk_table_new (3, 8, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 0);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  gtk_widget_show (table);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette1", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette1");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette2", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette2");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette3", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette3");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette4", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette4");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 3, 4, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette5", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette5");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 4, 5, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette6", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette6");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 5, 6, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette7", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette7");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 6, 7, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette8", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette8");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 7, 8, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette9", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette9");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette10", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette10");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette11", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette11");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette12", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette12");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 3, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette13", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette13");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 4, 5, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette14", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette14");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 5, 6, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette15", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette15");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 6, 7, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gtk_color_button_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "color-palette16", G_OBJECT (button), "color");
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "color-palette16");
  gtk_tooltips_set_tip (dialog->tooltips, button, g_param_spec_get_blurb (pspec), NULL);
  gtk_table_attach (GTK_TABLE (table), button, 7, 8, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);

  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 "terminal-colors",
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("Colors"),
                      INDEX_COLUMN, index,
                      -1);
  g_object_unref (G_OBJECT (icon));


  /*
     Shortcuts
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  index = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
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
                                 "terminal-shortcuts",
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("Shortcuts"),
                      INDEX_COLUMN, index,
                      -1);
  g_object_unref (icon);


  /*
     Advanced
   */
  box = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  index = gtk_notebook_append_page (GTK_NOTEBOOK (dialog->notebook), box, NULL);
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
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("ASCII DEL"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Escape sequence"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Control-H"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "binding-backspace", G_OBJECT (combo), "active");
  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (g_object_notify), "active");
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  /* set Atk label relation for the combo */
  object = gtk_widget_get_accessible (combo);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  label = gtk_label_new_with_mnemonic (_("_Delete key generates:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("ASCII DEL"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Escape sequence"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Control-H"));
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "binding-delete", G_OBJECT (combo), "active");
  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (g_object_notify), "active");
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  /* set Atk label relation for the combo */
  object = gtk_widget_get_accessible (combo);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  label = g_object_new (GTK_TYPE_LABEL, "label", _("<tt>$TERM</tt> setting:"), "use-markup", TRUE, NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "term", G_OBJECT (entry), "text");
  gtk_tooltips_set_tip (dialog->tooltips, entry, _("This specifies the value the $TERM environment variable is set "
                                                   "to, when a new terminal tab or terminal window is opened. The default "
                                                   "should be ok for most systems. If you have problems with colors in "
                                                   "some applications, try xterm-color here."), NULL);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (entry);

  /* set Atk label relation for the entry */
  object = gtk_widget_get_accessible (entry);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

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
  exo_mutual_binding_new (G_OBJECT (dialog->preferences), "word-chars", G_OBJECT (entry), "text");
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  /* set Atk label relation for the entry */
  object = gtk_widget_get_accessible (entry);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  icon = gtk_widget_render_icon (GTK_WIDGET (dialog->icon_bar),
                                 "terminal-advanced",
                                 GTK_ICON_SIZE_DIALOG,
                                 NULL);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      PIXBUF_COLUMN, icon,
                      TEXT_COLUMN, _("Advanced"),
                      INDEX_COLUMN, index,
                      -1);
  g_object_unref (icon);

  exo_mutual_binding_new (G_OBJECT (dialog->notebook), "page", G_OBJECT (dialog->icon_bar), "active");

  g_object_unref (G_OBJECT (store));
}



static void
terminal_preferences_dialog_finalize (GObject *object)
{
  TerminalPreferencesDialog *dialog = TERMINAL_PREFERENCES_DIALOG (object);
  g_object_unref (G_OBJECT (dialog->preferences));
  g_object_unref (G_OBJECT (dialog->tooltips));
  G_OBJECT_CLASS (parent_class)->finalize (object);
}



static void
terminal_preferences_dialog_response (TerminalPreferencesDialog      *dialog,
                                      gint                            response)
{
  GtkWidget *message;
  GError    *error = NULL;
  gchar     *command;

  if (response == GTK_RESPONSE_HELP)
    {
      command = g_strconcat (TERMINAL_HELP_BIN, " preferences", NULL);
      if (!g_spawn_command_line_async (command, &error))
        {
          message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                            GTK_DIALOG_DESTROY_WITH_PARENT
                                            | GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            _("Unable to launch online help: %s"),
                                            error->message);
          g_signal_connect (G_OBJECT (message), "response",
                            G_CALLBACK (gtk_widget_destroy), NULL);
          gtk_widget_show (message);

          g_error_free (error);
        }
      g_free (command);
    }
  else
    {
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}



static void
update_preview (GtkFileChooser *chooser,
                GtkWidget      *image)
{
  GdkPixbuf *pixbuf;
  GdkPixbuf *temp;
  gchar     *filename;
  gchar     *basename;
  gchar     *name;
  gint       height;
  gint       width;

  filename = gtk_file_chooser_get_filename (chooser);

  if (G_UNLIKELY (filename == NULL))
    {
      gtk_image_set_from_stock (GTK_IMAGE (image),
                                GTK_STOCK_MISSING_IMAGE,
                                GTK_ICON_SIZE_DIALOG);
      gtk_frame_set_label (GTK_FRAME (image->parent), NULL);
      gtk_widget_set_sensitive (image->parent, FALSE);
      return;
    }
  else
    {
      basename = g_path_get_basename (filename);
      name = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
      gtk_frame_set_label (GTK_FRAME (image->parent), name);
      g_free (basename);
      g_free (name);
    }

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  if (G_LIKELY (pixbuf != NULL))
    {
      g_object_get (G_OBJECT (pixbuf),
                    "height", &height,
                    "width", &width,
                    NULL);

      if (width > 256 || height > 256)
        {
          temp = exo_gdk_pixbuf_scale_ratio (pixbuf, 256);
          g_object_unref (G_OBJECT (pixbuf));
          pixbuf = temp;
        }

      gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
      gtk_widget_set_sensitive (image->parent, TRUE);
      g_object_unref (G_OBJECT (pixbuf));
    }
  else
    {
      gtk_image_set_from_stock (GTK_IMAGE (image),
                                GTK_STOCK_MISSING_IMAGE,
                                GTK_ICON_SIZE_DIALOG);
      gtk_widget_set_sensitive (image->parent, FALSE);
    }

  g_free (filename);
}



static void
terminal_preferences_open_image_file (GtkWidget *button,
                                      GtkWidget *entry)
{
  GtkFileFilter *filter;
  GtkWidget     *toplevel;
  GtkWidget     *frame;
  GtkWidget     *preview;
  GtkWidget     *dialog;
  gchar        **extensions;
  gchar        **mime_types;
  gchar         *filename;
  gchar         *pattern;
  GSList        *formats;
  GSList        *lp;
  guint          n;

  toplevel = gtk_widget_get_toplevel (entry);
  dialog = gtk_file_chooser_dialog_new (_("Select background image file"),
                                        GTK_WINDOW (toplevel),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);

  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_set_use_preview_label (GTK_FILE_CHOOSER (dialog), FALSE);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All Files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Image Files"));
  formats = gdk_pixbuf_get_formats ();
  for (lp = formats; lp != NULL; lp = lp->next)
    {
      extensions = gdk_pixbuf_format_get_extensions (lp->data);
      for (n = 0; extensions[n] != NULL; ++n)
        {
          pattern = g_strconcat ("*.", extensions[n], NULL);
          gtk_file_filter_add_pattern (filter, pattern);
          g_free (pattern);
        }
      g_strfreev (extensions);

      mime_types = gdk_pixbuf_format_get_mime_types (lp->data);
      for (n = 0; mime_types[n] != NULL; ++n)
        gtk_file_filter_add_mime_type (filter, mime_types[n]);
      g_strfreev (mime_types);
    }
  g_slist_free (formats);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  frame = g_object_new (GTK_TYPE_FRAME,
                        "sensitive", FALSE,
                        "shadow-type", GTK_SHADOW_IN,
                        NULL);
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dialog), frame);
  gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_widget_show (frame);

  preview = g_object_new (GTK_TYPE_IMAGE,
                          "height-request", 256 + 2 * 12,
                          "width-request", 256 + 2 * 12,
                          NULL);
  g_signal_connect (G_OBJECT (dialog), "update-preview",
                    G_CALLBACK (update_preview), preview);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  filename = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  if (G_LIKELY (filename != NULL))
    {
      if (*filename != '\0')
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), filename);
      g_free (filename);
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      gtk_entry_set_text (GTK_ENTRY (entry), filename);
      g_free (filename);
    }

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
