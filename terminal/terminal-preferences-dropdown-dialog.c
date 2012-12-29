/*-
 * Copyright (C) 2012 Nick Schermer <nick@xfce.org>
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
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-preferences-dropdown-dialog.h>
#include <terminal/terminal-private.h>



static void terminal_preferences_dropdown_dialog_finalize (GObject                           *object);
static void terminal_preferences_dropdown_dialog_response (GtkWidget                         *widget,
                                                           gint                               response,
                                                           TerminalPreferencesDropdownDialog *dialog);



struct _TerminalPreferencesDropdownDialogClass
{
  GtkBuilderClass __parent__;
};

struct _TerminalPreferencesDropdownDialog
{
  GtkBuilder __parent__;

  TerminalPreferences *preferences;
  GSList              *bindings;
};



G_DEFINE_TYPE (TerminalPreferencesDropdownDialog, terminal_preferences_dropdown_dialog, GTK_TYPE_BUILDER)



static void
terminal_preferences_dropdown_dialog_class_init (TerminalPreferencesDropdownDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_preferences_dropdown_dialog_finalize;
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
terminal_preferences_dropdown_dialog_init (TerminalPreferencesDropdownDialog *dialog)
{
  GError           *error = NULL;
  guint             i;
  GObject          *object;
  gchar            *file;
  GBinding         *binding;
  const gchar      *props_active[] = { "dropdown-keep-open-default",
                                       "dropdown-keep-above",
                                       "dropdown-toggle-focus",
                                       "dropdown-status-icon",
                                       "dropdown-move-to-active" };
  const gchar      *props_value[] = { "dropdown-height",
                                      "dropdown-width",
                                      "dropdown-position",
                                      "dropdown-opacity",
                                      "dropdown-animation-time" };

  dialog->preferences = terminal_preferences_get ();

  /* hack to initialize the XfceTitledDialog class */
  if (xfce_titled_dialog_get_type () == 0)
    return;

  /* lookup the ui file */
  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "xfce4/terminal/terminal-preferences-dropdown.ui");
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
      G_CALLBACK (terminal_preferences_dropdown_dialog_response), dialog);

  /* bind active properties */
  for (i = 0; i < G_N_ELEMENTS (props_active); i++)
    BIND_PROPERTIES (props_active[i], "active");

  /* bind adjustment properties */
  for (i = 0; i < G_N_ELEMENTS (props_value); i++)
    BIND_PROPERTIES (props_value[i], "value");

  object = gtk_builder_get_object (GTK_BUILDER (dialog), "scale-position");
  terminal_return_if_fail (G_IS_OBJECT (object));
  for (i = 0; i <= 100; i += 25)
    gtk_scale_add_mark (GTK_SCALE (object), i, GTK_POS_BOTTOM, NULL);
}



static void
terminal_preferences_dropdown_dialog_finalize (GObject *object)
{
  TerminalPreferencesDropdownDialog *dialog = TERMINAL_PREFERENCES_DROPDOWN_DIALOG (object);

  /* release the preferences */
  g_object_unref (G_OBJECT (dialog->preferences));

  (*G_OBJECT_CLASS (terminal_preferences_dropdown_dialog_parent_class)->finalize) (object);
}



static void
terminal_preferences_dropdown_dialog_response (GtkWidget                         *widget,
                                               gint                               response,
                                               TerminalPreferencesDropdownDialog *dialog)
{
  GSList *li;

  /* check if we should open the user manual */
  if (G_UNLIKELY (response == 1))
    {
      /* open the "PreferencesDropdown" section of the user manual */
      xfce_dialog_show_help (GTK_WINDOW (widget), "terminal",
                             "dropdown", NULL);
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



/**
 * terminal_preferences_dropdown_dialog_new:
 * @parent      : A #GtkWindow or %NULL.
 *
 * Return value :
 **/
GtkWidget*
terminal_preferences_dropdown_dialog_new (void)
{
  GtkBuilder *builder;
  GObject    *dialog;

  builder = g_object_new (TERMINAL_TYPE_PREFERENCES_DROPDOWN_DIALOG, NULL);
  dialog = gtk_builder_get_object (builder, "dialog");

  return GTK_WIDGET (dialog);
}
