/* $Id$ */
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
#include <gdk/gdkx.h>

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
#include <libsn/sn-launchee.h>
#endif

#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-options.h>
#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-tab-header.h>
#include <terminal/terminal-toolbars-view.h>
#include <terminal/terminal-window.h>



enum
{
  PROP_0,
  PROP_SHOW_MENUBAR,
  PROP_SHOW_BORDERS,
  PROP_SHOW_TOOLBARS,
};

enum
{
  NEW_WINDOW,
  NEW_WINDOW_WITH_SCREEN,
  LAST_SIGNAL,
};



static void            terminal_window_dispose                  (GObject                *object);
static void            terminal_window_finalize                 (GObject                *object);
static void            terminal_window_show                     (GtkWidget              *widget);
static TerminalScreen *terminal_window_get_active               (TerminalWindow         *window);
static gboolean        terminal_window_confirm_close            (TerminalWindow         *window);
static void            terminal_window_queue_reset_size         (TerminalWindow         *window);
static gboolean        terminal_window_reset_size               (TerminalWindow         *window);
static void            terminal_window_reset_size_destroy       (TerminalWindow         *window);
static void            terminal_window_set_size_force_grid      (TerminalWindow         *window,
                                                                 TerminalScreen         *screen,
                                                                 gint                    force_grid_width,
                                                                 gint                    force_grid_height);
static void            terminal_window_update_geometry          (TerminalWindow         *window,
                                                                 TerminalScreen         *screen);
static void            terminal_window_update_actions           (TerminalWindow         *window);
static void            terminal_window_update_mnemonics         (TerminalWindow         *window);
static void            terminal_window_rebuild_gomenu           (TerminalWindow         *window);
static gboolean        terminal_window_delete_event             (TerminalWindow         *window);
static void            terminal_window_page_notified            (GtkNotebook            *notebook,
                                                                 GParamSpec             *pspec,
                                                                 TerminalWindow         *window);
static void            terminal_window_page_switched            (GtkNotebook            *notebook,
                                                                 GtkNotebookPage        *page,
                                                                 guint                   page_num,
                                                                 TerminalWindow         *window);
static void            terminal_window_context_menu             (TerminalScreen         *screen,
                                                                 GdkEvent               *event,
                                                                 TerminalWindow         *window);
static void            terminal_window_detach_screen            (TerminalWindow         *window,
                                                                 TerminalTabHeader      *header);
static void            terminal_window_notify_title             (TerminalScreen         *screen,
                                                                 GParamSpec             *pspec,
                                                                 TerminalWindow         *window);
