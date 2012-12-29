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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

#include <terminal/terminal-private.h>
#include <terminal/terminal-window.h>
#include <terminal/terminal-dialogs.h>
#include <terminal/terminal-window-dropdown.h>
#include <terminal/terminal-preferences-dropdown-dialog.h>



enum
{
  PROP_0,
  PROP_DROPDOWN_WIDTH,
  PROP_DROPDOWN_HEIGHT,
  PROP_DROPDOWN_POSITION,
  PROP_DROPDOWN_OPACITY,
  PROP_DROPDOWN_STATUS_ICON,
  PROP_DROPDOWN_KEEP_ABOVE,
  N_PROPERTIES
};



static void            terminal_window_dropdown_finalize                      (GObject                *object);
static void            terminal_window_dropdown_set_property                  (GObject                *object,
                                                                               guint                   prop_id,
                                                                               const GValue           *value,
                                                                               GParamSpec             *pspec);
static gboolean        terminal_window_dropdown_focus_in_event                (GtkWidget              *widget,
                                                                               GdkEventFocus          *event);
static gboolean        terminal_window_dropdown_focus_out_event               (GtkWidget              *widget,
                                                                               GdkEventFocus          *event);
static gboolean        terminal_window_dropdown_status_icon_press_event       (GtkStatusIcon          *status_icon,
                                                                               GdkEventButton         *event,
                                                                               TerminalWindowDropdown *dropdown);
static void            terminal_window_dropdown_position                      (TerminalWindowDropdown *dropdown,
                                                                               guint32                 timestamp);
static void            terminal_window_dropdown_toggle_real                   (TerminalWindowDropdown *dropdown,
                                                                               guint32                 timestamp);
static void            terminal_window_dropdown_preferences                   (TerminalWindowDropdown *dropdown);



struct _TerminalWindowDropdownClass
{
  TerminalWindowClass __parent__;
};

struct _TerminalWindowDropdown
{
  TerminalWindow __parent__;

  TerminalPreferences *preferences;

  /* ui widgets */
  GtkWidget           *keep_open;

  /* measurements */
  gdouble              rel_width;
  gdouble              rel_height;
  gdouble              rel_position;

  GtkWidget           *preferences_dialog;

  GtkStatusIcon       *status_icon;

  /* last screen and monitor */
  GdkScreen           *screen;
  gint                 monitor_num;

  /* server time of focus out with grab */
  gint64               focus_out_time;
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

  /* install all properties */
  g_object_class_install_properties (gobject_class, N_PROPERTIES, dropdown_props);
}



static void
terminal_window_dropdown_init (TerminalWindowDropdown *dropdown)
{
  TerminalWindow *window = TERMINAL_WINDOW (dropdown);
  GtkAction      *action;
  GtkWidget      *hbox;
  GtkWidget      *button;
  GtkWidget      *img;
  guint           n;
  const gchar    *name;
  gboolean        keep_open;

  dropdown->preferences = terminal_preferences_get ();
  dropdown->rel_width = 0.80;
  dropdown->rel_height = 0.50;
  dropdown->rel_position = 0.50;

  /* shared setting to disable some functionality in TerminalWindow */
  window->drop_down = TRUE;

  /* default window settings */
  gtk_window_set_resizable (GTK_WINDOW (dropdown), FALSE);
  gtk_window_set_decorated (GTK_WINDOW (dropdown), FALSE);
  gtk_window_set_gravity (GTK_WINDOW (dropdown), GDK_GRAVITY_STATIC);
  gtk_window_set_type_hint  (GTK_WINDOW (dropdown), GDK_WINDOW_TYPE_HINT_DIALOG); /* avoids smart placement */
  gtk_window_stick (GTK_WINDOW (dropdown));

  /* this avoids to return focus to the window after dialog changes,
   * but we have terminal_activate_window() for that */
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dropdown), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dropdown), TRUE);

  /* adjust notebook for drop-down usage */
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), TRUE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (window->notebook), GTK_POS_BOTTOM);
  terminal_set_style_thinkess (window->notebook, 1);

  /* actions we don't want */
  action = gtk_action_group_get_action (window->action_group, "show-borders");
  gtk_action_set_visible (action, FALSE);

  /* notebook buttons */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_notebook_set_action_widget (GTK_NOTEBOOK (window->notebook), hbox, GTK_PACK_END);
  gtk_widget_show (hbox);

  button = dropdown->keep_open = gtk_toggle_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (button, _("Keep window open when it loses focus"));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  gtk_widget_show (button);

  g_object_get (G_OBJECT (dropdown->preferences), "dropdown-keep-open-default", &keep_open, NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), keep_open);

  img = gtk_image_new_from_stock (GTK_STOCK_GOTO_BOTTOM, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (img);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (button, _("Drop-down Preferences..."));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  g_signal_connect_swapped (G_OBJECT (button), "clicked",
      G_CALLBACK (terminal_window_dropdown_preferences), dropdown);
  gtk_widget_show (button);

  img = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (img);

  /* connect bindings */
  for (n = 1; n < N_PROPERTIES; n++)
    {
      name = g_param_spec_get_name (dropdown_props[n]);
      g_object_bind_property (G_OBJECT (dropdown->preferences), name,
                              G_OBJECT (dropdown), name,
                              G_BINDING_SYNC_CREATE);
    }
}



