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
 *
 * The geometry handling code was taken from gnome-terminal. The geometry hacks
 * were initially written by Owen Taylor.
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
#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
#include <libsn/sn-launchee.h>
#endif

#include <terminal/terminal-dialogs.h>
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-options.h>
#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-private.h>
#include <terminal/terminal-stock.h>
#include <terminal/terminal-marshal.h>
#include <terminal/terminal-tab-header.h>
#include <terminal/terminal-toolbars-view.h>
#include <terminal/terminal-window.h>
#include <terminal/terminal-window-ui.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_SHOW_MENUBAR,
  PROP_SHOW_BORDERS,
  PROP_SHOW_TOOLBARS,
};

/* Signal identifiers */
enum
{
  NEW_WINDOW,
  NEW_WINDOW_WITH_SCREEN,
  LAST_SIGNAL,
};



static void            terminal_window_dispose                  (GObject                *object);
static void            terminal_window_finalize                 (GObject                *object);
static void            terminal_window_show                     (GtkWidget              *widget);
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
static void            terminal_window_notebook_visibility      (TerminalWindow         *window);
static void            terminal_window_page_reordered           (GtkNotebook            *notebook,
                                                                 GtkNotebookPage        *page,
                                                                 guint                   page_num,
                                                                 TerminalWindow         *window);
static void            terminal_window_page_added               (GtkNotebook            *notebook,
                                                                 GtkWidget              *child,
                                                                 guint                   page_num,
                                                                 TerminalWindow         *window);
static void            terminal_window_page_removed             (GtkNotebook            *notebook,
                                                                 GtkWidget              *child,
                                                                 guint                   page_num,
                                                                 TerminalWindow         *window);
static void            terminal_window_page_drag_data_received  (GtkWidget              *widget,
                                                                 GdkDragContext         *context,
                                                                 gint                    x,
                                                                 gint                    y,
                                                                 GtkSelectionData       *selection_data,
                                                                 guint                   info,
                                                                 guint                   time,
                                                                 TerminalWindow         *window);
static GtkNotebook    *terminal_window_page_detach              (GtkNotebook            *notebook,
                                                                 GtkWidget              *child,
                                                                 gint                    x,
                                                                 gint                    y,
                                                                 TerminalWindow         *window);
static GtkWidget      *terminal_window_get_context_menu         (TerminalScreen         *screen,
                                                                 TerminalWindow         *window);
static void            terminal_window_detach_screen            (TerminalWindow         *window,
                                                                 TerminalTabHeader      *header);
