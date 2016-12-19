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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif

#include <terminal/terminal-util.h>
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-options.h>
#include <terminal/terminal-preferences-dialog.h>
#include <terminal/terminal-search-dialog.h>
#include <terminal/terminal-private.h>
#include <terminal/terminal-marshal.h>
#include <terminal/terminal-encoding-action.h>
#include <terminal/terminal-window.h>
#include <terminal/terminal-window-dropdown.h>
#include <terminal/terminal-window-ui.h>
#include <terminal/terminal-widget.h>



/* Closed tabs stored info */
typedef struct
{
  gchar   *custom_title;
  gchar   *working_directory;
  gint     position;
  gboolean was_active;
} TerminalWindowTabInfo;

/* Signal identifiers */
enum
{
  NEW_WINDOW,
  NEW_WINDOW_WITH_SCREEN,
  LAST_SIGNAL
};



static void         terminal_window_finalize                      (GObject                *object);
static gboolean     terminal_window_delete_event                  (GtkWidget              *widget,
                                                                   GdkEventAny            *event);
static gboolean     terminal_window_state_event                   (GtkWidget              *widget,
                                                                   GdkEventWindowState    *event);
static void         terminal_window_style_set                     (GtkWidget              *widget,
                                                                   GtkStyle               *previous_style);
static gboolean     terminal_window_scroll_event                  (GtkWidget              *widget,
                                                                   GdkEventScroll         *event);
static gboolean     terminal_window_key_press_event               (GtkWidget              *widget,
                                                                   GdkEventKey            *event);
static gboolean     terminal_window_confirm_close                 (TerminalWindow         *window);
static void         terminal_window_size_push                     (TerminalWindow         *window);
static gboolean     terminal_window_size_pop                      (gpointer                data);
static void         terminal_window_set_size_force_grid           (TerminalWindow         *window,
                                                                   TerminalScreen         *screen,
                                                                   glong                   force_grid_width,
                                                                   glong                   force_grid_height);
static gboolean     terminal_window_accel_activate                (GtkAccelGroup          *accel_group,
                                                                   GObject                *acceleratable,
                                                                   guint                   accel_key,
                                                                   GdkModifierType         accel_mods,
                                                                   TerminalWindow         *window);
static void         terminal_window_update_actions                (TerminalWindow         *window);
static void         terminal_window_notebook_page_switched        (GtkNotebook            *notebook,
                                                                   GtkWidget              *page,
                                                                   guint                   page_num,
                                                                   TerminalWindow         *window);
static void         terminal_window_notebook_page_reordered       (GtkNotebook            *notebook,
                                                                   GtkWidget              *child,
                                                                   guint                   page_num,
                                                                   TerminalWindow         *window);
static void         terminal_window_notebook_page_added           (GtkNotebook            *notebook,
                                                                   GtkWidget              *child,
                                                                   guint                   page_num,
                                                                   TerminalWindow         *window);
static void         terminal_window_notebook_page_removed         (GtkNotebook            *notebook,
                                                                   GtkWidget              *child,
                                                                   guint                   page_num,
                                                                   TerminalWindow         *window);
static gboolean     terminal_window_notebook_event_in_allocation  (gint                    event_x,
                                                                   gint                    event_y,
                                                                   GtkWidget              *widget);
static gboolean     terminal_window_notebook_button_press_event   (GtkNotebook            *notebook,
                                                                   GdkEventButton         *event,
                                                                   TerminalWindow         *window);
static gboolean     terminal_window_notebook_button_release_event (GtkNotebook            *notebook,
                                                                   GdkEventButton         *event,
                                                                   TerminalWindow         *window);
static gboolean     terminal_window_notebook_scroll_event         (GtkNotebook            *notebook,
                                                                   GdkEventScroll         *event,
                                                                   TerminalWindow         *window);
static void         terminal_window_notebook_drag_data_received   (GtkWidget              *widget,
                                                                   GdkDragContext         *context,
                                                                   gint                    x,
                                                                   gint                    y,
                                                                   GtkSelectionData       *selection_data,
                                                                   guint                   info,
                                                                   guint                   time,
                                                                   TerminalWindow         *window);
static GtkNotebook *terminal_window_notebook_create_window        (GtkNotebook            *notebook,
                                                                   GtkWidget              *child,
                                                                   gint                    x,
                                                                   gint                    y,
                                                                   TerminalWindow         *window);
static GtkWidget   *terminal_window_get_context_menu              (TerminalScreen         *screen,
                                                                   TerminalWindow         *window);
