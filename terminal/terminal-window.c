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

#include <gdk/gdkkeysyms.h>
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



/* Signal identifiers */
enum
{
  NEW_WINDOW,
  NEW_WINDOW_WITH_SCREEN,
  LAST_SIGNAL
};



static void            terminal_window_finalize                      (GObject                *object);
static gboolean        terminal_window_delete_event                  (GtkWidget              *widget,
                                                                      GdkEventAny            *event);
static gboolean        terminal_window_state_event                   (GtkWidget              *widget,
                                                                      GdkEventWindowState    *event);
static void            terminal_window_style_set                     (GtkWidget              *widget,
                                                                      GtkStyle               *previous_style);
static gboolean        terminal_window_confirm_close                 (TerminalWindow         *window);
static void            terminal_window_size_push                     (TerminalWindow         *window);
static gboolean        terminal_window_size_pop                      (gpointer                data);
static void            terminal_window_set_size_force_grid           (TerminalWindow         *window,
                                                                      TerminalScreen         *screen,
                                                                      glong                   force_grid_width,
                                                                      glong                   force_grid_height);
static gboolean        terminal_window_accel_activate                (GtkAccelGroup          *accel_group,
                                                                      GObject                *acceleratable,
                                                                      guint                   accel_key,
                                                                      GdkModifierType         accel_mods,
                                                                      TerminalWindow         *window);
static void            terminal_window_update_actions                (TerminalWindow         *window);
static void            terminal_window_rebuild_tabs_menu             (TerminalWindow         *window);
static void            terminal_window_notebook_page_switched        (GtkNotebook            *notebook,
                                                                      guint                   page_num,
                                                                      TerminalWindow         *window);
static void            terminal_window_notebook_page_reordered       (GtkNotebook            *notebook,
                                                                      guint                   page_num,
                                                                      TerminalWindow         *window);
static void            terminal_window_notebook_page_added           (GtkNotebook            *notebook,
                                                                      GtkWidget              *child,
                                                                      guint                   page_num,
                                                                      TerminalWindow         *window);
static void            terminal_window_notebook_page_removed         (GtkNotebook            *notebook,
                                                                      GtkWidget              *child,
                                                                      guint                   page_num,
                                                                      TerminalWindow         *window);
static gboolean        terminal_window_notebook_button_press_event   (GtkNotebook            *notebook,
                                                                      GdkEventButton         *event,
                                                                      TerminalWindow         *window);
static gboolean        terminal_window_notebook_button_release_event (GtkNotebook            *notebook,
                                                                      GdkEventButton         *event,
                                                                      TerminalWindow         *window);
static void            terminal_window_notebook_drag_data_received   (GtkWidget              *widget,
                                                                      GdkDragContext         *context,
                                                                      gint                    x,
                                                                      gint                    y,
                                                                      GtkSelectionData       *selection_data,
                                                                      guint                   info,
                                                                      guint                   time,
                                                                      TerminalWindow         *window);
static GtkNotebook    *terminal_window_notebook_create_window        (GtkNotebook            *notebook,
                                                                      GtkWidget              *child,
                                                                      gint                    x,
                                                                      gint                    y,
                                                                      TerminalWindow         *window);
static GtkWidget      *terminal_window_get_context_menu              (TerminalScreen         *screen,
                                                                      TerminalWindow         *window);
