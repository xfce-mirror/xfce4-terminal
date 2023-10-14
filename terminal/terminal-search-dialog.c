/*-
 * Copyright 2012 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

#include <terminal/terminal-search-dialog.h>
#include <terminal/terminal-preferences.h>



static void terminal_search_dialog_finalize           (GObject              *object);
static void terminal_search_dialog_opacity_changed    (TerminalSearchDialog *dialog);
static void terminal_search_dialog_clear_gregex       (TerminalSearchDialog *dialog);
static void terminal_search_dialog_entry_icon_release (GtkWidget            *entry,
                                                       GtkEntryIconPosition  icon_pos);
static void terminal_search_dialog_entry_changed      (GtkWidget            *entry,
                                                       TerminalSearchDialog *dialog);


struct _TerminalSearchDialogClass
{
  XfceTitledDialogClass parent_class;
};

struct _TerminalSearchDialog
{
  XfceTitledDialog parent_instance;

  VteRegex        *last_gregex;

  GtkWidget     *button_prev;
  GtkWidget     *button_next;

  GtkWidget     *entry;

  GtkWidget     *match_case;
  GtkWidget     *match_regex;
  GtkWidget     *match_word;
  GtkWidget     *wrap_around;

  GtkAdjustment *opacity_adjustment;
};



G_DEFINE_TYPE (TerminalSearchDialog, terminal_search_dialog, XFCE_TYPE_TITLED_DIALOG)



static void
terminal_search_dialog_class_init (TerminalSearchDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_search_dialog_finalize;
}



static void
terminal_search_dialog_init (TerminalSearchDialog *dialog)
{
  TerminalPreferences *preferences;
  GtkWidget     *close_button;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *label;
  GtkWidget     *opacity_box;
  GtkWidget     *opacity_scale;
  GtkWidget     *opacity_label;
  GtkWidget     *percent_label;
  GdkScreen     *screen = gtk_widget_get_screen (GTK_WIDGET (dialog));
  GtkAccelGroup *group = gtk_accel_group_new ();
  GtkAccelKey    key_prev = {0}, key_next = {0};

  gtk_window_set_title (GTK_WINDOW (dialog), _("Find"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 400, -1);
  gtk_window_add_accel_group (GTK_WINDOW (dialog), group);

#if !LIBXFCE4UI_CHECK_VERSION (4, 19, 3)
  xfce_titled_dialog_create_action_area (XFCE_TITLED_DIALOG (dialog));
#endif

  close_button = xfce_gtk_button_new_mixed ("window-close", _("_Close"));
  xfce_titled_dialog_add_action_widget (XFCE_TITLED_DIALOG (dialog), close_button, GTK_RESPONSE_CLOSE);

  dialog->button_prev = xfce_gtk_button_new_mixed ("go-previous", _("_Previous"));
  xfce_titled_dialog_add_action_widget (XFCE_TITLED_DIALOG (dialog), dialog->button_prev, TERMINAL_RESPONSE_SEARCH_PREV);
  gtk_widget_set_can_default (dialog->button_prev, TRUE);
  gtk_widget_grab_default (dialog->button_prev);

  dialog->button_next = xfce_gtk_button_new_mixed ("go-next", _("_Next"));
  xfce_titled_dialog_add_action_widget (XFCE_TITLED_DIALOG (dialog), dialog->button_next, TERMINAL_RESPONSE_SEARCH_NEXT);

  gtk_accel_map_lookup_entry ("<Actions>/terminal-window/search-prev", &key_prev);
  if (key_prev.accel_key != 0)
    gtk_widget_add_accelerator (dialog->button_prev, "activate", group, key_prev.accel_key, key_prev.accel_mods, key_prev.accel_flags);
  gtk_accel_map_lookup_entry ("<Actions>/terminal-window/search-next", &key_next);
  if (key_next.accel_key != 0)
    gtk_widget_add_accelerator (dialog->button_next, "activate", group, key_next.accel_key, key_next.accel_mods, key_next.accel_flags);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), vbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  label = gtk_label_new_with_mnemonic (_("_Search for:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  dialog->entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), dialog->entry, TRUE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->entry);
  gtk_entry_set_activates_default (GTK_ENTRY (dialog->entry), TRUE);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->entry), GTK_ENTRY_ICON_SECONDARY, "edit-clear");
  g_signal_connect (G_OBJECT (dialog->entry), "icon-release",
      G_CALLBACK (terminal_search_dialog_entry_icon_release), NULL);
  g_signal_connect (G_OBJECT (dialog->entry), "changed",
      G_CALLBACK (terminal_search_dialog_entry_changed), dialog);

  dialog->match_case = gtk_check_button_new_with_mnemonic (_("C_ase sensitive"));
  gtk_box_pack_start (GTK_BOX (vbox), dialog->match_case, FALSE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (dialog->match_case), "toggled",
      G_CALLBACK (terminal_search_dialog_clear_gregex), dialog);

  dialog->match_regex = gtk_check_button_new_with_mnemonic (_("Match as _regular expression"));
  gtk_box_pack_start (GTK_BOX (vbox), dialog->match_regex, FALSE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (dialog->match_regex), "toggled",
      G_CALLBACK (terminal_search_dialog_clear_gregex), dialog);

  dialog->match_word = gtk_check_button_new_with_mnemonic (_("Match _entire word only"));
  gtk_box_pack_start (GTK_BOX (vbox), dialog->match_word, FALSE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (dialog->match_word), "toggled",
      G_CALLBACK (terminal_search_dialog_clear_gregex), dialog);

  dialog->wrap_around = gtk_check_button_new_with_mnemonic (_("_Wrap around"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->wrap_around), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->wrap_around, FALSE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (dialog->wrap_around), "toggled",
      G_CALLBACK (terminal_search_dialog_clear_gregex), dialog);

  opacity_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start (opacity_box, 6);

  opacity_label = gtk_label_new_with_mnemonic (_("_Opacity:"));
  gtk_box_pack_start (GTK_BOX (opacity_box), opacity_label, FALSE, FALSE, 0);

  opacity_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 10.0, 100.0, 1.0);
  gtk_scale_set_value_pos (GTK_SCALE (opacity_scale), GTK_POS_RIGHT);
  gtk_box_pack_start (GTK_BOX (opacity_box), opacity_scale, TRUE, TRUE, 0);
  /* don't want text entry to lose focus when user changes opacity */
  gtk_widget_set_can_focus (opacity_scale, FALSE);

  percent_label = gtk_label_new ("%");
  gtk_box_pack_start (GTK_BOX (opacity_box), percent_label, FALSE, FALSE, 0);

  /* connect opacity properties */
  dialog->opacity_adjustment = gtk_range_get_adjustment (GTK_RANGE (opacity_scale));
  preferences = terminal_preferences_get ();
  g_object_bind_property (G_OBJECT (preferences), "misc-search-dialog-opacity",
                          G_OBJECT (dialog->opacity_adjustment), "value",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_signal_connect_swapped (G_OBJECT (dialog->opacity_adjustment), "value-changed",
                            G_CALLBACK (terminal_search_dialog_opacity_changed), dialog);
  g_object_unref (preferences);

  /* don't show opacity controls if compositing is not enabled */
  if (gdk_screen_is_composited (screen))
    gtk_box_pack_start (GTK_BOX (vbox), opacity_box, FALSE, TRUE, 0);

  terminal_search_dialog_entry_changed (dialog->entry, dialog);
}



