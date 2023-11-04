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

#include <gdk/gdk.h>
#ifdef ENABLE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

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
#include <terminal/terminal-widget.h>



/* Signal identifiers */
enum
{
  NEW_WINDOW,
  NEW_WINDOW_WITH_SCREEN,
  LAST_SIGNAL
};

/* Used by confirm_close() */
enum
{
  CONFIRMED_NONE,
  CONFIRMED_CLOSE_TAB,
  CONFIRMED_CLOSE_WINDOW
};

/* CSS for slim notebook tabs style */
#define NOTEBOOK_NAME PACKAGE_NAME "-notebook"
const gchar *CSS_SLIM_TABS =
"#" NOTEBOOK_NAME " tab {\n"
"  min-height: 0;\n"
"  font-weight: normal;\n"
"  padding: 1px;\n"
"  margin: 0;\n"
"}\n"
"#" NOTEBOOK_NAME " tab button {\n"
"  min-height: 0;\n"
"  min-width: 0;\n"
"  padding: 1px;\n"
"  margin: 0;\n"
"}\n"
"#" NOTEBOOK_NAME " button {\n"
"  min-height: 0;\n"
"  min-width: 0;\n"
"  padding: 1px;\n"
"}\n";

/* See gnome-terminal bug #789356 */
#if GTK_CHECK_VERSION (3, 22, 23)
#define WINDOW_STATE_TILED (GDK_WINDOW_STATE_TILED       | \
                            GDK_WINDOW_STATE_LEFT_TILED  | \
                            GDK_WINDOW_STATE_RIGHT_TILED | \
                            GDK_WINDOW_STATE_TOP_TILED   | \
                            GDK_WINDOW_STATE_BOTTOM_TILED)
#else
#define WINDOW_STATE_TILED (GDK_WINDOW_STATE_TILED)
#endif



typedef struct
{
    TerminalWindow *window;
    gint            signal;
} SendSignalData;



static void         terminal_window_finalize                      (GObject             *object);
static gboolean     terminal_window_delete_event                  (GtkWidget           *widget,
                                                                   GdkEventAny         *event);
static gboolean     terminal_window_state_event                   (GtkWidget           *widget,
                                                                   GdkEventWindowState *event);
static void         terminal_window_style_set                     (GtkWidget           *widget,
                                                                   GtkStyle            *previous_style);
static gboolean     terminal_window_scroll_event                  (GtkWidget           *widget,
                                                                   GdkEventScroll      *event);
static gboolean     terminal_window_map_event                     (GtkWidget           *widget,
                                                                   GdkEventAny         *event);
static gboolean     terminal_window_focus_in_event                (GtkWidget           *widget,
                                                                   GdkEventFocus       *event);
static gboolean     terminal_window_key_press_event               (GtkWidget           *widget,
                                                                   GdkEventKey         *event);
static gint         terminal_window_confirm_close                 (TerminalScreen      *screen,
                                                                   TerminalWindow      *window);
static void         terminal_window_size_push                     (TerminalWindow      *window);
static gboolean     terminal_window_size_pop                      (gpointer             data);
static void         terminal_window_set_size_force_grid           (TerminalWindow      *window,
                                                                   TerminalScreen      *screen,
                                                                   glong                force_grid_width,
                                                                   glong                force_grid_height);
static void         terminal_window_update_slim_tabs              (TerminalWindow      *window);
static void         terminal_window_update_mnemonic_modifier      (TerminalWindow      *window);
static void         terminal_window_notebook_page_switched        (GtkNotebook         *notebook,
                                                                   GtkWidget           *page,
                                                                   guint                page_num,
                                                                   TerminalWindow      *window);
static void         terminal_window_notebook_page_added           (GtkNotebook         *notebook,
                                                                   GtkWidget           *child,
                                                                   guint                page_num,
                                                                   TerminalWindow      *window);
static void         terminal_window_notebook_page_removed         (GtkNotebook         *notebook,
                                                                   GtkWidget           *child,
                                                                   guint                page_num,
                                                                   TerminalWindow      *window);
static gboolean     terminal_window_notebook_event_in_allocation  (gint                 event_x,
                                                                   gint                 event_y,
                                                                   GtkWidget           *widget);
static gboolean     terminal_window_notebook_button_press_event   (GtkNotebook         *notebook,
                                                                   GdkEventButton      *event,
                                                                   TerminalWindow      *window);
static gboolean     terminal_window_notebook_button_release_event (GtkNotebook         *notebook,
                                                                   GdkEventButton      *event,
                                                                   TerminalWindow      *window);
static gboolean     terminal_window_notebook_scroll_event         (GtkNotebook         *notebook,
                                                                   GdkEventScroll      *event,
                                                                   TerminalWindow      *window);
static void         terminal_window_notebook_drag_data_received   (GtkWidget           *widget,
                                                                   GdkDragContext      *context,
                                                                   gint                 x,
                                                                   gint                 y,
                                                                   GtkSelectionData    *selection_data,
                                                                   guint                info,
                                                                   guint                time,
                                                                   TerminalWindow      *window);
static GtkNotebook *terminal_window_notebook_create_window        (GtkNotebook         *notebook,
                                                                   GtkWidget           *child,
                                                                   gint                 x,
                                                                   gint                 y,
                                                                   TerminalWindow      *window);
static GtkWidget   *terminal_window_get_context_menu              (TerminalScreen      *screen,
                                                                   TerminalWindow      *window);
static void         terminal_window_notify_title                  (TerminalScreen      *screen,
                                                                   GParamSpec          *pspec,
                                                                   TerminalWindow      *window);
static void         terminal_window_action_set_encoding           (GtkAction           *action,
                                                                   const gchar         *charset,
                                                                   TerminalWindow      *window);
static gboolean     terminal_window_action_new_tab                (TerminalWindow      *window);
static gboolean     terminal_window_action_new_window             (TerminalWindow      *window);
static gboolean     terminal_window_action_open_folder            (TerminalWindow      *window);
static gboolean     terminal_window_action_undo_close_tab         (TerminalWindow      *window);
static gboolean     terminal_window_action_detach_tab             (TerminalWindow      *window);
static gboolean     terminal_window_action_close_tab              (TerminalWindow      *window);
static gboolean     terminal_window_action_close_other_tabs       (TerminalWindow      *window);
static gboolean     terminal_window_action_close_window           (TerminalWindow      *window);
static gboolean     terminal_window_action_copy                   (TerminalWindow      *window);
static gboolean     terminal_window_action_copy_html              (TerminalWindow      *window);
static gboolean     terminal_window_action_paste                  (TerminalWindow      *window);
static gboolean     terminal_window_action_paste_selection        (TerminalWindow      *window);
static gboolean     terminal_window_action_select_all             (TerminalWindow      *window);
static gboolean     terminal_window_action_copy_input             (TerminalWindow      *window);
static gboolean     terminal_window_action_prefs                  (TerminalWindow      *window);
static gboolean     terminal_window_action_toggle_menubar         (TerminalWindow      *window);
static gboolean     terminal_window_action_show_menubar           (TerminalWindow      *window);
static gboolean     terminal_window_action_toggle_toolbar         (TerminalWindow      *window);
static gboolean     terminal_window_action_toggle_borders         (TerminalWindow      *window);
static gboolean     terminal_window_action_fullscreen             (TerminalWindow      *window);
static gboolean     terminal_window_action_readonly               (TerminalWindow      *window);
static gboolean     terminal_window_action_scroll_on_output       (TerminalWindow      *window);
static gboolean     terminal_window_action_zoom_in                (TerminalWindow      *window);
static gboolean     terminal_window_action_zoom_out               (TerminalWindow      *window);
static gboolean     terminal_window_action_zoom_reset             (TerminalWindow      *window);
static gboolean     terminal_window_action_prev_tab               (TerminalWindow      *window);
static gboolean     terminal_window_action_next_tab               (TerminalWindow      *window);
static gboolean     terminal_window_action_last_active_tab        (TerminalWindow      *window);
static gboolean     terminal_window_action_move_tab_left          (TerminalWindow      *window);
static gboolean     terminal_window_action_move_tab_right         (TerminalWindow      *window);
static gboolean     terminal_window_action_goto_tab               (GtkRadioAction      *action,
                                                                   GtkNotebook         *notebook);
static gboolean     terminal_window_action_set_title              (TerminalWindow      *window);
static gboolean     terminal_window_action_set_title_color        (TerminalWindow      *window);
static gboolean     terminal_window_action_search                 (TerminalWindow      *window);
static gboolean     terminal_window_action_search_next            (TerminalWindow      *window);
static gboolean     terminal_window_action_search_prev            (TerminalWindow      *window);
static gboolean     terminal_window_action_save_contents          (TerminalWindow      *window);
static gboolean     terminal_window_action_reset                  (TerminalWindow      *window);
static gboolean     terminal_window_action_reset_and_clear        (TerminalWindow      *window);
static void         terminal_window_action_send_signal            (SendSignalData      *data);
static gboolean     terminal_window_action_contents               (TerminalWindow      *window);
static gboolean     terminal_window_action_about                  (TerminalWindow      *window);
static gboolean     terminal_window_action_do_nothing             (TerminalWindow      *window);

static void         terminal_window_zoom_update_screens           (TerminalWindow      *window);
static void         terminal_window_switch_tab                    (GtkNotebook         *notebook,
                                                                   gboolean             switch_left);
static void         terminal_window_move_tab                      (GtkNotebook         *notebook,
                                                                   gboolean             move_left);
static void         title_popover_close                           (GtkWidget           *popover,
                                                                   TerminalWindow      *window);
static void         terminal_window_do_close_tab                  (TerminalScreen      *screen,
                                                                   TerminalWindow      *window);

static void         terminal_window_create_menu                   (TerminalWindow      *window,
                                                                   TerminalWindowAction action,
                                                                   GCallback            cb_update_menu);
static void         terminal_window_menu_clean                    (GtkMenu             *menu);
static void         terminal_window_menu_add_section              (TerminalWindow      *window,
                                                                   GtkWidget           *menu,
                                                                   MenuSections         sections,
                                                                   gboolean             as_submenu);
static void         terminal_window_update_file_menu              (TerminalWindow      *window,
                                                                   GtkWidget           *menu);
static void         terminal_window_update_edit_menu              (TerminalWindow      *window,
                                                                   GtkWidget           *menu);
static void         terminal_window_update_view_menu              (TerminalWindow      *window,
                                                                   GtkWidget           *menu);
static void         terminal_window_update_terminal_menu          (TerminalWindow      *window,
                                                                   GtkWidget           *menu);
static void         terminal_window_update_tabs_menu              (TerminalWindow      *window,
                                                                   GtkWidget           *menu);
static void         terminal_window_update_help_menu              (TerminalWindow      *window,
                                                                   GtkWidget           *menu);
static gboolean     terminal_window_can_go_left                   (TerminalWindow      *window);
static gboolean     terminal_window_can_go_right                  (TerminalWindow      *window);


struct _TerminalWindowPrivate
{
  GtkWidget           *vbox;
  GtkWidget           *notebook;
  GtkWidget           *tabs_menu; /* used for the go-to tab accelerators */

  /* for the drop-down to keep open with dialogs */
  guint                n_child_windows;

  guint                tabs_menu_merge_id;
  GSList              *tabs_menu_actions;

  TerminalPreferences *preferences;
  GtkWidget           *preferences_dialog;

  GtkAccelGroup       *accel_group;

  GtkWidget           *search_dialog;
  GtkWidget           *title_popover;

  /* pushed size of screen */
  glong                grid_width;
  glong                grid_height;

  GtkAction           *encoding_action;

  TerminalScreen      *active;
  TerminalScreen      *last_active;

  GQueue              *closed_tabs_list;

  gchar               *font;

  TerminalVisibility   scrollbar_visibility;
  TerminalZoomLevel    zoom;

  /* if this is a TerminalWindowDropdown */
  guint                drop_down : 1;
};

static guint   window_signals[LAST_SIGNAL];
static gchar  *window_notebook_group = PACKAGE_NAME;
static GQuark  tabs_menu_action_quark = 0;
static gchar  *signal_names[] =
{
  NULL,
  "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "BUS", "FPE", "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM", "TERM",
  "STKFLT", "CHLD", "CONT", "STOP", "TSTP", "TTIN", "TTOU", "URG", "XCPU", "XFSZ", "VTALRM", "PROF", "INCH", "IO",
  "PWR", "SYS"
};



