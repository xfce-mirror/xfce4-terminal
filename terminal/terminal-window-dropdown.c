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



static void            terminal_window_dropdown_finalize                      (GObject                *object);
static gboolean        terminal_window_dropdown_focus_out_event               (GtkWidget              *widget,
                                                                               GdkEventFocus          *event);
static void            terminal_window_dropdown_position                      (TerminalWindowDropdown *dropdown);
static void            terminal_window_dropdown_preferences                   (TerminalWindowDropdown *dropdown);


typedef enum
{
  DROPDOWN_STATE_HIDDEN,
  DROPDOWN_STATE_VISIBLE,
}
DropdownState;

struct _TerminalWindowDropdownClass
{
  TerminalWindowClass __parent__;
};

struct _TerminalWindowDropdown
{
  TerminalWindow __parent__;

  /* ui widgets */
  GtkWidget     *keep_open;

  /* size */
  gdouble        rel_width;
  gdouble        rel_height;
};



G_DEFINE_TYPE (TerminalWindowDropdown, terminal_window_dropdown, TERMINAL_TYPE_WINDOW)



static void
terminal_window_dropdown_class_init (TerminalWindowDropdownClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_window_dropdown_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->focus_out_event = terminal_window_dropdown_focus_out_event;
}



static void
terminal_window_dropdown_init (TerminalWindowDropdown *dropdown)
{
  TerminalWindow *window = TERMINAL_WINDOW (dropdown);
  GtkAction      *action;
  GtkWidget      *hbox;
  GtkWidget      *button;
  GtkWidget      *img;

  /* shared setting to disable some functionality in TerminalWindow */
  window->drop_down = TRUE;

  dropdown->rel_width = 0.8;
  dropdown->rel_height = 0.6;

  /* default window settings */
  gtk_window_set_resizable (GTK_WINDOW (dropdown), FALSE);
  gtk_window_set_decorated (GTK_WINDOW (dropdown), FALSE);
  gtk_window_set_keep_above (GTK_WINDOW (dropdown), TRUE);
  gtk_window_set_gravity (GTK_WINDOW (dropdown), GDK_GRAVITY_STATIC);
  gtk_window_set_type_hint  (GTK_WINDOW (dropdown), GDK_WINDOW_TYPE_HINT_DIALOG); /* avoid smart placement */
  gtk_window_set_opacity (GTK_WINDOW (dropdown), 0.85);
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
}



static void
terminal_window_dropdown_finalize (GObject *object)
{
  //TerminalWindowDropdown *dropdown = TERMINAL_WINDOW_DROPDOWN (object);

  (*G_OBJECT_CLASS (terminal_window_dropdown_parent_class)->finalize) (object);
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
      && gtk_grab_get_current () == NULL /* popup menu check */
      && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dropdown->keep_open)))
    {
      /* check if the user is not pressing a key */
      status = gdk_keyboard_grab (event->window, FALSE, GDK_CURRENT_TIME);
      if (status == GDK_GRAB_SUCCESS)
        {
          /* drop the grab */
          gdk_keyboard_ungrab (GDK_CURRENT_TIME);

          /* hide the window */
          gtk_widget_hide (GTK_WIDGET (dropdown));
        }
    }

  return retval;
}



static void
terminal_window_dropdown_position (TerminalWindowDropdown *dropdown)
{
  TerminalWindow *window = TERMINAL_WINDOW (dropdown);
  gint            w, h;
  GdkRectangle    monitor_geo;
  gint            x_dest, y_dest;
  GdkScreen      *screen;
  gint            monitor_num;
  gint            xpad, ypad;
  glong           char_width, char_height;
  GtkRequisition  req1, req2;

  /* nothing to do if the window is hidden */
  if (!gtk_widget_get_visible (GTK_WIDGET (dropdown)))
    return;

  /* get the active monitor size */
  screen = xfce_gdk_screen_get_active (&monitor_num);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor_geo);

  /* move window to correct screen */
  gtk_window_set_screen (GTK_WINDOW (dropdown), screen);

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
  x_dest = monitor_geo.x + (monitor_geo.width - w) / 2;
  y_dest = monitor_geo.y;

  /* move */
  gtk_window_move (GTK_WINDOW (dropdown), x_dest, y_dest);
}



static void
terminal_window_dropdown_preferences (TerminalWindowDropdown *dropdown)
{

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

  if (gtk_widget_get_visible (GTK_WIDGET (dropdown)))
    {
      /* hide */
      gtk_widget_hide (GTK_WIDGET (dropdown));
    }
  else
    {
      /* show with event time */
      timestamp = terminal_window_dropdown_get_timestamp (GTK_WIDGET (dropdown), startup_id);
      gtk_window_present_with_time (GTK_WINDOW (dropdown), timestamp);

      /* check position */
      terminal_window_dropdown_position (dropdown);
    }

  /* window is focussed or hidden */
  if (startup_id != NULL)
    gdk_notify_startup_complete_with_id (startup_id);
}
