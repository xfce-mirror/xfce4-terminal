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

#include <stdlib.h>

#include <exo/exo.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gdk-pixbuf/gdk-pixdata.h>

#include <terminal/stock-icons.h>
#include <terminal/terminal-app.h>



static void
stock_icons_init (void)
{
  GtkIconFactory *factory;
  GtkIconSource  *source;
  GtkIconSet     *set;
  GdkPixbuf      *pixbuf;

  factory = gtk_icon_factory_new ();

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_advanced48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-advanced", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_appearance48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-appearance", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_closetab16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_closetab24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-closetab", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_closewindow16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_closewindow24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-closewindow", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_colors48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-colors", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_compose48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-compose", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_fullscreen24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-fullscreen", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_general48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-general", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_newtab16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_newtab24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-newtab", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_newwindow16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_newwindow24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-newwindow", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_reportbug16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_reportbug24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-reportbug", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_shortcuts48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-shortcuts", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_showborders24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-showborders", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_from_pixdata (&stock_showmenu24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (factory, "terminal-showmenu", set);
  gtk_icon_set_unref (set);

  gtk_icon_factory_add_default (factory);
}



static void
usage (FILE *fp)
{
  fprintf (fp, "%s\n", _("Usage: Terminal [OPTION...]"));
  fprintf (fp, "\n");
  fprintf (fp, "%s\n", _("  -h, --help                          Print this help message and exit"));
  fprintf (fp, "%s\n", _("  -v, --version                       Print version information and exit"));
  fprintf (fp, "%s\n", _("  --disable-server                    Do not register with the D-BUS\n"
                         "                                      session message bus"));
  fprintf (fp, "\n");
  fprintf (fp, "%s\n", _("  -x, --execute                       Execute the remainder of the command\n"
                         "                                      line inside the terminal"));
  fprintf (fp, "%s\n", _("  -e, --command=STRING                Execute the argument to this option\n"
                         "                                      inside the terminal"));
  fprintf (fp, "%s\n", _("  --working-directory=DIRNAME         Set the terminals working directory"));
  fprintf (fp, "%s\n", _("  -T, --title=TITLE                   Set the terminals title"));
  fprintf (fp, "\n");
  fprintf (fp, "%s\n", _("  --display=DISPLAY                   X display to use for the last-\n"
                         "                                      specified window"));
  fprintf (fp, "%s\n", _("  --geometry=GEOMETRY                 X geometry specification (see \"X\"\n"
                         "                                      man page), can be specified once per\n"
                         "                                      window to be opened"));
  fprintf (fp, "%s\n", _("  --role=ROLE                         Set the role for the last-specified;\n"
                         "                                      window; applies to only one window;\n"
                         "                                      can be specified once for each window\n"
                         "                                      you create from the command line"));
  fprintf (fp, "%s\n", _("  --startup-id=STRING                 ID for the startup notification\n"
                         "                                      protocol"));
  fprintf (fp, "%s\n", _("  --fullscreen                        Set the last-specified window into\n"
                         "                                      fullscreen mode; applies to only one\n"
                         "                                      window; can be specified once for\n"
                         "                                      each window you create from the\n"
                         "                                      command line."));
  fprintf (fp, "%s\n", _("  --show-menubar                      Turn on the menubar for the last-\n"
                         "                                      specified window; applies to only one\n"
                         "                                      window; can be specified once for\n"
                         "                                      each window you create from the\n"
                         "                                      command line"));
  fprintf (fp, "%s\n", _("  --hide-menubar                      Turn off the menubar for the last-\n"
                         "                                      specified window; applies to only one\n"
                         "                                      window; can be specified once for\n"
                         "                                      each window you create from the\n"
                         "                                      command line"));
  fprintf (fp, "%s\n", _("  --show-borders                      Turn on the window decorations for\n"
                         "                                      the last-specified window; applies\n"
                         "                                      to only one window; can be specified\n"
                         "                                      once for each window you create from\n"
                         "                                      the command line"));
  fprintf (fp, "%s\n", _("  --hide-borders                      Turn off the window decorations for\n"
                         "                                      the last-specified window; applies\n"
                         "                                      to only one window; can be specified\n"
                         "                                      once for each window you create from\n"
                         "                                      the command line"));
  fprintf (fp, "%s\n", _("  --show-toolbars                     Turn on the toolbars for the last-\n"
                         "                                      specified window; applies to only one\n"
                         "                                      window; can be specified once for\n"
                         "                                      each window you create from the\n"
                         "                                      command line"));
  fprintf (fp, "%s\n", _("  --hide-toolbars                     Turn off the toolbars for the last-\n"
                         "                                      specified window; applies to only one\n"
                         "                                      window; can be specified once for\n"
                         "                                      each window you create from the\n"
                         "                                      command line"));
  fprintf (fp, "\n");
  fprintf (fp, "%s\n", _("  --tab                               Open a new tab in the last-specified\n"
                         "                                      window; more than one of these\n"
                         "                                      options can be provided"));
  fprintf (fp, "%s\n", _("  --window                            Open a new window containing one tab;\n"
                         "                                      more than one of these options can be\n"
                         "                                      provided"));
  fprintf (fp, "\n");
  fprintf (fp, "%s\n", _("  --default-display=DISPLAY           default X display to use"));
  fprintf (fp, "%s\n", _("  --default-working-directory=DIRNAME Set the default terminals working\n"
                         "                                      directory"));
  fprintf (fp, "\n");
}



