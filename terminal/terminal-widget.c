/* $Id: terminal-widget.c,v 1.10 2004/09/17 23:37:55 bmeurer Exp $ */
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
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <exo/exo.h>
#include <gdk/gdkkeysyms.h>
#include <vte/vte.h>

#include <terminal/terminal-preferences.h>
#include <terminal/terminal-widget.h>



enum
{
  CONTEXT_MENU,
  SELECTION_CHANGED,
  TITLE_CHANGED,
  LAST_SIGNAL,
};



struct _TerminalWidget
{
  GtkHBox              __parent__;
  TerminalPreferences *preferences;
  GtkWidget           *terminal;
  GtkWidget           *scrollbar;
  GPid                 pid;

  gchar              **custom_command;

  GtkWidget           *tab_box;
  GtkWidget           *tab_label;
  GtkWidget           *tab_button;

  guint                background_timer_id;
};



static void     terminal_widget_finalize                  (GObject          *object);
static void     terminal_widget_map                       (GtkWidget        *widget);
static gboolean terminal_widget_get_child_command         (TerminalWidget   *widget,
                                                           gchar           **command,
                                                           gchar          ***argv,
                                                           GError          **error);
static void terminal_widget_set_tab_title                 (TerminalWidget   *widget);
static void terminal_widget_update_background             (TerminalWidget   *widget);
static void terminal_widget_update_binding_backspace      (TerminalWidget   *widget);
static void terminal_widget_update_binding_delete         (TerminalWidget   *widget);
static void terminal_widget_update_color_foreground       (TerminalWidget   *widget);
static void terminal_widget_update_color_background       (TerminalWidget   *widget);
static void terminal_widget_update_font_name              (TerminalWidget   *widget);
static void terminal_widget_update_misc_bell_audible      (TerminalWidget   *widget);
static void terminal_widget_update_misc_bell_visible      (TerminalWidget   *widget);
static void terminal_widget_update_misc_cursor_blinks     (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_bar          (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_lines        (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_on_output    (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_on_keystroke (TerminalWidget   *widget);
static void terminal_widget_update_title_initial          (TerminalWidget   *widget);
static void terminal_widget_update_title_mode             (TerminalWidget   *widget);
static void terminal_widget_update_word_chars                 (TerminalWidget   *widget);
static void terminal_widget_vte_child_exited                  (VteTerminal      *terminal,
                                                               TerminalWidget   *widget);
static void terminal_widget_vte_drag_data_received            (VteTerminal      *terminal,
                                                               GdkDragContext   *context,
                                                               gint              x,
                                                               gint              y,
                                                               GtkSelectionData *selection_data,
                                                               guint             info,
                                                               guint             time,
                                                               TerminalWidget   *widget);
static void terminal_widget_vte_eof                           (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static gboolean terminal_widget_vte_button_press_event        (VteTerminal    *terminal,
                                                               GdkEventButton *event,
                                                               TerminalWidget *widget);
static gboolean terminal_widget_vte_key_press_event           (VteTerminal    *terminal,
                                                               GdkEventKey    *event,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_selection_changed         (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_window_title_changed      (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static gboolean terminal_widget_timer_background              (gpointer        user_data);
static void     terminal_widget_timer_background_destroy      (gpointer        user_data);



static GObjectClass *parent_class;
static guint widget_signals[LAST_SIGNAL];

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

static GtkTargetEntry target_table[] =
{
  { "text/uri-list", 0, TARGET_URI_LIST },
  { "text/x-moz-url", 0, TARGET_MOZ_URL },
  { "UTF8_STRING", 0, TARGET_UTF8_STRING },
  { "TEXT", 0, TARGET_TEXT },
  { "COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT },
  { "STRING", 0, TARGET_STRING },
  { "text/plain", 0, TARGET_TEXT_PLAIN },
};



G_DEFINE_TYPE (TerminalWidget, terminal_widget, GTK_TYPE_HBOX);



static void
terminal_widget_class_init (TerminalWidgetClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_widget_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->map = terminal_widget_map;

  /**
   * TerminalWidget::context-menu
   **/
  widget_signals[CONTEXT_MENU] =
    g_signal_new ("context-menu",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWidgetClass, context_menu),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  /**
   * TerminalWidget::selection-changed
   **/
  widget_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWidgetClass, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * TerminalWidget::window-title-changed
   **/
  widget_signals[TITLE_CHANGED] =
    g_signal_new ("title-changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalWidgetClass, title_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /* register custom icon size for the tab close buttons */
  if (gtk_icon_size_from_name ("terminal-icon-size-tab") == GTK_ICON_SIZE_INVALID)
    gtk_icon_size_register ("terminal-icon-size-tab", 14, 14);
}



static void
terminal_widget_init (TerminalWidget *widget)
{
  GtkWidget *image;

  widget->terminal = vte_terminal_new ();
  g_signal_connect (G_OBJECT (widget->terminal), "child-exited",
                    G_CALLBACK (terminal_widget_vte_child_exited), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "eof",
                    G_CALLBACK (terminal_widget_vte_eof), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "button-press-event",
                    G_CALLBACK (terminal_widget_vte_button_press_event), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "key-press-event",
                    G_CALLBACK (terminal_widget_vte_key_press_event), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "selection-changed",
                    G_CALLBACK (terminal_widget_vte_selection_changed), widget);
  g_signal_connect (G_OBJECT (widget->terminal), "window-title-changed",
                    G_CALLBACK (terminal_widget_vte_window_title_changed), widget);
  gtk_box_pack_start (GTK_BOX (widget), widget->terminal, TRUE, TRUE, 0);
  gtk_widget_show (widget->terminal);

  /* setup Drag'n'Drop support */
  g_signal_connect (G_OBJECT (widget->terminal), "drag-data-received",
                    G_CALLBACK (terminal_widget_vte_drag_data_received), widget);
  gtk_drag_dest_set (GTK_WIDGET (widget->terminal),
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     target_table, G_N_ELEMENTS (target_table),
                     GDK_ACTION_COPY);

  widget->scrollbar = gtk_vscrollbar_new (VTE_TERMINAL (widget->terminal)->adjustment);
  gtk_box_pack_start (GTK_BOX (widget), widget->scrollbar, FALSE, FALSE, 0);
  gtk_widget_show (widget->scrollbar);

  widget->preferences = terminal_preferences_get ();
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::background-mode",
                            G_CALLBACK (terminal_widget_update_background), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::background-image-file",
                            G_CALLBACK (terminal_widget_update_background), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::background-darkness",
                            G_CALLBACK (terminal_widget_update_background), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::binding-backspace",
                            G_CALLBACK (terminal_widget_update_binding_backspace), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::binding-delete",
                            G_CALLBACK (terminal_widget_update_binding_delete), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::color-foreground",
                            G_CALLBACK (terminal_widget_update_color_foreground), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::color-background",
                            G_CALLBACK (terminal_widget_update_color_background), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::font-name",
                            G_CALLBACK (terminal_widget_update_font_name), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::misc-bell-audible",
                            G_CALLBACK (terminal_widget_update_misc_bell_audible), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::misc-bell-visible",
                            G_CALLBACK (terminal_widget_update_misc_bell_visible), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::misc-cursor-blinks",
                            G_CALLBACK (terminal_widget_update_misc_cursor_blinks), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::scrolling-bar",
                            G_CALLBACK (terminal_widget_update_scrolling_bar), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::scrolling-lines",
                            G_CALLBACK (terminal_widget_update_scrolling_lines), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::scrolling-on-output",
                            G_CALLBACK (terminal_widget_update_scrolling_on_output), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::scrolling-on-keystroke",
                            G_CALLBACK (terminal_widget_update_scrolling_on_keystroke), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::title-initial",
                            G_CALLBACK (terminal_widget_update_title_initial), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::title-mode",
                            G_CALLBACK (terminal_widget_update_title_mode), widget);
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::word-chars",
                            G_CALLBACK (terminal_widget_update_word_chars), widget);

  widget->tab_box = gtk_hbox_new (FALSE, 2);
  g_object_ref (G_OBJECT (widget->tab_box));
  gtk_object_sink (GTK_OBJECT (widget->tab_box));
  gtk_widget_show (widget->tab_box);

  widget->tab_label = exo_ellipsized_label_new ("");
  exo_ellipsized_label_set_mode (EXO_ELLIPSIZED_LABEL (widget->tab_label),
                                 EXO_PANGO_ELLIPSIZE_END);
  gtk_misc_set_alignment (GTK_MISC (widget->tab_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (widget->tab_box), widget->tab_label, TRUE, TRUE, 0);
  gtk_widget_show (widget->tab_label);

  image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, gtk_icon_size_from_name ("terminal-icon-size-tab"));
  gtk_widget_show (image);

  widget->tab_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (widget->tab_button), GTK_RELIEF_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (widget->tab_button), 0);
  gtk_container_add (GTK_CONTAINER (widget->tab_button), image);
  g_signal_connect_swapped (G_OBJECT (widget->tab_button), "clicked",
                            G_CALLBACK (gtk_widget_destroy), widget);
  gtk_box_pack_start (GTK_BOX (widget->tab_box), widget->tab_button, FALSE, FALSE, 0);
  gtk_widget_show (widget->tab_button);

  /* apply current settings */
  terminal_widget_update_binding_backspace (widget);
  terminal_widget_update_binding_delete (widget);
  terminal_widget_update_font_name (widget);
  terminal_widget_update_misc_bell_audible (widget);
  terminal_widget_update_misc_bell_visible (widget);
  terminal_widget_update_misc_cursor_blinks (widget);
  terminal_widget_update_scrolling_bar (widget);
  terminal_widget_update_scrolling_lines (widget);
  terminal_widget_update_scrolling_on_output (widget);
  terminal_widget_update_scrolling_on_keystroke (widget);
  terminal_widget_update_word_chars (widget);
}



static void
terminal_widget_finalize (GObject *object)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  if (widget->background_timer_id != 0)
    g_source_remove (widget->background_timer_id);

  g_signal_handlers_disconnect_matched (G_OBJECT (widget->preferences), G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, widget);

  g_object_unref (G_OBJECT (widget->preferences));
  g_object_unref (G_OBJECT (widget->tab_box));

  parent_class->finalize (object);
}



static void
terminal_widget_map (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (parent_class)->map (widget);

  terminal_widget_timer_background (TERMINAL_WIDGET (widget));
  terminal_widget_update_color_foreground (TERMINAL_WIDGET (widget));
  terminal_widget_update_color_background (TERMINAL_WIDGET (widget));
}



static gboolean
terminal_widget_get_child_command (TerminalWidget   *widget,
                                   gchar           **command,
                                   gchar          ***argv,
                                   GError          **error)
{
  struct passwd *pw;
  const gchar   *shell_name;
  gboolean       result;
  gboolean       command_login_shell;
  gboolean       command_run_custom;
  gchar         *command_custom;

  if (widget->custom_command != NULL)
    {
      *command = g_strdup (widget->custom_command[0]);
      *argv    = g_strdupv (widget->custom_command);
    }
  else
    {
      g_object_get (G_OBJECT (widget->preferences),
                    "command-run-custom", &command_run_custom,
                    NULL);

      if (command_run_custom)
        {
          g_object_get (G_OBJECT (widget->preferences),
                        "command-custom", &command_custom,
                        NULL);
          result = g_shell_parse_argv (command_custom, NULL, argv, error);
          g_free (command_custom);
          if (result)
            *command = g_strdup ((*argv)[0]);
          return result;
        }
      else
        {
          pw = getpwuid (getuid ());
          if (G_UNLIKELY (pw == NULL))
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                           _("Current user unknown"));
              return FALSE;
            }

          g_object_get (G_OBJECT (widget->preferences),
                        "command-login-shell", &command_login_shell,
                        NULL);

          shell_name = strrchr (pw->pw_shell, '/');
          if (shell_name != NULL)
            ++shell_name;
          else
            shell_name = pw->pw_shell;
          *command = g_strdup (pw->pw_shell);

          *argv = g_new (gchar *, 2);
          if (command_login_shell)
            (*argv)[0] = g_strconcat ("-", shell_name, NULL);
          else
            (*argv)[0] = g_strdup (shell_name);
          (*argv)[1] = NULL;
        }
    }

  return TRUE;
}



static void
terminal_widget_set_tab_title (TerminalWidget *widget)
{
  gchar *title;

  title = terminal_widget_get_title (widget);
  exo_ellipsized_label_set_full_text (EXO_ELLIPSIZED_LABEL (widget->tab_label), title);
  g_free (title);
}



static void
terminal_widget_update_background (TerminalWidget *widget)
{
  if (G_UNLIKELY (widget->background_timer_id != 0))
    g_source_remove (widget->background_timer_id);

  widget->background_timer_id = g_timeout_add_full (G_PRIORITY_LOW, 250, terminal_widget_timer_background,
                                                    widget, terminal_widget_timer_background_destroy);
}



static void
terminal_widget_update_binding_backspace (TerminalWidget *widget)
{
  TerminalEraseBinding binding;

  g_object_get (G_OBJECT (widget->preferences), "binding-backspace", &binding, NULL);

  switch (binding)
    {
    case TERMINAL_ERASE_BINDING_ASCII_BACKSPACE:
      vte_terminal_set_backspace_binding (VTE_TERMINAL (widget->terminal), VTE_ERASE_ASCII_BACKSPACE);
      break;

    case TERMINAL_ERASE_BINDING_ASCII_DELETE:
      vte_terminal_set_backspace_binding (VTE_TERMINAL (widget->terminal), VTE_ERASE_ASCII_DELETE);
      break;

    case TERMINAL_ERASE_BINDING_DELETE_SEQUENCE:
      vte_terminal_set_backspace_binding (VTE_TERMINAL (widget->terminal), VTE_ERASE_DELETE_SEQUENCE);
      break;

    default:
      g_assert_not_reached ();
    }
}



static void
terminal_widget_update_binding_delete (TerminalWidget *widget)
{
  TerminalEraseBinding binding;

  g_object_get (G_OBJECT (widget->preferences), "binding-delete", &binding, NULL);

  switch (binding)
    {
    case TERMINAL_ERASE_BINDING_ASCII_BACKSPACE:
      vte_terminal_set_delete_binding (VTE_TERMINAL (widget->terminal), VTE_ERASE_ASCII_BACKSPACE);
      break;

    case TERMINAL_ERASE_BINDING_ASCII_DELETE:
      vte_terminal_set_delete_binding (VTE_TERMINAL (widget->terminal), VTE_ERASE_ASCII_DELETE);
      break;

    case TERMINAL_ERASE_BINDING_DELETE_SEQUENCE:
      vte_terminal_set_delete_binding (VTE_TERMINAL (widget->terminal), VTE_ERASE_DELETE_SEQUENCE);
      break;

    default:
      g_assert_not_reached ();
    }
}



static void
terminal_widget_update_color_foreground (TerminalWidget *widget)
{
  GdkColor *color;

  if (GTK_WIDGET_MAPPED (widget))
    {
      g_object_get (G_OBJECT (widget->preferences), "color-foreground", &color, NULL);
      vte_terminal_set_color_foreground (VTE_TERMINAL (widget->terminal), color);
      gdk_color_free (color);
    }
}



static void
terminal_widget_update_color_background (TerminalWidget *widget)
{
  GdkColor *color;

  if (GTK_WIDGET_MAPPED (widget))
    {
      g_object_get (G_OBJECT (widget->preferences), "color-background", &color, NULL);
      vte_terminal_set_color_background (VTE_TERMINAL (widget->terminal), color);
      gdk_color_free (color);
    }
}



static void
terminal_widget_update_font_name (TerminalWidget *widget)
{
  gchar *font_name;

  g_object_get (G_OBJECT (widget->preferences), "font-name", &font_name, NULL);
  if (G_LIKELY (font_name != NULL))
    {
      vte_terminal_set_font_from_string (VTE_TERMINAL (widget->terminal), font_name);
      g_free (font_name);
    }
}



static void
terminal_widget_update_misc_bell_audible (TerminalWidget *widget)
{
  gboolean bval;
  g_object_get (G_OBJECT (widget->preferences), "misc-bell-audible", &bval, NULL);
  vte_terminal_set_audible_bell (VTE_TERMINAL (widget->terminal), bval);
}



static void
terminal_widget_update_misc_bell_visible (TerminalWidget *widget)
{
  gboolean bval;
  g_object_get (G_OBJECT (widget->preferences), "misc-bell-visible", &bval, NULL);
  vte_terminal_set_visible_bell (VTE_TERMINAL (widget->terminal), bval);
}



static void
terminal_widget_update_misc_cursor_blinks (TerminalWidget *widget)
{
  gboolean bval;
  g_object_get (G_OBJECT (widget->preferences), "misc-cursor-blinks", &bval, NULL);
  vte_terminal_set_cursor_blinks (VTE_TERMINAL (widget->terminal), bval);
}



static void
terminal_widget_update_scrolling_bar (TerminalWidget *widget)
{
  TerminalScrollbar scrollbar;

  g_object_get (G_OBJECT (widget->preferences), "scrolling-bar", &scrollbar, NULL);

  switch (scrollbar)
    {
    case TERMINAL_SCROLLBAR_NONE:
      gtk_widget_hide (widget->scrollbar);
      break;

    case TERMINAL_SCROLLBAR_LEFT:
      gtk_box_reorder_child (GTK_BOX (widget), widget->scrollbar, 0);
      gtk_widget_show (widget->scrollbar);
      break;

    case TERMINAL_SCROLLBAR_RIGHT:
      gtk_box_reorder_child (GTK_BOX (widget), widget->scrollbar, 1);
      gtk_widget_show (widget->scrollbar);
      break;
    }
}



static void
terminal_widget_update_scrolling_lines (TerminalWidget *widget)
{
  guint lines;
  g_object_get (G_OBJECT (widget->preferences), "scrolling-lines", &lines, NULL);
  vte_terminal_set_scrollback_lines (VTE_TERMINAL (widget->terminal), lines);
}



static void
terminal_widget_update_scrolling_on_output (TerminalWidget *widget)
{
  gboolean scroll;
  g_object_get (G_OBJECT (widget->preferences), "scrolling-on-output", &scroll, NULL);
  vte_terminal_set_scroll_on_output (VTE_TERMINAL (widget->terminal), scroll);
}



static void
terminal_widget_update_scrolling_on_keystroke (TerminalWidget *widget)
{
  gboolean scroll;
  g_object_get (G_OBJECT (widget->preferences), "scrolling-on-keystroke", &scroll, NULL);
  vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (widget->terminal), scroll);
}



static void
terminal_widget_update_title_initial (TerminalWidget *widget)
{
  terminal_widget_set_tab_title (widget);
  g_signal_emit (G_OBJECT (widget), widget_signals[TITLE_CHANGED], 0);
}



static void
terminal_widget_update_title_mode (TerminalWidget *widget)
{
  terminal_widget_set_tab_title (widget);
  g_signal_emit (G_OBJECT (widget), widget_signals[TITLE_CHANGED], 0);
}



static void
terminal_widget_update_word_chars (TerminalWidget *widget)
{
  gchar *word_chars;

  g_object_get (G_OBJECT (widget->preferences), "word-chars", &word_chars, NULL);
  if (G_LIKELY (word_chars != NULL))
    {
      vte_terminal_set_word_chars (VTE_TERMINAL (widget->terminal), word_chars);
      g_free (word_chars);
    }
}



static void
terminal_widget_vte_child_exited (VteTerminal    *terminal,
                                  TerminalWidget *widget)
{
  g_return_if_fail (VTE_IS_TERMINAL (terminal));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_destroy (GTK_WIDGET (widget));
}



static void
terminal_widget_vte_drag_data_received (VteTerminal      *terminal,
                                        GdkDragContext   *context,
                                        gint              x,
                                        gint              y,
                                        GtkSelectionData *selection_data,
                                        guint             info,
                                        guint             time,
                                        TerminalWidget   *widget)
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
            vte_terminal_feed_child (VTE_TERMINAL (terminal), text, strlen (text));
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
          vte_terminal_feed_child (VTE_TERMINAL (terminal),
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
              vte_terminal_feed_child (VTE_TERMINAL (widget->terminal), filename, strlen (filename));
              g_free (filename);
            }
          else
            {
              vte_terminal_feed_child (VTE_TERMINAL (widget->terminal), str->str, str->len);
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
              vte_terminal_feed_child (VTE_TERMINAL (widget->terminal), text, strlen (text));
              g_strfreev (uris);
            }
        }
      break;
    }
}



