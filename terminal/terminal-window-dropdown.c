/*-
 * Copyright (C) 2012 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_GTK_LAYER_SHELL
#include <gtk-layer-shell/gtk-layer-shell.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

#include <terminal/terminal-private.h>
#include <terminal/terminal-window.h>
#include <terminal/terminal-util.h>
#include <terminal/terminal-window-dropdown.h>

/* animation fps */
#define ANIMATION_FPS (1000 / 20)



enum
{
  PROP_0,
  PROP_DROPDOWN_WIDTH,
  PROP_DROPDOWN_HEIGHT,
  PROP_DROPDOWN_POSITION,
  PROP_DROPDOWN_POSITION_VERTICAL,
  PROP_DROPDOWN_OPACITY,
  PROP_DROPDOWN_STATUS_ICON,
  PROP_DROPDOWN_KEEP_ABOVE,
  PROP_DROPDOWN_ANIMATION_TIME,
  PROP_DROPDOWN_ALWAYS_SHOW_TABS,
  PROP_DROPDOWN_SHOW_BORDERS,
  N_PROPERTIES
};



static void     terminal_window_dropdown_finalize                (GObject                *object);
static void     terminal_window_dropdown_set_property            (GObject                *object,
                                                                  guint                   prop_id,
                                                                  const GValue           *value,
                                                                  GParamSpec             *pspec);
static gboolean terminal_window_dropdown_focus_in_event          (GtkWidget              *widget,
                                                                  GdkEventFocus          *event);
static gboolean terminal_window_dropdown_focus_out_event         (GtkWidget              *widget,
                                                                  GdkEventFocus          *event);
static gboolean terminal_window_dropdown_status_icon_press_event (GtkStatusIcon          *status_icon,
                                                                  GdkEventButton         *event,
                                                                  TerminalWindowDropdown *dropdown);
static void     terminal_window_dropdown_status_icon_popup_menu  (GtkStatusIcon          *status_icon,
                                                                  guint                   button,
                                                                  guint32                 timestamp,
                                                                  TerminalWindowDropdown *dropdown);
static gboolean terminal_window_dropdown_can_grab                (gpointer                data);
static void     terminal_window_dropdown_can_grab_destroyed      (gpointer                data);
static void     terminal_window_dropdown_get_monitor_geometry    (GdkScreen              *screen,
                                                                  gint                    monitor_num,
                                                                  GdkRectangle           *geometry);
static gboolean terminal_window_dropdown_animate_down            (gpointer                data);
static gboolean terminal_window_dropdown_animate_up              (gpointer                data);
static void     terminal_window_dropdown_animate_destroyed       (gpointer                data);
static void     terminal_window_dropdown_hide                    (TerminalWindowDropdown *dropdown);
static void     terminal_window_dropdown_show                    (TerminalWindowDropdown *dropdown,
                                                                  guint32                 timestamp,
                                                                  gboolean                activate);
static void     terminal_window_dropdown_toggle_real             (TerminalWindowDropdown *dropdown,
                                                                  guint32                 timestamp,
                                                                  gboolean                force_show);
static guint32  terminal_window_dropdown_get_timestamp           (const gchar            *startup_id);
static void     terminal_dropdown_window_screen_size_changed     (GdkScreen              *screen,
                                                                  TerminalWindowDropdown *dropdown);



struct _TerminalWindowDropdownClass
{
  TerminalWindowClass parent_class;
};

typedef enum
{
  ANIMATION_DIR_NONE,
  ANIMATION_DIR_UP,
  ANIMATION_DIR_DOWN,
} TerminalDirection;

struct _TerminalWindowDropdown
{
  TerminalWindow     parent_instance;

  /* timeout for animation */
  guint              animation_timeout_id;
  guint              animation_time;
  TerminalDirection  animation_dir;

  /* ui widgets */
  GtkWidget         *keep_open;

  /* idle for detecting focus out during grabs (Alt+Tab) */
  guint              grab_timeout_id;

  /* measurements */
  gdouble            rel_width;
  gdouble            rel_height;
  gdouble            rel_position;
  gdouble            rel_position_vertical;

  GtkStatusIcon     *status_icon;

  /* last screen and monitor */
  GdkScreen         *screen;
  gint               monitor_num;
#ifdef HAVE_GTK_LAYER_SHELL
  GdkMonitor        *monitor;
  gboolean           move_to_active;
#endif

  /* server time of focus out with grab */
  gint64             focus_out_time;

  /* used to not hide the terminal when dialogs are opened */
  gboolean            ignore_next_focus_out_event;
};



G_DEFINE_TYPE (TerminalWindowDropdown, terminal_window_dropdown, TERMINAL_TYPE_WINDOW)



static GParamSpec *dropdown_props[N_PROPERTIES] = { NULL, };