static void            terminal_window_screen_removed           (GtkNotebook            *notebook,
                                                                 TerminalScreen         *screen,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_new_tab           (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_new_window        (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_close_tab         (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_close_window      (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_copy              (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_paste             (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_edit_toolbars     (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_prefs             (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_show_menubar      (GtkToggleAction        *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_show_toolbars     (GtkToggleAction        *action,
                                                                 TerminalWindow         *window);
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

  gchar               *startup_id;

  TerminalPreferences *preferences;
  GtkWidget           *preferences_dialog;

  GtkActionGroup      *action_group;
  GtkUIManager        *ui_manager;
  
  GtkWidget           *menubar;
  GtkWidget           *toolbars;
  GtkWidget           *notebook;

  GtkWidget           *gomenu;

  guint                reset_size_idle_id;
  guint                prefs_idle_id;
  guint                title_idle_id;
  guint                about_idle_id;
};



static GObjectClass *parent_class;
static guint         window_signals[LAST_SIGNAL];

static GtkActionEntry action_entries[] =
{
  { "file-menu", NULL, N_ ("_File"), NULL, },
  { "new-tab", "terminal-newtab", N_ ("Open _Tab"), NULL, N_ ("Open a new terminal tab"), G_CALLBACK (terminal_window_action_new_tab), }, 
  { "new-window", "terminal-newwindow", N_ ("Open _Terminal"), "<control><shift>N", N_ ("Open a new terminal window"), G_CALLBACK (terminal_window_action_new_window), }, 
  { "close-tab", "terminal-closetab", N_ ("C_lose Tab"), NULL, N_ ("Close the current terminal tab"), G_CALLBACK (terminal_window_action_close_tab), },
  { "close-window", "terminal-closewindow", N_ ("_Close Window"), NULL, N_ ("Close the terminal window"), G_CALLBACK (terminal_window_action_close_window), },
  { "edit-menu", NULL, N_ ("_Edit"), NULL, },
  { "copy", GTK_STOCK_COPY, N_ ("_Copy"), NULL, N_ ("Copy to clipboard"), G_CALLBACK (terminal_window_action_copy), },
  { "paste", GTK_STOCK_PASTE, N_ ("_Paste"), NULL, N_ ("Paste from clipboard"), G_CALLBACK (terminal_window_action_paste), },
  { "edit-toolbars", NULL, N_ ("_Toolbars..."), NULL, N_ ("Customize the toolbars"), G_CALLBACK (terminal_window_action_edit_toolbars), },
  { "preferences", GTK_STOCK_PREFERENCES, N_ ("_Preferences..."), NULL, N_ ("Open the Terminal preferences dialog"), G_CALLBACK (terminal_window_action_prefs), },
  { "view-menu", NULL, N_ ("_View"), NULL, },
  { "terminal-menu", NULL, N_ ("_Terminal"), NULL, },
  { "set-title", NULL, N_ ("_Set Title..."), NULL, N_ ("Set a custom title for the current tab"), G_CALLBACK (terminal_window_action_set_title), },
  { "reset", GTK_STOCK_REFRESH, N_ ("_Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset), },
  { "reset-and-clear", GTK_STOCK_CLEAR, N_ ("Reset and C_lear"), NULL, NULL, G_CALLBACK (terminal_window_action_reset_and_clear), },
  { "go-menu", NULL, N_ ("_Go"), NULL, },
  { "prev-tab", GTK_STOCK_GO_BACK, N_ ("_Previous Tab"), NULL, N_ ("Switch to previous tab"), G_CALLBACK (terminal_window_action_prev_tab), },
  { "next-tab", GTK_STOCK_GO_FORWARD, N_ ("_Next Tab"), NULL, N_ ("Switch to next tab"), G_CALLBACK (terminal_window_action_next_tab), },
  { "help-menu", NULL, N_ ("_Help"), NULL, },
  { "contents", GTK_STOCK_HELP, N_ ("_Contents"), NULL, N_ ("Display help contents"), G_CALLBACK (terminal_window_action_contents), },
  { "report-bug", "terminal-reportbug", N_ ("_Report a bug"), NULL, N_ ("Report a bug in Terminal"), G_CALLBACK (terminal_window_action_report_bug), },
#if GTK_CHECK_VERSION(2,6,0)
  { "about", GTK_STOCK_ABOUT, N_ ("_About"), NULL, N_ ("Display information about Terminal"), G_CALLBACK (terminal_window_action_about), },
#else
  { "about", GTK_STOCK_DIALOG_INFO, N_ ("_About"), NULL, N_ ("Display information about Terminal"), G_CALLBACK (terminal_window_action_about), },
#endif
  { "input-methods", NULL, N_ ("_Input Methods"), NULL, NULL, NULL, },
};

static GtkToggleActionEntry toggle_action_entries[] =
{
  { "show-menubar", "terminal-showmenu", N_ ("Show _Menubar"), NULL, N_ ("Show/hide the menubar"), G_CALLBACK (terminal_window_action_show_menubar), TRUE, },
  { "show-toolbars", NULL, N_ ("Show _Toolbars"), NULL, N_ ("Show/hide the toolbars"), G_CALLBACK (terminal_window_action_show_toolbars), FALSE, },
  { "show-borders", "terminal-showborders", N_ ("Show Window _Borders"), NULL, N_ ("Show/hide the window decorations"), G_CALLBACK (terminal_window_action_show_borders), TRUE, },
  { "fullscreen", "terminal-fullscreen", N_ ("_Fullscreen"), NULL, N_ ("Toggle fullscreen mode"), G_CALLBACK (terminal_window_action_fullscreen), FALSE, },
};



G_DEFINE_TYPE (TerminalWindow, terminal_window, GTK_TYPE_WINDOW);



static void
terminal_window_class_init (TerminalWindowClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;
  
  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_window_dispose;
  gobject_class->finalize = terminal_window_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->show = terminal_window_show;

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

  /**
   * TerminalWindow::new-window-with-screen:
   **/
  window_signals[NEW_WINDOW_WITH_SCREEN] =
    g_signal_new ("new-window-with-screen",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWindowClass, new_window_with_screen),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_OBJECT);
}



static void
terminal_window_init (TerminalWindow *window)
{
  GtkAccelGroup  *accel_group;
  GtkAction      *action;
  GtkWidget      *item;
  GtkWidget      *vbox;
  gboolean        bval;
  GError         *error = NULL;
  gchar          *role;
  gchar          *file;

  window->preferences = terminal_preferences_get ();

  /* The Terminal size needs correction when the font name or the scrollbar
   * visibility is changed.
   */
  g_signal_connect_swapped (G_OBJECT (window->preferences), "notify::font-name",
                            G_CALLBACK (terminal_window_queue_reset_size), window);
  g_signal_connect_swapped (G_OBJECT (window->preferences), "notify::scrolling-bar",
                            G_CALLBACK (terminal_window_queue_reset_size), window);
  g_signal_connect_swapped (G_OBJECT (window->preferences), "notify::shortcuts-no-mnemonics",
                            G_CALLBACK (terminal_window_update_mnemonics), window);

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

  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_lookup (XFCE_RESOURCE_DATA, "Terminal/Terminal.ui");
  xfce_resource_pop_path (XFCE_RESOURCE_DATA);

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

  /* determine if we have a "Go" menu */
  item = gtk_ui_manager_get_widget (window->ui_manager, "/main-menu/go-menu");
  if (GTK_IS_MENU_ITEM (item))
    {
      window->gomenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (item));
      if (G_LIKELY (window->gomenu != NULL))
        {
          /* required for gtk_menu_item_set_accel_path() later */
          gtk_menu_set_accel_group (GTK_MENU (window->gomenu), accel_group);

          /* add an explicit separator */
          item = gtk_separator_menu_item_new ();
          gtk_menu_shell_append (GTK_MENU_SHELL (window->gomenu), item);
          gtk_widget_show (item);
        }
    }

  /* setup mnemonics */
  g_object_get (G_OBJECT (window->preferences), "shortcuts-no-mnemonics", &bval, NULL);
  if (G_UNLIKELY (bval))
    terminal_window_update_mnemonics (window);

  /* setup fullscreen mode */
  if (!gdk_net_wm_supports (gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
    {
      action = gtk_action_group_get_action (window->action_group, "fullscreen");
      g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
    }

  window->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                                   "homogeneous", TRUE,
                                   "scrollable", TRUE,
                                   "show-border", FALSE,
                                   "tab-hborder", 0,
                                   "tab-vborder", 0,
                                   NULL);
  g_signal_connect_after (G_OBJECT (window->notebook), "switch-page",
                          G_CALLBACK (terminal_window_page_switched), window);
  g_signal_connect (G_OBJECT (window->notebook), "notify::page",
                    G_CALLBACK (terminal_window_page_notified), window);
  g_signal_connect (G_OBJECT (window->notebook), "remove",
                    G_CALLBACK (terminal_window_screen_removed), window);
  gtk_box_pack_start (GTK_BOX (vbox), window->notebook, TRUE, TRUE, 0);
  gtk_widget_show (window->notebook);

  g_object_connect (G_OBJECT (window),
                    "signal::delete-event", G_CALLBACK (terminal_window_delete_event), NULL,
                    "signal-after::style-set", G_CALLBACK (terminal_window_queue_reset_size), NULL,
                    NULL);

  /* set a unique role on each window (for session management) */
  role = g_strdup_printf ("Terminal-%p-%d-%d", window, getpid (), (gint) time (NULL));
  gtk_window_set_role (GTK_WINDOW (window), role);
  g_free (role);
}



static void
terminal_window_dispose (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  if (window->reset_size_idle_id != 0)
    g_source_remove (window->reset_size_idle_id);
  if (window->prefs_idle_id != 0)
    g_source_remove (window->prefs_idle_id);
  if (window->title_idle_id != 0)
    g_source_remove (window->title_idle_id);
  if (window->about_idle_id != 0)
    g_source_remove (window->about_idle_id);

  g_signal_handlers_disconnect_matched (G_OBJECT (window->preferences),
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, window);

  parent_class->dispose (object);
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  g_object_unref (G_OBJECT (window->preferences));
  g_object_unref (G_OBJECT (window->action_group));
  g_object_unref (G_OBJECT (window->ui_manager));

  g_free (window->startup_id);

  parent_class->finalize (object);
}



static void
terminal_window_show (GtkWidget *widget)
{
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  SnLauncheeContext *sn_context = NULL;
  TerminalWindow    *window = TERMINAL_WINDOW (widget);
  GdkScreen         *screen;
  SnDisplay         *sn_display = NULL;

  if (!GTK_WIDGET_REALIZED (widget))
    gtk_widget_realize (widget);

  if (window->startup_id != NULL)
    {
      screen = gtk_window_get_screen (GTK_WINDOW (window));

      sn_display = sn_display_new (GDK_SCREEN_XDISPLAY (screen),
                                   (SnDisplayErrorTrapPush) gdk_error_trap_push,
                                   (SnDisplayErrorTrapPop) gdk_error_trap_pop);

      sn_context = sn_launchee_context_new (sn_display,
                                            gdk_screen_get_number (screen),
                                            window->startup_id);
      sn_launchee_context_setup_window (sn_context, GDK_WINDOW_XWINDOW (widget->window));
    }
#endif

  GTK_WIDGET_CLASS (parent_class)->show (widget);

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  if (G_LIKELY (sn_context != NULL))
    {
      sn_launchee_context_complete (sn_context);
      sn_launchee_context_unref (sn_context);
      sn_display_unref (sn_display);
    }
#endif
}



static TerminalScreen*
terminal_window_get_active (TerminalWindow *window)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (window->notebook);
  gint         page_num;

  page_num = gtk_notebook_get_current_page (notebook);
  if (G_LIKELY (page_num >= 0))
    return TERMINAL_SCREEN (gtk_notebook_get_nth_page (notebook, page_num));
  else
    return NULL;
}



static gboolean
terminal_window_confirm_close (TerminalWindow *window)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *checkbox;
  gboolean   confirm_close;
  gchar     *message;
  gchar     *markup;
  gint       response;
  gint       n_tabs;

  n_tabs = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  if (G_UNLIKELY (n_tabs < 2))
    return TRUE;

  g_object_get (G_OBJECT (window->preferences), "misc-confirm-close", &confirm_close, NULL);
  if (!confirm_close)
    return TRUE;

  dialog = gtk_dialog_new_with_buttons (_("Warning"), GTK_WINDOW (window),
                                        GTK_DIALOG_DESTROY_WITH_PARENT
                                        | GTK_DIALOG_NO_SEPARATOR
                                        | GTK_DIALOG_MODAL,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        _("Close all tabs"), GTK_RESPONSE_YES,
                                        NULL);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  message = g_strdup_printf (_("This window has %d tabs open. Closing\nthis "
                               "window will also close all its tabs."), n_tabs);
  markup = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
                            _("Close all tabs?"), message);
  g_free (message);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", markup,
                        "use-markup", TRUE,
                        "xalign", 0.0,
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  g_free (markup);

  checkbox = gtk_check_button_new_with_mnemonic (_("Do _not ask me again"));
  gtk_box_pack_start (GTK_BOX (vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show (checkbox);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_YES)
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox)))
        {
          g_object_set (G_OBJECT (window->preferences),
                        "misc-confirm-close", FALSE,
                        NULL);
        }
    }

  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_YES);
}



static void
terminal_window_queue_reset_size (TerminalWindow *window)
{
  if (GTK_WIDGET_REALIZED (window) && window->reset_size_idle_id == 0)
    {
      /* Gtk+ uses a priority of G_PRIORITY_HIGH_IDLE + 10 for resizing operations, so we
       * use a slightly higher priority for the reset size operation.
       */
      window->reset_size_idle_id = g_idle_add_full (G_PRIORITY_HIGH_IDLE + 5,
                                                    (GSourceFunc) terminal_window_reset_size, window,
                                                    (GDestroyNotify) terminal_window_reset_size_destroy);
    }
}



static gboolean
terminal_window_reset_size (TerminalWindow *window)
{
  TerminalScreen *active;
  gint            grid_width;
  gint            grid_height;
  
  /* The trick is rather simple here. This is called before any Gtk+ resizing operation takes
   * place, so the columns/rows on the active terminal screen are still set to their old values.
   * We simply query these values and force them to be set with the new style.
   */
  active = terminal_window_get_active (window);
  if (G_LIKELY (active != NULL))
    {
      terminal_screen_get_size (active, &grid_width, &grid_height);
      terminal_window_set_size_force_grid (window, active, grid_width, grid_height);
    }

  return FALSE;
}



static void
terminal_window_reset_size_destroy (TerminalWindow *window)
{
  window->reset_size_idle_id = 0;
}



static void
terminal_window_set_size_force_grid (TerminalWindow *window,
                                     TerminalScreen *screen,
                                     gint            force_grid_width,
                                     gint            force_grid_height)
{
  /* Required to get the char height/width right */
  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (screen)))
    gtk_widget_realize (GTK_WIDGET (screen));

  terminal_window_update_geometry (window, screen);
  terminal_screen_force_resize_window (screen, GTK_WINDOW (window),
                                       force_grid_width, force_grid_height);
}



static void
terminal_window_update_geometry (TerminalWindow *window,
                                 TerminalScreen *screen)
{
  terminal_screen_set_window_geometry_hints (screen, GTK_WINDOW (window));
}



static void
terminal_window_update_actions (TerminalWindow *window)
{
  TerminalScreen *terminal;
  GtkNotebook    *notebook = GTK_NOTEBOOK (window->notebook);
  GtkAction      *action;
  GtkWidget      *tabitem;
  gboolean        cycle_tabs;
  gint            page_num;
  gint            n_pages;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    {
      page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (terminal));
      n_pages = gtk_notebook_get_n_pages (notebook);

      g_object_get (G_OBJECT (window->preferences),
                    "misc-cycle-tabs", &cycle_tabs,
                    NULL);

      action = gtk_action_group_get_action (window->action_group, "close-tab");
      g_object_set (G_OBJECT (action),
                    "sensitive", n_pages > 1,
                    NULL);

      action = gtk_action_group_get_action (window->action_group, "prev-tab");
      g_object_set (G_OBJECT (action),
                    "sensitive", (cycle_tabs && n_pages > 0) || (page_num > 0),
                    NULL);

      action = gtk_action_group_get_action (window->action_group, "next-tab");
      g_object_set (G_OBJECT (action),
                    "sensitive", (cycle_tabs && n_pages > 0 ) || (page_num < n_pages - 1),
                    NULL);

      action = gtk_action_group_get_action (window->action_group, "copy");
      g_object_set (G_OBJECT (action),
                    "sensitive", terminal_screen_has_selection (terminal),
                    NULL);

      /* update the "Go" menu */
      tabitem = g_object_get_data (G_OBJECT (terminal), "terminal-window-go-menu-item");
      if (G_LIKELY (tabitem != NULL))
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (tabitem), TRUE);
    }
}