static XfceGtkActionEntry action_entries[] =
{
    /* Use old accelerator paths for backwards compatibility */
    { TERMINAL_WINDOW_ACTION_FILE_MENU,             "<Actions>/terminal-window/file-menu",             "",                          XFCE_GTK_MENU_ITEM,       N_ ("_File"),                         NULL,                                     NULL,                     NULL, },
    { TERMINAL_WINDOW_ACTION_NEW_TAB,               "<Actions>/terminal-window/new-tab",               "<control><shift>t",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Open _Tab"),                     N_ ("Open a new terminal tab"),           "tab-new",                G_CALLBACK (terminal_window_action_new_tab), },
    { TERMINAL_WINDOW_ACTION_NEW_WINDOW,            "<Actions>/terminal-window/new-window",            "<control><shift>n",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Open T_erminal"),                N_ ("Open a new terminal window"),        "window-new",             G_CALLBACK (terminal_window_action_new_window), },
    { TERMINAL_WINDOW_ACTION_OPEN_FOLDER,           "<Actions>/terminal-window/open-folder",           "",                          XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Open File Manager Here"),       N_ ("Open the current directory"),        "folder-open",            G_CALLBACK (terminal_window_action_open_folder), },
    { TERMINAL_WINDOW_ACTION_UNDO_CLOSE_TAB,        "<Actions>/terminal-window/undo-close-tab",        "<control><shift>d",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Undo Close Tab"),               NULL,                                     "document-revert",        G_CALLBACK (terminal_window_action_undo_close_tab), },
    { TERMINAL_WINDOW_ACTION_DETACH_TAB,            "<Actions>/terminal-window/detach-tab",            "",                          XFCE_GTK_MENU_ITEM,       N_ ("_Detach Tab"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_detach_tab), },
    { TERMINAL_WINDOW_ACTION_CLOSE_TAB,             "<Actions>/terminal-window/close-tab",             "<control><shift>w",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Close T_ab"),                    NULL,                                     "window-close",           G_CALLBACK (terminal_window_action_close_tab), },
    { TERMINAL_WINDOW_ACTION_CLOSE_OTHER_TABS,      "<Actions>/terminal-window/close-other-tabs",      "",                          XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Close Other Ta_bs"),             NULL,                                     "edit-clear",             G_CALLBACK (terminal_window_action_close_other_tabs), },
    { TERMINAL_WINDOW_ACTION_CLOSE_WINDOW,          "<Actions>/terminal-window/close-window",          "<control><shift>q",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Close _Window"),                 NULL,                                     "application-exit",       G_CALLBACK (terminal_window_action_close_window), },
    { TERMINAL_WINDOW_ACTION_EDIT_MENU,             "<Actions>/terminal-window/edit-menu",             "",                          XFCE_GTK_MENU_ITEM,       N_ ("_Edit"),                         NULL,                                     NULL,                     NULL, },
    { TERMINAL_WINDOW_ACTION_COPY,                  "<Actions>/terminal-window/copy",                  "<control><shift>c",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Copy"),                         N_ ("Copy to clipboard"),                 "edit-copy",              G_CALLBACK (terminal_window_action_copy), },
    { TERMINAL_WINDOW_ACTION_COPY_HTML,             "<Actions>/terminal-window/copy-html",             "",                          XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Copy as _HTML"),                 N_ ("Copy to clipboard as HTML"),         "edit-copy",              G_CALLBACK (terminal_window_action_copy_html), },
    { TERMINAL_WINDOW_ACTION_PASTE,                 "<Actions>/terminal-window/paste",                 "<control><shift>v",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Paste"),                        N_ ("Paste from clipboard"),              "edit-paste",             G_CALLBACK (terminal_window_action_paste), },
    { TERMINAL_WINDOW_ACTION_PASTE_ALT,             "<Actions>/terminal-window/paste-alt",             "<control><shift>Insert",    XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Paste Alt"),                    NULL,                                     "edit-paste-alt",         G_CALLBACK (terminal_window_action_paste), },
    { TERMINAL_WINDOW_ACTION_PASTE_SELECTION,       "<Actions>/terminal-window/paste-selection",       "",                          XFCE_GTK_MENU_ITEM,       N_ ("Paste _Selection"),              NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_paste_selection), },
    { TERMINAL_WINDOW_ACTION_PASTE_SELECTION_ALT,   "<Actions>/terminal-window/paste-selection-alt",   "",                          XFCE_GTK_MENU_ITEM,       N_ ("Paste _Selection Alt"),          NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_paste_selection), },
    { TERMINAL_WINDOW_ACTION_SELECT_ALL,            "<Actions>/terminal-window/select-all",            "<control><shift>a",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Select _All"),                   NULL,                                     "edit-select-all",        G_CALLBACK (terminal_window_action_select_all), },
    { TERMINAL_WINDOW_ACTION_COPY_INPUT,            "<Actions>/terminal-window/copy-input",            "",                          XFCE_GTK_MENU_ITEM,       N_ ("Copy _Input To All Tabs..."),    NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_copy_input), },
    { TERMINAL_WINDOW_ACTION_PREFERENCES,           "<Actions>/terminal-window/preferences",           "",                          XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Pr_eferences..."),               N_ ("Open the preferences dialog"),       "preferences-system",     G_CALLBACK (terminal_window_action_prefs), },
    { TERMINAL_WINDOW_ACTION_VIEW_MENU,             "<Actions>/terminal-window/view-menu",             "",                          XFCE_GTK_MENU_ITEM,       N_ ("_View"),                         NULL,                                     NULL,                     NULL, },
    { TERMINAL_WINDOW_ACTION_ZOOM_IN,               "<Actions>/terminal-window/zoom-in",               "<control>plus",             XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Zoom _In"),                      N_ ("Zoom in with larger font"),          "zoom-in",                G_CALLBACK (terminal_window_action_zoom_in), },
    { TERMINAL_WINDOW_ACTION_ZOOM_IN_ALT,           "<Actions>/terminal-window/zoom-in-alt",           "<control>KP_Add",           XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Zoom In Alt"),                   NULL,                                     "zoom-in-alt",            G_CALLBACK (terminal_window_action_zoom_in), },
    { TERMINAL_WINDOW_ACTION_ZOOM_OUT,              "<Actions>/terminal-window/zoom-out",              "<control>minus",            XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Zoom _Out"),                     N_ ("Zoom out with smaller font"),        "zoom-out",               G_CALLBACK (terminal_window_action_zoom_out), },
    { TERMINAL_WINDOW_ACTION_ZOOM_OUT_ALT,          "<Actions>/terminal-window/zoom-out-alt",          "<control>KP_Subtract",      XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Zoom Out Alt"),                  NULL,                                     "zoom-out-alt",           G_CALLBACK (terminal_window_action_zoom_out), },
    { TERMINAL_WINDOW_ACTION_ZOOM_RESET,            "<Actions>/terminal-window/zoom-reset",            "<control>0",                XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Normal Size"),                  N_ ("Zoom to default size"),              "zoom-original",          G_CALLBACK (terminal_window_action_zoom_reset), },
    { TERMINAL_WINDOW_ACTION_ZOOM_RESET_ALT,        "<Actions>/terminal-window/zoom-reset-alt",        "<control>KP_0",             XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Normal Size Alt"),              NULL,                                     "zoom-original-alt",      G_CALLBACK (terminal_window_action_zoom_reset), },
    { TERMINAL_WINDOW_ACTION_TERMINAL_MENU,         "<Actions>/terminal-window/terminal-menu",         "",                          XFCE_GTK_MENU_ITEM,       N_ ("_Terminal"),                     NULL,                                     NULL,                     NULL, },
    { TERMINAL_WINDOW_ACTION_SET_TITLE,             "<Actions>/terminal-window/set-title",             "<control><shift>s",         XFCE_GTK_MENU_ITEM,       N_ ("_Set Title..."),                 NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_set_title), },
    { TERMINAL_WINDOW_ACTION_SET_TITLE_COLOR,       "<Actions>/terminal-window/set-title-color",       "",                          XFCE_GTK_MENU_ITEM,       N_ ("Set Title Co_lor..."),           NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_set_title_color), },
    { TERMINAL_WINDOW_ACTION_SEARCH,                "<Actions>/terminal-window/search",                "<control><shift>f",         XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Find..."),                      N_ ("Search terminal contents"),          "edit-find",              G_CALLBACK (terminal_window_action_search), },
    { TERMINAL_WINDOW_ACTION_SEARCH_NEXT,           "<Actions>/terminal-window/search-next",           "",                          XFCE_GTK_MENU_ITEM,       N_ ("Find Ne_xt"),                    NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_search_next), },
    { TERMINAL_WINDOW_ACTION_SEARCH_PREV,           "<Actions>/terminal-window/search-prev",           "",                          XFCE_GTK_MENU_ITEM,       N_ ("Find Pre_vious"),                NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_search_prev), },
    { TERMINAL_WINDOW_ACTION_SAVE_CONTENTS,         "<Actions>/terminal-window/save-contents",         "",                          XFCE_GTK_IMAGE_MENU_ITEM, N_ ("Sa_ve Contents..."),             NULL,                                     "document-save-as",       G_CALLBACK (terminal_window_action_save_contents), },
    { TERMINAL_WINDOW_ACTION_RESET,                 "<Actions>/terminal-window/reset",                 "",                          XFCE_GTK_MENU_ITEM,       N_ ("_Reset"),                        NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_reset), },
    { TERMINAL_WINDOW_ACTION_RESET_AND_CLEAR,       "<Actions>/terminal-window/reset-and-clear",       "",                          XFCE_GTK_MENU_ITEM,       N_ ("_Clear Scrollback and Reset"),   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_reset_and_clear), },
    { TERMINAL_WINDOW_ACTION_TABS_MENU,             "<Actions>/terminal-window/tabs-menu",             "",                          XFCE_GTK_MENU_ITEM,       N_ ("T_abs"),                         NULL,                                     NULL,                     NULL, },
    { TERMINAL_WINDOW_ACTION_PREV_TAB,              "<Actions>/terminal-window/prev-tab",              "<control>Page_Up",          XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Previous Tab"),                 N_ ("Switch to previous tab"),            "go-previous",            G_CALLBACK (terminal_window_action_prev_tab), },
    { TERMINAL_WINDOW_ACTION_NEXT_TAB,              "<Actions>/terminal-window/next-tab",              "<control>Page_Down",        XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Next Tab"),                     N_ ("Switch to next tab"),                "go-next",                G_CALLBACK (terminal_window_action_next_tab), },
    { TERMINAL_WINDOW_ACTION_LAST_ACTIVE_TAB,       "<Actions>/terminal-window/last-active-tab",       "",                          XFCE_GTK_MENU_ITEM,       N_ ("Last _Active Tab"),              N_ ("Switch to last active tab"),         NULL,                     G_CALLBACK (terminal_window_action_last_active_tab), },
    { TERMINAL_WINDOW_ACTION_MOVE_TAB_LEFT,         "<Actions>/terminal-window/move-tab-left",         "<control><shift>Page_Up",   XFCE_GTK_MENU_ITEM,       N_ ("Move Tab _Left"),                NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_move_tab_left), },
    { TERMINAL_WINDOW_ACTION_MOVE_TAB_RIGHT,        "<Actions>/terminal-window/move-tab-right",        "<control><shift>Page_Down", XFCE_GTK_MENU_ITEM,       N_ ("Move Tab _Right"),               NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_move_tab_right), },
    { TERMINAL_WINDOW_ACTION_HELP_MENU,             "<Actions>/terminal-window/help-menu",             "",                          XFCE_GTK_MENU_ITEM,       N_ ("_Help"),                         NULL,                                     NULL,                     NULL, },
    { TERMINAL_WINDOW_ACTION_CONTENTS,              "<Actions>/terminal-window/contents",              "F1",                        XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_Contents"),                     N_ ("Display help contents"),             "help-browser",           G_CALLBACK (terminal_window_action_contents), },
    { TERMINAL_WINDOW_ACTION_ABOUT,                 "<Actions>/terminal-window/about",                 "",                          XFCE_GTK_IMAGE_MENU_ITEM, N_ ("_About"),                        NULL,                                     "help-about",             G_CALLBACK (terminal_window_action_about), },
    { TERMINAL_WINDOW_ACTION_ZOOM_MENU,             "<Actions>/terminal-window/zoom-menu",             "",                          XFCE_GTK_MENU_ITEM,       N_ ("_Zoom"),                         NULL,                                     NULL,                     NULL, },
    { TERMINAL_WINDOW_ACTION_TOGGLE_MENUBAR,        "<Actions>/terminal-window/toggle-menubar",        "F10",                       XFCE_GTK_MENU_ITEM,       N_ ("Show Menubar Temporarily"),      "",                                       NULL,                     G_CALLBACK (terminal_window_action_toggle_menubar), },
    { TERMINAL_WINDOW_ACTION_SHOW_MENUBAR,          "<Actions>/terminal-window/show-menubar",          "",                          XFCE_GTK_CHECK_MENU_ITEM, N_ ("Show _Menubar"),                 N_ ("Show/hide the menubar"),             NULL,                     G_CALLBACK (terminal_window_action_show_menubar), },
    { TERMINAL_WINDOW_ACTION_SHOW_TOOLBAR,          "<Actions>/terminal-window/show-toolbar",          "",                          XFCE_GTK_CHECK_MENU_ITEM, N_ ("Show _Toolbar"),                 N_ ("Show/hide the toolbar"),             NULL,                     G_CALLBACK (terminal_window_action_toggle_toolbar), },
    { TERMINAL_WINDOW_ACTION_SHOW_BORDERS,          "<Actions>/terminal-window/show-borders",          "",                          XFCE_GTK_CHECK_MENU_ITEM, N_ ("Show Window _Borders"),          N_ ("Show/hide the window decorations"),  NULL,                     G_CALLBACK (terminal_window_action_toggle_borders), },
    { TERMINAL_WINDOW_ACTION_FULLSCREEN,            "<Actions>/terminal-window/fullscreen",            "F11",                       XFCE_GTK_CHECK_MENU_ITEM, N_ ("_Fullscreen"),                   N_ ("Toggle fullscreen mode"),            "view-fullscreen",        G_CALLBACK (terminal_window_action_fullscreen), },
    { TERMINAL_WINDOW_ACTION_READ_ONLY,             "<Actions>/terminal-window/read-only",             "",                          XFCE_GTK_CHECK_MENU_ITEM, N_ ("_Read-Only"),                    N_ ("Toggle read-only mode"),             NULL,                     G_CALLBACK (terminal_window_action_readonly), },
    { TERMINAL_WINDOW_ACTION_SCROLL_ON_OUTPUT,      "<Actions>/terminal-window/scroll-on-output",      "",                          XFCE_GTK_CHECK_MENU_ITEM, N_ ("Scroll on _Output"),             N_ ("Toggle scroll on output"),           NULL,                     G_CALLBACK (terminal_window_action_scroll_on_output), },
    /* used for changing the accelerator keys via the ShortcutsEditor, the logic is still handle by GtkActionEntries */
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_1,            "<Actions>/terminal-window/goto-tab-1",            "<Alt>1",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 1"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_2,            "<Actions>/terminal-window/goto-tab-2",            "<Alt>2",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 2"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_3,            "<Actions>/terminal-window/goto-tab-3",            "<Alt>3",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 3"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_4,            "<Actions>/terminal-window/goto-tab-4",            "<Alt>4",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 4"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_5,            "<Actions>/terminal-window/goto-tab-5",            "<Alt>5",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 5"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_6,            "<Actions>/terminal-window/goto-tab-6",            "<Alt>6",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 6"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_7,            "<Actions>/terminal-window/goto-tab-7",            "<Alt>7",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 7"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_8,            "<Actions>/terminal-window/goto-tab-8",            "<Alt>8",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 8"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
    { TERMINAL_WINDOW_ACTION_GOTO_TAB_9,            "<Actions>/terminal-window/goto-tab-9",            "<Alt>9",                    XFCE_GTK_CHECK_MENU_ITEM, N_ ("Go to Tab 9"),                   NULL,                                     NULL,                     G_CALLBACK (terminal_window_action_do_nothing), },
};

#define get_action_entry(id) xfce_gtk_get_action_entry_by_id(action_entries,G_N_ELEMENTS(action_entries),id)



G_DEFINE_TYPE_WITH_CODE (TerminalWindow, terminal_window, GTK_TYPE_WINDOW, G_ADD_PRIVATE (TerminalWindow))



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
  gtkwidget_class->map_event = terminal_window_map_event;
  gtkwidget_class->focus_in_event = terminal_window_focus_in_event;
  gtkwidget_class->key_press_event = terminal_window_key_press_event;

  xfce_gtk_translate_action_entries (action_entries, G_N_ELEMENTS (action_entries));

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
  gboolean         always_show_tabs;
  GdkScreen       *screen;
  GdkVisual       *visual;
  GtkStyleContext *context;
  GtkToolItem     *item;

  window->priv = terminal_window_get_instance_private (window);

  window->priv->preferences = terminal_preferences_get ();

  window->priv->font = NULL;
  window->priv->zoom = TERMINAL_ZOOM_LEVEL_DEFAULT;
  window->priv->closed_tabs_list = g_queue_new ();

  /* try to set the rgba colormap so vte can use real transparency */
  screen = gtk_window_get_screen (GTK_WINDOW (window));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual != NULL)
    gtk_widget_set_visual (GTK_WIDGET (window), visual);

  /* required for vte transparency support: see https://bugzilla.gnome.org/show_bug.cgi?id=729884 */
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

  window->priv->accel_group = gtk_accel_group_new ();
  xfce_gtk_accel_map_add_entries (action_entries, G_N_ELEMENTS (action_entries));
  xfce_gtk_accel_group_connect_action_entries (window->priv->accel_group,
                                               action_entries,
                                               G_N_ELEMENTS (action_entries),
                                               window);

  gtk_window_add_accel_group (GTK_WINDOW (window), window->priv->accel_group);

  window->menubar = gtk_menu_bar_new ();
  terminal_window_create_menu (window, TERMINAL_WINDOW_ACTION_FILE_MENU, G_CALLBACK (terminal_window_update_file_menu));
  terminal_window_create_menu (window, TERMINAL_WINDOW_ACTION_EDIT_MENU, G_CALLBACK (terminal_window_update_edit_menu));
  terminal_window_create_menu (window, TERMINAL_WINDOW_ACTION_VIEW_MENU, G_CALLBACK (terminal_window_update_view_menu));
  terminal_window_create_menu (window, TERMINAL_WINDOW_ACTION_TERMINAL_MENU, G_CALLBACK (terminal_window_update_terminal_menu));
  terminal_window_create_menu (window, TERMINAL_WINDOW_ACTION_TABS_MENU, G_CALLBACK (terminal_window_update_tabs_menu));
  terminal_window_create_menu (window, TERMINAL_WINDOW_ACTION_HELP_MENU, G_CALLBACK (terminal_window_update_help_menu));
  gtk_widget_show_all (window->menubar);
  
  window->toolbar = gtk_toolbar_new();
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_NEW_TAB), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_NEW_WINDOW), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), gtk_separator_tool_item_new (), 2);
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_COPY), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_PASTE), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), gtk_separator_tool_item_new (), 5);
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SEARCH), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), gtk_separator_tool_item_new (), 7);
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_PREV_TAB), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_NEXT_TAB), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), gtk_separator_tool_item_new (), 10);
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_PREFERENCES), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), gtk_separator_tool_item_new (), 12);
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_CONTENTS), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  /* expand last separator */
  item = gtk_separator_tool_item_new ();
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_tool_item_set_expand (item, TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), item, 14);
  xfce_gtk_tool_button_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_FULLSCREEN), G_OBJECT (window), GTK_TOOLBAR (window->toolbar));
  gtk_widget_show_all (window->toolbar);

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
  g_signal_connect_swapped (G_OBJECT (window->priv->preferences), "notify::misc-always-show-tabs",
                            G_CALLBACK (terminal_window_notebook_show_tabs), window);

  /* set the notebook group id */
  gtk_notebook_set_group_name (GTK_NOTEBOOK (window->priv->notebook), window_notebook_group);

  /* set notebook tabs style */
  gtk_widget_set_name (window->priv->notebook, NOTEBOOK_NAME);
  terminal_window_update_slim_tabs (window);

  /* signals */
  g_signal_connect (G_OBJECT (window->priv->notebook), "switch-page",
      G_CALLBACK (terminal_window_notebook_page_switched), window);
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

  gtk_box_pack_start (GTK_BOX (window->priv->vbox), window->menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (window->priv->vbox), window->toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (window->priv->vbox), window->priv->notebook, TRUE, TRUE, 0);
  gtk_widget_show_all (window->priv->vbox);

  /* create encoding action */
  window->priv->encoding_action = terminal_encoding_action_new ("set-encoding", _("Set _Encoding"));
  g_signal_connect (G_OBJECT (window->priv->encoding_action), "encoding-changed",
      G_CALLBACK (terminal_window_action_set_encoding), window);

  /* monitor the shortcuts-no-mnemonics setting */
  terminal_window_update_mnemonic_modifier (window);
  g_signal_connect_swapped (G_OBJECT (window->priv->preferences), "notify::shortcuts-no-mnemonics",
                            G_CALLBACK (terminal_window_update_mnemonic_modifier), window);

  window->fullscreen_supported = TRUE;