static void
terminal_window_dropdown_class_init (TerminalWindowDropdownClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = terminal_window_dropdown_set_property;
  gobject_class->finalize = terminal_window_dropdown_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->focus_in_event = terminal_window_dropdown_focus_in_event;
  gtkwidget_class->focus_out_event = terminal_window_dropdown_focus_out_event;

  dropdown_props[PROP_DROPDOWN_WIDTH] =
      g_param_spec_uint ("dropdown-width",
                         NULL, NULL,
                         10, 100, 80,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_HEIGHT] =
      g_param_spec_uint ("dropdown-height",
                         NULL, NULL,
                         10, 100, 50,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_POSITION] =
      g_param_spec_uint ("dropdown-position",
                         NULL, NULL,
                         0, 100, 0,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_POSITION_VERTICAL] =
      g_param_spec_uint ("dropdown-position-vertical",
                         NULL, NULL,
                         0, 100, 0,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_OPACITY] =
      g_param_spec_uint ("dropdown-opacity",
                         NULL, NULL,
                         0, 100, 100,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_STATUS_ICON] =
      g_param_spec_boolean ("dropdown-status-icon",
                            NULL, NULL,
                            TRUE,
                            G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_KEEP_ABOVE] =
      g_param_spec_boolean ("dropdown-keep-above",
                            NULL, NULL,
                            TRUE,
                            G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_ANIMATION_TIME] =
      g_param_spec_uint ("dropdown-animation-time",
                         NULL, NULL,
                         0, 500, 0,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_ALWAYS_SHOW_TABS] =
      g_param_spec_boolean ("dropdown-always-show-tabs",
                            NULL, NULL,
                            TRUE,
                            G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  dropdown_props[PROP_DROPDOWN_SHOW_BORDERS] =
      g_param_spec_boolean ("dropdown-show-borders",
                            NULL, NULL,
                            FALSE,
                            G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

  /* install all properties */
  g_object_class_install_properties (gobject_class, N_PROPERTIES, dropdown_props);
}



#ifdef HAVE_GTK_LAYER_SHELL
static gboolean
monitor_changed (TerminalWindowDropdown *dropdown)
{
  GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (dropdown));
  GdkMonitor *monitor = gdk_display_get_monitor_at_window (gdk_display_get_default (), window);

  if (monitor != dropdown->monitor)
    {
      /* reposition and/or resize the window */
      dropdown->monitor = monitor;
      dropdown->monitor_num = -1;
      terminal_window_dropdown_show (dropdown, 0, FALSE);
    }

  return FALSE;
}



static void
set_monitor (TerminalWindowDropdown *dropdown)
{
  g_signal_handlers_disconnect_by_func (dropdown, set_monitor, NULL);

  if (dropdown->move_to_active)
    {
      /* let the compositor position the window where it sees fit, and with a bit of luck
       * (and/or configuration) it'll position it where the pointer is */
      gtk_layer_set_monitor (GTK_WINDOW (dropdown), NULL);
    }
  else
    {
      /* we should now have the right monitor, let's fix it */
      g_signal_handlers_disconnect_by_func (dropdown, monitor_changed, NULL);
      gtk_layer_set_monitor (GTK_WINDOW (dropdown), dropdown->monitor);
    }
}



static void
move_to_active_changed (TerminalWindowDropdown *dropdown)
{
  /* in any case, it's best to wait until the window is hidden before setting the monitor,
   * as gtk_layer_set_monitor() remaps the window and causes it to blink if shown */
  g_signal_connect (dropdown, "hide", G_CALLBACK (set_monitor), NULL);

  g_object_get (terminal_window_get_preferences (TERMINAL_WINDOW (dropdown)),
                "dropdown-move-to-active", &dropdown->move_to_active, NULL);
  if (dropdown->move_to_active)
    {
      /* there doesn't seem to be a better signal for tracking monitor changes */
      g_signal_connect_after (dropdown, "draw", G_CALLBACK (monitor_changed), NULL);
      if (dropdown->monitor == NULL)
        dropdown->monitor = gdk_display_get_monitor (gdk_display_get_default (), 0);
    }
  else
    {
      /* !move_to_active at startup: we have to wait until the window has been drawn a few
       * times before gdk_display_get_monitor_at_window() returns the correct result */
      if (dropdown->monitor == NULL)
        g_signal_connect_after (dropdown, "draw", G_CALLBACK (monitor_changed), NULL);
    }
}



static void
prefs_dialog_hidden (TerminalWindow *window)
{
  gboolean keep_above;
  g_object_get (terminal_window_get_preferences (window), "dropdown-keep-above", &keep_above, NULL);
  gtk_layer_set_layer (GTK_WINDOW (window), keep_above ? GTK_LAYER_SHELL_LAYER_TOP
                                                       : GTK_LAYER_SHELL_LAYER_BOTTOM);
  g_signal_handlers_disconnect_by_func (terminal_window_get_preferences_dialog (window),
                                        prefs_dialog_hidden, window);
}
#endif



static void
show_prefs_dialog (TerminalWindow *window)
{
  XfceGtkActionEntry *entry = terminal_window_get_action_entry (window, TERMINAL_WINDOW_ACTION_PREFERENCES);
  ((gboolean (*) (TerminalWindow *)) entry->callback) (window);
#ifdef HAVE_GTK_LAYER_SHELL
  if (gtk_layer_is_supported ())
    {
      /* ensure the prefs dialog is above the terminal the time it's shown */
      gtk_layer_set_layer (GTK_WINDOW (window), GTK_LAYER_SHELL_LAYER_BOTTOM);
      g_signal_connect_swapped (terminal_window_get_preferences_dialog (window), "hide",
                                G_CALLBACK (prefs_dialog_hidden), window);
    }
#endif
}



static void
terminal_window_dropdown_init (TerminalWindowDropdown *dropdown)
{
  TerminalWindow      *window = TERMINAL_WINDOW (dropdown);
  TerminalPreferences *preferences;
  XfceGtkActionEntry  *entry;
  GtkWidget           *hbox;
  GtkWidget           *button;
  GtkWidget           *img;
  guint                n;
  const gchar         *name;
  gboolean             keep_open;

  dropdown->rel_width = 0.80;
  dropdown->rel_height = 0.50;
  dropdown->rel_position = 0.50;
  dropdown->rel_position_vertical = 0.0;
  dropdown->animation_dir = ANIMATION_DIR_NONE;

  preferences = terminal_preferences_get();

#ifdef HAVE_GTK_LAYER_SHELL
  if (gtk_layer_is_supported ())
    {
      gtk_layer_init_for_window (GTK_WINDOW (window));
      gtk_layer_set_keyboard_mode (GTK_WINDOW (window), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
      gtk_layer_set_exclusive_zone (GTK_WINDOW (window), -1);
      gtk_layer_set_anchor (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
      gtk_layer_set_anchor (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
      g_signal_connect_object (preferences, "notify::dropdown-move-to-active",
                               G_CALLBACK (move_to_active_changed), dropdown, G_CONNECT_SWAPPED);
      move_to_active_changed (dropdown);
    }
  else
#endif
  if (WINDOWING_IS_WAYLAND ())
    {
      static gboolean warn = TRUE;
      if (warn)
        {
          g_warning ("Gtk Layer Shell unsupported: drop-down terminal won't work properly");
          warn = FALSE;
        }
    }

  /* shared setting to disable some functionality in TerminalWindow */
  terminal_window_set_drop_down (window, TRUE);

  /* default window settings */
  gtk_window_set_decorated (GTK_WINDOW (dropdown), FALSE);
  gtk_window_set_gravity (GTK_WINDOW (dropdown), GDK_GRAVITY_STATIC);
  gtk_window_set_type_hint (GTK_WINDOW (dropdown), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_stick (GTK_WINDOW (dropdown));

  /* this avoids to return focus to the window after dialog changes,
   * but we have terminal_util_activate_window() for that */
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dropdown), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dropdown), TRUE);

  /* adjust notebook for drop-down usage */
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (terminal_window_get_notebook (window)), GTK_POS_BOTTOM);
  terminal_window_notebook_show_tabs (window);

  /* notebook buttons */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_notebook_set_action_widget (GTK_NOTEBOOK (terminal_window_get_notebook (window)), hbox, GTK_PACK_END);

  button = dropdown->keep_open = gtk_toggle_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (button, _("Keep window open when it loses focus"));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_widget_set_focus_on_click (button, FALSE);

  g_object_get (terminal_window_get_preferences (window), "dropdown-keep-open-default", &keep_open, NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), keep_open);

  /* synchronize the `keep-open` toggle with the `keep-open` checkbox in the preferences dialog (unidirectional preferences-dialog -> notebook) */
  g_object_bind_property (G_OBJECT (preferences), "dropdown-keep-open-default",
                          GTK_TOGGLE_BUTTON(dropdown->keep_open), "active",
                          G_BINDING_DEFAULT);

  img = gtk_image_new_from_icon_name ("go-bottom", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), img);

  entry = terminal_window_get_action_entry (window, TERMINAL_WINDOW_ACTION_PREFERENCES);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (button, entry->menu_item_tooltip_text);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_widget_set_focus_on_click (button, FALSE);
  g_signal_connect_swapped (G_OBJECT (button), "clicked", G_CALLBACK (show_prefs_dialog), window);

  img = gtk_image_new_from_icon_name (entry->menu_item_icon_name, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), img);

  /* connect bindings */
  for (n = 1; n < N_PROPERTIES; n++)
    {
      name = g_param_spec_get_name (dropdown_props[n]);
      g_object_bind_property (terminal_window_get_preferences (window), name,
                              G_OBJECT (dropdown), name,
                              G_BINDING_SYNC_CREATE);
    }

  gtk_widget_show_all (hbox);

  g_object_unref (preferences);
}



