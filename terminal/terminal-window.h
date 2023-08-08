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

#ifndef TERMINAL_WINDOW_H
#define TERMINAL_WINDOW_H

#include <terminal/terminal-screen.h>
#include <terminal/terminal-options.h>
#include <terminal/terminal-preferences.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_WINDOW            (terminal_window_get_type ())
#define TERMINAL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_WINDOW, TerminalWindow))
#define TERMINAL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW, TerminalWindowClass))
#define TERMINAL_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_WINDOW))
#define TERMINAL_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_WINDOW))
#define TERMINAL_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_WINDOW, TerminalWindowClass))

typedef struct _TerminalWindowPrivate TerminalWindowPrivate;

/* #XfceGtkActionEntrys provided by this widget */
typedef enum
{
    TERMINAL_WINDOW_ACTION_FILE_MENU,
    TERMINAL_WINDOW_ACTION_NEW_TAB,
    TERMINAL_WINDOW_ACTION_NEW_WINDOW,
    TERMINAL_WINDOW_ACTION_OPEN_FOLDER,
    TERMINAL_WINDOW_ACTION_UNDO_CLOSE_TAB,
    TERMINAL_WINDOW_ACTION_DETACH_TAB,
    TERMINAL_WINDOW_ACTION_CLOSE_TAB,
    TERMINAL_WINDOW_ACTION_CLOSE_OTHER_TABS,
    TERMINAL_WINDOW_ACTION_CLOSE_WINDOW,
    TERMINAL_WINDOW_ACTION_EDIT_MENU,
    TERMINAL_WINDOW_ACTION_COPY,
    TERMINAL_WINDOW_ACTION_COPY_HTML,
    TERMINAL_WINDOW_ACTION_PASTE,
    TERMINAL_WINDOW_ACTION_PASTE_ALT,
    TERMINAL_WINDOW_ACTION_PASTE_SELECTION,
    TERMINAL_WINDOW_ACTION_PASTE_SELECTION_ALT,
    TERMINAL_WINDOW_ACTION_SELECT_ALL,
    TERMINAL_WINDOW_ACTION_COPY_INPUT,
    TERMINAL_WINDOW_ACTION_PREFERENCES,
    TERMINAL_WINDOW_ACTION_VIEW_MENU,
    TERMINAL_WINDOW_ACTION_ZOOM_IN,
    TERMINAL_WINDOW_ACTION_ZOOM_IN_ALT,
    TERMINAL_WINDOW_ACTION_ZOOM_OUT,
    TERMINAL_WINDOW_ACTION_ZOOM_OUT_ALT,
    TERMINAL_WINDOW_ACTION_ZOOM_RESET,
    TERMINAL_WINDOW_ACTION_ZOOM_RESET_ALT,
    TERMINAL_WINDOW_ACTION_TERMINAL_MENU,
    TERMINAL_WINDOW_ACTION_SET_TITLE,
    TERMINAL_WINDOW_ACTION_SET_TITLE_COLOR,
    TERMINAL_WINDOW_ACTION_SEARCH,
    TERMINAL_WINDOW_ACTION_SEARCH_NEXT,
    TERMINAL_WINDOW_ACTION_SEARCH_PREV,
    TERMINAL_WINDOW_ACTION_SAVE_CONTENTS,
    TERMINAL_WINDOW_ACTION_RESET,
    TERMINAL_WINDOW_ACTION_RESET_AND_CLEAR,
    TERMINAL_WINDOW_ACTION_TABS_MENU,
    TERMINAL_WINDOW_ACTION_PREV_TAB,
    TERMINAL_WINDOW_ACTION_NEXT_TAB,
    TERMINAL_WINDOW_ACTION_LAST_ACTIVE_TAB,
    TERMINAL_WINDOW_ACTION_MOVE_TAB_LEFT,
    TERMINAL_WINDOW_ACTION_MOVE_TAB_RIGHT,
    TERMINAL_WINDOW_ACTION_HELP_MENU,
    TERMINAL_WINDOW_ACTION_CONTENTS,
    TERMINAL_WINDOW_ACTION_ABOUT,
    TERMINAL_WINDOW_ACTION_ZOOM_MENU,
    TERMINAL_WINDOW_ACTION_TOGGLE_MENUBAR,
    TERMINAL_WINDOW_ACTION_SHOW_MENUBAR,
    TERMINAL_WINDOW_ACTION_SHOW_TOOLBAR,
    TERMINAL_WINDOW_ACTION_SHOW_BORDERS,
    TERMINAL_WINDOW_ACTION_FULLSCREEN,
    TERMINAL_WINDOW_ACTION_READ_ONLY,
    TERMINAL_WINDOW_ACTION_SCROLL_ON_OUTPUT,

    /* go-to tab accelerators */
    TERMINAL_WINDOW_ACTION_GOTO_TAB_1,
    TERMINAL_WINDOW_ACTION_GOTO_TAB_2,
    TERMINAL_WINDOW_ACTION_GOTO_TAB_3,
    TERMINAL_WINDOW_ACTION_GOTO_TAB_4,
    TERMINAL_WINDOW_ACTION_GOTO_TAB_5,
    TERMINAL_WINDOW_ACTION_GOTO_TAB_6,
    TERMINAL_WINDOW_ACTION_GOTO_TAB_7,
    TERMINAL_WINDOW_ACTION_GOTO_TAB_8,
    TERMINAL_WINDOW_ACTION_GOTO_TAB_9,

    TERMINAL_WINDOW_ACTION_N
} TerminalWindowAction;

