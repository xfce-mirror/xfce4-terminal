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
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 *
 * The geometry handling code was taken from gnome-terminal. The geometry hacks
 * where initially written by Owen Taylor.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <exo/exo.h>
#include <gdk/gdkkeysyms.h>

#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-tab-header.h>
#include <terminal/terminal-window.h>



enum
{
  NEW_WINDOW,
  LAST_SIGNAL,
};



static void            terminal_window_dispose                  (GObject         *object);
static void            terminal_window_finalize                 (GObject         *object);
static TerminalWidget *terminal_window_get_active               (TerminalWindow  *window);
static void            terminal_window_set_size_force_grid      (TerminalWindow  *window,
                                                                 TerminalWidget  *widget,
                                                                 gint             force_grid_width,
                                                                 gint             force_grid_height);
static void            terminal_window_update_geometry          (TerminalWindow  *window);
static void            terminal_window_update_actions           (TerminalWindow  *window);
static void            terminal_window_notify_page              (GtkNotebook     *notebook,
                                                                 GParamSpec      *pspec,
                                                                 TerminalWindow  *window);
static void            terminal_window_context_menu             (TerminalWidget  *widget,
                                                                 GdkEvent        *event,
                                                                 TerminalWindow  *window);
static void            terminal_window_notify_title             (TerminalWidget       *widget,
                                                                 GParamSpec           *pspec,
                                                                 TerminalWindow       *window);