static void         terminal_window_notify_title                  (TerminalScreen         *screen,
                                                                   GParamSpec             *pspec,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_set_encoding           (GtkAction              *action,
                                                                   const gchar            *charset,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_new_tab                (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_new_window             (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_undo_close_tab         (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_detach_tab             (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_close_tab              (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_close_other_tabs       (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_close_window           (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_copy                   (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_paste                  (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_paste_selection        (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_select_all             (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_prefs                  (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_show_menubar           (GtkToggleAction        *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_show_toolbar           (GtkToggleAction        *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_show_borders           (GtkToggleAction        *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_fullscreen             (GtkToggleAction        *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_readonly               (GtkToggleAction        *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_zoom_in                (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_zoom_out               (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_zoom_reset             (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_prev_tab               (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_next_tab               (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_move_tab_left          (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_move_tab_right         (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_goto_tab               (GtkRadioAction         *action,
                                                                   GtkNotebook            *notebook);
static void         terminal_window_action_set_title              (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_search                 (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_search_next            (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_search_prev            (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_save_contents          (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_reset                  (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_reset_and_clear        (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_contents               (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_action_about                  (GtkAction              *action,
                                                                   TerminalWindow         *window);
static void         terminal_window_zoom_update_screens           (TerminalWindow         *window);
static void         terminal_window_switch_tab                    (GtkNotebook            *notebook,
                                                                   gboolean                switch_left);
static void         terminal_window_move_tab                      (GtkNotebook            *notebook,
                                                                   gboolean                move_left);
static void         terminal_window_tab_info_free                 (TerminalWindowTabInfo  *tab_info);
static void         terminal_window_menubar_deactivate            (GtkWidget              *widget,
                                                                   TerminalWindow         *window);



struct _TerminalWindowPrivate
{
  GtkUIManager        *ui_manager;

  GtkWidget           *vbox;
  GtkWidget           *notebook;
  GtkWidget           *menubar;
  GtkWidget           *toolbar;

  /* for the drop-down to keep open with dialogs */
  guint                n_child_windows;

  guint                tabs_menu_merge_id;
  GSList              *tabs_menu_actions;

  TerminalPreferences *preferences;
  GtkWidget           *preferences_dialog;

  GtkActionGroup      *action_group;

  GtkWidget           *search_dialog;
  GtkWidget           *title_dialog;

  /* pushed size of screen */
  glong                grid_width;
  glong                grid_height;

  GtkAction           *encoding_action;

  TerminalScreen      *active;
  TerminalScreen      *last_closed_active;

  /* cached actions to avoid lookups */
  GtkAction           *action_undo_close_tab;
  GtkAction           *action_detach_tab;
  GtkAction           *action_close_other_tabs;
  GtkAction           *action_prev_tab;
  GtkAction           *action_next_tab;
  GtkAction           *action_move_tab_left;
  GtkAction           *action_move_tab_right;
  GtkAction           *action_copy;
  GtkAction           *action_search_next;
  GtkAction           *action_search_prev;
  GtkAction           *action_fullscreen;

  GQueue              *closed_tabs_list;

  TerminalVisibility   scrollbar_visibility;
};

static guint   window_signals[LAST_SIGNAL];
static gchar   *window_notebook_group = PACKAGE_NAME;
static GQuark  tabs_menu_action_quark = 0;



static const GtkActionEntry action_entries[] =
{
  { "file-menu", NULL, N_ ("_File"), NULL, NULL, NULL, },
    { "new-tab", "tab-new", N_ ("Open _Tab"), "<control><shift>t", N_ ("Open a new terminal tab"), G_CALLBACK (terminal_window_action_new_tab), },
    { "new-window", "window-new", N_ ("Open T_erminal"), "<control><shift>n", N_ ("Open a new terminal window"), G_CALLBACK (terminal_window_action_new_window), },
    { "undo-close-tab", "document-revert", N_ ("_Undo Close Tab"), NULL, NULL, G_CALLBACK (terminal_window_action_undo_close_tab), },
    { "detach-tab", NULL, N_ ("_Detach Tab"), "<control><shift>d", NULL, G_CALLBACK (terminal_window_action_detach_tab), },
    { "close-tab", "window-close", N_ ("Close T_ab"), "<control><shift>w", NULL, G_CALLBACK (terminal_window_action_close_tab), },
    { "close-other-tabs", "edit-clear", N_ ("Close Other Ta_bs"), NULL, NULL, G_CALLBACK (terminal_window_action_close_other_tabs), },
    { "close-window", "application-exit", N_ ("Close _Window"), "<control><shift>q", NULL, G_CALLBACK (terminal_window_action_close_window), },
  { "edit-menu", NULL, N_ ("_Edit"), NULL, NULL, NULL, },
    { "copy", "edit-copy", N_ ("_Copy"), "<control><shift>c", N_ ("Copy to clipboard"), G_CALLBACK (terminal_window_action_copy), },
    { "paste", "edit-paste", N_ ("_Paste"), "<control><shift>v", N_ ("Paste from clipboard"), G_CALLBACK (terminal_window_action_paste), },
    { "paste-selection", NULL, N_ ("Paste _Selection"), NULL, NULL, G_CALLBACK (terminal_window_action_paste_selection), },
    { "select-all", "edit-select-all", N_ ("Select _All"), "<control><shift>a", NULL, G_CALLBACK (terminal_window_action_select_all), },
    { "preferences", "preferences-system", N_ ("Pr_eferences..."), NULL, N_ ("Open the preferences dialog"), G_CALLBACK (terminal_window_action_prefs), },
  { "view-menu", NULL, N_ ("_View"), NULL, NULL, NULL, },
    { "zoom-in", "zoom-in", N_ ("Zoom _In"), "<control>plus", N_ ("Zoom in with larger font"), G_CALLBACK (terminal_window_action_zoom_in), },
    { "zoom-out", "zoom-out", N_ ("Zoom _Out"), "<control>minus", N_ ("Zoom out with smaller font"), G_CALLBACK (terminal_window_action_zoom_out), },
    { "zoom-reset", "zoom-original", N_ ("_Normal Size"), "<control>0", N_ ("Zoom to default size"), G_CALLBACK (terminal_window_action_zoom_reset), },
  { "terminal-menu", NULL, N_ ("_Terminal"), NULL, NULL, NULL, },
    { "set-title", NULL, N_ ("_Set Title..."), "<control><shift>s", NULL, G_CALLBACK (terminal_window_action_set_title), },
    { "search", "edit-find", N_ ("_Find..."), "<control><shift>f", N_ ("Search terminal contents"), G_CALLBACK (terminal_window_action_search), },
    { "search-next", NULL, N_ ("Find Ne_xt"), NULL, NULL, G_CALLBACK (terminal_window_action_search_next), },
    { "search-prev", NULL, N_ ("Find Pre_vious"), NULL, NULL, G_CALLBACK (terminal_window_action_search_prev), },
    { "save-contents", "document-save-as", N_ ("Sa_ve Contents..."), NULL, NULL, G_CALLBACK (terminal_window_action_save_contents), },
    { "reset", NULL, N_ ("_Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset), },
    { "reset-and-clear", NULL, N_ ("_Clear Scrollback and Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset_and_clear), },
  { "tabs-menu", NULL, N_ ("T_abs"), NULL, NULL, NULL, },
    { "prev-tab", "go-previous", N_ ("_Previous Tab"), "<control>Page_Up", N_ ("Switch to previous tab"), G_CALLBACK (terminal_window_action_prev_tab), },
    { "next-tab", "go-next", N_ ("_Next Tab"), "<control>Page_Down", N_ ("Switch to next tab"), G_CALLBACK (terminal_window_action_next_tab), },
    { "move-tab-left", NULL, N_ ("Move Tab _Left"), "<control><shift>Page_Up", NULL, G_CALLBACK (terminal_window_action_move_tab_left), },
    { "move-tab-right", NULL, N_ ("Move Tab _Right"), "<control><shift>Page_Down", NULL, G_CALLBACK (terminal_window_action_move_tab_right), },
  { "help-menu", NULL, N_ ("_Help"), NULL, NULL, NULL, },
    { "contents", "help-browser", N_ ("_Contents"), "F1", N_ ("Display help contents"), G_CALLBACK (terminal_window_action_contents), },
    { "about", "help-about", N_ ("_About"), NULL, NULL, G_CALLBACK (terminal_window_action_about), },
  { "zoom-menu", NULL, N_ ("_Zoom"), NULL, NULL, NULL, },
};

static const GtkToggleActionEntry toggle_action_entries[] =
{
  { "show-menubar", NULL, N_ ("Show _Menubar"), NULL, N_ ("Show/hide the menubar"), G_CALLBACK (terminal_window_action_show_menubar), FALSE, },
  { "show-toolbar", NULL, N_ ("Show _Toolbar"), NULL, N_ ("Show/hide the toolbar"), G_CALLBACK (terminal_window_action_show_toolbar), FALSE, },
  { "show-borders", NULL, N_ ("Show Window _Borders"), NULL, N_ ("Show/hide the window decorations"), G_CALLBACK (terminal_window_action_show_borders), TRUE, },
  { "fullscreen", "view-fullscreen", N_ ("_Fullscreen"), "F11", N_ ("Toggle fullscreen mode"), G_CALLBACK (terminal_window_action_fullscreen), FALSE, },
  { "read-only", NULL, N_ ("_Read-Only"), NULL, N_ ("Toggle read-only mode"), G_CALLBACK (terminal_window_action_readonly), FALSE, },
};



G_DEFINE_TYPE (TerminalWindow, terminal_window, GTK_TYPE_WINDOW)



static void
terminal_window_class_init (TerminalWindowClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_window_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->window_state_event = terminal_window_state_event;
  gtkwidget_class->delete_event = terminal_window_delete_event;
  gtkwidget_class->style_set = terminal_window_style_set;
  gtkwidget_class->scroll_event = terminal_window_scroll_event;
  gtkwidget_class->key_press_event = terminal_window_key_press_event;

  /**
   * TerminalWindow::new-window
   **/
  window_signals[NEW_WINDOW] =
    g_signal_new (I_("new-window"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
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
                  0, NULL, NULL,
                  _terminal_marshal_VOID__OBJECT_INT_INT,
                  G_TYPE_NONE, 3,
                  G_TYPE_OBJECT,
                  G_TYPE_INT, G_TYPE_INT);

  g_type_class_add_private (gobject_class, sizeof (TerminalWindowPrivate));

  /* initialize quark */
  tabs_menu_action_quark = g_quark_from_static_string ("tabs-menu-item");
}



static void
terminal_window_init (TerminalWindow *window)
{
  GtkAccelGroup   *accel_group;
  gboolean         always_show_tabs;
  GdkScreen       *screen;
  GdkVisual       *visual;
  GtkStyleContext *context;

  window->priv = G_TYPE_INSTANCE_GET_PRIVATE (window, TERMINAL_TYPE_WINDOW, TerminalWindowPrivate);

  window->priv->preferences = terminal_preferences_get ();

  window->font = NULL;
  window->zoom = TERMINAL_ZOOM_LEVEL_DEFAULT;
  window->priv->closed_tabs_list = g_queue_new ();

  /* try to set the rgba colormap so vte can use real transparency */
  screen = gtk_window_get_screen (GTK_WINDOW (window));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual != NULL)
    gtk_widget_set_visual (GTK_WIDGET (window), visual);

  /* required for vte transparency support: see https://bugzilla.gnome.org/show_bug.cgi?id=729884 */
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

  window->priv->action_group = gtk_action_group_new ("terminal-window");
  gtk_action_group_set_translation_domain (window->priv->action_group,
                                           GETTEXT_PACKAGE);
  gtk_action_group_add_actions (window->priv->action_group,
                                action_entries,
                                G_N_ELEMENTS (action_entries),
                                GTK_WIDGET (window));
  gtk_action_group_add_toggle_actions (window->priv->action_group,
                                       toggle_action_entries,
                                       G_N_ELEMENTS (toggle_action_entries),
                                       GTK_WIDGET (window));

  window->priv->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (window->priv->ui_manager, window->priv->action_group, 0);
  gtk_ui_manager_add_ui_from_string (window->priv->ui_manager, terminal_window_ui, terminal_window_ui_length, NULL);

  accel_group = gtk_ui_manager_get_accel_group (window->priv->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  g_signal_connect_after (G_OBJECT (accel_group), "accel-activate",
      G_CALLBACK (terminal_window_accel_activate), window);

  window->priv->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (window), window->priv->vbox);

  /* avoid transparent widgets, such as menubar or tabbar */
  context = gtk_widget_get_style_context (window->priv->vbox);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_BACKGROUND);

  /* allocate the notebook for the terminal screens */
  g_object_get (G_OBJECT (window->priv->preferences), "misc-always-show-tabs", &always_show_tabs, NULL);
  window->priv->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                                   "scrollable", TRUE,
                                   "show-border", FALSE,
                                   "show-tabs", always_show_tabs,
                                   NULL);
  gtk_widget_add_events (window->priv->notebook, GDK_SCROLL_MASK);

  /* set the notebook group id */
  gtk_notebook_set_group_name (GTK_NOTEBOOK (window->priv->notebook), window_notebook_group);

  /* signals */
  g_signal_connect (G_OBJECT (window->priv->notebook), "switch-page",
      G_CALLBACK (terminal_window_notebook_page_switched), window);
  g_signal_connect (G_OBJECT (window->priv->notebook), "page-reordered",
      G_CALLBACK (terminal_window_notebook_page_reordered), window);
  g_signal_connect (G_OBJECT (window->priv->notebook), "page-removed",
      G_CALLBACK (terminal_window_notebook_page_removed), window);
  g_signal_connect (G_OBJECT (window->priv->notebook), "page-added",
      G_CALLBACK (terminal_window_notebook_page_added), window);
  g_signal_connect (G_OBJECT (window->priv->notebook), "create-window",
      G_CALLBACK (terminal_window_notebook_create_window), window);
  g_signal_connect (G_OBJECT (window->priv->notebook), "button-press-event",
      G_CALLBACK (terminal_window_notebook_button_press_event), window);
  g_signal_connect (G_OBJECT (window->priv->notebook), "button-release-event",
      G_CALLBACK (terminal_window_notebook_button_release_event), window);
  g_signal_connect (G_OBJECT (window->priv->notebook), "scroll-event",
      G_CALLBACK (terminal_window_notebook_scroll_event), window);

  gtk_box_pack_start (GTK_BOX (window->priv->vbox), window->priv->notebook, TRUE, TRUE, 0);
  gtk_widget_show_all (window->priv->vbox);

  /* create encoding action */
  window->priv->encoding_action = terminal_encoding_action_new ("set-encoding", _("Set _Encoding"));
  gtk_action_group_add_action (window->priv->action_group, window->priv->encoding_action);
  g_signal_connect (G_OBJECT (window->priv->encoding_action), "encoding-changed",
      G_CALLBACK (terminal_window_action_set_encoding), window);

  /* cache action pointers */
  window->priv->action_undo_close_tab = terminal_window_get_action (window, "undo-close-tab");
  window->priv->action_detach_tab = terminal_window_get_action (window, "detach-tab");
  window->priv->action_close_other_tabs = terminal_window_get_action (window, "close-other-tabs");
  window->priv->action_prev_tab = terminal_window_get_action (window, "prev-tab");
  window->priv->action_next_tab = terminal_window_get_action (window, "next-tab");
  window->priv->action_move_tab_left = terminal_window_get_action (window, "move-tab-left");
  window->priv->action_move_tab_right = terminal_window_get_action (window, "move-tab-right");
  window->priv->action_copy = terminal_window_get_action (window, "copy");
  window->priv->action_search_next = terminal_window_get_action (window, "search-next");
  window->priv->action_search_prev = terminal_window_get_action (window, "search-prev");
  window->priv->action_fullscreen = terminal_window_get_action (window, "fullscreen");

#if defined(GDK_WINDOWING_X11)
  if (GDK_IS_X11_SCREEN (screen))
    {
      /* setup fullscreen mode */
      if (!gdk_x11_screen_supports_net_wm_hint (screen, gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
        gtk_action_set_sensitive (window->priv->action_fullscreen, FALSE);
    }
#endif
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  g_object_unref (G_OBJECT (window->priv->preferences));
  g_object_unref (G_OBJECT (window->priv->action_group));
  g_object_unref (G_OBJECT (window->priv->ui_manager));
  g_object_unref (G_OBJECT (window->priv->encoding_action));

  g_slist_free (window->priv->tabs_menu_actions);
  g_free (window->font);
  g_queue_foreach (window->priv->closed_tabs_list, (GFunc) terminal_window_tab_info_free, NULL);
  g_queue_free (window->priv->closed_tabs_list);

  (*G_OBJECT_CLASS (terminal_window_parent_class)->finalize) (object);
}



static gboolean
terminal_window_delete_event (GtkWidget   *widget,
                              GdkEventAny *event)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);

  /* disconnect remove signal if we're closing the window */
  if (terminal_window_confirm_close (window))
    {
      /* avoid a lot of page remove calls */
      g_signal_handlers_disconnect_by_func (G_OBJECT (window->priv->notebook),
          G_CALLBACK (terminal_window_notebook_page_removed), window);

      /* let gtk close the window */
      if (GTK_WIDGET_CLASS (terminal_window_parent_class)->delete_event != NULL)
        return (*GTK_WIDGET_CLASS (terminal_window_parent_class)->delete_event) (widget, event);

      return FALSE;
    }

  return TRUE;
}



static gboolean
terminal_window_state_event (GtkWidget           *widget,
                             GdkEventWindowState *event)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);
  gboolean        fullscreen;

  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  /* update the fullscreen action if the fullscreen state changed by the wm */
  if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) != 0
      && gtk_widget_get_visible (widget))
    {
      fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
      if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (window->priv->action_fullscreen)) != fullscreen)
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (window->priv->action_fullscreen), fullscreen);
    }

  if (GTK_WIDGET_CLASS (terminal_window_parent_class)->window_state_event != NULL)
    return (*GTK_WIDGET_CLASS (terminal_window_parent_class)->window_state_event) (widget, event);

  return FALSE;
}



static void
terminal_window_style_set (GtkWidget *widget,
                           GtkStyle  *previous_style)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);

  if (previous_style != NULL)
    terminal_window_size_push (window);

  (*GTK_WIDGET_CLASS (terminal_window_parent_class)->style_set) (widget, previous_style);

  /* delay the pop until after size allocate */
  if (previous_style != NULL)
    g_idle_add (terminal_window_size_pop, window);
}



static gboolean
terminal_window_scroll_event (GtkWidget      *widget,
                              GdkEventScroll *event)
{
  gboolean mouse_wheel_zoom;
  TerminalWindow *window = TERMINAL_WINDOW (widget);

  g_object_get (G_OBJECT (window->priv->preferences),
                "misc-mouse-wheel-zoom", &mouse_wheel_zoom, NULL);

  if (mouse_wheel_zoom && event->state == (GDK_SHIFT_MASK | GDK_CONTROL_MASK)
      && event->direction == GDK_SCROLL_UP)
    {
      terminal_window_action_zoom_in (NULL, window);
      return TRUE;
    }

  if (mouse_wheel_zoom && event->state == (GDK_SHIFT_MASK | GDK_CONTROL_MASK)
      && event->direction == GDK_SCROLL_DOWN)
    {
      terminal_window_action_zoom_out (NULL, window);
      return TRUE;
    }

  return FALSE;
}



static gboolean
terminal_window_key_press_event (GtkWidget   *widget,
                                 GdkEventKey *event)
{
  GdkModifierType  mod;
  guint            key;
  gboolean         no_menukey;
  gchar           *menu_bar_accel;
  TerminalWindow  *window = TERMINAL_WINDOW (widget);

  g_object_get (G_OBJECT (window->priv->preferences),
                "shortcuts-no-menukey", &no_menukey,
                NULL);

  if (!no_menukey && terminal_window_get_menubar_height (window) == 0)
    {
      g_object_get (G_OBJECT (gtk_settings_get_default ()),
                    "gtk-menu-bar-accel", &menu_bar_accel,
                    NULL);
      gtk_accelerator_parse (menu_bar_accel, &key, &mod);
      g_free (menu_bar_accel);
      if (event->keyval == key && (event->state & gtk_accelerator_get_default_mod_mask ()) == mod)
        {
          terminal_window_size_push (window);
          gtk_widget_show (window->priv->menubar);
          terminal_window_size_pop (window);
        }
    }

  return (*GTK_WIDGET_CLASS (terminal_window_parent_class)->key_press_event) (widget, event);
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

  n_tabs = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
  if (G_UNLIKELY (n_tabs < 2))
    return TRUE;

  g_object_get (G_OBJECT (window->priv->preferences), "misc-confirm-close", &confirm_close, NULL);
  if (!confirm_close)
    return TRUE;

  dialog = gtk_dialog_new_with_buttons (_("Warning"), GTK_WINDOW (window),
                                        GTK_DIALOG_DESTROY_WITH_PARENT
                                        | GTK_DIALOG_MODAL,
                                        _("_Cancel"),
                                        GTK_RESPONSE_CANCEL,
                                        NULL);

  button = xfce_gtk_button_new_mixed ("window-close", _("Close T_ab"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_CLOSE);

  button = xfce_gtk_button_new_mixed ("application-exit", _("Close _Window"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_YES);
  gtk_widget_grab_focus (button);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), hbox, TRUE, TRUE, 0);

  image = gtk_image_new_from_icon_name ("dialog-warning", GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (image, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  message = g_strdup_printf (_("This window has %d tabs open. Closing this window\n"
                               "will also close all its tabs."), n_tabs);
  markup = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
                            _("Close all tabs?"), message);
  g_free (message);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", markup,
                        "use-markup", TRUE,
                        "xalign", 0.0,
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

  g_free (markup);

  checkbox = gtk_check_button_new_with_mnemonic (_("Do _not ask me again"));
  gtk_box_pack_start (GTK_BOX (vbox), checkbox, FALSE, FALSE, 0);

  gtk_widget_show_all (dialog);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_YES)
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox)))
        {
          g_object_set (G_OBJECT (window->priv->preferences),
                        "misc-confirm-close", FALSE,
                        NULL);
        }
    }
  else if (response == GTK_RESPONSE_CLOSE
           && window->priv->active != NULL)
    {
      gtk_widget_destroy (GTK_WIDGET (window->priv->active));
    }

  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_YES);
}



static void
terminal_window_size_push (TerminalWindow *window)
{
  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));

  if (window->priv->active != NULL)
    {
      terminal_screen_get_size (window->priv->active,
                                &window->priv->grid_width,
                                &window->priv->grid_height);
    }
}



static gboolean
terminal_window_size_pop (gpointer data)
{
  TerminalWindow *window = TERMINAL_WINDOW (data);

  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  if (window->priv->active != NULL)
    {
      terminal_window_set_size_force_grid (window, window->priv->active,
                                           window->priv->grid_width,
                                           window->priv->grid_height);
    }

  return FALSE;
}



static void
terminal_window_set_size_force_grid (TerminalWindow *window,
                                     TerminalScreen *screen,
                                     glong           force_grid_width,
                                     glong           force_grid_height)
{
  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  /* required to get the char height/width right */
  if (gtk_widget_get_realized (GTK_WIDGET (screen))
      && !window->drop_down)
    {
      terminal_screen_force_resize_window (screen, GTK_WINDOW (window),
                                           force_grid_width, force_grid_height);
    }
}



static gboolean
terminal_window_accel_activate (GtkAccelGroup   *accel_group,
                                GObject         *acceleratable,
                                guint            accel_key,
                                GdkModifierType  accel_mods,
                                TerminalWindow  *window)
{
  GtkAction   *actions[] = { window->priv->action_prev_tab, window->priv->action_next_tab };
  guint        n;
  GtkAccelKey  key;

  for (n = 0; n < G_N_ELEMENTS (actions); n++)
    {
      /* pretend we handled the accelerator if the event matches one of
       * the insensitive actions, so we don't send weird key events to vte
       * see http://bugzilla.xfce.org/show_bug.cgi?id=3715 */
      if (!gtk_action_is_sensitive (actions[n])
          && gtk_accel_map_lookup_entry (gtk_action_get_accel_path (actions[n]), &key)
          && key.accel_key == accel_key
          && key.accel_mods == accel_mods)
        return TRUE;
    }

  return FALSE;
}



static void
terminal_window_update_actions (TerminalWindow *window)
{
  GtkNotebook    *notebook = GTK_NOTEBOOK (window->priv->notebook);
  GtkAction      *action;
  gboolean        cycle_tabs;
  gint            page_num;
  gint            n_pages;
  gboolean        can_search;

  /* determine the number of pages */
  n_pages = gtk_notebook_get_n_pages (notebook);

  /* "Detach Tab", "Close Other Tabs" and move tab actions are only sensitive
   * if we have at least two pages */
  gtk_action_set_sensitive (window->priv->action_detach_tab, (n_pages > 1));
  gtk_action_set_sensitive (window->priv->action_close_other_tabs, n_pages > 1);
  gtk_action_set_sensitive (window->priv->action_move_tab_left, n_pages > 1);
  gtk_action_set_sensitive (window->priv->action_move_tab_right, n_pages > 1);

  gtk_action_set_sensitive (window->priv->action_undo_close_tab, !g_queue_is_empty (window->priv->closed_tabs_list));

  /* update the actions for the current terminal screen */
  if (G_LIKELY (window->priv->active != NULL))
    {
      page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (window->priv->active));

      g_object_get (G_OBJECT (window->priv->preferences),
                    "misc-cycle-tabs", &cycle_tabs,
                    NULL);

      gtk_action_set_sensitive (window->priv->action_prev_tab,
                                (cycle_tabs && n_pages > 1) || (page_num > 0));
      gtk_action_set_sensitive (window->priv->action_next_tab,
                                (cycle_tabs && n_pages > 1) || (page_num < n_pages - 1));

      gtk_action_set_sensitive (window->priv->action_copy,
                                terminal_screen_has_selection (window->priv->active));

      can_search = terminal_screen_search_has_gregex (window->priv->active);
      gtk_action_set_sensitive (window->priv->action_search_next, can_search);
      gtk_action_set_sensitive (window->priv->action_search_prev, can_search);

      /* update the "Go" menu */
      action = g_object_get_qdata (G_OBJECT (window->priv->active), tabs_menu_action_quark);
      if (G_LIKELY (action != NULL))
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
    }
}



static void
terminal_window_notebook_page_switched (GtkNotebook     *notebook,
                                        GtkWidget       *page,
                                        guint            page_num,
                                        TerminalWindow  *window)
{
  TerminalScreen *active;
  gboolean        was_null;
  const gchar    *encoding;

  /* get the new active page */
  active = TERMINAL_SCREEN (page);

  terminal_return_if_fail (window == NULL);
  terminal_return_if_fail (active == NULL || TERMINAL_IS_SCREEN (active));

  /* only update when really changed */
  if (G_LIKELY (window->priv->active != active))
    {
      /* check if we need to set the size or if this was already done
       * in the page add function */
      was_null = (window->priv->active == NULL);

      /* last active tab was closed; used by the undo close action to restore tab focus */
      if (!was_null &&
          gtk_notebook_page_num (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (window->priv->active)) == -1)
        window->priv->last_closed_active = window->priv->active;

      /* set new active tab */
      window->priv->active = active;

      /* set the new window title */
      terminal_window_notify_title (active, NULL, window);

      /* reset the activity counter */
      terminal_screen_reset_activity (active);

      /* set charset for menu */
      encoding = terminal_screen_get_encoding (window->priv->active);
      terminal_encoding_action_set_charset (window->priv->encoding_action, encoding);

      /* set the new geometry widget */
      if (G_LIKELY (!was_null))
        terminal_screen_set_window_geometry_hints (active, GTK_WINDOW (window));
    }

  /* update actions in the window */
  terminal_window_update_actions (window);
}



static void
terminal_window_notebook_page_reordered (GtkNotebook     *notebook,
                                         GtkWidget       *child,
                                         guint            page_num,
                                         TerminalWindow  *window)
{

  /* Regenerate the "Go" menu */
  terminal_window_rebuild_tabs_menu (window);
}



static void
terminal_window_notebook_page_added (GtkNotebook    *notebook,
                                     GtkWidget      *child,
                                     guint           page_num,
                                     TerminalWindow *window)
{
  TerminalScreen *screen = TERMINAL_SCREEN (child);
  glong           w, h;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (child));
  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  terminal_return_if_fail (GTK_IS_NOTEBOOK (notebook));
  terminal_return_if_fail (window->priv->notebook == GTK_WIDGET (notebook));

  /* connect screen signals */
  g_signal_connect (G_OBJECT (screen), "get-context-menu",
      G_CALLBACK (terminal_window_get_context_menu), window);
  g_signal_connect (G_OBJECT (screen), "notify::title",
      G_CALLBACK (terminal_window_notify_title), window);
  g_signal_connect_swapped (G_OBJECT (screen), "selection-changed",
      G_CALLBACK (terminal_window_update_actions), window);
  g_signal_connect (G_OBJECT (screen), "drag-data-received",
      G_CALLBACK (terminal_window_notebook_drag_data_received), window);

  /* release to the grid size applies */
  gtk_widget_realize (GTK_WIDGET (screen));

  if (G_LIKELY (window->priv->active != NULL))
    {
      /* match the size of the active screen */
      terminal_screen_get_size (window->priv->active, &w, &h);
      terminal_screen_set_size (screen, w, h);

      /* show the tabs when needed */
      terminal_window_notebook_show_tabs (window);
    }
  else if (G_UNLIKELY (window->drop_down))
    {
      /* try to calculate a decent grid size based on the info we have now */
      terminal_window_dropdown_get_size (TERMINAL_WINDOW_DROPDOWN (window), screen, &w, &h);
      terminal_screen_set_size (screen, w, h);
    }
  else
    {
      /* force a screen size, needed for misc-default-geometry */
      terminal_screen_get_size (screen, &w, &h);
      terminal_window_set_size_force_grid (window, screen, w, h);
    }

  /* regenerate the "Go" menu */
  terminal_window_rebuild_tabs_menu (window);
}



static void
terminal_window_notebook_page_removed (GtkNotebook    *notebook,
                                       GtkWidget      *child,
                                       guint           page_num,
                                       TerminalWindow *window)
{
  GtkWidget             *new_page;
  gint                   new_page_num;
  gint                   npages;
  TerminalWindowTabInfo *tab_info;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (child));
  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));

  /* unset the go menu item */
  g_object_set_qdata (G_OBJECT (child), tabs_menu_action_quark, NULL);

  /* disconnect signals */
  g_signal_handlers_disconnect_by_func (G_OBJECT (child),
      terminal_window_get_context_menu, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (child),
      terminal_window_notify_title, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (child),
      terminal_window_update_actions, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (child),
      terminal_window_notebook_drag_data_received, window);

  /* set tab visibility */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
  if (G_UNLIKELY (npages == 0))
    {
      /* no tabs, destroy the window */
      gtk_widget_destroy (GTK_WIDGET (window));
      return;
    }

  /* store info on the tab being closed */
  tab_info = g_new (TerminalWindowTabInfo, 1);
  tab_info->was_active = (TERMINAL_SCREEN (child) == window->priv->last_closed_active);
  tab_info->position = page_num;
  tab_info->working_directory = g_strdup (terminal_screen_get_working_directory (TERMINAL_SCREEN (child)));
  tab_info->custom_title = IS_STRING (terminal_screen_get_custom_title (TERMINAL_SCREEN (child))) ?
                           g_strdup (terminal_screen_get_custom_title (TERMINAL_SCREEN (child))) : NULL;
  g_queue_push_tail (window->priv->closed_tabs_list, tab_info);

  /* show the tabs when needed */
  terminal_window_notebook_show_tabs (window);

  /* regenerate the "Go" menu */
  terminal_window_rebuild_tabs_menu (window);

  /* send a signal about switching to another tab */
  new_page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->priv->notebook));
  new_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), new_page_num);
  terminal_window_notebook_page_switched (notebook, new_page, new_page_num, window);
}



static gboolean
terminal_window_notebook_event_in_allocation (gint       event_x,
                                              gint       event_y,
                                              GtkWidget *widget)
{
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);

  if (event_x >= allocation.x && event_x <= allocation.x + allocation.width
      && event_y >= allocation.y && event_y <= allocation.y + allocation.height)
    {
      return TRUE;
    }

  return FALSE;
}



static gboolean
terminal_window_notebook_button_press_event (GtkNotebook    *notebook,
                                             GdkEventButton *event,
                                             TerminalWindow *window)
{
  GtkWidget *page, *label, *menu;
  gint       page_num = 0;
  gboolean   close_middle_click;
  gint       x, y;

  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);
  terminal_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), FALSE);

  gdk_window_get_position (event->window, &x, &y);
  x += event->x;
  y += event->y;

  if (event->button == 1)
    {
      if (event->type == GDK_2BUTTON_PRESS)
        {
          /* check if the user double-clicked on the label */
          label = gtk_notebook_get_tab_label (notebook, GTK_WIDGET (window->priv->active));
          if (terminal_window_notebook_event_in_allocation (x, y, label))
            {
              terminal_window_action_set_title (NULL, window);
              return TRUE;
            }
        }
    }
  else if (event->type == GDK_BUTTON_PRESS && event->button <= 3)
    {
      /* select the page the user clicked on */
      while ((page = gtk_notebook_get_nth_page (notebook, page_num)) != NULL)
        {
          label = gtk_notebook_get_tab_label (notebook, page);
          if (terminal_window_notebook_event_in_allocation (x, y, label))
            break;
          page_num++;
        }

      /* leave if somehow no tab was found */
      if (G_UNLIKELY (page == NULL))
        return FALSE;

      if (event->button == 2)
        {
          /* close the tab on middle click */
          g_object_get (G_OBJECT (window->priv->preferences),
                        "misc-tab-close-middle-click", &close_middle_click, NULL);
          if (close_middle_click)
            gtk_widget_destroy (page);
        }
      else
        {
          /* update the current tab before we show the menu */
          gtk_notebook_set_current_page (notebook, page_num);

          /* show the tab menu */
          menu = gtk_ui_manager_get_widget (window->priv->ui_manager, "/tab-menu");
#if GTK_CHECK_VERSION (3, 22, 0)
          gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
#else
          gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
        }

      return TRUE;
    }

  return FALSE;
}



static gboolean
terminal_window_notebook_button_release_event (GtkNotebook    *notebook,
                                               GdkEventButton *event,
                                               TerminalWindow *window)
{
  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);
  terminal_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), FALSE);

  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_focus (window->priv->active);

  return FALSE;
}



static gboolean
terminal_window_notebook_scroll_event (GtkNotebook    *notebook,
                                       GdkEventScroll *event,
                                       TerminalWindow *window)
{
  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);
  terminal_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), FALSE);

  if ((event->state & gtk_accelerator_get_default_mod_mask ()) != 0)
    return FALSE;

  switch (event->direction) {
    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      gtk_notebook_next_page (notebook);
      return TRUE;

    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      gtk_notebook_prev_page (notebook);
      return TRUE;

    case GDK_SCROLL_SMOOTH:
      switch (gtk_notebook_get_tab_pos (notebook)) {
        case GTK_POS_LEFT:
        case GTK_POS_RIGHT:
          if (event->delta_y > 0)
            gtk_notebook_next_page (notebook);
          else if (event->delta_y < 0)
            gtk_notebook_prev_page (notebook);
          break;

        case GTK_POS_TOP:
        case GTK_POS_BOTTOM:
          if (event->delta_x > 0)
            gtk_notebook_next_page (notebook);
          else if (event->delta_x < 0)
            gtk_notebook_prev_page (notebook);
          break;
      }
      return TRUE;
  }

  return FALSE;
}



static void
terminal_window_notebook_drag_data_received (GtkWidget        *widget,
                                             GdkDragContext   *context,
                                             gint              x,
                                             gint              y,
                                             GtkSelectionData *selection_data,
                                             guint             info,
                                             guint             drag_time,
                                             TerminalWindow   *window)
{
  GtkWidget  *notebook;
  GtkWidget **screen;
  GtkWidget  *child, *label, *orig_window;
  gint        i, n_pages;
  gboolean    succeed = FALSE;
  GtkAllocation allocation;
  TerminalWindowTabInfo *tab_info;

  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (widget));

  /* check */
  if (G_LIKELY (info == TARGET_GTK_NOTEBOOK_TAB))
    {
      /* get the source notebook (other window) */
      notebook = gtk_drag_get_source_widget (context);
      terminal_return_if_fail (GTK_IS_NOTEBOOK (notebook));

      /* get the dragged screen */
      screen = (GtkWidget **) gtk_selection_data_get_data (selection_data);
      if (!TERMINAL_IS_SCREEN (*screen))
        goto leave;

      /* leave if we dropped in the same screen and there is only one
       * page in the notebook (window will close before we insert) */
      if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) < 2
          && *screen == widget)
        goto leave;

      /* figure out where to insert the tab in the notebook */
      n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
      for (i = 0; i < n_pages; i++)
        {
          /* get the child label */
          child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), i);
          label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (window->priv->notebook), child);
          gtk_widget_get_allocation (label, &allocation);

          /* break if we have a matching drop position */
          if (x < (allocation.x + allocation.width / 2))
            break;
        }

      if (notebook == window->priv->notebook)
        {
          /* if we're in the same notebook, don't risk anything and do a
           * simple reorder */
          gtk_notebook_reorder_child (GTK_NOTEBOOK (notebook), *screen, i);
        }
      else
        {
          /* take a reference */
          g_object_ref (G_OBJECT (*screen));
          g_object_ref (G_OBJECT (window));

          /* remove the document from the source notebook */
#if GTK_CHECK_VERSION (3,16,0)
          gtk_notebook_detach_tab (GTK_NOTEBOOK (notebook), *screen);
#else
          gtk_container_remove (GTK_CONTAINER (notebook), *screen);
#endif

          /* add the screen to the new window */
          terminal_window_add (window, TERMINAL_SCREEN (*screen));

          /* move the child to the correct position */
          gtk_notebook_reorder_child (GTK_NOTEBOOK (window->priv->notebook), *screen, i);

          /* release reference */
          g_object_unref (G_OBJECT (*screen));
          g_object_unref (G_OBJECT (window));

          /* erase last closed tabs entry from the original window as we don't want it on DND;
           * if it's not a window, it means the last tab was dropped here - don't do anything */
          orig_window = gtk_widget_get_toplevel (notebook);
          if (TERMINAL_IS_WINDOW (orig_window))
            {
              tab_info = g_queue_pop_tail (TERMINAL_WINDOW (orig_window)->priv->closed_tabs_list);
              terminal_window_tab_info_free (tab_info);
              /* update actions to make the undo action inactive */
              terminal_window_update_actions (TERMINAL_WINDOW (orig_window));
            }
        }

      /* looks like everything worked out */
      succeed = TRUE;
    }

  /* finish the drag */