static void
terminal_widget_vte_eof (VteTerminal    *terminal,
                         TerminalWidget *widget)
{
  g_return_if_fail (VTE_IS_TERMINAL (terminal));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  gtk_widget_destroy (GTK_WIDGET (widget));
}



static gboolean
terminal_widget_vte_button_press_event (VteTerminal    *terminal,
                                        GdkEventButton *event,
                                        TerminalWidget *widget)
{
  if (event->button == 3)
    {
      g_signal_emit (G_OBJECT (widget), widget_signals[CONTEXT_MENU], 0, event);
      return TRUE;
    }

  return FALSE;
}



static gboolean
terminal_widget_vte_key_press_event (VteTerminal    *terminal,
                                     GdkEventKey    *event,
                                     TerminalWidget *widget)
{
  if (event->state == 0 && event->keyval == GDK_Menu)
    {
      g_signal_emit (G_OBJECT (widget), widget_signals[CONTEXT_MENU], 0, event);
      return TRUE;
    }

  return FALSE;
}



static void
terminal_widget_vte_selection_changed (VteTerminal    *terminal,
                                       TerminalWidget *widget)
{
  g_return_if_fail (VTE_IS_TERMINAL (terminal));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  g_signal_emit (G_OBJECT (widget), widget_signals[SELECTION_CHANGED], 0);
}



