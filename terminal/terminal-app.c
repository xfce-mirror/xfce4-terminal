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

#include <gdk/gdkkeysyms.h>

#include <terminal/terminal-app.h>
#include <terminal/terminal-config.h>
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-private.h>
#include <terminal/terminal-window.h>
#include <terminal/terminal-window-dropdown.h>

#define ACCEL_MAP_PATH "xfce4/terminal/accels.scm"



static void               terminal_app_finalize                 (GObject            *object);
static void               terminal_app_update_accels            (TerminalApp        *app);
static void               terminal_app_update_mnemonics         (TerminalApp        *app);
static gboolean           terminal_app_accel_map_load           (gpointer            user_data);
static gboolean           terminal_app_accel_map_save           (gpointer            user_data);
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
static void               terminal_app_save_yourself            (XfceSMClient       *client,
                                                                 TerminalApp        *app);
static void               terminal_app_open_window              (TerminalApp        *app,
                                                                 TerminalWindowAttr *attr);



struct _TerminalAppClass
{
  GObjectClass  __parent__;
};

struct _TerminalApp
{
  GObject              __parent__;
  TerminalPreferences *preferences;
  XfceSMClient        *session_client;
  gchar               *initial_menu_bar_accel;
  GSList              *windows;

  guint                accel_map_load_id;
  guint                accel_map_save_id;
  GtkAccelMap         *accel_map;
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
  g_signal_connect_swapped (G_OBJECT (app->preferences), "notify::shortcuts-no-mnemonics",
                            G_CALLBACK (terminal_app_update_mnemonics), app);

  /* remember the original menu bar accel */
  g_object_get (G_OBJECT (gtk_settings_get_default ()),
                "gtk-menu-bar-accel", &app->initial_menu_bar_accel,
                NULL);

  terminal_app_update_accels (app);
  terminal_app_update_mnemonics (app);

  /* schedule accel map load */
  app->accel_map_load_id = g_idle_add_full (G_PRIORITY_LOW, terminal_app_accel_map_load, app, NULL);
}



static void
terminal_app_finalize (GObject *object)
{
  TerminalApp *app = TERMINAL_APP (object);
  GSList      *lp;

  /* stop accel map stuff */
  if (G_UNLIKELY (app->accel_map_load_id != 0))
    g_source_remove (app->accel_map_load_id);
  if (app->accel_map != NULL)
    g_object_unref (G_OBJECT (app->accel_map));
  if (G_UNLIKELY (app->accel_map_save_id != 0))
    {
      g_source_remove (app->accel_map_save_id);
      terminal_app_accel_map_save (app);
    }

  for (lp = app->windows; lp != NULL; lp = lp->next)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (lp->data), G_CALLBACK (terminal_app_window_destroyed), app);
      g_signal_handlers_disconnect_by_func (G_OBJECT (lp->data), G_CALLBACK (terminal_app_new_window), app);
      gtk_widget_destroy (GTK_WIDGET (lp->data));
    }
  g_slist_free (app->windows);

  g_signal_handlers_disconnect_by_func (G_OBJECT (app->preferences), G_CALLBACK (terminal_app_update_accels), app);
  g_signal_handlers_disconnect_by_func (G_OBJECT (app->preferences), G_CALLBACK (terminal_app_update_mnemonics), app);
  g_object_unref (G_OBJECT (app->preferences));

  if (app->initial_menu_bar_accel != NULL)
    g_free (app->initial_menu_bar_accel);

  if (app->session_client != NULL)
    g_object_unref (G_OBJECT (app->session_client));

  (*G_OBJECT_CLASS (terminal_app_parent_class)->finalize) (object);
}



static void
terminal_app_update_accels (TerminalApp *app)
{
  gboolean no_menukey;

  g_object_get (G_OBJECT (app->preferences),
                "shortcuts-no-menukey", &no_menukey,
                NULL);

  gtk_settings_set_string_property (gtk_settings_get_default (),
                                    "gtk-menu-bar-accel",
                                    no_menukey ? "" : app->initial_menu_bar_accel,
                                    PACKAGE_NAME);
}



static void
terminal_app_update_mnemonics (TerminalApp *app)
{
  gboolean no_mnemonics;

  g_object_get (G_OBJECT (app->preferences),
                "shortcuts-no-mnemonics", &no_mnemonics,
                NULL);
  g_object_set (G_OBJECT (gtk_settings_get_default ()),
                "gtk-enable-mnemonics", !no_mnemonics,
                NULL);
}