leave:
  gtk_drag_finish (context, succeed, FALSE, drag_time);
}



static GtkNotebook *
terminal_window_notebook_create_window (GtkNotebook    *notebook,
                                        GtkWidget      *child,
                                        gint            x,
                                        gint            y,
                                        TerminalWindow *window)
{
  TerminalScreen        *screen;
  TerminalWindowTabInfo *tab_info;

  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (child), NULL);
  terminal_return_val_if_fail (notebook == GTK_NOTEBOOK (window->priv->notebook), NULL);

  /* only create new window when there are more then 2 tabs (bug #2686) */
  if (gtk_notebook_get_n_pages (notebook) >= 2)
    {
      /* get the screen */
      screen = TERMINAL_SCREEN (child);

      /* take a reference */
      g_object_ref (G_OBJECT (screen));

      /* remove screen from active window */
      #if GTK_CHECK_VERSION (3,16,0)
        gtk_notebook_detach_tab (notebook, child);
      #else
        gtk_container_remove (GTK_CONTAINER (notebook), child);
      #endif

      /* create new window with the screen */
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW_WITH_SCREEN], 0, screen, x, y);

      /* release our reference */
      g_object_unref (G_OBJECT (screen));

      /* erase last closed tabs entry as we don't want it on detach */
      tab_info = g_queue_pop_tail (window->priv->closed_tabs_list);
      terminal_window_tab_info_free (tab_info);
      /* and update action to make the undo action inactive */
      terminal_window_update_actions (window);
    }

  return NULL;
}



