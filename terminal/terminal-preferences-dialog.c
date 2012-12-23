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

#include <libxfce4ui/libxfce4ui.h>

#include <terminal/terminal-dialogs.h>
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-encoding-action.h>
#include <terminal/terminal-private.h>



static void terminal_preferences_dialog_finalize          (GObject                   *object);
static void terminal_preferences_dialog_response          (GtkWidget                 *widget,
                                                           gint                       response,
                                                           TerminalPreferencesDialog *dialog);
static void terminal_preferences_dialog_palette_changed   (GtkWidget                 *button,
                                                           TerminalPreferencesDialog *dialog);
static void terminal_preferences_dialog_palette_notify    (TerminalPreferencesDialog *dialog);
static void terminal_preferences_dialog_presets_load      (TerminalPreferencesDialog *dialog);
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
static void terminal_preferences_dialog_encoding_changed  (GtkComboBox               *combobox,
                                                           TerminalPreferencesDialog *dialog);



struct _TerminalPreferencesDialogClass
{
  GtkBuilderClass __parent__;
};

struct _TerminalPreferencesDialog
{
  GtkBuilder           __parent__;

  TerminalPreferences *preferences;
  GSList              *bindings;

  gulong               bg_image_signal_id;
  gulong               palette_signal_id;
};

enum
{
  PRESET_COLUMN_TITLE,
  PRESET_COLUMN_IS_SEPARATOR,
  PRESET_COLUMN_PATH,
  N_PRESET_COLUMNS
};



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
  binding = g_object_bind_property (G_OBJECT (dialog->preferences), name, \
                                    G_OBJECT (object), property, \
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL); \
  dialog->bindings = g_slist_prepend (dialog->bindings, binding); \
  } G_STMT_END



static void
terminal_preferences_dialog_init (TerminalPreferencesDialog *dialog)
{
  GError           *error = NULL;
  guint             i;
  GObject          *object, *object2;
  gchar             palette_name[16];
  GtkFileFilter    *filter;
  gchar            *file;
  GBinding         *binding;
  GtkTreeModel     *model;
  gchar            *current;
  GtkTreeIter       current_iter;
  const gchar      *props_active[] = { "title-mode", "command-login-shell",
                                       "command-update-records", "scrolling-single-line",
                                       "scrolling-on-output", "scrolling-on-keystroke",
                                       "scrolling-bar", "font-allow-bold",
                                       "misc-menubar-default", "misc-toolbars-default",
                                       "misc-borders-default", "color-selection-use-default",
                                       "shortcuts-no-mnemonics", "shortcuts-no-menukey",
                                       "binding-backspace", "binding-delete",
                                       "background-mode", "background-image-style",
                                       "color-background-vary"
                                     };
  const gchar      *props_color[] =  { "color-foreground", "color-cursor",
                                       "color-background", "tab-activity-color",
                                       "color-selection"
                                     };

  dialog->preferences = terminal_preferences_get ();

  /* hack to initialize the XfceTitledDialog class */
  if (xfce_titled_dialog_get_type () == 0)
    return;

  /* lookup the ui file */
  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfce4/terminal/terminal-preferences.glade");
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
      object = gtk_builder_get_object (GTK_BUILDER (dialog), palette_name);
      terminal_return_if_fail (G_IS_OBJECT (object));
      g_signal_connect (G_OBJECT (object), "color-set",
          G_CALLBACK (terminal_preferences_dialog_palette_changed), dialog);
    }

  /* watch color changes in property */
  dialog->palette_signal_id = g_signal_connect_swapped (G_OBJECT (dialog->preferences),
      "notify::color-palette", G_CALLBACK (terminal_preferences_dialog_palette_notify), dialog);
  terminal_preferences_dialog_palette_notify (dialog);

  /* color presets */
  terminal_preferences_dialog_presets_load (dialog);

  /* other properties */
  BIND_PROPERTIES ("font-name", "font-name");
  BIND_PROPERTIES ("title-initial", "text");
  BIND_PROPERTIES ("term", "text");
  BIND_PROPERTIES ("word-chars", "text");
  BIND_PROPERTIES ("scrolling-lines", "value");
  BIND_PROPERTIES ("tab-activity-timeout", "value");
  BIND_PROPERTIES ("background-darkness", "value");

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

  /* inverted action between cursor color selections */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "color-selection-use-color");
  terminal_return_if_fail (G_IS_OBJECT (object));
  g_object_bind_property (G_OBJECT (dialog->preferences), "color-selection-use-default",
                          G_OBJECT (object), "active",
                          G_BINDING_INVERT_BOOLEAN | G_BINDING_SYNC_CREATE);

  /* sensitivity for custom selection color */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "color-selection-use-color");
  terminal_return_if_fail (G_IS_OBJECT (object));
  object2 = gtk_builder_get_object (GTK_BUILDER (dialog), "color-selection");
  terminal_return_if_fail (G_IS_OBJECT (object2));
  g_object_bind_property (G_OBJECT (object), "active",
                          G_OBJECT (object2), "sensitive",
                          G_BINDING_SYNC_CREATE);

  /* background widgets visibility */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "background-mode");
  terminal_return_if_fail (G_IS_OBJECT (object));
  g_signal_connect (G_OBJECT (object), "changed",
      G_CALLBACK (terminal_preferences_dialog_background_mode), dialog);
  terminal_preferences_dialog_background_mode (GTK_WIDGET (object), dialog);

  /* background image file */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "background-image-file");
  terminal_return_if_fail (G_IS_OBJECT (object));
  dialog->bg_image_signal_id = g_signal_connect (G_OBJECT (dialog->preferences),
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

  /* encoding combo */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "encoding-combo");
  g_object_get (dialog->preferences, "encoding", &current, NULL);
  model = terminal_encoding_model_new (current, &current_iter);
  gtk_combo_box_set_model (GTK_COMBO_BOX (object), model);
  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (object), &current_iter);
  g_signal_connect (G_OBJECT (object), "changed",
      G_CALLBACK (terminal_preferences_dialog_encoding_changed), dialog);
  g_object_unref (G_OBJECT (model));
  g_free (current);
}