static void            terminal_window_widget_removed           (GtkNotebook     *notebook,
                                                                 TerminalWidget  *widget,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_new_tab           (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_new_window        (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_close_tab         (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_close_window      (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_copy              (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_paste             (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_prefs             (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_show_menubar      (GtkToggleAction *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_show_borders      (GtkToggleAction *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_fullscreen        (GtkToggleAction *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_prev_tab          (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_next_tab          (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_set_title         (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_reset             (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_reset_and_clear   (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_contents          (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_report_bug        (GtkAction       *action,
                                                                 TerminalWindow  *window);
static void            terminal_window_action_about             (GtkAction       *action,
                                                                 TerminalWindow  *window);
static gboolean        terminal_window_prefs_idle               (gpointer         user_data);
static void            terminal_window_prefs_idle_destroy       (gpointer         user_data);
static gboolean        terminal_window_title_idle               (gpointer         user_data);
static void            terminal_window_title_idle_destroy       (gpointer         user_data);
static gboolean        terminal_window_about_idle               (gpointer         user_data);
static void            terminal_window_about_idle_destroy       (gpointer         user_data);



struct _TerminalWindow
{
  GtkWindow            __parent__;

  TerminalPreferences *preferences;

  GtkActionGroup      *action_group;
  GtkUIManager        *ui_manager;
  
  GtkWidget           *menubar;
  GtkWidget           *notebook;

  guint                prefs_idle_id;
  guint                title_idle_id;
  guint                about_idle_id;
};



static GObjectClass *parent_class;
static guint         window_signals[LAST_SIGNAL];

static GtkActionEntry action_entries[] =
{
  { "file-menu", NULL, N_ ("_File"), },
  { "new-tab", "terminal-newtab", N_ ("Open _Tab"), NULL, NULL, G_CALLBACK (terminal_window_action_new_tab), }, 
  { "new-window", "terminal-newwindow", N_ ("Open _Terminal"), "<control><shift>N", NULL, G_CALLBACK (terminal_window_action_new_window), }, 
  { "close-tab", NULL, N_ ("C_lose Tab"), NULL, NULL, G_CALLBACK (terminal_window_action_close_tab), },
  { "close-window", NULL, N_ ("_Close Window"), NULL, NULL, G_CALLBACK (terminal_window_action_close_window), },
  { "edit-menu", NULL, N_ ("_Edit"),  },
  { "copy", GTK_STOCK_COPY, N_ ("_Copy"), NULL, NULL, G_CALLBACK (terminal_window_action_copy), },
  { "paste", GTK_STOCK_PASTE, N_ ("_Paste"), NULL, NULL, G_CALLBACK (terminal_window_action_paste), },
  { "preferences", GTK_STOCK_PREFERENCES, N_ ("Preferences"), NULL, NULL, G_CALLBACK (terminal_window_action_prefs), },
  { "view-menu", NULL, N_ ("_View"), },
  { "terminal-menu", NULL, N_ ("_Terminal"), },
  { "prev-tab", GTK_STOCK_GO_BACK, N_ ("_Previous Tab"), NULL, NULL, G_CALLBACK (terminal_window_action_prev_tab), },
  { "next-tab", GTK_STOCK_GO_FORWARD, N_ ("_Next Tab"), NULL, NULL, G_CALLBACK (terminal_window_action_next_tab), },
  { "set-title", NULL, N_ ("_Set Title"), NULL, NULL, G_CALLBACK (terminal_window_action_set_title), },
  { "reset", NULL, N_ ("_Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset), },
  { "reset-and-clear", NULL, N_ ("Reset and C_lear"), NULL, NULL, G_CALLBACK (terminal_window_action_reset_and_clear), },
  { "help-menu", NULL, N_ ("_Help"), },
  { "contents", GTK_STOCK_HELP, N_ ("_Contents"), NULL, NULL, G_CALLBACK (terminal_window_action_contents), },
  { "report-bug", "terminal-reportbug", N_ ("_Report a bug"), NULL, NULL, G_CALLBACK (terminal_window_action_report_bug), },
  { "about", GTK_STOCK_DIALOG_INFO, N_ ("_About"), NULL, NULL, G_CALLBACK (terminal_window_action_about), },
  { "input-methods", NULL, N_ ("_Input Methods"), NULL, NULL, NULL, },
};

static GtkToggleActionEntry toggle_action_entries[] =
{
  { "show-menubar", NULL, N_ ("Show _Menubar"), NULL, NULL, G_CALLBACK (terminal_window_action_show_menubar), },
  { "show-borders", NULL, N_ ("Show Window _Borders"), NULL, NULL, G_CALLBACK (terminal_window_action_show_borders), },
  { "fullscreen", NULL, N_ ("_Fullscreen"), NULL, NULL, G_CALLBACK (terminal_window_action_fullscreen), },
};

#if 0
static const gchar ui_description[] =
 "<ui>"
 "  <menubar name='main-menu'>"
 "    <menu action='file-menu'>"
 "      <menuitem action='new-tab'/>"
 "      <menuitem action='new-window'/>"
 "      <separator/>"
 "      <menuitem action='close-tab'/>"
 "      <menuitem action='close-window'/>"
 "    </menu>"
 "    <menu action='edit-menu'>"
 "      <menuitem action='copy'/>"
 "      <menuitem action='paste'/>"
 "      <separator/>"
 "      <menuitem action='preferences'/>"
 "    </menu>"
 "    <menu action='view-menu'>"
 "      <menuitem action='show-menubar'/>"
 "      <menuitem action='show-borders'/>"
 "      <menuitem action='fullscreen'/>"
 "    </menu>"
 "    <menu action='terminal-menu'>"
 "      <menuitem action='prev-tab'/>"
 "      <menuitem action='next-tab'/>"
 "      <separator/>"
 "      <menuitem action='set-title'/>"
 "      <menuitem action='reset'/>"
 "      <menuitem action='reset-and-clear'/>"
 "    </menu>"
 "    <menu action='help-menu'>"
 "      <menuitem action='contents'/>"
 "      <menuitem action='report-bug'/>"
 "      <menuitem action='about'/>"
 "    </menu>"
 "  </menubar>"
 ""
 "  <popup name='popup-menu'>"
 "    <menuitem action='new-window'/>"
 "    <menuitem action='new-tab'/>"
 "    <separator/>"
 "    <menuitem action='close-tab'/>"
 "    <separator/>"
 "    <menuitem action='copy'/>"
 "    <menuitem action='paste'/>"
 "    <separator/>"
 "    <menuitem action='show-menubar'/>"
 "    <menuitem action='fullscreen'/>"
 "    <menuitem action='preferences'/>"
 "    <separator/>"
 "    <menuitem action='input-methods'/>"
 "  </popup>"
 "</ui>";
#endif



G_DEFINE_TYPE (TerminalWindow, terminal_window, GTK_TYPE_WINDOW);



static void
terminal_window_class_init (TerminalWindowClass *klass)
{
  GObjectClass *gobject_class;
  
  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_window_dispose;
  gobject_class->finalize = terminal_window_finalize;

  /**
   * TerminalWindow::new-window
   **/
  window_signals[NEW_WINDOW] =
    g_signal_new ("new-window",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWindowClass, new_window),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}



static void
terminal_window_init (TerminalWindow *window)
{
  GtkAccelGroup       *accel_group;
  GtkAction           *action;
  GtkWidget           *vbox;
  gboolean             bval;
  GError              *error = NULL;
  gchar               *role;
  gchar               *file;

  window->preferences = terminal_preferences_get ();

  window->action_group = gtk_action_group_new ("terminal-window");
  gtk_action_group_set_translation_domain (window->action_group,
                                           GETTEXT_PACKAGE);
  gtk_action_group_add_actions (window->action_group,
                                action_entries,
                                G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (window));
  gtk_action_group_add_toggle_actions (window->action_group,
                                       toggle_action_entries,
                                       G_N_ELEMENTS (toggle_action_entries),
                                       GTK_WIDGET (window));

  window->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (window->ui_manager, window->action_group, 0);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "Terminal/Terminal.ui");
  if (G_LIKELY (file != NULL))
    {
      if (gtk_ui_manager_add_ui_from_file (window->ui_manager, file, &error) == 0)
        {
          g_warning ("Unable to load %s: %s", file, error->message);
          g_error_free (error);
        }
      gtk_ui_manager_ensure_update (window->ui_manager);
      g_free (file);
    }
  else
    {
      g_warning ("Unable to locate Terminal/Terminal.ui, the menus won't be available");
    }

  accel_group = gtk_ui_manager_get_accel_group (window->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  window->menubar = gtk_ui_manager_get_widget (window->ui_manager, "/main-menu");
  if (G_LIKELY (window->menubar != NULL))
    {
      gtk_box_pack_start (GTK_BOX (vbox), window->menubar, FALSE, FALSE, 0);
      gtk_widget_show (window->menubar);
    }

  window->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                                   "scrollable", TRUE,
                                   "show-border", FALSE,
                                   "tab-hborder", 0,
                                   "tab-vborder", 0,
                                   NULL);
  g_signal_connect (G_OBJECT (window->notebook), "notify::page",
                    G_CALLBACK (terminal_window_notify_page), window);
  g_signal_connect (G_OBJECT (window->notebook), "remove",
                    G_CALLBACK (terminal_window_widget_removed), window);
  gtk_box_pack_start (GTK_BOX (vbox), window->notebook, TRUE, TRUE, 0);
  gtk_widget_show (window->notebook);

  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (gtk_widget_destroy), window);

  /* setup menubar visibility */
  g_object_get (G_OBJECT (window->preferences), "misc-menubar-default", &bval, NULL);
  action = gtk_action_group_get_action (window->action_group, "show-menubar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), !bval);
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), bval);

  /* setup borders visibility */
  g_object_get (G_OBJECT (window->preferences), "misc-borders-default", &bval, NULL);
  action = gtk_action_group_get_action (window->action_group, "show-borders");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), !bval);
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), bval);

  /* setup fullscreen mode */
  if (!gdk_net_wm_supports (gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
    {
      action = gtk_action_group_get_action (window->action_group, "fullscreen");
      g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
    }

  /* set a unique role on each window (for session management) */
  role = g_strdup_printf ("Terminal-%p-%d-%d", window, getpid (), (gint) time (NULL));
  gtk_window_set_role (GTK_WINDOW (window), role);
  g_free (role);
}



static void
terminal_window_dispose (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  if (window->prefs_idle_id != 0)
    g_source_remove (window->prefs_idle_id);
  if (window->title_idle_id != 0)
    g_source_remove (window->title_idle_id);
  if (window->about_idle_id != 0)
    g_source_remove (window->about_idle_id);

  parent_class->dispose (object);
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  g_object_unref (G_OBJECT (window->preferences));
  g_object_unref (G_OBJECT (window->action_group));
  g_object_unref (G_OBJECT (window->ui_manager));

  parent_class->finalize (object);
}



static TerminalWidget*
terminal_window_get_active (TerminalWindow *window)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (window->notebook);
  gint         page_num;

  page_num = gtk_notebook_get_current_page (notebook);
  if (G_LIKELY (page_num >= 0))
    return TERMINAL_WIDGET (gtk_notebook_get_nth_page (notebook, page_num));
  else
    return NULL;
}



static void
terminal_window_set_size_force_grid (TerminalWindow *window,
                                     TerminalWidget *widget,
                                     gint            force_grid_width,
                                     gint            force_grid_height)
{
  terminal_window_update_geometry (window);
  terminal_widget_force_resize_window (widget, GTK_WINDOW (window),
                                       force_grid_width, force_grid_height);
}



static void
terminal_window_update_geometry (TerminalWindow *window)
{
  TerminalWidget *widget;

  widget = terminal_window_get_active (window);
  if (G_UNLIKELY (widget == NULL))
    return;

  terminal_widget_set_window_geometry_hints (widget, GTK_WINDOW (window));
}



static void
terminal_window_update_actions (TerminalWindow *window)
{
  TerminalWidget *terminal;
  GtkNotebook    *notebook = GTK_NOTEBOOK (window->notebook);
  GtkAction      *action;
  gint            page_num;
  gint            n_pages;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    {
      page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (terminal));
      n_pages = gtk_notebook_get_n_pages (notebook);

      action = gtk_action_group_get_action (window->action_group, "prev-tab");
      g_object_set (G_OBJECT (action),
                    "sensitive", page_num > 0,
                    NULL);

      action = gtk_action_group_get_action (window->action_group, "next-tab");
      g_object_set (G_OBJECT (action),
                    "sensitive", page_num < n_pages - 1,
                    NULL);

      action = gtk_action_group_get_action (window->action_group, "copy");
      g_object_set (G_OBJECT (action),
                    "sensitive", terminal_widget_has_selection (terminal),
                    NULL);
    }
}



static void
terminal_window_notify_page (GtkNotebook    *notebook,
                             GParamSpec     *pspec,
                             TerminalWindow *window)
{
  TerminalWidget *terminal;
  gchar          *title;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    {
      title = terminal_widget_get_title (terminal);
      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);

      terminal_window_update_actions (window);
    }
}



static void
terminal_window_context_menu (TerminalWidget  *widget,
                              GdkEvent        *event,
                              TerminalWindow  *window)
{
  TerminalWidget *terminal;
  GtkWidget      *popup;
  GtkWidget      *menu;
  GtkWidget      *item;
  gint            button = 0;
  gint            time;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (widget == terminal))
    {
      popup = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu");
      if (G_UNLIKELY (popup == NULL))
        return;

      item = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu/input-methods");
      if (G_UNLIKELY (item == NULL))
        return;

      /* append input methods */
      menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (item));
      if (G_LIKELY (menu != NULL))
        gtk_widget_destroy (menu);
      menu = gtk_menu_new ();
      terminal_widget_im_append_menuitems (widget, GTK_MENU_SHELL (menu));
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

      if (event != NULL)
        {
          if (event->type == GDK_BUTTON_PRESS)
            button = event->button.button;
          time = event->button.time;
        }
      else
        {
          time = gtk_get_current_event_time ();
        }

      gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
                      NULL, NULL, button, time);
    }
}



static void
terminal_window_notify_title (TerminalWidget *widget,
                              GParamSpec     *pspec,
                              TerminalWindow *window)
{
  TerminalWidget *terminal;
  gchar          *title;

  terminal = terminal_window_get_active (window);
  if (widget == terminal)
    {
      title = terminal_widget_get_title (terminal);
      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);
    }
}



static void
terminal_window_widget_removed (GtkNotebook     *notebook,
                                TerminalWidget  *widget,
                                TerminalWindow  *window)
{
  TerminalWidget *active;
  gint            npages;

  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  if (G_UNLIKELY (npages == 0))
    {
      gtk_widget_destroy (GTK_WIDGET (window));
    }
  else
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), npages > 1);

      active = terminal_window_get_active (window);
      terminal_window_set_size_force_grid (window, active, -1, -1);

      terminal_window_update_actions (window);
    }
}



static void
terminal_window_action_new_tab (GtkAction       *action,
                                TerminalWindow  *window)
{
  TerminalWidget *active;
  const gchar    *directory;
  GtkWidget      *terminal;

  terminal = terminal_widget_new ();

  active = terminal_window_get_active (window);
  if (G_LIKELY (active != NULL))
    {
      directory = terminal_widget_get_working_directory (active);
      terminal_widget_set_working_directory (TERMINAL_WIDGET (terminal),
                                             directory);
    }

  terminal_window_add (window, TERMINAL_WIDGET (terminal));
  terminal_widget_launch_child (TERMINAL_WIDGET (terminal));
}



static void
terminal_window_action_new_window (GtkAction       *action,
                                   TerminalWindow  *window)
{
  TerminalWidget *active;
  const gchar    *directory;

  active = terminal_window_get_active (window);
  if (G_LIKELY (active != NULL))
    {
      directory = terminal_widget_get_working_directory (active);
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW], 0, directory);
    }
}



static void
terminal_window_action_close_tab (GtkAction       *action,
                                  TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    gtk_widget_destroy (GTK_WIDGET (terminal));
}



static void
terminal_window_action_close_window (GtkAction       *action,
                                     TerminalWindow  *window)
{
  gtk_widget_destroy (GTK_WIDGET (window));
}



static void
terminal_window_action_copy (GtkAction       *action,
                             TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_copy_clipboard (terminal);
}



static void
terminal_window_action_paste (GtkAction       *action,
                              TerminalWindow  *window)
{
  TerminalWidget *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    terminal_widget_paste_clipboard (terminal);
}



static void
terminal_window_action_show_menubar (GtkToggleAction *action,
                                     TerminalWindow  *window)
{
  if (G_UNLIKELY (window->menubar == NULL))
    return;

  if (gtk_toggle_action_get_active (action))
    gtk_widget_show (window->menubar);
  else
    gtk_widget_hide (window->menubar);
}



static void
terminal_window_action_show_borders (GtkToggleAction *action,
                                     TerminalWindow  *window)
{
  gtk_window_set_decorated (GTK_WINDOW (window),
                            gtk_toggle_action_get_active (action));
}



static void
terminal_window_action_fullscreen (GtkToggleAction *action,
                                   TerminalWindow  *window)
{
  if (gtk_toggle_action_get_active (action))
    gtk_window_fullscreen (GTK_WINDOW (window));
  else
    gtk_window_unfullscreen (GTK_WINDOW (window));
}



static void
terminal_window_action_prefs (GtkAction       *action,
                              TerminalWindow  *window)
{
  if (G_LIKELY (window->prefs_idle_id == 0))
    {
      window->prefs_idle_id = g_idle_add_full (G_PRIORITY_LOW, terminal_window_prefs_idle,
                                               window, terminal_window_prefs_idle_destroy);
    }
}



static void
terminal_window_action_prev_tab (GtkAction       *action,
                                 TerminalWindow  *window)
{
  gtk_notebook_prev_page (GTK_NOTEBOOK (window->notebook));
}



static void
terminal_window_action_next_tab (GtkAction       *action,
                                 TerminalWindow  *window)
{
  gtk_notebook_next_page (GTK_NOTEBOOK (window->notebook));
}



static void
terminal_window_action_set_title (GtkAction       *action,
                                  TerminalWindow  *window)
{
  if (G_LIKELY (window->title_idle_id == 0))
    {
      window->title_idle_id = g_idle_add_full (G_PRIORITY_LOW, terminal_window_title_idle,
                                               window, terminal_window_title_idle_destroy);
    }
}



static void
terminal_window_action_reset (GtkAction      *action,
                              TerminalWindow *window)
{
  TerminalWidget *active;

  active = terminal_window_get_active (window);
  terminal_widget_reset (active, FALSE);
}



static void
terminal_window_action_reset_and_clear (GtkAction       *action,
                                        TerminalWindow  *window)
{
  TerminalWidget *active;

  active = terminal_window_get_active (window);
  terminal_widget_reset (active, TRUE);
}



static void
terminal_window_action_report_bug (GtkAction       *action,
                                   TerminalWindow  *window)
{
  GtkWidget *dialog;
  GError    *error = NULL;
  gchar     *command;

  command = g_strconcat (TERMINAL_HELP_BIN, " support", NULL);
  if (!g_spawn_command_line_async (command, &error))
    {
      dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                       GTK_DIALOG_DESTROY_WITH_PARENT
                                       | GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       _("Unable to launch online help: %s"),
                                       error->message);
      g_signal_connect (G_OBJECT (dialog), "response",
                        G_CALLBACK (gtk_widget_destroy), NULL);
      gtk_widget_show (dialog);

      g_error_free (error);
    }
  g_free (command);
}



static void
terminal_window_action_contents (GtkAction       *action,
                                 TerminalWindow  *window)
{
  GtkWidget *dialog;
  GError    *error = NULL;

  if (!g_spawn_command_line_async (TERMINAL_HELP_BIN, &error))
    {
      dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                       GTK_DIALOG_DESTROY_WITH_PARENT
                                       | GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       _("Unable to launch online help: %s"),
                                       error->message);
      g_signal_connect (G_OBJECT (dialog), "response",
                        G_CALLBACK (gtk_widget_destroy), NULL);
      gtk_widget_show (dialog);

      g_error_free (error);
    }
}



