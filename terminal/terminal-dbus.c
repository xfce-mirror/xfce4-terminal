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
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gmodule.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <libxfce4util/libxfce4util.h>

#include <terminal/terminal-config.h>
#include <terminal/terminal-dbus.h>



static DBusHandlerResult
handle_message (DBusConnection *connection,
                DBusMessage    *message,
                void           *user_data)
{
  DBusMessageIter  iter;
  TerminalApp     *app = TERMINAL_APP (user_data);
  DBusMessage     *reply;
  GError          *error = NULL;
  uid_t            user_id;
  gchar          **argv;
  gint             argc;

  /* The D-BUS interface currently looks like this:
   *
   * The Terminal service offers a method "Launch", with
   * the following parameters:
   *
   *  - UserId:int64
   *    This is the real user id of the calling process.
   *
   *  - Arguments:string array
   *    The argument array as passed to main(), with some
   *    additions.
   *
   * "Launch" returns an empty reply message if everything
   * worked, and an error message if anything failed. The
   * error can be TERMINAL_DBUS_ERROR_USER if the user ids
   * doesn't match, where the caller is expected to spawn
   * a new terminal of its own and do _NOT_ establish a
   * D-BUS service, or TERMINAL_DBUS_ERROR_GENERAL, which
   * indicates a general problem, e.g. invalid parameters
   * or something like that.
   */

  if (dbus_message_is_method_call (message, 
                                   TERMINAL_DBUS_INTERFACE,
                                   TERMINAL_DBUS_METHOD_LAUNCH))
    {
      if (!dbus_message_iter_init (message, &iter))
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

      /* check user id */
      user_id = (uid_t) dbus_message_iter_get_int64 (&iter);
      if (user_id != getuid ())
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_USER,
                                          _("User id mismatch"));
          dbus_connection_send (connection, reply, NULL);
          dbus_message_unref (reply);
          return DBUS_HANDLER_RESULT_HANDLED;
        }

      if (!dbus_message_iter_next (&iter))
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_GENERAL,
                                          _("Invalid arguments"));
          dbus_connection_send (connection, reply, NULL);
          dbus_message_unref (reply);
          return DBUS_HANDLER_RESULT_HANDLED;
        }

      if (!dbus_message_iter_get_string_array (&iter, &argv, &argc))
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_GENERAL,
                                          _("Invalid arguments"));
          dbus_connection_send (connection, reply, NULL);
          dbus_message_unref (reply);
          return DBUS_HANDLER_RESULT_HANDLED;
        }

      if (!terminal_app_process (app, argv, argc, &error))
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_GENERAL,
                                          error->message);
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



/**
 * terminal_dbus_register_service:
 * @app   : A #TerminalApp.
 * @error :
 *
 * Return value:
 **/
gboolean
terminal_dbus_register_service (TerminalApp *app,
                                GError     **error)
{
  static const struct DBusObjectPathVTable vtable = { NULL, handle_message, NULL, };

  DBusConnection *connection;
  DBusError       derror;

#ifndef HAVE_DBUS_BUS_REQUEST_NAME
  int (*acquire_service) (DBusConnection *, const char *, unsigned int, DBusError *);
  GModule *module;
#endif

  g_return_val_if_fail (TERMINAL_IS_APP (app), FALSE);

  /* check if the application object is already registered */
  connection = g_object_get_data (G_OBJECT (app), "terminal-dbus-connection");
  if (G_UNLIKELY (connection != NULL))
    return TRUE;

  dbus_error_init (&derror);

  /* try to connect to the session bus */
  connection = dbus_bus_get (DBUS_BUS_SESSION, &derror);
  if (G_UNLIKELY (connection == NULL))
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);
      return FALSE;
    }

  /* register DBus connection with GLib main loop */
  dbus_connection_setup_with_g_main (connection, NULL);

  /* Thanks havoc for this really very nice breakage!
   * The only way to work-around the bus API break between
   * D-BUS 0.23 and 0.30 is to use g_module_symbol() to
   * determine the available function for acquiring services
   * on the message bus (yeah, I still call it service,
   * because calling it "name" is really confusing).
   * This works for platforms that support
   *
   *  dlsym(RTLD_DEFAULT, symbol);
   *
   * Pray that it works for your platform as well...
   */