static gboolean
terminal_app_accel_map_save (gpointer user_data)
{
  TerminalApp *app = TERMINAL_APP (user_data);
  gchar       *path;

  app->accel_map_save_id = 0;

  /* save the current accel map */
  path = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, ACCEL_MAP_PATH, TRUE);
  if (G_LIKELY (path != NULL))
    {
      /* save the accel map */
      gtk_accel_map_save (path);
      g_free (path);
    }

  return FALSE;
}



static void
terminal_app_accel_map_changed (TerminalApp *app)
{
  /* stop pending save */
  if (app->accel_map_save_id != 0)
    {
      g_source_remove (app->accel_map_save_id);
      app->accel_map_save_id = 0;
    }

  /* schedule new save */
  app->accel_map_save_id = g_timeout_add_seconds (10, terminal_app_accel_map_save, app);
}



static gboolean
terminal_app_accel_map_load (gpointer user_data)
{
  TerminalApp *app = TERMINAL_APP (user_data);
  gchar       *path;
  gchar        name[50];
  guint        i;

  app->accel_map_load_id = 0;

  path = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, ACCEL_MAP_PATH);
  if (G_LIKELY (path != NULL))
    {
      /* load the accel map */
      gtk_accel_map_load (path);
      g_free (path);
    }
  else
    {
      /* create default Alt+N accelerators */
      for (i = 1; i < 10; i++)
        {
          g_snprintf (name, sizeof (name), "<Actions>/terminal-window/goto-tab-%d", i);
          gtk_accel_map_change_entry (name, GDK_KEY_0 + i, GDK_MOD1_MASK, FALSE);
        }
    }

  /* watch for changes */
  app->accel_map = gtk_accel_map_get ();
  g_signal_connect_swapped (G_OBJECT (app->accel_map), "changed",
      G_CALLBACK (terminal_app_accel_map_changed), app);

  return FALSE;
}



static void
terminal_app_take_window (TerminalApp *app,
                          GtkWindow   *window)
{
  GtkWindowGroup *group;

  terminal_return_if_fail (GTK_IS_WINDOW (window));

  group = gtk_window_group_new ();
  gtk_window_group_add_window (group, window);
  g_object_weak_ref (G_OBJECT (window), (GWeakNotify) g_object_unref, group);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (terminal_app_window_destroyed), app);
  g_signal_connect (G_OBJECT (window), "new-window",
                    G_CALLBACK (terminal_app_new_window), app);
  g_signal_connect (G_OBJECT (window), "new-window-with-screen",
                    G_CALLBACK (terminal_app_new_window_with_terminal), app);
  app->windows = g_slist_prepend (app->windows, window);
}



static GtkWidget*
terminal_app_create_window (TerminalApp       *app,
                            const gchar       *role,
                            gboolean           fullscreen,
                            TerminalVisibility menubar,
                            TerminalVisibility borders,
                            TerminalVisibility toolbar)
{
  GtkWidget *window;
  gchar     *new_role = NULL;

  if (role == NULL)
    {
      /* create a new window role */
      new_role =  g_strdup_printf (PACKAGE_NAME "-%u-%d", (guint) time (NULL), g_random_int ());
      role = new_role;
    }

  window = terminal_window_new (role, fullscreen, menubar, borders, toolbar);
  g_free (new_role);

  terminal_app_take_window (app, GTK_WINDOW (window));

  return window;
}



static GtkWidget*
terminal_app_create_drop_down (TerminalApp        *app,
                               const gchar        *role,
                               gboolean            fullscreen,
                               TerminalVisibility  menubar,
                               TerminalVisibility  toolbar)
{
  GtkWidget *window;

  window = terminal_window_dropdown_new (role, fullscreen, menubar, toolbar);

  terminal_app_take_window (app, GTK_WINDOW (window));

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
  glong               w, h;

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
          win_attr->geometry = g_strdup_printf ("%ldx%ld", w, h);
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

  terminal_return_if_fail (TERMINAL_IS_WINDOW (existing));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (terminal));
  terminal_return_if_fail (TERMINAL_IS_APP (app));

  window = terminal_app_create_window (app, NULL, FALSE,
                                       TERMINAL_VISIBILITY_DEFAULT,
                                       TERMINAL_VISIBILITY_DEFAULT,
                                       TERMINAL_VISIBILITY_DEFAULT);

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
  terminal_return_if_fail (g_slist_find (app->windows, window) != NULL);

  app->windows = g_slist_remove (app->windows, window);

  if (G_UNLIKELY (app->windows == NULL))
    gtk_main_quit ();
}