static void
terminal_widget_vte_window_title_changed (VteTerminal    *terminal,
                                          TerminalWidget *widget)
{
  g_return_if_fail (VTE_IS_TERMINAL (terminal));
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  terminal_widget_set_tab_title (widget);
  g_signal_emit (G_OBJECT (widget), widget_signals[TITLE_CHANGED], 0);
}



static gboolean
terminal_widget_timer_background (gpointer user_data)
{
  TerminalBackground background_mode;
  TerminalWidget    *widget = TERMINAL_WIDGET (user_data);
  gdouble            background_darkness;
  gchar             *background_file;

  if (GTK_WIDGET_MAPPED (widget))
    {
      g_object_get (G_OBJECT (widget->preferences), "background-mode", &background_mode, NULL);

      if (background_mode == TERMINAL_BACKGROUND_IMAGE)
        {
          g_object_get (G_OBJECT (widget->preferences), "background-image-file", &background_file, NULL);
          if (G_LIKELY (background_file != NULL))
            {
              vte_terminal_set_background_image_file (VTE_TERMINAL (widget->terminal), background_file);
              g_free (background_file);
            }
        }
      else
        {
          vte_terminal_set_background_image_file (VTE_TERMINAL (widget->terminal), NULL);
        }

      if (background_mode == TERMINAL_BACKGROUND_SOLID)
        {
          vte_terminal_set_background_saturation (VTE_TERMINAL (widget->terminal), 0.0);
        }
      else
        {
          g_object_get (G_OBJECT (widget->preferences), "background-darkness", &background_darkness, NULL);
          vte_terminal_set_background_saturation (VTE_TERMINAL (widget->terminal), 1.0 - background_darkness);
        }

      vte_terminal_set_background_transparent (VTE_TERMINAL (widget->terminal),
                                               background_mode == TERMINAL_BACKGROUND_TRANSPARENT);
    }

  return FALSE;
}



