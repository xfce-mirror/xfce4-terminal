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
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <stdlib.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <terminal/terminal-accel-map.h>
#include <terminal/terminal-app.h>
#include <terminal/terminal-config.h>
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-window.h>



static void               terminal_app_class_init       (TerminalAppClass   *klass);
static void               terminal_app_init             (TerminalApp        *app);
static void               terminal_app_finalize         (GObject            *object);
static void               terminal_app_update_accels    (TerminalApp        *app);
static void               terminal_app_unregister       (DBusConnection     *connection,
                                                         void               *user_data);
static DBusHandlerResult  terminal_app_message          (DBusConnection     *connection,
                                                         DBusMessage        *message,
                                                         void               *user_data);
static void               terminal_app_new_window       (TerminalWindow     *window,
                                                         const gchar        *working_directory,
                                                         TerminalApp        *app);
static void               terminal_app_window_destroyed (GtkWidget          *window,
                                                         TerminalApp        *app);
static void               terminal_app_save_yourself    (ExoXsessionClient  *client,
                                                         TerminalApp        *app);
static GdkScreen         *terminal_app_find_screen      (const gchar        *display_name);



struct _TerminalAppClass
{
  GObjectClass  __parent__;
};

struct _TerminalApp
{
  GObject              __parent__;
  TerminalPreferences *preferences;
  TerminalAccelMap    *accel_map;
  ExoXsessionClient   *session_client;
  gchar               *initial_menu_bar_accel;
  GList               *windows;
  guint                server_running : 1;
};



static GObjectClass *parent_class;

static const struct DBusObjectPathVTable terminal_app_vtable =
{
  terminal_app_unregister,
  terminal_app_message,
  NULL,
};



G_DEFINE_TYPE (TerminalApp, terminal_app, G_TYPE_OBJECT);



static void
terminal_app_class_init (TerminalAppClass *klass)
{
  GObjectClass *gobject_class;
  
  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_app_finalize;
}



static void
terminal_app_init (TerminalApp *app)
{
  app->preferences = terminal_preferences_get ();
  g_signal_connect_swapped (G_OBJECT (app->preferences), "notify::shortcuts-no-menukey",
                            G_CALLBACK (terminal_app_update_accels), app);

  /* remember the original menu bar accel */
  g_object_get (G_OBJECT (gtk_settings_get_default ()),
                "gtk-menu-bar-accel", &app->initial_menu_bar_accel,
                NULL);

  terminal_app_update_accels (app);

  /* connect the accel map */
  app->accel_map = terminal_accel_map_new ();
}



static void
terminal_app_finalize (GObject *object)
{
  TerminalApp *app = TERMINAL_APP (object);
  GList       *lp;

  for (lp = app->windows; lp != NULL; lp = lp->next)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (lp->data), G_CALLBACK (terminal_app_window_destroyed), app);
      g_signal_handlers_disconnect_by_func (G_OBJECT (lp->data), G_CALLBACK (terminal_app_new_window), app);
      gtk_widget_destroy (GTK_WIDGET (lp->data));
    }
  g_list_free (app->windows);

  g_signal_handlers_disconnect_by_func (G_OBJECT (app->preferences),
                                        G_CALLBACK (terminal_app_update_accels),
                                        app);
  g_object_unref (G_OBJECT (app->preferences));

  if (app->initial_menu_bar_accel != NULL)
    g_free (app->initial_menu_bar_accel);

  if (app->session_client != NULL)
    g_object_unref (G_OBJECT (app->session_client));

  g_object_unref (G_OBJECT (app->accel_map));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}



static void
terminal_app_update_accels (TerminalApp *app)
{
  const gchar *accel;
  gboolean     shortcuts_no_menukey;

  g_object_get (G_OBJECT (app->preferences), 
                "shortcuts-no-menukey", &shortcuts_no_menukey,
                NULL);

  if (shortcuts_no_menukey)
    accel = "<Shift><Control><Mod1><Mod2><Mod3><Mod4><Mod5>F10";
  else
    accel = app->initial_menu_bar_accel;

  gtk_settings_set_string_property (gtk_settings_get_default (),
                                    "gtk-menu-bar-accel", accel,
                                    "Terminal");
}



