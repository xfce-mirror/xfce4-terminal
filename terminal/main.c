/*-F
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

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "terminal-app.h"
#include "terminal-gdbus.h"
#include "terminal-preferences-dialog.h"
#include "terminal-private.h"
#include "terminal-widget.h"
#include "terminal-window.h"



static void
colortable_sub (const gchar *bright,
                guint start)
{
  guint n;
  guint fg;
  guint bg;

  for (n = start; n <= 37; n++)
    {
      if (n == 28)
        fg = 0;
      else if (n == 29)
        fg = 1;
      else
        fg = n;

      /* blank */
      g_print (" %*s%2dm |",
               2, bright, fg);

      /* without background color */
      g_print ("\e[%s%dm %*s%2dm ",
               bright, fg, 2, bright, fg);

      /* with background color */
      for (bg = 40; bg <= 47; bg++)
        {
          g_print ("\e[%s%d;%dm %*s%2dm ",
                   bright, fg, bg, 2, bright, fg);
        }

      g_print ("\e[0m\n");
    }
}



static void
colortable (void)
{
  guint bg;

  /* header */
  g_print ("%*s|%*s", 7, "", 7, "");
  for (bg = 40; bg <= 47; bg++)
    g_print ("   %dm ", bg);
  g_print ("\n");

  /* normal */
  colortable_sub ("", 28);

  /* bright */
  colortable_sub ("1;", 30);
}



// clang-format off: WhitespaceSensitiveMacros is buggy
static void
usage (void)
{
  g_print ("%s\n"
           "  %s [%s...]\n\n",
           _("Usage:"), PACKAGE_NAME, _("OPTION"));

  g_print ("%s:\n"
           "  -h, --help; -V, --version; --disable-server; --color-table; --preferences;\n"
           "  --default-display=%s; --default-working-directory=%s\n\n",
           _("General Options"),
           /* parameter of --default-display */
           _("display"),
           /* parameter of --default-working-directory */
           _("directory"));

  g_print ("%s:\n"
           "  --tab; --window\n\n",
           _("Window or Tab Separators"));

  g_print ("%s:\n"
           "  -x, --execute; -e, --command=%s; -T, --title=%s;\n"
           "  --dynamic-title-mode=%s ('replace', 'before', 'after', 'none');\n"
           "  --initial-title=%s; --working-directory=%s; -H, --hold;\n"
           "  --active-tab; --color-text=%s; --color-bg=%s\n\n",
           _("Tab Options"),
           /* parameter of --command */
           _("command"),
           /* parameter of --title */
           _("title"),
           /* parameter of --dynamic-title-mode */
           _("mode"),
           /* parameter of --initial-title */
           _("title"),
           /* parameter of --working-directory */
           _("directory"),
           /* parameter of --color-text */
           _("color"),
           /* parameter of --color-bg */
           _("color"));

  g_print ("%s:\n"
           "  --display=%s; --geometry=%s; --role=%s; --drop-down;\n"
           "  --startup-id=%s; -I, --icon=%s; --fullscreen; --maximize; --minimize;\n"
           "  --show-menubar, --hide-menubar; --show-borders, --hide-borders;\n"
           "  --show-toolbar, --hide-toolbar; --show-scrollbar, --hide-scrollbar;\n"
           "  --font=%s; --zoom=%s; --class=%s\n\n",
           _("Window Options"),
           /* parameter of --display */
           _("display"),
           /* parameter of --geometry */
           _("geometry"),
           /* parameter of --role */
           _("role"),
           /* parameter of --startup-id */
           _("string"),
           /* parameter of --icon */
           _("icon"),
           /* parameter of --font */
           _("font"),
           /* parameter of --zoom */
           _("zoom"),
           /* parameter of --class */
           _("class"));

  g_print (_("See the %s man page for full explanation of the options above."),
           PACKAGE_NAME);

  g_print ("\n\n");
}
// clang-format on