#ifdef ENABLE_X11
  if (WINDOWING_IS_X11 ())
    {
      /* setup fullscreen mode */
      if (!gdk_x11_screen_supports_net_wm_hint (screen, gdk_atom_intern ("_NET_WM_STATE_FULLSCREEN", FALSE)))
        window->fullscreen_supported = FALSE;
    }
#endif
}



static void
terminal_window_action_prefs_died (gpointer  user_data,
                                   GObject  *where_the_object_was)
{
  TerminalWindow *window = TERMINAL_WINDOW (user_data);

  window->priv->preferences_dialog = NULL;
  window->priv->n_child_windows--;

  if (window->priv->drop_down)
    terminal_util_activate_window (GTK_WINDOW (window));
}



static void
terminal_window_finalize (GObject *object)
{
  TerminalWindow *window = TERMINAL_WINDOW (object);

  /* disconnect prefs signal handlers */
  g_signal_handlers_disconnect_by_func (G_OBJECT (window->priv->preferences),
                                        G_CALLBACK (terminal_window_update_mnemonic_modifier), window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (window->priv->preferences),
                                        G_CALLBACK (terminal_window_notebook_show_tabs), window);

  if (window->priv->preferences_dialog != NULL)
    {
      g_object_weak_unref (G_OBJECT (window->priv->preferences_dialog),
                           terminal_window_action_prefs_died, window);
      gtk_widget_destroy (window->priv->preferences_dialog);
    }
  g_object_unref (G_OBJECT (window->priv->preferences));
  g_object_unref (G_OBJECT (window->priv->accel_group));
  g_object_unref (G_OBJECT (window->priv->encoding_action));

  g_slist_free (window->priv->tabs_menu_actions);
  g_free (window->priv->font);
  g_queue_free_full (window->priv->closed_tabs_list, (GDestroyNotify) terminal_tab_attr_free);

  (*G_OBJECT_CLASS (terminal_window_parent_class)->finalize) (object);
}



static gboolean
terminal_window_delete_event (GtkWidget   *widget,
                              GdkEventAny *event)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);
  GtkWidget      *child;
  gint            n_pages, i, response;

  response = terminal_window_confirm_close (NULL, window);

  /* disconnect remove signal if we're closing the window */
  if (response == CONFIRMED_CLOSE_WINDOW)
    {
      /* disconnect handlers for closing Set Title dialog */
      if (window->priv->title_popover != NULL)
        {
          n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
          for (i = 0; i < n_pages; i++)
            {
              child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), i);
              g_signal_handlers_disconnect_by_func (G_OBJECT (child),
                  G_CALLBACK (title_popover_close), window);
            }
        }

      /* avoid a lot of page remove calls */
      g_signal_handlers_disconnect_by_func (G_OBJECT (window->priv->notebook),
          G_CALLBACK (terminal_window_notebook_page_removed), window);

      /* let gtk close the window */
      if (GTK_WIDGET_CLASS (terminal_window_parent_class)->delete_event != NULL)
        return (*GTK_WIDGET_CLASS (terminal_window_parent_class)->delete_event) (widget, event);

      return FALSE;
    }
  else if (response == CONFIRMED_CLOSE_TAB)
    terminal_window_do_close_tab (window->priv->active, window);

  return TRUE;
}



static gboolean
terminal_window_state_event (GtkWidget           *widget,
                             GdkEventWindowState *event)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);

  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  /* update the fullscreen action if the fullscreen state changed by the wm */
  if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) != 0
      && gtk_widget_get_visible (widget))
    {
      window->is_fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;

      /* update drop-down window geometry, otherwise it'll be incorrect */
      if (!window->is_fullscreen && window->priv->drop_down)
        terminal_window_dropdown_update_geometry (TERMINAL_WINDOW_DROPDOWN (window));
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
    gdk_threads_add_idle (terminal_window_size_pop, window);
}



static gboolean
terminal_window_scroll_event (GtkWidget      *widget,
                              GdkEventScroll *event)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);
  gboolean        mouse_wheel_zoom;
  const guint     modifiers = event->state & gtk_accelerator_get_default_mod_mask ();

  g_object_get (G_OBJECT (window->priv->preferences),
                "misc-mouse-wheel-zoom", &mouse_wheel_zoom, NULL);

  if (mouse_wheel_zoom && modifiers == (GDK_SHIFT_MASK | GDK_CONTROL_MASK)
      && event->direction == GDK_SCROLL_UP)
    {
      terminal_window_action_zoom_in (window);
      return TRUE;
    }

  if (mouse_wheel_zoom && modifiers == (GDK_SHIFT_MASK | GDK_CONTROL_MASK)
      && event->direction == GDK_SCROLL_DOWN)
    {
      terminal_window_action_zoom_out (window);
      return TRUE;
    }

  return FALSE;
}