static void
terminal_widget_timer_background_destroy (gpointer user_data)
{
  TERMINAL_WIDGET (user_data)->background_timer_id = 0;
}



/**
 * terminal_widget_new:
 *
 * Return value :
 **/
GtkWidget*
terminal_widget_new (void)
{
  return g_object_new (TERMINAL_TYPE_WIDGET, NULL);
}



/**
 * terminal_widget_launch_child:
 * @widget  : A #TerminalWidget.
 *
 * Starts the terminal child process.
 **/
void
terminal_widget_launch_child (TerminalWidget *widget)
{
  gboolean update;
  GError  *error = NULL;
  gchar   *command;
  gchar  **argv;
  GPid     pid;

  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  if (!terminal_widget_get_child_command (widget, &command, &argv, &error))
    {
      g_error_free (error);
      return;
    }

  g_object_get (G_OBJECT (widget->preferences),
                "command-update-records", &update,
                NULL);

  pid = vte_terminal_fork_command (VTE_TERMINAL (widget->terminal),
                                   command, argv, NULL, NULL,
                                   update, update, update);

  g_strfreev (argv);
  g_free (command);
}



/**
 * terminal_widget_set_custom_command:
 * @widget  : A #TerminalWidget.
 * @command :
 **/
void
terminal_widget_set_custom_command (TerminalWidget *widget,
                                    gchar         **command)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  if (G_UNLIKELY (widget->custom_command != NULL))
    g_strfreev (widget->custom_command);

  if (G_LIKELY (command != NULL && *command != NULL))
    widget->custom_command = g_strdupv (command);
  else
    widget->custom_command = NULL;
}



