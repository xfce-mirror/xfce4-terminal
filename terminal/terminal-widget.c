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
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <exo/exo.h>
#include <gdk/gdkkeysyms.h>
#include <vte/vte.h>

#include <terminal/terminal-image-loader.h>
#include <terminal/terminal-widget.h>



enum
{
  PROP_0,
  PROP_CUSTOM_TITLE,
  PROP_ENCODING,
  PROP_TITLE,
};

enum
{
  CONTEXT_MENU,
  SELECTION_CHANGED,
  LAST_SIGNAL,
};



struct _TerminalWidget
{
  GtkHBox              __parent__;
  TerminalPreferences *preferences;
  GtkWidget           *terminal;
  GtkWidget           *scrollbar;

  GPid                 pid;
  gchar               *working_directory;

  gchar              **custom_command;
  gchar               *custom_title;

  guint                background_timer_id;
};



static void     terminal_widget_finalize                      (GObject          *object);
static void     terminal_widget_get_property                  (GObject          *object,
                                                               guint             prop_id,
                                                               GValue           *value,
                                                               GParamSpec       *pspec);
static void     terminal_widget_set_property                  (GObject          *object,
                                                               guint             prop_id,
                                                               const GValue     *value,
                                                               GParamSpec       *pspec);
static gboolean terminal_widget_get_child_command             (TerminalWidget   *widget,
                                                               gchar           **command,
                                                               gchar          ***argv,
                                                               GError          **error);