static void
terminal_search_dialog_finalize (GObject *object)
{
  terminal_search_dialog_clear_gregex (TERMINAL_SEARCH_DIALOG (object));

  (*G_OBJECT_CLASS (terminal_search_dialog_parent_class)->finalize) (object);
}



static void
terminal_search_dialog_opacity_changed (TerminalSearchDialog *dialog)
{
  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (dialog));
  gdouble    opacity = gdk_screen_is_composited (screen)
                           ? gtk_adjustment_get_value (dialog->opacity_adjustment) / 100.0
                           : 1.0;
  gtk_widget_set_opacity (GTK_WIDGET (dialog), opacity);
}



static void
terminal_search_dialog_clear_gregex (TerminalSearchDialog *dialog)
{
  if (dialog->last_gregex != NULL)
    {
      vte_regex_unref (dialog->last_gregex);
      dialog->last_gregex = NULL;
    }
}



static void
terminal_search_dialog_entry_icon_release (GtkWidget            *entry,
                                           GtkEntryIconPosition  icon_pos)
{
  if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
    gtk_entry_set_text (GTK_ENTRY (entry), "");
}



static void
terminal_search_dialog_entry_changed (GtkWidget            *entry,
                                      TerminalSearchDialog *dialog)
{
  const gchar *text;
  gboolean     has_text;

  text = gtk_entry_get_text (GTK_ENTRY (dialog->entry));
  has_text = IS_STRING (text);

  terminal_search_dialog_clear_gregex (dialog);

  gtk_widget_set_sensitive (dialog->button_prev, has_text);
  gtk_widget_set_sensitive (dialog->button_next, has_text);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
    has_text ? TERMINAL_RESPONSE_SEARCH_PREV : GTK_RESPONSE_CLOSE);
}