/**
 **/
void
terminal_widget_get_size (TerminalWidget *widget,
                          gint           *width_chars,
                          gint           *height_chars)
{
  *width_chars = VTE_TERMINAL (widget->terminal)->column_count;
  *height_chars = VTE_TERMINAL (widget->terminal)->row_count;
}



/**
 **/
void
terminal_widget_set_size (TerminalWidget *widget,
                          gint            width_chars,
                          gint            height_chars)
{
  vte_terminal_set_size (VTE_TERMINAL (widget->terminal), width_chars, height_chars);
}



/**
 * terminal_widget_force_resize_window:
 *
 * I don't like this way, but its required to work-around a Gtk+
 * bug (maybe also caused by a Vte bug, not sure).
 **/
void
terminal_widget_force_resize_window (TerminalWidget *widget,
                                     GtkWindow      *window,
                                     gint            force_columns,
                                     gint            force_rows)
{
  GtkRequisition terminal_requisition;
  GtkRequisition window_requisition;
  gint           width;
  gint           height;
  gint           columns;
  gint           rows;
  gint           xpad;
  gint           ypad;

  gtk_widget_set_size_request (widget->terminal, 2000, 2000);
  gtk_widget_size_request (GTK_WIDGET (window), &window_requisition);
  gtk_widget_size_request (widget->terminal, &terminal_requisition);

  width = window_requisition.width - terminal_requisition.width;
  height = window_requisition.height - terminal_requisition.height;

  if (force_columns < 0)
    columns = VTE_TERMINAL (widget->terminal)->column_count;
  else
    columns = force_columns;

  if (force_rows < 0)
    rows = VTE_TERMINAL (widget->terminal)->row_count;
  else
    rows = force_rows;

  vte_terminal_get_padding (VTE_TERMINAL (widget->terminal), &xpad, &ypad);

  width += xpad + VTE_TERMINAL (widget->terminal)->char_width * columns;
  height += ypad + VTE_TERMINAL (widget->terminal)->char_height * rows;

  if (GTK_WIDGET_MAPPED (window))
    gtk_window_resize (window, width, height);
  else
    gtk_window_set_default_size (window, width, height);
}



