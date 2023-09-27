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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include <terminal/terminal-options.h>
#include <terminal/terminal-private.h>



/**
 * terminal_option_cmp:
 * @long_name     : long option text or %NULL
 * @short_name    : short option character or 0
 * @argc          : length of the argument vector
 * @argv          : pointer to the argument vector
 * @argv_offset   : current offset in the argument vector
 * @return_string : return location of a pointer to the option
 *                  arguments, or %NULL to make this function
 *                  behave for string comparison.
 * @short_offset  : current offset in grouped short arguments e.g. -abcd
 *
 * Return value: %TRUE if @long_name or @short_name was found.
 **/
static gboolean
terminal_option_cmp (const gchar   *long_name,
                     gchar          short_name,
                     gint           argc,
                     gchar        **argv,
                     gint          *argv_offset,
                     gchar        **return_string,
                     gint          *short_offset)
{
  gint   len, offset;
  gchar *arg = argv[*argv_offset];
  gboolean short_option = FALSE;

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
  else if (short_name != 0 && *(arg + 1 + *short_offset) == short_name)
    {
      if (return_string == NULL)
        {
          if (*(arg + 2 + *short_offset) != '\0')
            {
              ++*short_offset;
              --*argv_offset;
            }
          else
            *short_offset = 0;

          return TRUE;
        }

      offset = 2 + *short_offset;
      *short_offset = 0;
      short_option = TRUE;
    }
  else
    {
      return FALSE;
    }

  terminal_assert (return_string != NULL);
  if (*(arg + offset) == '\0')
    *return_string = argv[++*argv_offset];
  else if (short_option)
    *return_string = arg + offset;
  else if (*(arg + offset) == '=')
    *return_string = arg + (offset + 1);
  else
    return FALSE;

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
  const size_t pref_len = strlen ("--show-");

  terminal_return_val_if_fail (long_name != NULL, FALSE);
  terminal_return_val_if_fail (return_visibility != NULL, FALSE);

  if ((strncmp (arg, "--show-", pref_len) == 0 || strncmp (arg, "--hide-", pref_len) == 0)
      && strcmp (arg + pref_len, long_name) == 0)
    {
      if (*(arg + 2) == 's')
        *return_visibility = TERMINAL_VISIBILITY_SHOW;
      else
        *return_visibility = TERMINAL_VISIBILITY_HIDE;

      return TRUE;
    }

  return FALSE;
}



void
terminal_options_parse (gint              argc,
                        gchar           **argv,
                        TerminalOptions  *options)
{
  gint n, short_offset = 0;

  for (n = 1; n < argc; ++n)
    {
      /* all arguments should at least start with a dash */
      if (argv[n] == NULL || *argv[n] != '-')
        continue;

      /* "--" option delimiter */
      if (terminal_option_cmp ("", 0, argc, argv, &n, NULL, &short_offset))
        break;

      /* everything after execute belongs to the command */
      if (terminal_option_cmp ("execute", 'x', argc, argv, &n, NULL, &short_offset))
        break;

      if (terminal_option_cmp ("help", 'h', argc, argv, &n, NULL, &short_offset))
        {
          options->show_help = 1;
          break;
        }

      if (terminal_option_cmp ("version", 'V', argc, argv, &n, NULL, &short_offset))
        {
          options->show_version = 1;
          break;
        }

      if (terminal_option_cmp ("color-table", 0, argc, argv, &n, NULL, &short_offset))
        {
          options->show_colors = 1;
          break;
        }

      if (terminal_option_cmp ("preferences", 0, argc, argv, &n, NULL, &short_offset))
        {
          options->show_preferences = 1;
          break;
        }

      if (terminal_option_cmp ("disable-server", 0, argc, argv, &n, NULL, &short_offset))
        options->disable_server = 1;
    }
}



