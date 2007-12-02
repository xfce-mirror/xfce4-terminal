/* $Id$ */
/*-
 * Copyright (c) 2004-2007 os-cillation e.K.
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
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gdk/gdkkeysyms.h>

#include <terminal/terminal-dialogs.h>
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-marshal.h>
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-widget.h>



#define USERCHARS       "-A-Za-z0-9"
#define PASSCHARS       "-A-Za-z0-9,?;.:/!%$^*&~\"#'"
#define HOSTCHARS       "-A-Za-z0-9"
#define USER            "[" USERCHARS "]+(:["PASSCHARS "]+)?"
#define MATCH_BROWSER1  "((file|https?|ftps?)://(" USER "@)?)[" HOSTCHARS ".]+(:[0-9]+)?" \
                        "(/[-A-Za-z0-9_$.+!*(),;:@&=?/~#%]*[^]'.}>) \t\r\n,\\\"])?"
#define MATCH_BROWSER2  "(www|ftp)[" HOSTCHARS "]*\\.[" HOSTCHARS ".]+(:[0-9]+)?" \
                        "(/[-A-Za-z0-9_$.+!*(),;:@&=?/~#%]*[^]'.}>) \t\r\n,\\\"])?"
#if !defined(__GLIBC__)
#define MATCH_MAILER    "(mailto:)?[a-z0-9][a-z0-9.-]*@[a-z0-9][a-z0-9-]*(\\.[a-z0-9][a-z0-9-]*)+"
#else
#define MATCH_MAILER    "\\<(mailto:)?[a-z0-9][a-z0-9.-]*@[a-z0-9][a-z0-9-]*(\\.[a-z0-9][a-z0-9-]*)+\\>"
#endif



enum
{
  GET_CONTEXT_MENU,
  LAST_SIGNAL,
};

enum
{
  TARGET_URI_LIST,
  TARGET_UTF8_STRING,
  TARGET_TEXT,
  TARGET_COMPOUND_TEXT,
  TARGET_STRING,
  TARGET_TEXT_PLAIN,
  TARGET_MOZ_URL,
  TARGET_APPLICATION_X_COLOR,
};



static void     terminal_widget_class_init            (TerminalWidgetClass  *klass);
static void     terminal_widget_init                  (TerminalWidget       *widget);
static void     terminal_widget_finalize              (GObject              *object);
static gboolean terminal_widget_button_press_event    (GtkWidget            *widget,
                                                       GdkEventButton       *event);
static void     terminal_widget_drag_data_received    (GtkWidget            *widget,
                                                       GdkDragContext       *context,
                                                       gint                  x,
                                                       gint                  y,
                                                       GtkSelectionData     *selection_data,
                                                       guint                 info,
                                                       guint                 time);
static gboolean terminal_widget_key_press_event       (GtkWidget            *widget,
                                                       GdkEventKey          *event);
static void     terminal_widget_open_uri              (TerminalWidget       *widget,
                                                       const gchar          *link,
                                                       gint                  tag);
static void     terminal_widget_update_highlight_urls (TerminalWidget       *widget);



struct _TerminalWidgetClass
{
  VteTerminalClass __parent__;

  /* signals */
  GtkWidget*  (*get_context_menu) (TerminalWidget        *widget);
};

struct _TerminalWidget
{
  VteTerminal __parent__;

  /*< private >*/
  TerminalPreferences *preferences;
  gint                 tag_browser1;
  gint                 tag_browser2;
  gint                 tag_mailer;
};



static guint widget_signals[LAST_SIGNAL];



