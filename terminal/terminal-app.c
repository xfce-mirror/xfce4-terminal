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

#include <libxfce4ui/libxfce4ui.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include <terminal/terminal-app.h>
#include <terminal/terminal-config.h>
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-private.h>
#include <terminal/terminal-window.h>
#include <terminal/terminal-window-dropdown.h>

#define ACCEL_MAP_PATH "xfce4/terminal/accels.scm"
#define TERMINAL_DESKTOP_FILE (DATADIR "/applications/xfce4-terminal.desktop")



static void     terminal_app_finalize                 (GObject            *object);
static void     terminal_app_update_accels            (TerminalApp        *app);
static void     terminal_app_update_tab_key_accels    (gpointer            data,
                                                       const gchar        *accel_path,
                                                       guint               accel_key,
                                                       GdkModifierType     accel_mods,
                                                       gboolean            changed);
static void     terminal_app_update_windows_accels    (gpointer            user_data);
static gboolean terminal_app_accel_map_load           (gpointer            user_data);
static gboolean terminal_app_accel_map_save           (gpointer            user_data);
static gboolean terminal_app_unset_urgent_bell        (TerminalWindow     *window,
                                                       GdkEvent           *event,
                                                       TerminalApp        *app);
static void     terminal_app_new_window               (TerminalWindow     *window,
                                                       const gchar        *working_directory,
                                                       TerminalApp        *app);
static void     terminal_app_new_window_with_terminal (TerminalWindow     *existing,
                                                       TerminalScreen     *terminal,
                                                       gint                x,
                                                       gint                y,
                                                       TerminalApp        *app);
static void     terminal_app_window_destroyed         (GtkWidget          *window,
                                                       TerminalApp        *app);
static void     terminal_app_save_yourself            (XfceSMClient       *client,
                                                       TerminalApp        *app);
static void     terminal_app_open_window              (TerminalApp        *app,
                                                       TerminalWindowAttr *attr);



struct _TerminalAppClass
{
  GObjectClass parent_class;
};

struct _TerminalApp
{
  GObject              parent_instance;
  TerminalPreferences *preferences;
  XfceSMClient        *session_client;
  gchar               *initial_menu_bar_accel;
  GSList              *windows;

  guint                accel_map_load_id;
  guint                accel_map_save_id;
  GtkAccelMap         *accel_map;
  GSList              *tab_key_accels;
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
  g_signal_connect_swapped (G_OBJECT (app->preferences), "notify::shortcuts-no-helpkey",
                            G_CALLBACK (terminal_app_update_accels), app);

  /* remember the original menu bar accel */
  g_object_get (G_OBJECT (gtk_settings_get_default ()),
                "gtk-menu-bar-accel", &app->initial_menu_bar_accel,
                NULL);

  terminal_app_update_accels (app);

  /* schedule accel map load and update windows when finished */
  app->accel_map_load_id = gdk_threads_add_idle_full (G_PRIORITY_LOW, terminal_app_accel_map_load, app,
                                                      terminal_app_update_windows_accels);
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
      g_signal_handlers_disconnect_by_func (G_OBJECT (lp->data), G_CALLBACK (terminal_app_new_window_with_terminal), app);
      g_signal_handlers_disconnect_by_func (G_OBJECT (lp->data), G_CALLBACK (terminal_app_unset_urgent_bell), app);
      gtk_widget_destroy (GTK_WIDGET (lp->data));
    }
  g_slist_free (app->windows);

  g_signal_handlers_disconnect_by_func (G_OBJECT (app->preferences), G_CALLBACK (terminal_app_update_accels), app);
  g_object_unref (G_OBJECT (app->preferences));

  if (app->initial_menu_bar_accel != NULL)
    g_free (app->initial_menu_bar_accel);

  if (app->session_client != NULL)
    g_object_unref (G_OBJECT (app->session_client));

  for (lp = app->tab_key_accels; lp != NULL; lp = lp->next)
    g_free (((TerminalAccel*) lp->data)->path);
  g_slist_free_full (app->tab_key_accels, g_free);

  (*G_OBJECT_CLASS (terminal_app_parent_class)->finalize) (object);
}