static void
terminal_window_update_mnemonics (TerminalWindow *window)
{
  gboolean disable;
  GSList  *wp;
  GList   *actions;
  GList   *ap;
  gchar   *label;
  gchar   *tmp;

  g_object_get (G_OBJECT (window->preferences),
                "shortcuts-no-mnemonics", &disable,
                NULL);

  actions = gtk_action_group_list_actions (window->action_group);
  for (ap = actions; ap != NULL; ap = ap->next)
    for (wp = gtk_action_get_proxies (ap->data); wp != NULL; wp = wp->next)
      if (G_TYPE_CHECK_INSTANCE_TYPE (wp->data, GTK_TYPE_MENU_ITEM))
        {
          g_object_get (G_OBJECT (ap->data), "label", &label, NULL);
          if (disable)
            {
              tmp = exo_str_elide_underscores (label);
              g_free (label);
              label = tmp;
            }

          g_object_set (G_OBJECT (GTK_BIN (wp->data)->child),
                        "label", label,
                        NULL);

          g_free (label);
        }
  g_list_free (actions);
}



static void
item_destroy (gpointer item)
{
  gtk_widget_destroy (GTK_WIDGET (item));
  g_object_unref (G_OBJECT (item));
}



static void
item_toggled (GtkWidget   *item,
              GtkNotebook *notebook)
{
  GtkWidget *screen;
  gint       page;

  if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item)))
    {
      screen = g_object_get_data (G_OBJECT (item), "terminal-window-screen");
      if (G_LIKELY (screen != NULL))
        {
          page = gtk_notebook_page_num (notebook, screen);
          gtk_notebook_set_current_page (notebook, page);
        }
    }
}