static const GtkTargetEntry targets[] =
{
  { "text/uri-list", 0, TARGET_URI_LIST },
  { "text/x-moz-url", 0, TARGET_MOZ_URL },
  { "UTF8_STRING", 0, TARGET_UTF8_STRING },
  { "TEXT", 0, TARGET_TEXT },
  { "COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT },
  { "STRING", 0, TARGET_STRING },
  { "text/plain", 0, TARGET_TEXT_PLAIN },
  { "application/x-color", 0, TARGET_APPLICATION_X_COLOR },
};



G_DEFINE_TYPE (TerminalWidget, terminal_widget, VTE_TYPE_TERMINAL);



static void
terminal_widget_class_init (TerminalWidgetClass *klass)
{
  GtkWidgetClass  *gtkwidget_class;
  GObjectClass    *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_widget_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event = terminal_widget_button_press_event;
  gtkwidget_class->drag_data_received = terminal_widget_drag_data_received;
  gtkwidget_class->key_press_event    = terminal_widget_key_press_event;

  /**
   * TerminalWidget::get-context-menu:
   **/
  widget_signals[GET_CONTEXT_MENU] =
    g_signal_new (I_("context-menu"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWidgetClass, get_context_menu),
                  NULL, NULL,
                  _terminal_marshal_OBJECT__VOID,
                  GTK_TYPE_MENU, 0);
}



static void
terminal_widget_init (TerminalWidget *widget)
{
  widget->tag_browser1 = -1;
  widget->tag_browser2 = -1;
  widget->tag_mailer   = -1;

  /* query preferences connection */
  widget->preferences = terminal_preferences_get ();

  /* setup Drag'n'Drop support */
  gtk_drag_dest_set (GTK_WIDGET (widget),
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     targets, G_N_ELEMENTS (targets),
                     GDK_ACTION_COPY | GDK_ACTION_LINK);

  /* monitor the misc-highlight-urls setting */
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::misc-highlight-urls",
                            G_CALLBACK (terminal_widget_update_highlight_urls), widget);

  /* apply the initial misc-highlight-urls setting */
  terminal_widget_update_highlight_urls (widget);
}



static void
terminal_widget_finalize (GObject *object)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  /* disconnect the misc-highlight-urls watch */
  g_signal_handlers_disconnect_by_func (G_OBJECT (widget->preferences), G_CALLBACK (terminal_widget_update_highlight_urls), widget);

  /* disconnect from the preferences */
  g_object_unref (G_OBJECT (widget->preferences));

  (*G_OBJECT_CLASS (terminal_widget_parent_class)->finalize) (object);
}



static void
terminal_widget_context_menu_copy (TerminalWidget *widget,
                                   GtkWidget      *item)
{
  GtkClipboard *clipboard;
  const gchar  *link;
  GdkDisplay   *display;

  link = g_object_get_data (G_OBJECT (item), "terminal-widget-link");
  if (G_LIKELY (link != NULL))
    {
      display = gtk_widget_get_display (GTK_WIDGET (widget));

      /* copy the URI to "CLIPBOARD" */
      clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_CLIPBOARD);
      gtk_clipboard_set_text (clipboard, link, -1);

      /* copy the URI to "PRIMARY" */
      clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_PRIMARY);
      gtk_clipboard_set_text (clipboard, link, -1);
    }
}



static void
terminal_widget_context_menu_open (TerminalWidget *widget,
                                   GtkWidget      *item)
{
  const gchar *link;
  gint        *tag;

  link = g_object_get_data (G_OBJECT (item), "terminal-widget-link");
  tag  = g_object_get_data (G_OBJECT (item), "terminal-widget-tag");

  if (G_LIKELY (link != NULL && tag != NULL))
    terminal_widget_open_uri (widget, link, *tag);
}