static void
terminal_preferences_dialog_finalize (GObject *object)
{
  TerminalPreferencesDialog *dialog = TERMINAL_PREFERENCES_DIALOG (object);

  /* disconnect signals */
  if (G_LIKELY (dialog->bg_image_signal_id != 0))
    g_signal_handler_disconnect (dialog->preferences, dialog->bg_image_signal_id);
  if (G_LIKELY (dialog->palette_signal_id != 0))
    g_signal_handler_disconnect (dialog->preferences, dialog->palette_signal_id);

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
      xfce_dialog_show_help (GTK_WINDOW (widget), "terminal",
                             "preferences", NULL);
    }
  else
    {
      /* disconnect all the bindings */
      for (li = dialog->bindings; li != NULL; li = li->next)
        g_object_unref (G_OBJECT (li->data));
      g_slist_free (dialog->bindings);

      /* close the preferences dialog */
      gtk_widget_destroy (widget);
    }
}



static void
terminal_preferences_dialog_palette_changed (GtkWidget                 *button,
                                             TerminalPreferencesDialog *dialog)
{
  gchar     name[16];
  guint     i;
  GObject  *obj;
  GdkColor  color;
  gchar    *color_str;
  GString  *array;

  array = g_string_sized_new (225);

  for (i = 1; i <= 16; i++)
    {
      /* get color value from button */
      g_snprintf (name, sizeof (name), "color-palette%d", i);
      obj = gtk_builder_get_object (GTK_BUILDER (dialog), name);
      terminal_return_if_fail (GTK_IS_COLOR_BUTTON (obj));
      gtk_color_button_get_color (GTK_COLOR_BUTTON (obj), &color);

      /* append to string */
      color_str = gdk_color_to_string (&color);
      g_string_append (array, color_str);
      g_free (color_str);

      if (i != 16)
        g_string_append_c (array, ';');
    }

  /* set property */
  g_signal_handler_block (dialog->preferences, dialog->palette_signal_id);
  g_object_set (dialog->preferences, "color-palette", array->str, NULL);
  g_signal_handler_unblock (dialog->preferences, dialog->palette_signal_id);
  g_string_free (array, TRUE);
}