static void
terminal_app_save_yourself (XfceSMClient *client,
                            TerminalApp  *app)
{
  GSList               *result = NULL;
  GSList               *lp;
  const gchar * const  *oargv;
  gchar               **argv;
  gint                  argc;
  gint                  n;

  for (lp = app->windows, n = 0; lp != NULL; lp = lp->next)
    {
      /* don't session save dropdown windows */
      if (TERMINAL_IS_WINDOW_DROPDOWN (lp->data))
        continue;

      if (n++ != 0)
        result = g_slist_append (result, g_strdup ("--window"));
      result = g_slist_concat (result, terminal_window_get_restart_command (lp->data));
    }

  argc = g_slist_length (result) + 1;
  argv = g_new (gchar*, argc + 1);
  for (lp = result, n = 1; n < argc; lp = lp->next, ++n)
    argv[n] = lp->data;
  argv[n] = NULL;

  oargv = xfce_sm_client_get_restart_command (client);
  if (oargv != NULL)
    {
      terminal_assert (oargv[0] != NULL);
      argv[0] = g_strdup (oargv[0]);
    }
  else
    {
      argv[0] = g_strdup (PACKAGE_NAME);
    }

  xfce_sm_client_set_restart_command (client, argv);

  g_slist_free (result);
  g_strfreev (argv);
}



static GdkDisplay *
terminal_app_find_display (const gchar *display_name,
                           gint        *screen_num)
{
  const gchar *other_name;
  GdkDisplay  *display = NULL;
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

      /* extract screen number from display name */
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

      g_free (name);

      if (display == NULL)
        display = gdk_display_open (display_name);
    }

  if (display == NULL)
    display = gdk_display_get_default ();

  if (screen_num != NULL)
    *screen_num = num;

  return display;
}



static GdkScreen *
terminal_app_find_screen (GdkDisplay *display,
                          gint        screen_num)
{
  GdkScreen *screen = NULL;

  if (display != NULL)
    {
      if (screen_num >= 0)
        screen = gdk_display_get_screen (display, screen_num);

      if (screen == NULL)
        screen = gdk_display_get_default_screen (display);

      if (screen != NULL)
        g_object_ref (G_OBJECT (screen));
    }

  if (screen == NULL)
    {
      screen = gdk_screen_get_default ();
      g_object_ref (G_OBJECT (screen));
    }

  return screen;
}



static GdkScreen *
terminal_app_find_screen_by_name (const gchar *display_name)
{
  GdkDisplay *display;
  gint        screen_num;

  display = terminal_app_find_display (display_name, &screen_num);
  return terminal_app_find_screen (display, screen_num);
}