static void
terminal_widget_context_menu (TerminalWidget *widget,
                              gint            button,
                              gint            time,
                              gint            x,
                              gint            y)
{
  VteTerminal *terminal = VTE_TERMINAL (widget);
  GMainLoop   *loop;
  GtkWidget   *menu = NULL;
  GtkWidget   *item_copy = NULL;
  GtkWidget   *item_open = NULL;
  GtkWidget   *item_separator = NULL;
  GList       *children;
  gchar       *match;
  guint        id;
  gint         tag;

  g_signal_emit (G_OBJECT (widget), widget_signals[GET_CONTEXT_MENU], 0, &menu);
  if (G_UNLIKELY (menu == NULL))
    return;

  /* check if we have a match */
  match = vte_terminal_match_check (terminal, x / terminal->char_width, y / terminal->char_height, &tag);
  if (G_UNLIKELY (match != NULL))
    {
      /* prepend a separator to the menu if it does not already contain one */
      children = gtk_container_get_children (GTK_CONTAINER (menu));
      item_separator = g_list_nth_data (children, 0);
      if (G_LIKELY (item_separator != NULL && !GTK_IS_SEPARATOR_MENU_ITEM (item_separator)))
        {
          item_separator = gtk_separator_menu_item_new ();
          gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item_separator);
          gtk_widget_show (item_separator);
        }
      else
        item_separator = NULL;
      g_list_free (children);

      /* create menu items with appriorate labels */
      if (tag == widget->tag_mailer)
        {
          item_copy = gtk_menu_item_new_with_label (_("Copy Email Address"));
          item_open = gtk_menu_item_new_with_label (_("Compose Email"));
        }
      else
        {
          item_copy = gtk_menu_item_new_with_label (_("Copy Link Address"));
          item_open = gtk_menu_item_new_with_label (_("Open Link"));
        }

      /* prepend the "COPY" menu item */
      g_object_set_data_full (G_OBJECT (item_copy), I_("terminal-widget-link"), g_strdup (match), g_free);
      g_signal_connect_swapped (G_OBJECT (item_copy), "activate", G_CALLBACK (terminal_widget_context_menu_copy), widget);
      gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item_copy);
      gtk_widget_show (item_copy);

      /* prepend the "OPEN" menu item */
      g_object_set_data_full (G_OBJECT (item_open), I_("terminal-widget-link"), g_strdup (match), g_free);
      g_object_set_data_full (G_OBJECT (item_open), I_("terminal-widget-tag"), g_memdup (&tag, sizeof (tag)), g_free);
      g_signal_connect_swapped (G_OBJECT (item_open), "activate", G_CALLBACK (terminal_widget_context_menu_open), widget);
      gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item_open);
      gtk_widget_show (item_open);

      g_free (match);
    }

  /* take a reference on the menu */
  g_object_ref (G_OBJECT (menu));
  gtk_object_sink (GTK_OBJECT (menu));

  if (time < 0)
    time = gtk_get_current_event_time ();

  loop = g_main_loop_new (NULL, FALSE);

  /* connect the deactivate handler */
  id = g_signal_connect_swapped (G_OBJECT (menu), "deactivate", G_CALLBACK (g_main_loop_quit), loop);

  /* make sure the menu is on the proper screen */
  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (GTK_WIDGET (widget)));

  /* run our custom main loop */
  gtk_grab_add (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, button, time);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
  gtk_grab_remove (menu);

  /* remove the additional items (if any) */
  if (item_separator != NULL) gtk_widget_destroy (item_separator);
  if (item_open != NULL) gtk_widget_destroy (item_open);
  if (item_copy != NULL) gtk_widget_destroy (item_copy);

  /* unlink this deactivate callback */
  g_signal_handler_disconnect (G_OBJECT (menu), id);

  /* decrease the reference count on the menu */
  g_object_unref (G_OBJECT (menu));
}



static void
terminal_widget_commit (TerminalWidget *widget,
                        gchar          *data,
                        guint           length,
                        gboolean       *committed)
{
  *committed = TRUE;
}



static gboolean
terminal_widget_button_press_event (GtkWidget       *widget,
                                    GdkEventButton  *event)
{
  gboolean committed = FALSE;
  gchar   *match;
  guint    signal_id = 0;
  gint     tag;

  if (event->button == 2 && event->type == GDK_BUTTON_PRESS)
    {
      /* middle-clicking on an URI fires the responsible application */
      match = vte_terminal_match_check (VTE_TERMINAL (widget),
                                        event->x / VTE_TERMINAL (widget)->char_width,
                                        event->y / VTE_TERMINAL (widget)->char_height,
                                        &tag);
      if (G_UNLIKELY (match != NULL))
        {
          terminal_widget_open_uri (TERMINAL_WIDGET (widget), match, tag);
          g_free (match);
          return TRUE;
        }
    }
  else if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
      signal_id = g_signal_connect (G_OBJECT (widget), "commit",
                                    G_CALLBACK (terminal_widget_commit), &committed);
    }

  (*GTK_WIDGET_CLASS (terminal_widget_parent_class)->button_press_event) (widget, event);

  if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
      g_signal_handler_disconnect (G_OBJECT (widget), signal_id);

      /* no data (mouse actions) was committed to the terminal application
       * which means, we can safely popup a context menu now.
       */
      if (!committed || (event->state & GDK_MODIFIER_MASK) == GDK_SHIFT_MASK)
        {
          terminal_widget_context_menu (TERMINAL_WIDGET (widget),
                                        event->button, event->time,
                                        event->x, event->y);
        }
    }

  return TRUE;
}



