/*-
 * Copyright (c) 2004-2008 os-cillation e.K.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <terminal/terminal-private.h>



/**
 * terminal_option_cmp:
 * @long_name     : long option text or %NULL
 * @short_name    : short option character or 0
 * @argv          : pointer to the argument vector
 * @argc          : length of the argument vector
 * @argv_offset   : current offset in the argument vector
 * @return_string : return location of a pointer to the option
 *                  arguments, or %NULL to make this function
 *                  behave for string comparison.
 *
 * Return value: %TRUE if @long_name or @short_name was found.
 **/
static gboolean
terminal_option_cmp (const gchar   *long_name,
                     gchar          short_name,
                     gint           argc,
                     gchar        **argv,
                     gint          *argv_offset,
                     gchar        **return_string)
{
  gint   len, offset;
  gchar *arg = argv[*argv_offset];

  if (long_name != NULL && *(arg + 1) == '-')
    {
      /* a boolean compare, check if the remaining string matches */
      if (return_string == NULL)
        return (strcmp (arg + 2, long_name) == 0);

      len = strlen (long_name);
      if (strncmp (arg + 2, long_name, len) != 0)
        return FALSE;

      offset = 2 + len;
    }
  else if (short_name != 0 && *(arg + 1) == short_name)
    {
      if (return_string == NULL)
        return (*(arg + 2) == '\0');

      offset = 2;
    }
  else
    {
      return FALSE;
    }

  terminal_assert (return_string != NULL);
  if (*(arg + offset) == '=')
    *return_string = arg + (offset + 1);
  else if (*argv_offset + 1 > argc)
    *return_string = NULL;
  else
    *return_string = argv[++*argv_offset];

  return TRUE;
}



static gboolean
terminal_option_show_hide_cmp (const gchar         *long_name,
                               gint                 argc,
                               gchar              **argv,
                               gint                *argv_offset,
                               TerminalVisibility  *return_visibility)
{
  gchar *arg = argv[*argv_offset];

  terminal_return_val_if_fail (long_name != NULL, FALSE);
  terminal_return_val_if_fail (return_visibility != NULL, FALSE);

  if ((strncmp (arg, "--show-", 7) == 0 || strncmp (arg, "--hide-", 7) == 0)
      && strcmp (arg + 7, long_name) == 0)
    {
      if (*(arg + 2) == 's')
        *return_visibility = TERMINAL_VISIBILITY_SHOW;
      else
        *return_visibility = TERMINAL_VISIBILITY_HIDE;

      return TRUE;
    }

  return FALSE;
}



static void
terminal_tab_attr_free (TerminalTabAttr *attr)
{
  terminal_return_if_fail (attr != NULL);

  g_strfreev (attr->command);
  g_free (attr->directory);
  g_free (attr->title);
  g_slice_free (TerminalTabAttr, attr);
}



void
terminal_options_parse (gint       argc,
                        gchar    **argv,
                        gboolean  *show_help,
                        gboolean  *show_version,
                        gboolean  *show_colors,
                        gboolean  *disable_server)
{
  gint   n;

  for (n = 1; n < argc; ++n)
    {
      /* all arguments should atleast start with a dash */
      if (argv[n] == NULL || *argv[n] != '-')
        continue;

      /* everything after execute belongs to the command */
      if (terminal_option_cmp ("execute", 'x', argc, argv, &n, NULL))
        break;

      if (terminal_option_cmp ("help", 'h', argc, argv, &n, NULL))
        *show_help = TRUE;
      else if (terminal_option_cmp ("version", 'V', argc, argv, &n, NULL))
        *show_version = TRUE;
      else if (terminal_option_cmp ("disable-server", 0, argc, argv, &n, NULL))
        *disable_server = TRUE;
      else if (terminal_option_cmp ("color-table", 0, argc, argv, &n, NULL))
        *show_colors = TRUE;
    }
}



/**
 * terminal_window_attr_parse:
 * @argc            :
 * @argv            :
 * @error           :
 *
 * Return value: %NULL on failure.
 **/