int
main (int argc, char **argv)
{
  TerminalOptions *options;
  TerminalApp     *app;
#if GLIB_CHECK_VERSION(2,6,0)
  const gchar     *description;
#endif
  const gchar     *startup_id;
  const gchar     *display;
  GdkPixbuf       *icon;
  GError          *error = NULL;
  gchar          **nargv;
  gint             nargc;
  gint             n;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  g_set_application_name (_("Terminal"));

#if 0
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
#endif

#if GLIB_CHECK_VERSION(2,6,0)
  description = glib_check_version (GLIB_MAJOR_VERSION,
                                    GLIB_MINOR_VERSION,
                                    GLIB_MICRO_VERSION);
  if (G_UNLIKELY (description != NULL))
    g_warning ("GLib version mismatch: %s", description);
#endif

  if (!terminal_options_parse (argc, argv, NULL, &options, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      usage (stderr);
      return EXIT_FAILURE;
    }

  if (G_UNLIKELY (options->show_version))
    {
      printf (_("%s (Xfce %s)\n\n"
                "Copyright (c) 2003-2004\n"
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
      usage (stdout);
      return EXIT_SUCCESS;
    }

  /* create a copy of the standard arguments with our additional stuff */
  nargv = g_new (gchar*, argc + 5); nargc = 0;
  nargv[nargc++] = g_strdup (argv[0]);
  nargv[nargc++] = g_strdup ("--default-working-directory");
  nargv[nargc++] = g_get_current_dir ();

  /* append startup if given */
  startup_id = g_getenv ("DESKTOP_STARTUP_ID");
  if (startup_id != NULL)
    {
      nargv[nargc++] = g_strdup_printf ("--startup-id=%s", startup_id);
      xfce_putenv ("DESKTOP_STARTUP_ID=");
    }

  /* append default display if given */
  display = g_getenv ("DISPLAY");
  if (display != NULL)
    nargv[nargc++] = g_strdup_printf ("--default-display=%s", display);

  /* append all given arguments */
  for (n = 1; n < argc; ++n)
    nargv[nargc++] = g_strdup (argv[n]);
  nargv[nargc] = NULL;

  if (!options->disable_server)
    {
      /* try to connect to an existing Terminal service */
      if (terminal_app_try_invoke (nargc, nargv, &error))
        {
          return EXIT_SUCCESS;
        }
      else
        {
#ifdef DEBUG
          g_warning ("Unable to invoke remote terminal: %s",
                     error->message);
#endif
          g_error_free (error);
          error = NULL;
        }
    }

  /* disable automatic startup notification completion */
  gtk_window_set_auto_startup_notification (FALSE);

  gtk_init (&argc, &argv);
  stock_icons_init ();

  icon = gdk_pixbuf_from_pixdata (&stock_general48, FALSE, NULL);
  gtk_window_set_default_icon (icon);
  g_object_unref (G_OBJECT (icon));

  app = terminal_app_new ();

  if (!options->disable_server)
    {
      if (!terminal_app_start_server (app, &error))
        {
          g_printerr (_("Unable to start terminal server: %s\n"), error->message);
          g_error_free (error);
          error = NULL;
        }
    }
 
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