static void
terminal_preferences_dialog_palette_notify (TerminalPreferencesDialog *dialog)
{
  gchar    *color_str;
  gchar   **colors;
  guint     i;
  gchar     name[16];
  GObject  *obj;
  GdkColor  color;

  g_object_get (dialog->preferences, "color-palette", &color_str, NULL);
  if (G_LIKELY (color_str != NULL))
    {
      /* make array */
      colors = g_strsplit (color_str, ";", -1);
      g_free (color_str);

      /* apply values to buttons */
      if (colors != NULL)
        for (i = 0; colors[i] != NULL && i < 16; i++)
          {
            g_snprintf (name, sizeof (name), "color-palette%d", i + 1);
            obj = gtk_builder_get_object (GTK_BUILDER (dialog), name);
            terminal_return_if_fail (GTK_IS_COLOR_BUTTON (obj));

            if (gdk_color_parse (colors[i], &color))
              gtk_color_button_set_color (GTK_COLOR_BUTTON (obj), &color);
          }

      g_strfreev (colors);
    }
}



static gboolean
terminal_preferences_dialog_presets_sepfunc (GtkTreeModel *model,
                                             GtkTreeIter  *iter,
                                             gpointer      user_data)
{
  gboolean is_separator;
  gtk_tree_model_get (model, iter, PRESET_COLUMN_IS_SEPARATOR, &is_separator, -1);
  return is_separator;
}



static void
terminal_preferences_dialog_presets_changed (GtkComboBox               *combobox,
                                             TerminalPreferencesDialog *dialog)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *path;
  XfceRc       *rc;
  guint         n;
  const gchar  *blurb;
  GObjectClass *gobject_class;
  GParamSpec   *pspec;
  const gchar  *str;
  GValue        value = { 0, };
  const gchar  *props[] = { "color-foreground", "color-background",
                            "color-cursor", "color-selection",
                            "color-palette", "tab-activity-color" };

  if (!gtk_combo_box_get_active_iter (combobox, &iter))
    return;

  model = gtk_combo_box_get_model (combobox);
  gtk_tree_model_get (model, &iter, PRESET_COLUMN_PATH, &path, -1);
  if (path == NULL)
    return;

  /* load file */
  rc = xfce_rc_simple_open (path, TRUE);
  g_free (path);
  if (G_UNLIKELY (rc == NULL))
    return;

  xfce_rc_set_group (rc, "Scheme");

  gobject_class = G_OBJECT_GET_CLASS (dialog->preferences);
  for (n = 0; n < G_N_ELEMENTS (props); n++)
    {
      /* lookup the property */
      pspec = g_object_class_find_property (gobject_class, props[n]);
      terminal_assert (pspec != NULL && G_IS_PARAM_SPEC_STRING (pspec));

      /* read key from scheme */
      blurb = g_param_spec_get_blurb (pspec);
      str = xfce_rc_read_entry_untranslated (rc, blurb, NULL);

      /* store value or use default */
      g_value_init (&value, G_TYPE_STRING);
      if (str != NULL)
        g_value_set_static_string (&value, str);
      else
        g_param_value_set_default (pspec, &value);

      /* set */
      g_object_set_property (G_OBJECT (dialog->preferences), props[n], &value);
      g_value_unset (&value);
    }

  xfce_rc_close (rc);
}