static GtkWidget *
terminal_window_get_context_menu (TerminalScreen  *screen,
                                  TerminalWindow  *window)
{
  GtkWidget *popup = NULL;

  if (G_LIKELY (screen == window->priv->active))
    popup = gtk_ui_manager_get_widget (window->priv->ui_manager, "/popup-menu");

  return popup;
}



static void
terminal_window_notify_title (TerminalScreen *screen,
                              GParamSpec     *pspec,
                              TerminalWindow *window)
{
  gchar *title;

  /* update window title */
  if (screen == window->priv->active)
    {
      title = terminal_screen_get_title (window->priv->active);
      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);
    }
}



static void
terminal_window_action_set_encoding (GtkAction      *action,
                                     const gchar    *charset,
                                     TerminalWindow *window)
{
  const gchar *new;

  if (G_LIKELY (window->priv->active != NULL))
    {
      /* set the charset */
      terminal_screen_set_encoding (window->priv->active, charset);

      /* update menu */
      new = terminal_screen_get_encoding (window->priv->active);
      terminal_encoding_action_set_charset (action, new);
    }
}



static void
terminal_window_action_new_tab (GtkAction      *action,
                                TerminalWindow *window)
{
  const gchar    *directory = NULL;
  gchar          *default_dir;
  TerminalScreen *terminal;

  terminal = TERMINAL_SCREEN (g_object_new (TERMINAL_TYPE_SCREEN, NULL));
  g_object_get (G_OBJECT (window->priv->preferences), "misc-default-working-dir", &default_dir, NULL);

  if (g_strcmp0 (default_dir, "") != 0)
    directory = default_dir;
  else if (G_LIKELY (window->priv->active != NULL))
    directory = terminal_screen_get_working_directory (window->priv->active);

  if (directory != NULL)
    terminal_screen_set_working_directory (terminal, directory);

  g_free (default_dir);

  terminal_window_add (window, terminal);
  terminal_screen_launch_child (terminal);
}