static void
terminal_window_dropdown_finalize (GObject *object)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (object);

  if (dropdown->grab_timeout_id != 0)
    g_source_remove (dropdown->grab_timeout_id);

  if (dropdown->animation_timeout_id != 0)
    g_source_remove (dropdown->animation_timeout_id);

  if (dropdown->status_icon != NULL)
    g_object_unref (G_OBJECT (dropdown->status_icon));

  if (dropdown->screen != NULL)
    g_signal_handlers_disconnect_by_func (G_OBJECT (dropdown->screen),
                                          G_CALLBACK (terminal_dropdown_window_screen_size_changed),
                                          dropdown);

  (*G_OBJECT_CLASS (terminal_window_dropdown_parent_class)->finalize) (object);
}



static void
terminal_window_dropdown_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (object);
  TerminalWindow         *window = TERMINAL_WINDOW (object);
  gdouble                 opacity;
  GdkScreen              *screen;
  const gchar            *icon_name;

  switch (prop_id)
    {
    case PROP_DROPDOWN_WIDTH:
      dropdown->rel_width = g_value_get_uint (value) / 100.0;
      break;

    case PROP_DROPDOWN_HEIGHT:
      dropdown->rel_height = g_value_get_uint (value) / 100.0;
      break;

    case PROP_DROPDOWN_POSITION:
      dropdown->rel_position = g_value_get_uint (value) / 100.0;
      break;

    case PROP_DROPDOWN_POSITION_VERTICAL:
      dropdown->rel_position_vertical = g_value_get_uint (value) / 100.0;
      break;

    case PROP_DROPDOWN_OPACITY:
      screen = gtk_widget_get_screen (GTK_WIDGET (dropdown));
      if (gdk_screen_is_composited (screen))
        opacity = g_value_get_uint (value) / 100.0;
      else
        opacity = 1.00;

      gtk_widget_set_opacity (GTK_WIDGET (dropdown), opacity);
      return;

    case PROP_DROPDOWN_STATUS_ICON:
      /*
       * FIXME: GtkStatusIcon is X11 only
       * See also https://gitlab.xfce.org/apps/xfce4-terminal/-/issues/197
       */
      if (!WINDOWING_IS_X11 ())
        return;

      if (g_value_get_boolean (value))
        {
          /* icon_name will be NULL initially after creation; will update it later */
          icon_name = gtk_window_get_icon_name (GTK_WINDOW (object));
          if (dropdown->status_icon == NULL && icon_name != NULL)
            {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              dropdown->status_icon = g_path_is_absolute (icon_name)
                ? gtk_status_icon_new_from_file (icon_name)
                : gtk_status_icon_new_from_icon_name (icon_name);
              gtk_status_icon_set_title (dropdown->status_icon, _("Drop-down Terminal"));
              gtk_status_icon_set_tooltip_text (dropdown->status_icon, _("Toggle Drop-down Terminal"));
G_GNUC_END_IGNORE_DEPRECATIONS
              g_signal_connect (G_OBJECT (dropdown->status_icon), "button-press-event",
                  G_CALLBACK (terminal_window_dropdown_status_icon_press_event), dropdown);
              g_signal_connect (G_OBJECT (dropdown->status_icon), "popup-menu",
                  G_CALLBACK (terminal_window_dropdown_status_icon_popup_menu), dropdown);
            }
        }
      else if (dropdown->status_icon != NULL)
        {
          g_object_unref (G_OBJECT (dropdown->status_icon));
          dropdown->status_icon = NULL;
        }
      return;

    case PROP_DROPDOWN_KEEP_ABOVE:
      gtk_window_set_keep_above (GTK_WINDOW (dropdown), g_value_get_boolean (value));
      if (terminal_window_get_preferences_dialog (window) != NULL)
        terminal_util_activate_window (GTK_WINDOW (terminal_window_get_preferences_dialog (window)));
#ifdef HAVE_GTK_LAYER_SHELL
      else if (gtk_layer_is_supported ())
        gtk_layer_set_layer (GTK_WINDOW (dropdown),
                             g_value_get_boolean (value) ? GTK_LAYER_SHELL_LAYER_TOP : GTK_LAYER_SHELL_LAYER_BOTTOM);
#endif
      return;

    case PROP_DROPDOWN_ANIMATION_TIME:
      dropdown->animation_time = g_value_get_uint (value);
      return;

    case PROP_DROPDOWN_ALWAYS_SHOW_TABS:
      terminal_window_notebook_show_tabs (TERMINAL_WINDOW (dropdown));
      return;

    case PROP_DROPDOWN_SHOW_BORDERS:
      gtk_notebook_set_show_border (GTK_NOTEBOOK (terminal_window_get_notebook (window)),
                                    g_value_get_boolean (value));
      return;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }

  if (gtk_widget_get_visible (GTK_WIDGET (dropdown)))
    terminal_window_dropdown_show (dropdown, 0, TRUE);
}



