/* $Id$ */
/*-
 * Copyright (c) 2004-2005 os-cillation e.K.
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

#include <stdlib.h>

#include <exo/exo.h>

#include <terminal/terminal-app.h>
#include <terminal/terminal-stock.h>

#ifdef HAVE_DBUS
#include <terminal/terminal-dbus.h>
#endif



static void
usage (void)
{
  g_print ("%s\n", _("Usage: Terminal [OPTION...]"));
  g_print ("\n");
  g_print ("%s\n", _("  -h, --help                          Print this help message and exit"));
  g_print ("%s\n", _("  -v, --version                       Print version information and exit"));
  g_print ("%s\n", _("  --disable-server                    Do not register with the D-BUS\n"
                     "                                      session message bus"));
  g_print ("\n");
  g_print ("%s\n", _("  -x, --execute                       Execute the remainder of the command\n"
                     "                                      line inside the terminal"));
  g_print ("%s\n", _("  -e, --command=STRING                Execute the argument to this option\n"
                     "                                      inside the terminal"));
  g_print ("%s\n", _("  --working-directory=DIRNAME         Set the terminals working directory"));
  g_print ("%s\n", _("  -T, --title=TITLE                   Set the terminals title"));
  g_print ("%s\n", _("  -H, --hold                          Do not immediately destroy the tab\n"
                     "                                      when the child command exits"));
  g_print ("\n");
  g_print ("%s\n", _("  --display=DISPLAY                   X display to use for the last-\n"
                     "                                      specified window"));
  g_print ("%s\n", _("  --geometry=GEOMETRY                 X geometry specification (see \"X\"\n"
                     "                                      man page), can be specified once per\n"
                     "                                      window to be opened"));
  g_print ("%s\n", _("  --role=ROLE                         Set the role for the last-specified;\n"
                     "                                      window; applies to only one window;\n"
                     "                                      can be specified once for each window\n"
                     "                                      you create from the command line"));
  g_print ("%s\n", _("  --startup-id=STRING                 ID for the startup notification\n"
                     "                                      protocol"));
  g_print ("%s\n", _("  --fullscreen                        Set the last-specified window into\n"
                     "                                      fullscreen mode; applies to only one\n"
                     "                                      window; can be specified once for\n"
                     "                                      each window you create from the\n"
                     "                                      command line."));
  g_print ("%s\n", _("  --show-menubar                      Turn on the menubar for the last-\n"
                     "                                      specified window; applies to only one\n"
                     "                                      window; can be specified once for\n"
                     "                                      each window you create from the\n"
                     "                                      command line"));
  g_print ("%s\n", _("  --hide-menubar                      Turn off the menubar for the last-\n"
                     "                                      specified window; applies to only one\n"
                     "                                      window; can be specified once for\n"
                     "                                      each window you create from the\n"
                     "                                      command line"));
  g_print ("%s\n", _("  --show-borders                      Turn on the window decorations for\n"
                     "                                      the last-specified window; applies\n"
                     "                                      to only one window; can be specified\n"
                     "                                      once for each window you create from\n"
                     "                                      the command line"));
  g_print ("%s\n", _("  --hide-borders                      Turn off the window decorations for\n"
                     "                                      the last-specified window; applies\n"
                     "                                      to only one window; can be specified\n"
                     "                                      once for each window you create from\n"
                     "                                      the command line"));
  g_print ("%s\n", _("  --show-toolbars                     Turn on the toolbars for the last-\n"
                     "                                      specified window; applies to only one\n"
                     "                                      window; can be specified once for\n"
                     "                                      each window you create from the\n"
                     "                                      command line"));
  g_print ("%s\n", _("  --hide-toolbars                     Turn off the toolbars for the last-\n"
                     "                                      specified window; applies to only one\n"
                     "                                      window; can be specified once for\n"
                     "                                      each window you create from the\n"
                     "                                      command line"));
  g_print ("\n");
  g_print ("%s\n", _("  --tab                               Open a new tab in the last-specified\n"
                     "                                      window; more than one of these\n"
                     "                                      options can be provided"));
  g_print ("%s\n", _("  --window                            Open a new window containing one tab;\n"
                     "                                      more than one of these options can be\n"
                     "                                      provided"));
  g_print ("\n");
  g_print ("%s\n", _("  --default-display=DISPLAY           default X display to use"));
  g_print ("%s\n", _("  --default-working-directory=DIRNAME Set the default terminals working\n"
                     "                                      directory"));
  g_print ("\n");
}



int
main (int argc, char **argv)
{
  TerminalOptions *options;
  GdkModifierType  modifiers;
  TerminalApp     *app;
  const gchar     *startup_id;
  const gchar     *display;
  GError          *error = NULL;
  gchar          **nargv;
  gint             nargc;
  gint             n;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  g_set_application_name (_("Terminal"));

  /* required because we don't call gtk_init() prior to usage() */
  gtk_set_locale ();