static void
terminal_window_dropdown_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (object);
  gdouble                 opacity;
  GdkScreen              *screen;

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

    case PROP_DROPDOWN_OPACITY:
      screen = gtk_window_get_screen (GTK_WINDOW (dropdown));
      if (gdk_screen_is_composited (screen))
        opacity = g_value_get_uint (value) / 100.0;
      else
        opacity = 1.00;

      gtk_window_set_opacity (GTK_WINDOW (dropdown), opacity);
      return;

    case PROP_DROPDOWN_STATUS_ICON:
      if (g_value_get_boolean (value))
        {
          if (dropdown->status_icon == NULL)
            {
              dropdown->status_icon = gtk_status_icon_new_from_icon_name ("utilities-terminal");
              gtk_status_icon_set_name (dropdown->status_icon, PACKAGE_NAME);
              gtk_status_icon_set_title (dropdown->status_icon, _("Drop-down Terminal"));
              gtk_status_icon_set_tooltip_text (dropdown->status_icon, _("Toggle Drop-down Terminal"));
              g_signal_connect (G_OBJECT (dropdown->status_icon), "button-press-event",
                  G_CALLBACK (terminal_window_dropdown_status_icon_press_event), dropdown);
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
      if (dropdown->preferences_dialog != NULL)
        terminal_activate_window (GTK_WINDOW (dropdown->preferences_dialog));
      return;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }

  if (gtk_widget_get_visible (GTK_WIDGET (dropdown)))
    terminal_window_dropdown_position (dropdown, 0);
}



static void
terminal_window_dropdown_finalize (GObject *object)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (object);

  g_object_unref (dropdown->preferences);

  if (dropdown->status_icon != NULL)
    g_object_unref (G_OBJECT (dropdown->status_icon));

  (*G_OBJECT_CLASS (terminal_window_dropdown_parent_class)->finalize) (object);
}



static gboolean
terminal_window_dropdown_focus_in_event (GtkWidget     *widget,
                                         GdkEventFocus *event)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (widget);

  /* unset */
  dropdown->focus_out_time = 0;

  return (*GTK_WIDGET_CLASS (terminal_window_dropdown_parent_class)->focus_in_event) (widget, event);;
}