static gboolean
terminal_window_map_event (GtkWidget   *widget,
                           GdkEventAny *event)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);

  if (window->is_fullscreen)
    gtk_window_fullscreen (GTK_WINDOW (widget));

  return FALSE;
}



static gboolean
terminal_window_focus_in_event (GtkWidget     *widget,
                                GdkEventFocus *event)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);

  /* reset activity indicator for the active tab when focusing window */
  terminal_screen_reset_activity (window->priv->active);

  return (*GTK_WIDGET_CLASS (terminal_window_parent_class)->focus_in_event) (widget, event);
}



static gboolean
terminal_window_key_press_event (GtkWidget   *widget,
                                 GdkEventKey *event)
{
  TerminalWindow *window = TERMINAL_WINDOW (widget);
  
  if (xfce_gtk_handle_tab_accels (event, window->priv->accel_group, window, action_entries, G_N_ELEMENTS (action_entries)) == TRUE)
    return TRUE;

  return (*GTK_WIDGET_CLASS (terminal_window_parent_class)->key_press_event) (widget, event);
}



static gint
terminal_window_confirm_close (TerminalScreen *screen,
                               TerminalWindow *window)
{
  GtkWidget   *dialog;
  GtkWidget   *button;
  GtkWidget   *hbox;
  GtkWidget   *image;
  GtkWidget   *vbox;
  GtkWidget   *label;
  GtkWidget   *checkbox;
  const gchar *title;
  gchar       *message;
  gchar       *markup;
  gint         response;
  gint         i, n_tabs;
  gboolean     confirm_close;

  g_object_get (G_OBJECT (window->priv->preferences), "misc-confirm-close", &confirm_close, NULL);
  if (!confirm_close)
    return (screen != NULL) ? CONFIRMED_CLOSE_TAB : CONFIRMED_CLOSE_WINDOW;

  n_tabs = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
  confirm_close = FALSE;
  for (i = 0; i < n_tabs; ++i)
    {
      TerminalScreen *tab = TERMINAL_SCREEN (gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), i));
      if ((screen == NULL || screen == tab) && terminal_screen_has_foreground_process (tab))
        {
          confirm_close = TRUE;
          break;
        }
    }

  if ((screen != NULL || n_tabs < 2) && !confirm_close)
    return (screen != NULL) ? CONFIRMED_CLOSE_TAB : CONFIRMED_CLOSE_WINDOW;

  dialog = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Warning"));

  button = xfce_gtk_button_new_mixed ("gtk-cancel", _("_Cancel"));
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_CANCEL);

  if (screen == NULL && n_tabs > 1)
    {
      /* closing window with multiple tabs */
      if (confirm_close)
        {
          /* and process running */
          message = g_strdup (_("There are still processes running in some tabs.\n"
                                "Closing this window will kill all of them."));
        }
      else
        {
          /* and no process running */
          message = g_strdup_printf (_("This window has %d tabs open. Closing this window\n"
                                       "will also close all its tabs."), n_tabs);
        }

      title = _("Close all tabs?");
    }
  else
    {
      if (screen != NULL)
        {
          /* closing a tab, and process running */
          message = g_strdup (_("There is still a process running.\n"
                                "Closing this tab will kill it."));
          title = _("Close tab?");
        }
      else
        {
          /* closing a single tab window, and process running */
          message = g_strdup (_("There is still a process running.\n"
                                "Closing this window will kill it."));
          title = _("Close window?");
        }
    }

  if (screen != NULL || n_tabs > 1)
    {
      button = xfce_gtk_button_new_mixed ("window-close", _("Close T_ab"));
      gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_CLOSE);
      gtk_widget_grab_focus (button);
    }

  if (screen == NULL)
    {
      button = xfce_gtk_button_new_mixed ("application-exit", _("Close _Window"));
      gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_YES);
      gtk_widget_grab_focus (button);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), hbox, TRUE, TRUE, 0);

  image = gtk_image_new_from_icon_name ("dialog-warning", GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (image, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  markup = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s", title, message);
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

  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_YES)
    return CONFIRMED_CLOSE_WINDOW;
  if (response == GTK_RESPONSE_CLOSE)
    return CONFIRMED_CLOSE_TAB;
  return CONFIRMED_NONE;
}



static void
terminal_window_size_push (TerminalWindow *window)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));

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

  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

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
  GdkWindow *gdk_window = gtk_widget_get_window (GTK_WIDGET (window));

  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_SCREEN (screen));

  /* required to get the char height/width right */
  if (gtk_widget_get_realized (GTK_WIDGET (screen))
      && gdk_window != NULL
      && (gdk_window_get_state (gdk_window) & (GDK_WINDOW_STATE_FULLSCREEN | WINDOW_STATE_TILED)) == 0
      && !window->priv->drop_down )
    {
      terminal_screen_force_resize_window (screen, GTK_WINDOW (window),
                                           force_grid_width, force_grid_height);
    }
}



static void
terminal_window_update_slim_tabs (TerminalWindow *window)
{
  GdkScreen      *screen = gtk_window_get_screen (GTK_WINDOW (window));
  GtkCssProvider *provider;
  gboolean        slim_tabs;

  g_object_get (G_OBJECT (window->priv->preferences),
                "misc-slim-tabs", &slim_tabs,
                NULL);
  if (slim_tabs)
    {
      provider = gtk_css_provider_new ();
      gtk_style_context_add_provider_for_screen (screen,
                                                 GTK_STYLE_PROVIDER (provider),
                                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      gtk_css_provider_load_from_data (provider, CSS_SLIM_TABS, -1, NULL);
      g_object_unref (provider);
    }
}



static void
terminal_window_update_mnemonic_modifier (TerminalWindow *window)
{
  gboolean no_mnemonics;

  g_object_get (G_OBJECT (window->priv->preferences),
                "shortcuts-no-mnemonics", &no_mnemonics,
                NULL);
  if (no_mnemonics)
    gtk_window_set_mnemonic_modifier (GTK_WINDOW (window), GDK_MODIFIER_MASK & ~GDK_RELEASE_MASK);
  else
    gtk_window_set_mnemonic_modifier (GTK_WINDOW (window), GDK_MOD1_MASK);
}



static void
terminal_window_notebook_page_switched (GtkNotebook     *notebook,
                                        GtkWidget       *page,
                                        guint            page_num,
                                        TerminalWindow  *window)
{
  TerminalScreen *active = TERMINAL_SCREEN (page);
  const gchar    *encoding;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (active == NULL || TERMINAL_IS_SCREEN (active));

  /* only update when really changed */
  if (G_LIKELY (window->priv->active != active))
    {
      /* store last and set new active tab */
      window->priv->last_active = window->priv->active;
      window->priv->active = active;

      /* set the new window title */
      terminal_window_notify_title (active, NULL, window);

      /* reset the activity counter */
      terminal_screen_reset_activity (active);

      /* set charset for menu */
      encoding = terminal_screen_get_encoding (window->priv->active);
      terminal_encoding_action_set_charset (window->priv->encoding_action, encoding);
      terminal_screen_widget_append_accels (active, window->priv->accel_group);
    }
}



static void
terminal_window_close_tab_request (TerminalScreen *screen,
                                   TerminalWindow *window)
{
  if (terminal_window_confirm_close (screen, window) == CONFIRMED_CLOSE_TAB)
    terminal_window_do_close_tab (screen, window);
}



static void
terminal_window_notebook_page_added (GtkNotebook    *notebook,
                                     GtkWidget      *child,
                                     guint           page_num,
                                     TerminalWindow *window)
{
  TerminalScreen *screen = TERMINAL_SCREEN (child);
  glong           w, h;

  g_return_if_fail (TERMINAL_IS_SCREEN (child));
  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (GTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (window->priv->notebook == GTK_WIDGET (notebook));

  /* connect screen signals */
  g_signal_connect (G_OBJECT (screen), "get-context-menu",
      G_CALLBACK (terminal_window_get_context_menu), window);
  g_signal_connect (G_OBJECT (screen), "notify::title",
      G_CALLBACK (terminal_window_notify_title), window);
  g_signal_connect (G_OBJECT (screen), "close-tab-request",
      G_CALLBACK (terminal_window_close_tab_request), window);
  g_signal_connect (G_OBJECT (screen), "drag-data-received",
      G_CALLBACK (terminal_window_notebook_drag_data_received), window);

  /* release to the grid size applies */
  gtk_widget_realize (GTK_WIDGET (screen));

  /* match zoom and font */
  if (window->priv->font || window->priv->zoom != TERMINAL_ZOOM_LEVEL_DEFAULT)
    terminal_screen_update_font (screen);

  if (G_LIKELY (window->priv->active != NULL))
    {
      /* match the size of the active screen */
      terminal_screen_get_size (window->priv->active, &w, &h);
      terminal_screen_set_size (screen, w, h);

      /* show the tabs when needed */
      terminal_window_notebook_show_tabs (window);
    }
  else if (G_UNLIKELY (window->priv->drop_down))
    {
      /* try to calculate a decent grid size based on the info we have now */
      terminal_window_dropdown_get_size (TERMINAL_WINDOW_DROPDOWN (window), screen, &w, &h);
      terminal_screen_set_size (screen, w, h);
    }

  terminal_screen_widget_append_accels (TERMINAL_SCREEN (child), window->priv->accel_group);

  /* update the go-to accelerators */
  terminal_window_update_tabs_menu (window, window->priv->tabs_menu);
}



static void
terminal_window_notebook_page_removed (GtkNotebook    *notebook,
                                       GtkWidget      *child,
                                       guint           page_num,
                                       TerminalWindow *window)
{
  GtkWidget *new_page;
  gint       new_page_num;
  gint       npages;

  g_return_if_fail (TERMINAL_IS_SCREEN (child));
  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  /* unset the go menu item */
  g_object_set_qdata (G_OBJECT (child), tabs_menu_action_quark, NULL);

  /* disconnect signals */
  g_signal_handlers_disconnect_by_func (G_OBJECT (child),
      terminal_window_get_context_menu, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (child),
      terminal_window_notify_title, window);
  g_signal_handlers_disconnect_by_func (G_OBJECT (child),
      terminal_window_close_tab_request, window);
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

  /* clear last_active tab if it has just been closed */
  if (child == GTK_WIDGET (window->priv->last_active))
    window->priv->last_active = NULL;

  /* show the tabs when needed */
  terminal_window_notebook_show_tabs (window);

  /* send a signal about switching to another tab */
  new_page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (window->priv->notebook));
  new_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), new_page_num);
  terminal_window_notebook_page_switched (notebook, new_page, new_page_num, window);

  /* update the go-to accelerators */
  terminal_window_update_tabs_menu (window, window->priv->tabs_menu);
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

  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), FALSE);

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
              terminal_window_action_set_title (window);
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
            terminal_window_close_tab_request (TERMINAL_SCREEN (page), window);
        }
      else
        {
          /* update the current tab before we show the menu */
          gtk_notebook_set_current_page (notebook, page_num);

          /* show the tab menu */
          menu = gtk_menu_new ();
          terminal_window_update_tabs_menu (window, menu);
          gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
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
  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), FALSE);

  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_focus (window->priv->active);

  return FALSE;
}



static gboolean
terminal_window_notebook_scroll_event (GtkNotebook    *notebook,
                                       GdkEventScroll *event,
                                       TerminalWindow *window)
{
  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), FALSE);

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

    default: /* GDK_SCROLL_SMOOTH */
      switch (gtk_notebook_get_tab_pos (notebook)) {
        case GTK_POS_LEFT:
        case GTK_POS_RIGHT:
          if (event->delta_y > 0)
            gtk_notebook_next_page (notebook);
          else if (event->delta_y < 0)
            gtk_notebook_prev_page (notebook);
          break;

        default: /* GTK_POS_TOP or GTK_POS_BOTTOM */
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
                                             guint             time,
                                             TerminalWindow   *window)
{
  GtkWidget *notebook;
  GtkWidget *screen;
  gint       i, n_pages;
  gboolean   succeed = FALSE;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_SCREEN (widget));

  /* check */
  if (G_LIKELY (info == TARGET_GTK_NOTEBOOK_TAB))
    {
      /* get the source notebook (other window) */
      notebook = gtk_drag_get_source_widget (context);
      g_return_if_fail (GTK_IS_NOTEBOOK (notebook));

      /* get the dragged screen: selection_data's data is a (GtkWidget **) screen */
      memcpy (&screen, gtk_selection_data_get_data (selection_data), sizeof (screen));
      if (!TERMINAL_IS_SCREEN (screen))
        goto leave;

      /* leave if we dropped in the same screen and there is only one
       * page in the notebook (window will close before we insert) */
      if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) < 2 && screen == widget)
        goto leave;

      /* figure out where to insert the tab in the notebook */
      n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
      for (i = 0; i < n_pages; i++)
        {
          /* get the child label */
          GtkAllocation allocation;
          GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), i);
          GtkWidget *label = gtk_notebook_get_tab_label (GTK_NOTEBOOK (window->priv->notebook), child);
          gtk_widget_get_allocation (label, &allocation);

          /* break if we have a matching drop position */
          if (x < (allocation.x + allocation.width / 2))
            break;
        }

      if (notebook == window->priv->notebook)
        {
          /* if we're in the same notebook, don't risk anything and do a
           * simple reorder */
          gtk_notebook_reorder_child (GTK_NOTEBOOK (notebook), screen, i);
        }
      else
        {
          /* take a reference */
          g_object_ref (G_OBJECT (screen));
          g_object_ref (G_OBJECT (window));

          /* remove the document from the source notebook */
          gtk_notebook_detach_tab (GTK_NOTEBOOK (notebook), screen);

          /* add the screen to the new window */
          terminal_window_add (window, TERMINAL_SCREEN (screen));

          /* move the child to the correct position */
          gtk_notebook_reorder_child (GTK_NOTEBOOK (window->priv->notebook), screen, i);

          /* release reference */
          g_object_unref (G_OBJECT (screen));
          g_object_unref (G_OBJECT (window));
        }

      /* looks like everything worked out */
      succeed = TRUE;
    }

  /* finish the drag */