static void
terminal_window_action_about (GtkAction       *action,
                              TerminalWindow  *window)
{
  if (G_LIKELY (window->about_idle_id == 0))
    {
      window->about_idle_id = g_idle_add_full (G_PRIORITY_LOW, terminal_window_about_idle,
                                               window, terminal_window_about_idle_destroy);
    }
}



static gboolean
terminal_window_prefs_idle (gpointer user_data)
{
  GtkWidget *dialog;

  dialog = terminal_preferences_dialog_new (GTK_WINDOW (user_data));
  gtk_widget_show (dialog);

  return FALSE;
}



static void
terminal_window_prefs_idle_destroy (gpointer user_data)
{
  TERMINAL_WINDOW (user_data)->prefs_idle_id = 0;
}



static void
title_dialog_response (GtkWidget *dialog,
                       gint       response)
{
  GtkWidget *message;
  GError    *error = NULL;
  gchar     *command;

  if (response == GTK_RESPONSE_HELP)
    {
      command = g_strconcat (TERMINAL_HELP_BIN, " usage set-title", NULL);
      if (!g_spawn_command_line_async (command, &error))
        {
          message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                            GTK_DIALOG_DESTROY_WITH_PARENT
                                            | GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            _("Unable to launch online help: %s"),
                                            error->message);
          g_signal_connect (G_OBJECT (message), "response",
                            G_CALLBACK (gtk_widget_destroy), NULL);
          gtk_widget_show (message);

          g_error_free (error);
        }
      g_free (command);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}