static gboolean
terminal_window_dropdown_focus_out_event (GtkWidget     *widget,
                                          GdkEventFocus *event)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (widget);
  gboolean                retval;
  GdkGrabStatus           status;

  /* let Gtk do its thingy */
  retval = (*GTK_WIDGET_CLASS (terminal_window_dropdown_parent_class)->focus_out_event) (widget, event);

  /* check if keep open is not enabled */
  if (gtk_widget_get_visible (widget)
      && TERMINAL_WINDOW (dropdown)->n_child_windows == 0
      && gtk_grab_get_current () == NULL) /* popup menu check */
    {
      if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dropdown->keep_open)))
        {
          /* check if the user is not pressing a key */
          status = gdk_keyboard_grab (event->window, FALSE, GDK_CURRENT_TIME);
          if (status == GDK_GRAB_SUCCESS)
            {
              /* drop the grab */
              gdk_keyboard_ungrab (GDK_CURRENT_TIME);
         
              /* hide the window */
              gtk_widget_hide (GTK_WIDGET (dropdown));

              return retval;
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
  if (gtk_widget_get_visible (GTK_WIDGET (dropdown)))
    gtk_widget_hide (GTK_WIDGET (dropdown));
  else
    terminal_window_dropdown_position (dropdown, event->time);

  return FALSE;
}



static void
terminal_window_dropdown_position (TerminalWindowDropdown *dropdown,
                                   guint32                 timestamp)
{
  TerminalWindow *window = TERMINAL_WINDOW (dropdown);
  gint            w, h;
  GdkRectangle    monitor_geo;
  gint            x_dest, y_dest;
  gint            xpad, ypad;
  glong           char_width, char_height;
  GtkRequisition  req1, req2;
  gboolean        move_to_active;

  /* show window */
  if (!gtk_widget_get_visible (GTK_WIDGET (dropdown)))
    gtk_window_present_with_time (GTK_WINDOW (dropdown), timestamp);

  g_object_get (G_OBJECT (dropdown->preferences),
                "dropdown-move-to-active", &move_to_active, NULL);

  if (move_to_active
      || dropdown->screen == NULL
      || dropdown->monitor_num == -1)
    dropdown->screen = xfce_gdk_screen_get_active (&dropdown->monitor_num);

  /* get the active monitor size */
  gdk_screen_get_monitor_geometry (dropdown->screen, dropdown->monitor_num, &monitor_geo);

  /* move window to correct screen */
  gtk_window_set_screen (GTK_WINDOW (dropdown), dropdown->screen);

  /* calculate size */
  w = monitor_geo.width * dropdown->rel_width;
  h = monitor_geo.height * dropdown->rel_height;

  /* get terminal size */
  terminal_screen_get_geometry (window->active, &char_width, &char_height, &xpad, &ypad);

  /* correct padding with notebook size */
  gtk_widget_size_request (GTK_WIDGET (window->notebook), &req1);
  gtk_widget_size_request (GTK_WIDGET (window->active), &req2);
  xpad += MAX (req1.width - req2.width, 0);
  ypad += MAX (req1.height - req2.height, 0);

  /* minimize to fit terminal charaters */
  w -= (w - xpad) % char_width;
  h -= (h - ypad) % char_height;

  /* resize the notebook */
  gtk_widget_set_size_request (window->notebook, w, h);

  /* calc position */
  x_dest = monitor_geo.x + (monitor_geo.width - w) * dropdown->rel_position;
  y_dest = monitor_geo.y;

  /* move */
  gtk_window_move (GTK_WINDOW (dropdown), x_dest, y_dest);
}



static void
terminal_window_dropdown_toggle_real (TerminalWindowDropdown *dropdown,
                                      guint32                 timestamp)
{
  gboolean toggle_focus;

  if (gtk_widget_get_visible (GTK_WIDGET (dropdown)))
    {
      g_object_get (G_OBJECT (dropdown->preferences), "dropdown-toggle-focus", &toggle_focus, NULL);

      /* if the focus was lost for 0.1 second and toggle-focus is used, we had
       * focus until the shortcut was pressed, and then we hide the window */
      if (!toggle_focus
          || (g_get_real_time () - dropdown->focus_out_time) < G_USEC_PER_SEC / 10)
        {
          /* hide */
          gtk_widget_hide (GTK_WIDGET (dropdown));
        }
      else
        {
          terminal_window_dropdown_position (dropdown, timestamp);
          terminal_activate_window (GTK_WINDOW (dropdown));
        }
    }
  else
    {
      /* popup */
      terminal_window_dropdown_position (dropdown, timestamp);
    }
}



static void
terminal_window_dropdown_preferences_died (gpointer  user_data,
                                           GObject  *where_the_object_was)
{
  TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (user_data);

  dropdown->preferences_dialog = NULL;
  TERMINAL_WINDOW (dropdown)->n_child_windows--;

  terminal_activate_window (GTK_WINDOW (dropdown));
}



static void
terminal_window_dropdown_preferences (TerminalWindowDropdown *dropdown)
{
  if (dropdown->preferences_dialog == NULL)
    {
      dropdown->preferences_dialog = terminal_preferences_dropdown_dialog_new ();
      if (G_LIKELY (dropdown->preferences_dialog != NULL))
        {
          TERMINAL_WINDOW (dropdown)->n_child_windows++;
          g_object_weak_ref (G_OBJECT (dropdown->preferences_dialog),
                             terminal_window_dropdown_preferences_died, dropdown);
        }
    }

  if (dropdown->preferences_dialog != NULL)
    {
      gtk_window_set_transient_for (GTK_WINDOW (dropdown->preferences_dialog), GTK_WINDOW (dropdown));
      gtk_window_present (GTK_WINDOW (dropdown->preferences_dialog));
      gtk_window_set_modal (GTK_WINDOW (dropdown->preferences_dialog), TRUE);
    }
}



static guint32
terminal_window_dropdown_get_timestamp (GtkWidget   *widget,
                                        const gchar *startup_id)
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



GtkWidget *
terminal_window_dropdown_new (void)
{
  return g_object_new (TERMINAL_TYPE_WINDOW_DROPDOWN, NULL);
}



void
terminal_window_dropdown_toggle (TerminalWindowDropdown *dropdown,
                                 const gchar            *startup_id)
{
  guint32 timestamp;

  /* toggle window */
  timestamp = terminal_window_dropdown_get_timestamp (GTK_WIDGET (dropdown), startup_id);
  terminal_window_dropdown_toggle_real (dropdown, timestamp);

  /* window is focussed or hidden */
  if (startup_id != NULL)
    gdk_notify_startup_complete_with_id (startup_id);
}