leave:
  gtk_drag_finish (context, succeed, FALSE, time);
}



static GtkNotebook*
terminal_window_notebook_create_window (GtkNotebook    *notebook,
                                        GtkWidget      *child,
                                        gint            x,
                                        gint            y,
                                        TerminalWindow *window)
{
  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);
  g_return_val_if_fail (TERMINAL_IS_SCREEN (child), NULL);
  g_return_val_if_fail (notebook == GTK_NOTEBOOK (window->priv->notebook), NULL);

  /* only create new window when there are more then 2 tabs (bug #2686) */
  if (gtk_notebook_get_n_pages (notebook) >= 2)
    {
      /* take a reference */
      g_object_ref (G_OBJECT (child));

      /* remove screen from active window */
      gtk_notebook_detach_tab (notebook, child);

      /* create new window with the screen */
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW_WITH_SCREEN], 0, TERMINAL_SCREEN (child), x, y);

      /* release our reference */
      g_object_unref (G_OBJECT (child));
    }

  return NULL;
}



static GtkWidget*
terminal_window_get_context_menu (TerminalScreen  *screen,
                                  TerminalWindow  *window)
{
  GtkWidget *context_menu;
  GList     *children, *lp;

  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);

  context_menu = g_object_new (GTK_TYPE_MENU, NULL);

  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_NEW_TAB), G_OBJECT (window), GTK_MENU_SHELL (context_menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_NEW_WINDOW), G_OBJECT (window), GTK_MENU_SHELL (context_menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_OPEN_FOLDER), G_OBJECT (window), GTK_MENU_SHELL (context_menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (context_menu));

  terminal_window_menu_add_section (window, context_menu, MENU_SECTION_COPY | MENU_SECTION_PASTE, FALSE);

  xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SHOW_MENUBAR), G_OBJECT (window), gtk_widget_is_visible (window->menubar), GTK_MENU_SHELL (context_menu));
  xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_FULLSCREEN), G_OBJECT (window), window->is_fullscreen, GTK_MENU_SHELL (context_menu));
  xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_READ_ONLY), G_OBJECT (window), !terminal_screen_get_input_enabled (window->priv->active), GTK_MENU_SHELL (context_menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (context_menu));

  terminal_window_menu_add_section (window, context_menu, MENU_SECTION_ZOOM | MENU_SECTION_SIGNAL, TRUE);

  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SAVE_CONTENTS), G_OBJECT (window), GTK_MENU_SHELL (context_menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (context_menu));

  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_PREFERENCES), G_OBJECT (window), GTK_MENU_SHELL (context_menu));

  /* hide accel labels */
  children = gtk_container_get_children (GTK_CONTAINER (context_menu));
  for (lp = children; lp != NULL; lp = lp->next)
    xfce_gtk_menu_item_set_accel_label (lp->data, NULL);
  g_list_free (children);

  return context_menu;
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



static gchar*
terminal_window_get_working_directory (TerminalWindow *window)
{
  gchar    *default_dir;
  gboolean  use_default_dir;

  g_object_get (G_OBJECT (window->priv->preferences),
                "use-default-working-dir", &use_default_dir,
                "default-working-dir", &default_dir,
                NULL);

  if (use_default_dir && g_strcmp0 (default_dir, "") != 0)
    return default_dir;

  if (G_LIKELY (window->priv->active != NULL))
    return g_strdup (terminal_screen_get_working_directory (window->priv->active));

  return NULL;
}



static gboolean
terminal_window_action_new_tab (TerminalWindow *window)
{
  TerminalScreen *terminal = TERMINAL_SCREEN (g_object_new (TERMINAL_TYPE_SCREEN, NULL));
  gchar          *directory = terminal_window_get_working_directory (window);

  if (directory != NULL)
    {
      terminal_screen_set_working_directory (terminal, directory);
      g_free (directory);
    }

  terminal_window_add (window, terminal);
  terminal_screen_launch_child (terminal);

  return TRUE;
}



static gboolean
terminal_window_action_new_window (TerminalWindow *window)
{
  gchar *directory = terminal_window_get_working_directory (window);

  if (directory != NULL)
    {
      g_signal_emit (G_OBJECT (window), window_signals[NEW_WINDOW], 0, directory);
      g_free (directory);
    }

  return TRUE;
}



static gboolean
terminal_window_action_open_folder (TerminalWindow *window)
{
  GError *error = NULL;
  const gchar *directory = terminal_screen_get_working_directory (window->priv->active);

  if (directory != NULL)
    {
      gchar *dir = g_shell_quote (directory);
      gchar *cmd = g_strdup_printf ("xdg-open %s", dir);

      if (!g_spawn_command_line_async (cmd, &error))
        {
          xfce_dialog_show_error (GTK_WINDOW (window), error,
                                  _("Failed to open directory"));
          g_error_free (error);
        }

      g_free (cmd);
      g_free (dir);
    }

  return TRUE;
}



static gboolean
terminal_window_action_undo_close_tab (TerminalWindow *window)
{
  TerminalScreen  *terminal;
  TerminalTabAttr *tab_attr;
  GtkWidget       *current = GTK_WIDGET (window->priv->active);

  if (G_UNLIKELY (g_queue_is_empty (window->priv->closed_tabs_list)))
    return TRUE;

  /* get info on the last closed tab and remove it from the list */
  tab_attr = g_queue_pop_tail (window->priv->closed_tabs_list);

  terminal = terminal_screen_new (tab_attr, window->priv->grid_width, window->priv->grid_height);
  terminal_window_add (window, terminal);

  /* restore tab title color */
  if (tab_attr->color_title != NULL)
    terminal_screen_set_custom_title_color (terminal, tab_attr->color_title);

  /* restore tab position */
  gtk_notebook_reorder_child (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (terminal), tab_attr->position);

  /* restore tab focus if the unclosed one wasn't active when it was closed */
  if (!tab_attr->active)
    {
      gint page_num = gtk_notebook_page_num (GTK_NOTEBOOK (window->priv->notebook), current);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook), page_num);
    }

  /* free info */
  terminal_tab_attr_free (tab_attr);

  terminal_screen_launch_child (terminal);

  return TRUE;
}



static gboolean
terminal_window_action_detach_tab (TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_window_notebook_create_window (GTK_NOTEBOOK (window->priv->notebook),
                                            GTK_WIDGET (window->priv->active),
                                            -1, -1, window);
  return TRUE;
}



static gboolean
terminal_window_action_close_tab (TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_window_close_tab_request (window->priv->active, window);
  return TRUE;
}



static gboolean
terminal_window_action_close_other_tabs (TerminalWindow *window)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (window->priv->notebook);
  gint         npages, n;

  /* move current page to the beginning */
  gtk_notebook_reorder_child (notebook, GTK_WIDGET (window->priv->active), 0);

  /* remove the others */
  npages = gtk_notebook_get_n_pages (notebook);
  for (n = npages - 1; n > 0; n--)
    terminal_window_close_tab_request (TERMINAL_SCREEN (gtk_notebook_get_nth_page (notebook, n)), window);

  return TRUE;
}



static gboolean
terminal_window_action_close_window (TerminalWindow *window)
{
  /* this will invoke the "delete-event" handler */
  gtk_window_close (GTK_WINDOW (window));
  return TRUE;
}



static gboolean
terminal_window_action_copy (TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_copy_clipboard (window->priv->active);
  return TRUE;
}



static gboolean
terminal_window_action_copy_html (TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_copy_clipboard_html (window->priv->active);
  return TRUE;
}



static gboolean
terminal_window_action_paste (TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_paste_clipboard (window->priv->active);
  return TRUE;
}



static gboolean
terminal_window_action_paste_selection (TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_paste_primary (window->priv->active);
  return TRUE;
}



static gboolean
terminal_window_action_select_all (TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_select_all (window->priv->active);
  return TRUE;
}



static void
copy_input_popover_close (GtkWidget      *popover,
                          TerminalWindow *window)
{
  /* need for hiding on focus */
  if (window->priv->drop_down)
    terminal_util_activate_window (GTK_WINDOW (window));

  /* close the dialog */
  gtk_widget_destroy (popover);

  /* focus the terminal */
  if (TERMINAL_IS_SCREEN (window->priv->active))
    terminal_screen_focus (window->priv->active);
}



static void
copy_input_popover_do_copy (GtkWidget *popover,
                            GtkWidget *entry)
{
  TerminalWindow *window = TERMINAL_WINDOW (gtk_widget_get_toplevel (popover));
  GtkNotebook    *notebook = GTK_NOTEBOOK (window->priv->notebook);
  gint            n, npages = gtk_notebook_get_n_pages (notebook);

  /* copy the input to all tabs */
  for (n = 0; n < npages; n++)
    {
      TerminalScreen *screen = TERMINAL_SCREEN (gtk_notebook_get_nth_page (notebook, n));
      terminal_screen_feed_text (screen, gtk_entry_get_text (GTK_ENTRY (entry)));
    }
}



static gboolean
terminal_window_action_copy_input (TerminalWindow *window)
{
  GtkWidget *popover, *button, *box, *label, *entry;

  popover = gtk_popover_new (GTK_WIDGET (window->menubar));
  gtk_popover_set_position (GTK_POPOVER (popover), GTK_POS_BOTTOM);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (box), 6);
  gtk_container_add (GTK_CONTAINER (popover), box);

  label = gtk_label_new_with_mnemonic (_("Copy _Input:"));
  gtk_container_add (GTK_CONTAINER (box), label);

  entry = gtk_entry_new ();
  gtk_widget_set_tooltip_text (entry, _("Enter the text to be copied to all tabs"));
  gtk_container_add (GTK_CONTAINER (box), entry);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (copy_input_popover_do_copy), entry);

  button = gtk_button_new_from_icon_name ("edit-copy-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text (button, _("Copy input"));
  gtk_container_add (GTK_CONTAINER (box), button);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (copy_input_popover_do_copy), entry);

  g_signal_connect (G_OBJECT (popover), "closed", G_CALLBACK (copy_input_popover_close), window);

  gtk_widget_show_all (popover);

  return TRUE;
}