static void
terminal_window_rebuild_gomenu (TerminalWindow *window)
{
  GtkWidget *terminal;
  GtkWidget *label;
  GtkWidget *item;
  GSList    *group = NULL;
  GList     *closures;
  gchar      name[32];
  gchar     *path;
  gint       npages;
  gint       n;

  if (G_UNLIKELY (window->gomenu == NULL))
    return;

  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  for (n = 0; n < npages; ++n)
    {
      terminal = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), n);

      item = gtk_radio_menu_item_new (group);
      gtk_menu_shell_append (GTK_MENU_SHELL (window->gomenu), item);
      gtk_widget_show (item);

      label = g_object_new (GTK_TYPE_ACCEL_LABEL, "xalign", 0.0, NULL);
      exo_binding_new (G_OBJECT (terminal), "title", G_OBJECT (label), "label");
      gtk_container_add (GTK_CONTAINER (item), label);
      gtk_widget_show (label);

      /* only connect an accelerator if we have a preference for this item */
      g_snprintf (name, 32, "accel-switch-to-tab%d", n + 1);
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (window->preferences), name) != NULL)
        {
          /* connect the menu item to an accelerator */
          path = g_strconcat ("<Actions>/terminal-window/", name + 6, NULL);
          gtk_menu_item_set_accel_path (GTK_MENU_ITEM (item), path);
          g_free (path);

          /* display accelerator in the item label */
          closures = gtk_widget_list_accel_closures (item);
          if (G_LIKELY (closures != NULL))
            {
              g_object_set (G_OBJECT (label),
                            "accel-closure", closures->data,
                            NULL);
              g_list_free (closures);
            }
        }

      if (gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook)) == n)
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);

      /* keep an extra ref terminal -> item, so we don't in trouble with gtk_widget_destroy */
      g_object_set_data_full (G_OBJECT (terminal), "terminal-window-go-menu-item",
                              G_OBJECT (item), (GDestroyNotify) item_destroy);
      g_object_ref (G_OBJECT (item));

      /* do the item -> terminal connects */
      g_object_set_data (G_OBJECT (item), "terminal-window-screen", terminal);
      g_signal_connect (G_OBJECT (item), "toggled", G_CALLBACK (item_toggled), window->notebook);

      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    }
}