static void
terminal_app_update_accels (TerminalApp *app)
{
  gboolean        no_key;
  GdkModifierType mod;
  guint           key;

  g_object_get (G_OBJECT (app->preferences),
                "shortcuts-no-menukey", &no_key,
                NULL);
  g_object_set (G_OBJECT (gtk_settings_get_default ()),
                "gtk-menu-bar-accel",
                no_key ? NULL : app->initial_menu_bar_accel,
                NULL);
  gtk_accelerator_parse (app->initial_menu_bar_accel, &key, &mod);
  gtk_accel_map_change_entry ("<Actions>/terminal-window/toggle-menubar",
                              no_key ? 0 : key, no_key ? 0 : mod, TRUE);

  g_object_get (G_OBJECT (app->preferences),
                "shortcuts-no-helpkey", &no_key,
                NULL);
  gtk_accel_map_change_entry ("<Actions>/terminal-window/contents",
                              no_key ? 0 : GDK_KEY_F1, 0, TRUE);
}



static void
terminal_app_update_tab_key_accels (gpointer         data,
                                    const gchar     *accel_path,
                                    guint            accel_key,
                                    GdkModifierType  accel_mods,
                                    gboolean         changed)
{
  if (accel_key == GDK_KEY_Tab || accel_key == GDK_KEY_ISO_Left_Tab)
    {
      TerminalApp *app = TERMINAL_APP (data);
      TerminalAccel *accel = g_new0 (TerminalAccel, 1);

      accel->path = g_strdup (g_strrstr (accel_path, "/") + 1); // <Actions>/terminal-window/action
      accel->mods = accel_mods;

      app->tab_key_accels = g_slist_prepend (app->tab_key_accels, accel);
    }
}



static void
terminal_app_update_windows_accels (gpointer user_data)
{
  TerminalApp *app = TERMINAL_APP (user_data);
  GSList      *lp;

  for (lp = app->windows; lp != NULL; lp = lp->next)
    {
      terminal_window_rebuild_tabs_menu (TERMINAL_WINDOW (lp->data));
      terminal_window_update_tab_key_accels (TERMINAL_WINDOW (lp->data), app->tab_key_accels);
    }

  app->accel_map_load_id = 0;
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
  app->accel_map_save_id = gdk_threads_add_timeout_seconds (10, terminal_app_accel_map_save, app);
}



static gboolean
terminal_app_accel_map_load (gpointer user_data)
{
  TerminalApp *app = TERMINAL_APP (user_data);
  gchar       *path;
  gchar        name[50];
  guint        i;

  path = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, ACCEL_MAP_PATH);
  if (G_LIKELY (path != NULL))
    {
      /* load the accel map */
      gtk_accel_map_load (path);
      g_free (path);
    }

  /* watch for changes */
  app->accel_map = gtk_accel_map_get ();
  g_signal_connect_swapped (G_OBJECT (app->accel_map), "changed",
      G_CALLBACK (terminal_app_accel_map_changed), app);

  /* check and create default Alt+N accelerators */
  for (i = 1; i < 10; i++)
    {
      g_snprintf (name, sizeof (name), "<Actions>/terminal-window/goto-tab-%d", i);
      if (!gtk_accel_map_lookup_entry (name, NULL))
        gtk_accel_map_change_entry (name, GDK_KEY_0 + i, GDK_MOD1_MASK, FALSE);
    }

  /* identify accelerators containing the Tab key */
  if (app->tab_key_accels != NULL)
    {
      GSList *lp;
      for (lp = app->tab_key_accels; lp != NULL; lp = lp->next)
        g_free (((TerminalAccel*) lp->data)->path);
      g_slist_free_full (app->tab_key_accels, g_free);
      app->tab_key_accels = NULL;
    }
  gtk_accel_map_foreach (app, terminal_app_update_tab_key_accels);

  return FALSE;
}