static gboolean
terminal_window_title_idle (gpointer user_data)
{
  ExoPropertyProxy *proxy;
  TerminalWindow   *window = TERMINAL_WINDOW (user_data);
  TerminalWidget   *widget;
  GtkWidget        *dialog;
  GtkWidget        *box;
  GtkWidget        *label;
  GtkWidget        *entry;

  widget = terminal_window_get_active (window);
  if (G_LIKELY (widget != NULL))
    {
      dialog = gtk_dialog_new_with_buttons (_("Set Title"),
                                            GTK_WINDOW (window),
                                            GTK_DIALOG_DESTROY_WITH_PARENT
                                            | GTK_DIALOG_NO_SEPARATOR,
                                            GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                            NULL);

      box = gtk_hbox_new (FALSE, 6);
      gtk_container_set_border_width (GTK_CONTAINER (box), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), box, TRUE, TRUE, 0);
      gtk_widget_show (box);

      label = g_object_new (GTK_TYPE_LABEL,
                            "label", _("<b>Title:</b>"),
                            "use-markup", TRUE,
                            NULL);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
      g_signal_connect_swapped (G_OBJECT (entry), "activate",
                                G_CALLBACK (gtk_widget_destroy), dialog);
      gtk_widget_show (entry);

      proxy = exo_property_proxy_new ();
      exo_property_proxy_add (proxy, G_OBJECT (widget), "custom-title", NULL, NULL, NULL);
      exo_property_proxy_add (proxy, G_OBJECT (entry), "text", NULL, NULL, NULL);
      g_object_unref (G_OBJECT (proxy));

      g_signal_connect (G_OBJECT (dialog), "response",
                        G_CALLBACK (title_dialog_response), NULL);

      gtk_widget_show (dialog);
    }

  return FALSE;
}