static gboolean
terminal_window_delete_event (TerminalWindow *window)
{
  return !terminal_window_confirm_close (window);
}



static void
terminal_window_page_notified (GtkNotebook    *notebook,
                               GParamSpec     *pspec,
                               TerminalWindow *window)
{
  TerminalScreen *terminal;
  gchar          *title;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    {
      title = terminal_screen_get_title (terminal);
      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);

      terminal_window_update_actions (window);
    }
}



static void
terminal_window_page_switched (GtkNotebook     *notebook,
                               GtkNotebookPage *page,
                               guint            page_num,
                               TerminalWindow  *window)
{
  TerminalScreen *terminal;
  gint            grid_width;
  gint            grid_height;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    {
      /* FIXME: This isn't really needed anymore, instead we
       * could grab the grid size of the previously selected
       * page and apply that to the newly selected page.
       */

      /* This is a bit tricky, but seems to be necessary to get the
       * sizing correct: First we query the current terminal size
       * (rows x columns), then we force a size update on the terminal
       * window (which may reset the terminal size). Afterwards we
       * reset the terminal screen size to the original values and
       * resize the terminal window accordingly.
       */
      terminal_screen_get_size (terminal, &grid_width, &grid_height);
      terminal_screen_set_size (terminal, grid_width, grid_height);
      terminal_window_set_size_force_grid (window, terminal, grid_width, grid_height);
    }
}



static void
terminal_window_context_menu (TerminalScreen  *screen,
                              GdkEvent        *event,
                              TerminalWindow  *window)
{
  TerminalScreen *terminal;
  GtkWidget      *popup;
  GtkWidget      *menu;
  GtkWidget      *item;
  gint            button = 0;
  gint            time;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (screen == terminal))
    {
      popup = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu");
      if (G_UNLIKELY (popup == NULL))
        return;
      gtk_menu_set_screen (GTK_MENU (popup), gtk_window_get_screen (GTK_WINDOW (window)));

      item = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu/input-methods");
      if (G_UNLIKELY (item == NULL))
        return;

      /* append input methods */
      menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (item));
      if (G_LIKELY (menu != NULL))
        gtk_widget_destroy (menu);
      menu = gtk_menu_new ();
      terminal_screen_im_append_menuitems (screen, GTK_MENU_SHELL (menu));
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
terminal_window_detach_screen (TerminalWindow     *window,
                               TerminalTabHeader  *header)
{
  GtkWidget *screen;

  screen = g_object_get_data (G_OBJECT (header), "terminal-window-screen");
  if (G_LIKELY (screen != NULL))
    {
      g_object_ref (G_OBJECT (screen));
      gtk_container_remove (GTK_CONTAINER (window->notebook), screen);
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW_WITH_SCREEN], 0, screen);
      g_object_unref (G_OBJECT (screen));
    }
}