GtkWidget *
terminal_search_dialog_new (GtkWindow *parent)
{
  return g_object_new (TERMINAL_TYPE_SEARCH_DIALOG,
                       "transient-for", parent,
                       "destroy-with-parent", TRUE,
                       NULL);
}



gboolean
terminal_search_dialog_get_wrap_around (TerminalSearchDialog *dialog)
{
  terminal_return_val_if_fail (TERMINAL_IS_SEARCH_DIALOG (dialog), FALSE);
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->wrap_around));
}



VteRegex *
terminal_search_dialog_get_regex (TerminalSearchDialog  *dialog,
                                  GError               **error)
{
  const gchar        *pattern;
  guint32             flags = PCRE2_UTF | PCRE2_NO_UTF_CHECK | PCRE2_MULTILINE;
  gchar              *pattern_escaped = NULL;
  gchar              *word_regex = NULL;
  VteRegex           *regex;

  terminal_return_val_if_fail (TERMINAL_IS_SEARCH_DIALOG (dialog), NULL);
  terminal_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* if not cleared, use the old regex */
  if (dialog->last_gregex != NULL)
    return vte_regex_ref (dialog->last_gregex);

  /* unset if no pattern is typed */
  pattern = gtk_entry_get_text (GTK_ENTRY (dialog->entry));
  if (!IS_STRING (pattern))
    return NULL;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_case)))
    flags |= PCRE2_CASELESS;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_regex)))
    {
/* MULTILINE flag is always used for pcre2 */
      flags |= G_REGEX_MULTILINE;
    }
  else
    {
      pattern_escaped = g_regex_escape_string (pattern, -1);
      pattern = pattern_escaped;
    }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_word)))
    {
      word_regex = g_strdup_printf ("\\b%s\\b", pattern);
      pattern = word_regex;
    }

  regex = vte_regex_new_for_search (pattern, -1, flags, error);

  g_free (pattern_escaped);
  g_free (word_regex);

  /* keep around */
  if (regex != NULL)
    dialog->last_gregex = vte_regex_ref (regex);

  return regex;
}



void
terminal_search_dialog_present (TerminalSearchDialog *dialog)
{
  terminal_return_if_fail (TERMINAL_IS_SEARCH_DIALOG (dialog));

  gtk_widget_show_all (GTK_WIDGET (dialog));

  /* Manjaro's version of GTK doesn't like this placed before show_all() */
  terminal_search_dialog_opacity_changed (dialog);

  gtk_window_present (GTK_WINDOW (dialog));
  gtk_entry_grab_focus_without_selecting (GTK_ENTRY (dialog->entry));
}