static gchar  **terminal_widget_get_child_environment         (TerminalWidget   *widget);
static void terminal_widget_update_background                 (TerminalWidget   *widget);
static void terminal_widget_update_binding_backspace          (TerminalWidget   *widget);
static void terminal_widget_update_binding_delete             (TerminalWidget   *widget);
static void terminal_widget_update_colors                     (TerminalWidget   *widget);
static void terminal_widget_update_font                       (TerminalWidget   *widget);
static void terminal_widget_update_misc_bell                  (TerminalWidget   *widget);
static void terminal_widget_update_misc_cursor_blinks         (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_bar              (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_lines            (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_on_output        (TerminalWidget   *widget);
static void terminal_widget_update_scrolling_on_keystroke     (TerminalWidget   *widget);
static void terminal_widget_update_title                      (TerminalWidget   *widget);
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
static void     terminal_widget_vte_encoding_changed          (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_eof                       (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static gboolean terminal_widget_vte_button_press_event        (VteTerminal    *terminal,
                                                               GdkEventButton *event,
                                                               TerminalWidget *widget);
static gboolean terminal_widget_vte_key_press_event           (VteTerminal    *terminal,
                                                               GdkEventKey    *event,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_realize                   (VteTerminal    *terminal,
                                                               TerminalWidget *widget);
static void     terminal_widget_vte_map                       (VteTerminal    *terminal,
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
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_widget_finalize;
  gobject_class->get_property = terminal_widget_get_property;
  gobject_class->set_property = terminal_widget_set_property;

  /**
   * TerminalWidget:custom-title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_TITLE,
                                   g_param_spec_string ("custom-title",
                                                        _("Custom title"),
                                                        _("Custom title"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalWidget:encoding:
   *
   * The encoding the terminal will expect data from the child to be encoded
   * with. For certain terminal types, applications executing in the terminal
   * can change the encoding. The default encoding is defined by the application's
   * locale settings.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ENCODING,
                                   g_param_spec_string ("encoding",
                                                        _("Encoding"),
                                                        _("Terminal encoding"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalWidget:title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        _("Title"),
                                                        _("Title"),
                                                        NULL,
                                                        G_PARAM_READABLE));

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
}



static void
terminal_widget_init (TerminalWidget *widget)
{
  widget->working_directory = g_get_current_dir ();
  widget->custom_title = g_strdup ("");

  widget->terminal = vte_terminal_new ();
  g_object_connect (G_OBJECT (widget->terminal),
                    "signal::child-exited", G_CALLBACK (terminal_widget_vte_child_exited), widget,
                    "signal::encoding-changed", G_CALLBACK (terminal_widget_vte_encoding_changed), widget,
                    "signal::eof", G_CALLBACK (terminal_widget_vte_eof), widget,
                    "signal::button-press-event", G_CALLBACK (terminal_widget_vte_button_press_event), widget,
                    "signal::key-press-event", G_CALLBACK (terminal_widget_vte_key_press_event), widget,
                    "signal::selection-changed", G_CALLBACK (terminal_widget_vte_selection_changed), widget,
                    "signal::window-title-changed", G_CALLBACK (terminal_widget_vte_window_title_changed), widget,
                    "signal-after::realize", G_CALLBACK (terminal_widget_vte_realize), widget,
                    "signal-after::map", G_CALLBACK (terminal_widget_vte_map), widget,
                    "swapped-signal::size-allocate", G_CALLBACK (terminal_widget_timer_background), widget,
                    "swapped-signal::style-set", G_CALLBACK (terminal_widget_update_colors), widget,
                    NULL);
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
  g_object_connect (G_OBJECT (widget->preferences),
                    "swapped-signal::notify::background-mode", G_CALLBACK (terminal_widget_update_background), widget,
                    "swapped-signal::notify::background-image-file", G_CALLBACK (terminal_widget_update_background), widget,
                    "swapped-signal::notify::background-image-style", G_CALLBACK (terminal_widget_update_background), widget,
                    "swapped-signal::notify::background-darkness", G_CALLBACK (terminal_widget_update_background), widget,
                    "swapped-signal::notify::binding-backspace", G_CALLBACK (terminal_widget_update_binding_backspace), widget,
                    "swapped-signal::notify::binding-delete", G_CALLBACK (terminal_widget_update_binding_delete), widget,
                    "swapped-signal::notify::color-system-theme", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-foreground", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-background", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette1", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette2", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette3", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette4", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette5", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette6", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette7", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette8", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette9", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette10", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette11", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette12", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette13", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette14", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette15", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::color-palette16", G_CALLBACK (terminal_widget_update_colors), widget,
                    "swapped-signal::notify::font-anti-alias", G_CALLBACK (terminal_widget_update_font), widget,
                    "swapped-signal::notify::font-name", G_CALLBACK (terminal_widget_update_font), widget,
                    "swapped-signal::notify::misc-bell", G_CALLBACK (terminal_widget_update_misc_bell), widget,
                    "swapped-signal::notify::misc-cursor-blinks", G_CALLBACK (terminal_widget_update_misc_cursor_blinks), widget,
                    "swapped-signal::notify::scrolling-bar", G_CALLBACK (terminal_widget_update_scrolling_bar), widget,
                    "swapped-signal::notify::scrolling-lines", G_CALLBACK (terminal_widget_update_scrolling_lines), widget,
                    "swapped-signal::notify::scrolling-on-output", G_CALLBACK (terminal_widget_update_scrolling_on_output), widget,
                    "swapped-signal::notify::scrolling-on-keystroke", G_CALLBACK (terminal_widget_update_scrolling_on_keystroke), widget,
                    "swapped-signal::notify::title-initial", G_CALLBACK (terminal_widget_update_title), widget,
                    "swapped-signal::notify::title-mode", G_CALLBACK (terminal_widget_update_title), widget,
                    "swapped-signal::notify::word-chars", G_CALLBACK (terminal_widget_update_word_chars), widget,
                    NULL);

  /* apply current settings */
  terminal_widget_update_binding_backspace (widget);
  terminal_widget_update_binding_delete (widget);
  terminal_widget_update_font (widget);
  terminal_widget_update_misc_bell (widget);
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

  if (G_UNLIKELY (widget->background_timer_id != 0))
    g_source_remove (widget->background_timer_id);

  g_signal_handlers_disconnect_matched (G_OBJECT (widget->preferences),
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, widget);

  g_object_unref (G_OBJECT (widget->preferences));

  g_free (widget->working_directory);
  g_strfreev (widget->custom_command);
  g_free (widget->custom_title);

  parent_class->finalize (object);
}



static void
terminal_widget_get_property (GObject          *object,
                              guint             prop_id,
                              GValue           *value,
                              GParamSpec       *pspec)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_CUSTOM_TITLE:
      g_value_set_string (value, widget->custom_title);
      break;

    case PROP_ENCODING:
      g_value_set_string (value, vte_terminal_get_encoding (VTE_TERMINAL (widget->terminal)));
      break;

    case PROP_TITLE:
      g_value_take_string (value, terminal_widget_get_title (widget));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
terminal_widget_set_property (GObject          *object,
                              guint             prop_id,
                              const GValue     *value,
                              GParamSpec       *pspec)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_CUSTOM_TITLE:
      terminal_widget_set_custom_title (widget, g_value_get_string (value));
      break;

    case PROP_ENCODING:
      vte_terminal_set_encoding (VTE_TERMINAL (widget->terminal), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
terminal_widget_get_child_command (TerminalWidget   *widget,
                                   gchar           **command,
                                   gchar          ***argv,
                                   GError          **error)
{
  struct passwd *pw;
  const gchar   *shell_name;
  gboolean       command_login_shell;

  if (widget->custom_command != NULL)
    {
      *command = g_strdup (widget->custom_command[0]);
      *argv    = g_strdupv (widget->custom_command);
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

  return TRUE;
}



static gchar**
terminal_widget_get_child_environment (TerminalWidget *widget)
{
  extern gchar    **environ;
  gchar           **result;
  gchar           **p;
  gchar            *term;
  guint             n;

  /* count env vars that are set */
  for (p = environ; *p != NULL; ++p);

  n = p - environ;
  result = g_new (gchar *, n + 1 + 4);

  for (n = 0, p = environ; *p != NULL; ++p)
    {
      if ((strncmp (*p, "COLUMNS=", 8) == 0)
          || (strncmp (*p, "LINES=", 6) == 0)
          || (strncmp (*p, "WINDOWID=", 9) == 0)
          || (strncmp (*p, "TERM=", 5) == 0)
          || (strncmp (*p, "GNOME_DESKTOP_ICON=", 19) == 0)
          || (strncmp (*p, "COLORTERM=", 10) == 0)
          || (strncmp ( *p, "DISPLAY=", 8) == 0))
        {
          /* nothing: do not copy */
        }
      else
        {
          result[n] = g_strdup (*p);
          ++n;
        }
    }

  result[n++] = g_strdup ("COLORTERM=Terminal");

  g_object_get (G_OBJECT (widget->preferences), "term", &term, NULL);
  if (G_LIKELY (term != NULL))
    {
      result[n++] = g_strdup_printf ("TERM=%s", term);
      g_free (term);
    }

  if (GTK_WIDGET_REALIZED (widget->terminal))
    {
      result[n++] = g_strdup_printf ("WINDOWID=%ld", (glong) GDK_WINDOW_XWINDOW (widget->terminal->window));
      result[n++] = g_strdup_printf ("DISPLAY=%s", gdk_display_get_name (gtk_widget_get_display (widget->terminal)));
    }

  result[n] = NULL;

  return result;
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
query_color (TerminalPreferences *preferences,
             const gchar         *property,
             GdkColor            *color_return)
{
  GdkColor *color;

  g_object_get (G_OBJECT (preferences), property, &color, NULL);
  *color_return = *color;
  gdk_color_free (color);
}



static void
terminal_widget_update_colors (TerminalWidget *widget)
{
  gboolean system_theme;
  GdkColor palette[16];
  GdkColor bg;
  GdkColor fg;
  gchar    name[32];
  guint    n;

  g_object_get (G_OBJECT (widget->preferences),
                "color-system-theme", &system_theme,
                NULL);

  if (system_theme)
    {
      bg = widget->terminal->style->base[GTK_STATE_NORMAL];
      fg = widget->terminal->style->text[GTK_STATE_NORMAL];
    }
  else
    {
      query_color (widget->preferences, "color-background", &bg);
      query_color (widget->preferences, "color-foreground", &fg);
    }

  for (n = 0; n < 16; ++n)
    {
      g_snprintf (name, 32, "color-palette%u", n + 1);
      query_color (widget->preferences, name, palette + n);
    }

  vte_terminal_set_colors (VTE_TERMINAL (widget->terminal), &fg, &bg, palette, 16);
  vte_terminal_set_background_tint_color (VTE_TERMINAL (widget->terminal), &bg);
}



static void
terminal_widget_update_font (TerminalWidget *widget)
{
  VteTerminalAntiAlias antialias;
  gboolean             font_anti_alias;
  gchar               *font_name;

  g_object_get (G_OBJECT (widget->preferences),
                "font-anti-alias", &font_anti_alias,
                "font-name", &font_name,
                NULL);

  if (G_LIKELY (font_name != NULL))
    {
      antialias = font_anti_alias
                ? VTE_ANTI_ALIAS_USE_DEFAULT
                : VTE_ANTI_ALIAS_FORCE_DISABLE;

      vte_terminal_set_font_from_string_full (VTE_TERMINAL (widget->terminal),
                                              font_name, antialias);
      g_free (font_name);
    }
}



static void
terminal_widget_update_misc_bell (TerminalWidget *widget)
{
  gboolean bval;
  g_object_get (G_OBJECT (widget->preferences), "misc-bell", &bval, NULL);
  vte_terminal_set_audible_bell (VTE_TERMINAL (widget->terminal), bval);
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
terminal_widget_update_title (TerminalWidget *widget)
{
  g_object_notify (G_OBJECT (widget), "title");
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
terminal_widget_vte_encoding_changed (VteTerminal     *terminal,
                                      TerminalWidget  *widget)
{
  g_object_notify (G_OBJECT (widget), "encoding");
}



static void
terminal_widget_vte_eof (VteTerminal    *terminal,
                         TerminalWidget *widget)
{
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
terminal_widget_vte_realize (VteTerminal    *terminal,
                             TerminalWidget *widget)
{
  vte_terminal_set_allow_bold (terminal, TRUE);
  terminal_widget_update_colors (TERMINAL_WIDGET (widget));
}



static void
terminal_widget_vte_map (VteTerminal    *terminal,
                         TerminalWidget *widget)
{
  terminal_widget_timer_background (TERMINAL_WIDGET (widget));
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

  g_object_notify (G_OBJECT (widget), "title");
}



static gboolean
terminal_widget_timer_background (gpointer user_data)
{
  TerminalImageLoader *loader;
  TerminalBackground   background_mode;
  TerminalWidget      *widget = TERMINAL_WIDGET (user_data);
  GdkPixbuf           *image;
  gdouble              background_darkness;

  if (GTK_WIDGET_MAPPED (widget))
    {
      g_object_get (G_OBJECT (widget->preferences), "background-mode", &background_mode, NULL);

      if (background_mode == TERMINAL_BACKGROUND_SOLID)
        {
          vte_terminal_set_background_image (VTE_TERMINAL (widget->terminal), NULL);
          vte_terminal_set_background_saturation (VTE_TERMINAL (widget->terminal), 1.0);
          vte_terminal_set_background_transparent (VTE_TERMINAL (widget->terminal), FALSE);
        }
      else if (background_mode == TERMINAL_BACKGROUND_IMAGE)
        {
          vte_terminal_set_background_saturation (VTE_TERMINAL (widget->terminal), 1.0);
          vte_terminal_set_background_transparent (VTE_TERMINAL (widget->terminal), FALSE);

          loader = terminal_image_loader_get ();
          image = terminal_image_loader_load (loader,
                                              widget->terminal->allocation.width,
                                              widget->terminal->allocation.height);
          vte_terminal_set_background_image (VTE_TERMINAL (widget->terminal), image);
          if (GDK_IS_PIXBUF (image))
            g_object_unref (G_OBJECT (image));
          g_object_unref (G_OBJECT (loader));
        }
      else
        {
          g_object_get (G_OBJECT (widget->preferences), "background-darkness", &background_darkness, NULL);
          vte_terminal_set_background_image (VTE_TERMINAL (widget->terminal), NULL);
          vte_terminal_set_background_saturation (VTE_TERMINAL (widget->terminal), 1.0 - background_darkness);
          vte_terminal_set_background_transparent (VTE_TERMINAL (widget->terminal), TRUE);
        }
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
  gchar  **env;

  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

#ifdef DEBUG
  if (!GTK_WIDGET_REALIZED (widget))
    g_error ("Tried to launch command in a TerminalWidget that is not realized");
#endif

  if (!terminal_widget_get_child_command (widget, &command, &argv, &error))
    {
      g_error_free (error);
      return;
    }

  g_object_get (G_OBJECT (widget->preferences),
                "command-update-records", &update,
                NULL);

  env = terminal_widget_get_child_environment (widget);

  widget->pid = vte_terminal_fork_command (VTE_TERMINAL (widget->terminal),
                                           command, argv, env,
                                           widget->working_directory,
                                           update, update, update);

  g_strfreev (argv);
  g_strfreev (env);
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
 * terminal_widget_set_custom_title:
 * @widget  : A #TerminalWidget.
 * @title   : Title string.
 **/
void
terminal_widget_set_custom_title (TerminalWidget *widget,
                                  const gchar    *title)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  if (!exo_str_is_equal (widget->custom_title, title))
    {
      g_free (widget->custom_title);
      widget->custom_title = g_strdup (title != NULL ? title : "");
      g_object_notify (G_OBJECT (widget), "custom-title");
      g_object_notify (G_OBJECT (widget), "title");
    }
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

  gtk_widget_grab_focus (widget->terminal);
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
 * terminal_widget_get_title:
 * @widget      : A #TerminalWidget.
 *
 * Return value : The title to set for this terminal screen.
 *                The returned string should be freed when
 *                no longer needed.
 **/
gchar*
terminal_widget_get_title (TerminalWidget *widget)
{
  TerminalTitle mode;
  const gchar  *vte_title;
  gboolean      vte_workaround_title_bug;
  gchar        *window_title;
  gchar        *initial;
  gchar        *title;
  gchar        *tmp;

  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), NULL);

  if (G_UNLIKELY (*widget->custom_title != '\0'))
    return g_strdup (widget->custom_title);

  g_object_get (G_OBJECT (widget->preferences),
                "title-initial", &initial,
                "title-mode", &mode,
                "vte-workaround-title-bug", &vte_workaround_title_bug,
                NULL);

  vte_title = vte_terminal_get_window_title (VTE_TERMINAL (widget->terminal));
  window_title = (vte_title != NULL) ? g_strdup (vte_title) : NULL;

  /* work around VTE problem, see #10 for details */
  if (window_title != NULL && vte_workaround_title_bug)
    {
      /* remove initial title from the end of the dynamic title */
      tmp = g_strconcat (" - ", initial, NULL);
      while (g_str_has_suffix (window_title, tmp))
        window_title[strlen (window_title) - strlen (tmp)] = '\0';
      g_free (tmp);

      /* remove initial title from the end of the dynamic title */
      tmp = g_strconcat (initial, " - ", NULL);
      while (g_str_has_prefix (window_title, tmp))
        {
          memmove (window_title, window_title + strlen (tmp),
                   strlen (window_title) + 1 - strlen (tmp));
        }
      g_free (tmp);

#ifdef DEBUG
      g_print ("Terminal application specified title \"%s\", "
               "we've stripped that down to \"%s\"\n",
               vte_title, window_title);
#endif
    }

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
        title = g_strconcat (window_title, " - ", initial, NULL);
      else
        title = g_strdup (initial);
      break;

    case TERMINAL_TITLE_APPEND:
      if (window_title != NULL)
        title = g_strconcat (initial, " - ", window_title, NULL);
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

  g_free (window_title);
  g_free (initial);

  return title;
}



/**
 * terminal_widget_get_working_directory:
 * @widget      : A #TerminalWidget.
 *
 * Determinies the working directory using various OS-specific mechanisms.
 *
 * Return value : The current working directory of @widget.
 **/
const gchar*
terminal_widget_get_working_directory (TerminalWidget *widget)
{
  gchar  buffer[4096 + 1];
  gchar *file;
  gchar *cwd;
  gint   length;

  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), NULL);

  if (widget->pid >= 0)
    {
      file = g_strdup_printf ("/proc/%d/cwd", widget->pid);
      length = readlink (file, buffer, sizeof (buffer));

      if (length > 0 && *buffer == '/')
        {
          buffer[length] = '\0';
          g_free (widget->working_directory);
          widget->working_directory = g_strdup (buffer);
        }
      else if (length == 0)
        {
          cwd = g_get_current_dir ();
          if (G_LIKELY (cwd != NULL))
            {
              if (chdir (file) == 0)
                {
                  g_free (widget->working_directory);
                  widget->working_directory = g_get_current_dir ();
                  chdir (cwd);
                }

              g_free (cwd);
            }
        }

      g_free (file);
    }

  return widget->working_directory;
}



/**
 * terminal_widget_set_working_directory:
 * @widget    : A #TerminalWidget.
 * @directory :
 **/
void
terminal_widget_set_working_directory (TerminalWidget *widget,
                                       const gchar    *directory)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));
  g_return_if_fail (directory != NULL);

  g_free (widget->working_directory);
  widget->working_directory = g_strdup (directory);
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



/**
 * terminal_widget_get_restart_command:
 * @widget  : A #TerminalWidget.
 *
 * Return value:
 **/
GList*
terminal_widget_get_restart_command (TerminalWidget *widget)
{
  const gchar *directory;
  GList       *result = NULL;

  g_return_val_if_fail (TERMINAL_IS_WIDGET (widget), NULL);

  if (widget->custom_command != NULL)
    {
      result = g_list_append (result, g_strdup ("-e"));
      result = g_list_append (result, g_strjoinv (" ", widget->custom_command));
    }

  if (widget->custom_title != NULL)
    {
      result = g_list_append (result, g_strdup ("-t"));
      result = g_list_append (result, g_strdup (widget->custom_title));
    }

  directory = terminal_widget_get_working_directory (widget);
  if (G_LIKELY (directory != NULL))
    {
      result = g_list_append (result, g_strdup ("--working-directory"));
      result = g_list_append (result, g_strdup (directory));
    }

  return result;
}