static void            terminal_window_notify_title             (TerminalScreen         *screen,
                                                                 GParamSpec             *pspec,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_new_tab           (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_new_window        (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_detach_tab        (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_close_tab         (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_close_window      (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_copy              (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_paste             (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_paste_selection   (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_edit_toolbars     (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_prefs             (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_show_menubar      (GtkToggleAction        *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_show_toolbars     (GtkToggleAction        *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_show_borders      (GtkToggleAction        *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_fullscreen        (GtkToggleAction        *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_prev_tab          (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_next_tab          (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_set_title         (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_reset             (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_reset_and_clear   (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_contents          (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_report_bug        (GtkAction              *action,
                                                                 TerminalWindow         *window);
static void            terminal_window_action_about             (GtkAction              *action,
                                                                 TerminalWindow         *window);



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
};



static guint window_signals[LAST_SIGNAL];
static gconstpointer window_notebook_group = "Terminal";



static const GtkActionEntry action_entries[] =
{
  { "file-menu", NULL, N_ ("_File"), NULL, NULL, NULL, },
  { "new-tab", TERMINAL_STOCK_NEWTAB, N_ ("Open _Tab"), NULL, N_ ("Open a new terminal tab"), G_CALLBACK (terminal_window_action_new_tab), }, 
  { "new-window", TERMINAL_STOCK_NEWWINDOW, N_ ("Open T_erminal"), "<control><shift>N", N_ ("Open a new terminal window"), G_CALLBACK (terminal_window_action_new_window), }, 
  { "detach-tab", NULL, N_ ("_Detach Tab"), NULL, N_ ("Open a new window for the current terminal tab"), G_CALLBACK (terminal_window_action_detach_tab), },
  { "close-tab", TERMINAL_STOCK_CLOSETAB, N_ ("C_lose Tab"), NULL, N_ ("Close the current terminal tab"), G_CALLBACK (terminal_window_action_close_tab), },
  { "close-window", TERMINAL_STOCK_CLOSEWINDOW, N_ ("_Close Window"), NULL, N_ ("Close the terminal window"), G_CALLBACK (terminal_window_action_close_window), },
  { "edit-menu", NULL, N_ ("_Edit"), NULL, NULL, NULL, },
  { "copy", GTK_STOCK_COPY, N_ ("_Copy"), NULL, N_ ("Copy to clipboard"), G_CALLBACK (terminal_window_action_copy), },
  { "paste", GTK_STOCK_PASTE, N_ ("_Paste"), NULL, N_ ("Paste from clipboard"), G_CALLBACK (terminal_window_action_paste), },
  { "paste-selection", NULL, N_ ("Paste _Selection"), NULL, N_ ("Paste from primary selection"), G_CALLBACK (terminal_window_action_paste_selection), },
  { "edit-toolbars", NULL, N_ ("_Toolbars..."), NULL, N_ ("Customize the toolbars"), G_CALLBACK (terminal_window_action_edit_toolbars), },
  { "preferences", GTK_STOCK_PREFERENCES, N_ ("Pr_eferences..."), NULL, N_ ("Open the Terminal preferences dialog"), G_CALLBACK (terminal_window_action_prefs), },
  { "view-menu", NULL, N_ ("_View"), NULL, NULL, NULL, },
  { "terminal-menu", NULL, N_ ("_Terminal"), NULL, NULL, NULL, },
  { "set-title", NULL, N_ ("_Set Title..."), NULL, N_ ("Set a custom title for the current tab"), G_CALLBACK (terminal_window_action_set_title), },
  { "reset", GTK_STOCK_REFRESH, N_ ("_Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset), },
  { "reset-and-clear", GTK_STOCK_CLEAR, N_ ("Reset and C_lear"), NULL, NULL, G_CALLBACK (terminal_window_action_reset_and_clear), },
  { "go-menu", NULL, N_ ("_Go"), NULL, NULL, NULL, },
  { "prev-tab", GTK_STOCK_GO_BACK, N_ ("_Previous Tab"), NULL, N_ ("Switch to previous tab"), G_CALLBACK (terminal_window_action_prev_tab), },
  { "next-tab", GTK_STOCK_GO_FORWARD, N_ ("_Next Tab"), NULL, N_ ("Switch to next tab"), G_CALLBACK (terminal_window_action_next_tab), },
  { "help-menu", NULL, N_ ("_Help"), NULL, NULL, NULL, },
  { "contents", GTK_STOCK_HELP, N_ ("_Contents"), NULL, N_ ("Display help contents"), G_CALLBACK (terminal_window_action_contents), },
  { "report-bug", TERMINAL_STOCK_REPORTBUG, N_ ("_Report a bug"), NULL, N_ ("Report a bug in Terminal"), G_CALLBACK (terminal_window_action_report_bug), },
  { "about", GTK_STOCK_ABOUT, N_ ("_About"), NULL, N_ ("Display information about Terminal"), G_CALLBACK (terminal_window_action_about), },
  { "input-methods", NULL, N_ ("_Input Methods"), NULL, NULL, NULL, },
};

static const GtkToggleActionEntry toggle_action_entries[] =
{
  { "show-menubar", TERMINAL_STOCK_SHOWMENU, N_ ("Show _Menubar"), NULL, N_ ("Show/hide the menubar"), G_CALLBACK (terminal_window_action_show_menubar), TRUE, },
  { "show-toolbars", NULL, N_ ("Show _Toolbars"), NULL, N_ ("Show/hide the toolbars"), G_CALLBACK (terminal_window_action_show_toolbars), FALSE, },
  { "show-borders", TERMINAL_STOCK_SHOWBORDERS, N_ ("Show Window _Borders"), NULL, N_ ("Show/hide the window decorations"), G_CALLBACK (terminal_window_action_show_borders), TRUE, },
  { "fullscreen", TERMINAL_STOCK_FULLSCREEN, N_ ("_Fullscreen"), NULL, N_ ("Toggle fullscreen mode"), G_CALLBACK (terminal_window_action_fullscreen), FALSE, },
};



G_DEFINE_TYPE (TerminalWindow, terminal_window, GTK_TYPE_WINDOW)



static void
terminal_window_class_init (TerminalWindowClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_window_dispose;
  gobject_class->finalize = terminal_window_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->show = terminal_window_show;

  /**
   * TerminalWindow::new-window
   **/
  window_signals[NEW_WINDOW] =
    g_signal_new (I_("new-window"),
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
    g_signal_new (I_("new-window-with-screen"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (TerminalWindowClass, new_window_with_screen),
                  NULL, NULL,
                  _terminal_marshal_VOID__OBJECT_INT_INT,
                  G_TYPE_NONE, 3,
                  G_TYPE_OBJECT,
                  G_TYPE_INT, G_TYPE_INT);
}



static void
terminal_window_init (TerminalWindow *window)
{
  GtkAccelGroup  *accel_group;
  GtkAction      *action;
  GtkWidget      *item;
  GtkWidget      *vbox;
  gboolean        bval;
  gchar          *role;

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
  gtk_ui_manager_add_ui_from_string (window->ui_manager, terminal_window_ui, terminal_window_ui_length, NULL);
  

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

#if defined(GDK_WINDOWING_X11)
  /* setup fullscreen mode */
  if (!gdk_net_wm_supports (gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
    {
      action = gtk_action_group_get_action (window->action_group, "fullscreen");
      gtk_action_set_sensitive (action, FALSE);
    }
#endif

  /* check if tabs should always be shown */
  g_object_get (G_OBJECT (window->preferences), "misc-always-show-tabs", &bval, NULL);

  /* allocate the notebook for the terminal screens */
  window->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                                   "homogeneous", TRUE,
                                   "scrollable", TRUE,
                                   "show-border", FALSE,
                                   "show-tabs", bval,
                                   "tab-hborder", 0,
                                   "tab-vborder", 0,
                                   NULL);
  exo_binding_new (G_OBJECT (window->preferences), "misc-tab-position", G_OBJECT (window->notebook), "tab-pos");

  /* set the notebook group id */
  gtk_notebook_set_group (GTK_NOTEBOOK (window->notebook), 
                          (gpointer) window_notebook_group);

  /* signals */
  g_signal_connect (G_OBJECT (window->notebook), "notify::page",
                    G_CALLBACK (terminal_window_page_notified), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-reordered",
                    G_CALLBACK (terminal_window_page_reordered), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-removed",
                    G_CALLBACK (terminal_window_page_removed), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-added",
                    G_CALLBACK (terminal_window_page_added), window);
  g_signal_connect (G_OBJECT (window->notebook), "create-window",
                    G_CALLBACK (terminal_window_page_detach), window);

  gtk_box_pack_start (GTK_BOX (vbox), window->notebook, TRUE, TRUE, 0);
  gtk_widget_show (window->notebook);

  g_object_connect (G_OBJECT (window),
                    "signal::delete-event", G_CALLBACK (terminal_window_delete_event), NULL,
                    "signal-after::style-set", G_CALLBACK (terminal_window_queue_reset_size), NULL,
                    NULL);

  /* set a unique role on each window (for session management) */
  role = g_strdup_printf ("Terminal-%p-%d-%d", window, (gint) getpid (), (gint) time (NULL));
  gtk_window_set_role (GTK_WINDOW (window), role);
  g_free (role);
}



static void
terminal_window_dispose (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  if (window->reset_size_idle_id != 0)
    g_source_remove (window->reset_size_idle_id);

  g_signal_handlers_disconnect_matched (G_OBJECT (window->preferences),
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, window);

  (*G_OBJECT_CLASS (terminal_window_parent_class)->dispose) (object);
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  g_object_unref (G_OBJECT (window->preferences));
  g_object_unref (G_OBJECT (window->action_group));
  g_object_unref (G_OBJECT (window->ui_manager));

  g_free (window->startup_id);

  (*G_OBJECT_CLASS (terminal_window_parent_class)->finalize) (object);
}



static void
terminal_window_show (GtkWidget *widget)
{
#if defined(GDK_WINDOWING_X11) && defined(HAVE_LIBSTARTUP_NOTIFICATION)
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

  (*GTK_WIDGET_CLASS (terminal_window_parent_class)->show) (widget);

#if defined(GDK_WINDOWING_X11) && defined(HAVE_LIBSTARTUP_NOTIFICATION)
  if (G_LIKELY (sn_context != NULL))
    {
      sn_launchee_context_complete (sn_context);
      sn_launchee_context_unref (sn_context);
      sn_display_unref (sn_display);
    }
#endif
}



static gboolean
terminal_window_confirm_close (TerminalWindow *window)
{
  GtkWidget *dialog;
  GtkWidget *button;
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
                                        NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  GTK_STOCK_CANCEL,
                                  GTK_RESPONSE_CANCEL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("Close all tabs"),
                                  GTK_RESPONSE_YES);
  gtk_widget_grab_default (button);
  gtk_widget_grab_focus (button);

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

  GDK_THREADS_ENTER ();

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

  GDK_THREADS_LEAVE ();

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

  /* determine the number of pages */
  n_pages = gtk_notebook_get_n_pages (notebook);

  /* "Detach Tab" is only sensitive if we have atleast two pages */
  action = gtk_action_group_get_action (window->action_group, "detach-tab");
  gtk_action_set_sensitive (action, (n_pages > 1));

  /* ... same for the "Close Tab" action */
  action = gtk_action_group_get_action (window->action_group, "close-tab");
  gtk_action_set_sensitive (action, (n_pages > 1));

  /* update the actions for the current terminal screen */
  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    {
      page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (terminal));

      g_object_get (G_OBJECT (window->preferences),
                    "misc-cycle-tabs", &cycle_tabs,
                    NULL);

      action = gtk_action_group_get_action (window->action_group, "prev-tab");
      gtk_action_set_sensitive (action, (cycle_tabs && n_pages > 1) || (page_num > 0));

      action = gtk_action_group_get_action (window->action_group, "next-tab");
      gtk_action_set_sensitive (action, (cycle_tabs && n_pages > 1 ) || (page_num < n_pages - 1));

      action = gtk_action_group_get_action (window->action_group, "copy");
      gtk_action_set_sensitive (action, terminal_screen_has_selection (terminal));

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

      /* Create the new radio menu item, and be sure to override
       * the "can-activate-accel" method, which by default requires
       * that the widget is on-screen, but we want the accelerators
       * to work even if the menubar is hidden.
       */
      item = gtk_radio_menu_item_new (group);
      g_signal_connect (G_OBJECT (item), "can-activate-accel", G_CALLBACK (gtk_true), NULL);
      gtk_menu_shell_append (GTK_MENU_SHELL (window->gomenu), item);
      gtk_widget_show (item);

      label = g_object_new (GTK_TYPE_ACCEL_LABEL, "xalign", 0.0, NULL);
      exo_binding_new (G_OBJECT (terminal), "title", G_OBJECT (label), "label");
      gtk_container_add (GTK_CONTAINER (item), label);
      gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), item);
      gtk_widget_show (label);

      /* only connect an accelerator if we have a preference for this item */
      g_snprintf (name, 32, "accel-switch-to-tab%d", n + 1);
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (window->preferences), name) != NULL)
        {
          /* connect the menu item to an accelerator */
          path = g_strconcat ("<Actions>/terminal-window/", name + 6, NULL);
          gtk_menu_item_set_accel_path (GTK_MENU_ITEM (item), path);
          g_free (path);
        }

      if (gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook)) == n)
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);

      /* keep an extra ref terminal -> item, so we don't in trouble with gtk_widget_destroy */
      g_object_set_data_full (G_OBJECT (terminal), I_("terminal-window-go-menu-item"),
                              G_OBJECT (item), (GDestroyNotify) item_destroy);
      g_object_ref (G_OBJECT (item));

      /* do the item -> terminal connects */
      g_object_set_data (G_OBJECT (item), I_("terminal-window-screen"), terminal);
      g_signal_connect (G_OBJECT (item), "toggled", G_CALLBACK (item_toggled), window->notebook);

      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    }
}



static gboolean
terminal_window_delete_event (TerminalWindow *window)
{
  gboolean result;

  /* get close confirmation from the user */
  result = terminal_window_confirm_close (window);

  /* disconnect remove signal if we're closing the window */
  if (result)
    g_signal_handlers_disconnect_by_func (G_OBJECT (window->notebook),
                                          G_CALLBACK (terminal_window_page_removed), window);

  return !result;
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
      g_object_set (G_OBJECT (terminal), "activity", FALSE, NULL);
    }
}



static void
terminal_window_notebook_visibility (TerminalWindow *window)
{
  TerminalScreen *active;
  gboolean        always_show_tabs, tabs_shown;
  gint            npages;
  gint            grid_width, grid_height;

  /* check if we should always display tabs */
  g_object_get (G_OBJECT (window->preferences), "misc-always-show-tabs", &always_show_tabs, NULL);

  /* current notebook status */
  tabs_shown = gtk_notebook_get_show_tabs (GTK_NOTEBOOK (window->notebook));
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));

  /* change the visibility if the new status differs */
  if (((npages > 1) != tabs_shown) || (always_show_tabs && !tabs_shown))
    {
      /* get active screen */
      active = terminal_window_get_active (window);

      /* get screen grid size */
      terminal_screen_get_size (active, &grid_width, &grid_height);

      /* show or hide the tabs */
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), !tabs_shown);

      /* don't focus the notebook */
      GTK_WIDGET_UNSET_FLAGS (window->notebook, GTK_CAN_FOCUS);

      /* resize the window */
      terminal_screen_force_resize_window (active, GTK_WINDOW (window), grid_width, grid_height);
    }
}



static void
terminal_window_page_reordered (GtkNotebook     *notebook,
                                GtkNotebookPage *page,
                                guint            page_num,
                                TerminalWindow  *window)
{

  /* Regenerate the "Go" menu */
  terminal_window_rebuild_gomenu (window);
}



static void
terminal_window_page_added (GtkNotebook    *notebook,
                            GtkWidget      *child,
                            guint           page_num,
                            TerminalWindow *window)
{
  gint               grid_width, grid_height;
  GtkAction         *action;
  TerminalTabHeader *header;
  TerminalScreen    *active;

  _terminal_return_if_fail (TERMINAL_IS_SCREEN (child));
  _terminal_return_if_fail (TERMINAL_IS_WINDOW (window));

  /* get tab header */
  header = g_object_get_data (G_OBJECT (child), I_("terminal-tab-header"));

  _terminal_return_if_fail (TERMINAL_IS_TAB_HEADER (header));

  /* tab position binding */
  header->tab_pos_binding = exo_binding_new (G_OBJECT (window->notebook), "tab-pos", G_OBJECT (header), "tab-pos");

  /* connect screen signals */
  g_signal_connect (G_OBJECT (child), "get-context-menu", G_CALLBACK (terminal_window_get_context_menu), window);
  g_signal_connect (G_OBJECT (child), "notify::title", G_CALLBACK (terminal_window_notify_title), window);
  g_signal_connect_swapped (G_OBJECT (child), "selection-changed", G_CALLBACK (terminal_window_update_actions), window);
  g_signal_connect (G_OBJECT (child), "drag-data-received", G_CALLBACK (terminal_window_page_drag_data_received), window);

  /* get action from window action group */
  action = gtk_action_group_get_action (window->action_group, "set-title");

  /* connect tab label signals */
  g_signal_connect_swapped (G_OBJECT (header), "detach-tab", G_CALLBACK (terminal_window_detach_screen), window);
  g_signal_connect_swapped (G_OBJECT (header), "set-title", G_CALLBACK (gtk_action_activate), action);

  /* set visibility of the notebook */
  terminal_window_notebook_visibility (window);

  /* get the active tab */
  active = terminal_window_get_active (window);
  if (G_LIKELY (active != NULL))
    {
      /* set new screen grid size based on the old one */
      terminal_screen_get_size (active, &grid_width, &grid_height);
      terminal_screen_set_size (TERMINAL_SCREEN (child), grid_width, grid_height);
    }
  else
    {
      /* set the window size (needed for resizing) */
      terminal_screen_get_size (TERMINAL_SCREEN (child), &grid_width, &grid_height);
      terminal_window_set_size_force_grid (window, TERMINAL_SCREEN (child), grid_width, grid_height);
    }

  /* regenerate the "Go" menu */
  terminal_window_rebuild_gomenu (window);

  /* update all screen sensitive actions (Copy, Prev Tab, ...) */
  terminal_window_update_actions (window);
}



static void
terminal_window_page_removed (GtkNotebook    *notebook,
                              GtkWidget      *child,
                              guint           page_num,
                              TerminalWindow *window)
{
  TerminalTabHeader *header = g_object_get_data (G_OBJECT (child), I_("terminal-tab-header"));
  GtkAction         *action;
  gint               npages;

  _terminal_return_if_fail (TERMINAL_IS_SCREEN (child));
  _terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  _terminal_return_if_fail (TERMINAL_IS_TAB_HEADER (header));
  _terminal_return_if_fail (header->tab_pos_binding != NULL);

  /* get old window action */
  action = gtk_action_group_get_action (window->action_group, "set-title");

  /* unset the go menu item */
  g_object_set_data (G_OBJECT (child), I_("terminal-window-go-menu-item"), NULL);

  /* disconnect signals */
  g_signal_handlers_disconnect_by_func (G_OBJECT (child), terminal_window_get_context_menu, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (child), terminal_window_notify_title, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (child), terminal_window_update_actions, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (child), terminal_window_page_drag_data_received, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (header), terminal_window_detach_screen, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (header), gtk_action_activate, action);

  /* remove exo binding */
  exo_binding_unbind (header->tab_pos_binding);

  /* set tab visibility */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  if (G_UNLIKELY (npages == 0))
    {
      /* no tabs, destroy the window */
      gtk_widget_destroy (GTK_WIDGET (window));
    }
  else
    {
      /* set visibility of the notebook */
      terminal_window_notebook_visibility (window);

      /* regenerate the "Go" menu */
      terminal_window_rebuild_gomenu (window);

      /* update all screen sensitive actions (Copy, Prev Tab, ...) */
      terminal_window_update_actions (window);
    }
}



static void
terminal_window_page_drag_data_received (GtkWidget        *widget,
                                         GdkDragContext   *context,
                                         gint              x,
                                         gint              y,
                                         GtkSelectionData *selection_data,
                                         guint             info,
                                         guint32           drag_time,
                                         TerminalWindow *window)
{
  GtkWidget  *source_widget;
  GtkWidget **screen;
  GtkWidget  *child, *label;
  gint        i, n_pages;

  _terminal_return_if_fail (TERMINAL_IS_WINDOW (window));

  /* get the source notebook (other window) */
  source_widget = gtk_drag_get_source_widget (context);

  /* get the dragged screen */
  screen = (GtkWidget **) selection_data->data;

  /* check */
  if (source_widget && TERMINAL_IS_SCREEN (*screen))
    {
      /* take a reference */
      g_object_ref (G_OBJECT (*screen));

      /* remove the document from the source notebook */
      gtk_container_remove (GTK_CONTAINER (source_widget), *screen);

      /* get the number of pages in the new window */
      n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));

      /* figure out where to insert the tab in the notebook */
      for (i = 0; i < n_pages; i++)
        {
          /* get the child label */
          child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), i);
          label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (window->notebook), child);

          /* break if we have a matching drop position */
          if (x < (label->allocation.x + label->allocation.width / 2))
            break;
        }

      /* add the screen to the new window */
      terminal_window_add (window, TERMINAL_SCREEN (*screen));

      /* move the child to the correct position */
      gtk_notebook_reorder_child (GTK_NOTEBOOK (window->notebook), *screen, i);

      /* release reference */
      g_object_unref (G_OBJECT (*screen));

      /* finish the drag */
      gtk_drag_finish (context, TRUE, TRUE, drag_time);
    }
}



static GtkNotebook *
terminal_window_page_detach (GtkNotebook    *notebook,
                             GtkWidget      *child,
                             gint            x,
                             gint            y,
                             TerminalWindow *window)
{
  TerminalScreen *screen;

  _terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);
  _terminal_return_val_if_fail (TERMINAL_IS_SCREEN (child), NULL);
  _terminal_return_val_if_fail (notebook == GTK_NOTEBOOK (window->notebook), NULL);

  /* only create new window when there are more then 2 tabs (bug #2686) */
  if (gtk_notebook_get_n_pages (notebook) >= 2)
    {
      /* get the screen */
      screen = TERMINAL_SCREEN (child);

      /* take a reference */
      g_object_ref (G_OBJECT (screen));

      /* remove screen from active window */
      gtk_container_remove (GTK_CONTAINER (window->notebook), child);

      /* create new window with the screen */
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW_WITH_SCREEN], 0, screen, x, y);

      /* release our reference */
      g_object_unref (G_OBJECT (screen));
    }
    
  return NULL;
}



static GtkWidget*
terminal_window_get_context_menu (TerminalScreen  *screen,
                                  TerminalWindow  *window)
{
  TerminalScreen *terminal;
  GtkWidget      *popup = NULL;
  GtkWidget      *menu;
  GtkWidget      *item;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (screen == terminal))
    {
      popup = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu");
      if (G_LIKELY (popup != NULL))
        {
          item = gtk_ui_manager_get_widget (window->ui_manager, "/popup-menu/input-methods");
          if (G_LIKELY (item != NULL && GTK_IS_MENU_ITEM (item)))
            {
              /* append input methods */
              menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (item));
              if (G_LIKELY (menu != NULL))
                gtk_widget_destroy (menu);
              menu = gtk_menu_new ();
              terminal_screen_im_append_menuitems (screen, GTK_MENU_SHELL (menu));
              gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
            }
        }
    }

  return popup;
}



static void
terminal_window_detach_screen (TerminalWindow     *window,
                               TerminalTabHeader  *header)
{
  GtkWidget *screen;

  /* get the screen */
  screen = g_object_get_data (G_OBJECT (header), I_("terminal-window-screen"));
  if (G_LIKELY (screen != NULL))
    terminal_window_page_detach (GTK_NOTEBOOK (window->notebook), screen, -1, -1, window);
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
terminal_window_action_detach_tab (GtkAction      *action,
                                   TerminalWindow *window)
{
  TerminalScreen *terminal;

  /* get active terminal window */
  terminal = terminal_window_get_active (window);

  if (G_LIKELY (terminal != NULL))
    terminal_window_page_detach (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (terminal), -1, -1, window);
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
terminal_window_action_paste_selection (GtkAction      *action,
                                        TerminalWindow *window)
{
  TerminalScreen *terminal;

  terminal = terminal_window_get_active (window);
  if (G_LIKELY (terminal != NULL))
    terminal_screen_paste_primary (terminal);
}



static void
terminal_window_action_edit_toolbars (GtkAction       *action,
                                      TerminalWindow  *window)
{
  if (G_LIKELY (window->toolbars != NULL))
    terminal_toolbars_view_edit (TERMINAL_TOOLBARS_VIEW (window->toolbars));
}



static void
terminal_window_action_prefs (GtkAction      *action,
                              TerminalWindow *window)
{
  /* check if we already have a preferences dialog instance */
  if (G_UNLIKELY (window->preferences_dialog != NULL)) 
    {
      /* move to the current screen and make transient for this window */
      gtk_window_set_screen (GTK_WINDOW (window->preferences_dialog), gtk_widget_get_screen (GTK_WIDGET (window)));
      gtk_window_set_transient_for (GTK_WINDOW (window->preferences_dialog), GTK_WINDOW (window));

      /* present the preferences dialog on the current workspace */
      gtk_window_present (GTK_WINDOW (window->preferences_dialog));
    }
  else
    {
      /* allocate a new preferences dialog instance */
      window->preferences_dialog = terminal_preferences_dialog_new (GTK_WINDOW (window));
      g_object_add_weak_pointer (G_OBJECT (window->preferences_dialog), (gpointer) &window->preferences_dialog);
      gtk_widget_show (window->preferences_dialog);
    }
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

      gtk_action_set_sensitive (action_edit, TRUE);
    }
  else
    {
      if (window->toolbars != NULL)
        gtk_widget_destroy (window->toolbars);

      gtk_action_set_sensitive (action_edit, FALSE);
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
terminal_window_action_prev_tab (GtkAction       *action,
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
terminal_window_action_next_tab (GtkAction       *action,
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
title_dialog_response (GtkWidget *dialog,
                       gint       response)
{
  /* check if we should open the user manual */
  if (response == GTK_RESPONSE_HELP)
    {
      /* open the "Set Title" paragraph in the "Usage" section */
      terminal_dialogs_show_help (GTK_WIDGET (dialog), "usage", "set-title");
    }
  else
    {
      /* close the dialog */
      gtk_widget_destroy (dialog);
    }
}



static void
terminal_window_action_set_title (GtkAction      *action,
                                  TerminalWindow *window)
{
  TerminalScreen *screen;
  AtkRelationSet *relations;
  AtkRelation    *relation;
  AtkObject      *object;
  GtkWidget      *dialog;
  GtkWidget      *box;
  GtkWidget      *label;
  GtkWidget      *entry;

  screen = terminal_window_get_active (window);
  if (G_LIKELY (screen != NULL))
    {
      dialog = gtk_dialog_new_with_buttons (Q_("Window Title|Set Title"),
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
  /* open the "Support" section of the user manual */
  terminal_dialogs_show_help (GTK_WIDGET (window), "support", NULL);
}



static void
terminal_window_action_contents (GtkAction       *action,
                                 TerminalWindow  *window)
{
  /* open the Terminal user manual */
  terminal_dialogs_show_help (GTK_WIDGET (window), NULL, NULL);
}



static void
terminal_window_action_about (GtkAction      *action,
                              TerminalWindow *window)
{
  /* display the about dialog */
  terminal_dialogs_show_about (GTK_WINDOW (window));
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
                     TerminalVisibility toolbars,
                     gboolean           maximize)
{
  TerminalWindow *window;
  GtkAction      *action;
  gboolean        setting;
  
  window = g_object_new (TERMINAL_TYPE_WINDOW, NULL);

  /* setup full screen */
  action = gtk_action_group_get_action (window->action_group, "fullscreen");
  if (fullscreen && gtk_action_is_sensitive (action))
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

  /* maximize */
  if (maximize)
    gtk_window_maximize (&window->__parent__);

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
  gtk_action_set_sensitive (action, FALSE);
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
  GtkWidget      *header;
  gint            page;

  _terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  _terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  header = terminal_tab_header_new ();
  exo_binding_new (G_OBJECT (screen), "title", G_OBJECT (header), "title");
  exo_binding_new (G_OBJECT (screen), "activity", G_OBJECT (header), "activity");
  g_signal_connect_swapped (G_OBJECT (header), "close-tab", G_CALLBACK (gtk_widget_destroy), screen);
  g_object_set_data_full (G_OBJECT (header), I_("terminal-window-screen"), g_object_ref (G_OBJECT (screen)), (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (screen), I_("terminal-tab-header"), g_object_ref (G_OBJECT (header)), (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (screen), I_("terminal-window"), g_object_ref (G_OBJECT (window)), (GDestroyNotify) g_object_unref);
  gtk_widget_show (header);

  page = gtk_notebook_append_page (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (screen), header);
  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (screen), TRUE, TRUE, GTK_PACK_START);

  /* allow tab sorting and dnd */
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (screen), TRUE);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (screen), TRUE);

  /* need to show this first, else we cannot switch to it */
  gtk_widget_show (GTK_WIDGET (screen));
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), page);
}



/**
 * terminal_window_remove:
 * @window  : A #TerminalWindow.
 * @screen  : A #TerminalScreen.
 **/
void
terminal_window_remove (TerminalWindow *window,
                        TerminalScreen *screen)
{
  _terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  _terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  gtk_widget_destroy (GTK_WIDGET (screen));
}



/**
 * terminal_window_get_active:
 * @window : a #TerminalWindow.
 *
 * Returns the active #TerminalScreen for @window
 * or %NULL.
 *
 * Return value: the active #TerminalScreen for @window.
 **/
TerminalScreen*
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

/**
 * terminal_window_is_screen_active:
 * @screen : a #TerminalScreen.
 *
 * Return value: TRUE if @screen is active.
 **/
gboolean 
terminal_window_is_screen_active (TerminalScreen *screen)
{
  TerminalWindow *window = NULL;
  GtkNotebook    *notebook;
  gint            page_num;
  
  window = g_object_get_data (G_OBJECT (screen), I_("terminal-window"));
  _terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);
  notebook = GTK_NOTEBOOK (window->notebook);
  page_num = gtk_notebook_get_current_page (notebook);
  if (G_LIKELY (page_num >= 0))
    return (TERMINAL_SCREEN (gtk_notebook_get_nth_page (notebook, page_num)) == screen);
  else
    return FALSE;
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
  _terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  _terminal_return_if_fail (startup_id != NULL);

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

  _terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);

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
