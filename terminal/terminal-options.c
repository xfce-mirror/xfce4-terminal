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
 * terminal_options_parse:
 * @argc            :
 * @argv            :
 * @attrs_return    :
 * @options_return  :
 * @error           :
 *
 * Return value: %TRUE on success.
 **/
gboolean
terminal_options_parse (gint              argc,
                        gchar           **argv,
                        GList           **attrs_return,
                        TerminalOptions **options_return,
                        GError          **error)
{
  TerminalWindowAttr *win_attr = NULL;
  TerminalTabAttr    *tab_attr = NULL;
  gchar              *default_directory = NULL;
  gchar              *default_display = NULL;
  gchar              *s;
  GList              *tp;
  GList              *wp;
  gint                i;
  gint                j;
  gint                n;
  
  if (attrs_return != NULL)
    {
      win_attr = terminal_window_attr_new ();
      tab_attr = win_attr->tabs->data;
      *attrs_return = g_list_append (NULL, win_attr);
    }

  if (options_return != NULL)
    *options_return = g_new0 (TerminalOptions, 1);

  for (n = 1; n < argc; ++n)
    {
      if (strcmp ("--help", argv[n]) == 0 || strcmp ("-h", argv[n]) == 0)
        {
          if (options_return != NULL)
            (*options_return)->show_help = TRUE;
        }
      else if (strcmp ("--version", argv[n]) == 0 || strcmp ("-v", argv[n]) == 0)
        {
          if (options_return != NULL)
            (*options_return)->show_version = TRUE;
        }
      else if (strcmp ("--disable-server", argv[n]) == 0)
        {
          if (options_return != NULL)
            (*options_return)->disable_server = TRUE;
        }
      else if (strcmp ("--sm-client-id", argv[n]) == 0
            || strncmp ("--sm-client-id=", argv[n], 15) == 0)
        {
          s = argv[n] + 14;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--sm-client-id\" requires specifying "
                             "the session id as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (options_return != NULL)
            {
              g_free ((*options_return)->session_id);
              (*options_return)->session_id = g_strdup (s);
            }
        }
      else if (strcmp ("--execute", argv[n]) == 0 || strcmp ("-x", argv[n]) == 0)
        {
          i = argc - ++n;

          if (i <= 0)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--execute/-x\" requires specifying the command "
                             "to run on the rest of the command line"));
              goto failed;
            }

          if (tab_attr != NULL)
            {
              g_strfreev (tab_attr->command);
              tab_attr->command = g_new (gchar *, i + 1);
              for (j = 0; j < i; ++j, ++n)
                tab_attr->command[j] = g_strdup (argv[n]);
              tab_attr->command[j] = NULL;
            }

          break;
        }
      else if (strcmp ("--command", argv[n]) == 0
            || strncmp ("--command=", argv[n], 10) == 0
            || strcmp ("-e", argv[n]) == 0)
        {
          s = argv[n] + 9;

          if (strcmp ("-e", argv[n]) != 0 && *s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--command/-e\" requires specifying "
                             "the command to run as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (tab_attr != NULL)
            {
              g_strfreev (tab_attr->command);
              tab_attr->command = NULL;
              if (!g_shell_parse_argv (s, NULL, &tab_attr->command, error))
                goto failed;
            }
        }
      else if (strcmp ("--working-directory", argv[n]) == 0
            || strncmp ("--working-directory=", argv[n], 19) == 0)
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

          if (tab_attr != NULL)
            {
              g_free (tab_attr->directory);
              tab_attr->directory = g_strdup (s);
            }
        }
      else if (strcmp ("--title", argv[n]) == 0
            || strncmp ("--title=", argv[n], 8) == 0
            || strcmp ("-t", argv[n]) == 0
            || strcmp ("-T", argv[n]) == 0)
        {
          s = argv[n] + 7;

          if (strcmp ("-T", argv[n]) != 0
              && strcmp ("-t", argv[n]) != 0
              && *s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--title/-T\" requires specifying "
                             "the title as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (tab_attr != NULL)
            {
              g_free (tab_attr->title);
              tab_attr->title = g_strdup (s);
            }
        }
      else if (strcmp ("--display", argv[n]) == 0
            || strncmp ("--display=", argv[n], 10) == 0)
        {
          s = argv[n] + 9;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--display\" requires specifying "
                             "the X display as its parameters"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (win_attr != NULL)
            {
              g_free (win_attr->display);
              win_attr->display = g_strdup (s);
            }
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
                           _("Option \"--geometry\" requires specifying "
                             "the window geometry as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (win_attr != NULL)
            {
              g_free (win_attr->geometry);
              win_attr->geometry = g_strdup (s);
            }
        }
      else if (strcmp ("--role", argv[n]) == 0
            || strncmp ("--role=", argv[n], 7) == 0)
        {
          s = argv[n] + 6;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--role\" requires specifying "
                             "the window geometry as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (win_attr != NULL)
            {
              g_free (win_attr->role);
              win_attr->role = g_strdup (s);
            }
        }
      else if (strcmp ("--startup-id", argv[n]) == 0
            || strncmp ("--startup-id=", argv[n], 13) == 0)
        {
          s = argv[n] + 12;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--startup-id\" requires specifying "
                             "the startup id as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          if (win_attr != NULL)
            {
              g_free (win_attr->startup_id);
              win_attr->startup_id = g_strdup (s);
            }
        }
      else if (strcmp ("--show-menubar", argv[n]) == 0
            || strcmp ("--hide-menubar", argv[n]) == 0)
        {
          if (win_attr != NULL)
            {
              win_attr->menubar = (argv[n][2] == 's') 
                                ? TERMINAL_VISIBILITY_SHOW
                                : TERMINAL_VISIBILITY_HIDE;
            }
        }
      else if (strcmp ("--fullscreen", argv[n]) == 0)
        {
          if (win_attr != NULL)
            {
              win_attr->fullscreen = TRUE;
            }
        }
      else if (strcmp ("--show-borders", argv[n]) == 0
            || strcmp ("--hide-borders", argv[n]) == 0)
        {
          if (win_attr != NULL)
            {
              win_attr->borders = (argv[n][2] == 's')
                                ? TERMINAL_VISIBILITY_SHOW
                                : TERMINAL_VISIBILITY_HIDE;
            }
        }
      else if (strcmp ("--show-toolbars", argv[n]) == 0
            || strcmp ("--hide-toolbars", argv[n]) == 0)
        {
          if (win_attr != NULL)
            {
              win_attr->toolbars = (argv[n][2] == 's')
                                ? TERMINAL_VISIBILITY_SHOW
                                : TERMINAL_VISIBILITY_HIDE;
            }
        }
      else if (strcmp ("--tab", argv[n]) == 0)
        {
          if (win_attr != NULL)
            {
              tab_attr = g_new0 (TerminalTabAttr, 1);
              win_attr->tabs = g_list_append (win_attr->tabs, tab_attr);
            }
        }
      else if (strcmp ("--window", argv[n]) == 0)
        {
          if (attrs_return != NULL)
            {
              win_attr = terminal_window_attr_new ();
              tab_attr = win_attr->tabs->data;
              *attrs_return = g_list_append (*attrs_return, win_attr);
            }
        }
      else if (strcmp ("--default-display", argv[n]) == 0
            || strncmp ("--default-display=", argv[n], 18) == 0)
        {
          s = argv[n] + 17;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--default-display\" requires specifying "
                             "the default X display as its parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          g_free (default_display);
          default_display = g_strdup (s);
        }
      else if (strcmp ("--default-working-directory", argv[n]) == 0
            || strncmp ("--default-working-directory=", argv[n], 28) == 0)
        {
          s = argv[n] + 27;

          if (*s == '=')
            {
              ++s;
            }
          else if (n + 1 >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--default-working-directory\" requires "
                             "specifying the default working directory as its "
                             "parameter"));
              goto failed;
            }
          else
            {
              s = argv[++n];
            }

          g_free (default_directory);
          default_directory = g_strdup (s);
        }
      else
        {
          g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                       _("Unknown option \"%s\""), argv[n]);
          goto failed;
        }
    }

  /* substitute default working directory and default display if any */
  if (attrs_return != NULL)
    {
      for (wp = *attrs_return; wp != NULL; wp = wp->next)
        {
          win_attr = wp->data;
          for (tp = win_attr->tabs; tp != NULL; tp = tp->next)
            {
              tab_attr = tp->data;
              if (tab_attr->directory == NULL && default_directory != NULL)
                tab_attr->directory = g_strdup (default_directory);
            }

          if (win_attr->display == NULL && default_display != NULL)
            win_attr->display = g_strdup (default_display);
        }
    }
  g_free (default_directory);
  g_free (default_display);
  
  return TRUE;