static void            terminal_window_notify_title                  (TerminalScreen         *screen,
                                                                      GParamSpec             *pspec,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_set_encoding           (GtkAction              *action,
                                                                      const gchar            *charset,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_new_tab                (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_new_window             (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_detach_tab             (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_close_tab              (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_close_window           (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_copy                   (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_paste                  (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_paste_selection        (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_select_all             (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_prefs                  (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_show_menubar           (GtkToggleAction        *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_show_toolbar           (GtkToggleAction        *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_show_borders           (GtkToggleAction        *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_fullscreen             (GtkToggleAction        *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_prev_tab               (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_next_tab               (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_move_tab_left          (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_move_tab_right         (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_goto_tab               (GtkRadioAction         *action,
                                                                      GtkNotebook            *notebook);
static void            terminal_window_action_set_title              (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_search                 (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_search_next            (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_search_prev            (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_reset                  (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_reset_and_clear        (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_contents               (GtkAction              *action,
                                                                      TerminalWindow         *window);
static void            terminal_window_action_about                  (GtkAction              *action,
                                                                      TerminalWindow         *window);



static guint   window_signals[LAST_SIGNAL];
static gchar   *window_notebook_group = PACKAGE_NAME;
static GQuark  tabs_menu_action_quark = 0;



static const GtkActionEntry action_entries[] =
{
  { "file-menu", NULL, N_ ("_File"), NULL, NULL, NULL, },
    { "new-tab", "tab-new", N_ ("Open _Tab"), "<control><shift>t", N_ ("Open a new terminal tab"), G_CALLBACK (terminal_window_action_new_tab), },
    { "new-window", "window-new", N_ ("Open T_erminal"), "<control><shift>n", N_ ("Open a new terminal window"), G_CALLBACK (terminal_window_action_new_window), },
    { "detach-tab", NULL, N_ ("_Detach Tab"), "<control><shift>d", NULL, G_CALLBACK (terminal_window_action_detach_tab), },
    { "close-tab", GTK_STOCK_CLOSE, N_ ("Close T_ab"), "<control><shift>w", NULL, G_CALLBACK (terminal_window_action_close_tab), },
    { "close-window", GTK_STOCK_QUIT, N_ ("Close _Window"), "<control><shift>q", NULL, G_CALLBACK (terminal_window_action_close_window), },
  { "edit-menu", NULL, N_ ("_Edit"), NULL, NULL, NULL, },
    { "copy", GTK_STOCK_COPY, N_ ("_Copy"), "<control><shift>c", N_ ("Copy to clipboard"), G_CALLBACK (terminal_window_action_copy), },
    { "paste", GTK_STOCK_PASTE, N_ ("_Paste"), "<control><shift>v", N_ ("Paste from clipboard"), G_CALLBACK (terminal_window_action_paste), },
    { "paste-selection", NULL, N_ ("Paste _Selection"), NULL, NULL, G_CALLBACK (terminal_window_action_paste_selection), },
    { "select-all", GTK_STOCK_SELECT_ALL, N_ ("Select _All"), "<control><shift>a", NULL, G_CALLBACK (terminal_window_action_select_all), },
    { "preferences", GTK_STOCK_PREFERENCES, N_ ("Pr_eferences..."), NULL, N_ ("Open the preferences dialog"), G_CALLBACK (terminal_window_action_prefs), },
  { "view-menu", NULL, N_ ("_View"), NULL, NULL, NULL, },
  { "terminal-menu", NULL, N_ ("_Terminal"), NULL, NULL, NULL, },
    { "set-title", NULL, N_ ("_Set Title..."), NULL, NULL, G_CALLBACK (terminal_window_action_set_title), },
    { "search", GTK_STOCK_FIND, N_ ("_Find..."), "<control><shift>f", NULL, G_CALLBACK (terminal_window_action_search), },
    { "search-next", NULL, N_ ("Find Ne_xt"), NULL, NULL, G_CALLBACK (terminal_window_action_search_next), },
    { "search-prev", NULL, N_ ("Find Pre_vious"), NULL, NULL, G_CALLBACK (terminal_window_action_search_prev), },
    { "reset", NULL, N_ ("_Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset), },
    { "reset-and-clear", NULL, N_ ("_Clear Scrollback and Reset"), NULL, NULL, G_CALLBACK (terminal_window_action_reset_and_clear), },
  { "tabs-menu", NULL, N_ ("T_abs"), NULL, NULL, NULL, },
    { "prev-tab", GTK_STOCK_GO_BACK, N_ ("_Previous Tab"), "<Control>Page_Up", N_ ("Switch to previous tab"), G_CALLBACK (terminal_window_action_prev_tab), },
    { "next-tab", GTK_STOCK_GO_FORWARD, N_ ("_Next Tab"), "<Control>Page_Down", N_ ("Switch to next tab"), G_CALLBACK (terminal_window_action_next_tab), },
    { "move-tab-left", NULL, N_ ("Move Tab _Left"), NULL, NULL, G_CALLBACK (terminal_window_action_move_tab_left), },
    { "move-tab-right", NULL, N_ ("Move Tab _Right"), NULL, NULL, G_CALLBACK (terminal_window_action_move_tab_right), },
  { "help-menu", NULL, N_ ("_Help"), NULL, NULL, NULL, },
    { "contents", GTK_STOCK_HELP, N_ ("_Contents"), "F1", N_ ("Display help contents"), G_CALLBACK (terminal_window_action_contents), },
    { "about", GTK_STOCK_ABOUT, N_ ("_About"), NULL, NULL, G_CALLBACK (terminal_window_action_about), },
  { "input-methods", NULL, N_ ("_Input Methods"), NULL, NULL, NULL, },
};

static const GtkToggleActionEntry toggle_action_entries[] =
{
  { "show-menubar", NULL, N_ ("Show _Menubar"), NULL, N_ ("Show/hide the menubar"), G_CALLBACK (terminal_window_action_show_menubar), FALSE, },
  { "show-toolbar", NULL, N_ ("Show _Toolbar"), NULL, N_ ("Show/hide the toolbar"), G_CALLBACK (terminal_window_action_show_toolbar), FALSE, },
  { "show-borders", NULL, N_ ("Show Window _Borders"), NULL, N_ ("Show/hide the window decorations"), G_CALLBACK (terminal_window_action_show_borders), TRUE, },
  { "fullscreen", GTK_STOCK_FULLSCREEN, N_ ("_Fullscreen"), "F11", N_ ("Toggle fullscreen mode"), G_CALLBACK (terminal_window_action_fullscreen), FALSE, },
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

  /* initialize quark */
  tabs_menu_action_quark = g_quark_from_static_string ("tabs-menu-item");
}



static void
terminal_window_init (TerminalWindow *window)
{
  GtkAccelGroup  *accel_group;
  GtkWidget      *vbox;
  gboolean        always_show_tabs;
  GdkScreen      *screen;
  GdkVisual    *visual;

  window->preferences = terminal_preferences_get ();

  /* try to set the rgba colormap so vte can use real transparency */
  screen = gtk_window_get_screen (GTK_WINDOW (window));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual != NULL)
    gtk_widget_set_visual (GTK_WIDGET (window), visual);

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
  g_signal_connect_after (G_OBJECT (accel_group), "accel-activate",
      G_CALLBACK (terminal_window_accel_activate), window);

  window->vbox = vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  /* allocate the notebook for the terminal screens */
  g_object_get (G_OBJECT (window->preferences), "misc-always-show-tabs", &always_show_tabs, NULL);
  window->notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                                   "homogeneous", TRUE,
                                   "scrollable", TRUE,
                                   "show-border", FALSE,
                                   "show-tabs", always_show_tabs,
                                   "tab-hborder", 0,
                                   "tab-vborder", 0,
                                   NULL);

  /* hide the ugly terminal border when tabs are shown */
  terminal_util_set_style_thinkess (window->notebook, 0);

  /* set the notebook group id */
  gtk_notebook_set_group_name (GTK_NOTEBOOK (window->notebook), window_notebook_group);

  /* signals */
  g_signal_connect (G_OBJECT (window->notebook), "switch-page",
    G_CALLBACK (terminal_window_notebook_page_switched), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-reordered",
    G_CALLBACK (terminal_window_notebook_page_reordered), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-removed",
    G_CALLBACK (terminal_window_notebook_page_removed), window);
  g_signal_connect (G_OBJECT (window->notebook), "page-added",
    G_CALLBACK (terminal_window_notebook_page_added), window);
  g_signal_connect (G_OBJECT (window->notebook), "create-window",
    G_CALLBACK (terminal_window_notebook_create_window), window);
  g_signal_connect (G_OBJECT (window->notebook), "button-press-event",
      G_CALLBACK (terminal_window_notebook_button_press_event), window);
  g_signal_connect (G_OBJECT (window->notebook), "button-release-event",
      G_CALLBACK (terminal_window_notebook_button_release_event), window);

  gtk_box_pack_start (GTK_BOX (vbox), window->notebook, TRUE, TRUE, 0);
  gtk_widget_show (window->notebook);

  /* create encoding action */
  window->encoding_action = terminal_encoding_action_new ("set-encoding", _("Set _Encoding"));
  gtk_action_group_add_action (window->action_group, window->encoding_action);
  g_signal_connect (G_OBJECT (window->encoding_action), "encoding-changed",
      G_CALLBACK (terminal_window_action_set_encoding), window);

  /* cache action pointers */
  window->action_detah_tab = gtk_action_group_get_action (window->action_group, "detach-tab");
  window->action_close_tab = gtk_action_group_get_action (window->action_group, "close-tab");
  window->action_prev_tab = gtk_action_group_get_action (window->action_group, "prev-tab");
  window->action_next_tab = gtk_action_group_get_action (window->action_group, "next-tab");
  window->action_move_tab_left = gtk_action_group_get_action (window->action_group, "move-tab-left");
  window->action_move_tab_right = gtk_action_group_get_action (window->action_group, "move-tab-right");
  window->action_copy = gtk_action_group_get_action (window->action_group, "copy");
  window->action_search_next = gtk_action_group_get_action (window->action_group, "search-next");
  window->action_search_prev = gtk_action_group_get_action (window->action_group, "search-prev");
  window->action_fullscreen = gtk_action_group_get_action (window->action_group, "fullscreen");

#if defined(GDK_WINDOWING_X11)
  /* setup fullscreen mode */
  if (!gdk_x11_screen_supports_net_wm_hint (screen, gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
    gtk_action_set_sensitive (window->action_fullscreen, FALSE);
#endif
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  g_object_unref (G_OBJECT (window->preferences));
  g_object_unref (G_OBJECT (window->action_group));
  g_object_unref (G_OBJECT (window->ui_manager));
  g_object_unref (G_OBJECT (window->encoding_action));

  g_slist_free (window->tabs_menu_actions);

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
      g_signal_handlers_disconnect_by_func (G_OBJECT (window->notebook),
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
      if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (window->action_fullscreen)) != fullscreen)
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (window->action_fullscreen), fullscreen);
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
                                        | 0
                                        | GTK_DIALOG_MODAL,
                                        NULL);

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

  button = xfce_gtk_button_new_mixed (GTK_STOCK_CLOSE, _("Close T_ab"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_CLOSE);
  gtk_widget_show (button);

  button = xfce_gtk_button_new_mixed (GTK_STOCK_QUIT, _("Close _Window"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_YES);
  gtk_widget_grab_focus (button);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

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
  else if (response == GTK_RESPONSE_CLOSE
           && window->active != NULL)
    {
      gtk_widget_destroy (GTK_WIDGET (window->active));
    }

  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_YES);
}



static void
terminal_window_size_push (TerminalWindow *window)
{
  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));

  if (window->active != NULL)
    {
      terminal_screen_get_size (window->active,
                                &window->grid_width,
                                &window->grid_height);
    }
}



static gboolean
terminal_window_size_pop (gpointer data)
{
  TerminalWindow *window = TERMINAL_WINDOW (data);

  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  if (window->active != NULL)
    {
      terminal_window_set_size_force_grid (window, window->active,
                                           window->grid_width,
                                           window->grid_height);
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
  GtkAction   *actions[] = { window->action_prev_tab, window->action_next_tab };
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
  GtkNotebook    *notebook = GTK_NOTEBOOK (window->notebook);
  GtkAction      *action;
  gboolean        cycle_tabs;
  gint            page_num;
  gint            n_pages;
  gboolean        can_search;

  /* determine the number of pages */
  n_pages = gtk_notebook_get_n_pages (notebook);

  /* "Detach Tab" is only sensitive if we have atleast two pages */
  gtk_action_set_sensitive (window->action_detah_tab, (n_pages > 1));

  /* ... same for the "Close Tab" action */
  gtk_action_set_sensitive (window->action_close_tab, (n_pages > 1));

  /* update the actions for the current terminal screen */
 if (G_LIKELY (window->active != NULL))
    {
      page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (window->active));

      g_object_get (G_OBJECT (window->preferences),
                    "misc-cycle-tabs", &cycle_tabs,
                    NULL);

      gtk_action_set_sensitive (window->action_prev_tab,
                                (cycle_tabs && n_pages > 1) || (page_num > 0));
      gtk_action_set_sensitive (window->action_next_tab,
                                (cycle_tabs && n_pages > 1 ) || (page_num < n_pages - 1));

      gtk_action_set_sensitive (window->action_move_tab_left, n_pages > 1);
      gtk_action_set_sensitive (window->action_move_tab_right, n_pages > 1);

      gtk_action_set_sensitive (window->action_copy,
                                terminal_screen_has_selection (window->active));

      can_search = terminal_screen_search_has_gregex (window->active);
      gtk_action_set_sensitive (window->action_search_next, can_search);
      gtk_action_set_sensitive (window->action_search_prev, can_search);

      /* update the "Go" menu */
      action = g_object_get_qdata (G_OBJECT (window->active), tabs_menu_action_quark);
      if (G_LIKELY (action != NULL))
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
    }
}



static void
terminal_window_rebuild_tabs_menu (TerminalWindow *window)
{
  gint            npages, n;
  GtkWidget      *page;
  GSList         *group = NULL;
  GtkRadioAction *radio_action;
  gchar           name[50];
  GSList         *lp;

  if (window->tabs_menu_merge_id != 0)
    {
      /* remove merge id */
      gtk_ui_manager_remove_ui (window->ui_manager, window->tabs_menu_merge_id);

      /* drop all the old accels from the action group */
      for (lp = window->tabs_menu_actions; lp != NULL; lp = lp->next)
        gtk_action_group_remove_action (window->action_group, GTK_ACTION (lp->data));

      g_slist_free (window->tabs_menu_actions);
      window->tabs_menu_actions = NULL;
    }

  /* create a new merge id */
  window->tabs_menu_merge_id = gtk_ui_manager_new_merge_id (window->ui_manager);
  terminal_assert (window->tabs_menu_actions == NULL);

  /* walk the tabs */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  for (n = 0; n < npages; n++)
    {
      page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), n);

      g_snprintf (name, sizeof (name), "goto-tab-%d", n + 1);

      /* create action */
      radio_action = gtk_radio_action_new (name, NULL, NULL, NULL, n);
      gtk_action_set_sensitive (GTK_ACTION (radio_action), npages > 1);
      g_object_bind_property (G_OBJECT (page), "title",
                              G_OBJECT (radio_action), "label",
                              G_BINDING_SYNC_CREATE);
      gtk_radio_action_set_group (radio_action, group);
      group = gtk_radio_action_get_group (radio_action);
      gtk_action_group_add_action (window->action_group, GTK_ACTION (radio_action));
      g_signal_connect (G_OBJECT (radio_action), "activate",
          G_CALLBACK (terminal_window_action_goto_tab), window->notebook);

      /* connect action to the page so we can active it when a tab is switched */
      g_object_set_qdata_full (G_OBJECT (page), tabs_menu_action_quark,
                               radio_action, g_object_unref);

      /* add action in the menu */
      gtk_ui_manager_add_ui (window->ui_manager, window->tabs_menu_merge_id,
                             "/main-menu/tabs-menu/placeholder-tab-items",
                             name, name, GTK_UI_MANAGER_MENUITEM, FALSE);

      if (npages > 1)
        {
          /* add to right-click tab menu */
          gtk_ui_manager_add_ui (window->ui_manager, window->tabs_menu_merge_id,
                                 "/tab-menu/tabs-menu/placeholder-tab-items",
                                 name, name, GTK_UI_MANAGER_MENUITEM, FALSE);
        }

      /* set an accelerator path */
      g_snprintf (name, sizeof (name), "<Actions>/terminal-window/goto-tab-%d", n + 1);
      gtk_action_set_accel_path (GTK_ACTION (radio_action), name);

      /* store */
      window->tabs_menu_actions = g_slist_prepend (window->tabs_menu_actions, radio_action);
    }
}



static void
terminal_window_notebook_page_switched (GtkNotebook     *notebook,
                                        guint            page_num,
                                        TerminalWindow  *window)
{
  TerminalScreen *active;
  gboolean        was_null;
  const gchar    *encoding;

  /* get the new active page */
  active = TERMINAL_SCREEN (gtk_notebook_get_nth_page (notebook, page_num));
  terminal_return_if_fail (active == NULL || TERMINAL_IS_SCREEN (active));

  /* only update when really changed */
  if (G_LIKELY (window->active != active))
    {
      /* check if we need to set the size or if this was already done
       * in the page add function */
      was_null = (window->active == NULL);

      /* set new active tab */
      window->active = active;

      /* set the new window title */
      terminal_window_notify_title (active, NULL, window);

      /* update actions in the window */
      terminal_window_update_actions (window);

      /* reset the activity counter */
      terminal_screen_reset_activity (active);

      /* set charset for menu */
      encoding = terminal_screen_get_encoding (window->active);
      terminal_encoding_action_set_charset (window->encoding_action, encoding);

      /* set the new geometry widget */
      if (G_LIKELY (!was_null))
        terminal_screen_set_window_geometry_hints (active, GTK_WINDOW (window));
    }
}



static void
terminal_window_notebook_page_reordered (GtkNotebook     *notebook,
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
  terminal_return_if_fail (window->notebook == GTK_WIDGET (notebook));

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

  if (G_LIKELY (window->active != NULL))
    {
      /* match the size of the active screen */
      terminal_screen_get_size (window->active, &w, &h);
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
  gint npages;

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
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
  if (G_UNLIKELY (npages == 0))
    {
      /* no tabs, destroy the window */
      gtk_widget_destroy (GTK_WIDGET (window));
    }
  else
    {
      /* show the tabs when needed */
      terminal_window_notebook_show_tabs (window);

      /* regenerate the "Go" menu */
      terminal_window_rebuild_tabs_menu (window);
    }
}

static gboolean
terminal_window_notebook_event_in_allocation (gint event_x,
											  gint event_y,
											  GtkWidget *widget)
{
	cairo_rectangle_int_t *allocation;
	gtk_widget_get_allocation (widget, allocation);

	if (event_x >= allocation->x \
		&& event_x <= allocation->x + allocation->width \
		&& event_y >= allocation->y \
		&& event_y <= allocation->y + allocation->height)
		{
			return TRUE;
		}
	else
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
          label = gtk_notebook_get_tab_label (notebook, GTK_WIDGET (window->active));
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
          g_object_get (G_OBJECT (window->preferences),
                        "misc-tab-close-middle-click", &close_middle_click, NULL);
          if (close_middle_click)
            gtk_widget_destroy (page);
        }
      else
        {
          /* update the current tab before we show the menu */
          gtk_notebook_set_current_page (notebook, page_num);

          /* show the tab menu */
          menu = gtk_ui_manager_get_widget (window->ui_manager, "/tab-menu");
          gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);
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

  if (G_LIKELY (window->active != NULL))
    terminal_screen_focus (window->active);

  return FALSE;
}



static void
terminal_window_notebook_drag_data_received (GtkWidget        *widget,
                                             GdkDragContext   *context,
                                             gint              x,
                                             gint              y,
                                             GtkSelectionData *selection_data,
                                             guint             info,
                                             guint32           drag_time,
                                             TerminalWindow   *window)
{
  GtkWidget  *notebook;
  GtkWidget **screen;
  GtkWidget  *child, *label;
  gint        i, n_pages;
  gboolean    succeed = FALSE;
  cairo_rectangle_int_t *allocation;

  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (widget));

  /* check */
  if (G_LIKELY (info == TARGET_GTK_NOTEBOOK_TAB))
    {
      /* get the source notebook (other window) */
      notebook = gtk_drag_get_source_widget (context);
      terminal_return_if_fail (GTK_IS_NOTEBOOK (notebook));

      /* leave if there is only one
       * page in the notebook (window will close before we insert) */
      if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) < 2
          && *screen == widget)
        goto leave;

      /* figure out where to insert the tab in the notebook */
      n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook));
      for (i = 0; i < n_pages; i++)
        {
          /* get the child label */
          child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), i);
          label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (window->notebook), child);
		  gtk_widget_get_allocation (label, allocation);
          /* break if we have a matching drop position */
          if (x < (allocation->x + allocation->width / 2))
            break;
        }

      if (notebook == window->notebook)
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
          gtk_container_remove (GTK_CONTAINER (notebook), *screen);

          /* add the screen to the new window */
          terminal_window_add (window, TERMINAL_SCREEN (*screen));

          /* move the child to the correct position */
          gtk_notebook_reorder_child (GTK_NOTEBOOK (window->notebook), *screen, i);

          /* release reference */
          g_object_unref (G_OBJECT (*screen));
          g_object_unref (G_OBJECT (window));
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
  TerminalScreen *screen;

  terminal_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (child), NULL);
  terminal_return_val_if_fail (notebook == GTK_NOTEBOOK (window->notebook), NULL);

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



static GtkWidget *
terminal_window_get_context_menu (TerminalScreen  *screen,
                                  TerminalWindow  *window)
{
  GtkWidget *popup;
  GtkWidget *menu;
  GtkWidget *item;

  if (G_UNLIKELY (screen != window->active))
    return NULL;

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

  return popup;
}



static void
terminal_window_notify_title (TerminalScreen *screen,
                              GParamSpec     *pspec,
                              TerminalWindow *window)
{
  gchar *title;

  /* update window title */
  if (screen == window->active)
    {
      title = terminal_screen_get_title (window->active);
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

  if (G_LIKELY (window->active != NULL))
    {
      /* set the charset */
      terminal_screen_set_encoding (window->active, charset);

      /* update menu */
      new = terminal_screen_get_encoding (window->active);
      terminal_encoding_action_set_charset (action, new);
    }
}



static void
terminal_window_action_new_tab (GtkAction       *action,
                                TerminalWindow  *window)
{
  const gchar *directory;
  GtkWidget   *terminal;

  terminal = g_object_new (TERMINAL_TYPE_SCREEN, NULL);

  if (G_LIKELY (window->active != NULL))
    {
      directory = terminal_screen_get_working_directory (window->active);
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
  const gchar *directory;

  if (G_LIKELY (window->active != NULL))
    {
      directory = terminal_screen_get_working_directory (window->active);
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW], 0, directory);
    }
}



static void
terminal_window_action_detach_tab (GtkAction      *action,
                                   TerminalWindow *window)
{
  if (G_LIKELY (window->active != NULL))
    terminal_window_notebook_create_window (GTK_NOTEBOOK (window->notebook),
                                            GTK_WIDGET (window->active),
                                            -1, -1, window);
}



static void
terminal_window_action_close_tab (GtkAction       *action,
                                  TerminalWindow  *window)
{
  if (G_LIKELY (window->active != NULL))
    gtk_widget_destroy (GTK_WIDGET (window->active));
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
  if (G_LIKELY (window->active != NULL))
    terminal_screen_copy_clipboard (window->active);
}



static void
terminal_window_action_paste (GtkAction       *action,
                              TerminalWindow  *window)
{
  if (G_LIKELY (window->active != NULL))
    terminal_screen_paste_clipboard (window->active);
}



static void
terminal_window_action_paste_selection (GtkAction      *action,
                                        TerminalWindow *window)
{
  if (G_LIKELY (window->active != NULL))
    terminal_screen_paste_primary (window->active);
}



static void
terminal_window_action_select_all (GtkAction      *action,
                                   TerminalWindow *window)
{
  if (G_LIKELY (window->active != NULL))
    terminal_screen_select_all (window->active);
}



static void
terminal_window_action_prefs_died (gpointer  user_data,
                                   GObject  *where_the_object_was)
{
  TerminalWindow *window = TERMINAL_WINDOW (user_data);

  window->preferences_dialog = NULL;
  window->n_child_windows--;

  if (window->drop_down)
    terminal_util_activate_window (GTK_WINDOW (window));
}



static void
terminal_window_action_prefs (GtkAction      *action,
                              TerminalWindow *window)
{
  if (window->preferences_dialog == NULL)
    {
      window->preferences_dialog = terminal_preferences_dialog_new (window->drop_down);
      if (G_LIKELY (window->preferences_dialog != NULL))
        {
          window->n_child_windows++;
          g_object_weak_ref (G_OBJECT (window->preferences_dialog),
                             terminal_window_action_prefs_died, window);
        }
    }

  if (window->preferences_dialog != NULL)
    {
      gtk_window_set_transient_for (GTK_WINDOW (window->preferences_dialog), GTK_WINDOW (window));
      gtk_window_present (GTK_WINDOW (window->preferences_dialog));
    }
}



static void
terminal_window_action_show_menubar (GtkToggleAction *action,
                                     TerminalWindow  *window)
{
  terminal_return_if_fail (GTK_IS_UI_MANAGER (window->ui_manager));

  terminal_window_size_push (window);

  if (gtk_toggle_action_get_active (action))
    {
      if (G_LIKELY (window->menubar == NULL))
        {
          window->menubar = gtk_ui_manager_get_widget (window->ui_manager, "/main-menu");
          gtk_box_pack_start (GTK_BOX (window->vbox), window->menubar, FALSE, FALSE, 0);
          gtk_box_reorder_child (GTK_BOX (window->vbox), window->menubar, 0);
        }

      gtk_widget_show (window->menubar);
    }
  else if (window->menubar != NULL)
    {
      gtk_widget_hide (window->menubar);
    }

  terminal_window_size_pop (window);
}



static void
terminal_window_action_show_toolbar (GtkToggleAction *action,
                                     TerminalWindow  *window)
{
  terminal_return_if_fail (GTK_IS_UI_MANAGER (window->ui_manager));
  terminal_return_if_fail (GTK_IS_ACTION_GROUP (window->action_group));

  terminal_window_size_push (window);

  if (gtk_toggle_action_get_active (action))
    {
      if (window->toolbar == NULL)
        {
          window->toolbar = gtk_ui_manager_get_widget (window->ui_manager, "/main-toolbar");
          gtk_box_pack_start (GTK_BOX (window->vbox), window->toolbar, FALSE, FALSE, 0);
          gtk_box_reorder_child (GTK_BOX (window->vbox), window->toolbar, window->menubar != NULL ? 1 : 0);
        }

      gtk_widget_show (window->toolbar);
    }
  else if (window->toolbar != NULL)
    {
      gtk_widget_hide (window->toolbar);
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
terminal_window_action_move_tab_left (GtkAction      *action,
                                      TerminalWindow *window)
{
  gint       page_num;
  gint       last_page;
  GtkWidget *page;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook));
  last_page = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook)) - 1;
  page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), page_num);

  gtk_notebook_reorder_child (GTK_NOTEBOOK (window->notebook),
                              page, page_num == 0 ? last_page : page_num - 1);
}



static void
terminal_window_action_move_tab_right (GtkAction      *action,
                                       TerminalWindow *window)
{
  gint       page_num;
  gint       last_page;
  GtkWidget *page;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->notebook));
  last_page = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->notebook)) - 1;
  page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->notebook), page_num);

  gtk_notebook_reorder_child (GTK_NOTEBOOK (window->notebook),
                              page, page_num == last_page ? 0 : page_num + 1);
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
title_dialog_response (GtkWidget      *dialog,
                       gint            response,
                       TerminalWindow *window)
{
  /* check if we should open the user manual */
  if (response == GTK_RESPONSE_HELP)
    {
      /* open the "Set Title" paragraph in the "Usage" section */
      xfce_dialog_show_help (GTK_WINDOW (dialog), "terminal", "usage", NULL);
    }
  else
    {
      /* need for hiding on focus */
      if (window->drop_down)
        terminal_util_activate_window (GTK_WINDOW (window));

      /* close the dialog */
      window->n_child_windows--;
      gtk_widget_destroy (dialog);
    }
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
  GtkWidget *dialog;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *entry;

  if (G_LIKELY (window->active != NULL))
    {
      window->n_child_windows++;

      dialog = gtk_dialog_new_with_buttons (Q_("Window Title|Set Title"),
                                            GTK_WINDOW (window),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                            NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

      box = gtk_hbox_new (FALSE, 12);
      gtk_container_set_border_width (GTK_CONTAINER (box), 6);
      gtk_widget_show (box);

      label = gtk_label_new_with_mnemonic (_("_Title:"));
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      entry = gtk_entry_new ();
      gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
      gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
      gtk_entry_set_icon_from_stock (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
      g_signal_connect (G_OBJECT (entry), "icon-release", G_CALLBACK (title_dialog_clear), NULL);
      gtk_widget_show (entry);

      /* set Atk description and label relation for the entry */
      object = gtk_widget_get_accessible (entry);
      atk_object_set_description (object, _("Enter the title for the current terminal tab"));

      g_object_bind_property (G_OBJECT (window->active), "custom-title",
                              G_OBJECT (entry), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

      g_signal_connect (G_OBJECT (dialog), "response",
                        G_CALLBACK (title_dialog_response), window);

      gtk_widget_show (dialog);
    }
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
  terminal_return_if_fail (TERMINAL_IS_SCREEN (window->active));
  terminal_return_if_fail (window->search_dialog == dialog);

  if (response_id == TERMINAL_RESPONSE_SEARCH_NEXT
      || response_id == TERMINAL_RESPONSE_SEARCH_PREV)
    {
      regex = terminal_search_dialog_get_regex (TERMINAL_SEARCH_DIALOG (dialog), &error);
      if (G_LIKELY (error == NULL))
        {
          wrap_around = terminal_search_dialog_get_wrap_around (TERMINAL_SEARCH_DIALOG (dialog));
          terminal_screen_search_set_gregex (window->active, regex, wrap_around);
          if (regex != NULL)
            g_regex_unref (regex);

          if (response_id == TERMINAL_RESPONSE_SEARCH_NEXT)
            terminal_screen_search_find_next (window->active);
          else
            terminal_screen_search_find_previous (window->active);
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
      window->n_child_windows--;
      gtk_widget_hide (dialog);
    }

  /* update actions */
  can_search = terminal_screen_search_has_gregex (window->active);
  gtk_action_set_sensitive (window->action_search_next, can_search);
  gtk_action_set_sensitive (window->action_search_prev, can_search);
}



static void
terminal_window_action_search (GtkAction      *action,
                               TerminalWindow *window)
{
  if (window->search_dialog == NULL)
    {
      window->search_dialog = terminal_search_dialog_new (GTK_WINDOW (window));
      g_signal_connect (G_OBJECT (window->search_dialog), "response",
          G_CALLBACK (terminal_window_action_search_response), window);
      g_signal_connect (G_OBJECT (window->search_dialog), "delete-event",
          G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    }

  /* increase child counter */
  if (!gtk_widget_get_visible (window->search_dialog))
    window->n_child_windows++;

  terminal_search_dialog_present (TERMINAL_SEARCH_DIALOG (window->search_dialog));
}



static void
terminal_window_action_search_next (GtkAction      *action,
                                    TerminalWindow *window)
{
  if (G_LIKELY (window->active != NULL))
    terminal_screen_search_find_next (window->active);
}



static void
terminal_window_action_search_prev (GtkAction      *action,
                                    TerminalWindow *window)
{
  if (G_LIKELY (window->active != NULL))
    terminal_screen_search_find_previous (window->active);
}



static void
terminal_window_action_reset (GtkAction      *action,
                              TerminalWindow *window)
{
  if (G_LIKELY (window->active != NULL))
    terminal_screen_reset (window->active, FALSE);
}



static void
terminal_window_action_reset_and_clear (GtkAction       *action,
                                        TerminalWindow  *window)
{
  if (G_LIKELY (window->active != NULL))
    {
      terminal_screen_reset (window->active, TRUE);
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
  g_object_get (G_OBJECT (window->preferences),
                "misc-menubar-default", &show_menubar,
                "misc-toolbar-default", &show_toolbar,
                "misc-borders-default", &show_borders,
                NULL);

  /* setup full screen */
  if (fullscreen && gtk_action_is_sensitive (window->action_fullscreen))
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (window->action_fullscreen), TRUE);

  /* setup menubar visibility */
  if (G_LIKELY (menubar != TERMINAL_VISIBILITY_DEFAULT))
    show_menubar = (menubar == TERMINAL_VISIBILITY_SHOW);
  action = gtk_action_group_get_action (window->action_group, "show-menubar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show_menubar);

  /* setup toolbar visibility */
  if (G_LIKELY (toolbar != TERMINAL_VISIBILITY_DEFAULT))
    show_toolbar = (toolbar == TERMINAL_VISIBILITY_SHOW);
  action = gtk_action_group_get_action (window->action_group, "show-toolbar");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show_toolbar);

  /* setup borders visibility */
  if (G_LIKELY (borders != TERMINAL_VISIBILITY_DEFAULT))
    show_borders = (borders == TERMINAL_VISIBILITY_SHOW);
  action = gtk_action_group_get_action (window->action_group, "show-borders");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show_borders);

  /* property that is not suitable for init */
  g_object_bind_property (G_OBJECT (window->preferences), "misc-tab-position",
                          G_OBJECT (window->notebook), "tab-pos",
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

  page = gtk_notebook_append_page (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (screen), label);
  // TODO: should not be used anymore according to Gtk docs, remove if sure
  //gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (screen), TRUE, TRUE, GTK_PACK_START);

  /* allow tab sorting and dnd */
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (screen), TRUE);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (window->notebook), GTK_WIDGET (screen), TRUE);

  /* show the terminal screen */
  gtk_widget_realize (GTK_WIDGET (screen));
  gtk_widget_show (GTK_WIDGET (screen));

  /* switch to the new tab */
  gtk_notebook_set_current_page (GTK_NOTEBOOK (window->notebook), page);

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
  return window->active;
}



void
terminal_window_notebook_show_tabs (TerminalWindow *window)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (window->notebook);
  gboolean     show_tabs = TRUE;
  gint         npages;

  terminal_return_if_fail (TERMINAL_IS_WINDOW (window));

  /* check preferences */
  npages = gtk_notebook_get_n_pages (notebook);
  if (npages < 2)
    {
      g_object_get (G_OBJECT (window->preferences),
                    window->drop_down ? "dropdown-always-show-tabs" :
                    "misc-always-show-tabs", &show_tabs, NULL);
    }

  /* set the visibility of the tabs */
  if (gtk_notebook_get_show_tabs (notebook) != show_tabs)
    {
      /* store size */
      terminal_window_size_push (window);

      /* show or hdie the tabs */
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

  if (G_LIKELY (window->active != NULL))
    {
      terminal_screen_get_size (window->active, &w, &h);
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

  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (window->action_fullscreen)))
    result = g_slist_prepend (result, g_strdup ("--fullscreen"));

  action = gtk_action_group_get_action (window->action_group, "show-menubar");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_slist_prepend (result, g_strdup ("--show-menubar"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-menubar"));

  action = gtk_action_group_get_action (window->action_group, "show-borders");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_slist_prepend (result, g_strdup ("--show-borders"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-borders"));

  action = gtk_action_group_get_action (window->action_group, "show-toolbar");
  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    result = g_slist_prepend (result, g_strdup ("--show-toolbar"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-toolbar"));

  /* set restart commands of the tabs */
  children = gtk_container_get_children (GTK_CONTAINER (window->notebook));
  for (lp = children; lp != NULL; lp = lp->next)
    {
      if (lp != children)
        result = g_slist_prepend (result, g_strdup ("--tab"));
      result = g_slist_concat (terminal_screen_get_restart_command (lp->data), result);
    }
  g_list_free (children);

  return g_slist_reverse (result);
}
