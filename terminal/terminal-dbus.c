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

#include <terminal/terminal-config.h>
#include <terminal/terminal-dbus.h>



/* Yay for D-BUS breakage... */
#ifndef DBUS_USE_NEW_API
#define dbus_bus_request_name(c, n, f, e) dbus_bus_acquire_service((c), (n), (f), (e))
#define dbus_message_set_auto_start(m, v) dbus_message_set_auto_activation ((m), (v))
#endif



static DBusHandlerResult
handle_message (DBusConnection *connection,
                DBusMessage    *message,
                void           *user_data)
{
  TerminalApp *app = TERMINAL_APP (user_data);
  DBusMessage *reply;
  DBusError    derror;
  GError      *error = NULL;
  dbus_int64_t user_id;
  gchar      **argv;
  gint         argc;

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
      dbus_error_init (&derror);

      /* query the list of arguments to this call */
      if (!dbus_message_get_args (message, &derror,
                                  DBUS_TYPE_INT64, &user_id,
                                  DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &argv, &argc,
                                  DBUS_TYPE_INVALID))
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_GENERAL,
                                          derror.message);
          dbus_connection_send (connection, reply, NULL);
          dbus_message_unref (reply);
          dbus_error_free (&derror);
          return DBUS_HANDLER_RESULT_HANDLED;
        }

      /* check user id */
      if (user_id != getuid ())
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_USER,
                                          _("User id mismatch"));
          dbus_connection_send (connection, reply, NULL);
          dbus_free_string_array (argv);
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
#ifdef DBUS_USE_NEW_API
                                   DBUS_INTERFACE_LOCAL,
#else
                                   DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL,
#endif
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
  static const struct DBusObjectPathVTable vtable = { NULL, handle_message, NULL, NULL, NULL, NULL };

  DBusConnection *connection;
  DBusError       derror;

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

  if (dbus_bus_request_name (connection, TERMINAL_DBUS_SERVICE, 0, &derror) < 0)
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);
      return FALSE;
    }

  /* register the application object */
  if (!dbus_connection_register_object_path (connection, TERMINAL_DBUS_PATH, &vtable, app))
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
  DBusConnection *connection;
  dbus_int64_t    uid;
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
  dbus_message_set_auto_start (message, FALSE);

  uid = getuid ();

  dbus_message_append_args (message,
#ifdef DBUS_USE_NEW_API
                            DBUS_TYPE_INT64, &uid,
                            DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &argv, argc,
#else
                            DBUS_TYPE_INT64, uid,
                            DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, argv, argc,
#endif
                            DBUS_TYPE_INVALID);

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