static void
terminal_window_notify_title (TerminalScreen *screen,
                              GParamSpec     *pspec,
                              TerminalWindow *window)
{
  TerminalScreen *terminal;
  gchar          *title;

  /* update window title */
  terminal = terminal_window_get_active (window);
  if (screen == terminal)
    {
      title = terminal_screen_get_title (screen);
      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);
    }
}



static void
terminal_window_screen_removed (GtkNotebook     *notebook,
                                TerminalScreen  *screen,
                                TerminalWindow  *window)
{
  TerminalScreen *active;
  gint            npages;
  gint            grid_width;
  gint            grid_height;

  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  if (G_UNLIKELY (npages == 0))
    {
      gtk_widget_destroy (GTK_WIDGET (window));
    }
  else
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), npages > 1);

      active = terminal_window_get_active (window);
      terminal_screen_get_size (screen, &grid_width, &grid_height);
      terminal_window_set_size_force_grid (window, active, grid_width, grid_height);

      /* regenerate the "Go" menu */
      terminal_window_rebuild_gomenu (window);

      /* update all screen sensitive actions (Copy, Prev Tab, ...) */
      terminal_window_update_actions (window);
    }
}



static void
terminal_window_action_new_tab (GtkAction       *action,
                                TerminalWindow  *window)
{
  TerminalScreen *active;
  const gchar    *directory;
  GtkWidget      *terminal;

  terminal = terminal_screen_new ();

  active = terminal_window_get_active (window);
  if (G_LIKELY (active != NULL))
    {
      directory = terminal_screen_get_working_directory (active);
      terminal_screen_set_working_directory (TERMINAL_SCREEN (terminal),
                                             directory);
    }

  terminal_window_add (window, TERMINAL_SCREEN (terminal));
  terminal_screen_launch_child (TERMINAL_SCREEN (terminal));
}



static void
terminal_window_action_new_window (GtkAction       *action,
                                   TerminalWindow  *window)
{
  TerminalScreen *active;
  const gchar    *directory;

  active = terminal_window_get_active (window);
  if (G_LIKELY (active != NULL))
    {
      directory = terminal_screen_get_working_directory (active);
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW], 0, directory);
    }
}



static void
terminal_window_action_close_tab (GtkAction       *action,
                                  TerminalWindow  *window)
{
  TerminalScreen *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    gtk_widget_destroy (GTK_WIDGET (terminal));
}



static void
terminal_window_action_close_window (GtkAction       *action,
                                     TerminalWindow  *window)
{
  if (terminal_window_confirm_close (window))
    gtk_widget_destroy (GTK_WIDGET (window));
}



static void
terminal_window_action_copy (GtkAction       *action,
                             TerminalWindow  *window)
{
  TerminalScreen *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    terminal_screen_copy_clipboard (terminal);
}



static void
terminal_window_action_paste (GtkAction       *action,
                              TerminalWindow  *window)
{
  TerminalScreen *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    terminal_screen_paste_clipboard (terminal);
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

  terminal_window_queue_reset_size (window);
}



static void
terminal_window_action_show_toolbars (GtkToggleAction *action,
                                      TerminalWindow  *window)
{
  GtkAction *action_edit;
  GtkWidget *vbox;

  action_edit = gtk_action_group_get_action (window->action_group,
                                             "edit-toolbars");

  if (gtk_toggle_action_get_active (action))
    {
      if (window->toolbars == NULL)
        {
          vbox = gtk_bin_get_child (GTK_BIN (window));

          window->toolbars = terminal_toolbars_view_new (window->ui_manager);
          gtk_box_pack_start (GTK_BOX (vbox), window->toolbars, FALSE, FALSE, 0);
          gtk_box_reorder_child (GTK_BOX (vbox), window->toolbars, 1);
          gtk_widget_show (window->toolbars);

          g_object_add_weak_pointer (G_OBJECT (window->toolbars),
                                     (gpointer) &window->toolbars);
        }

      g_object_set (G_OBJECT (action_edit),
                    "sensitive", TRUE,
                    NULL);
    }
  else
    {
      if (window->toolbars != NULL)
        gtk_widget_destroy (window->toolbars);

      g_object_set (G_OBJECT (action_edit),
                    "sensitive", FALSE,
                    NULL);
    }

  terminal_window_queue_reset_size (window);
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
terminal_window_action_edit_toolbars (GtkAction       *action,
                                      TerminalWindow  *window)
{
  if (G_LIKELY (window->toolbars != NULL))
    terminal_toolbars_view_edit (TERMINAL_TOOLBARS_VIEW (window->toolbars));
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
  gint page_num;
  gint n_pages;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook));
  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook),
                                 (page_num + 1) % n_pages);
}



static void
terminal_window_action_next_tab (GtkAction       *action,
                                 TerminalWindow  *window)
{
  gint page_num;
  gint n_pages;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook));
  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook),
                                 (page_num - 1) % n_pages);
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
  TerminalScreen *active;

  active = terminal_window_get_active (window);
  terminal_screen_reset (active, FALSE);
}