static void
terminal_app_open_window (TerminalApp        *app,
                          TerminalWindowAttr *attr)
{
  TerminalTabAttr *tab_attr = NULL;
  GtkWidget       *window;
  GtkWidget       *terminal;
  GdkScreen       *screen;
  gchar           *geometry;
  GSList          *lp;
  gboolean         reuse_window = FALSE;
  GdkDisplay      *attr_display;
  gint             attr_screen_num;

  terminal_return_if_fail (TERMINAL_IS_APP (app));
  terminal_return_if_fail (attr != NULL);

  if (attr->drop_down)
    {
      /* look for an exising drop-down window */
      attr_display = terminal_app_find_display (attr->display, &attr_screen_num);
      for (lp = app->windows; lp != NULL; lp = lp->next)
        {
          if (TERMINAL_IS_WINDOW_DROPDOWN (lp->data))
            {
              /* check if the screen of the display matches (bug #9957) */
              screen = gtk_window_get_screen (GTK_WINDOW (lp->data));
              if (gdk_screen_get_display (screen) == attr_display)
                break;
            }
        }

      if (lp != NULL)
        {
          if (G_UNLIKELY (attr->reuse_last_window))
            {
              /* use the drop-down window to insert the tab */
              window = lp->data;
              reuse_window = TRUE;
            }
          else
            {
              /* toggle state of visible window */
              terminal_window_dropdown_toggle (lp->data, attr->startup_id, FALSE);
              return;
            }
        }
      else
        {
          /* create new drop-down window */
          window = terminal_app_create_drop_down (app,
                                                  attr->role,
                                                  attr->fullscreen,
                                                  attr->menubar,
                                                  attr->toolbar);

          /* put it on the correct screen/display */
          screen = terminal_app_find_screen (attr_display, attr_screen_num);
          gtk_window_set_screen (GTK_WINDOW (window), screen);
        }
    }
  else if (attr->reuse_last_window
           && app->windows != NULL)
    {
      /* open the tabs in an existing window */
      window = app->windows->data;
      reuse_window = TRUE;
    }
  else
    {
      /* create new window */
      window = terminal_app_create_window (app,
                                           attr->role,
                                           attr->fullscreen,
                                           attr->menubar,
                                           attr->borders,
                                           attr->toolbar);

      /* apply normal window properties */
      if (attr->maximize)
        gtk_window_maximize (GTK_WINDOW (window));

      if (attr->startup_id != NULL)
        gtk_window_set_startup_id (GTK_WINDOW (window), attr->startup_id);

      if (attr->icon != NULL)
        {
          if (g_path_is_absolute (attr->icon))
            gtk_window_set_icon_from_file (GTK_WINDOW (window), attr->icon, NULL);
          else
            gtk_window_set_icon_name (GTK_WINDOW (window), attr->icon);
        }

      screen = terminal_app_find_screen_by_name (attr->display);
      if (G_LIKELY (screen != NULL))
        {
          gtk_window_set_screen (GTK_WINDOW (window), screen);
          g_object_unref (G_OBJECT (screen));
        }
    }

  /* add the tabs */
  for (lp = attr->tabs; lp != NULL; lp = lp->next)
    {
      terminal = g_object_new (TERMINAL_TYPE_SCREEN, NULL);

      tab_attr = lp->data;
      if (tab_attr->command != NULL)
        terminal_screen_set_custom_command (TERMINAL_SCREEN (terminal), tab_attr->command);
      if (tab_attr->directory != NULL)
        terminal_screen_set_working_directory (TERMINAL_SCREEN (terminal), tab_attr->directory);
      if (tab_attr->title != NULL)
        terminal_screen_set_custom_title (TERMINAL_SCREEN (terminal), tab_attr->title);
      terminal_screen_set_hold (TERMINAL_SCREEN (terminal), tab_attr->hold);

      terminal_window_add (TERMINAL_WINDOW (window), TERMINAL_SCREEN (terminal));

      terminal_screen_launch_child (TERMINAL_SCREEN (terminal));
    }

  if (!attr->drop_down)
    {
      /* don't apply other attributes to the window when reusing */
      if (reuse_window)
        return;

      /* set the window geometry, this can only be set after one of the tabs
       * has been added, because vte is the geometry widget, so atleast one
       * call should have been made to terminal_screen_set_window_geometry_hints */
      if (G_LIKELY (attr->geometry == NULL))
        g_object_get (G_OBJECT (app->preferences), "misc-default-geometry", &geometry, NULL);
      else
        geometry = g_strdup (attr->geometry);

      /* try to apply the geometry to the window */
      if (!gtk_window_parse_geometry (GTK_WINDOW (window), geometry))
        g_printerr (_("Invalid geometry string \"%s\"\n"), geometry);

      /* cleanup */
      g_free (geometry);
    }

  /* show the window */
  if (attr->drop_down)
    terminal_window_dropdown_toggle (TERMINAL_WINDOW_DROPDOWN (window), attr->startup_id, reuse_window);
  else
    gtk_widget_show (window);
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
  GSList             *attrs, *lp;
  gchar              *sm_client_id = NULL;
  TerminalWindowAttr *attr;
  GError             *err = NULL;

  attrs = terminal_window_attr_parse (argc, argv, app->windows != NULL, error);
  if (G_UNLIKELY (attrs == NULL))
    return FALSE;

  /* Connect to session manager first before starting any other windows */
  for (lp = attrs; lp != NULL; lp = lp->next)
    {
      attr = lp->data;

      /* take first sm client id */
      if (attr->sm_client_id != NULL)
        {
          sm_client_id = g_strdup (attr->sm_client_id);
          break;
        }
    }

  if (G_LIKELY (app->session_client == NULL))
    {
      app->session_client = xfce_sm_client_get_full (XFCE_SM_CLIENT_RESTART_NORMAL,
                                                     XFCE_SM_CLIENT_PRIORITY_DEFAULT,
                                                     sm_client_id,
                                                     xfce_get_homedir (),
                                                     NULL,
                                                     PACKAGE_NAME ".desktop");
      if (xfce_sm_client_connect (app->session_client, &err))
        {
          g_signal_connect (G_OBJECT (app->session_client), "save-state",
                            G_CALLBACK (terminal_app_save_yourself), app);
          g_signal_connect (G_OBJECT (app->session_client), "quit",
                            G_CALLBACK (gtk_main_quit), NULL);
        }
      else
        {
          g_printerr ("Failed to connect to session manager: %s\n", err->message);
          g_error_free (err);
        }
    }

  for (lp = attrs; lp != NULL; lp = lp->next)
    {
      attr = lp->data;

      terminal_app_open_window (app, attr);
      terminal_window_attr_free (attr);
    }

  g_slist_free (attrs);
  g_free (sm_client_id);

  return TRUE;
}
