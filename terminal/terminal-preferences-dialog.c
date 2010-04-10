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
#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-shortcut-editor.h>
#include <terminal/terminal-stock.h>
#include <terminal/terminal-private.h>
#include <terminal/xfce-titled-dialog.h>



static void terminal_preferences_dialog_finalize          (GObject                   *object);
static void terminal_preferences_dialog_response          (GtkWidget                 *widget,
                                                           gint                       response,
                                                           TerminalPreferencesDialog *dialog);
static void terminal_preferences_dialog_reset_compat      (GtkWidget                 *button,
                                                           TerminalPreferencesDialog *dialog);
static void terminal_preferences_dialog_reset_word_chars  (GtkWidget                 *button,
                                                           TerminalPreferencesDialog *dialog);
static void terminal_preferences_dialog_background_mode   (GtkWidget                 *combobox,
                                                           TerminalPreferencesDialog *dialog);
static void terminal_preferences_dialog_background_notify (GObject                   *object,
                                                           GParamSpec                *pspec,
                                                           GObject                   *widget);
static void terminal_preferences_dialog_background_set    (GtkFileChooserButton      *widget,
                                                           TerminalPreferencesDialog *dialog);



G_DEFINE_TYPE (TerminalPreferencesDialog, terminal_preferences_dialog, GTK_TYPE_BUILDER)



static void
terminal_preferences_dialog_class_init (TerminalPreferencesDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_preferences_dialog_finalize;
}



#define BIND_PROPERTIES(name, property) \
  G_STMT_START { \
  object = gtk_builder_get_object (GTK_BUILDER (dialog), name); \
  terminal_return_if_fail (G_IS_OBJECT (object)); \
  binding = exo_mutual_binding_new (G_OBJECT (dialog->preferences), name, \
                                    G_OBJECT (object), property); \
  dialog->bindings = g_slist_prepend (dialog->bindings, binding); \
  } G_STMT_END



static void
terminal_preferences_dialog_init (TerminalPreferencesDialog *dialog)
{
  GError           *error = NULL;
  guint             i;
  GObject          *object, *object2;
  GtkWidget        *editor;
  gchar             palette_name[16];
  GtkFileFilter    *filter;
  gchar            *file;
  ExoMutualBinding *binding;
  const gchar      *props_active[] = { "title-mode", "command-login-shell",
                                    "command-update-records", "scrolling-single-line",
                                    "scrolling-on-output", "scrolling-on-keystroke",
                                    "scrolling-bar", "font-allow-bold",
                                    "misc-menubar-default", "misc-toolbars-default",
                                    "misc-borders-default", "color-selection-use-default",
                                    "shortcuts-no-mnemonics", "shortcuts-no-menukey",
                                    "binding-backspace", "binding-delete",
                                    "background-mode", "background-image-style"
#if TERMINAL_HAS_ANTI_ALIAS_SETTING
                                    , "font-anti-alias"
#endif
                                    };
  const gchar      *props_color[] =  { "color-foreground", "color-cursor",
                                       "color-background", "tab-activity-color",
                                       "color-selection" };

  dialog->preferences = terminal_preferences_get ();

#if !LIBXFCE4UTIL_CHECK_VERSION (4, 7, 2)
  /* restore the default translation domain. this is broken by libxfce4util
   * < 4.7.2, after calling XFCE_LICENSE_GPL. see bug #5842. */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
#endif

  /* hack to initialize the XfceTitledDialog class */
  if (xfce_titled_dialog_get_type () == 0)
    return;

  /* lookup the ui file */
  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "Terminal/Terminal.glade");
  xfce_resource_pop_path (XFCE_RESOURCE_DATA);

  if (G_UNLIKELY (file == NULL))
    {
      g_set_error (&error, 0, 0, "file not found");
      goto error;
    }

  /* load the builder data into the object */
  if (gtk_builder_add_from_file (GTK_BUILDER (dialog), file, &error) == 0)
    {
error:
      g_critical ("Failed to load ui file: %s.", error->message);
      g_error_free (error);
      return;
    }

  /* connect response to dialog */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "dialog");
  terminal_return_if_fail (G_IS_OBJECT (object));
  g_object_weak_ref (G_OBJECT (object), (GWeakNotify) g_object_unref, dialog);
  g_signal_connect (G_OBJECT (object), "response",
      G_CALLBACK (terminal_preferences_dialog_response), dialog);

  /* bind active properties */
  for (i = 0; i < G_N_ELEMENTS (props_active); i++)
    BIND_PROPERTIES (props_active[i], "active");

  /* bind color properties */
  for (i = 0; i < G_N_ELEMENTS (props_color); i++)
    BIND_PROPERTIES (props_color[i], "color");

  /* bind color palette properties */
  for (i = 1; i <= 16; i++)
    {
      g_snprintf (palette_name, sizeof (palette_name), "color-palette%d", i);
      BIND_PROPERTIES (palette_name, "color");
    }

  /* other properties */
  BIND_PROPERTIES ("font-name", "font-name");
  BIND_PROPERTIES ("title-initial", "text");
  BIND_PROPERTIES ("term", "text");
  BIND_PROPERTIES ("word-chars", "text");
  BIND_PROPERTIES ("scrolling-lines", "value");
  BIND_PROPERTIES ("tab-activity-timeout", "value");
  BIND_PROPERTIES ("background-darkness", "value");

