/*-
 * Copyright (c) 2012 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gio/gio.h>

#include <terminal/terminal-config.h>
#include <terminal/terminal-gdbus.h>
#include <terminal/terminal-private.h>



static const gchar terminal_gdbus_introspection_xml[] =
  "<node>"
    "<interface name='" TERMINAL_DBUS_INTERFACE "'>"
      "<method name='" TERMINAL_DBUS_METHOD_LAUNCH "'>"
        "<arg type='u' name='uid' direction='in'/>"
        "<arg type='ay' name='display-name' direction='in'/>"
        "<arg type='aay' name='argv' direction='in'/>"
      "</method>"
    "</interface>"
  "</node>";



static gchar *
terminal_gdbus_display_name (void)
{
  const gchar *display_name;
  gchar       *name;
  gchar       *period;

  display_name = g_getenv ("DISPLAY");
  if (G_UNLIKELY (display_name == NULL))
    return "";

  name = g_strdup (display_name);
  period = strrchr (name, '.');
  if (period != NULL)
    *period = '\0';

  return name;
}



static void
terminal_gdbus_method_call (GDBusConnection       *connection,
                            const gchar           *sender,
                            const gchar           *object_path,
                            const gchar           *interface_name,
                            const gchar           *method_name,
                            GVariant              *parameters,
                            GDBusMethodInvocation *invocation,
                            gpointer               user_data)
{
  TerminalApp  *app = TERMINAL_APP (user_data);
  guint32       uid = G_MAXUINT32;
  gchar        *display_name = NULL;
  gchar       **argv = NULL;
  GError       *error = NULL;
  gchar        *display_name2;

  terminal_return_if_fail (TERMINAL_IS_APP (app));
  terminal_return_if_fail (!g_strcmp0 (object_path, TERMINAL_DBUS_PATH));
  terminal_return_if_fail (!g_strcmp0 (interface_name, TERMINAL_DBUS_INTERFACE));

  if (g_strcmp0 (method_name, TERMINAL_DBUS_METHOD_LAUNCH) == 0)
    {
      /* get paramenters */
      g_variant_get (parameters, "(u^ay^aay)", &uid, &display_name, &argv);

      display_name2 = terminal_gdbus_display_name ();

      if (uid != getuid ())
        {
          g_dbus_method_invocation_return_error (invocation,
              TERMINAL_ERROR, TERMINAL_ERROR_USER_MISMATCH,
              _("User id mismatch"));
        }
      else if (g_strcmp0 (display_name, display_name2) != 0)
        {
          g_dbus_method_invocation_return_error (invocation,
              TERMINAL_ERROR, TERMINAL_ERROR_DISPLAY_MISMATCH,
              _("Display mismatch"));
        }
      else if (!terminal_app_process (app, argv, g_strv_length (argv), &error))
        {
          g_dbus_method_invocation_return_error (invocation,
              TERMINAL_ERROR, TERMINAL_ERROR_OPTIONS,
              "%s", error->message);
          g_error_free (error);
        }
      else
        {
          /* everything went fine */
          g_dbus_method_invocation_return_value (invocation, NULL);
        }

      g_free (display_name);
      g_free (display_name2);
      g_strfreev (argv);
    }
  else
    {
      g_dbus_method_invocation_return_error (invocation,
          G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
          "Unknown method for DBus service " TERMINAL_DBUS_SERVICE);
    }
}



static const GDBusInterfaceVTable terminal_gdbus_vtable =
{
  terminal_gdbus_method_call,
  NULL, /* get property */
  NULL  /* set property */
};



static void
terminal_gdbus_bus_acquired (GDBusConnection *connection,
                             const gchar     *name,
                             gpointer         user_data)
{
  guint          register_id;
  GDBusNodeInfo *info;
  GError        *error = NULL;

  info = g_dbus_node_info_new_for_xml (terminal_gdbus_introspection_xml, NULL);
  terminal_assert (info != NULL);
  terminal_assert (*info->interfaces != NULL);

  register_id = g_dbus_connection_register_object (connection,
                                                   TERMINAL_DBUS_PATH,
                                                   *info->interfaces, /* first iface */
                                                   &terminal_gdbus_vtable,
                                                   user_data,
                                                   NULL,
                                                   &error);

  if (register_id == 0)
    {
      g_message ("Failed to register object: %s", error->message);
      g_error_free (error);
    }

  g_dbus_node_info_unref (info);
}



gboolean
terminal_gdbus_register_service (TerminalApp *app,
                                 GError     **error)
{
  guint owner_id;

  terminal_return_val_if_fail (TERMINAL_IS_APP (app), FALSE);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             TERMINAL_DBUS_SERVICE,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             terminal_gdbus_bus_acquired,
                             NULL,
                             NULL,
                             app,
                             NULL);

  return (owner_id != 0);
}



gboolean
terminal_gdbus_invoke_launch (gint     argc,
                              gchar  **argv,
                              GError **error)
{
  GVariant        *reply;
  GDBusConnection *connection;
  GError          *err = NULL;
  gboolean         result;
  guint32          uid;
  gchar           *display_name;

  terminal_return_val_if_fail (argc == (gint) g_strv_length (argv), FALSE);

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, error);
  if (G_UNLIKELY (connection == NULL))
    return FALSE;

  /* store in an uin32 for gvariant */
  uid = getuid ();
  display_name = terminal_gdbus_display_name ();

  reply = g_dbus_connection_call_sync (connection,
                                       TERMINAL_DBUS_SERVICE,
                                       TERMINAL_DBUS_PATH,
                                       TERMINAL_DBUS_INTERFACE,
                                       TERMINAL_DBUS_METHOD_LAUNCH,
                                       g_variant_new ("(u^ay^aay)",
                                                      uid,
                                                      display_name,
                                                      argv),
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                       2000,
                                       NULL,
                                       &err);

  g_object_unref (connection);
  g_free (display_name);

  result = (reply != NULL);
  if (G_LIKELY (result))
    g_variant_unref (reply);
  else
    g_propagate_error (error, err);

  return result;
}

