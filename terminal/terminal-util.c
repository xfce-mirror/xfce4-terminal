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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#ifdef HAVE_LIBX11
#include <X11/Xlib.h>
#endif
#ifdef HAVE_GTK_LAYER_SHELL
#include <gtk-layer-shell/gtk-layer-shell.h>
#endif

#include <terminal/terminal-util.h>
#include <terminal/terminal-private.h>



/**
 * terminal_util_show_about_dialog:
 * @parent : the parent #GtkWindow or %NULL.
 * @title  : the software title.
 * @format : the printf()-style format for the main text in the about dialog.
 * @...    : argument list for the @format.
 *
 * Displays the Terminal about dialog with @format as main text.
 **/
void
terminal_util_show_about_dialog (GtkWindow *parent)
{
  static const gchar *authors[] =
  {
    "Benedikt Meurer <benny@xfce.org>",
    "Nick Schermer <nick@xfce.org>",
    "Igor Zakharov <f2404@yandex.ru>",
    "Sergios - Anestis Kefalidis <sergioskefalidis@gmail.com>",
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
                         "copyright", "Copyright \302\251 2003-2023 The Xfce development team",
                         "license", XFCE_LICENSE_GPL,
                         "logo-icon-name", "org.xfce.terminal",
                         "program-name", PACKAGE_NAME,
                         "translator-credits", _("translator-credits"),
                         "version", PACKAGE_VERSION,
                         "website", "https://docs.xfce.org/apps/terminal/start",
                         "website-label", _("Visit Xfce Terminal website"),
                         NULL);
}



#ifdef HAVE_GTK_LAYER_SHELL
static void
is_active_changed (GtkWindow *window)
{
  g_signal_handlers_disconnect_by_func (window, is_active_changed, NULL);
  gtk_layer_set_keyboard_mode (window, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
}
#endif



void
terminal_util_activate_window (GtkWindow *window)
{
  terminal_return_if_fail (GTK_IS_WINDOW (window));
  terminal_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (window)));

  /* leave if the window is already active */
  if (gtk_window_is_active (window))
    return;

#ifdef HAVE_LIBX11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    {
      guint32              timestamp;
      XClientMessageEvent  event;
      GdkWindow           *gdk_window = gtk_widget_get_window (GTK_WIDGET (window));
      GdkDisplay          *display = gdk_window_get_display (gdk_window);

      timestamp = gtk_get_current_event_time ();
      if (timestamp == 0)
        timestamp = gdk_x11_get_server_time (gdk_window);

      /* we need a slightly custom version of the call through Gtk+ to
       * properly focus the panel when a plugin calls
       * xfce_panel_plugin_focus_widget() */
      event.type = ClientMessage;
      event.window = GDK_WINDOW_XID (gdk_window);
      event.message_type = gdk_x11_get_xatom_by_name ("_NET_ACTIVE_WINDOW");
      event.format = 32;
      event.data.l[0] = 1; /* app */
      event.data.l[1] = timestamp;
      event.data.l[2] = event.data.l[3] = event.data.l[4] = 0;

      gdk_x11_display_error_trap_push (display);

      XSendEvent (gdk_x11_get_default_xdisplay (),
                  gdk_x11_get_default_root_xwindow (), False,
                  StructureNotifyMask, (XEvent *) &event);

      gdk_display_flush (display);

      if (gdk_x11_display_error_trap_pop (display) != 0)
        g_critical ("Failed to focus window");
    }
  else
#endif
#ifdef HAVE_GTK_LAYER_SHELL
  if (gtk_layer_is_supported () && gtk_layer_is_layer_window (window))
    {
      gtk_layer_set_keyboard_mode (window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
      g_signal_connect (window, "notify::is-active", G_CALLBACK (is_active_changed), NULL);
    }
  else
#endif
    /* our best guess on non-x11 and non layer-shell clients */
    gtk_window_present (window);
}



void
terminal_util_free_data (gpointer  data,
                         GClosure *closure)
{
  g_free (data);
}
