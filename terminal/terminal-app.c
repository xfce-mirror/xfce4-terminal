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

#include <terminal/terminal-accel-map.h>
#include <terminal/terminal-app.h>
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-window.h>



static void               terminal_app_class_init       (TerminalAppClass *klass);
static void               terminal_app_init             (TerminalApp      *app);
static void               terminal_app_finalize         (GObject          *object);
static void               terminal_app_update_accels    (TerminalApp      *app);
static void               terminal_app_unregister       (DBusConnection   *connection,
                                                         void             *user_data);
static DBusHandlerResult  terminal_app_message          (DBusConnection   *connection,
                                                         DBusMessage      *message,
                                                         void             *user_data);
static void               terminal_app_new_window       (TerminalWindow   *window,
                                                         const gchar      *working_directory,
                                                         TerminalApp      *app);
static void               terminal_app_window_destroyed (GtkWidget        *window,
                                                         TerminalApp      *app);



struct _TerminalApp
{
  GObject              __parent__;
  TerminalPreferences *preferences;
  TerminalAccelMap    *accel_map;
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
  TerminalOptions *options;
  TerminalApp     *app = TERMINAL_APP (user_data);
  DBusMessage     *reply;
  gboolean         result;
  GError          *error = NULL;

  if (dbus_message_is_method_call (message, 
                                   TERMINAL_APP_INTERFACE,
                                   TERMINAL_APP_METHOD_LAUNCH))
    {
      options = terminal_options_from_message (message);
      result = terminal_app_launch_with_options (app, options, &error);
      terminal_options_free (options);

      if (!result)
        {
          g_printerr (_("TerminalServer: %s\n"), error->message);
          g_error_free (error);
        }

      reply = dbus_message_new_method_return (message);
      dbus_connection_send (connection, reply, NULL);
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
  TerminalOptions options;

  options.mask = TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY;
  options.working_directory = (gchar *) working_directory;

  terminal_app_launch_with_options (app, &options, NULL);
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

  if (!dbus_bus_acquire_service (connection, TERMINAL_APP_SERVICE, 0, &derror))
    {
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   _("Unable to acquire service %s: %s"),
                   TERMINAL_APP_SERVICE, derror.message);
      dbus_error_free (&derror);
      return FALSE;
    }

  if (dbus_connection_register_object_path (connection,
                                            TERMINAL_APP_PATH,
                                            &terminal_app_vtable,
                                            app) < 0)
    {
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   _("Unable to register object %s\n"),
                   TERMINAL_APP_PATH);
      return FALSE;
    }

  app->server_running = 1;

  return TRUE;
}



/**
 * terminal_app_launch_with_options:
 * @app         : A #TerminalApp.
 * @options     :
 * @error       : Location to store error to, or %NULL.
 *
 * Return value : %TRUE on success, %FALSE on error.
 **/
gboolean
terminal_app_launch_with_options (TerminalApp     *app,
                                  TerminalOptions *options,
                                  GError         **error)
{
  GtkWidget *terminal;
  GtkWidget *window;

  g_return_val_if_fail (TERMINAL_IS_APP (app), FALSE);

  /* setup the terminal widget */
  terminal = terminal_widget_new ();
  if (options != NULL)
    {
      if (options->mask & TERMINAL_OPTIONS_MASK_COMMAND)
        terminal_widget_set_custom_command (TERMINAL_WIDGET (terminal), options->command);
      if (options->mask & TERMINAL_OPTIONS_MASK_TITLE)
        terminal_widget_set_custom_title (TERMINAL_WIDGET (terminal), options->title);
      if (options->mask & TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY)
        terminal_widget_set_working_directory (TERMINAL_WIDGET (terminal), options->working_directory);
    }

  /* setup the window */
  if (options != NULL && (options->flags & TERMINAL_FLAGS_OPENTAB) != 0 && app->windows != NULL)
    {
      window = GTK_WIDGET (g_list_last (app->windows)->data);
    }
  else
    {
      window = terminal_window_new ();
      g_signal_connect (G_OBJECT (window), "destroy",
                        G_CALLBACK (terminal_app_window_destroyed), app);
      g_signal_connect (G_OBJECT (window), "new-window",
                        G_CALLBACK (terminal_app_new_window), app);
      app->windows = g_list_append (app->windows, window);
    }

  terminal_window_add (TERMINAL_WINDOW (window), TERMINAL_WIDGET (terminal));
  terminal_widget_launch_child (TERMINAL_WIDGET (terminal));

  if (options != NULL)
    {
      if (options->mask & TERMINAL_OPTIONS_MASK_GEOMETRY)
        {
          if (!gtk_window_parse_geometry (GTK_WINDOW (window), options->geometry))
            g_printerr (_("Invalid geometry string \"%s\"\n"), options->geometry);
        }
    }

  gtk_widget_show (window);

  return TRUE;
}



/**
 * terminal_app_try_existing:
 * @options     : Pointer to a #TerminalOptions struct.
 *
 * Return value : %TRUE on success, %FALSE on error.
 **/
gboolean
terminal_app_try_existing (TerminalOptions *options)
{
  DBusMessageIter iter;
  DBusConnection *connection;
  DBusMessage    *message;

  g_return_val_if_fail (options != NULL, FALSE);

  connection = exo_dbus_bus_connection ();
  if (connection == NULL)
    return FALSE;

  message = dbus_message_new_method_call (TERMINAL_APP_SERVICE,
                                          TERMINAL_APP_PATH,
                                          TERMINAL_APP_INTERFACE,
                                          TERMINAL_APP_METHOD_LAUNCH);
  dbus_message_set_auto_activation (message, FALSE);
  dbus_message_append_iter_init (message, &iter);
  dbus_message_iter_append_uint32 (&iter, options->mask);

  /* FIXME: Move this to terminal_options_pack() or sth like that */
  if (options->mask & TERMINAL_OPTIONS_MASK_COMMAND)
    dbus_message_iter_append_string_array (&iter, (const gchar **) options->command, options->command_len);
  if (options->mask & TERMINAL_OPTIONS_MASK_TITLE)
    dbus_message_iter_append_string (&iter, options->title);
  if (options->mask & TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY)
    dbus_message_iter_append_string (&iter, options->working_directory);
  if (options->mask & TERMINAL_OPTIONS_MASK_FLAGS)
    dbus_message_iter_append_uint32 (&iter, options->flags);
  if (options->mask & TERMINAL_OPTIONS_MASK_GEOMETRY)
    dbus_message_iter_append_string (&iter, options->geometry);

  if (!dbus_connection_send_with_reply_and_block (connection, message, 2000, NULL))
    {
      dbus_message_unref (message);
      return FALSE;
    }

  dbus_message_unref (message);

  return TRUE;
}