#if 0
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
#endif

  if (!terminal_options_parse (argc, argv, NULL, &options, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      usage ();
      return EXIT_FAILURE;
    }

  if (G_UNLIKELY (options->show_version))
    {
      g_print (_("%s (Xfce %s)\n\n"
                 "Copyright (c) 2003-2006\n"
                 "        os-cillation e.K. All rights reserved.\n\n"
                 "Written by Benedikt Meurer <benny@xfce.org>.\n\n"
                 "Built with Gtk+-%d.%d.%d, running with Gtk+-%d.%d.%d.\n\n"
                 "Please report bugs to <%s>.\n"),
                 PACKAGE_STRING, xfce_version_string (),
                 GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION,
                 gtk_major_version, gtk_minor_version, gtk_micro_version,
                 PACKAGE_BUGREPORT);
      return EXIT_SUCCESS;
    }
  else if (G_UNLIKELY (options->show_help))
    {
      usage ();
      return EXIT_SUCCESS;
    }

  /* create a copy of the standard arguments with our additional stuff */
  nargv = g_new (gchar*, argc + 5); nargc = 0;
  nargv[nargc++] = g_strdup (argv[0]);
  nargv[nargc++] = g_strdup ("--default-working-directory");
  nargv[nargc++] = g_get_current_dir ();

  /* append startup if given */
  startup_id = g_getenv ("DESKTOP_STARTUP_ID");
  if (G_LIKELY (startup_id != NULL))
    {
      nargv[nargc++] = g_strdup_printf ("--startup-id=%s", startup_id);
      xfce_putenv ("DESKTOP_STARTUP_ID=");
    }

  /* append default display if given */
  display = g_getenv ("DISPLAY");
  if (G_LIKELY (display != NULL))
    nargv[nargc++] = g_strdup_printf ("--default-display=%s", display);

  /* append all given arguments */
  for (n = 1; n < argc; ++n)
    nargv[nargc++] = g_strdup (argv[n]);
  nargv[nargc] = NULL;

#ifdef HAVE_DBUS
  if (!options->disable_server)
    {
      /* try to connect to an existing Terminal service */
      if (terminal_dbus_invoke_launch (nargc, nargv, &error))
        {
          return EXIT_SUCCESS;
        }
      else
        {
#ifdef DEBUG
          g_warning ("Unable to invoke remote terminal: %s",
                     error->message);
#endif

          /* handle "User mismatch" special */
          if (error->domain == TERMINAL_ERROR
              && error->code == TERMINAL_ERROR_USER_MISMATCH)
            {
              /* don't try to establish another service here */
              options->disable_server = TRUE;
            }

          g_error_free (error);
          error = NULL;
        }
    }
#endif /* !HAVE_DBUS */

  /* disable automatic startup notification completion */
  gtk_window_set_auto_startup_notification (FALSE);

  /* initialize Gtk+ */
  gtk_init (&argc, &argv);

  /* Make GtkAccelGroup accept Mod5 (Windows Key) as modifier */
  modifiers = gtk_accelerator_get_default_mod_mask ();
  gtk_accelerator_set_default_mod_mask (modifiers | GDK_MOD4_MASK);

  /* register our stock icons */
  terminal_stock_init ();

  /* set default window icon */
  gtk_window_set_default_icon_name ("Terminal");

  app = terminal_app_new ();

#ifdef HAVE_DBUS
  if (!options->disable_server)
    {
      if (!terminal_dbus_register_service (app, &error))
        {
          g_printerr (_("Unable to register terminal service: %s\n"), error->message);
          g_error_free (error);
          error = NULL;
        }
    }
#endif /* !HAVE_DBUS */
 
  if (!terminal_app_process (app, nargv, nargc, &error))
    {
      g_printerr (_("Unable to launch terminal: %s\n"), error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  /* free parsed options */
  terminal_options_free (options);

  /* free temporary arguments */
  g_strfreev (nargv);

  gtk_main ();

  g_object_unref (G_OBJECT (app));

  return EXIT_SUCCESS;
}