static gboolean
terminal_window_action_prefs (TerminalWindow *window)
{
  if (window->priv->preferences_dialog == NULL)
    {
      window->priv->preferences_dialog = terminal_preferences_dialog_new (window->priv->drop_down, window->priv->drop_down);
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

  return TRUE;
}



static void
hide_menubar (GtkMenuShell *menubar)
{
  g_signal_handlers_disconnect_by_func (menubar, hide_menubar, NULL);
  gtk_widget_hide (GTK_WIDGET (menubar));
}



static gboolean
terminal_window_action_toggle_menubar (TerminalWindow *window)
{
  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  if (!gtk_widget_get_visible (window->menubar))
    {
      gboolean shortcuts_no_menukey;
      g_object_get (G_OBJECT (window->priv->preferences),
                    "shortcuts-no-menukey", &shortcuts_no_menukey,
                    NULL);
      if (!shortcuts_no_menukey)
        {
          GList *items = gtk_container_get_children (GTK_CONTAINER (window->menubar));
          gtk_widget_show (window->menubar);
          gtk_menu_shell_select_first (GTK_MENU_SHELL (window->menubar), TRUE);
          g_signal_connect (window->menubar, "deactivate", G_CALLBACK (hide_menubar), NULL);
          g_list_free (items);
          return TRUE;
        }
    }

  return FALSE;
}



static gboolean
terminal_window_action_show_menubar (TerminalWindow *window)
{
  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  terminal_window_size_push (window);

  if (gtk_widget_is_visible (window->menubar) == FALSE)
    gtk_widget_show (window->menubar);
  else
    gtk_widget_hide (window->menubar);

  terminal_window_size_pop (window);

  return TRUE;
}



static gboolean
terminal_window_action_toggle_toolbar (TerminalWindow  *window)
{
  terminal_window_size_push (window);

  if (gtk_widget_is_visible (window->toolbar) == FALSE)
    gtk_widget_show (window->toolbar);
  else
    gtk_widget_hide (window->toolbar);

  terminal_window_size_pop (window);
  return TRUE;
}



static gboolean
terminal_window_action_toggle_borders (TerminalWindow  *window)
{
  gtk_window_set_decorated (GTK_WINDOW (window), !gtk_window_get_decorated (GTK_WINDOW (window)));
  return TRUE;
}



static gboolean
terminal_window_action_fullscreen (TerminalWindow  *window)
{
  if (gtk_widget_get_visible (GTK_WIDGET (window)))
    {
      window->is_fullscreen = !window->is_fullscreen;
      if (window->is_fullscreen)
        gtk_window_fullscreen (GTK_WINDOW (window));
      else
        gtk_window_unfullscreen (GTK_WINDOW (window));
    }
  return TRUE;
}



static gboolean
terminal_window_action_readonly (TerminalWindow  *window)
{
  g_return_val_if_fail (window->priv->active != NULL, FALSE);

  terminal_screen_set_input_enabled (window->priv->active, !terminal_screen_get_input_enabled (window->priv->active));
  return TRUE;
}



static gboolean
terminal_window_action_scroll_on_output (TerminalWindow  *window)
{
  g_return_val_if_fail (window->priv->active != NULL, FALSE);

  terminal_screen_set_scroll_on_output (window->priv->active, !terminal_screen_get_scroll_on_output (window->priv->active));
  return TRUE;
}



static gboolean
terminal_window_action_zoom_in (TerminalWindow *window)
{
  g_return_val_if_fail (window->priv->active != NULL, FALSE);

  if (window->priv->zoom < TERMINAL_ZOOM_LEVEL_MAXIMUM)
    {
      ++window->priv->zoom;
      terminal_window_zoom_update_screens (window);
    }
  return TRUE;
}



static gboolean
terminal_window_action_zoom_out (TerminalWindow *window)
{
  g_return_val_if_fail (window->priv->active != NULL, FALSE);

  if (window->priv->zoom > TERMINAL_ZOOM_LEVEL_MINIMUM)
    {
      --window->priv->zoom;
      terminal_window_zoom_update_screens (window);
    }
  return TRUE;
}



static gboolean
terminal_window_action_zoom_reset (TerminalWindow *window)
{
  g_return_val_if_fail (window->priv->active != NULL, FALSE);

  if (window->priv->zoom != TERMINAL_ZOOM_LEVEL_DEFAULT)
    {
      window->priv->zoom = TERMINAL_ZOOM_LEVEL_DEFAULT;
      terminal_window_zoom_update_screens (window);
    }
  return TRUE;
}



static gboolean
terminal_window_action_prev_tab (TerminalWindow *window)
{
  if (terminal_window_can_go_left (window) == TRUE)
    {
      terminal_window_switch_tab (GTK_NOTEBOOK (window->priv->notebook), TRUE);
      return TRUE;
    }
  else
    return FALSE;
}



static gboolean
terminal_window_action_next_tab (TerminalWindow *window)
{
  if (terminal_window_can_go_right (window) == TRUE)
    {
      terminal_window_switch_tab (GTK_NOTEBOOK (window->priv->notebook), FALSE);
      return TRUE;
    }
  else
    return FALSE;
}



static gboolean
terminal_window_action_last_active_tab (TerminalWindow *window)
{
  if (window->priv->last_active != NULL)
    {
      GtkNotebook *notebook = GTK_NOTEBOOK (window->priv->notebook);
      gint page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (window->priv->last_active));
      gtk_notebook_set_current_page (notebook, page_num);
    }

  return TRUE;
}



static gboolean
terminal_window_action_move_tab_left (TerminalWindow *window)
{
  if (terminal_window_can_go_left (window) == TRUE)
    {
      terminal_window_move_tab (GTK_NOTEBOOK (window->priv->notebook), TRUE);
      return TRUE;
    }
  else
    return FALSE;
}



static gboolean
terminal_window_action_move_tab_right (TerminalWindow *window)
{
  if (terminal_window_can_go_right (window) == TRUE)
    {
      terminal_window_move_tab (GTK_NOTEBOOK (window->priv->notebook), FALSE);
      return TRUE;
    }
  else
    return FALSE;
}



static gboolean
terminal_window_action_goto_tab (GtkRadioAction *action,
                                 GtkNotebook    *notebook)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), FALSE);
  g_return_val_if_fail (GTK_IS_RADIO_ACTION (action), FALSE);

  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
    {
      /* switch to the new page */
      gint page = gtk_radio_action_get_current_value (action);
      gtk_notebook_set_current_page (notebook, page);
    }
G_GNUC_END_IGNORE_DEPRECATIONS

  return TRUE;
}



static void
title_popover_close (GtkWidget      *popover,
                     TerminalWindow *window)
{
  g_return_if_fail (window->priv->title_popover != NULL);

  /* need for hiding on focus */
  if (window->priv->drop_down)
    terminal_util_activate_window (GTK_WINDOW (window));

  /* close the dialog */
  gtk_widget_destroy (window->priv->title_popover);
  window->priv->title_popover = NULL;

  /* focus the terminal: bug #13754 */
  if (TERMINAL_IS_SCREEN (window->priv->active))
    terminal_screen_focus (window->priv->active);
}



static void
title_popover_help (GtkWidget      *popover,
                    TerminalWindow *window)
{
  /* open the "Set Title" paragraph in the "Usage" section */
  xfce_dialog_show_help (GTK_WINDOW (window), "terminal", "usage#to_change_the_terminal_title", NULL);
}



static void
title_popover_clear (GtkWidget            *entry,
                     GtkEntryIconPosition  icon_pos)
{
  if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
    gtk_entry_set_text (GTK_ENTRY (entry), "");
}



static gboolean
terminal_window_action_set_title (TerminalWindow *window)
{
  GtkWidget *button;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *entry;

  g_return_val_if_fail (window->priv->active != NULL, FALSE);

  if (window->priv->title_popover == NULL)
    {
      if (gtk_notebook_get_show_tabs (GTK_NOTEBOOK (window->priv->notebook)))
        {
          window->priv->title_popover =
              gtk_popover_new (gtk_notebook_get_tab_label (GTK_NOTEBOOK (window->priv->notebook),
                                                           GTK_WIDGET (window->priv->active)));
        }
      else
        {
          window->priv->title_popover = gtk_popover_new (GTK_WIDGET (window->menubar));
          gtk_popover_set_position (GTK_POPOVER (window->priv->title_popover), GTK_POS_BOTTOM);
        }

      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_container_set_border_width (GTK_CONTAINER (box), 6);
      gtk_container_add (GTK_CONTAINER (window->priv->title_popover), box);

      label = gtk_label_new_with_mnemonic (_("_Title:"));
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);

      entry = gtk_entry_new ();
      gtk_widget_set_tooltip_text (entry, _("Enter the title for the current terminal tab"));
      gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
      gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, "edit-clear");
      gtk_entry_set_icon_tooltip_text (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, _("Reset"));
      g_signal_connect (G_OBJECT (entry), "icon-release", G_CALLBACK (title_popover_clear), NULL);
      g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (title_popover_close), window);

      g_object_bind_property (G_OBJECT (window->priv->active), "custom-title",
                              G_OBJECT (entry), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

      button = gtk_button_new_from_icon_name ("help-browser", GTK_ICON_SIZE_BUTTON);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_widget_set_tooltip_text (button, _("Help"));
      g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (title_popover_help), window);
      gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

      button = gtk_button_new_from_icon_name ("window-close-symbolic", GTK_ICON_SIZE_BUTTON);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_widget_set_tooltip_text (button, _("Close"));
      g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (title_popover_close), window);
      gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

      g_signal_connect (G_OBJECT (window->priv->title_popover), "closed",
                        G_CALLBACK (title_popover_close), window);
    }

  gtk_widget_show_all (window->priv->title_popover);

  return TRUE;
}



static gboolean
terminal_window_action_set_title_color (TerminalWindow *window)
{
  GtkWidget *dialog;
  gchar     *color_string;
  GdkRGBA    color;
  int        response;

  dialog = gtk_color_chooser_dialog_new (_("Choose title color"), GTK_WINDOW (window));
  gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Reset"), GTK_RESPONSE_NO);
  response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_OK)
    {
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (dialog), &color);
      color_string = gdk_rgba_to_string (&color);
      terminal_screen_set_custom_title_color (window->priv->active, color_string);
      g_free (color_string);
    }
  else if (response == GTK_RESPONSE_NO)
    terminal_screen_set_custom_title_color (window->priv->active, NULL);

  gtk_widget_destroy (dialog);

  return TRUE;
}



static void
terminal_window_action_search_response (GtkWidget      *dialog,
                                        gint            response_id,
                                        TerminalWindow *window)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_SEARCH_DIALOG (dialog));
  g_return_if_fail (TERMINAL_IS_SCREEN (window->priv->active));
  g_return_if_fail (window->priv->search_dialog == dialog);

  if (response_id == TERMINAL_RESPONSE_SEARCH_NEXT)
    terminal_window_action_search_next (window);
  else if (response_id == TERMINAL_RESPONSE_SEARCH_PREV)
    terminal_window_action_search_prev (window);
  else
    {
      /* need for hiding on focus */
      if (window->priv->drop_down)
        terminal_util_activate_window (GTK_WINDOW (window));

      /* hide dialog */
      window->priv->n_child_windows--;
      gtk_widget_hide (dialog);
    }
}



static gboolean
terminal_window_action_search (TerminalWindow *window)
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

  return TRUE;
}



static gboolean
prepare_regex (TerminalWindow *window)
{
  VteRegex *regex;
  GError   *error = NULL;
  gboolean  wrap_around;

  /* may occur if next/prev actions are activated by keyboard shortcut */
  if (window->priv->search_dialog == NULL)
    return FALSE;

  regex = terminal_search_dialog_get_regex (TERMINAL_SEARCH_DIALOG (window->priv->search_dialog), &error);
  if (G_LIKELY (error == NULL))
    {
      wrap_around = terminal_search_dialog_get_wrap_around (TERMINAL_SEARCH_DIALOG (window->priv->search_dialog));
      terminal_screen_search_set_gregex (window->priv->active, regex, wrap_around);
      if (regex != NULL)
        vte_regex_unref (regex);

      return TRUE;
    }

  xfce_dialog_show_error (GTK_WINDOW (window->priv->search_dialog), error,
                          _("Failed to create the regular expression"));
  g_error_free (error);

  return FALSE;
}



static gboolean
terminal_window_action_search_next (TerminalWindow *window)
{
  if (prepare_regex (window))
    terminal_screen_search_find_next (window->priv->active);
  return TRUE;
}



static gboolean
terminal_window_action_search_prev (TerminalWindow *window)
{
  if (prepare_regex (window))
    terminal_screen_search_find_previous (window->priv->active);
  return TRUE;
}



static gboolean
terminal_window_action_save_contents (TerminalWindow *window)
{
  GtkWidget     *dialog;
  GFile         *file;
  GOutputStream *stream;
  GError        *error = NULL;
  gchar         *filename_uri;
  gint           response;

  g_return_val_if_fail (window->priv->active != NULL, FALSE);

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
      return TRUE;
    }

  filename_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
  gtk_widget_destroy (dialog);

  if (filename_uri == NULL)
    return TRUE;

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
  return TRUE;
}



static gboolean
terminal_window_action_reset (TerminalWindow *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    terminal_screen_reset (window->priv->active, FALSE);
  return TRUE;
}



static gboolean
terminal_window_action_reset_and_clear (TerminalWindow  *window)
{
  if (G_LIKELY (window->priv->active != NULL))
    {
      terminal_screen_reset (window->priv->active, TRUE);
    }
  return TRUE;
}



static void
terminal_window_action_send_signal (SendSignalData *data)
{
  if (G_LIKELY (data->window->priv->active != NULL))
    terminal_screen_send_signal (data->window->priv->active, data->signal);
}



static gboolean
terminal_window_action_contents (TerminalWindow  *window)
{
  /* don't hide the drop-down terminal */
  if (TERMINAL_IS_WINDOW_DROPDOWN (window))
    terminal_window_dropdown_ignore_next_focus_out_event (TERMINAL_WINDOW_DROPDOWN (window));

  /* open the Terminal user manual */
  xfce_dialog_show_help (GTK_WINDOW (window), "terminal", NULL, NULL);
  return TRUE;
}