static void
terminal_window_action_reset_and_clear (GtkAction       *action,
                                        TerminalWindow  *window)
{
  TerminalScreen *active;

  active = terminal_window_get_active (window);
  terminal_screen_reset (active, TRUE);
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
  TerminalWindow *window = TERMINAL_WINDOW (user_data);

  if (G_UNLIKELY (window->preferences_dialog != NULL)) 
    {
      gtk_window_present (GTK_WINDOW (window->preferences_dialog));
    }
  else
    {
      window->preferences_dialog = terminal_preferences_dialog_new (GTK_WINDOW (window));
      g_object_add_weak_pointer (G_OBJECT (window->preferences_dialog),
                                 (gpointer) &window->preferences_dialog);
      gtk_widget_show (window->preferences_dialog);
    }

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
  TerminalWindow   *window = TERMINAL_WINDOW (user_data);
  TerminalScreen   *screen;
  AtkRelationSet   *relations;
  AtkRelation      *relation;
  AtkObject        *object;
  GtkWidget        *dialog;
  GtkWidget        *box;
  GtkWidget        *label;
  GtkWidget        *entry;

  screen = terminal_window_get_active (window);
  if (G_LIKELY (screen != NULL))
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

      /* set Atk description and label relation for the entry */
      object = gtk_widget_get_accessible (entry);
      atk_object_set_description (object, _("Enter the title for the current terminal tab"));
      relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
      relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
      atk_relation_set_add (relations, relation);
      g_object_unref (G_OBJECT (relation));

      exo_mutual_binding_new (G_OBJECT (screen), "custom-title", G_OBJECT (entry), "text");

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
  gchar          *s;
  guint           n;

  static const struct { gchar *name, *email, *language; } translators[] = {
    { "Dwayne Bailey", "dwayne@translate.org.za", "en_GB" },
    { "Jaime Buffery", "nestu@lunar-linux.org", "es" },
    { "Yuval Tanny", "tanai@int.gov.il", "he" },
    { "Jasper Huijsmans", "jasper@xfce.org", "nl" },
    { "Anthony Ivanoff", "a-i@bk.ru", "ru" },
    { "Hydonsingore Sie", "hydonsingore@mail.educities.edu.tw", "zh_TW" },
    { NULL, },
  };

  /* Try to load the Terminal icon using the icon-spec
   * lookup. Fallback to the well-known location.
   */
  icon = xfce_themed_icon_load ("Terminal", 48);
  if (G_UNLIKELY (icon == NULL))
    icon = gdk_pixbuf_new_from_file (DATADIR "/icons/hicolor/48x48/apps/Terminal.png", NULL);

  info = xfce_about_info_new ("Terminal", VERSION, _("X Terminal Emulator"),
                              XFCE_COPYRIGHT_TEXT ("2003-2005", "os-cillation"),
                              XFCE_LICENSE_GPL);
  xfce_about_info_set_homepage (info, "http://www.os-cillation.com/");
  xfce_about_info_add_credit (info, "Benedikt Meurer", "benny@xfce.org", _("Maintainer"));
  xfce_about_info_add_credit (info, "Francois Le Clainche", "fleclainche@wanadoo.fr", _("Icon Designer"));

  for (n = 0; translators[n].name != NULL; ++n)
    {
      s = g_strdup_printf (_("Translator (%s)"), translators[n].language);
      xfce_about_info_add_credit (info, translators[n].name, translators[n].email, s);
      g_free (s);
    }

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
 * @fullscreen: Whether to set the window to fullscreen.
 * @menubar   : Visibility setting for the menubar.
 * @borders   : Visibility setting for the window borders.
 * @toolbars  : Visibility setting for the toolbars.
 *
 * Return value:
 **/
GtkWidget*
terminal_window_new (gboolean           fullscreen,
                     TerminalVisibility menubar,
                     TerminalVisibility borders,
                     TerminalVisibility toolbars)
{
  TerminalWindow *window;
  GtkAction      *action;
  gboolean        setting;
  
  window = g_object_new (TERMINAL_TYPE_WINDOW, NULL);

  /* setup full screen */
  action = gtk_action_group_get_action (window->action_group, "fullscreen");
  g_object_get (G_OBJECT (action), "sensitive", &setting, NULL);
  if (setting && fullscreen)
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

  /* setup menubar visibility */
  if (menubar == TERMINAL_VISIBILITY_DEFAULT)
    g_object_get (G_OBJECT (window->preferences), "misc-menubar-default", &setting, NULL);
  else
    setting = (menubar == TERMINAL_VISIBILITY_SHOW);
  action = gtk_action_group_get_action (window->action_group, "show-menubar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), setting);

  /* setup toolbars visibility */
  if (toolbars == TERMINAL_VISIBILITY_DEFAULT)
    g_object_get (G_OBJECT (window->preferences), "misc-toolbars-default", &setting, NULL);
  else
    setting = (toolbars == TERMINAL_VISIBILITY_SHOW);
  action = gtk_action_group_get_action (window->action_group, "edit-toolbars");
  g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
  action = gtk_action_group_get_action (window->action_group, "show-toolbars");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), setting);

  /* setup borders visibility */
  if (borders == TERMINAL_VISIBILITY_DEFAULT)
    g_object_get (G_OBJECT (window->preferences), "misc-borders-default", &setting, NULL);
  else
    setting = (borders == TERMINAL_VISIBILITY_SHOW);
  action = gtk_action_group_get_action (window->action_group, "show-borders");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), setting);

  return GTK_WIDGET (window);
}



/**
 * terminal_window_add:
 * @window  : A #TerminalWindow.
 * @screen  : A #TerminalScreen.
 **/