static void
terminal_window_action_new_window (GtkAction      *action,
                                   TerminalWindow *window)
{
  const gchar *directory = NULL;
  gchar       *default_dir;

  g_object_get (G_OBJECT (window->priv->preferences), "misc-default-working-dir", &default_dir, NULL);

  if (g_strcmp0 (default_dir, "") != 0)
    directory = default_dir;
  else if (G_LIKELY (window->priv->active != NULL))
    directory = terminal_screen_get_working_directory (window->priv->active);

  if (directory != NULL)
    g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW], 0, directory);

  g_free (default_dir);
}



static void
terminal_window_action_undo_close_tab (GtkAction      *action,
                                       TerminalWindow *window)
{
  TerminalScreen        *terminal;
  TerminalWindowTabInfo *tab_info;
  gint                   current = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->priv->notebook));

  terminal = TERMINAL_SCREEN (g_object_new (TERMINAL_TYPE_SCREEN, NULL));
  terminal_window_add (window, terminal);

  if (G_LIKELY (!g_queue_is_empty (window->priv->closed_tabs_list)))
    {
      /* get info on the last closed tab and remove it from the list */
      tab_info = g_queue_pop_tail (window->priv->closed_tabs_list);

      /* set info to the new tab */
      terminal_screen_set_working_directory (terminal, tab_info->working_directory);
      if (tab_info->custom_title != NULL)
        terminal_screen_set_custom_title (terminal, tab_info->custom_title);
      gtk_notebook_reorder_child (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (terminal), tab_info->position);

      /* restore tab focus if the unclosed one wasn't active when it was closed */
      if (!tab_info->was_active)
        gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook), current);

      /* free info */
      terminal_window_tab_info_free (tab_info);
    }

  terminal_screen_launch_child (terminal);
}



static void
terminal_window_action_detach_tab (GtkAction      *action,
                                   TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_window_notebook_create_window (GTK_NOTEBOOK (window->priv->notebook),
                                            GTK_WIDGET (window->priv->active),
                                            -1, -1, window);
}



static void
terminal_window_action_close_tab (GtkAction      *action,
                                  TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    gtk_widget_destroy (GTK_WIDGET (window->priv->active));
}



static void
terminal_window_action_close_other_tabs (GtkAction      *action,
                                         TerminalWindow *window)
{
    gint         npages, n;
    GtkWidget   *child;
    GtkNotebook *notebook = GTK_NOTEBOOK (window->priv->notebook);

    npages = gtk_notebook_get_n_pages (notebook);
    child = gtk_notebook_get_nth_page (notebook,
                                       gtk_notebook_get_current_page (notebook));
    /* move current page to the beginning */
    gtk_notebook_reorder_child (notebook, child, 0);
    /* remove the others */
    for (n = npages - 1; n > 0; n--)
      gtk_notebook_remove_page (notebook, n);
}



static void
terminal_window_action_close_window (GtkAction      *action,
                                     TerminalWindow *window)
{
  if (terminal_window_confirm_close (window))
    gtk_widget_destroy (GTK_WIDGET (window));
}