GSList *
terminal_window_attr_parse (gint              argc,
                            gchar           **argv,
                            gboolean          can_reuse_tab,
                            GError          **error)
{
  TerminalWindowAttr *win_attr;
  TerminalTabAttr    *tab_attr;
  gchar              *default_directory = NULL;
  gchar              *default_display = NULL;
  gchar              *s;
  GSList             *tp, *wp;
  GSList             *attrs;
  gint                n;
  TerminalVisibility  visible;

  win_attr = terminal_window_attr_new ();
  tab_attr = win_attr->tabs->data;
  attrs = g_slist_prepend (NULL, win_attr);

  for (n = 1; n < argc; ++n)
    {
      /* all arguments should atleast start with a dash */
      if (argv[n] == NULL || *argv[n] != '-')
        goto unknown_option;

      if (terminal_option_cmp ("default-display", 0, argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--default-display\" requires specifying "
                             "the default X display as its parameter"));
              goto failed;
            }
          else
            {
              g_free (default_display);
              default_display = g_strdup (s);
              continue;
            }
        }
      else if (terminal_option_cmp ("default-working-directory", 0, argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--default-working-directory\" requires "
                             "specifying the default working directory as its "
                             "parameter"));
              goto failed;
            }
          else
            {
              g_free (default_directory);
              default_directory = g_strdup (s);
              continue;
            }
        }
      else if (terminal_option_cmp ("execute", 'x', argc, argv, &n, NULL))
        {
          if (++n >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--execute/-x\" requires specifying the command "
                             "to run on the rest of the command line"));
              goto failed;
            }
          else
            {
              g_strfreev (tab_attr->command);
              tab_attr->command = g_strdupv (argv + n);
            }

          break;
        }
      else if (terminal_option_cmp ("command", 'e', argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--command/-e\" requires specifying "
                             "the command to run as its parameter"));
              goto failed;
            }
          else
            {
              g_strfreev (tab_attr->command);
              tab_attr->command = NULL;
              if (!g_shell_parse_argv (s, NULL, &tab_attr->command, error))
                goto failed;
            }
        }
      else if (terminal_option_cmp ("working-directory", 0, argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--working-directory\" requires specifying "
                             "the working directory as its parameter"));
              goto failed;
            }
          else
            {
              g_free (tab_attr->directory);
              tab_attr->directory = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("title", 'T', argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--title/-T\" requires specifying "
                             "the title as its parameter"));
              goto failed;
            }
          else
            {
              g_free (tab_attr->title);
              tab_attr->title = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("hold", 'H', argc, argv, &n, NULL))
        {
          tab_attr->hold = TRUE;
        }
      else if (terminal_option_cmp ("display", 0, argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--display\" requires specifying "
                             "the X display as its parameters"));
              goto failed;
            }
          else
            {
              g_free (win_attr->display);
              win_attr->display = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("geometry", 0, argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--geometry\" requires specifying "
                             "the window geometry as its parameter"));
              goto failed;
            }
          else
            {
              g_free (win_attr->geometry);
              win_attr->geometry = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("role", 0, argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--role\" requires specifying "
                             "the window role as its parameter"));
              goto failed;
            }
          else
            {
              g_free (win_attr->role);
              win_attr->role = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("sm-client-id", 0, argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--sm-client-id\" requires specifying "
                             "the unique session id as its parameter"));
              goto failed;
            }
          else
            {
              g_free (win_attr->sm_client_id);
              win_attr->sm_client_id = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("startup-id", 0, argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--startup-id\" requires specifying "
                             "the startup id as its parameter"));
              goto failed;
            }
          else
            {
              g_free (win_attr->startup_id);
              win_attr->startup_id = g_strdup (s);
              continue;
            }
        }
      else if (terminal_option_cmp ("icon", 'I', argc, argv, &n, &s))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--icon/-I\" requires specifying "
                             "an icon name or filename as its parameter"));
              goto failed;
            }
          else
            {
              g_free (win_attr->icon);
              win_attr->icon = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("drop-down", 0, argc, argv, &n, NULL))
        {
          win_attr->drop_down = TRUE;
        }
      else if (terminal_option_show_hide_cmp ("menubar", argc, argv, &n, &visible))
        {
          win_attr->menubar = visible;
        }
      else if (terminal_option_cmp ("fullscreen", 0, argc, argv, &n, NULL))
        {
          win_attr->fullscreen = TRUE;
        }
      else if (terminal_option_cmp ("maximize", 0, argc, argv, &n, NULL))
        {
          win_attr->maximize = TRUE;
        }
      else if (terminal_option_show_hide_cmp ("borders", argc, argv, &n, &visible))
        {
          win_attr->borders = visible;
        }
      else if (terminal_option_show_hide_cmp ("toolbar", argc, argv, &n, &visible))
        {
          win_attr->toolbar = visible;
        }
      else if (terminal_option_cmp ("tab", 0, argc, argv, &n, NULL))
        {
          if (can_reuse_tab)
            {
              /* tab is the first user option, reuse existing window */
              win_attr->reuse_last_window = TRUE;
            }
          else
            {
              /* add new tab */
              tab_attr = g_slice_new0 (TerminalTabAttr);
              win_attr->tabs = g_slist_append (win_attr->tabs, tab_attr);
            }
        }
      else if (terminal_option_cmp ("window", 0, argc, argv, &n, NULL))
        {
          /* multiple windows, don't reuse */
          win_attr->reuse_last_window = FALSE;

          /* setup new window */
          win_attr = terminal_window_attr_new ();
          tab_attr = win_attr->tabs->data;
          attrs = g_slist_append (attrs, win_attr);
        }
      else if (terminal_option_cmp ("disable-server", 0, argc, argv, &n, NULL)
               || terminal_option_cmp ("sync", 0, argc, argv, &n, NULL)
               || terminal_option_cmp ("g-fatal-warnings", 0, argc, argv, &n, NULL))
        {
          /* options we can ignore */
          continue;
        }
      else
        {
unknown_option:
          g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                       _("Unknown option \"%s\""), argv[n]);
          goto failed;
        }

      /* not the first option anymore */
      can_reuse_tab = FALSE;
    }

  /* substitute default working directory and default display if any */
  if (default_display != NULL || default_directory != NULL)
    {
      for (wp = attrs; wp != NULL; wp = wp->next)
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

  return attrs;

failed:

  for (wp = attrs; wp != NULL; wp = wp->next)
    terminal_window_attr_free (wp->data);
  g_slist_free (attrs);

  g_free (default_directory);
  g_free (default_display);

  return NULL;
}