static gboolean
terminal_window_dropdown_focus_in_event (GtkWidget     *widget,
                                         GdkEventFocus *event)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (widget);

  /* unset */
  dropdown->focus_out_time = 0;

  /* stop a possible grab test */
  if (dropdown->grab_timeout_id != 0)
    g_source_remove (dropdown->grab_timeout_id);

  return (*GTK_WIDGET_CLASS (terminal_window_dropdown_parent_class)->focus_in_event) (widget, event);
}



static gboolean
terminal_window_dropdown_focus_out_event (GtkWidget     *widget,
                                          GdkEventFocus *event)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (widget);
  gboolean                retval;

  /* let Gtk do its thingy */
  retval = (*GTK_WIDGET_CLASS (terminal_window_dropdown_parent_class)->focus_out_event) (widget, event);

  /* ignore this event and reset the flag */
  if (dropdown->ignore_next_focus_out_event == TRUE)
    {
      dropdown->ignore_next_focus_out_event = FALSE;
      return retval;
    }

  /* check if keep open is not enabled */
  if (gtk_widget_get_visible (widget)
      && !terminal_window_has_children (TERMINAL_WINDOW (dropdown))
      && gtk_grab_get_current () == NULL
      && dropdown->animation_dir != ANIMATION_DIR_UP) /* popup menu check */
    {
      if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dropdown->keep_open)))
        {
          /* check if the user is not pressing a key */
          if (!terminal_window_dropdown_can_grab (dropdown))
            {
              /* hide the window */
              terminal_window_dropdown_hide (dropdown);

              return retval;
            }
          else if (dropdown->grab_timeout_id == 0)
            {
              /* focus-out with keyboard grab */
              dropdown->grab_timeout_id =
                  gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE, 50, terminal_window_dropdown_can_grab,
                                                dropdown, terminal_window_dropdown_can_grab_destroyed);
            }
        }

      /* focus out time */
      dropdown->focus_out_time = g_get_real_time ();

    }

  return retval;
}