static void
terminal_window_action_copy (GtkAction      *action,
                             TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_copy_clipboard (window->priv->active);
}



static void
terminal_window_action_paste (GtkAction      *action,
                              TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_paste_clipboard (window->priv->active);
}



static void
terminal_window_action_paste_selection (GtkAction      *action,
                                        TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_paste_primary (window->priv->active);
}



static void
terminal_window_action_select_all (GtkAction      *action,
                                   TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_select_all (window->priv->active);
}



static void
terminal_window_action_prefs_died (gpointer  user_data,
                                   GObject  *where_the_object_was)
{
  TerminalWindow *window = TERMINAL_WINDOW (user_data);

  window->priv->preferences_dialog = NULL;
  window->priv->n_child_windows--;

  if (window->drop_down)
    terminal_util_activate_window (GTK_WINDOW (window));
}



static void
terminal_window_action_prefs (GtkAction      *action,
                              TerminalWindow *window)
{
  if (window->priv->preferences_dialog == NULL)
    {
      window->priv->preferences_dialog = terminal_preferences_dialog_new (window->drop_down);
      if (G_LIKELY (window->priv->preferences_dialog != NULL))
        {
          window->priv->n_child_windows++;
          g_object_weak_ref (G_OBJECT (window->priv->preferences_dialog),
                             terminal_window_action_prefs_died, window);
        }
    }

  if (window->priv->preferences_dialog != NULL)
    {
      gtk_window_set_transient_for (GTK_WINDOW (window->priv->preferences_dialog), GTK_WINDOW (window));
      gtk_window_present (GTK_WINDOW (window->priv->preferences_dialog));
    }
}



static void
terminal_window_action_show_menubar (GtkToggleAction *action,
                                     TerminalWindow  *window)
{
  terminal_window_size_push (window);

  if (gtk_toggle_action_get_active (action))
    gtk_widget_show (window->priv->menubar);
  else
    gtk_widget_hide (window->priv->menubar);

  terminal_window_size_pop (window);
}



static void
terminal_window_action_show_toolbar (GtkToggleAction *action,
                                     TerminalWindow  *window)
{
  terminal_return_if_fail (GTK_IS_UI_MANAGER (window->priv->ui_manager));
  terminal_return_if_fail (GTK_IS_ACTION_GROUP (window->priv->action_group));

  terminal_window_size_push (window);

  if (gtk_toggle_action_get_active (action))
    {
      if (window->priv->toolbar == NULL)
        {
          window->priv->toolbar = gtk_ui_manager_get_widget (window->priv->ui_manager, "/main-toolbar");
          gtk_box_pack_start (GTK_BOX (window->priv->vbox), window->priv->toolbar, FALSE, FALSE, 0);
          gtk_box_reorder_child (GTK_BOX (window->priv->vbox),
                                 window->priv->toolbar,
                                 window->priv->menubar != NULL ? 1 : 0);
        }

      gtk_widget_show (window->priv->toolbar);
    }
  else if (window->priv->toolbar != NULL)
    {
      gtk_widget_hide (window->priv->toolbar);
    }

  terminal_window_size_pop (window);
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
terminal_window_action_readonly (GtkToggleAction *action,
                                 TerminalWindow  *window)
{
  gboolean input_enabled;

  terminal_return_if_fail (window->priv->active != NULL);

  input_enabled = !gtk_toggle_action_get_active (action);
  gtk_action_set_sensitive (terminal_window_get_action (window, "reset"), input_enabled);
  gtk_action_set_sensitive (terminal_window_get_action (window, "reset-and-clear"), input_enabled);
  terminal_screen_set_input_enabled (window->priv->active, input_enabled);
}



static void
terminal_window_action_zoom_in (GtkAction     *action,
                               TerminalWindow *window)
{
  terminal_return_if_fail (window->priv->active != NULL);

  if (window->zoom < TERMINAL_ZOOM_LEVEL_MAXIMUM)
    {
      ++window->zoom;
      terminal_window_zoom_update_screens (window);
    }
}



static void
terminal_window_action_zoom_out (GtkAction      *action,
                                 TerminalWindow *window)
{
  terminal_return_if_fail (window->priv->active != NULL);

  if (window->zoom > TERMINAL_ZOOM_LEVEL_MINIMUM)
    {
      --window->zoom;
      terminal_window_zoom_update_screens (window);
    }
}



static void
terminal_window_action_zoom_reset (GtkAction      *action,
                                   TerminalWindow *window)
{
  terminal_return_if_fail (window->priv->active != NULL);

  if (window->zoom != TERMINAL_ZOOM_LEVEL_DEFAULT)
    {
      window->zoom = TERMINAL_ZOOM_LEVEL_DEFAULT;
      terminal_window_zoom_update_screens (window);
    }
}



static void
terminal_window_action_prev_tab (GtkAction       *action,
                                 TerminalWindow  *window)
{
  terminal_window_switch_tab (GTK_NOTEBOOK (window->priv->notebook), TRUE);
}



static void
terminal_window_action_next_tab (GtkAction       *action,
                                 TerminalWindow  *window)
{
  terminal_window_switch_tab (GTK_NOTEBOOK (window->priv->notebook), FALSE);
}



static void
terminal_window_action_move_tab_left (GtkAction      *action,
                                      TerminalWindow *window)
{
  terminal_window_move_tab (GTK_NOTEBOOK (window->priv->notebook), TRUE);
}



static void
terminal_window_action_move_tab_right (GtkAction      *action,
                                       TerminalWindow *window)
{
  terminal_window_move_tab (GTK_NOTEBOOK (window->priv->notebook), FALSE);
}



static void
terminal_window_action_goto_tab (GtkRadioAction *action,
                                 GtkNotebook    *notebook)
{
  gint page;

  terminal_return_if_fail (GTK_IS_NOTEBOOK (notebook));
  terminal_return_if_fail (GTK_IS_RADIO_ACTION (action));

  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    {
      /* switch to the new page */
      page = gtk_radio_action_get_current_value (action);
      gtk_notebook_set_current_page (notebook, page);
    }
}



static void
title_dialog_close (GtkWidget      *dialog,
                    TerminalWindow *window)
{
  terminal_return_if_fail (window->priv->title_dialog == dialog);

  /* need for hiding on focus */
  if (window->drop_down)
    terminal_util_activate_window (GTK_WINDOW (window));

  /* close the dialog */
  window->priv->n_child_windows--;
  gtk_widget_destroy (dialog);
  window->priv->title_dialog = NULL;
}



static void
title_dialog_response (GtkWidget      *dialog,
                       gint            response,
                       TerminalWindow *window)
{
  if (response == GTK_RESPONSE_CLOSE)
    title_dialog_close (dialog, window);
}



static void
title_dialog_clear (GtkWidget            *entry,
                    GtkEntryIconPosition  icon_pos)
{
  if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
    gtk_entry_set_text (GTK_ENTRY (entry), "");
}



static void
terminal_window_action_set_title (GtkAction      *action,
                                  TerminalWindow *window)
{
  AtkObject *object;
  GtkWidget *button;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *entry;

  terminal_return_if_fail (window->priv->active != NULL);

  if (window->priv->title_dialog == NULL)
    {
      window->priv->title_dialog =
          gtk_dialog_new_with_buttons (Q_("Window Title|Set Title"),
                                       GTK_WINDOW (window),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       NULL,
                                       NULL);

      /* set window height to minimum to fix huge size under wayland */
      gtk_window_set_default_size (GTK_WINDOW (window->priv->title_dialog), -1, 1);

      button = xfce_gtk_button_new_mixed ("window-close", _("_Close"));
      gtk_widget_set_can_default (button, TRUE);
      gtk_dialog_add_action_widget (GTK_DIALOG (window->priv->title_dialog), button, GTK_RESPONSE_CLOSE);
      gtk_dialog_set_default_response (GTK_DIALOG (window->priv->title_dialog), GTK_RESPONSE_CLOSE);

      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (box), 6);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (window->priv->title_dialog))),
                          box, TRUE, TRUE, 0);

      label = gtk_label_new_with_mnemonic (_("_Title:"));
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);

      entry = gtk_entry_new ();
      gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
      gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, "edit-clear");
      g_signal_connect (G_OBJECT (entry), "icon-release", G_CALLBACK (title_dialog_clear), NULL);

      /* set Atk description and label relation for the entry */
      object = gtk_widget_get_accessible (entry);
      atk_object_set_description (object, _("Enter the title for the current terminal tab"));

      g_object_bind_property (G_OBJECT (window->priv->active), "custom-title",
                              G_OBJECT (entry), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

      g_signal_connect (G_OBJECT (window->priv->title_dialog), "response",
                        G_CALLBACK (title_dialog_response), window);
      g_signal_connect (G_OBJECT (window->priv->title_dialog), "close",
                        G_CALLBACK (title_dialog_close), window);
    }

    if (!gtk_widget_get_visible (window->priv->title_dialog))
      window->priv->n_child_windows++;

    gtk_widget_show_all (window->priv->title_dialog);
    gtk_window_present (GTK_WINDOW (window->priv->title_dialog));
}



static void
terminal_window_action_search_response (GtkWidget      *dialog,
                                        gint            response_id,
                                        TerminalWindow *window)
{
  GRegex   *regex;
  GError   *error = NULL;
  gboolean  wrap_around;
  gboolean  can_search;

  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  terminal_return_if_fail (TERMINAL_IS_SEARCH_DIALOG (dialog));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (window->priv->active));
  terminal_return_if_fail (window->priv->search_dialog == dialog);

  if (response_id == TERMINAL_RESPONSE_SEARCH_NEXT
      || response_id == TERMINAL_RESPONSE_SEARCH_PREV)
    {
      regex = terminal_search_dialog_get_regex (TERMINAL_SEARCH_DIALOG (dialog), &error);
      if (G_LIKELY (error == NULL))
        {
          wrap_around = terminal_search_dialog_get_wrap_around (TERMINAL_SEARCH_DIALOG (dialog));
          terminal_screen_search_set_gregex (window->priv->active, regex, wrap_around);
          if (regex != NULL)
            g_regex_unref (regex);

          if (response_id == TERMINAL_RESPONSE_SEARCH_NEXT)
            terminal_screen_search_find_next (window->priv->active);
          else
            terminal_screen_search_find_previous (window->priv->active);
        }
      else
        {
          xfce_dialog_show_error (GTK_WINDOW (dialog), error, _("Failed to create the regular expression"));
          g_error_free (error);
        }
    }
  else
    {
      /* need for hiding on focus */
      if (window->drop_down)
        terminal_util_activate_window (GTK_WINDOW (window));

      /* hide dialog */
      window->priv->n_child_windows--;
      gtk_widget_hide (dialog);
    }

  /* update actions */
  can_search = terminal_screen_search_has_gregex (window->priv->active);
  gtk_action_set_sensitive (window->priv->action_search_next, can_search);
  gtk_action_set_sensitive (window->priv->action_search_prev, can_search);
}