/**
 * terminal_widget_set_window_geometry_hints:
 *
 * I don't like this way, but its required to work-around a Gtk+
 * bug (maybe also caused by a Vte bug, not sure).
 **/
void
terminal_widget_set_window_geometry_hints (TerminalWidget *widget,
                                           GtkWindow      *window)
{
  GdkGeometry hints;
  gint        xpad;
  gint        ypad;

  vte_terminal_get_padding (VTE_TERMINAL (widget->terminal), &xpad, &ypad);

  hints.base_width = xpad;
  hints.base_height = ypad;
  hints.width_inc = VTE_TERMINAL (widget->terminal)->char_width;
  hints.height_inc = VTE_TERMINAL (widget->terminal)->char_height;
  hints.min_width = hints.base_width + hints.width_inc * 4;
  hints.min_height = hints.base_height + hints.height_inc * 2;

  gtk_window_set_geometry_hints (GTK_WINDOW (window),
                                 widget->terminal,
                                 &hints,
                                 GDK_HINT_RESIZE_INC
                                 | GDK_HINT_MIN_SIZE
                                 | GDK_HINT_BASE_SIZE);
}



/**
 * terminal_widget_get_tab_box:
 * @widget      : A #TerminalWidget.
 *
 * FIXME: get rid of this, and replace it with a property "active-title", which
 * gets connected to the "full-text" property of the label!
 *
 * Return value :
 **/