/**
 * terminal_window_attr_parse:
 * @argc                : argument count
 * @argv                : arguments
 * @can_reuse_window    : a %gboolean denoting whether new tabs should be opened in an existing window or not
 * @error               : a %GError used for error reporting
 *
 * As far as "--tab" and "--window" are concerned: Each "--window" argument opens an additional window.
 * Each "--tab" argument opens an additional tab in the preceding window (or in the current active window, if such exists,
 * and no "--window" argument has been read previously).
 *
 *
 * Return value: a list of %TerminalWindowAttr or %NULL on failure.
 **/
GSList *
terminal_window_attr_parse (gint              argc,
                            gchar           **argv,
                            gboolean          can_reuse_window,
                            GError          **error)
{
  TerminalWindowAttr *win_attr;
  TerminalTabAttr    *tab_attr;
  gchar              *default_directory = NULL;
  gchar              *default_display = NULL;
  gchar              *s;
  GSList             *tp, *wp;
  GSList             *attrs;
  gint                n, short_offset = 0;
  gchar              *end_ptr = NULL;
  TerminalVisibility  visible;
  gboolean            ignore_window_option = TRUE;

  win_attr = terminal_window_attr_new ();
  tab_attr = win_attr->tabs->data;
  attrs = g_slist_prepend (NULL, win_attr); /* used as the return value */

  for (n = 1; n < argc; ++n)
    {
      /* all arguments should at least start with a dash */
      if (argv[n] == NULL || *argv[n] != '-')
        goto unknown_option;

      /* "--" option delimiter*/
      if (terminal_option_cmp ("", 0, argc, argv, &n, NULL, &short_offset))
        break;

      if (terminal_option_cmp ("default-display", 0, argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("default-working-directory", 0, argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("execute", 'x', argc, argv, &n, NULL, &short_offset))
        {
          if (short_offset > 0 || ++n >= argc)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--execute/-x\" requires specifying the command "
                             "to run on the rest of the command line separated from "
                             "\"--execute/-x\""));
              goto failed;
            }
          else
            {
              g_strfreev (tab_attr->command);
              tab_attr->command = g_strdupv (argv + n);
            }

          break;
        }
      else if (terminal_option_cmp ("command", 'e', argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("working-directory", 0, argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("title", 'T', argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("dynamic-title-mode", 0, argc, argv, &n, &s, &short_offset))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--dynamic-title-mode\" requires specifying "
                             "the dynamic title mode as its parameter"));
              goto failed;
            }
          else if (g_ascii_strcasecmp (s, "replace") == 0)
            tab_attr->dynamic_title_mode = TERMINAL_TITLE_REPLACE;
          else if (g_ascii_strcasecmp (s, "before") == 0)
            tab_attr->dynamic_title_mode = TERMINAL_TITLE_PREPEND;
          else if (g_ascii_strcasecmp (s, "after") == 0)
            tab_attr->dynamic_title_mode = TERMINAL_TITLE_APPEND;
          else if (g_ascii_strcasecmp (s, "none") == 0)
            tab_attr->dynamic_title_mode = TERMINAL_TITLE_HIDE;
          else
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Invalid argument for option \"--dynamic-title-mode\": %s"),
                           s);
              goto failed;
            }
        }
      else if (terminal_option_cmp ("initial-title", 0, argc, argv, &n, &s, &short_offset))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--initial-title\" requires specifying "
                             "the initial title as its parameter"));
              goto failed;
            }
          else
            {
              g_free (tab_attr->initial_title);
              tab_attr->initial_title = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("hold", 'H', argc, argv, &n, NULL, &short_offset))
        {
          tab_attr->hold = TRUE;
        }
      else if (terminal_option_cmp ("active-tab", 0, argc, argv, &n, NULL, &short_offset))
        {
          tab_attr->active = TRUE;
        }
      else if (terminal_option_cmp ("color-text", 0, argc, argv, &n, &s, &short_offset))
        {
          GdkRGBA color;
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"%s\" requires specifying "
                             "the color as its parameter"), "--color-text");
              goto failed;
            }
          else if (!gdk_rgba_parse (&color, s))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Unable to parse color: %s"), s);
              goto failed;
            }
          g_free (tab_attr->color_text);
          tab_attr->color_text = g_strdup (s);
        }
      else if (terminal_option_cmp ("color-bg", 0, argc, argv, &n, &s, &short_offset))
        {
          GdkRGBA color;
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"%s\" requires specifying "
                             "the color as its parameter"), "--color-bg");
              goto failed;
            }
          else if (!gdk_rgba_parse (&color, s))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Unable to parse color: %s"), s);
              goto failed;
            }
          g_free (tab_attr->color_bg);
          tab_attr->color_bg = g_strdup (s);
        }
      else if (terminal_option_cmp ("display", 0, argc, argv, &n, &s, &short_offset))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--display\" requires specifying "
                             "the X display as its parameter"));
              goto failed;
            }
          else
            {
              g_free (win_attr->display);
              win_attr->display = g_strdup (s);
            }
        }
      else if (terminal_option_cmp ("geometry", 0, argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("role", 0, argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("workspace", 0, argc, argv, &n, &s, &short_offset))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--workspace\" requires specifying "
                             "the workspace number as its parameter"));
              goto failed;
            }
          else
            {
              win_attr->workspace = strtol (s, &end_ptr, 0);
            }
        }
      else if (terminal_option_cmp ("sm-client-id", 0, argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("startup-id", 0, argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("icon", 'I', argc, argv, &n, &s, &short_offset))
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
      else if (terminal_option_cmp ("drop-down", 0, argc, argv, &n, NULL, &short_offset))
        {
          win_attr->drop_down = TRUE;
        }
      else if (terminal_option_show_hide_cmp ("menubar", argc, argv, &n, &visible))
        {
          win_attr->menubar = visible;
        }
      else if (terminal_option_cmp ("fullscreen", 0, argc, argv, &n, NULL, &short_offset))
        {
          win_attr->fullscreen = TRUE;
        }
      else if (terminal_option_cmp ("maximize", 0, argc, argv, &n, NULL, &short_offset))
        {
          win_attr->maximize = TRUE;
        }
      else if (terminal_option_cmp ("minimize", 0, argc, argv, &n, NULL, &short_offset))
        {
          win_attr->minimize = TRUE;
        }
      else if (terminal_option_show_hide_cmp ("borders", argc, argv, &n, &visible))
        {
          win_attr->borders = visible;
        }
      else if (terminal_option_show_hide_cmp ("toolbar", argc, argv, &n, &visible))
        {
          win_attr->toolbar = visible;
        }
      else if (terminal_option_show_hide_cmp ("scrollbar", argc, argv, &n, &visible))
        {
          win_attr->scrollbar = visible;
        }
      else if (terminal_option_cmp ("tab", 0, argc, argv, &n, NULL, &short_offset))
        {
          if (can_reuse_window)
            {
              /* add tabs in an existing active window, until a '--window' option in encountered */
              win_attr->reuse_last_window = TRUE;
              can_reuse_window = FALSE;
            }
          else
            {
              /* add new tab */
              tab_attr = terminal_tab_attr_new ();
              win_attr->tabs = g_slist_append (win_attr->tabs, tab_attr);
            }
        }
      else if (terminal_option_cmp ("window", 0, argc, argv, &n, NULL, &short_offset))
        {
          /* --window is ignored if no tab has been added in the active window or no other window has been created.
           * This way users can separate between adding tabs to the existing window and adding tabs to a single new window.
           */
          if (can_reuse_window == TRUE && ignore_window_option == TRUE)
            {
              ignore_window_option = FALSE;
              can_reuse_window = FALSE;
              continue;
            }

          /* all new tabs will be added to new windows */
          can_reuse_window = FALSE;

          /* setup new window */
          win_attr = terminal_window_attr_new ();
          tab_attr = win_attr->tabs->data;
          attrs = g_slist_append (attrs, win_attr);
        }
      else if (terminal_option_cmp ("font", 0, argc, argv, &n, &s, &short_offset))
        {
          if (G_UNLIKELY (s == NULL))
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--font\" requires specifying "
                             "the font name as its parameter"));
              goto failed;
            }
          else
            {
              g_free (win_attr->font);
              win_attr->font = g_strdup (s);
              continue;
            }
        }
      else if (terminal_option_cmp ("zoom", 0, argc, argv, &n, &s, &short_offset))
        {
          if (G_UNLIKELY (s == NULL) ||
              strtol (s, &end_ptr, 0) < TERMINAL_ZOOM_LEVEL_MINIMUM ||
              strtol (s, &end_ptr, 0) > TERMINAL_ZOOM_LEVEL_MAXIMUM)
            {
              g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                           _("Option \"--zoom\" requires specifying "
                             "the zoom (%d .. %d) as its parameter"),
                           TERMINAL_ZOOM_LEVEL_MINIMUM, TERMINAL_ZOOM_LEVEL_MAXIMUM);
              goto failed;
            }
          else
            {
              win_attr->zoom = strtol (s, &end_ptr, 0);
              continue;
            }
        }
      else if (terminal_option_cmp ("disable-server", 0, argc, argv, &n, NULL, &short_offset)
               || terminal_option_cmp ("sync", 0, argc, argv, &n, NULL, &short_offset)
               || terminal_option_cmp ("g-fatal-warnings", 0, argc, argv, &n, NULL, &short_offset))
        {
          /* options we can ignore */
          continue;
        }
      else
        {
unknown_option:
          if (short_offset > 0)
            g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                         _("Unknown option \"-%c\""), *(argv[n] + 1 + short_offset));
          else
            g_set_error (error, G_SHELL_ERROR, G_SHELL_ERROR_FAILED,
                         _("Unknown option \"%s\""), argv[n]);

          goto failed;
        }
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
  TerminalWindowAttr *win_attr = g_slice_new0 (TerminalWindowAttr);

  win_attr->workspace = -1;
  win_attr->fullscreen = FALSE;
  win_attr->menubar = TERMINAL_VISIBILITY_DEFAULT;
  win_attr->borders = TERMINAL_VISIBILITY_DEFAULT;
  win_attr->toolbar = TERMINAL_VISIBILITY_DEFAULT;
  win_attr->scrollbar = TERMINAL_VISIBILITY_DEFAULT;
  win_attr->zoom = TERMINAL_ZOOM_LEVEL_DEFAULT;
  win_attr->tabs = g_slist_prepend (NULL, terminal_tab_attr_new ());

  return win_attr;
}