static void
terminal_app_unregister (DBusConnection *connection,
                         void           *user_data)
{
}



static DBusHandlerResult
terminal_app_message (DBusConnection  *connection,
                      DBusMessage     *message,
                      void            *user_data)
{
  DBusMessageIter  iter;
  TerminalApp     *app = TERMINAL_APP (user_data);
  DBusMessage     *reply;
  GError          *error = NULL;
  gchar          **argv;
  gint             argc;

  if (dbus_message_is_method_call (message, 
                                   TERMINAL_DBUS_INTERFACE,
                                   TERMINAL_DBUS_METHOD_LAUNCH))
    {
      if (!dbus_message_iter_init (message, &iter))
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

      if (!dbus_message_iter_get_string_array (&iter, &argv, &argc))
        {
          reply = dbus_message_new_error (message, TERMINAL_DBUS_ERROR, "Invalid arguments");
          dbus_connection_send (connection, reply, NULL);
          dbus_message_unref (reply);
          return DBUS_HANDLER_RESULT_HANDLED;
        }

      if (!terminal_app_process (app, argv, argc, &error))
        {
          reply = dbus_message_new_error (message, TERMINAL_DBUS_ERROR, error->message);
          dbus_connection_send (connection, reply, NULL);
          dbus_message_unref (reply);
        }
      else
        {
          reply = dbus_message_new_method_return (message);
          dbus_connection_send (connection, reply, NULL);
        }

      dbus_free_string_array (argv);
      dbus_message_unref (reply);
    }
  else if (dbus_message_is_signal (message,
                                   DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL,
                                   "Disconnected"))
    {
      g_printerr (_("D-BUS message bus disconnected, exiting...\n"));
      gtk_main_quit ();
    }
  else
    {
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

  return DBUS_HANDLER_RESULT_HANDLED;
}



static void
terminal_app_new_window (TerminalWindow *window,
                         const gchar    *working_directory,
                         TerminalApp    *app)
{
  TerminalWindowAttr *win_attr;
  TerminalTabAttr    *tab_attr;
  GdkScreen          *screen;

  screen = gtk_window_get_screen (GTK_WINDOW (window));

  win_attr = terminal_window_attr_new ();
  win_attr->display = gdk_screen_make_display_name (screen);
  tab_attr = win_attr->tabs->data;
  tab_attr->directory = g_strdup (working_directory);

  terminal_app_open_window (app, win_attr);

  terminal_window_attr_free (win_attr);
}



static void
terminal_app_window_destroyed (GtkWidget   *window,
                               TerminalApp *app)
{
  g_return_if_fail (g_list_find (app->windows, window) != NULL);

  app->windows = g_list_remove (app->windows, window);

  if (G_UNLIKELY (app->windows == NULL))
    gtk_main_quit ();
}



static void
terminal_app_save_yourself (ExoXsessionClient *client,
                            TerminalApp       *app)
{
  GList               *result = NULL;
  GList               *lp;
  gchar              **argv;
  gint                 argc;
  gint                 n;

  for (lp = app->windows; lp != NULL; lp = lp->next)
    {
      if (lp != app->windows)
        result = g_list_append (result, g_strdup ("--window"));
      result = g_list_concat (result, terminal_window_get_restart_command (lp->data));
    }

  argc = g_list_length (result) + 1;
  argv = g_new (gchar*, argc + 1);
  argv[0] = g_strdup ("Terminal");
  for (lp = result, n = 1; n < argc; lp = lp->next, ++n)
    argv[n] = lp->data;
  argv[n] = NULL;

  exo_xsession_client_set_restart_command (client, argv, argc);

  g_list_free (result);
  g_strfreev (argv);
}



static GdkScreen*
terminal_app_find_screen (const gchar *display_name)
{
  const gchar *other_name;
  GdkDisplay  *display = NULL;
  GdkScreen   *screen = NULL;
  GSList      *displays;
  GSList      *dp;
  gulong       n;
  gchar       *period;
  gchar       *name;
  gchar       *end;
  gint         num = -1;

  if (display_name != NULL)
    {
      name = g_strdup (display_name);
      period = strrchr (name, '.');
      if (period != NULL)
        {
          errno = 0;
          *period++ = '\0';
          end = period;
          n = strtoul (period, &end, 0);
          if (errno == 0 && period != end)
            num = n;
        }

      displays = gdk_display_manager_list_displays (gdk_display_manager_get ());
      for (dp = displays; dp != NULL; dp = dp->next)
        {
          display = dp->data;
          other_name = gdk_display_get_name (display);
          if (strncmp (other_name, name, strlen (name)) == 0)
            break;
        }
      g_slist_free (displays);

      if (display == NULL)
        display = gdk_display_open (display_name);

      if (display != NULL)
        {
          if (num >= 0)
            screen = gdk_display_get_screen (display, num);

          if (screen == NULL)
            screen = gdk_display_get_default_screen (display);

          if (screen != NULL)
            g_object_ref (G_OBJECT (screen));
        }

      g_free (name);
    }

  if (screen == NULL)
    {
      screen = gdk_screen_get_default ();
      g_object_ref (G_OBJECT (screen));
    }

  return screen;
}



/**
 * terminal_app_new:
 *
 * Return value :
 **/
TerminalApp*
terminal_app_new (void)
{
  return g_object_new (TERMINAL_TYPE_APP, NULL);
}



/**
 * terminal_app_start_server:
 * @app         : A #TerminalApp.
 * @error       : The location to store the error to, or %NULL.
 *
 * Return value : %TRUE on success, %FALSE on failure.
 **/
gboolean
terminal_app_start_server (TerminalApp *app,
                           GError     **error)
{
  DBusConnection *connection;
  DBusError       derror;
  
  g_return_val_if_fail (TERMINAL_IS_APP (app), FALSE);

  if (G_UNLIKELY (app->server_running))
    return TRUE;

  connection = exo_dbus_bus_connection ();
  if (G_UNLIKELY (connection == NULL))
    {
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   _("Unable to connect to D-BUS message daemon"));
      return FALSE;
    }

  dbus_error_init (&derror);

  if (!dbus_bus_acquire_service (connection, TERMINAL_DBUS_SERVICE, 0, &derror))
    {
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   _("Unable to acquire service %s: %s"),
                   TERMINAL_DBUS_SERVICE, derror.message);
      dbus_error_free (&derror);
      return FALSE;
    }

  if (dbus_connection_register_object_path (connection,
                                            TERMINAL_DBUS_PATH,
                                            &terminal_app_vtable,
                                            app) < 0)
    {
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   _("Unable to register object %s\n"),
                   TERMINAL_DBUS_PATH);
      return FALSE;
    }

  app->server_running = 1;

  return TRUE;
}