failed:
  if (options_return != NULL)
    {
      terminal_options_free (*options_return);
      *options_return = NULL;
    }
  if (attrs_return != NULL)
    {
      for (wp = *attrs_return; wp != NULL; wp = wp->next)
        terminal_window_attr_free (wp->data);
      g_list_free (*attrs_return);
    }
  g_free (default_directory);
  g_free (default_display);

  return FALSE;
}



/**
 **/
void
terminal_options_free (TerminalOptions *options)
{
  if (G_LIKELY (options != NULL))
    {
      g_free (options->session_id);
      g_free (options);
    }
}



/**
 **/
void
terminal_tab_attr_free (TerminalTabAttr *attr)
{
  g_return_if_fail (attr != NULL);

  g_strfreev (attr->command);
  g_free (attr->directory);
  g_free (attr->title);
  g_free (attr);
}



/**
 **/
TerminalWindowAttr*
terminal_window_attr_new (void)
{
  TerminalWindowAttr *win_attr;
  TerminalTabAttr    *tab_attr;

  win_attr = g_new0 (TerminalWindowAttr, 1);
  win_attr->fullscreen = FALSE;
  win_attr->menubar = TERMINAL_VISIBILITY_DEFAULT;
  win_attr->borders = TERMINAL_VISIBILITY_DEFAULT;
  win_attr->toolbars = TERMINAL_VISIBILITY_DEFAULT;

  tab_attr = g_new0 (TerminalTabAttr, 1);
  win_attr->tabs = g_list_append (NULL, tab_attr);

  return win_attr;
}



/**
 * terminal_window_attr_free:
 * @attr  : A #TerminalWindowAttr.
 **/
void
terminal_window_attr_free (TerminalWindowAttr *attr)
{
  g_return_if_fail (attr != NULL);

  g_list_foreach (attr->tabs, (GFunc) terminal_tab_attr_free, NULL);
  g_list_free (attr->tabs);
  g_free (attr->startup_id);
  g_free (attr->geometry);
  g_free (attr->display);
  g_free (attr->role);
  g_free (attr);
}