GtkWidget*
terminal_widget_get_tab_box (TerminalWidget *widget)
{
  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), NULL);
  terminal_widget_set_tab_title (widget);
  return widget->tab_box;
}



/**
 * terminal_widget_get_title:
 * @widget      : A #TerminalWidget.
 *
 * Return value :
 **/
gchar*
terminal_widget_get_title (TerminalWidget *widget)
{
  TerminalTitle mode;
  const gchar  *window_title;
  gchar        *initial;
  gchar        *title;

  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), NULL);

  g_object_get (G_OBJECT (widget->preferences),
                "title-initial", &initial,
                "title-mode", &mode,
                NULL);

  window_title = vte_terminal_get_window_title (VTE_TERMINAL (widget->terminal));

  switch (mode)
    {
    case TERMINAL_TITLE_REPLACE:
      if (window_title != NULL)
        title = g_strdup (window_title);
      else
        title = g_strdup (_("Untitled"));
      break;

    case TERMINAL_TITLE_PREPEND:
      if (window_title != NULL)
        title = g_strdup_printf ("%s - %s", window_title, initial);
      else
        title = g_strdup (initial);
      break;

    case TERMINAL_TITLE_APPEND:
      if (window_title != NULL)
        title = g_strdup_printf ("%s - %s", initial, window_title);
      else
        title = g_strdup (initial);
      break;

    case TERMINAL_TITLE_HIDE:
      title = g_strdup (initial);
      break;

    default:
      g_assert_not_reached ();
      title = NULL;
    }

  g_free (initial);

  return title;
}