static void
terminal_window_title_idle_destroy (gpointer user_data)
{
  TERMINAL_WINDOW (user_data)->title_idle_id = 0;
}



static gboolean
terminal_window_about_idle (gpointer user_data)
{
  TerminalWindow *window = TERMINAL_WINDOW (user_data);
  XfceAboutInfo  *info;
  GtkWidget      *dialog;
  GdkPixbuf      *icon;

  icon = xfce_themed_icon_load ("terminal", 48);

  info = xfce_about_info_new ("Terminal", VERSION, _("X Terminal Emulator"),
                              XFCE_COPYRIGHT_TEXT ("2003-2004", "os-cillation"),
                              XFCE_LICENSE_GPL);
  xfce_about_info_set_homepage (info, "http://www.os-cillation.com/");
  xfce_about_info_add_credit (info, "Benedikt Meurer", "benny@xfce.org", _("Maintainer"));
  xfce_about_info_add_credit (info, "Francois Le Clainche", "fleclainche@wanadoo.fr", _("Icon Designer"));
  xfce_about_info_add_credit (info, "Jasper Huijsmans", "jasper@xfce.org", _("Translator (nl)"));

  dialog = xfce_about_dialog_new (GTK_WINDOW (window), info, icon);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  xfce_about_info_free (info);
  if (G_LIKELY (icon != NULL))
    g_object_unref (G_OBJECT (icon));

  return FALSE;
}