static void
terminal_widget_drag_data_received (GtkWidget        *widget,
                                    GdkDragContext   *context,
                                    gint              x,
                                    gint              y,
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    guint             time)
{
  const guint16 *ucs;
  GdkColor       color;
  GString       *str;
  GValue         value = { 0, };
  gchar        **uris;
  gchar         *filename;
  gchar         *text;
  gint           argc;
  gint           n;

  switch (info)
    {
    case TARGET_STRING:
    case TARGET_UTF8_STRING:
    case TARGET_COMPOUND_TEXT:
    case TARGET_TEXT:
      text = (gchar *) gtk_selection_data_get_text (selection_data);
      if (G_LIKELY (text != NULL))
        {
          if (G_LIKELY (*text != '\0'))
            vte_terminal_feed_child (VTE_TERMINAL (widget), text, strlen (text));
          g_free (text);
        }
      break;

    case TARGET_TEXT_PLAIN:
      if (selection_data->format != 8 || selection_data->length == 0)
        {
          g_printerr (_("Unable to drop selection of type text/plain to terminal: Wrong format (%d) or length (%d)\n"),
                      selection_data->format, selection_data->length);
        }
      else
        {
          vte_terminal_feed_child (VTE_TERMINAL (widget),
                                   (const gchar *) selection_data->data,
                                   selection_data->length);
        }
      break;

    case TARGET_MOZ_URL:
      if (selection_data->format != 8
          || selection_data->length == 0
          || (selection_data->length % 2) != 0)
        {
          g_printerr (_("Unable to drop Mozilla URL on terminal: Wrong format (%d) or length (%d)\n"),
                      selection_data->format, selection_data->length);
        }
      else
        {
          str = g_string_new (NULL);
          ucs = (const guint16 *) selection_data->data;
          for (n = 0; n < selection_data->length / 2 && ucs[n] != '\n'; ++n)
            g_string_append_unichar (str, (gunichar) ucs[n]);
          filename = g_filename_from_uri (str->str, NULL, NULL);
          if (filename != NULL)
            {
              vte_terminal_feed_child (VTE_TERMINAL (widget), filename, strlen (filename));
              g_free (filename);
            }
          else
            {
              vte_terminal_feed_child (VTE_TERMINAL (widget), str->str, str->len);
            }
          g_string_free (str, TRUE);
        }
      break;

    case TARGET_URI_LIST:
      if (selection_data->format != 8 || selection_data->length == 0)
        {
          g_printerr (_("Unable to drop URI list on terminal: Wrong format (%d) or length (%d)\n"),
                      selection_data->format, selection_data->length);
        }
      else
        {
          /* split the text/uri-list */
          text = g_strndup ((const gchar *) selection_data->data, selection_data->length);
          uris = g_uri_list_extract_uris (text);
          g_free (text);

          /* translate all file:-URIs to quoted file names */
          for (n = 0; uris[n] != NULL; ++n)
            {
              /* check if we have a local file here */
              filename = g_filename_from_uri (uris[n], NULL, NULL);
              if (G_LIKELY (filename != NULL))
                {
                  /* release the file:-URI */
                  g_free (uris[n]);

                  /* check if we need to quote the file name (for the shell) */
                  if (!g_shell_parse_argv (filename, &argc, NULL, NULL) || argc != 1)
                    {
                      /* we need to quote the filename */
                      uris[n] = g_shell_quote (filename);
                      g_free (filename);
                    }
                  else
                    {
                      /* no need to quote, shell will handle properly */
                      uris[n] = filename;
                    }
                }
            }

          text = g_strjoinv (" ", uris);
          vte_terminal_feed_child (VTE_TERMINAL (widget), text, strlen (text));
          g_strfreev (uris);
          g_free (text);
        }
      break;

    case TARGET_APPLICATION_X_COLOR:
      if (selection_data->format != 16 || selection_data->length != 8)
        {
          g_printerr (_("Received invalid color data: Wrong format (%d) or length (%d)\n"),
                      selection_data->format, selection_data->length);
        }
      else
        {
          /* get the color from the selection data (ignoring the alpha setting) */
          color.red = ((guint16 *) selection_data->data)[0];
          color.green = ((guint16 *) selection_data->data)[1];
          color.blue = ((guint16 *) selection_data->data)[2];

          /* prepare the value */
          g_value_init (&value, GDK_TYPE_COLOR);
          g_value_set_boxed (&value, &color);

          /* change the background to the specified color */
          g_object_set_property (G_OBJECT (TERMINAL_WIDGET (widget)->preferences), "color-background", &value);

          /* release the value */
          g_value_unset (&value);
        }
      break;
    }
}