static gboolean
terminal_window_dropdown_status_icon_press_event (GtkStatusIcon          *status_icon,
                                                  GdkEventButton         *event,
                                                  TerminalWindowDropdown *dropdown)
{
  /* keep this event for the menu */
  if (event->button == 3)
    return FALSE;

  if (gdk_window_is_visible (gtk_widget_get_window (GTK_WIDGET (dropdown))))
    terminal_window_dropdown_hide (dropdown);
  else
    terminal_window_dropdown_show (dropdown, event->time, TRUE);

  return TRUE;
}



static void
terminal_window_dropdown_status_icon_popup_menu (GtkStatusIcon          *status_icon,
                                                 guint                   button,
                                                 guint32                 timestamp,
                                                 TerminalWindowDropdown *dropdown)
{
  TerminalWindow *window;
  GtkWidget      *menu;

  window = TERMINAL_WINDOW (dropdown);

  menu = gtk_menu_new ();
  g_signal_connect (G_OBJECT (menu), "selection-done",
      G_CALLBACK (gtk_widget_destroy), NULL);

  xfce_gtk_menu_item_new_from_action_entry (terminal_window_get_action_entry (window, TERMINAL_WINDOW_ACTION_PREFERENCES), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));
  xfce_gtk_menu_item_new_from_action_entry (terminal_window_get_action_entry (window, TERMINAL_WINDOW_ACTION_CLOSE_WINDOW), G_OBJECT (window), GTK_MENU_SHELL (menu));
  xfce_gtk_menu_append_separator (GTK_MENU_SHELL (menu));

  gtk_widget_show_all (menu);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}



static gboolean
terminal_window_dropdown_can_grab (gpointer data)
{
  GdkWindow    *window = gtk_widget_get_window (GTK_WIDGET (data));
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  GdkGrabStatus status = gdk_keyboard_grab (window, FALSE, GDK_CURRENT_TIME);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (status == GDK_GRAB_SUCCESS)
    {
      /* drop the grab */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      gdk_keyboard_ungrab (GDK_CURRENT_TIME);
G_GNUC_END_IGNORE_DEPRECATIONS
      return FALSE;
    }

  return TRUE;
}



static void
terminal_window_dropdown_can_grab_destroyed (gpointer data)
{
  TERMINAL_WINDOW_DROPDOWN (data)->grab_timeout_id = 0;
}



static void
terminal_window_dropdown_get_monitor_geometry (GdkScreen    *screen,
                                               gint          monitor_num,
                                               GdkRectangle *geometry)
{
  GdkDisplay *display = gdk_screen_get_display (screen);
  GdkMonitor *monitor = gdk_display_get_monitor (display, monitor_num);
  gdk_monitor_get_geometry (monitor, geometry);
}



static gboolean
terminal_window_dropdown_animate_down (gpointer data)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (data);
  TerminalWindow         *window = TERMINAL_WINDOW (data);
  GtkRequisition          req1;
  GdkRectangle            rect;
  gint                    step_size, vbox_h;
  gboolean                fullscreen;

  /* get window size */
  terminal_window_dropdown_get_monitor_geometry (dropdown->screen, dropdown->monitor_num, &rect);

  fullscreen = window->is_fullscreen;
  if (!fullscreen)
    {
      /* calculate width/height if not fullscreen */
      rect.width *= dropdown->rel_width;
      rect.height *= dropdown->rel_height;
    }

  /* decrease each interval */
  step_size = rect.height * ANIMATION_FPS / dropdown->animation_time;
  if (step_size < 1)
    step_size = 1;

  /* new vbox size */
  gtk_widget_get_preferred_size (terminal_window_get_vbox (window), &req1, NULL);
  vbox_h = req1.height + step_size;
  if (vbox_h > rect.height)
    vbox_h = rect.height;

  /* resize */
  gtk_widget_set_size_request (terminal_window_get_vbox (window), req1.width, vbox_h);
  gtk_window_resize (GTK_WINDOW (window), req1.width, vbox_h);

  /* continue the animation */
  if (vbox_h < rect.height)
    return TRUE;

  /* restore the fullscreen state */
  if (fullscreen)
    gtk_window_fullscreen (GTK_WINDOW (window));

  /* animation complete */
  return FALSE;
}



