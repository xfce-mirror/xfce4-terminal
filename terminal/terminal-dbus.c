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
#include <terminal/terminal-private.h>



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
  gchar       *display_name;

  /* The D-BUS interface currently looks like this:
   *
   * The Terminal service offers a method "Launch", with
   * the following parameters:
   *
   *  - UserId:int64
   *    This is the real user id of the calling process.
   *
   *  - DisplayName:string
   *    Name of the display the service is running on, this to prevent
   *    craching terminals if a display is disconnected.
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
   * D-BUS service, a TERMINAL_DBUS_ERROR_OPTIONS when
   * parsing the options failed or TERMINAL_DBUS_ERROR_GENERAL,
   * which  indicates a general problem.
   */

  if (dbus_message_is_method_call (message,
                                   TERMINAL_DBUS_INTERFACE,
                                   TERMINAL_DBUS_METHOD_LAUNCH))
    {
      dbus_error_init (&derror);

      /* query the list of arguments to this call */
      if (!dbus_message_get_args (message, &derror,
                                  DBUS_TYPE_INT64, &user_id,
                                  DBUS_TYPE_STRING, &display_name,
                                  DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &argv, &argc,
                                  DBUS_TYPE_INVALID))
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_GENERAL,
                                          derror.message);
        }
      else if (user_id != getuid ())
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_USER,
                                          _("User id mismatch"));
        }
      else if (!exo_str_is_equal (display_name, g_getenv ("DISPLAY")))
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_DISPLAY,
                                          _("Display mismatch"));
        }
      else if (!terminal_app_process (app, argv, argc, &error))
        {
          reply = dbus_message_new_error (message,
                                          TERMINAL_DBUS_ERROR_OPTIONS,
                                          error->message);
          g_error_free (error);
        }
      else
        {
          reply = dbus_message_new_method_return (message);
        }

      dbus_connection_send (connection, reply, NULL);
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

  terminal_return_val_if_fail (TERMINAL_IS_APP (app), FALSE);

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
  dbus_connection_set_exit_on_disconnect (connection, FALSE);

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

  g_object_set_data_full (G_OBJECT (app), I_("terminal-dbus-connection"),
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
  gint            code;
  gboolean        retval = TRUE;
  const gchar    *display_name;

  terminal_return_val_if_fail (argc > 0, FALSE);
  terminal_return_val_if_fail (argv != NULL, FALSE);

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

  display_name = g_getenv ("DISPLAY");
  if (G_UNLIKELY (display_name == NULL))
    display_name = "";

  dbus_message_append_args (message,
#ifdef DBUS_USE_NEW_API
                            DBUS_TYPE_INT64, &uid,
                            DBUS_TYPE_STRING, &display_name,
                            DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &argv, argc,
#else
                            DBUS_TYPE_INT64, uid,
                            DBUS_TYPE_STRING, display_name,
                            DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, argv, argc,
#endif
                            DBUS_TYPE_INVALID);

  /* send the message */
  result = dbus_connection_send_with_reply_and_block (connection, message, 2000, &derror);
  dbus_message_unref (message);

  /* when we reply with an error, the error is put in the derror
   * and the result is %NULL */
  if (result == NULL)
    {
set_error:
      if (dbus_error_has_name (&derror, TERMINAL_DBUS_ERROR_USER))
        code = TERMINAL_ERROR_USER_MISMATCH;
      if (dbus_error_has_name (&derror, TERMINAL_DBUS_ERROR_DISPLAY))
        code = TERMINAL_ERROR_DISPLAY_MISMATCH;
      else if (dbus_error_has_name (&derror, TERMINAL_DBUS_ERROR_OPTIONS))
        code = TERMINAL_ERROR_OPTIONS;
      else
        code = TERMINAL_ERROR_FAILED;

      g_set_error (error, TERMINAL_ERROR, code, "%s", derror.message);
      dbus_error_free (&derror);

      return FALSE;
    }

  /* handle other types of errors */
  if (dbus_message_get_type (result) == DBUS_MESSAGE_TYPE_ERROR)
    {
      dbus_set_error_from_message (&derror, result);
      dbus_message_unref (result);
      goto set_error;
    }

  dbus_message_unref (result);

  return retval;
}

