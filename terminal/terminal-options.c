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

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include <terminal/terminal-options.h>



/**
 * terminal_options_from_args:
 **/
TerminalOptions*
terminal_options_from_args (gint     argc,
                            gchar  **argv,
                            GError **error)
{
  TerminalOptions *options;
  gchar           *s;
  gint             i;
  gint             j;
  gint             n;

  g_return_val_if_fail (argc > 0, NULL);
  g_return_val_if_fail (argv != NULL, NULL);

  options = g_new0 (TerminalOptions, 1);

  for (n = 1; n < argc; ++n)
    {
      if (strcmp ("--execute", argv[n]) == 0
          || strcmp ("-x", argv[n]) == 0)
        {
          i = argc - ++n;

          if (i <= 0)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--execute/-x\" requires specifying "
                             "the command to run on the rest of the command "
                             "line"));
              goto failed;
            }

          if (options->command != NULL)
            g_strfreev (options->command);
          options->command = g_new (gchar *, i + 1);
          for (j = 0; j < i; ++j, ++n)
            options->command[j] = g_strdup (argv[n]);
          options->command[j] = NULL;
          options->mask |= TERMINAL_OPTIONS_MASK_COMMAND;
          break;
        }
      else if (strcmp ("--command", argv[n]) == 0
          || strncmp ("--command=", argv[n], 10) == 0)
        {
          s = argv[n] + 9;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--command\" requires specifying "
                             "the command to run as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (options->command != NULL)
            g_strfreev (options->command);
          options->command = NULL;
          if (!g_shell_parse_argv (s, NULL, &options->command, error))
            goto failed;
          options->mask |= TERMINAL_OPTIONS_MASK_COMMAND;
        }
      else if (strcmp ("--tab", argv[n]) == 0)
        {
          options->flags |= TERMINAL_FLAGS_OPENTAB;
          options->mask |= TERMINAL_OPTIONS_MASK_FLAGS;
        }
      else if (strcmp ("--disable-server", argv[n]) == 0)
        {
          options->flags |= TERMINAL_FLAGS_DISABLESERVER;
          options->mask |= TERMINAL_OPTIONS_MASK_FLAGS;
        }
      else if (strcmp ("--version", argv[n]) == 0
          || strcmp ("-v", argv[n]) == 0)
        {
          options->flags |= TERMINAL_FLAGS_SHOWVERSION;
          options->mask |= TERMINAL_OPTIONS_MASK_FLAGS;
        }
      else if (strcmp ("--help", argv[n]) == 0
          || strcmp ("-h", argv[n]) == 0)
        {
          options->flags |= TERMINAL_FLAGS_SHOWHELP;
          options->mask |= TERMINAL_OPTIONS_MASK_FLAGS;
        }
      else if (strcmp ("--title", argv[n]) == 0
          || strncmp ("--title=", argv[n], 8) == 0)
        {
          s = argv[n] + 7;
          
          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--title\" requires specifying "
                             "the title as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (options->title != NULL)
            g_free (options->title);
          options->title = g_strdup (s);
          options->mask |= TERMINAL_OPTIONS_MASK_TITLE;
        }
      else if (strcmp ("--working-directory", argv[n]) == 0
          || strncmp ("--working-directory", argv[n], 19) == 0)
        {
          s = argv[n] + 18;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--working-directory\" requires specifying "
                             "the working directory as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (options->working_directory != NULL)
            g_free (options->working_directory);
          options->working_directory = g_strdup (s);
          options->mask |= TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY;
        }
      else if (strcmp ("--geometry", argv[n]) == 0
          || strncmp ("--geometry=", argv[n], 11) == 0)
        {
          s = argv[n] + 10;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--geometry\" requires specifying the "
                             "window geometry as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (options->geometry != NULL)
            g_free (options->geometry);
          options->geometry = g_strdup (s);
          options->mask |= TERMINAL_OPTIONS_MASK_GEOMETRY;
        }
      else
        {
          g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                       _("Unknown option \"%s\""), argv[n]);
          goto failed;
        }
    }

  if (options->command != NULL)
    {
      for (i = 0; options->command[i] != NULL; ++i)
        ;
      options->command_len = i;
    }

  return options;

failed:
  terminal_options_free (options);
  return NULL;
}



/**
 * terminal_options_from_message:
 * @message   :
 **/
TerminalOptions*
terminal_options_from_message (DBusMessage *message)
{
  TerminalOptions *options;
  DBusMessageIter  iter;
  gchar          **command;
  gint             length;
  gchar           *sval;
  gint             n;

  if (!dbus_message_iter_init (message, &iter))
    return NULL;

  options = g_new0 (TerminalOptions, 1);
  options->mask = dbus_message_iter_get_uint32 (&iter);
  dbus_message_iter_next (&iter);

  if (options->mask & TERMINAL_OPTIONS_MASK_COMMAND)
    {
      if (!dbus_message_iter_get_string_array (&iter, &command, &length))
        {
          options->mask &= ~TERMINAL_OPTIONS_MASK_COMMAND;
        }
      else
        {
          options->command = g_new (gchar *, length + 1);
          for (n = 0; n < length; ++n)
            options->command[n] = g_strdup (command[n]);
          options->command[n] = NULL;
          options->command_len = length;
          dbus_free_string_array (command);
          dbus_message_iter_next (&iter);
        }
    }

  if (options->mask & TERMINAL_OPTIONS_MASK_TITLE)
    {
      if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
        {
          options->mask &= ~TERMINAL_OPTIONS_MASK_TITLE;
        }
      else
        {
          sval = dbus_message_iter_get_string (&iter);
          options->title = g_strdup (sval);
          dbus_free (sval);
          dbus_message_iter_next (&iter);
        }
    }

  if (options->mask & TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY)
    {
      if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
        {
          options->mask &= ~TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY;
        }
      else
        {
          sval = dbus_message_iter_get_string (&iter);
          options->working_directory = g_strdup (sval);
          dbus_free (sval);
          dbus_message_iter_next (&iter);
        }
    }

  if (options->mask & TERMINAL_OPTIONS_MASK_FLAGS)
    {
      if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_UINT32)
        {
          options->mask &= ~TERMINAL_OPTIONS_MASK_FLAGS;
        }
      else
        {
          options->flags = dbus_message_iter_get_uint32 (&iter);
          dbus_message_iter_next (&iter);
        }
    }

  if (options->mask & TERMINAL_OPTIONS_MASK_GEOMETRY)
    {
      if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
        {
          options->mask &= ~TERMINAL_OPTIONS_MASK_GEOMETRY;
        }
      else
        {
          sval = dbus_message_iter_get_string (&iter);
          options->geometry = g_strdup (sval);
          dbus_free (sval);
          dbus_message_iter_next (&iter);
        }
    }

  return options;
}



/**
 * terminal_options_free:
 * @options :
 **/
void
terminal_options_free (TerminalOptions *options)
{
  if (options->command != NULL)
    g_strfreev (options->command);
  if (options->title != NULL)
    g_free (options->title);
  if (options->working_directory != NULL)
    g_free (options->working_directory);
  if (options->geometry != NULL)
    g_free (options->geometry);
  g_free (options);
}