#ifdef HAVE_DBUS_BUS_REQUEST_NAME
  if (dbus_bus_request_name (connection, TERMINAL_DBUS_SERVICE, 0, &derror) < 0)
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);
      return FALSE;
    }
#else
  module = g_module_open (NULL, 0);
  if (G_UNLIKELY (module == NULL))
    {
      g_set_error (error, TERMINAL_ERROR, TERMINAL_ERROR_LINKER_FAILURE,
                   "%s", g_module_error ());
      return FALSE;
    }
  else if (!g_module_symbol (module, "dbus_bus_request_name", (gpointer) &acquire_service)
        && !g_module_symbol (module, "dbus_bus_acquire_service", (gpointer) &acquire_service))
    {
      g_set_error (error, TERMINAL_ERROR, TERMINAL_ERROR_LINKER_FAILURE,
                   "Unsupported D-BUS library");
      g_module_close (module);
      return FALSE;
    }
  else if (acquire_service (connection, TERMINAL_DBUS_SERVICE, 0, &derror) < 0)
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);
      g_module_close (module);
      return FALSE;
    }
  g_module_close (module);
#endif /* !HAVE_DBUS_BUS_REQUEST_NAME */

  /* register the application object */
  if (dbus_connection_register_object_path (connection, TERMINAL_DBUS_PATH, &vtable, app) < 0)
    {
      g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                   _("Unable to register object %s"),
                   TERMINAL_DBUS_PATH);
      return FALSE;
    }

  g_object_set_data_full (G_OBJECT (app), "terminal-dbus-connection",
                          connection, (GDestroyNotify) dbus_connection_unref);

  return TRUE;
}



/**
 * terminal_dbus_invoke_launch:
 * @argc  :
 * @argv  :
 * @error :
 *
 * Return value:
 **/
gboolean
terminal_dbus_invoke_launch (gint     argc,
                             gchar  **argv,
                             GError **error)
{
  DBusMessageIter iter;
  DBusConnection *connection;
  DBusMessage    *message;
  DBusMessage    *result;
  DBusError       derror;

  g_return_val_if_fail (argc > 0, FALSE);
  g_return_val_if_fail (argv != NULL, FALSE);

  dbus_error_init (&derror);

  /* try to connect to the session bus */
  connection = dbus_bus_get (DBUS_BUS_SESSION, &derror);
  if (G_UNLIKELY (connection == NULL))
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);
      return FALSE;
    }
  
  /* generate the message */
  message = dbus_message_new_method_call (TERMINAL_DBUS_SERVICE,
                                          TERMINAL_DBUS_PATH,
                                          TERMINAL_DBUS_INTERFACE,
                                          TERMINAL_DBUS_METHOD_LAUNCH);
  dbus_message_set_auto_activation (message, FALSE);
  dbus_message_append_iter_init (message, &iter);
  dbus_message_iter_append_int64 (&iter, getuid ());
  dbus_message_iter_append_string_array (&iter, (const gchar **) argv, argc);

  /* send the message */
  result = dbus_connection_send_with_reply_and_block (connection, message, 2000, &derror);
  dbus_message_unref (message);

  if (result == NULL)
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);
      return FALSE;
    }

  if (dbus_message_is_error (result, TERMINAL_DBUS_ERROR_USER))
    {
      dbus_set_error_from_message (&derror, result);
      g_set_error (error, TERMINAL_ERROR, TERMINAL_ERROR_USER_MISMATCH,
                   "%s", derror.message);
      dbus_message_unref (result);
      dbus_error_free (&derror);
      return FALSE;
    }

  if (dbus_message_get_type (result) == DBUS_MESSAGE_TYPE_ERROR)
    {
      dbus_set_error_from_message (&derror, result);
      g_set_error (error, TERMINAL_ERROR, TERMINAL_ERROR_FAILED,
                   "%s", derror.message);
      dbus_message_unref (result);
      dbus_error_free (&derror);
      return FALSE;
    }

  dbus_message_unref (result);
  return TRUE;
}