/**
 * terminal_app_process:
 * @app
 * @argv
 * @argc
 * @error
 *
 * Return value:
 **/
gboolean
terminal_app_process (TerminalApp  *app,
                      gchar       **argv,
                      gint          argc,
                      GError      **error)
{
  GList  *attrs;
  GList  *lp;

  if (!terminal_options_parse (argc, argv, &attrs, NULL, error))
    return FALSE;

  for (lp = attrs; lp != NULL; lp = lp->next)
    terminal_app_open_window (app, lp->data);

  g_list_foreach (attrs, (GFunc) terminal_window_attr_free, NULL);
  g_list_free (attrs);

  return TRUE;
}



/**
 * terminal_app_open_window:
 * @app   : A #TerminalApp object.
 * @attr  : The attributes for the new window.
 **/
void
terminal_app_open_window (TerminalApp         *app,
                          TerminalWindowAttr  *attr)
{
  TerminalTabAttr *tab_attr;
  GdkDisplay      *display;
  GdkWindow       *leader;
  GtkWidget       *window;
  GtkWidget       *terminal;
  GdkScreen       *screen;
  GList           *lp;

  g_return_if_fail (TERMINAL_IS_APP (app));
  g_return_if_fail (attr != NULL);

  window = terminal_window_new (attr->fullscreen,
                                attr->menubar,
                                attr->borders,
                                attr->toolbars);
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (terminal_app_window_destroyed), app);
  g_signal_connect (G_OBJECT (window), "new-window",
                    G_CALLBACK (terminal_app_new_window), app);
  app->windows = g_list_append (app->windows, window);

  if (attr->role != NULL)
    gtk_window_set_role (GTK_WINDOW (window), attr->role);
  if (attr->startup_id != NULL)
    terminal_window_set_startup_id (TERMINAL_WINDOW (window), attr->startup_id);

  screen = terminal_app_find_screen (attr->display);
  if (G_LIKELY (screen != NULL))
    {
      gtk_window_set_screen (GTK_WINDOW (window), screen);
      g_object_unref (G_OBJECT (screen));
    }

  for (lp = attr->tabs; lp != NULL; lp = lp->next)
    {
      terminal = terminal_screen_new ();

      tab_attr = lp->data;
      if (tab_attr->command != NULL)
        terminal_screen_set_custom_command (TERMINAL_SCREEN (terminal), tab_attr->command);
      if (tab_attr->directory != NULL)
        terminal_screen_set_working_directory (TERMINAL_SCREEN (terminal), tab_attr->directory);
      if (tab_attr->title != NULL)
        terminal_screen_set_custom_title (TERMINAL_SCREEN (terminal), tab_attr->title);

      terminal_window_add (TERMINAL_WINDOW (window), TERMINAL_SCREEN (terminal));

      /* if this was the first tab, we set the geometry string now
       * and show the window. This is required to work around a hang
       * in Gdk, which I failed to figure the cause til now.
       */
      if (lp == attr->tabs)
        {
          if (attr->geometry != NULL && !gtk_window_parse_geometry (GTK_WINDOW (window), attr->geometry))
            g_printerr (_("Invalid geometry string \"%s\"\n"), attr->geometry);
          gtk_widget_show (window);
        }

      terminal_screen_launch_child (TERMINAL_SCREEN (terminal));
    }

  /* register with session manager on first display
   * opened by Terminal. This is to ensure that we
   * get restarted by only _one_ session manager (the
   * one running on the first display).
   */
  if (G_UNLIKELY (app->session_client == NULL))
    {
      display = gtk_widget_get_display (GTK_WIDGET (window));
      leader = gdk_display_get_default_group (display);
      app->session_client = exo_xsession_client_new_with_group (leader);
      g_signal_connect (G_OBJECT (app->session_client), "save-yourself",
                        G_CALLBACK (terminal_app_save_yourself), app);
    }
}