static gboolean
terminal_window_dropdown_animate_up (gpointer data)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (data);
  TerminalWindow         *window = TERMINAL_WINDOW (data);
  GtkRequisition          req1;
  GdkRectangle            rect;
  gint                    step_size, vbox_h, min_size;
  gboolean                fullscreen;

  /* get window size */
  terminal_window_dropdown_get_monitor_geometry (dropdown->screen, dropdown->monitor_num, &rect);

  fullscreen = window->is_fullscreen;
  if (!fullscreen)
    {
      /* calculate width/height if not fullscreen */
      rect.width *= dropdown->rel_width;
      rect.height *= dropdown->rel_height;
    }

  /* decrease each interval */
  step_size = rect.height * ANIMATION_FPS / dropdown->animation_time;
  if (step_size < 1)
    step_size = 1;

  /* new vbox size */
  gtk_widget_get_preferred_size (terminal_window_get_vbox (window), &req1, NULL);
  vbox_h = req1.height - step_size;

  /* sizes of the widgets that cannot be shrunk */
  gtk_widget_get_preferred_size (terminal_window_get_notebook (window), &req1, NULL);
  min_size = req1.height;
  min_size += terminal_window_get_menubar_height (window);
  min_size += terminal_window_get_toolbar_height (window);

  if (vbox_h < min_size)
    {
      /* animation complete */
      gtk_widget_hide (GTK_WIDGET (dropdown));
      return FALSE;
    }

  /* resize window */
  gtk_widget_set_size_request (terminal_window_get_vbox (window), rect.width, vbox_h);
  gtk_window_resize (GTK_WINDOW (window), rect.width, vbox_h);

  return TRUE;
}



static void
terminal_window_dropdown_animate_destroyed (gpointer data)
{
  TERMINAL_WINDOW_DROPDOWN (data)->animation_timeout_id = 0;
  TERMINAL_WINDOW_DROPDOWN (data)->animation_dir = ANIMATION_DIR_NONE;
}



static void
terminal_window_dropdown_hide (TerminalWindowDropdown *dropdown)
{
  if (dropdown->animation_timeout_id != 0)
    g_source_remove (dropdown->animation_timeout_id);

  if (dropdown->animation_time > 0)
    {
      dropdown->animation_dir = ANIMATION_DIR_UP;
      dropdown->animation_timeout_id =
          gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE, ANIMATION_FPS,
                                        terminal_window_dropdown_animate_up, dropdown,
                                        terminal_window_dropdown_animate_destroyed);
    }
  else
    {
      /* it seems that xfwm4 wants the window unmaximized when it's hidden (bug #15681) */
      if (gtk_window_is_maximized (GTK_WINDOW (dropdown)))
        gtk_window_unmaximize (GTK_WINDOW (dropdown));
      gtk_widget_hide (GTK_WIDGET (dropdown));
    }
}



static void
terminal_window_dropdown_move (GtkWindow *window,
                               gint x,
                               gint y,
                               GdkRectangle monitor_geo)
{
#ifdef HAVE_GTK_LAYER_SHELL
  if (gtk_layer_is_supported ())
    {
      gtk_layer_set_margin (window, GTK_LAYER_SHELL_EDGE_LEFT, x - monitor_geo.x);
      gtk_layer_set_margin (window, GTK_LAYER_SHELL_EDGE_TOP, y - monitor_geo.y);
    }
  else
#endif
    gtk_window_move (window, x, y);
}



static void
terminal_window_dropdown_gdk_screen_get_active (TerminalWindowDropdown *dropdown)
{
#ifdef HAVE_GTK_LAYER_SHELL
  if (gtk_layer_is_supported ())
    {
      GdkDisplay *display = gdk_display_get_default ();
      gint n_monitors = gdk_display_get_n_monitors (display);

      for (gint i = 0; i < n_monitors; i++)
        if (gdk_display_get_monitor (display, i) == dropdown->monitor)
          {
            dropdown->monitor_num = i;
            break;
          }
      dropdown->screen = gdk_screen_get_default ();
    }
  else
#endif
    dropdown->screen = xfce_gdk_screen_get_active (&dropdown->monitor_num);
}



