/*-
 * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
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

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <exo/exo.h>

#include <terminal/terminal-dialogs.h>
#include <terminal/terminal-private.h>



/**
 * terminal_dialogs_show_about:
 * @parent : the parent #GtkWindow or %NULL.
 * @title  : the software title.
 * @format : the printf()-style format for the main text in the about dialog.
 * @...    : argument list for the @format.
 *
 * Displays the Terminal about dialog with @format as main text.
 **/
void
terminal_dialogs_show_about (GtkWindow *parent)
{
  static const gchar *authors[] =
  {
    "Benedikt Meurer <benny@xfce.org>",
    "Nick Schermer <nick@xfce.org>",
    NULL,
  };

  static const gchar *artists[] =
  {
    "Francois Le Clainche <fleclainche@wanadoo.fr>",
    NULL,
  };

  static const gchar *documenters[] =
  {
    "Benedikt Meurer <benny@xfce.org>",
    "Andrew Conkling <andrewski@fr.st>",
    "Nick Schermer <nick@xfce.org>",
    NULL,
  };

  GdkPixbuf    *logo = NULL;
  GtkIconTheme *theme;

  /* try to load the about logo */
  theme = gtk_icon_theme_get_default ();
  if (gtk_icon_theme_has_icon (theme, "utilities-terminal"))
    logo = gtk_icon_theme_load_icon (theme, "utilities-terminal", 128, GTK_ICON_LOOKUP_FORCE_SVG, NULL);
  if (logo == NULL)
    logo = gdk_pixbuf_new_from_file_at_size (DATADIR "/icons/hicolor/scalable/apps/Terminal.svg", 128, 128, NULL);

  /* set dialog hook on gtk versions older then 2.18 */
#if !GTK_CHECK_VERSION (2, 18, 0)
#if EXO_CHECK_VERSION (0, 5, 0)
  gtk_about_dialog_set_email_hook (exo_gtk_url_about_dialog_hook, NULL, NULL);
  gtk_about_dialog_set_url_hook (exo_gtk_url_about_dialog_hook, NULL, NULL);
#else
  gtk_about_dialog_set_email_hook (exo_url_about_dialog_hook, NULL, NULL);
  gtk_about_dialog_set_url_hook (exo_url_about_dialog_hook, NULL, NULL);
#endif
#endif

  /* open the about dialog */
  gtk_show_about_dialog (parent,
                         "authors", authors,
                         "artists", artists,
                         "comments", _("Xfce Terminal Emulator"),
                         "documenters", documenters,
                         "copyright", "Copyright \302\251 2003-2008 Benedikt Meurer\n"
                                      "Copyright \302\251 2007-2010 Nick Schermer",
                         "license", XFCE_LICENSE_GPL,
                         "logo", logo,
                         "program-name", g_get_application_name (),
                         "translator-credits", _("translator-credits"),
                         "version", PACKAGE_VERSION,
                         "website", "http://goodies.xfce.org/projects/applications/terminal",
                         "website-label", _("Visit Terminal website"),
                         NULL);

  /* release the about logo (if any) */
  if (G_LIKELY (logo != NULL))
    g_object_unref (G_OBJECT (logo));
}



/**
 * terminal_dialogs_show_error:
 * @parent : a #GtkWidget on which the error dialog should be shown, or a #GdkScreen
 *           if no #GtkWidget is known. May also be %NULL, in which case the default
 *           #GdkScreen will be used.
 * @error  : a #GError, which gives a more precise description of the problem or %NULL.
 * @format : the printf()-style format for the primary problem description.
 * @...    : argument list for the @format.
 *
 * Displays an error dialog on @widget using the @format as primary message and optionally
 * displaying @error as secondary error text.
 *
 * If @widget is not %NULL and @widget is part of a #GtkWindow, the function makes sure
 * that the toplevel window is visible prior to displaying the error dialog.
 **/
