/* $Id$ */
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
#include <terminal/terminal-private.h>
#include <terminal/terminal-window.h>



static void               terminal_app_finalize                 (GObject            *object);
static void               terminal_app_update_accels            (TerminalApp        *app);
static GtkWidget           *terminal_app_create_window          (TerminalApp        *app,
                                                                 gboolean            fullscreen,
                                                                 TerminalVisibility  menubar,
                                                                 TerminalVisibility  borders,
                                                                 TerminalVisibility  toolbars,
                                                                 gboolean            maximize);
static void               terminal_app_new_window               (TerminalWindow     *window,
                                                                 const gchar        *working_directory,
                                                                 TerminalApp        *app);
static void               terminal_app_new_window_with_terminal (TerminalWindow     *window,
                                                                 TerminalScreen     *terminal,
                                                                 gint                x,
                                                                 gint                y,
                                                                 TerminalApp        *app);
static void               terminal_app_window_destroyed         (GtkWidget          *window,
                                                                 TerminalApp        *app);
static void               terminal_app_save_yourself            (ExoXsessionClient  *client,
                                                                 TerminalApp        *app);
static GdkScreen         *terminal_app_find_screen              (const gchar        *display_name);



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
};



GQuark
terminal_error_quark (void)
{
  static GQuark quark = 0;
  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("terminal-error-quark");
  return quark;
}



G_DEFINE_TYPE (TerminalApp, terminal_app, G_TYPE_OBJECT)



static void
terminal_app_class_init (TerminalAppClass *klass)
{
  GObjectClass *gobject_class;

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

  (*G_OBJECT_CLASS (terminal_app_parent_class)->finalize) (object);
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



static GtkWidget*
terminal_app_create_window (TerminalApp       *app,
                            gboolean           fullscreen,
                            TerminalVisibility menubar,
                            TerminalVisibility borders,
                            TerminalVisibility toolbars,
                            gboolean           maximize)
{
  GtkWidget *window;

  window = terminal_window_new (fullscreen, menubar, borders, toolbars, maximize);
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (terminal_app_window_destroyed), app);
  g_signal_connect (G_OBJECT (window), "new-window",
                    G_CALLBACK (terminal_app_new_window), app);
  g_signal_connect (G_OBJECT (window), "new-window-with-screen",
                    G_CALLBACK (terminal_app_new_window_with_terminal), app);
  app->windows = g_list_append (app->windows, window);

  return window;
}



static void
terminal_app_new_window (TerminalWindow *window,
                         const gchar    *working_directory,
                         TerminalApp    *app)
{
  TerminalWindowAttr *win_attr;
  TerminalTabAttr    *tab_attr;
  TerminalScreen     *terminal;
  GdkScreen          *screen;
  gboolean            inherit_geometry;
  gint                w, h;

  screen = gtk_window_get_screen (GTK_WINDOW (window));

  win_attr = terminal_window_attr_new ();
  win_attr->display = gdk_screen_make_display_name (screen);
  tab_attr = win_attr->tabs->data;
  tab_attr->directory = g_strdup (working_directory);

  /* check if we should try to inherit the parent geometry */
  g_object_get (G_OBJECT (app->preferences), "misc-inherit-geometry", &inherit_geometry, NULL);
  if (G_UNLIKELY (inherit_geometry))
    {
      /* determine the currently active terminal screen for the window */
      terminal = terminal_window_get_active (window);
      if (G_LIKELY (terminal != NULL))
        {
          /* let the new window inherit the geometry from its parent */
          g_free (win_attr->geometry);
          terminal_screen_get_size (terminal, &w, &h);
          win_attr->geometry = g_strdup_printf ("%dx%d", w, h);
        }
    }

  terminal_app_open_window (app, win_attr);

  terminal_window_attr_free (win_attr);
}