static void
terminal_window_dropdown_show (TerminalWindowDropdown *dropdown,
                               guint32                 timestamp,
                               gboolean                activate)
{
  TerminalWindow    *window = TERMINAL_WINDOW (dropdown);
  TerminalDirection  old_animation_dir = ANIMATION_DIR_NONE;
  GdkRectangle       monitor_geo;
  GtkRequisition     req1;
  gint               w, h;
  gint               x, y;
  gboolean           move_to_active;
  gboolean           keep_above;
  gboolean           visible;
  gint               vbox_h;
  gboolean           fullscreen;

  visible = gdk_window_is_visible (gtk_widget_get_window (GTK_WIDGET (dropdown)));

  if (dropdown->animation_timeout_id != 0)
    {
      old_animation_dir = dropdown->animation_dir;
      g_source_remove (dropdown->animation_timeout_id);
    }

  g_object_get (terminal_window_get_preferences (window),
                "dropdown-move-to-active", &move_to_active,
                NULL);

  /* target screen, either the last one or the active screen */
  if (move_to_active
      || dropdown->screen == NULL
      || dropdown->monitor_num == -1)
    {
      if (dropdown->screen != NULL)
        g_signal_handlers_disconnect_by_func (G_OBJECT (dropdown->screen),
                                              G_CALLBACK (terminal_dropdown_window_screen_size_changed),
                                              dropdown);

      terminal_window_dropdown_gdk_screen_get_active (dropdown);

      /* watch for screen size changes to update terminal geometry accordingly*/
      g_signal_connect (G_OBJECT (dropdown->screen), "size-changed",
                        G_CALLBACK (terminal_dropdown_window_screen_size_changed), dropdown);
    }

  /* get the active monitor size */
  terminal_window_dropdown_get_monitor_geometry (dropdown->screen, dropdown->monitor_num, &monitor_geo);

  /* move window to correct screen */
  gtk_window_set_screen (GTK_WINDOW (dropdown), dropdown->screen);

  /* correct padding with notebook size */
  fullscreen = window->is_fullscreen;
  if (fullscreen)
    {
      /* don't fullscreen during animation*/
      if (dropdown->animation_time > 0)
        gtk_window_unfullscreen (GTK_WINDOW (window));

      /* use monitor geometry */
      w = monitor_geo.width;
      h = monitor_geo.height;
    }
  else
    {
      /* calculate size */
      w = monitor_geo.width * dropdown->rel_width;
      h = monitor_geo.height * dropdown->rel_height;
    }

  /* vbox size if not animated */
  vbox_h = h;

  /* vbox start height for animation */
  if (dropdown->animation_time > 0)
    {
      if (!visible)
        {
          /* start animation collapsed */
          vbox_h = 0;
        }
      else if (old_animation_dir == ANIMATION_DIR_UP)
        {
          /* pick up where we aborted */
          gtk_widget_get_preferred_size (terminal_window_get_vbox (window), &req1, NULL);
          vbox_h = req1.height;
        }
    }

  /* resize */
  gtk_widget_set_size_request (terminal_window_get_vbox (window), w, vbox_h);

  /* calc position */
  x = monitor_geo.x + (monitor_geo.width - w) * dropdown->rel_position;
  y = monitor_geo.y + (monitor_geo.height - h) * dropdown->rel_position_vertical;

  /* move */
  terminal_window_dropdown_move (GTK_WINDOW (dropdown), x, y, monitor_geo);

  /* show window */
  if (!visible)
    gtk_window_present_with_time (GTK_WINDOW (dropdown), timestamp);

  /* move window after showing: https://bugzilla.xfce.org/show_bug.cgi?id=10713 */
  terminal_window_dropdown_move (GTK_WINDOW (dropdown), x, y, monitor_geo);

  /* force focus to the window */
  if (activate)
    terminal_util_activate_window (GTK_WINDOW (dropdown));

  if (dropdown->animation_time > 0
      && vbox_h < h)
    {
      dropdown->animation_dir = ANIMATION_DIR_DOWN;
      dropdown->animation_timeout_id =
          gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE, ANIMATION_FPS,
                                        terminal_window_dropdown_animate_down, dropdown,
                                        terminal_window_dropdown_animate_destroyed);
    }
  else
    {
      g_object_get (terminal_window_get_preferences (window),
                    "dropdown-keep-above", &keep_above,
                    NULL);
      gtk_window_set_keep_above (GTK_WINDOW (dropdown), keep_above);

      /* make sure all the content fits */
      gtk_window_resize (GTK_WINDOW (dropdown), w, h);
    }
}



static void
terminal_window_dropdown_toggle_real (TerminalWindowDropdown *dropdown,
                                      guint32                 timestamp,
                                      gboolean                force_show)
{
  TerminalWindow *window = TERMINAL_WINDOW (dropdown);
  gboolean        toggle_focus;

  if (!force_show
      && gdk_window_is_visible (gtk_widget_get_window (GTK_WIDGET (dropdown)))
      && dropdown->animation_dir != ANIMATION_DIR_UP)
    {
      g_object_get (terminal_window_get_preferences (window),
                    "dropdown-toggle-focus", &toggle_focus,
                    NULL);

      /* if the focus was lost for 0.1 second and toggle-focus is used, we had
       * focus until the shortcut was pressed, and then we hide the window */
      if (!toggle_focus
          || dropdown->focus_out_time == 0
          || (g_get_real_time () - dropdown->focus_out_time) < G_USEC_PER_SEC / 10)
        {
          /* hide */
          terminal_window_dropdown_hide (dropdown);
        }
      else
        {
          terminal_window_dropdown_show (dropdown, timestamp, TRUE);
        }
    }
  else
    {
      /* popup */
      terminal_window_dropdown_show (dropdown, timestamp, TRUE);
    }
}



static guint32
terminal_window_dropdown_get_timestamp (const gchar *startup_id)
{
  const gchar *timestr;
  guint32      timestamp;
  gchar       *end;

  /* extract the time from the id */
  if (startup_id != NULL)
    {
      timestr = g_strrstr (startup_id, "_TIME");
      if (G_LIKELY (timestr != NULL))
        {
          timestr += 5;
          errno = 0;

          /* translate into integer */
          timestamp = strtoul (timestr, &end, 0);
          if (end != timestr && errno == 0)
            return timestamp;
        }
    }

  return GDK_CURRENT_TIME;
}



static void
terminal_dropdown_window_screen_size_changed (GdkScreen              *screen,
                                              TerminalWindowDropdown *dropdown)
{
  /* resize/move terminal window due to a screen size change */
  if (gtk_widget_get_visible (GTK_WIDGET (dropdown)))
    terminal_window_dropdown_show (dropdown, 0, FALSE);
}