static gboolean
terminal_widget_key_press_event (GtkWidget    *widget,
                                 GdkEventKey  *event)
{
  GtkAdjustment *adjustment = VTE_TERMINAL (widget)->adjustment;
  gboolean       scrolling_single_line;
  gboolean       shortcuts_no_menukey;
  gdouble        value;
  gint           x;
  gint           y;

  /* determine current settings */
  g_object_get (G_OBJECT (TERMINAL_WIDGET (widget)->preferences),
                "scrolling-single-line", &scrolling_single_line,
                "shortcuts-no-menukey", &shortcuts_no_menukey,
                NULL);

  /* popup context menu if "Menu" or "<Shift>F10" is pressed */
  if (event->keyval == GDK_Menu ||
      (!shortcuts_no_menukey && (event->state & GDK_SHIFT_MASK) != 0 && event->keyval == GDK_F10))
    {
      gtk_widget_get_pointer (widget, &x, &y);
      terminal_widget_context_menu (TERMINAL_WIDGET (widget), 0, event->time, x, y);
      return TRUE;
    }
  else if (G_LIKELY (scrolling_single_line))
    {
      /* scroll up one line with "<Shift>Up" */
      if ((event->state & GDK_SHIFT_MASK) != 0 && (event->keyval == GDK_Up || event->keyval == GDK_KP_Up))
        {
          gtk_adjustment_set_value (adjustment, adjustment->value - 1);
          return TRUE;
        }
      /* scroll down one line with "<Shift>Down" */
      else if ((event->state & GDK_SHIFT_MASK) != 0 && (event->keyval == GDK_Down || event->keyval == GDK_KP_Down))
        {
          value = MIN (adjustment->value + 1, adjustment->upper - adjustment->page_size);
          gtk_adjustment_set_value (adjustment, value);
          return TRUE;
        }
    }

  return (*GTK_WIDGET_CLASS (terminal_widget_parent_class)->key_press_event) (widget, event);
}



static void
terminal_widget_open_uri (TerminalWidget *widget,
                          const gchar    *link,
                          gint            tag)
{
  GError *error = NULL;
  gchar  *uri;

  /* parse the URI from the link */
  if (tag == widget->tag_browser1)
    {
      uri = g_strdup (link);
    }
  else if (tag == widget->tag_browser2)
    {
      uri = g_strconcat ("http://", link, NULL);
    }
  else if (tag == widget->tag_mailer)
    {
      if (strncmp (link, "mailto:", 7) == 0)
        uri = g_strdup (link);
      else
        uri = g_strconcat ("mailto:", link, NULL);
    }
  else
    {
      g_warning ("Invalid tag specified while trying to open an URI.");
      return;
    }

  /* try to open the URI with the responsible application */
  if (!exo_url_show_on_screen (uri, NULL, gtk_widget_get_screen (GTK_WIDGET (widget)), &error))
    {
      /* tell the user that we were unable to open the responsible application */
      terminal_dialogs_show_error (widget, error, _("Failed to open the URL `%s'"), uri);
      g_error_free (error);
    }

  g_free (uri);
}



static void
terminal_widget_update_highlight_urls (TerminalWidget *widget)
{
  gboolean highlight_urls;

  g_object_get (G_OBJECT (widget->preferences), "misc-highlight-urls", &highlight_urls, NULL);
  if (!highlight_urls)
    {
      if (widget->tag_browser1 >= 0)
        {
          vte_terminal_match_remove (VTE_TERMINAL (widget), widget->tag_browser1);
          widget->tag_browser1 = -1;
        }

      if (widget->tag_browser2 >= 0)
        {
          vte_terminal_match_remove (VTE_TERMINAL (widget), widget->tag_browser2);
          widget->tag_browser2 = -1;
        }

      if (widget->tag_mailer >= 0)
        {
          vte_terminal_match_remove (VTE_TERMINAL (widget), widget->tag_mailer);
          widget->tag_mailer = -1;
        }
    }
  else
    {
      if (widget->tag_browser1 < 0)
        {
          widget->tag_browser1 = vte_terminal_match_add (VTE_TERMINAL (widget),
                                                         MATCH_BROWSER1);
          vte_terminal_match_set_cursor_type (VTE_TERMINAL (widget),
                                              widget->tag_browser1,
                                              GDK_HAND2);
        }

      if (widget->tag_browser2 < 0)
        {
          widget->tag_browser2 = vte_terminal_match_add (VTE_TERMINAL (widget),
                                                         MATCH_BROWSER2);
          vte_terminal_match_set_cursor_type (VTE_TERMINAL (widget),
                                              widget->tag_browser2,
                                              GDK_HAND2);
        }

      if (widget->tag_mailer < 0)
        {
          widget->tag_mailer = vte_terminal_match_add (VTE_TERMINAL (widget),
                                                       MATCH_MAILER);
          vte_terminal_match_set_cursor_type (VTE_TERMINAL (widget),
                                              widget->tag_mailer,
                                              GDK_HAND2);
        }
    }
}



/**
 * terminal_widget_new:
 *
 * Return value:
 **/
GtkWidget*
terminal_widget_new (void)
{
  return g_object_new (TERMINAL_TYPE_WIDGET, NULL);
}