#if !TERMINAL_HAS_ANTI_ALIAS_SETTING
  /* hide anti alias setting */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "font-anti-alias");
  terminal_return_if_fail (G_IS_OBJECT (object));
  gtk_widget_hide (GTK_WIDGET (object));
#endif

  /* reset comparibility button */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "reset-compatibility");
  terminal_return_if_fail (G_IS_OBJECT (object));
  g_signal_connect (G_OBJECT (object), "clicked",
      G_CALLBACK (terminal_preferences_dialog_reset_compat), dialog);

  /* reset word-chars button */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "reset-word-chars");
  terminal_return_if_fail (G_IS_OBJECT (object));
  g_signal_connect (G_OBJECT (object), "clicked",
      G_CALLBACK (terminal_preferences_dialog_reset_word_chars), dialog);

  /* add shortcuts editor */
  editor = g_object_new (TERMINAL_TYPE_SHORTCUT_EDITOR, NULL);
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "editor-container");
  terminal_return_if_fail (G_IS_OBJECT (object));
  gtk_container_add (GTK_CONTAINER (object), editor);
  gtk_widget_show (editor);

  /* inverted action between cursor color selections */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "color-selection-use-color");
  terminal_return_if_fail (G_IS_OBJECT (object));
  exo_binding_new_with_negation (G_OBJECT (dialog->preferences), "color-selection-use-default",
                                 G_OBJECT (object), "active");

  /* sensitivity for custom selection color */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "color-selection-use-color");
  terminal_return_if_fail (G_IS_OBJECT (object));
  object2 = gtk_builder_get_object (GTK_BUILDER (dialog), "color-selection");
  terminal_return_if_fail (G_IS_OBJECT (object2));
  exo_binding_new (G_OBJECT (object), "active", G_OBJECT (object2), "sensitive");

  /* background widgets visibility */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "background-mode");
  terminal_return_if_fail (G_IS_OBJECT (object));
  g_signal_connect (G_OBJECT (object), "changed",
      G_CALLBACK (terminal_preferences_dialog_background_mode), dialog);
  terminal_preferences_dialog_background_mode (GTK_WIDGET (object), dialog);

  /* background image file */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "background-image-file");
  terminal_return_if_fail (G_IS_OBJECT (object));
  dialog->signal_id = g_signal_connect (G_OBJECT (dialog->preferences),
      "notify::background-image-file", G_CALLBACK (terminal_preferences_dialog_background_notify), object);
  terminal_preferences_dialog_background_notify (G_OBJECT (dialog->preferences), NULL, object);
  g_signal_connect (G_OBJECT (object), "file-set",
      G_CALLBACK (terminal_preferences_dialog_background_set), dialog);

  /* add file filters */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All Files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (object), filter);

  /* add "Image Files" filter */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Image Files"));
  gtk_file_filter_add_pixbuf_formats (filter);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (object), filter);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (object), filter);
}



static void
terminal_preferences_dialog_finalize (GObject *object)
{
  TerminalPreferencesDialog *dialog = TERMINAL_PREFERENCES_DIALOG (object);

  /* disconnect signal */
  if (G_LIKELY (dialog->signal_id != 0))
    g_signal_handler_disconnect (dialog->preferences, dialog->signal_id);

  /* release the preferences */
  g_object_unref (G_OBJECT (dialog->preferences));

  (*G_OBJECT_CLASS (terminal_preferences_dialog_parent_class)->finalize) (object);
}