static void
terminal_window_about_idle_destroy (gpointer user_data)
{
  TERMINAL_WINDOW (user_data)->about_idle_id = 0;
}



/**
 * terminal_window_new:
 *
 * Return value :
 **/
GtkWidget*
terminal_window_new (void)
{
  return g_object_new (TERMINAL_TYPE_WINDOW, NULL);
}



/**
 * terminal_window_add:
 * @window  : A #TerminalWindow.
 * @widget  : A #TerminalWidget.
 **/
void
terminal_window_add (TerminalWindow *window,
                     TerminalWidget *widget)
{
  ExoPropertyProxy *proxy;
  TerminalWidget   *active;
  GtkWidget        *header;
  gint              npages;
  gint              page;
  gint              grid_width = -1;
  gint              grid_height = -1;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  active = terminal_window_get_active (window);
  if (G_LIKELY (active != NULL))
    {
      terminal_widget_get_size (active, &grid_width, &grid_height);
      terminal_widget_set_size (widget, grid_width, grid_height);
    }

  header = terminal_tab_header_new ();
  g_signal_connect_swapped (G_OBJECT (header), "close",
                            G_CALLBACK (gtk_widget_destroy), widget);
  gtk_widget_show (header);

  proxy = exo_property_proxy_new ();
  exo_property_proxy_add (proxy, G_OBJECT (widget), "title", NULL, NULL, NULL);
  exo_property_proxy_add (proxy, G_OBJECT (header), "title", NULL, NULL, NULL);
  g_object_unref (G_OBJECT (proxy));

  page = gtk_notebook_append_page (GTK_NOTEBOOK (window->notebook),
                                   GTK_WIDGET (widget), header);
  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (window->notebook),
                                      GTK_WIDGET (widget),
                                      TRUE, TRUE, GTK_PACK_START);

  /* need to show this first, else we cannot switch to it */
  gtk_widget_show (GTK_WIDGET (widget));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), page);

  g_signal_connect (G_OBJECT (widget), "context-menu",
                    G_CALLBACK (terminal_window_context_menu), window);
  g_signal_connect (G_OBJECT (widget), "notify::title",
                    G_CALLBACK (terminal_window_notify_title), window);
  g_signal_connect_swapped (G_OBJECT (widget), "selection-changed",
                            G_CALLBACK (terminal_window_update_actions), window);

  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), npages > 1);

  terminal_window_set_size_force_grid (window, widget, grid_width, grid_height);

  terminal_window_update_actions (window);
}



/**
 * terminal_window_remove:
 * @window  :
 * @widget  :
 **/
void
terminal_window_remove (TerminalWindow *window,
                        TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_destroy (GTK_WIDGET (widget));
}