static gboolean
terminal_window_action_about (TerminalWindow *window)
{
  /* don't hide the drop-down terminal */
  if (TERMINAL_IS_WINDOW_DROPDOWN (window))
    terminal_window_dropdown_ignore_next_focus_out_event (TERMINAL_WINDOW_DROPDOWN (window));

  /* display the about dialog */
  terminal_util_show_about_dialog (GTK_WINDOW (window));

  return TRUE;
}



/**
 * terminal_window_action_do_nothing:
 *
 * Used as a dummy value for go-to tab accelerators XfceGtkActionEntries.
 * If an entry doesn't have a Callback it won't appear in the XfceShortcutsEditor.
 * go-to tab accelerator entries don't have an actual tab because all their logic still uses
 * GtkActionEntries. Until that is dealt with we use their XfceGtkActionEntries only for changing the shortcuts via the editor.
 **/
static gboolean
terminal_window_action_do_nothing (TerminalWindow *window)
{
  return FALSE;
}



static void
terminal_window_zoom_update_screens (TerminalWindow *window)
{
  gint            npages, n;
  TerminalScreen *screen;

  g_return_if_fail (GTK_IS_NOTEBOOK (window->priv->notebook));

  /* walk the tabs */
  npages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
  for (n = 0; n < npages; n++)
    {
      screen = TERMINAL_SCREEN (gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), n));
      terminal_screen_update_font (screen);
    }
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
terminal_window_do_close_tab (TerminalScreen *screen,
                              TerminalWindow *window)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (window->priv->notebook);

  /* store attrs of the tab being closed */
  TerminalTabAttr *tab_attr = terminal_tab_attr_new ();
  tab_attr->active = (screen == window->priv->active);
  tab_attr->position = gtk_notebook_page_num (notebook, GTK_WIDGET (screen));
  tab_attr->directory = g_strdup (terminal_screen_get_working_directory (screen));
  if (IS_STRING (terminal_screen_get_custom_title (screen)))
    tab_attr->title = g_strdup (terminal_screen_get_custom_title (screen));
  if (IS_STRING (terminal_screen_get_custom_fg_color (screen)))
    tab_attr->color_text = g_strdup (terminal_screen_get_custom_fg_color (screen));
  if (IS_STRING (terminal_screen_get_custom_bg_color (screen)))
    tab_attr->color_bg = g_strdup (terminal_screen_get_custom_bg_color (screen));
  if (IS_STRING (terminal_screen_get_custom_title_color (screen)))
    tab_attr->color_title = g_strdup (terminal_screen_get_custom_title_color (screen));
  g_queue_push_tail (window->priv->closed_tabs_list, tab_attr);

  /* switch to the previously active tab */
  if (screen == window->priv->active && window->priv->last_active != NULL)
    {
      gint page_num = gtk_notebook_page_num (notebook, GTK_WIDGET (window->priv->last_active));
      gtk_notebook_set_current_page (notebook, page_num);
    }

  gtk_widget_destroy (GTK_WIDGET (screen));

  /* reconnect the accels of the active terminal */
  if (screen != window->priv->active)
    terminal_screen_widget_append_accels (window->priv->active, window->priv->accel_group);
}



static gint
terminal_window_get_workspace (TerminalWindow *window)
{
#ifdef ENABLE_X11
  GdkWindow    *gdk_window;
  gint          workspace = -1;
  GdkDisplay   *gdk_display;
  Display      *display;
  Window        xwin;
  Atom          property;
  Atom          actual_type = None;
  int           actual_format = 0;
  unsigned long nitems = 0;
  unsigned long bytes_after = 0;
  unsigned char *data_p = NULL;

  gdk_window = gtk_widget_get_window (GTK_WIDGET (window));
  if (gdk_window == NULL)
    return -1;

  if (WINDOWING_IS_X11 ())
    {
      gdk_display = gtk_widget_get_display (GTK_WIDGET (window));
      gdk_x11_display_error_trap_push (gdk_display);

      display = gdk_x11_display_get_xdisplay (gdk_display);
      xwin = gdk_x11_window_get_xid (gdk_window);
      property = XInternAtom (display, "_NET_WM_DESKTOP", False);

      if (Success == XGetWindowProperty (display, xwin, property, 0, 1, False,
                                         XA_CARDINAL, &actual_type,
                                         &actual_format, &nitems,
                                         &bytes_after, &data_p))
        {
          if (actual_type == XA_CARDINAL &&
              actual_format == 32 &&
              nitems == 1 &&
              data_p != NULL)
            {
              workspace = *(unsigned int *) data_p;
            }

          if (data_p != NULL)
            XFree (data_p);
        }

      gdk_x11_display_error_trap_pop_ignored (gdk_display);

      return workspace;
    }
  else
#endif
    {
      return -1;
    }
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

  /* setup menubar visibility */
  if (G_LIKELY (menubar != TERMINAL_VISIBILITY_DEFAULT))
    show_menubar = (menubar == TERMINAL_VISIBILITY_SHOW);
  gtk_widget_set_visible (window->menubar, show_menubar);

  /* setup toolbar visibility */
  if (G_LIKELY (toolbar != TERMINAL_VISIBILITY_DEFAULT))
    show_toolbar = (toolbar == TERMINAL_VISIBILITY_SHOW);
  gtk_widget_set_visible (window->toolbar, show_toolbar);

  /* setup borders visibility */
  if (G_LIKELY (borders != TERMINAL_VISIBILITY_DEFAULT))
    show_borders = (borders == TERMINAL_VISIBILITY_SHOW);
  gtk_window_set_decorated (GTK_WINDOW (window), show_borders);

  /* setup full screen */
  window->is_fullscreen = fullscreen && window->fullscreen_supported;

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
  GtkWidget *label;
  gint       page, position = -1;
  gboolean   adjacent;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));
  g_return_if_fail (TERMINAL_IS_SCREEN (screen));

  /* show the terminal screen first: see bug #13263*/
  gtk_widget_show (GTK_WIDGET (screen));

  /* create the tab label */
  label = terminal_screen_get_tab_label (screen);

  /* determine the tab position */
  g_object_get (G_OBJECT (window->priv->preferences), "misc-new-tab-adjacent", &adjacent, NULL);
  if (G_UNLIKELY (adjacent))
    position = gtk_notebook_page_num (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (window->priv->active)) + 1;

  page = gtk_notebook_insert_page (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (screen), label, position);
  gtk_container_child_set (GTK_CONTAINER (window->priv->notebook), GTK_WIDGET (screen), "tab-expand", TRUE, NULL);
  gtk_container_child_set (GTK_CONTAINER (window->priv->notebook), GTK_WIDGET (screen), "tab-fill", TRUE, NULL);

  /* allow tab sorting and dnd */
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (screen), TRUE);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (screen), TRUE);

  /* update scrollbar visibility */
  if (window->priv->scrollbar_visibility != TERMINAL_VISIBILITY_DEFAULT)
    terminal_screen_update_scrolling_bar (screen);

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
TerminalScreen*
terminal_window_get_active (TerminalWindow *window)
{
  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);
  return window->priv->active;
}



/**
 * terminal_window_notebook_show_tabs:
 * @window  : A #TerminalWindow.
 **/
void
terminal_window_notebook_show_tabs (TerminalWindow *window)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (window->priv->notebook);
  gboolean     show_tabs = TRUE;
  gint         npages;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  /* check preferences */
  npages = gtk_notebook_get_n_pages (notebook);
  if (npages < 2)
    {
      g_object_get (G_OBJECT (window->priv->preferences),
                    window->priv->drop_down ? "dropdown-always-show-tabs" :
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
  GdkScreen   *gscreen;
  GList       *children, *lp;
  GSList      *result = NULL;
  glong        w;
  glong        h;
  gint         workspace;

  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), NULL);

  if (G_LIKELY (window->priv->active != NULL))
    {
      terminal_screen_get_size (window->priv->active, &w, &h);
      result = g_slist_prepend (result, g_strdup_printf ("--geometry=%ldx%ld", w, h));
    }

  gscreen = gtk_window_get_screen (GTK_WINDOW (window));
  if (G_LIKELY (gscreen != NULL))
    {
      result = g_slist_prepend (result, g_strdup ("--display"));
      result = g_slist_prepend (result, g_strdup (gdk_display_get_name (gdk_screen_get_display (gscreen))));
    }

  role = gtk_window_get_role (GTK_WINDOW (window));
  if (G_LIKELY (role != NULL))
    result = g_slist_prepend (result, g_strdup_printf ("--role=%s", role));

  workspace = terminal_window_get_workspace (window);
  if (workspace != -1)
    {
      result = g_slist_prepend (result, g_strdup ("--workspace"));
      result = g_slist_prepend (result, g_strdup_printf ("%d", workspace));
    }

  if (window->is_fullscreen)
    result = g_slist_prepend (result, g_strdup ("--fullscreen"));

  if (gtk_widget_is_visible (window->menubar))
    result = g_slist_prepend (result, g_strdup ("--show-menubar"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-menubar"));

  if (gtk_window_get_decorated (GTK_WINDOW (window)))
    result = g_slist_prepend (result, g_strdup ("--show-borders"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-borders"));

  if (gtk_widget_is_visible (window->toolbar))
    result = g_slist_prepend (result, g_strdup ("--show-toolbar"));
  else
    result = g_slist_prepend (result, g_strdup ("--hide-toolbar"));

  if (window->priv->zoom != TERMINAL_ZOOM_LEVEL_DEFAULT)
    result = g_slist_prepend (result, g_strdup_printf ("--zoom=%d", window->priv->zoom));
  if (window->priv->font != NULL)
    result = g_slist_prepend (result, g_strdup_printf ("--font=%s", window->priv->font));

  /* set restart commands of the tabs */
  children = gtk_container_get_children (GTK_CONTAINER (window->priv->notebook));
  for (lp = children; lp != NULL; lp = lp->next)
    {
      if (lp != children)
        result = g_slist_prepend (result, g_strdup ("--tab"));
      if (window->priv->active == lp->data)
        result = g_slist_prepend (result, g_strdup ("--active-tab"));
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
 * terminal_window_get_font:
 * @window  : A #TerminalWindow.
 **/
const gchar*
terminal_window_get_font (TerminalWindow *window)
{
  return window->priv->font;
}



/**
 * terminal_window_set_font:
 * @window  : A #TerminalWindow.
 **/
void
terminal_window_set_font (TerminalWindow *window,
                          const gchar    *font)
{
  g_return_if_fail (font != NULL);
  g_free (window->priv->font);
  window->priv->font = g_strdup (font);
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
 * terminal_window_get_zoom_level:
 * @window  : A #TerminalWindow.
 **/
TerminalZoomLevel
terminal_window_get_zoom_level (TerminalWindow *window)
{
  return window->priv->zoom;
}



/**
 * terminal_window_set_zoom_level:
 * @window  : A #TerminalWindow.
 * @zoom    : Zoom level.
 **/
void
terminal_window_set_zoom_level (TerminalWindow    *window,
                                TerminalZoomLevel  zoom)
{
  window->priv->zoom = zoom;
}



/**
 * terminal_window_is_drop_down:
 * @window  : A #TerminalWindow.
 **/
gboolean
terminal_window_is_drop_down (TerminalWindow *window)
{
  return window->priv->drop_down != 0;
}



/**
 * terminal_window_set_drop_down:
 * @window    : A #TerminalWindow.
 * @drop_down : Whether the window is drop-down.
 **/
void
terminal_window_set_drop_down (TerminalWindow *window,
                               gboolean        drop_down)
{
  window->priv->drop_down = drop_down;
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

  if (window->menubar != NULL && gtk_widget_get_visible (window->menubar))
    gtk_widget_get_preferred_size (window->menubar, &req, NULL);

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

  if (window->toolbar != NULL && gtk_widget_get_visible (window->toolbar))
    gtk_widget_get_preferred_size (window->toolbar, &req, NULL);

  return req.height;
}



static void
terminal_window_create_menu (TerminalWindow        *window,
                             TerminalWindowAction   action,
                             GCallback              cb_update_menu)
{
  GtkWidget *item;
  GtkWidget *submenu;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (action), G_OBJECT (window), GTK_MENU_SHELL (window->menubar));

  submenu = g_object_new (GTK_TYPE_MENU, NULL);
  gtk_menu_set_accel_group (GTK_MENU (submenu), window->priv->accel_group);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), GTK_WIDGET (submenu));
  g_signal_connect_swapped (G_OBJECT (submenu), "show", G_CALLBACK (cb_update_menu), window);

  if (action == TERMINAL_WINDOW_ACTION_TABS_MENU)
    window->priv->tabs_menu = submenu;
}



void
terminal_window_menu_clean (GtkMenu *menu)
{
  GList     *children, *lp;
  GtkWidget *submenu;

  children = gtk_container_get_children (GTK_CONTAINER (menu));
  for (lp = children; lp != NULL; lp = lp->next)
    {
      submenu = gtk_menu_item_get_submenu (lp->data);
      if (submenu != NULL)
        gtk_widget_destroy (submenu);
      gtk_container_remove (GTK_CONTAINER (menu), lp->data);
    }
  g_list_free (children);
}



static void
terminal_window_menu_add_section (TerminalWindow      *window,
                                  GtkWidget           *menu,
                                  MenuSections         sections,
                                  gboolean             as_submenu)
{
#define AS_SUBMENU(text)  if (as_submenu)                                   \
  {                                                                         \
    item = gtk_menu_item_new_with_label (text);                             \
    submenu = g_object_new (GTK_TYPE_MENU, NULL);                           \
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), GTK_WIDGET (submenu)); \
    insert_to_menu = GTK_WIDGET (submenu);                                  \
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);                    \
  }

  GtkWidget         *item;
  GtkWidget         *insert_to_menu;
  GtkMenu           *submenu;
  static const gint  n_signal_names = 31;

  insert_to_menu = menu;

  if (sections & MENU_SECTION_SIGNAL)
    {
      AS_SUBMENU (_("Send Signal"))
      for (int i = 1; i < n_signal_names; i++)
        {
          gchar          *label;
          SendSignalData *p = g_new (SendSignalData, 1);
          p->window = window;
          p->signal = i;
          label = g_strdup_printf("%i - %s", i, signal_names[i]);
          item = gtk_menu_item_new_with_mnemonic (label);
          g_signal_connect_data (G_OBJECT (item), "activate", G_CALLBACK (terminal_window_action_send_signal), p, terminal_util_free_data, G_CONNECT_SWAPPED);
          gtk_menu_shell_append (GTK_MENU_SHELL (insert_to_menu), item);
        }
      xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
    }

  if (sections & MENU_SECTION_ZOOM)
    {
      AS_SUBMENU (_("Zoom"));

      item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_ZOOM_IN), G_OBJECT (window), GTK_MENU_SHELL (insert_to_menu));
      gtk_widget_set_sensitive (item, window->priv->zoom != TERMINAL_ZOOM_LEVEL_MAXIMUM);
      item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_ZOOM_OUT), G_OBJECT (window), GTK_MENU_SHELL (insert_to_menu));
      gtk_widget_set_sensitive (item, window->priv->zoom != TERMINAL_ZOOM_LEVEL_MINIMUM);
      xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_ZOOM_RESET), G_OBJECT (window), GTK_MENU_SHELL (insert_to_menu));
      xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
    }

  if (sections & MENU_SECTION_COPY)
    {
      AS_SUBMENU (_("Copy"));

      item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_COPY), G_OBJECT (window), GTK_MENU_SHELL (insert_to_menu));
      gtk_widget_set_sensitive (item, terminal_screen_has_selection (window->priv->active));
      item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_COPY_HTML), G_OBJECT (window), GTK_MENU_SHELL (insert_to_menu));
      gtk_widget_set_sensitive (item, terminal_screen_has_selection (window->priv->active));
      xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
    }

  if (sections & MENU_SECTION_PASTE)
    {
      AS_SUBMENU (_("Paste"));

      item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_PASTE), G_OBJECT (window), GTK_MENU_SHELL (insert_to_menu));
      gtk_widget_set_sensitive (item, terminal_screen_get_input_enabled (window->priv->active));
      item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_PASTE_SELECTION), G_OBJECT (window), GTK_MENU_SHELL (insert_to_menu));
      gtk_widget_set_sensitive (item, terminal_screen_get_input_enabled (window->priv->active));
      xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
    }

  if (sections & MENU_SECTION_VIEW)
    {
      AS_SUBMENU (_("View Options"));

      xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SHOW_MENUBAR), G_OBJECT (window), gtk_widget_is_visible (window->menubar), GTK_MENU_SHELL (insert_to_menu));
      xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SHOW_TOOLBAR), G_OBJECT (window), gtk_widget_is_visible (window->toolbar), GTK_MENU_SHELL (insert_to_menu));
      xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SHOW_BORDERS), G_OBJECT (window), gtk_window_get_decorated (GTK_WINDOW (window)), GTK_MENU_SHELL (insert_to_menu));
      xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_FULLSCREEN), G_OBJECT (window), window->is_fullscreen, GTK_MENU_SHELL (insert_to_menu));
      xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
    }