static void
terminal_window_action_search (GtkAction      *action,
                               TerminalWindow *window)
{
  if (window->priv->search_dialog == NULL)
    {
      window->priv->search_dialog = terminal_search_dialog_new (GTK_WINDOW (window));
      g_signal_connect (G_OBJECT (window->priv->search_dialog), "response",
          G_CALLBACK (terminal_window_action_search_response), window);
      g_signal_connect (G_OBJECT (window->priv->search_dialog), "delete-event",
          G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    }

  /* increase child counter */
  if (!gtk_widget_get_visible (window->priv->search_dialog))
    window->priv->n_child_windows++;

  terminal_search_dialog_present (TERMINAL_SEARCH_DIALOG (window->priv->search_dialog));
}



static void
terminal_window_action_search_next (GtkAction      *action,
                                    TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_search_find_next (window->priv->active);
}



static void
terminal_window_action_search_prev (GtkAction      *action,
                                    TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_search_find_previous (window->priv->active);
}



static void
terminal_window_action_save_contents (GtkAction      *action,
                                      TerminalWindow *window)
{
  GtkWidget     *dialog;
  GFile         *file;
  GOutputStream *stream;
  GError        *error = NULL;
  gchar         *filename_uri;
  gint           response;

  terminal_return_if_fail (window->priv->active != NULL);

  dialog = gtk_file_chooser_dialog_new (_("Save contents..."),
                                        GTK_WINDOW (window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Save"), GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

  /* save to current working directory */
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                       terminal_screen_get_working_directory (TERMINAL_SCREEN (window->priv->active)));

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  gtk_widget_show_all (dialog);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response != GTK_RESPONSE_ACCEPT)
    {
      gtk_widget_destroy (dialog);
      return;
    }

  filename_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
  gtk_widget_destroy (dialog);

  if (filename_uri == NULL)
    return;

  file = g_file_new_for_uri (filename_uri);
  stream = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error));
  if (stream)
    {
      terminal_screen_save_contents (TERMINAL_SCREEN (window->priv->active), stream, error);
      g_object_unref (stream);
    }

  if (error)
    {
      xfce_dialog_show_error (GTK_WINDOW (window), error, _("Failed to save terminal contents"));
      g_error_free (error);
    }

  g_object_unref (file);
  g_free (filename_uri);
}



static void
terminal_window_action_reset (GtkAction      *action,
                              TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_reset (window->priv->active, FALSE);
}



static void
terminal_window_action_reset_and_clear (GtkAction       *action,
                                        TerminalWindow  *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    {
      terminal_screen_reset (window->priv->active, TRUE);
      terminal_window_update_actions (window);
    }
}



static void
terminal_window_action_contents (GtkAction       *action,
                                 TerminalWindow  *window)
{
  /* open the Terminal user manual */
  xfce_dialog_show_help (GTK_WINDOW (window), "terminal", NULL, NULL);
}



static void
terminal_window_action_about (GtkAction      *action,
                              TerminalWindow *window)
{
  /* display the about dialog */
  terminal_util_show_about_dialog (GTK_WINDOW (window));
}



static void
terminal_window_zoom_update_screens (TerminalWindow *window)
{
  gint            npages, n;
  TerminalScreen *screen;
  GtkAction      *action;

  terminal_return_if_fail (GTK_IS_NOTEBOOK (window->priv->notebook));

  /* walk the tabs */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
  for (n = 0; n < npages; n++)
    {
      screen = TERMINAL_SCREEN (gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), n));
      terminal_screen_update_font (screen);
    }

  /* update zoom actions */
  action = terminal_window_get_action (window, "zoom-in");
  if (window->zoom == TERMINAL_ZOOM_LEVEL_MAXIMUM)
    gtk_action_set_sensitive (action, FALSE);
  else if (!gtk_action_is_sensitive (action))
    gtk_action_set_sensitive (action, TRUE);

  action = terminal_window_get_action (window, "zoom-out");
  if (window->zoom == TERMINAL_ZOOM_LEVEL_MINIMUM)
      gtk_action_set_sensitive (action, FALSE);
    else if (!gtk_action_is_sensitive (action))
      gtk_action_set_sensitive (action, TRUE);
}



static void
terminal_window_switch_tab (GtkNotebook *notebook,
                            gboolean     switch_left)
{
  gint page_num;
  gint n_pages;

  page_num = gtk_notebook_get_current_page (notebook);
  n_pages = gtk_notebook_get_n_pages (notebook);
  gtk_notebook_set_current_page (notebook,
                                 (switch_left ? page_num - 1 : page_num + 1) % n_pages);
}



static void
terminal_window_move_tab (GtkNotebook *notebook,
                          gboolean     move_left)
{
  gint       page_num;
  gint       last_page;
  GtkWidget *page;

  page_num = gtk_notebook_get_current_page (notebook);
  last_page = gtk_notebook_get_n_pages (notebook) - 1;
  page = gtk_notebook_get_nth_page (notebook, page_num);

  if (move_left)
    gtk_notebook_reorder_child (notebook, page,
                                page_num == 0 ? last_page : page_num - 1);
  else
    gtk_notebook_reorder_child (notebook, page,
                                page_num == last_page ? 0 : page_num + 1);
}



static void
terminal_window_tab_info_free (TerminalWindowTabInfo *tab_info)
{
  g_free (tab_info->custom_title);
  g_free (tab_info->working_directory);
  g_free (tab_info);
}



static void
terminal_window_menubar_deactivate (GtkWidget      *widget,
                                    TerminalWindow *window)
{
  GtkAction *action;

  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));

  action = terminal_window_get_action (window, "show-menubar");
  terminal_window_action_show_menubar (GTK_TOGGLE_ACTION (action), window);
}



/**
 * terminal_window_new:
 * @fullscreen: Whether to set the window to fullscreen.
 * @menubar   : Visibility setting for the menubar.
 * @borders   : Visibility setting for the window borders.
 * @toolbar   : Visibility setting for the toolbar.
 *
 * Return value:
 **/
GtkWidget*
terminal_window_new (const gchar       *role,
                     gboolean           fullscreen,
                     TerminalVisibility menubar,
                     TerminalVisibility borders,
                     TerminalVisibility toolbar)
{
  TerminalWindow *window;
  GtkAction      *action;
  gboolean        show_menubar;
  gboolean        show_toolbar;
  gboolean        show_borders;

  window = g_object_new (TERMINAL_TYPE_WINDOW, "role", role, NULL);

  /* read default preferences */
  g_object_get (G_OBJECT (window->priv->preferences),
                "misc-menubar-default", &show_menubar,
                "misc-toolbar-default", &show_toolbar,
                "misc-borders-default", &show_borders,
                NULL);

  /* setup full screen */
  if (fullscreen && gtk_action_is_sensitive (window->priv->action_fullscreen))
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (window->priv->action_fullscreen), TRUE);

  window->priv->menubar = gtk_ui_manager_get_widget (window->priv->ui_manager, "/main-menu");
  gtk_box_pack_start (GTK_BOX (window->priv->vbox), window->priv->menubar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (window->priv->vbox), window->priv->menubar, 0);
  /* auto-hide menubar if it was shown temporarily */
  g_signal_connect (G_OBJECT (window->priv->menubar), "deactivate",
      G_CALLBACK (terminal_window_menubar_deactivate), window);

  /* setup menubar visibility */
  if (G_LIKELY (menubar != TERMINAL_VISIBILITY_DEFAULT))
    show_menubar = (menubar == TERMINAL_VISIBILITY_SHOW);
  action = terminal_window_get_action (window, "show-menubar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show_menubar);
  terminal_window_action_show_menubar (GTK_TOGGLE_ACTION (action), window);

  /* setup toolbar visibility */
  if (G_LIKELY (toolbar != TERMINAL_VISIBILITY_DEFAULT))
    show_toolbar = (toolbar == TERMINAL_VISIBILITY_SHOW);
  action = terminal_window_get_action (window, "show-toolbar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show_toolbar);

  /* setup borders visibility */
  if (G_LIKELY (borders != TERMINAL_VISIBILITY_DEFAULT))
    show_borders = (borders == TERMINAL_VISIBILITY_SHOW);
  action = terminal_window_get_action (window, "show-borders");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show_borders);

  /* property that is not suitable for init */
  g_object_bind_property (G_OBJECT (window->priv->preferences), "misc-tab-position",
                          G_OBJECT (window->priv->notebook), "tab-pos",
                          G_BINDING_SYNC_CREATE);

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
  GtkWidget  *label;
  gint        page;

  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  /* create the tab label */
  label = terminal_screen_get_tab_label (screen);

  page = gtk_notebook_append_page (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (screen), label);
  gtk_container_child_set (GTK_CONTAINER (window->priv->notebook), GTK_WIDGET (screen), "tab-expand", TRUE, NULL);
  gtk_container_child_set (GTK_CONTAINER (window->priv->notebook), GTK_WIDGET (screen), "tab-fill", TRUE, NULL);

  /* allow tab sorting and dnd */
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (screen), TRUE);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (screen), TRUE);

  /* update scrollbar visibility */
  if (window->priv->scrollbar_visibility != TERMINAL_VISIBILITY_DEFAULT)
    terminal_screen_update_scrolling_bar (screen);

  /* update screen font from window */
  if (window->font || window->zoom != TERMINAL_ZOOM_LEVEL_DEFAULT)
    terminal_screen_update_font (screen);

  /* show the terminal screen */
  gtk_widget_realize (GTK_WIDGET (screen));
  gtk_widget_show (GTK_WIDGET (screen));

  /* switch to the new tab */
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook), page);

  /* focus the terminal */
  terminal_screen_focus (screen);
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
TerminalScreen *
terminal_window_get_active (TerminalWindow *window)
{
  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);
  return window->priv->active;
}