static void
terminal_preferences_dialog_response (GtkWidget                 *widget,
                                      gint                       response,
                                      TerminalPreferencesDialog *dialog)
{
  GSList *li;

  /* check if we should open the user manual */
  if (G_UNLIKELY (response == 1))
    {
      /* open the "Preferences" section of the user manual */
      terminal_dialogs_show_help (GTK_WINDOW (widget), "preferences.html", NULL);
    }
  else
    {
      /* disconnect all the bindings */
      for (li = dialog->bindings; li != NULL; li = li->next)
        exo_mutual_binding_unbind (li->data);
      g_slist_free (dialog->bindings);

      /* close the preferences dialog */
      gtk_widget_destroy (widget);
    }
}



static void
terminal_preferences_dialog_reset_compat (GtkWidget                 *button,
                                          TerminalPreferencesDialog *dialog)
{
  GParamSpec  *spec;
  GValue       value = { 0, };
  const gchar *properties[] = { "binding-backspace", "binding-delete", "term" };
  guint        i;

  for (i = 0; i < G_N_ELEMENTS (properties); i++)
    {
      spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences),
                                           properties[i]);
      if (G_LIKELY (spec != NULL))
        {
          g_value_init (&value, spec->value_type);
          g_param_value_set_default (spec, &value);
          g_object_set_property (G_OBJECT (dialog->preferences), properties[i], &value);
          g_value_unset (&value);
        }
    }
}



static void
terminal_preferences_dialog_reset_word_chars (GtkWidget                 *button,
                                              TerminalPreferencesDialog *dialog)
{
  GParamSpec  *spec;
  GValue       value = { 0, };

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (dialog->preferences), "word-chars");
  if (G_LIKELY (spec != NULL))
    {
      g_value_init (&value, spec->value_type);
      g_param_value_set_default (spec, &value);
      g_object_set_property (G_OBJECT (dialog->preferences), "word-chars", &value);
      g_value_unset (&value);
    }
}



static void
terminal_preferences_dialog_background_mode (GtkWidget                 *combobox,
                                             TerminalPreferencesDialog *dialog)
{
  GObject *object;
  gint     active;

  terminal_return_if_fail (TERMINAL_IS_PREFERENCES_DIALOG (dialog));
  terminal_return_if_fail (GTK_IS_COMBO_BOX (combobox));

  active = gtk_combo_box_get_active (GTK_COMBO_BOX (combobox));

  object = gtk_builder_get_object (GTK_BUILDER (dialog), "box-file");
  terminal_return_if_fail (G_IS_OBJECT (object));
  g_object_set (G_OBJECT (object), "visible", active == 1, NULL);

  object = gtk_builder_get_object (GTK_BUILDER (dialog), "box-opacity");
  terminal_return_if_fail (G_IS_OBJECT (object));
  g_object_set (G_OBJECT (object), "visible", active > 0, NULL);
}



static void
terminal_preferences_dialog_background_notify (GObject    *object,
                                               GParamSpec *pspec,
                                               GObject    *widget)
{
  gchar *button_file, *prop_file;

  terminal_return_if_fail (TERMINAL_IS_PREFERENCES (object));
  terminal_return_if_fail (GTK_IS_FILE_CHOOSER_BUTTON (widget));

  button_file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  g_object_get (G_OBJECT (object), "background-image-file", &prop_file, NULL);
  if (!exo_str_is_equal (button_file, prop_file))
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget), prop_file);
  g_free (button_file);
  g_free (prop_file);
}



static void
terminal_preferences_dialog_background_set (GtkFileChooserButton      *widget,
                                            TerminalPreferencesDialog *dialog)
{
  gchar *filename;

  terminal_return_if_fail (TERMINAL_IS_PREFERENCES_DIALOG (dialog));
  terminal_return_if_fail (GTK_IS_FILE_CHOOSER_BUTTON (widget));

  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  g_object_set (G_OBJECT (dialog->preferences), "background-image-file", filename, NULL);
  g_free (filename);
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
  GtkBuilder *builder;
  GObject    *dialog;

  builder = g_object_new (TERMINAL_TYPE_PREFERENCES_DIALOG, NULL);
  dialog = gtk_builder_get_object (builder, "dialog");
  if (parent != NULL && dialog != NULL)
    gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

  return GTK_WIDGET (dialog);
}