void
terminal_dialogs_show_error (gpointer      parent,
                             const GError *error,
                             const gchar  *format,
                             ...)
{
  GtkWidget *dialog;
  GtkWidget *window = NULL;
  GdkScreen *screen;
  va_list    args;
  gchar     *primary_text;

  terminal_return_if_fail (parent == NULL || GDK_IS_SCREEN (parent) || GTK_IS_WIDGET (parent));

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
      if (GTK_WIDGET_TOPLEVEL (window))
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

  /* determine the primary error text */
  va_start (args, format);
  primary_text = g_strdup_vprintf (format, args);
  va_end (args);

  /* allocate the error dialog */
  dialog = gtk_message_dialog_new ((GtkWindow *) window,
                                   GTK_DIALOG_DESTROY_WITH_PARENT
                                   | GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                   "%s.", primary_text);

  /* move the dialog to the appropriate screen */
  if (window == NULL && screen != NULL)
    gtk_window_set_screen (GTK_WINDOW (dialog), screen);

  /* set secondary text if an error is provided */
  if (G_LIKELY (error != NULL))
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s.", error->message);

  /* display the dialog */
  gtk_dialog_run (GTK_DIALOG (dialog));

  /* cleanup */
  gtk_widget_destroy (dialog);
  g_free (primary_text);
}



static gboolean
terminal_dialogs_show_help_ask_online (GtkWindow *parent)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *image;
  gint       response_id;

  dialog = gtk_message_dialog_new (parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_CANCEL,
                                   _("The %s user manual is not installed on your computer"),
                                   g_get_application_name ());
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
      _("You can read the user manual online. This manual may however "
        "not exactly match your %s version."), g_get_application_name ());
  gtk_window_set_title (GTK_WINDOW (dialog), _("User manual is missing"));

  button = gtk_button_new_with_mnemonic (_("_Read Online"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_ACCEPT);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name ("web-browser", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  gtk_widget_show (image);

  response_id = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return (response_id == GTK_RESPONSE_ACCEPT);
}



/**
 * terminal_dialogs_show_help:
 * @parent : a #GtkWindow.
 *           #GdkScreen will be used.
 * @page   : the name of the page of the user manual to display or %NULL to display
 *           the overview page.
 * @offset : the offset of the topic in the @page to display or %NULL to just display
 *           @page.
 *
 * Displays the Terminal user manual. If @page is not %NULL it specifies the basename
 * of the HTML file to display. @offset may refer to a anchor in the @page.
 **/
void
terminal_dialogs_show_help (GtkWindow   *parent,
                            const gchar *page,
                            const gchar *offset)
{
  GError   *error = NULL;
  gchar    *filename;
  gchar    *locale;
  gboolean  exists;
  gchar    *uri = NULL;

  terminal_return_if_fail (GTK_IS_WINDOW (parent));

  /* use index page */
  if (page == NULL)
    page = "index.html";

  /* get the locale of the user */
  locale = g_strdup (setlocale (LC_MESSAGES, NULL));
  if (G_LIKELY (locale != NULL))
    locale = g_strdelimit (locale, "._", '\0');
  else
    locale = g_strdup ("C");

  /* check if the help page exists on the system */
  filename = g_build_filename (HELPDIR, locale, page, NULL);
  exists = g_file_test (filename, G_FILE_TEST_EXISTS);
  if (!exists && strcmp (locale, "C") != 0)
    {
      g_free (filename);
      filename = g_build_filename (HELPDIR, "C", page, NULL);
      exists = g_file_test (filename, G_FILE_TEST_EXISTS);
    }

  /* build the full uri, fallback to online docs if nothing was found */
  if (G_LIKELY (exists))
    uri = g_strconcat ("file://", filename, "#", offset, NULL);
  else if (terminal_dialogs_show_help_ask_online (parent))
    uri = g_strconcat ("http://foo-projects.org/~nick/docs/terminal/?lang=",
                       locale, "&page=", page, "&offset=", offset, NULL);

  g_free (filename);
  g_free (locale);

  /* try to run the documentation browser */
  if (uri != NULL
      && !exo_execute_preferred_application_on_screen ("WebBrowser", uri, NULL,
                                                       NULL, gtk_window_get_screen (parent),
                                                       &error))
    {
      /* display an error message to the user */
      terminal_dialogs_show_error (parent, error, _("Failed to open the documentation browser"));
      g_error_free (error);
    }

  g_free (uri);
}