/**
 **/
TerminalWindowAttr*
terminal_window_attr_new (void)
{
  TerminalWindowAttr *win_attr;
  TerminalTabAttr    *tab_attr;

  win_attr = g_slice_new0 (TerminalWindowAttr);
  win_attr->fullscreen = FALSE;
  win_attr->menubar = TERMINAL_VISIBILITY_DEFAULT;
  win_attr->borders = TERMINAL_VISIBILITY_DEFAULT;
  win_attr->toolbar = TERMINAL_VISIBILITY_DEFAULT;

  tab_attr = g_slice_new0 (TerminalTabAttr);
  win_attr->tabs = g_slist_prepend (NULL, tab_attr);

  return win_attr;
}



/**
 * terminal_window_attr_free:
 * @attr  : A #TerminalWindowAttr.
 **/
void
terminal_window_attr_free (TerminalWindowAttr *attr)
{
  terminal_return_if_fail (attr != NULL);

  g_slist_foreach (attr->tabs, (GFunc) terminal_tab_attr_free, NULL);
  g_slist_free (attr->tabs);
  g_free (attr->startup_id);
  g_free (attr->sm_client_id);
  g_free (attr->geometry);
  g_free (attr->display);
  g_free (attr->role);
  g_free (attr->icon);
  g_slice_free (TerminalWindowAttr, attr);
}