/**
 * terminal_widget_has_selection:
 * @widget      : A #TerminalWidget.
 *
 * Checks if the terminal currently contains selected text. Note that this is different from
 * determining if the terminal is the owner of any GtkClipboard items.
 *
 * Return value : %TRUE if part of the text in the terminal is selected.
 **/
gboolean
terminal_widget_has_selection (TerminalWidget *widget)
{
  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), FALSE);
  return vte_terminal_get_has_selection (VTE_TERMINAL (widget->terminal));
}



/**
 * terminal_widget_copy_clipboard:
 * @widget  : A #TerminalWidget.
 *
 * Places the selected text in the terminal in the #GDK_SELECTIN_CLIPBOARD selection.
 **/
void
terminal_widget_copy_clipboard (TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  vte_terminal_copy_clipboard (VTE_TERMINAL (widget->terminal));
}



/**
 * terminal_widget_paste_clipboard:
 * @widget  : A #TerminalWidget.
 *
 * Sends the contents of the #GDK_SELECTION_CLIPBOARD selection to the terminal's
 * child. If neccessary, the data is converted from UTF-8 to the terminal's current
 * encoding.
 **/
void
terminal_widget_paste_clipboard (TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  vte_terminal_paste_clipboard (VTE_TERMINAL (widget->terminal));
}



/**
 * terminal_widget_reset:
 * @widget  : A #TerminalWidget.
 * @clear   : %TRUE to also clear the terminal screen.
 *
 * Resets the terminal.
 **/
void
terminal_widget_reset (TerminalWidget *widget,
                       gboolean        clear)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  vte_terminal_reset (VTE_TERMINAL (widget->terminal), TRUE, clear);
}



/**
 * terminal_widget_im_append_menuitems:
 * @widget    : A #TerminalWidget.
 * @menushell : A #GtkMenuShell.
 *
 * Appends menu items for various input methods to the given @menushell.
 * The user can select one of these items to modify the input method
 * used by the terminal.
 **/
void
terminal_widget_im_append_menuitems (TerminalWidget *widget,
                                     GtkMenuShell   *menushell)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  g_return_if_fail (GTK_IS_MENU_SHELL (menushell));

  vte_terminal_im_append_menuitems (VTE_TERMINAL (widget->terminal), menushell);
}