void
terminal_window_add (TerminalWindow *window,
                     TerminalScreen *screen)
{
  TerminalScreen   *active;
  GtkWidget        *header;
  GtkAction        *action;
  gint              npages;
  gint              page;
  gint              grid_width;
  gint              grid_height;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_SCREEN (screen));

  active = terminal_window_get_active (window);
  if (G_LIKELY (active != NULL))
    {
      terminal_screen_get_size (active, &grid_width, &grid_height);
      terminal_screen_set_size (screen, grid_width, grid_height);
    }

  action = gtk_action_group_get_action (window->action_group, "set-title");

  header = terminal_tab_header_new ();
  exo_binding_new (G_OBJECT (screen), "title", G_OBJECT (header), "title");
  g_signal_connect_swapped (G_OBJECT (header), "close-tab",
                            G_CALLBACK (gtk_widget_destroy), screen);
  g_signal_connect_swapped (G_OBJECT (header), "detach-tab",
                            G_CALLBACK (terminal_window_detach_screen), window);
  g_signal_connect_swapped (G_OBJECT (header), "set-title",
                            G_CALLBACK (gtk_action_activate), action);
  g_object_set_data_full (G_OBJECT (header), "terminal-window-screen",
                          g_object_ref (G_OBJECT (screen)),
                          (GDestroyNotify) g_object_unref);
  gtk_widget_show (header);

  page = gtk_notebook_append_page (GTK_NOTEBOOK (window->notebook),
                                   GTK_WIDGET (screen), header);
  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (window->notebook),
                                      GTK_WIDGET (screen),
                                      TRUE, TRUE, GTK_PACK_START);

  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), npages > 1);

  g_signal_connect (G_OBJECT (screen), "context-menu",
                    G_CALLBACK (terminal_window_context_menu), window);
  g_signal_connect (G_OBJECT (screen), "notify::title",
                    G_CALLBACK (terminal_window_notify_title), window);
  g_signal_connect_swapped (G_OBJECT (screen), "selection-changed",
                            G_CALLBACK (terminal_window_update_actions), window);

  terminal_window_rebuild_gomenu (window);

  /* need to save the grid size here for detached screens */
  terminal_screen_get_size (screen, &grid_width, &grid_height);

  /* need to show this first, else we cannot switch to it */
  gtk_widget_show (GTK_WIDGET (screen));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), page);

  terminal_window_set_size_force_grid (window, screen, grid_width, grid_height);

  gtk_widget_queue_draw (GTK_WIDGET (screen));
}



/**
 * terminal_window_remove:
 * @window  :
 * @screen  :
 **/
void
terminal_window_remove (TerminalWindow *window,
                        TerminalScreen *screen)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_SCREEN (screen));

  gtk_widget_destroy (GTK_WIDGET (screen));
}



/**
 * terminal_window_set_startup_id:
 * @window
 * @startup_id
 **/
void
terminal_window_set_startup_id (TerminalWindow     *window,
                                const gchar        *startup_id)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (startup_id != NULL);

  g_free (window->startup_id);
  window->startup_id = g_strdup (startup_id);
}



/**
 * terminal_window_get_restart_command:
 * @window  : A #TerminalWindow.
 *
 * Return value: A list of strings, which are required to
 *               restart the window properly with all tabs
 *               and settings. The strings and the list itself
 *               need to be freed afterwards.
 **/
GList*
terminal_window_get_restart_command (TerminalWindow *window)
{
  TerminalScreen  *screen;
  const gchar     *role;
  GtkAction       *action;
  GdkScreen       *gscreen;
  GList           *children;
  GList           *result = NULL;
  GList           *lp;
  gint             w;
  gint             h;

  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);

  screen = terminal_window_get_active (window);
  if (G_LIKELY (screen != NULL))
    {
      terminal_screen_get_size (screen, &w, &h);
      result = g_list_append (result, g_strdup_printf ("--geometry=%dx%d", w, h));
    }

  gscreen = gtk_window_get_screen (GTK_WINDOW (window));
  if (G_LIKELY (gscreen != NULL))
    {
      result = g_list_append (result, g_strdup ("--display"));
      result = g_list_append (result, gdk_screen_make_display_name (gscreen));
    }

  role = gtk_window_get_role (GTK_WINDOW (window));
  if (G_LIKELY (role != NULL))
    result = g_list_append (result, g_strdup_printf ("--role=%s", role));

  action = gtk_action_group_get_action (window->action_group, "fullscreen");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_list_append (result, g_strdup ("--fullscreen"));

  action = gtk_action_group_get_action (window->action_group, "show-menubar");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_list_append (result, g_strdup ("--show-menubar"));
  else
    result = g_list_append (result, g_strdup ("--hide-menubar"));

  action = gtk_action_group_get_action (window->action_group, "show-borders");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_list_append (result, g_strdup ("--show-borders"));
  else
    result = g_list_append (result, g_strdup ("--hide-borders"));

  action = gtk_action_group_get_action (window->action_group, "show-toolbars");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_list_append (result, g_strdup ("--show-toolbars"));
  else
    result = g_list_append (result, g_strdup ("--hide-toolbars"));

  /* set restart commands of the tabs */
  children = gtk_container_get_children (GTK_CONTAINER (window->notebook));
  for (lp = children; lp != NULL; lp = lp->next)
    {
      if (lp != children)
        result = g_list_append (result, g_strdup ("--tab"));
      result = g_list_concat (result, terminal_screen_get_restart_command (lp->data));
    }
  g_list_free (children);

  return result;
}