GtkWidget *
terminal_window_dropdown_new (const gchar        *role,
                              const gchar        *icon,
                              gboolean            fullscreen,
                              TerminalVisibility  menubar,
                              TerminalVisibility  toolbar)
{
  TerminalWindow *window;
  gboolean        show_menubar;
  gboolean        show_toolbar;
  gboolean        show_icon;
  GValue          value = G_VALUE_INIT;

  if (G_LIKELY (role == NULL))
    role = PACKAGE_NAME "-dropdown";

  window = g_object_new (TERMINAL_TYPE_WINDOW_DROPDOWN,
                         "role", role,
                         "icon-name", icon != NULL ? icon : "org.xfce.terminal",
                         NULL);

  /* read default preferences */
  g_object_get (terminal_window_get_preferences (window),
                "misc-menubar-default", &show_menubar,
                "misc-toolbar-default", &show_toolbar,
                "dropdown-status-icon", &show_icon,
                NULL);

  /* update status bar icon */
  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, show_icon);
  terminal_window_dropdown_set_property (G_OBJECT (window),
                                         PROP_DROPDOWN_STATUS_ICON, &value, NULL);

  /* setup menubar visibility */
  if (G_LIKELY (menubar != TERMINAL_VISIBILITY_DEFAULT))
    show_menubar = (menubar == TERMINAL_VISIBILITY_SHOW);
  gtk_widget_set_visible (window->menubar, show_menubar);

  /* setup toolbar visibility */
  if (G_LIKELY (toolbar != TERMINAL_VISIBILITY_DEFAULT))
    show_toolbar = (toolbar == TERMINAL_VISIBILITY_SHOW);
  gtk_widget_set_visible (window->toolbar, show_toolbar);

  /* setup full screen */
  window->is_fullscreen = fullscreen && window->fullscreen_supported;

  return GTK_WIDGET (window);
}



void
terminal_window_dropdown_toggle (TerminalWindowDropdown *dropdown,
                                 const gchar            *startup_id,
                                 gboolean                force_show)
{
  guint32 timestamp;

  /* toggle window */
  timestamp = terminal_window_dropdown_get_timestamp (startup_id);
  terminal_window_dropdown_toggle_real (dropdown, timestamp, force_show);

  /* window is focussed or hidden */
  if (startup_id != NULL)
    gdk_notify_startup_complete_with_id (startup_id);
}



void
terminal_window_dropdown_get_size (TerminalWindowDropdown *dropdown,
                                   TerminalScreen         *screen,
                                   glong                  *grid_width,
                                   glong                  *grid_height)
{
  GdkScreen      *gdkscreen;
  gint            monitor_num;
  GdkRectangle    monitor_geo;
  gint            xpad, ypad;
  glong           char_width, char_height;
  GtkRequisition  req;

  /* get the active monitor size */
  gdkscreen = xfce_gdk_screen_get_active (&monitor_num);
  terminal_window_dropdown_get_monitor_geometry (gdkscreen, monitor_num, &monitor_geo);

  /* get terminal size */
  terminal_screen_get_geometry (screen, &char_width, &char_height, &xpad, &ypad);

  /* correct padding with visible widgets */
  gtk_widget_get_preferred_size (terminal_window_get_vbox (TERMINAL_WINDOW (dropdown)), &req, NULL);
  xpad += 2;
  ypad += req.height;

  /* return grid size */
  if (G_LIKELY (grid_width != NULL))
    *grid_width = MAX (1, ((monitor_geo.width * dropdown->rel_width) - xpad) / char_width);
  if (G_LIKELY (grid_height != NULL))
    *grid_height = MAX (1, ((monitor_geo.height * dropdown->rel_height) - ypad) / char_height);
}



void
terminal_window_dropdown_update_geometry (TerminalWindowDropdown *dropdown)
{
  terminal_return_if_fail (TERMINAL_IS_WINDOW_DROPDOWN (dropdown));

  /* update geometry if toolbar or menu is shown */
  if (gtk_widget_get_visible (GTK_WIDGET (dropdown))
      && dropdown->animation_dir == ANIMATION_DIR_NONE)
    terminal_window_dropdown_show (dropdown, 0, TRUE);
}



#ifdef HAVE_GTK_LAYER_SHELL
static gboolean
focus_in_event (TerminalWindow *window)
{
  gboolean keep_above;
  g_object_get (terminal_window_get_preferences (window), "dropdown-keep-above", &keep_above, NULL);
  gtk_layer_set_layer (GTK_WINDOW (window), keep_above ? GTK_LAYER_SHELL_LAYER_TOP
                                                       : GTK_LAYER_SHELL_LAYER_BOTTOM);
  g_signal_handlers_disconnect_by_func (window, focus_in_event, NULL);

  return FALSE;
}
#endif



/**
 * terminal_window_dropdown_ignore_next_focus_out_event:
 * @dropdown  : A #TerminalWindowDropdown.
 *
 * Used to avoid hiding the terminal when it loses focus. Used when opening dialogs (like the `About` dialog).
 **/
void
terminal_window_dropdown_ignore_next_focus_out_event (TerminalWindowDropdown *dropdown)
{
  dropdown->ignore_next_focus_out_event = TRUE;
#ifdef HAVE_GTK_LAYER_SHELL
  if (gtk_layer_is_supported ())
    {
      /* ensure opened dialog is above the terminal at least until next focus-in event */
      gtk_layer_set_layer (GTK_WINDOW (dropdown), GTK_LAYER_SHELL_LAYER_BOTTOM);
      g_signal_connect (dropdown, "focus-in-event", G_CALLBACK (focus_in_event), NULL);
    }
#endif
}
