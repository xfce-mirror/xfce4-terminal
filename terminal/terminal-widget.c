/* $Id$ */
/*-
 * Copyright (c) 2004 os-cillation e.K.
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

#include <libxfce4util/libxfce4util.h>

#include <terminal/terminal-preferences.h>
#include <terminal/terminal-widget.h>



enum
{
  CONTEXT_MENU,
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
};



static void     terminal_widget_class_init          (TerminalWidgetClass  *klass);
static void     terminal_widget_init                (TerminalWidget       *widget);
static void     terminal_widget_finalize            (GObject              *object);
static gboolean terminal_widget_button_press_event  (GtkWidget            *widget,
                                                     GdkEventButton       *event);
static void     terminal_widget_drag_data_received  (GtkWidget            *widget,
                                                     GdkDragContext       *context,
                                                     gint                  x,
                                                     gint                  y,
                                                     GtkSelectionData     *selection_data,
                                                     guint                 info,
                                                     guint                 time);
static gboolean terminal_widget_key_press_event     (GtkWidget            *widget,
                                                     GdkEventKey          *event);



struct _TerminalWidgetClass
{
  VteTerminalClass __parent__;

  /* signals */
  void  (*context_menu)   (TerminalWidget *widget,
                           GdkEvent       *event);
};

struct _TerminalWidget
{
  VteTerminal __parent__;

  /*< private >*/
  TerminalPreferences *preferences;
};



static GObjectClass *parent_class;
static guint         widget_signals[LAST_SIGNAL];

static GtkTargetEntry targets[] =
{
  { "text/uri-list", 0, TARGET_URI_LIST },
  { "text/x-moz-url", 0, TARGET_MOZ_URL },
  { "UTF8_STRING", 0, TARGET_UTF8_STRING },
  { "TEXT", 0, TARGET_TEXT },
  { "COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT },
  { "STRING", 0, TARGET_STRING },
  { "text/plain", 0, TARGET_TEXT_PLAIN },
};



G_DEFINE_TYPE (TerminalWidget, terminal_widget, VTE_TYPE_TERMINAL);



static void
terminal_widget_class_init (TerminalWidgetClass *klass)
{
  GtkWidgetClass  *gtkwidget_class;
  GObjectClass    *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_widget_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event = terminal_widget_button_press_event;
  gtkwidget_class->drag_data_received = terminal_widget_drag_data_received;
  gtkwidget_class->key_press_event    = terminal_widget_key_press_event;

  /**
   * TerminalWidget::context-menu:
   **/
  widget_signals[CONTEXT_MENU] =
    g_signal_new ("context-menu",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWidgetClass, context_menu),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);
}



static void
terminal_widget_init (TerminalWidget *widget)
{
  /* query preferences connection */
  widget->preferences = terminal_preferences_get ();

  /* setup Drag'n'Drop support */
  gtk_drag_dest_set (GTK_WIDGET (widget),
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     targets, G_N_ELEMENTS (targets),
                     GDK_ACTION_COPY);

}



static void
terminal_widget_finalize (GObject *object)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  g_object_unref (G_OBJECT (widget->preferences));

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
  guint    signal_id = 0;

  if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
      signal_id = g_signal_connect (G_OBJECT (widget), "commit",
                                    G_CALLBACK (terminal_widget_commit), &committed);
    }

  GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);

  if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
      g_signal_handler_disconnect (G_OBJECT (widget), signal_id);

      /* no data (mouse actions) was committed to the terminal application
       * which means, we can safely popup a context menu now.
       */
      if (!committed || (event->state & GDK_MODIFIER_MASK) == GDK_SHIFT_MASK)
        g_signal_emit (G_OBJECT (widget), widget_signals[CONTEXT_MENU], 0, event);
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
  GString       *str;
  gchar        **uris;
  gchar         *filename;
  gchar         *text;
  gint           n;

  switch (info)
    {
    case TARGET_STRING:
    case TARGET_UTF8_STRING:
    case TARGET_COMPOUND_TEXT:
    case TARGET_TEXT:
      text = gtk_selection_data_get_text (selection_data);
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
                                   selection_data->data,
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
          text = g_strndup (selection_data->data, selection_data->length);
          uris = g_strsplit (text, "\r\n", 0);
          g_free (text);

          for (n = 0; uris != NULL && uris[n] != NULL; ++n)
            {
              filename = g_filename_from_uri (uris[n], NULL, NULL);
              if (G_LIKELY (filename != NULL))
                {
                  g_free (uris[n]);
                  uris[n] = filename;
                }
            }

          if (uris != NULL)
            {
              text = g_strjoinv (" ", uris);
              vte_terminal_feed_child (VTE_TERMINAL (widget), text, strlen (text));
              g_strfreev (uris);
            }
        }
      break;
    }
}



static gboolean
terminal_widget_key_press_event (GtkWidget    *widget,
                                 GdkEventKey  *event)
{
  /* popup context menu if "Menu" or "<Shift>F10" is pressed */
  if ((event->state == 0 && event->keyval == GDK_Menu)
   || (event->state == GDK_SHIFT_MASK && event->keyval == GDK_F10))
    {
      g_signal_emit (G_OBJECT (widget), widget_signals[CONTEXT_MENU], 0, event);
      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
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