/**
 * terminal_app_try_invoke:
 * @argc
 * @argv
 * @error
 *
 * Return value:
 **/
gboolean
terminal_app_try_invoke (gint              argc,
                         gchar           **argv,
                         GError          **error)
{
  DBusMessageIter  iter;
  DBusConnection  *connection;
  DBusMessage     *message;
  DBusMessage     *result;
  DBusError        derror;

  connection = exo_dbus_bus_connection ();
  if (G_UNLIKELY (connection == NULL))
    {
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   "No session message bus daemon running");
      return FALSE;
    }

  message = dbus_message_new_method_call (TERMINAL_DBUS_SERVICE,
                                          TERMINAL_DBUS_PATH,
                                          TERMINAL_DBUS_INTERFACE,
                                          TERMINAL_DBUS_METHOD_LAUNCH);
  dbus_message_set_auto_activation (message, FALSE);
  dbus_message_append_iter_init (message, &iter);
  dbus_message_iter_append_string_array (&iter, (const gchar **) argv, argc);

  dbus_error_init (&derror);
  result = dbus_connection_send_with_reply_and_block (connection,
                                                      message,
                                                      2000, &derror);
  dbus_message_unref (message);

  if (result == NULL)
    {
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   "%s", derror.message);
      dbus_error_free (&derror);
      return FALSE;
    }

  if (dbus_message_is_error (result, TERMINAL_DBUS_ERROR))
    {
      dbus_set_error_from_message (&derror, result);
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   "%s", derror.message);
      dbus_message_unref (result);
      dbus_error_free (&derror);
      return FALSE;
    }

  dbus_message_unref (result);
  return TRUE;
}




