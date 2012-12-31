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
#include <terminal/terminal-private.h>



static void terminal_search_dialog_finalize           (GObject              *object);
static void terminal_search_dialog_clear_gregex       (TerminalSearchDialog *dialog);
static void terminal_search_dialog_entry_icon_release (GtkWidget            *entry,
                                                       GtkEntryIconPosition  icon_pos);
static void terminal_search_dialog_entry_changed      (GtkWidget            *entry,
                                                       TerminalSearchDialog *dialog);


struct _TerminalSearchDialogClass
{
  GtkDialogClass __parent__;
};

struct _TerminalSearchDialog
{
  GtkDialog __parent__;

  GRegex    *last_gregex;

  GtkWidget *button_prev;
  GtkWidget *button_next;

  GtkWidget *entry;

  GtkWidget *match_case;
  GtkWidget *match_regex;
  GtkWidget *match_word;
  GtkWidget *wrap_around;
};



G_DEFINE_TYPE (TerminalSearchDialog, terminal_search_dialog, GTK_TYPE_DIALOG)



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
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;

  gtk_window_set_title (GTK_WINDOW (dialog), _("Find"));
  gtk_window_set_default_size (GTK_WINDOW (dialog), 400, -1);

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  dialog->button_prev = xfce_gtk_button_new_mixed (GTK_STOCK_GO_BACK, _("_Previous"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), dialog->button_prev, TERMINAL_RESPONSE_SEARCH_PREV);
  gtk_widget_set_can_default (dialog->button_prev, TRUE);
  gtk_widget_show (dialog->button_prev);

  dialog->button_next = xfce_gtk_button_new_mixed (GTK_STOCK_GO_FORWARD, _("_Next"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), dialog->button_next, TERMINAL_RESPONSE_SEARCH_NEXT);
  gtk_widget_show (dialog->button_next);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), vbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Search for:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  dialog->entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), dialog->entry, TRUE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->entry);
  gtk_entry_set_activates_default (GTK_ENTRY (dialog->entry), TRUE);
  gtk_entry_set_icon_from_stock (GTK_ENTRY (dialog->entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
  g_signal_connect (G_OBJECT (dialog->entry), "icon-release",
      G_CALLBACK (terminal_search_dialog_entry_icon_release), NULL);
  g_signal_connect (G_OBJECT (dialog->entry), "changed",
      G_CALLBACK (terminal_search_dialog_entry_changed), dialog);
  gtk_widget_show (dialog->entry);

  dialog->match_case = gtk_check_button_new_with_mnemonic (_("C_ase sensitive"));
  gtk_box_pack_start (GTK_BOX (vbox), dialog->match_case, FALSE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (dialog->match_case), "toggled",
      G_CALLBACK (terminal_search_dialog_clear_gregex), dialog);
  gtk_widget_show (dialog->match_case);

  dialog->match_regex = gtk_check_button_new_with_mnemonic (_("Match as _regular expression"));
  gtk_box_pack_start (GTK_BOX (vbox), dialog->match_regex, FALSE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (dialog->match_regex), "toggled",
      G_CALLBACK (terminal_search_dialog_clear_gregex), dialog);
  gtk_widget_show (dialog->match_regex);

  dialog->match_word = gtk_check_button_new_with_mnemonic (_("Match _entire word only"));
  gtk_box_pack_start (GTK_BOX (vbox), dialog->match_word, FALSE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (dialog->match_word), "toggled",
      G_CALLBACK (terminal_search_dialog_clear_gregex), dialog);
  gtk_widget_show (dialog->match_word);

  dialog->wrap_around = gtk_check_button_new_with_mnemonic (_("_Wrap around"));
  gtk_box_pack_start (GTK_BOX (vbox), dialog->wrap_around, FALSE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (dialog->wrap_around), "toggled",
      G_CALLBACK (terminal_search_dialog_clear_gregex), dialog);
  gtk_widget_show (dialog->wrap_around);

  terminal_search_dialog_entry_changed (dialog->entry, dialog);
}



static void
terminal_search_dialog_finalize (GObject *object)
{
  terminal_search_dialog_clear_gregex (TERMINAL_SEARCH_DIALOG (object));

  (*G_OBJECT_CLASS (terminal_search_dialog_parent_class)->finalize) (object);
}



static void
terminal_search_dialog_clear_gregex (TerminalSearchDialog *dialog)
{
  if (dialog->last_gregex != NULL)
    {
      g_regex_unref (dialog->last_gregex);
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
                       "transient-for", parent, NULL);
}



gboolean
terminal_search_dialog_get_wrap_around (TerminalSearchDialog *dialog)
{
  terminal_return_val_if_fail (TERMINAL_IS_SEARCH_DIALOG (dialog), FALSE);
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->wrap_around));
}



GRegex *
terminal_search_dialog_get_regex (TerminalSearchDialog  *dialog,
                                  GError               **error)
{
  const gchar        *pattern;
  GRegexCompileFlags  flags = G_REGEX_OPTIMIZE;
  gchar              *pattern_escaped = NULL;
  gchar              *word_regex = NULL;
  GRegex             *regex;

  terminal_return_val_if_fail (TERMINAL_IS_SEARCH_DIALOG (dialog), NULL);
  terminal_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* if not cleared, use the old regex */
  if (dialog->last_gregex != NULL)
    return g_regex_ref (dialog->last_gregex);

  /* unset if no pattern is typed */
  pattern = gtk_entry_get_text (GTK_ENTRY (dialog->entry));
  if (!IS_STRING (pattern))
    return NULL;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_case)))
    flags |= G_REGEX_CASELESS;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->match_regex)))
    flags |= G_REGEX_MULTILINE;
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

  regex = g_regex_new (pattern, flags, 0, error);

  g_free (pattern_escaped);
  g_free (word_regex);

  /* keep around */
  if (regex != NULL)
    dialog->last_gregex = g_regex_ref (regex);

  return regex;
}



void
terminal_search_dialog_present (TerminalSearchDialog  *dialog)
{
  terminal_return_if_fail (TERMINAL_IS_SEARCH_DIALOG (dialog));

  gtk_window_present (GTK_WINDOW (dialog));
  gtk_widget_grab_focus (dialog->entry);
}