#undef AS_SUBMENU
}



static void
terminal_window_update_file_menu (TerminalWindow      *window,
                                  GtkWidget           *menu)
{
  GtkWidget  *item;
  gint        n_pages;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  /* "Detach Tab" and "Close Other Tabs" are sensitive if we have at least two pages.
   * "Undo Close" is sensitive if there is a tab to unclose. */

  /* determine the number of pages */
  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));

  terminal_window_menu_clean (GTK_MENU (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_NEW_TAB), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_NEW_WINDOW), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_OPEN_FOLDER), G_OBJECT (window), GTK_MENU_SHELL (menu));
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_UNDO_CLOSE_TAB), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, !g_queue_is_empty (window->priv->closed_tabs_list));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_DETACH_TAB), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, n_pages > 1);
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_CLOSE_TAB), G_OBJECT (window), GTK_MENU_SHELL (menu));
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_CLOSE_OTHER_TABS), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, n_pages > 1);
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_CLOSE_WINDOW), G_OBJECT (window), GTK_MENU_SHELL (menu));

  gtk_widget_show_all (GTK_WIDGET (menu));
}



static void
terminal_window_update_edit_menu     (TerminalWindow      *window,
                                      GtkWidget           *menu)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  terminal_window_menu_clean (GTK_MENU (menu));
  terminal_window_menu_add_section (window, menu, MENU_SECTION_COPY | MENU_SECTION_PASTE, FALSE);
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SELECT_ALL), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_COPY_INPUT), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_PREFERENCES), G_OBJECT (window), GTK_MENU_SHELL (menu));

  gtk_widget_show_all (GTK_WIDGET (menu));
}



static void
terminal_window_update_view_menu     (TerminalWindow      *window,
                                      GtkWidget           *menu)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  terminal_window_menu_clean (GTK_MENU (menu));
  terminal_window_menu_add_section (window, menu, MENU_SECTION_VIEW, FALSE);
  terminal_window_menu_add_section (window, menu, MENU_SECTION_ZOOM, FALSE);

  gtk_widget_show_all (GTK_WIDGET (menu));
}



static void
terminal_window_update_terminal_menu (TerminalWindow      *window,
                                      GtkWidget           *menu)
{
  GtkWidget  *item;
  gboolean    can_search;

  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  terminal_window_menu_clean (GTK_MENU (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SET_TITLE), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SET_TITLE_COLOR), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  can_search = terminal_screen_search_has_gregex (window->priv->active);
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SEARCH), G_OBJECT (window), GTK_MENU_SHELL (menu));
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SEARCH_NEXT), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, can_search);
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SEARCH_PREV), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, can_search);
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));

  /* Set Encoding uses the TerminalAction, GtkAction, therefore it is deprecated */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_action_create_menu_item (window->priv->encoding_action));
G_GNUC_END_IGNORE_DEPRECATIONS
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_READ_ONLY), G_OBJECT (window), !terminal_screen_get_input_enabled (window->priv->active), GTK_MENU_SHELL (menu));
  xfce_gtk_toggle_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SCROLL_ON_OUTPUT), G_OBJECT (window), terminal_screen_get_scroll_on_output (window->priv->active), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_SAVE_CONTENTS), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  terminal_window_menu_add_section (window, menu, MENU_SECTION_SIGNAL, TRUE);
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_RESET), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_RESET_AND_CLEAR), G_OBJECT (window), GTK_MENU_SHELL (menu));

  gtk_widget_show_all (GTK_WIDGET (menu));
}



static void
terminal_window_update_tabs_menu     (TerminalWindow      *window,
                                      GtkWidget           *menu)
{
  GtkWidget  *item;
  gint        n_pages;
  gboolean    can_go_left;
  gboolean    can_go_right;

  /* go-to menu */
  gint            n;
  GtkWidget      *page;
  GSList         *group = NULL;
  GtkRadioAction *radio_action = NULL;
  GtkAccelKey     key = {0};
  gchar           name[50], buf[100];

  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));

  can_go_left = terminal_window_can_go_left (window);
  can_go_right = terminal_window_can_go_right (window);

  terminal_window_menu_clean (GTK_MENU (menu));
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_PREV_TAB), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, can_go_left);
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_NEXT_TAB), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, can_go_right);
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_LAST_ACTIVE_TAB), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, window->priv->last_active != NULL);
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_MOVE_TAB_LEFT), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, can_go_left);
  item = xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_MOVE_TAB_RIGHT), G_OBJECT (window), GTK_MENU_SHELL (menu));
  gtk_widget_set_sensitive (item, can_go_right);
  
  /* go-to menu */
  for (n = 0; n < n_pages; n++)
    {
      page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (window->priv->notebook), n);

      g_snprintf (name, sizeof (name), "goto-tab-%d", n + 1);

      /* create action */
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      radio_action = gtk_radio_action_new (name, NULL, NULL, NULL, n);
      gtk_action_set_sensitive (GTK_ACTION (radio_action), n_pages > 1);
      g_object_bind_property (G_OBJECT (page), "title",
                              G_OBJECT (radio_action), "label",
                              G_BINDING_SYNC_CREATE);
      gtk_radio_action_set_group (radio_action, group);
      group = gtk_radio_action_get_group (radio_action);
      gtk_action_set_accel_group (GTK_ACTION (radio_action), window->priv->accel_group);
      G_GNUC_END_IGNORE_DEPRECATIONS

      g_signal_connect (G_OBJECT (radio_action), "activate",
                        G_CALLBACK (terminal_window_action_goto_tab), window->priv->notebook);

      /* connect action to the page, so we can activate it when a tab is switched */
      g_object_set_qdata_full (G_OBJECT (page), tabs_menu_action_quark, radio_action, g_object_unref);

      /* set an accelerator path */
      g_snprintf (buf, sizeof (buf), "<Actions>/terminal-window/%s", name);
      if (gtk_accel_map_lookup_entry (buf, &key) && key.accel_key != 0)
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          gtk_action_set_accel_path (GTK_ACTION (radio_action), buf);
          G_GNUC_END_IGNORE_DEPRECATIONS
        }

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      /* add action in the menu */
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_action_create_menu_item (GTK_ACTION (radio_action)));
      G_GNUC_END_IGNORE_DEPRECATIONS

      /* store */
      window->priv->tabs_menu_actions = g_slist_prepend (window->priv->tabs_menu_actions, radio_action);
    }

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (n_pages > 1)
    gtk_radio_action_set_current_value (radio_action, gtk_notebook_get_current_page (GTK_NOTEBOOK (window->priv->notebook)));
G_GNUC_END_IGNORE_DEPRECATIONS

  gtk_widget_show_all (GTK_WIDGET (menu));
}



/**
 * terminal_window_update_goto_accels:
 * @window  : A #TerminalWindow.
 *
 * Calls terminal_window_update_tabs_menu to create the go-to actions and their accelerators.
 **/
void
terminal_window_update_goto_accels (TerminalWindow *window)
{
  terminal_window_update_tabs_menu (window, window->priv->tabs_menu);
}



static void
terminal_window_update_help_menu     (TerminalWindow      *window,
                                      GtkWidget           *menu)
{
  g_return_if_fail (TERMINAL_IS_WINDOW (window));

  terminal_window_menu_clean (GTK_MENU (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_CONTENTS), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (get_action_entry (TERMINAL_WINDOW_ACTION_ABOUT), G_OBJECT (window), GTK_MENU_SHELL (menu));

  gtk_widget_show_all (GTK_WIDGET (menu));
}



static gboolean
terminal_window_can_go_left (TerminalWindow *window)
{
  gint        page_num;
  gint        n_pages;
  gboolean    cycle_tabs;


  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (window->priv->active));

  g_object_get (G_OBJECT (window->priv->preferences),
                "misc-cycle-tabs", &cycle_tabs,
                NULL);

  return (cycle_tabs && n_pages > 1) || (page_num > 0);
}



static gboolean
terminal_window_can_go_right (TerminalWindow *window)
{
  gint        page_num;
  gint        n_pages;
  gboolean    cycle_tabs;


  g_return_val_if_fail (TERMINAL_IS_WINDOW (window), FALSE);

  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (window->priv->notebook));
  page_num = gtk_notebook_page_num (GTK_NOTEBOOK (window->priv->notebook), GTK_WIDGET (window->priv->active));

  g_object_get (G_OBJECT (window->priv->preferences),
                "misc-cycle-tabs", &cycle_tabs,
                NULL);

  return (cycle_tabs && n_pages > 1) || (page_num < n_pages - 1);
}



XfceGtkActionEntry*
terminal_window_get_action_entry (TerminalWindow      *window,
                                  TerminalWindowAction action)
{
  return (XfceGtkActionEntry*) get_action_entry (action);
}



XfceGtkActionEntry*
terminal_window_get_action_entries (void)
{
  return action_entries;
}