static void
terminal_app_new_window_with_terminal (TerminalWindow *existing,
                                       TerminalScreen *terminal,
                                       gint            x,
                                       gint            y,
                                       TerminalApp    *app)
{
  GtkWidget *window;
  GdkScreen *screen;

  _terminal_return_if_fail (TERMINAL_IS_WINDOW (existing));
  _terminal_return_if_fail (TERMINAL_IS_SCREEN (terminal));
  _terminal_return_if_fail (TERMINAL_IS_APP (app));

  window = terminal_app_create_window (app, FALSE,
                                       TERMINAL_VISIBILITY_DEFAULT,
                                       TERMINAL_VISIBILITY_DEFAULT,
                                       TERMINAL_VISIBILITY_DEFAULT,
                                       FALSE);

  /* set new window position */
  if (x > -1 && y > -1)
    gtk_window_move (GTK_WINDOW (window), x, y);

  /* place the new window on the same screen as
   * the existing window.
   */
  screen = gtk_window_get_screen (GTK_WINDOW (existing));
  if (G_LIKELY (screen != NULL))
    gtk_window_set_screen (GTK_WINDOW (window), screen);

  /* this is required to get the geometry right
   * later in the terminal_window_add() call.
   */
  gtk_widget_hide (GTK_WIDGET (terminal));

  terminal_window_add (TERMINAL_WINDOW (window), terminal);

  gtk_widget_show (window);
}



static void
terminal_app_window_destroyed (GtkWidget   *window,
                               TerminalApp *app)
{
  _terminal_return_if_fail (g_list_find (app->windows, window) != NULL);

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
  gchar              **oargv;
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
  for (lp = result, n = 1; n < argc; lp = lp->next, ++n)
    argv[n] = lp->data;
  argv[n] = NULL;

  if (exo_xsession_client_get_restart_command (client, &oargv, NULL))
    {
      _terminal_assert (oargv[0] != NULL);

      argv[0] = oargv[0];

      for (n = 1; oargv[n] != NULL; ++n)
        g_free (oargv[n]);
      g_free (oargv);
    }
  else
    {
      argv[0] = g_strdup ("Terminal");
    }

  exo_xsession_client_set_restart_command (client, argv, argc);

  g_list_free (result);
  g_strfreev (argv);
}



static GdkScreen*
terminal_app_find_screen (const gchar *display_name)
{
  const gchar *other_name;
  GdkColormap *colormap;
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
          other_name = gdk_display_get_name (dp->data);
          if (strncmp (other_name, name, strlen (name)) == 0)
            {
              display = dp->data;
              break;
            }
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

  /* check if we already checked this screen */
  if (g_object_get_data (G_OBJECT (screen), "terminal-checked-screen") == NULL)
    {
      /* check if we can use ARGB visual for this screen */
      colormap = gdk_screen_get_rgba_colormap (screen);
      if (G_LIKELY (colormap != NULL))
        {
          /* use ARGB visual for all windows on this screen, required
           * for real transparency with a compositing manager.
           */
          gdk_screen_set_default_colormap (screen, colormap);
        }

      /* mark this screen as handled */
      g_object_set_data (G_OBJECT (screen), I_("terminal-checked-screen"), GINT_TO_POINTER (1));
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
  gchar           *geometry;
  GList           *lp;

  _terminal_return_if_fail (TERMINAL_IS_APP (app));
  _terminal_return_if_fail (attr != NULL);

  window = terminal_app_create_window (app,
                                       attr->fullscreen,
                                       attr->menubar,
                                       attr->borders,
                                       attr->toolbars,
                                       attr->maximize);

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
      terminal_screen_set_hold (TERMINAL_SCREEN (terminal), tab_attr->hold);

      terminal_window_add (TERMINAL_WINDOW (window), TERMINAL_SCREEN (terminal));

      /* if this was the first tab, we set the geometry string now
       * and show the window. This is required to work around a hang
       * in Gdk, which I failed to figure the cause til now.
       */
      if (lp == attr->tabs)
        {
          /* determine the window geometry */
          if (G_LIKELY (attr->geometry == NULL))
            g_object_get (G_OBJECT (app->preferences), "misc-default-geometry", &geometry, NULL);
          else
            geometry = g_strdup (attr->geometry);

          /* try to apply the geometry to the window */
          if (!gtk_window_parse_geometry (GTK_WINDOW (window), geometry))
            g_printerr (_("Invalid geometry string \"%s\"\n"), geometry);

          /* cleanup */
          g_free (geometry);

          /* show the window */
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