/**
 **/
TerminalTabAttr*
terminal_tab_attr_new (void)
{
  TerminalTabAttr *tab_attr = g_slice_new0 (TerminalTabAttr);

  tab_attr->dynamic_title_mode = TERMINAL_TITLE_DEFAULT;
  tab_attr->position = -1;

  return tab_attr;
}



/**
 * terminal_tab_attr_free:
 * @attr  : A #TerminalTabAttr.
 **/
void
terminal_tab_attr_free (TerminalTabAttr *attr)
{
  terminal_return_if_fail (attr != NULL);

  g_strfreev (attr->command);
  g_free (attr->directory);
  g_free (attr->title);
  g_free (attr->initial_title);
  g_free (attr->color_text);
  g_free (attr->color_bg);
  g_free (attr->color_title);
  g_slice_free (TerminalTabAttr, attr);
}



/**
 * terminal_window_attr_free:
 * @attr  : A #TerminalWindowAttr.
 **/
void
terminal_window_attr_free (TerminalWindowAttr *attr)
{
  terminal_return_if_fail (attr != NULL);

  g_slist_free_full (attr->tabs, (GDestroyNotify) terminal_tab_attr_free);
  g_free (attr->startup_id);
  g_free (attr->sm_client_id);
  g_free (attr->geometry);
  g_free (attr->display);
  g_free (attr->role);
  g_free (attr->icon);
  g_free (attr->font);
  g_slice_free (TerminalWindowAttr, attr);
}