void
terminal_window_notebook_show_tabs (TerminalWindow *window)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (window->priv->notebook);
  gboolean     show_tabs = TRUE;
  gint         npages;

  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));

  /* check preferences */
  npages = gtk_notebook_get_n_pages (notebook);
  if (npages < 2)
    {
      g_object_get (G_OBJECT (window->priv->preferences),
                    window->drop_down ? "dropdown-always-show-tabs" :
                    "misc-always-show-tabs", &show_tabs, NULL);
    }

  /* set the visibility of the tabs */
  if (gtk_notebook_get_show_tabs (notebook) != show_tabs)
    {
      /* store size */
      terminal_window_size_push (window);

      /* show or hide the tabs */
      gtk_notebook_set_show_tabs (notebook, show_tabs);

      /* update the window geometry */
      terminal_window_size_pop (window);
    }
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
GSList*
terminal_window_get_restart_command (TerminalWindow *window)
{
  const gchar *role;
  GtkAction   *action;
  GdkScreen   *gscreen;
  GList       *children, *lp;
  GSList      *result = NULL;
  glong        w;
  glong        h;

  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);

  if (G_LIKELY (window->priv->active != NULL))
    {
      terminal_screen_get_size (window->priv->active, &w, &h);
      result = g_slist_prepend (result, g_strdup_printf ("--geometry=%ldx%ld", w, h));
    }

  gscreen = gtk_window_get_screen (GTK_WINDOW (window));
  if (G_LIKELY (gscreen != NULL))
    {
      result = g_slist_prepend (result, g_strdup ("--display"));
      result = g_slist_prepend (result, gdk_screen_make_display_name (gscreen));
    }

  role = gtk_window_get_role (GTK_WINDOW (window));
  if (G_LIKELY (role != NULL))
    result = g_slist_prepend (result, g_strdup_printf ("--role=%s", role));

  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (window->priv->action_fullscreen)))
    result = g_slist_prepend (result, g_strdup ("--fullscreen"));

  action = terminal_window_get_action (window, "show-menubar");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_slist_prepend (result, g_strdup ("--show-menubar"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-menubar"));

  action = terminal_window_get_action (window, "show-borders");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_slist_prepend (result, g_strdup ("--show-borders"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-borders"));

  action = terminal_window_get_action (window, "show-toolbar");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_slist_prepend (result, g_strdup ("--show-toolbar"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-toolbar"));

  if (window->zoom != TERMINAL_ZOOM_LEVEL_DEFAULT)
    result = g_slist_prepend (result, g_strdup_printf ("--zoom=%d", window->zoom));
  if (window->font != NULL)
    result = g_slist_prepend (result, g_strdup_printf ("--font=%s", window->font));

  /* set restart commands of the tabs */
  children = gtk_container_get_children (GTK_CONTAINER (window->priv->notebook));
  for (lp = children; lp != NULL; lp = lp->next)
    {
      if (lp != children)
        result = g_slist_prepend (result, g_strdup ("--tab"));
      result = g_slist_concat (terminal_screen_get_restart_command (lp->data), result);
    }
  g_list_free (children);

  return g_slist_reverse (result);
}


/**
 * terminal_window_set_grid_size:
 * @window  : A #TerminalWindow.
 * @width   : Window width.
 * @height  : Window height.
 **/
void
terminal_window_set_grid_size (TerminalWindow *window,
                               glong           width,
                               glong           height)
{
  window->priv->grid_width = width;
  window->priv->grid_height = height;
}



/**
 * terminal_window_has_children:
 * @window  : A #TerminalWindow.
 **/
gboolean
terminal_window_has_children (TerminalWindow *window)
{
  return window->priv->n_child_windows != 0;
}



/**
 * terminal_window_get_preferences:
 * @window  : A #TerminalWindow.
 **/
GObject*
terminal_window_get_preferences (TerminalWindow *window)
{
  return G_OBJECT (window->priv->preferences);
}



/**
 * terminal_window_get_vbox:
 * @window  : A #TerminalWindow.
 **/
GtkWidget*
terminal_window_get_vbox (TerminalWindow *window)
{
  return window->priv->vbox;
}



/**
 * terminal_window_get_notebook:
 * @window  : A #TerminalWindow.
 **/
GtkWidget*
terminal_window_get_notebook (TerminalWindow *window)
{
  return window->priv->notebook;
}



/**
 * terminal_window_get_preferences_dialog:
 * @window  : A #TerminalWindow.
 **/
GtkWidget*
terminal_window_get_preferences_dialog (TerminalWindow *window)
{
  return window->priv->preferences_dialog;
}



/**
 * terminal_window_get_action:
 * @window      : A #TerminalWindow.
 * @action_name : Name of action.
 **/
GtkAction*
terminal_window_get_action (TerminalWindow *window,
                            const gchar    *action_name)
{
  return gtk_action_group_get_action (window->priv->action_group, action_name);
}



/**
 * terminal_window_get_scrollbar_visibility:
 * @window  : A #TerminalWindow.
 **/
TerminalVisibility
terminal_window_get_scrollbar_visibility (TerminalWindow *window)
{
  return window->priv->scrollbar_visibility;
}



/**
 * terminal_window_set_scrollbar_visibility:
 * @window    : A #TerminalWindow.
 * @scrollbar : Scrollbar visibility.
 **/
void
terminal_window_set_scrollbar_visibility (TerminalWindow     *window,
                                          TerminalVisibility  scrollbar)
{
  window->priv->scrollbar_visibility = scrollbar;
}



/**
 * terminal_window_get_menubar_height:
 * @window  : A #TerminalWindow.
 **/
gint
terminal_window_get_menubar_height (TerminalWindow *window)
{
  GtkRequisition req;

  req.height = 0;

  if (window->priv->menubar != NULL && gtk_widget_get_visible (window->priv->menubar))
    gtk_widget_get_preferred_size (window->priv->menubar, &req, NULL);

  return req.height;
}



/**
 * terminal_window_get_toolbar_height:
 * @window  : A #TerminalWindow.
 **/
gint
terminal_window_get_toolbar_height (TerminalWindow *window)
{
  GtkRequisition req;

  req.height = 0;

  if (window->priv->toolbar != NULL && gtk_widget_get_visible (window->priv->toolbar))
    gtk_widget_get_preferred_size (window->priv->toolbar, &req, NULL);

  return req.height;
}



/**
 * terminal_window_rebuild_tabs_menu:
 * @window  : A #TerminalWindow.
 **/
void
terminal_window_rebuild_tabs_menu (TerminalWindow *window)
{
  gint            npages, n;
  GtkWidget      *page;
  GSList         *group = NULL;
  GtkRadioAction *radio_action;
  gchar           name[50];
  GSList         *lp;
  GtkAccelKey     key = {0};

  if (window->priv->tabs_menu_merge_id != 0)
    {
      /* remove merge id */
      gtk_ui_manager_remove_ui (window->priv->ui_manager, window->priv->tabs_menu_merge_id);

      /* drop all the old accels from the action group */
      for (lp = window->priv->tabs_menu_actions; lp != NULL; lp = lp->next)
        gtk_action_group_remove_action (window->priv->action_group, GTK_ACTION (lp->data));

      g_slist_free (window->priv->tabs_menu_actions);
      window->priv->tabs_menu_actions = NULL;
    }

  /* create a new merge id */
  window->priv->tabs_menu_merge_id = gtk_ui_manager_new_merge_id (window->priv->ui_manager);
  terminal_assert (window->priv->tabs_menu_actions == NULL);

  /* walk the tabs */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
  for (n = 0; n < npages; n++)
    {
      page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), n);

      g_snprintf (name, sizeof (name), "goto-tab-%d", n + 1);

      /* create action */
      radio_action = gtk_radio_action_new (name, NULL, NULL, NULL, n);
      gtk_action_set_sensitive (GTK_ACTION (radio_action), npages > 1);
      g_object_bind_property (G_OBJECT (page), "title",
                              G_OBJECT (radio_action), "label",
                              G_BINDING_SYNC_CREATE);
      gtk_radio_action_set_group (radio_action, group);
      group = gtk_radio_action_get_group (radio_action);
      gtk_action_group_add_action (window->priv->action_group, GTK_ACTION (radio_action));
      g_signal_connect (G_OBJECT (radio_action), "activate",
          G_CALLBACK (terminal_window_action_goto_tab), window->priv->notebook);

      /* connect action to the page so we can active it when a tab is switched */
      g_object_set_qdata_full (G_OBJECT (page), tabs_menu_action_quark,
                               radio_action, g_object_unref);

      /* add action in the menu */
      gtk_ui_manager_add_ui (window->priv->ui_manager, window->priv->tabs_menu_merge_id,
                             "/main-menu/tabs-menu/placeholder-tab-items",
                             name, name, GTK_UI_MANAGER_MENUITEM, FALSE);

      if (npages > 1)
        {
          /* add to right-click tab menu */
          gtk_ui_manager_add_ui (window->priv->ui_manager, window->priv->tabs_menu_merge_id,
                                 "/tab-menu/tabs-menu/placeholder-tab-items",
                                 name, name, GTK_UI_MANAGER_MENUITEM, FALSE);
        }

      /* set an accelerator path */
      g_snprintf (name, sizeof (name), "<Actions>/terminal-window/goto-tab-%d", n + 1);
      if (gtk_accel_map_lookup_entry (name, &key) && key.accel_key != 0)
        gtk_action_set_accel_path (GTK_ACTION (radio_action), name);

      /* store */
      window->priv->tabs_menu_actions = g_slist_prepend (window->priv->tabs_menu_actions, radio_action);
    }
}
