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
  GtkIconSet     *set;
  GdkPixbuf      *pixbuf;

  factory = gtk_icon_factory_new ();

  pixbuf = gdk_pixbuf_from_pixdata (&stock_newtab, FALSE, NULL);
  set = gtk_icon_set_new_from_pixbuf (pixbuf);
  gtk_icon_factory_add (factory, "terminal-newtab", set);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_set_unref (set);

  pixbuf = gdk_pixbuf_from_pixdata (&stock_newwindow, FALSE, NULL);
  set = gtk_icon_set_new_from_pixbuf (pixbuf);
  gtk_icon_factory_add (factory, "terminal-newwindow", set);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_set_unref (set);

  pixbuf = gdk_pixbuf_from_pixdata (&stock_reportbug, FALSE, NULL);
  set = gtk_icon_set_new_from_pixbuf (pixbuf);
  gtk_icon_factory_add (factory, "terminal-reportbug", set);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_set_unref (set);

  gtk_icon_factory_add_default (factory);
}



static void
usage (void)
{
  fprintf (stderr,
           _("Usage: Terminal [OPTION...]\n"
             "\n"
             "GTK+\n"
             "  --display=DISPLAY       X display to use\n"
             "\n"
             "Terminal\n"
             "  -e, --command=STRING    Execute the argument to this option\n"
             "                          inside the terminal.\n"
             "  -x, --execute           Execute the remainder of the command\n"
             "                          line inside the terminal.\n"
             "  --tab                   Open a new tab in the last-opened\n"
             "                          window.\n"
             "  --geometry=GEOMETRY     X geometry specification (see \"X\"\n"
             "                          manual page).\n"
             "  --disable-server        Do not register with the D-BUS session\n"
             "                          message bus.\n"
             "  --title=TITLE           Set the terminal's title.\n"
             "  --working-directory=DIR Set the terminal's working directory.\n"
             "  --help                  Print this help message and exit\n"
             "  --version               Print version information and exit\n"
             "\n"));
}



int
main (int argc, char **argv)
{
  TerminalOptions *options;
  TerminalApp     *app;
#if GLIB_CHECK_VERSION(2,6,0)
  const gchar     *description;
#endif
  GdkPixbuf       *icon;
  GError          *error = NULL;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  g_set_application_name (_("Terminal"));

#if GLIB_CHECK_VERSION(2,6,0)
  description = glib_check_version (GLIB_MAJOR_VERSION,
                                    GLIB_MINOR_VERSION,
                                    GLIB_MICRO_VERSION);
  if (G_UNLIKELY (description != NULL))
    g_warning ("GLib version mismatch: %s", description);
#endif

  options = terminal_options_from_args (argc, argv, &error);
  if (options == NULL) 
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      usage ();
      return EXIT_FAILURE;
    }

  if (G_UNLIKELY (options->flags & TERMINAL_FLAGS_SHOWVERSION))
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
  else if (G_UNLIKELY (options->flags & TERMINAL_FLAGS_SHOWHELP))
    {
      usage ();
      return EXIT_SUCCESS;
    }

  if ((options->flags & TERMINAL_FLAGS_DISABLESERVER) == 0)
    {
      if (terminal_app_try_existing (options))
        return EXIT_SUCCESS;
    }

  gtk_init (&argc, &argv);
  stock_icons_init ();

  icon = xfce_themed_icon_load ("terminal", 48);
  if (G_LIKELY (icon != NULL))
    {
      gtk_window_set_default_icon (icon);
      g_object_unref (G_OBJECT (icon));
    }

  app = terminal_app_new ();

  if ((options->flags & TERMINAL_FLAGS_DISABLESERVER) == 0)
    {
      if (!terminal_app_start_server (app, &error))
        {
          g_printerr (_("Unable to start terminal server: %s\n"), error->message);
          g_error_free (error);
          error = NULL;
        }
    }
 
  if (!terminal_app_launch_with_options (app, options, &error))
    {
      g_printerr (_("Unable to launch terminal: %s\n"), error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  gtk_main ();

  g_object_unref (G_OBJECT (app));

  return EXIT_SUCCESS;
}
