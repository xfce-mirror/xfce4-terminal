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

#include <libxfce4util/libxfce4util.h>

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

  /* open the about dialog */
  gtk_show_about_dialog (parent,
                         "authors", authors,
                         "artists", artists,
                         "comments", _("Xfce Terminal Emulator"),
                         "documenters", documenters,
                         "copyright", "Copyright \302\251 2003-2008 Benedikt Meurer\n"
                                      "Copyright \302\251 2007-2012 Nick Schermer",
                         "license", XFCE_LICENSE_GPL,
                         "logo-icon-name", "utilities-terminal",
                         "program-name", g_get_prgname (),
                         "translator-credits", _("translator-credits"),
                         "version", PACKAGE_VERSION,
                         "website", "http://goodies.xfce.org/projects/applications/terminal",
                         "website-label", _("Visit Terminal website"),
                         NULL);
}