int
main (int argc, char **argv)
{
  TerminalOptions options;
  TerminalApp *app;
  const gchar *startup_id;
  GdkDisplay *display;
  GError *error = NULL;
  gchar **nargv;
  gint nargc;
  gint n;
  const gchar *msg;

  /* initialize options */
  options.disable_server = options.show_version = options.show_colors = options.show_help = options.show_preferences = 0;

  /* install required signal handlers */
  signal (SIGPIPE, SIG_IGN);

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* initializes internationalization support */
  if (setlocale (LC_ALL, "") == NULL)
    g_warning ("Locale not supported by C library.");

  g_set_application_name (_("Xfce Terminal"));

  /* initialize GTK: do this before our parsing so GTK parses its options first */
  gtk_init (&argc, &argv);

  /* parse some options we need in main, not the windows attrs */
  terminal_options_parse (argc, argv, &options);

  if (G_UNLIKELY (options.show_version))
    {
      g_print ("%s %s (Xfce %s)\n\n", PACKAGE_NAME, VERSION_FULL, xfce_version_string ());
      g_print ("%s\n", "Copyright (c) 2003-" COPYRIGHT_YEAR);
      g_print ("\t%s\n\n", _("The Xfce development team. All rights reserved."));
      g_print ("%s\n", _("Written by Benedikt Meurer <benny@xfce.org>,"));
      g_print ("%s\n", _("Nick Schermer <nick@xfce.org>,"));
      g_print ("%s\n", _("Igor Zakharov <f2404@yandex.ru>,"));
      g_print ("%s\n\n", _("Sergios - Anestis Kefalidis <sergioskefalidis@gmail.com>."));
      g_print (_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
      g_print ("\n");

      return EXIT_SUCCESS;
    }
  else if (G_UNLIKELY (options.show_colors))
    {
      colortable ();
      return EXIT_SUCCESS;
    }
  else if (G_UNLIKELY (options.show_help))
    {
      usage ();
      return EXIT_SUCCESS;
    }
  else if (G_UNLIKELY (options.show_preferences))
    {
      GtkWidget *dialog;

      /* load the AccelMap for the XfceShortcutsEditor */
      app = g_object_new (TERMINAL_TYPE_APP, NULL);
      xfce_gtk_translate_action_entries (terminal_window_get_action_entries (), TERMINAL_WINDOW_ACTION_N);
      xfce_gtk_translate_action_entries (terminal_widget_get_action_entries (), TERMINAL_WIDGET_ACTION_N);
      xfce_gtk_accel_map_add_entries (terminal_window_get_action_entries (), TERMINAL_WINDOW_ACTION_N);
      xfce_gtk_accel_map_add_entries (terminal_widget_get_action_entries (), TERMINAL_WIDGET_ACTION_N);
      terminal_app_load_accels (app); /* manual execution, instead of the typical idle function */

      /* create and run the dialog */
      dialog = terminal_preferences_dialog_new (TRUE, FALSE);
      g_signal_connect_after (G_OBJECT (dialog), "destroy", G_CALLBACK (gtk_main_quit), NULL);
      gtk_window_present (GTK_WINDOW (dialog));
      gtk_main ();

      g_object_unref (G_OBJECT (app));

      return EXIT_SUCCESS;
    }

  /* create a copy of the standard arguments with our additional stuff */
  nargv = g_new (gchar *, argc + 5);
  nargc = 0;
  nargv[nargc++] = g_strdup (argv[0]);
  nargv[nargc++] = g_strdup ("--default-working-directory");
  nargv[nargc++] = g_get_current_dir ();

  /* append startup if given */
  startup_id = g_getenv ("DESKTOP_STARTUP_ID");
  if (G_LIKELY (startup_id != NULL))
    {
      nargv[nargc++] = g_strdup_printf ("--startup-id=%s", startup_id);
      g_unsetenv ("DESKTOP_STARTUP_ID");
    }

  /* append default display if given */
  display = gdk_display_get_default ();
  if (G_LIKELY (display != NULL))
    nargv[nargc++] = g_strdup_printf ("--default-display=%s", gdk_display_get_name (display));

  /* append all given arguments */
  for (n = 1; n < argc; ++n)
    nargv[nargc++] = g_strdup (argv[n]);
  nargv[nargc] = NULL;

  if (!options.disable_server)
    {
      /* try to connect to an existing Terminal service */
      if (terminal_gdbus_invoke_launch (nargc, nargv, &error))
        {
          g_strfreev (nargv);
          return EXIT_SUCCESS;
        }
      else
        {
          if (g_error_matches (error, TERMINAL_ERROR, TERMINAL_ERROR_USER_MISMATCH)
              || g_error_matches (error, TERMINAL_ERROR, TERMINAL_ERROR_DISPLAY_MISMATCH))
            {
              /* don't try to establish another service here */
              options.disable_server = 1;

              g_debug ("%s mismatch when invoking remote terminal: %s",
                       error->code == TERMINAL_ERROR_USER_MISMATCH ? "User" : "Display",
                       error->message);
            }
          else if (g_error_matches (error, TERMINAL_ERROR, TERMINAL_ERROR_OPTIONS))
            {
              /* skip the GDBus prefix */
              msg = strchr (error->message, ' ');
              if (G_LIKELY (msg != NULL))
                msg++;
              else
                msg = error->message;

              /* options were not parsed succesfully, don't try that again below */
              g_printerr ("%s: %s\n", PACKAGE_NAME, msg);
              g_error_free (error);
              g_strfreev (nargv);
              return EXIT_FAILURE;
            }
          else if (error != NULL)
            {
              g_debug ("D-Bus reply error: %s (%s: %d)", error->message,
                       g_quark_to_string (error->domain), error->code);
            }

          g_clear_error (&error);
        }
    }

  /* set default window icon */
  gtk_window_set_default_icon_name ("org.xfce.terminal");

  app = g_object_new (TERMINAL_TYPE_APP, NULL);

  if (!options.disable_server)
    {
      if (!terminal_gdbus_register_service (app, &error))
        {
          g_warning ("Unable to register terminal service: %s", error->message);
          g_clear_error (&error);
        }
    }

  if (!terminal_app_process (app, nargv, nargc, &error))
    {
      /* parsing one of the arguments failed */
      g_printerr ("%s: %s\n", PACKAGE_NAME, error->message);
      g_error_free (error);
      g_object_unref (G_OBJECT (app));
      g_strfreev (nargv);
      return EXIT_FAILURE;
    }

  /* free temporary arguments */
  g_strfreev (nargv);

  gtk_main ();

  g_object_unref (G_OBJECT (app));

  return EXIT_SUCCESS;
}