static void
terminal_preferences_dialog_presets_load (TerminalPreferencesDialog *dialog)
{
  gchar   **presets;
  guint     n;
  GObject  *object;
  guint     n_presets = 0;
  XfceRc   *rc;
  GtkListStore *store;
  GtkTreeIter iter;
  const gchar *title;
  gchar *path;

  /* load schemes */
  presets = xfce_resource_match (XFCE_RESOURCE_DATA, "xfce4/terminal/colorschemes/*", TRUE);
  if (G_LIKELY (presets != NULL))
    {
      /* create sorting store */
      store = gtk_list_store_new (N_PRESET_COLUMNS, G_TYPE_STRING,
                                  G_TYPE_BOOLEAN, G_TYPE_STRING);
      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                            PRESET_COLUMN_TITLE,
                                            GTK_SORT_ASCENDING);

      /* append files */
      for (n = 0; presets[n] != NULL; n++)
        {
          /* open the scheme */
          path = xfce_resource_lookup (XFCE_RESOURCE_DATA, presets[n]);
          if (G_UNLIKELY (path == NULL))
            continue;

          rc = xfce_rc_simple_open (path, TRUE);
          if (G_UNLIKELY (rc == NULL))
            {
              g_free (path);
              continue;
            }

          xfce_rc_set_group (rc, "Scheme");

          /* translated name */
          title = xfce_rc_read_entry (rc, "Name", NULL);
          if (G_LIKELY (title != NULL))
            {
              gtk_list_store_insert_with_values (store, NULL, n_presets++,
                                                 PRESET_COLUMN_TITLE, title,
                                                 PRESET_COLUMN_PATH, path,
                                                 -1);
            }

          xfce_rc_close (rc);
          g_free (path);
        }

      /* stop sorting */
      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                            GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                            GTK_SORT_ASCENDING);

      /* default item + separator */
      gtk_list_store_insert_with_values (store, &iter, 0,
                                         PRESET_COLUMN_TITLE, _("Load Presets..."),
                                         -1);
      gtk_list_store_insert_with_values (store, NULL, 1,
                                         PRESET_COLUMN_IS_SEPARATOR, TRUE,
                                         -1);

      /* set model */
      object = gtk_builder_get_object (GTK_BUILDER (dialog), "color-presets");
      terminal_return_if_fail (GTK_IS_COMBO_BOX (object));
      gtk_combo_box_set_model (GTK_COMBO_BOX (object), GTK_TREE_MODEL (store));
      gtk_combo_box_set_active_iter  (GTK_COMBO_BOX (object), &iter);
      gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (object),
          terminal_preferences_dialog_presets_sepfunc, NULL, NULL);
      g_signal_connect (G_OBJECT (object), "changed",
          G_CALLBACK (terminal_preferences_dialog_presets_changed), dialog);
      g_object_unref (store);
    }

  g_strfreev (presets);

  if (n_presets == 0)
    {
      /* hide frame + combo */
      object = gtk_builder_get_object (GTK_BUILDER (dialog), "color-presets-frame");
      terminal_return_if_fail (GTK_IS_WIDGET (object));
      gtk_widget_hide (GTK_WIDGET (object));
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
  if (g_strcmp0 (button_file, prop_file) != 0)
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



static void
terminal_preferences_dialog_encoding_changed (GtkComboBox               *combobox,
                                              TerminalPreferencesDialog *dialog)
{
  GtkTreeIter   iter;
  gchar        *encoding;
  GtkTreeModel *model;
  gboolean      is_charset;
  GtkTreeIter   child_iter;

  if (gtk_combo_box_get_active_iter (combobox, &iter))
    {
      model = gtk_combo_box_get_model (combobox);
      gtk_tree_model_get (model, &iter,
                          ENCODING_COLUMN_IS_CHARSET, &is_charset,
                          ENCODING_COLUMN_VALUE, &encoding, -1);

      /* select the child if a menu header is clicked */
      if (encoding == NULL && !is_charset)
        {
          if (gtk_tree_model_iter_children (model, &child_iter, &iter))
            gtk_combo_box_set_active_iter (combobox, &child_iter);
          return;
        }

      /* save new default */
      g_object_set (dialog->preferences, "encoding", encoding, NULL);
      g_free (encoding);
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
  GtkBuilder *builder;
  GObject    *dialog;

  builder = g_object_new (TERMINAL_TYPE_PREFERENCES_DIALOG, NULL);
  dialog = gtk_builder_get_object (builder, "dialog");
  if (parent != NULL && dialog != NULL)
    gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

  return GTK_WIDGET (dialog);
}