typedef enum
{
    MENU_SECTION_ZOOM       = 1 << 0,
    MENU_SECTION_SIGNAL     = 1 << 1,
    MENU_SECTION_COPY       = 1 << 2,
    MENU_SECTION_PASTE      = 1 << 3,
    MENU_SECTION_VIEW       = 1 << 4,
} MenuSections;

typedef struct
{
  GtkWindowClass parent_class;
} TerminalWindowClass;

typedef struct
{
  GtkWindow              parent_instance;

  GtkWidget             *menubar;
  GtkWidget             *toolbar;

  gboolean               fullscreen_supported;
  gboolean               is_fullscreen;

  TerminalWindowPrivate *priv;
} TerminalWindow;

typedef struct
{
  gchar *path;
  guint  mods;
} TerminalAccel;

GType              terminal_window_get_type                 (void) G_GNUC_CONST;

GtkWidget         *terminal_window_new                      (const gchar        *role,
                                                             gboolean            fullscreen,
                                                             TerminalVisibility  menubar,
                                                             TerminalVisibility  borders,
                                                             TerminalVisibility  toolbar);

void               terminal_window_add                      (TerminalWindow     *window,
                                                             TerminalScreen     *screen);

TerminalScreen    *terminal_window_get_active               (TerminalWindow     *window);

void               terminal_window_notebook_show_tabs       (TerminalWindow     *window);

GSList            *terminal_window_get_restart_command      (TerminalWindow     *window);

void               terminal_window_set_grid_size            (TerminalWindow     *window,
                                                             glong               width,
                                                             glong               height);

gboolean           terminal_window_has_children             (TerminalWindow     *window);

GObject           *terminal_window_get_preferences          (TerminalWindow     *window);

GtkWidget         *terminal_window_get_vbox                 (TerminalWindow     *window);

GtkWidget         *terminal_window_get_notebook             (TerminalWindow     *window);

GtkWidget         *terminal_window_get_preferences_dialog   (TerminalWindow     *window);
const gchar       *terminal_window_get_font                 (TerminalWindow     *window);
void               terminal_window_set_font                 (TerminalWindow     *window,
                                                             const gchar        *font);

TerminalVisibility terminal_window_get_scrollbar_visibility (TerminalWindow     *window);
void               terminal_window_set_scrollbar_visibility (TerminalWindow     *window,
                                                             TerminalVisibility  scrollbar);

TerminalZoomLevel  terminal_window_get_zoom_level           (TerminalWindow     *window);
void               terminal_window_set_zoom_level           (TerminalWindow     *window,
                                                             TerminalZoomLevel   zoom);

gboolean           terminal_window_is_drop_down             (TerminalWindow     *window);
void               terminal_window_set_drop_down            (TerminalWindow     *window,
                                                             gboolean            drop_down);

gint               terminal_window_get_menubar_height       (TerminalWindow     *window);

gint               terminal_window_get_toolbar_height       (TerminalWindow     *window);

void               terminal_window_update_goto_accels       (TerminalWindow     *window);

XfceGtkActionEntry *terminal_window_get_action_entry         (TerminalWindow      *window,
                                                              TerminalWindowAction action);

XfceGtkActionEntry *terminal_window_get_action_entries      (void);

G_END_DECLS

#endif /* !TERMINAL_WINDOW_H */