static gboolean
terminal_app_unset_urgent_bell (TerminalWindow *window,
                                GdkEvent       *event,
                                TerminalApp    *app)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (window));
  gtk_window_set_urgency_hint (GTK_WINDOW (toplevel), FALSE);

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
  g_object_unref (group);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (terminal_app_window_destroyed), app);
  g_signal_connect (G_OBJECT (window), "new-window",
                    G_CALLBACK (terminal_app_new_window), app);
  g_signal_connect (G_OBJECT (window), "new-window-with-screen",
                    G_CALLBACK (terminal_app_new_window_with_terminal), app);
  g_signal_connect (G_OBJECT (window), "focus-in-event",
                    G_CALLBACK (terminal_app_unset_urgent_bell), app);
  g_signal_connect (G_OBJECT (window), "key-release-event",
                    G_CALLBACK (terminal_app_unset_urgent_bell), app);
  app->windows = g_slist_prepend (app->windows, window);

  terminal_window_update_tab_key_accels (TERMINAL_WINDOW (window), app->tab_key_accels);
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
      new_role = g_strdup_printf ("%s-%u-%u", PACKAGE_NAME, (guint) time (NULL), g_random_int ());
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
                               const gchar        *icon,
                               gboolean            fullscreen,
                               TerminalVisibility  menubar,
                               TerminalVisibility  toolbar)
{
  GtkWidget *window;

  window = terminal_window_dropdown_new (role, icon, fullscreen, menubar, toolbar);

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
  win_attr->display = g_strdup (gdk_display_get_name (gdk_screen_get_display (screen)));
  tab_attr = win_attr->tabs->data;
  tab_attr->directory = g_strdup (working_directory);
  if (terminal_window_get_font (window))
    win_attr->font = g_strdup (terminal_window_get_font (window));
  win_attr->zoom = terminal_window_get_zoom_level (window);

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
  glong      width, height;

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

  if (G_UNLIKELY (terminal_window_is_drop_down (existing)))
    {
      /* resize new window to the default geometry */
#ifdef GDK_WINDOWING_X11
      gchar *geo;
      guint  w, h;
      g_object_get (G_OBJECT (app->preferences), "misc-default-geometry", &geo, NULL);
      if (G_LIKELY (geo != NULL))
        {
          XParseGeometry (geo, NULL, NULL, &w, &h);
          g_free (geo);
          terminal_screen_force_resize_window (terminal, GTK_WINDOW (window), w, h);
        }
#endif
    }
  else
    {
      /* resize new window to the original terminal geometry */
      terminal_screen_get_size (terminal, &width, &height);
      terminal_screen_force_resize_window (terminal, GTK_WINDOW (window), width, height);
    }

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

  /* no windows were saved - this can happen if there is only a dropdown window
     that we don't want to save */
  if (result == NULL)
    return;

  argc = g_slist_length (result) + 1;
  argv = g_new (gchar*, argc + 1);
  for (lp = result, n = 1; n < argc && lp != NULL; lp = lp->next, ++n)
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
  GtkWidget       *window;
  TerminalScreen  *terminal;
  GdkScreen       *screen;
  gchar           *geometry;
  GSList          *lp;
  gboolean         reuse_window = FALSE;
  GdkDisplay      *attr_display;
  gint             attr_screen_num;
  gint             active_tab = -1, i;
#ifdef GDK_WINDOWING_X11
  GdkGravity       gravity = GDK_GRAVITY_NORTH_WEST;
  gint             mask = NoValue, x, y, new_x, new_y;
  guint            width = 1, height = 1, new_width, new_height;
  gint             screen_width = 0, screen_height = 0;
  gint             window_width, window_height;
#endif

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
                                                  attr->icon,
                                                  attr->fullscreen,
                                                  attr->menubar,
                                                  attr->toolbar);

          /* drop-down window can be maximized */
          if (attr->maximize)
            gtk_window_maximize (GTK_WINDOW (window));

          /* put it on the correct screen/display */
          screen = terminal_app_find_screen (attr_display, attr_screen_num);
          gtk_window_set_screen (GTK_WINDOW (window), screen);
        }
    }
  else if (attr->reuse_last_window && app->windows != NULL)
    {
      /* open tabs in the existing window */
      window = app->windows->data;
      /* try to find active window (bug #13891) */
      for (lp = app->windows; lp != NULL; lp = lp->next)
        {
          if (gtk_window_has_toplevel_focus (GTK_WINDOW (lp->data)))
            {
              window = lp->data;
              break;
            }
        }
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
      if (attr->minimize)
        gtk_window_iconify (GTK_WINDOW (window));

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

  terminal_window_set_scrollbar_visibility (TERMINAL_WINDOW (window), attr->scrollbar);

  /* font and zoom for new window */
  if (!reuse_window)
    {
      if (attr->font)
        terminal_window_set_font (TERMINAL_WINDOW (window), attr->font);
      terminal_window_set_zoom_level (TERMINAL_WINDOW (window), attr->zoom);
    }

  if (!attr->drop_down)
    {
      /* try to apply the geometry to the window */
      g_object_get (G_OBJECT (app->preferences), "misc-default-geometry", &geometry, NULL);
#ifdef GDK_WINDOWING_X11
      /* defaults */
      mask = XParseGeometry (geometry, &x, &y, &width, &height);

      /* geometry provided via command line parameter */
      if (G_UNLIKELY (attr->geometry != NULL))
        {
          g_free (geometry);
          geometry = g_strdup (attr->geometry);
          mask = XParseGeometry (geometry, &new_x, &new_y, &new_width, &new_height);

          if (mask & WidthValue)
            width = new_width;
          if (mask & HeightValue)
            height = new_height;
          if (mask & XValue)
            x = new_x;
          if (mask & YValue)
            y = new_y;
        }
#endif
    }

  /* add the tabs */
  for (lp = attr->tabs, i = 0; lp != NULL; lp = lp->next, ++i)
    {
      TerminalTabAttr *tab_attr = (TerminalTabAttr *) lp->data;
      terminal = terminal_screen_new (tab_attr, width, height);
      terminal_window_add (TERMINAL_WINDOW (window), terminal);
      terminal_screen_launch_child (terminal);

      /* whether the tab was set as active */
      if (G_UNLIKELY (tab_attr->active))
        active_tab = i;
    }

  /* set active tab */
  if (active_tab > -1)
    {
      GtkNotebook *notebook = GTK_NOTEBOOK (terminal_window_get_notebook (TERMINAL_WINDOW (window)));
      gtk_notebook_set_current_page (notebook, active_tab);
    }

  if (!attr->drop_down)
    {
      /* move the window to desired position */
#ifdef GDK_WINDOWING_X11
      if ((mask & XValue) || (mask & YValue))
        {
          screen = gtk_window_get_screen (GTK_WINDOW (window));
          gdk_window_get_geometry (gdk_screen_get_root_window (screen), NULL, NULL,
                                   &screen_width, &screen_height);
          gtk_window_get_size (GTK_WINDOW (window), &window_width, &window_height);
          if (mask & XNegative)
            {
              x = screen_width - window_width + x;
              gravity = GDK_GRAVITY_NORTH_EAST;
            }
          if (mask & YNegative)
            {
              y = screen_height - window_height + y;
              gravity = (mask & XNegative) ? GDK_GRAVITY_SOUTH_EAST : GDK_GRAVITY_SOUTH_WEST;
            }
          gtk_window_set_gravity (GTK_WINDOW (window), gravity);
          gtk_window_move (GTK_WINDOW (window), x, y);
        }
      else if (!(mask & WidthValue) && !(mask & XValue))
#else
      if (!gtk_window_parse_geometry (GTK_WINDOW (window), geometry))
#endif
        g_printerr (_("Invalid geometry string \"%s\"\n"), geometry);

      /* cleanup */
      g_free (geometry);
    }

  /* show the window */
  if (attr->drop_down)
    terminal_window_dropdown_toggle (TERMINAL_WINDOW_DROPDOWN (window), attr->startup_id, reuse_window);
  else
    {
      /* save window geometry to prevent overriding */
      terminal_window_set_grid_size (TERMINAL_WINDOW (window), width, height);
      terminal_screen_force_resize_window (terminal_window_get_active (TERMINAL_WINDOW (window)),
                                           GTK_WINDOW (window), width, height);

      if (reuse_window)
        gtk_window_present (GTK_WINDOW (window));
      else
        gtk_widget_show (window);
    }
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
          xfce_sm_client_set_desktop_file (app->session_client, TERMINAL_DESKTOP_FILE);
          g_signal_connect (G_OBJECT (app->session_client), "save-state",
                            G_CALLBACK (terminal_app_save_yourself), app);
          g_signal_connect (G_OBJECT (app->session_client), "quit",
                            G_CALLBACK (gtk_main_quit), NULL);
        }
      else
        {
          g_printerr (_("Failed to connect to session manager: %s\n"), err->message);
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
