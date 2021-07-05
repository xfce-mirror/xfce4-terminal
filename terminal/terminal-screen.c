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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_LIBUTEMPTER
#include <utempter.h>
#endif

#include <sys/wait.h>

#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include <terminal/terminal-util.h>
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-image-loader.h>
#include <terminal/terminal-marshal.h>
#include <terminal/terminal-screen.h>
#include <terminal/terminal-widget.h>
#include <terminal/terminal-window.h>

#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif
#include <glib/gstdio.h>

/* offset of saturation random value */
#define SATURATION_WINDOW 0.20

/* taken from gnome-terminal (terminal-screen.c) */
#define SPAWN_TIMEOUT (30 * 1000 /* 30 s*/)

/* minimum terminal dimensions */
#define MIN_COLUMNS 4
#define MIN_ROWS    1



enum
{
  PROP_0,
  PROP_CUSTOM_TITLE,
  PROP_TITLE
};

enum
{
  GET_CONTEXT_MENU,
  SELECTION_CHANGED,
  CLOSE_TAB,
  LAST_SIGNAL
};

enum
{
  HSV_HUE,
  HSV_SATURATION,
  HSV_VALUE,
  N_HSV
};



static void       terminal_screen_finalize                      (GObject               *object);
static void       terminal_screen_get_property                  (GObject               *object,
                                                                 guint                  prop_id,
                                                                 GValue                *value,
                                                                 GParamSpec            *pspec);
static void       terminal_screen_set_property                  (GObject               *object,
                                                                 guint                  prop_id,
                                                                 const GValue          *value,
                                                                 GParamSpec            *pspec);
static void       terminal_screen_realize                       (GtkWidget             *widget);
static void       terminal_screen_unrealize                     (GtkWidget             *widget);
static void       terminal_screen_style_updated                 (GtkWidget             *widget);
static gboolean   terminal_screen_draw                          (GtkWidget             *widget,
                                                                 cairo_t               *cr,
                                                                 gpointer               user_data);
static void       terminal_screen_draw_border                   (cairo_t               *cr,
                                                                 const gchar           *color_spec,
                                                                 gint                   width,
                                                                 gint                   height);
static void       terminal_screen_preferences_changed           (TerminalPreferences   *preferences,
                                                                 GParamSpec            *pspec,
                                                                 TerminalScreen        *screen);
static gboolean   terminal_screen_get_child_command             (TerminalScreen        *screen,
                                                                 gchar                **command,
                                                                 gchar               ***argv,
                                                                 GError               **error);
static gchar     *terminal_screen_parse_title                   (TerminalScreen        *screen,
                                                                 const gchar           *title);
static gchar    **terminal_screen_get_child_environment         (TerminalScreen        *screen);
static void       terminal_screen_update_background             (TerminalScreen        *screen);
static void       terminal_screen_update_binding_backspace      (TerminalScreen        *screen);
static void       terminal_screen_update_binding_delete         (TerminalScreen        *screen);
static void       terminal_screen_update_binding_ambiguous_width(TerminalScreen        *screen);
static void       terminal_screen_update_encoding               (TerminalScreen        *screen);
static void       terminal_screen_update_colors                 (TerminalScreen        *screen);
static void       terminal_screen_update_misc_bell              (TerminalScreen        *screen);
static void       terminal_screen_update_misc_cursor_blinks     (TerminalScreen        *screen);
static void       terminal_screen_update_misc_cursor_shape      (TerminalScreen        *screen);
static void       terminal_screen_update_misc_mouse_autohide    (TerminalScreen        *screen);
static void       terminal_screen_update_misc_rewrap_on_resize  (TerminalScreen        *screen);
static void       terminal_screen_update_scrolling_lines        (TerminalScreen        *screen);
static void       terminal_screen_update_scrolling_on_output    (TerminalScreen        *screen);
static void       terminal_screen_update_scrolling_on_keystroke (TerminalScreen        *screen);
static void       terminal_screen_update_text_blink_mode        (TerminalScreen        *screen);
static void       terminal_screen_update_title                  (TerminalScreen        *screen);
static void       terminal_screen_update_word_chars             (TerminalScreen        *screen);
static void       terminal_screen_vte_child_exited              (VteTerminal           *terminal,
                                                                 gint                   status,
                                                                 TerminalScreen        *screen);
static void       terminal_screen_vte_eof                       (VteTerminal           *terminal,
                                                                 TerminalScreen        *screen);
static GtkWidget *terminal_screen_vte_get_context_menu          (TerminalWidget        *widget,
                                                                 TerminalScreen        *screen);
static void       terminal_screen_vte_selection_changed         (VteTerminal           *terminal,
                                                                 TerminalScreen        *screen);
static void       terminal_screen_vte_window_title_changed      (VteTerminal           *terminal,
                                                                 TerminalScreen        *screen);
static void       terminal_screen_vte_resize_window             (VteTerminal           *terminal,
                                                                 guint                  width,
                                                                 guint                  height,
                                                                 TerminalScreen        *screen);
static void       terminal_screen_vte_window_contents_changed   (TerminalScreen        *screen);
static void       terminal_screen_vte_window_contents_resized   (TerminalScreen        *screen);
static void       terminal_screen_update_label_orientation      (TerminalScreen        *screen);
static void       terminal_screen_urgent_bell                   (TerminalWidget        *widget,
                                                                 TerminalScreen        *screen);
static void       terminal_screen_set_custom_command            (TerminalScreen        *screen,
                                                                 gchar                **command);
static void       terminal_screen_set_tab_label_color           (TerminalScreen        *screen,
                                                                 const GdkRGBA         *color);
static GtkWidget* terminal_screen_unsafe_paste_dialog_new       (TerminalScreen        *screen,
                                                                 const gchar           *text);
static void       terminal_screen_paste_unsafe_text             (TerminalScreen        *screen,
                                                                 const gchar           *text,
                                                                 GdkAtom                original_clipboard);



struct _TerminalScreenClass
{
  GtkOverlayClass parent_class;
};

struct _TerminalScreen
{
  GtkOverlay           parent_instance;
  TerminalPreferences *preferences;
  TerminalImageLoader *loader;
  GtkWidget           *hbox;
  GtkWidget           *terminal;
  GtkWidget           *scrollbar;
  GtkWidget           *tab_label;

  GdkRGBA              background_color;

  guint                session_id;

  GPid                 pid;
  gchar               *working_directory;

  gchar              **custom_command;
  gchar               *custom_title;
  gchar               *initial_title;

  gchar               *custom_fg_color;
  gchar               *custom_bg_color;
  gchar               *custom_title_color;

  TerminalTitle        dynamic_title_mode;
  guint                hold : 1;
  guint                has_random_bg_color : 1;
#if !VTE_CHECK_VERSION (0, 51, 1)
  guint                scroll_on_output : 1;
#endif

  guint                activity_timeout_id;
  time_t               activity_resize_time;
};



static guint screen_signals[LAST_SIGNAL];
static guint screen_last_session_id = 0;



G_DEFINE_TYPE (TerminalScreen, terminal_screen, GTK_TYPE_OVERLAY)



static void
terminal_screen_class_init (TerminalScreenClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_screen_finalize;
  gobject_class->get_property = terminal_screen_get_property;
  gobject_class->set_property = terminal_screen_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = terminal_screen_realize;
  gtkwidget_class->unrealize = terminal_screen_unrealize;
  gtkwidget_class->style_updated = terminal_screen_style_updated;

  /**
   * TerminalScreen:custom-title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_TITLE,
                                   g_param_spec_string ("custom-title",
                                                        "custom-title",
                                                        "custom-title",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * TerminalScreen:title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "title",
                                                        "title",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * TerminalScreen::get-context-menu
   **/
  screen_signals[GET_CONTEXT_MENU] =
    g_signal_new (I_("get-context-menu"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _terminal_marshal_OBJECT__VOID,
                  GTK_TYPE_MENU, 0);

  /**
   * TerminalScreen::selection-changed
   **/
  screen_signals[SELECTION_CHANGED] =
    g_signal_new (I_("selection-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * TerminalScreen::close-tab-request
   **/
  screen_signals[CLOSE_TAB] =
    g_signal_new (I_("close-tab-request"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static void
terminal_screen_init (TerminalScreen *screen)
{
  screen->loader = NULL;
  screen->working_directory = g_get_current_dir ();
  screen->dynamic_title_mode = TERMINAL_TITLE_DEFAULT;
  screen->session_id = ++screen_last_session_id;
  screen->pid = -1;

  screen->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (screen), screen->hbox);

  screen->terminal = g_object_new (TERMINAL_TYPE_WIDGET, NULL);
  g_signal_connect (G_OBJECT (screen->terminal), "child-exited",
      G_CALLBACK (terminal_screen_vte_child_exited), screen);
  g_signal_connect (G_OBJECT (screen->terminal), "eof",
      G_CALLBACK (terminal_screen_vte_eof), screen);
  g_signal_connect (G_OBJECT (screen->terminal), "context-menu",
      G_CALLBACK (terminal_screen_vte_get_context_menu), screen);
  g_signal_connect (G_OBJECT (screen->terminal), "selection-changed",
      G_CALLBACK (terminal_screen_vte_selection_changed), screen);
  g_signal_connect (G_OBJECT (screen->terminal), "window-title-changed",
      G_CALLBACK (terminal_screen_vte_window_title_changed), screen);
  g_signal_connect (G_OBJECT (screen->terminal), "resize-window",
      G_CALLBACK (terminal_screen_vte_resize_window), screen);
  g_signal_connect (G_OBJECT (screen->terminal), "draw",
      G_CALLBACK (terminal_screen_draw), screen);
  g_signal_connect_swapped (G_OBJECT (screen->terminal), "paste-selection-request",
      G_CALLBACK (terminal_screen_paste_primary), screen);
  gtk_box_pack_start (GTK_BOX (screen->hbox), screen->terminal, TRUE, TRUE, 0);

  screen->scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL,
                                         gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (screen->terminal)));
  gtk_box_pack_start (GTK_BOX (screen->hbox), screen->scrollbar, FALSE, FALSE, 0);
  g_signal_connect_after (G_OBJECT (screen->scrollbar), "button-press-event", G_CALLBACK (gtk_true), NULL);

  /* watch preferences changes */
  screen->preferences = terminal_preferences_get ();
  g_signal_connect (G_OBJECT (screen->preferences), "notify",
      G_CALLBACK (terminal_screen_preferences_changed), screen);

  /* show the terminal */
  gtk_widget_show_all (screen->hbox);

  /* apply current settings */
  terminal_screen_update_binding_backspace (screen);
  terminal_screen_update_binding_delete (screen);
  terminal_screen_update_binding_ambiguous_width (screen);
  terminal_screen_update_encoding (screen);
  terminal_screen_update_font (screen);
  terminal_screen_update_misc_bell (screen);
  terminal_screen_update_misc_cursor_blinks (screen);
  terminal_screen_update_misc_cursor_shape (screen);
  terminal_screen_update_misc_mouse_autohide (screen);
  terminal_screen_update_misc_rewrap_on_resize (screen);
  terminal_screen_update_scrolling_bar (screen);
  terminal_screen_update_scrolling_lines (screen);
  terminal_screen_update_scrolling_on_output (screen);
  terminal_screen_update_scrolling_on_keystroke (screen);
  terminal_screen_update_text_blink_mode (screen);
  terminal_screen_update_word_chars (screen);
  terminal_screen_update_background (screen);
  terminal_screen_update_colors (screen);

  /* last, connect contents-changed to avoid a race with updates above */
  g_signal_connect_swapped (G_OBJECT (screen->terminal), "contents-changed",
      G_CALLBACK (terminal_screen_vte_window_contents_changed), screen);
  g_signal_connect_swapped (G_OBJECT (screen->terminal), "size-allocate",
      G_CALLBACK (terminal_screen_vte_window_contents_resized), screen);
}



static void
terminal_screen_finalize (GObject *object)
{
  TerminalScreen *screen = TERMINAL_SCREEN (object);

  if (screen->activity_timeout_id != 0)
    g_source_remove (screen->activity_timeout_id);

  /* detach from preferences */
  g_signal_handlers_disconnect_by_func (screen->preferences,
      G_CALLBACK (terminal_screen_preferences_changed), screen);
  g_object_unref (G_OBJECT (screen->preferences));

  if (screen->loader != NULL)
    g_object_unref (G_OBJECT (screen->loader));

  g_strfreev (screen->custom_command);
  g_free (screen->working_directory);
  g_free (screen->custom_title);
  g_free (screen->initial_title);
  g_free (screen->custom_fg_color);
  g_free (screen->custom_bg_color);
  g_free (screen->custom_title_color);

  (*G_OBJECT_CLASS (terminal_screen_parent_class)->finalize) (object);
}



static void
terminal_screen_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  TerminalScreen *screen = TERMINAL_SCREEN (object);
  const gchar    *title = NULL;
  TerminalTitle   mode;
  gchar          *initial;
  gchar          *parsed_title = NULL;
  gchar          *custom_title;

  switch (prop_id)
    {
    case PROP_CUSTOM_TITLE:
      if (screen->custom_title != NULL)
        g_value_set_string (value, screen->custom_title);
      else
        g_value_set_static_string (value, "");
      break;

    case PROP_TITLE:
      if (G_UNLIKELY (screen->custom_title != NULL))
        {
          custom_title = terminal_screen_parse_title (screen, screen->custom_title);
          g_value_take_string (value, custom_title);
        }
      else
        {
          if (G_UNLIKELY (screen->dynamic_title_mode != TERMINAL_TITLE_DEFAULT))
            mode = screen->dynamic_title_mode;
          else
            g_object_get (G_OBJECT (screen->preferences), "title-mode", &mode, NULL);

          if (G_UNLIKELY (mode == TERMINAL_TITLE_HIDE))
            {
              /* show the initial title if the dynamic title is set to hidden */
              if (G_UNLIKELY (screen->initial_title != NULL))
                initial = g_strdup (screen->initial_title);
              else
                g_object_get (G_OBJECT (screen->preferences), "title-initial", &initial, NULL);
              parsed_title = terminal_screen_parse_title (screen, initial);
              title = parsed_title;
              g_free (initial);
            }
          else if (G_LIKELY (screen->terminal != NULL))
            {
              title = vte_terminal_get_window_title (VTE_TERMINAL (screen->terminal));
            }

          /* TRANSLATORS: title for the tab/window used when all other
           * possible titles were empty strings */
          if (title == NULL || *title == '\0')
            title = _("Untitled");

          g_value_set_string (value, title);

          g_free (parsed_title);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
terminal_screen_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  TerminalScreen *screen = TERMINAL_SCREEN (object);

  switch (prop_id)
    {
    case PROP_CUSTOM_TITLE:
      terminal_screen_set_custom_title (screen, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
terminal_screen_realize (GtkWidget *widget)
{
  GdkScreen *screen;

  (*GTK_WIDGET_CLASS (terminal_screen_parent_class)->realize) (widget);

  /* make sure the TerminalWidget is realized as well */
  if (!gtk_widget_get_realized (TERMINAL_SCREEN (widget)->terminal))
    gtk_widget_realize (TERMINAL_SCREEN (widget)->terminal);

  /* connect to the "composited-changed" signal */
  screen = gtk_widget_get_screen (widget);
  g_signal_connect_swapped (G_OBJECT (screen), "composited-changed", G_CALLBACK (terminal_screen_update_background), widget);
}



static void
terminal_screen_unrealize (GtkWidget *widget)
{
  GdkScreen *screen;

  /* disconnect the "composited-changed" handler */
  screen = gtk_widget_get_screen (widget);
  g_signal_handlers_disconnect_by_func (G_OBJECT (screen), terminal_screen_update_background, widget);

  (*GTK_WIDGET_CLASS (terminal_screen_parent_class)->unrealize) (widget);
}



static void
terminal_screen_style_updated (GtkWidget *widget)
{
  (*GTK_WIDGET_CLASS (terminal_screen_parent_class)->style_updated) (widget);

  terminal_screen_update_colors (TERMINAL_SCREEN (widget));
}



static gboolean
terminal_screen_draw (GtkWidget *widget,
                      cairo_t   *cr,
                      gpointer   user_data)
{
  TerminalScreen     *screen = TERMINAL_SCREEN (user_data);
  TerminalBackground  background_mode;
  gchar              *color_border_spec;
  GdkPixbuf          *image;
  gint                width, height;
  cairo_surface_t    *surface;
  cairo_t            *ctx;

  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), FALSE);
  terminal_return_val_if_fail (VTE_IS_TERMINAL (screen->terminal), FALSE);

  g_object_get (G_OBJECT (screen->preferences),
                "background-mode", &background_mode,
                "misc-color-border-window", &color_border_spec,
                NULL);

  if (G_LIKELY (background_mode != TERMINAL_BACKGROUND_IMAGE))
    return FALSE;

  width = gtk_widget_get_allocated_width (screen->terminal);
  height = gtk_widget_get_allocated_height (screen->terminal);

  if (screen->loader == NULL)
    screen->loader = terminal_image_loader_get ();
  image = terminal_image_loader_load (screen->loader, width, height);

  if (G_UNLIKELY (image == NULL))
    return FALSE;

  g_signal_handlers_disconnect_by_func (G_OBJECT (screen->terminal),
      G_CALLBACK (terminal_screen_draw), screen);

  cairo_save (cr);

  /* draw background image; cairo_set_operator() allows PNG transparency */
  gdk_cairo_set_source_pixbuf (cr, image, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  g_object_unref (G_OBJECT (image));

  /* draw vte terminal */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  ctx = cairo_create (surface);


  gtk_widget_draw (screen->terminal, ctx);

  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_paint (cr);

  /* Draw a subtle border if set in configuration file.
   * Only draw if color set by user. No default value.
   * */
  if (G_UNLIKELY (color_border_spec != NULL)) {
    terminal_screen_draw_border (cr, color_border_spec, width, height);
    g_free (color_border_spec);
  }

  cairo_destroy (ctx);
  cairo_surface_destroy (surface);

  cairo_restore (cr);

  g_signal_connect (G_OBJECT (screen->terminal), "draw",
      G_CALLBACK (terminal_screen_draw), screen);

  return TRUE;
}

static void
terminal_screen_draw_border(cairo_t     *cr,
                            const gchar *color_spec,
                            gint         width,
                            gint         height)
{
  GdkRGBA                 rgba;

  gdk_rgba_parse (&rgba, color_spec);

  /* Set path for border.
   * Here one could also have a CSS approach by width for each side
   * in case user do not want borders on all sides.
   * Likely best with GtkCssProvider or similar.
   * */
  cairo_move_to(cr, 0, 0);
  cairo_line_to(cr, width, 0);
  cairo_line_to(cr, width, height);
  cairo_line_to(cr, 0, height);
  cairo_close_path(cr);

  /* OPERATOR ADD: source and destination layers are accumulated.
   * https://cairographics.org/operators/#add
   * */
  cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
  cairo_set_source_rgba (cr, rgba.red, rgba.green, rgba.blue, rgba.alpha);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);

  /* One could paint twice to give it some effect, but complicated
   * how to deal with user preferences if mixing colors.
   cairo_set_source_rgba (cr, 0.9, 0.9, 0.9, 0.7);
   cairo_set_line_width (cr, 2.0);
   cairo_stroke_preserve (cr);
   ...
   cairo_set_line_width (cr, 1.0);
   cairo_stroke (cr);
   */

}

static void
terminal_screen_preferences_changed (TerminalPreferences *preferences,
                                     GParamSpec          *pspec,
                                     TerminalScreen      *screen)
{
  const gchar *name;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (TERMINAL_IS_PREFERENCES (preferences));
  terminal_return_if_fail (screen->preferences == preferences);

  /* get name */
  name = g_param_spec_get_name (pspec);
  terminal_assert (name != NULL);

  if (strncmp ("background-", name, strlen ("background-")) == 0)
    terminal_screen_update_background (screen);
  else if (strcmp ("binding-backspace", name) == 0)
    terminal_screen_update_binding_backspace (screen);
  else if (strcmp ("binding-delete", name) == 0)
    terminal_screen_update_binding_delete (screen);
  else if (strcmp ("binding-ambiguous-width", name) == 0)
    terminal_screen_update_binding_ambiguous_width (screen);
#if VTE_CHECK_VERSION (0, 51, 3)
  else if (strcmp ("cell-width-scale", name) == 0 || strcmp ("cell-height-scale", name) == 0)
    terminal_screen_update_font (screen);
#endif
  else if (strncmp ("color-", name, strlen ("color-")) == 0)
    terminal_screen_update_colors (screen);
  else if (strncmp ("font-", name, strlen ("font-")) == 0)
    terminal_screen_update_font (screen);
  else if (strncmp ("misc-bell", name, strlen ("misc-bell")) == 0)
    terminal_screen_update_misc_bell (screen);
  else if (strcmp ("misc-cursor-blinks", name) == 0)
    terminal_screen_update_misc_cursor_blinks (screen);
  else if (strcmp ("misc-cursor-shape", name) == 0)
    terminal_screen_update_misc_cursor_shape (screen);
  else if (strcmp ("misc-mouse-autohide", name) == 0)
    terminal_screen_update_misc_mouse_autohide (screen);
  else if (strcmp ("misc-rewrap-on-resize", name) == 0)
    terminal_screen_update_misc_rewrap_on_resize (screen);
  else if (strcmp ("scrolling-bar", name) == 0)
    terminal_screen_update_scrolling_bar (screen);
  else if (strcmp ("scrolling-lines", name) == 0 || strcmp ("scrolling-unlimited", name) == 0)
    terminal_screen_update_scrolling_lines (screen);
  else if (strcmp ("scrolling-on-output", name) == 0)
    terminal_screen_update_scrolling_on_output (screen);
  else if (strcmp ("scrolling-on-keystroke", name) == 0)
    terminal_screen_update_scrolling_on_keystroke (screen);
  else if (strcmp ("text-blink-mode", name) == 0)
    terminal_screen_update_text_blink_mode (screen);
  else if (strncmp ("title-", name, strlen ("title-")) == 0)
    terminal_screen_update_title (screen);
  else if (strcmp ("word-chars", name) == 0)
    terminal_screen_update_word_chars (screen);
  else if (strcmp ("misc-tab-position", name) == 0)
    terminal_screen_update_label_orientation (screen);
}



static gboolean
terminal_screen_get_child_command (TerminalScreen   *screen,
                                   gchar           **command,
                                   gchar          ***argv,
                                   GError          **error)
{
  struct passwd *pw;
  const gchar   *shell_name;
  const gchar   *shell_fullpath = NULL;
  gchar         *custom_command = NULL;
  gboolean       command_login_shell;
  gboolean       run_custom_command;
  guint          i;
  const gchar   *shells[] = { "/bin/sh",
                              "/bin/bash", "/usr/bin/bash",
                              "/bin/dash", "/usr/bin/dash",
                              "/bin/zsh",  "/usr/bin/zsh",
                              "/bin/tcsh", "/usr/bin/tcsh",
                              "/bin/csh",  "/usr/bin/csh",
                              "/bin/ksh",  "/usr/bin/ksh" };

  if (screen->custom_command != NULL)
    {
      *command = g_strdup (screen->custom_command[0]);
      *argv    = g_strdupv (screen->custom_command);
    }
  else
    {
      g_object_get (G_OBJECT (screen->preferences),
                    "command-login-shell", &command_login_shell,
                    "run-custom-command", &run_custom_command,
                    NULL);

      if (run_custom_command)
        {
          /* use custom command specified in preferences */
          g_object_get (G_OBJECT (screen->preferences),
                        "custom-command", &custom_command,
                        NULL);
          shell_fullpath = custom_command;
        }
      else
        {
          /* use the SHELL environement variable if we're in
          * non-setuid mode and the path is executable */
          if (geteuid () == getuid ()
              && getegid () == getgid ())
            {
              shell_fullpath = g_getenv ("SHELL");
              if (shell_fullpath != NULL
                  && g_access (shell_fullpath, X_OK) != 0)
                shell_fullpath = NULL;
            }

          if (shell_fullpath == NULL)
            {
              pw = getpwuid (getuid ());
              if (pw != NULL
                  && pw->pw_shell != NULL
                  && g_access (pw->pw_shell, X_OK) == 0)
                {
                  /* set the shell from the password database */
                  shell_fullpath = pw->pw_shell;
                }
              else
                {
                  /* lookup a good fallback */
                  for (i = 0; i < G_N_ELEMENTS (shells); i++)
                    {
                      if (access (shells [i], X_OK) == 0)
                        {
                          shell_fullpath = shells [i];
                          break;
                        }
                    }

                  if (G_UNLIKELY (shell_fullpath == NULL))
                    {
                      /* the system is truly broken */
                      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                                   _("Unable to determine your login shell."));
                      return FALSE;
                    }
                 }
            }
        }

      terminal_assert (shell_fullpath != NULL);
      shell_name = strrchr (shell_fullpath, '/');
      if (shell_name != NULL)
        ++shell_name;
      else
        shell_name = shell_fullpath;
      *command = g_strdup (shell_fullpath);

      *argv = g_new (gchar *, 2);
      if (command_login_shell)
        (*argv)[0] = g_strconcat ("-", shell_name, NULL);
      else
        (*argv)[0] = g_strdup (shell_name);
      (*argv)[1] = NULL;

      if (custom_command != NULL)
        g_free (custom_command);
    }

  return TRUE;
}



static gchar *
terminal_screen_parse_title (TerminalScreen *screen,
                             const gchar    *title)
{
  GString     *string;
  const gchar *remainder;
  const gchar *percent;
  const gchar *directory = NULL;
  gchar       *base_name;
  const gchar *vte_title;

  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);

  if (G_UNLIKELY (title == NULL))
    return g_strdup ("");

  string = g_string_new (NULL);
  remainder = title;

  /* walk from % character to % character */
  for (;;)
    {
      /* look out for the next % character */
      percent = strchr (remainder, '%');
      if (percent == NULL)
        {
          /* we parsed the whole string */
          g_string_append (string, remainder);
          break;
        }

      /* append the characters in between */
      g_string_append_len (string, remainder, percent - remainder);
      remainder = percent + 1;

      /* handle the "%" character */
      switch (*remainder)
        {
        case '#':
          g_string_append_printf (string, "%u", screen->session_id);
          break;

        case 'd':
        case 'D':
          if (directory == NULL)
            directory = terminal_screen_get_working_directory (screen);

          if (G_LIKELY (directory != NULL))
            {
              if (*remainder == 'D')
                {
                  /* long directory name */
                  g_string_append (string, directory);
                }
              else
                {
                  /* short directory name */
                  base_name = g_path_get_basename (directory);
                  g_string_append (string, base_name);
                  g_free (base_name);
                }
            }
          break;

        case 'w':
          /* window title from vte */
          vte_title = vte_terminal_get_window_title (VTE_TERMINAL (screen->terminal));
          if (G_UNLIKELY (vte_title == NULL))
            vte_title = _("Untitled");
          g_string_append (string, vte_title);
          break;

        default:
          g_string_append_c (string, '%');
          continue;
        }

      remainder++;
    }

  return g_string_free (string, FALSE);
}



static gchar**
terminal_screen_get_child_environment (TerminalScreen *screen)
{
  GtkWidget     *toplevel;
  const gchar   *display_name;
  gchar        **result;
  gchar        **p;
  guint          n;
  gchar        **env;
  const gchar   *value;

  /* get all the environ variables */
  env = g_listenv ();

  n = g_strv_length (env);
  result = g_new (gchar *, n + 4);

  for (n = 0, p = env; *p != NULL; ++p)
    {
      /* do not copy the following variables */
      if (strcmp (*p, "COLUMNS") == 0
          || strcmp (*p, "LINES") == 0
          || strcmp (*p, "WINDOWID") == 0
          || strcmp (*p, "GNOME_DESKTOP_ICON") == 0
          || strcmp (*p, "COLORTERM") == 0
          || strcmp (*p, "DISPLAY") == 0
          || strcmp (*p, "TERM") == 0)
        continue;

#if !VTE_CHECK_VERSION (0, 51, 90)
      /* copy working directory to $PWD, to preserve symlinks
       * see https://bugzilla.gnome.org/show_bug.cgi?id=758452 */
      if (strcmp (*p, "PWD") == 0)
        {
          result[n++] = g_strconcat (*p, "=", screen->working_directory, NULL);
          continue;
        }
#endif

      /* copy the variable */
      value = g_getenv (*p);
      if (G_LIKELY (value != NULL))
        result[n++] = g_strconcat (*p, "=", value, NULL);
    }

  g_strfreev (env);

  result[n++] = g_strdup_printf ("COLORTERM=%s", PACKAGE_NAME);

#ifdef GDK_WINDOWING_X11
  /* determine the toplevel widget */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (screen));
  if (toplevel != NULL && gtk_widget_get_realized (toplevel) && GDK_IS_X11_WINDOW (gtk_widget_get_window (toplevel)))
    {
      result[n++] = g_strdup_printf ("WINDOWID=%ld", (glong) gdk_x11_window_get_xid (gtk_widget_get_window (toplevel)));

      /* determine the DISPLAY value for the command */
      display_name = gdk_display_get_name (gdk_screen_get_display (gtk_widget_get_screen (toplevel)));
      result[n++] = g_strdup_printf ("DISPLAY=%s", display_name);
    }
#endif

  result[n] = NULL;

  return result;
}



static void
terminal_screen_update_background (TerminalScreen *screen)
{
  TerminalBackground background_mode;
  gdouble            background_alpha;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (VTE_IS_TERMINAL (screen->terminal));

  g_object_get (G_OBJECT (screen->preferences), "background-mode", &background_mode, NULL);

  if (G_UNLIKELY (background_mode == TERMINAL_BACKGROUND_TRANSPARENT))
    g_object_get (G_OBJECT (screen->preferences), "background-darkness", &background_alpha, NULL);
  else if (G_UNLIKELY (background_mode == TERMINAL_BACKGROUND_IMAGE))
    g_object_get (G_OBJECT (screen->preferences), "background-image-shading", &background_alpha, NULL);
  else
    background_alpha = 1.0;

  screen->background_color.alpha = background_alpha;
  vte_terminal_set_color_background (VTE_TERMINAL (screen->terminal), &screen->background_color);

  gtk_widget_queue_draw (GTK_WIDGET (screen));
}



static VteEraseBinding
terminal_screen_binding_vte (TerminalEraseBinding binding)
{
  switch (binding)
    {
    case TERMINAL_ERASE_BINDING_AUTO:
      return VTE_ERASE_AUTO;

    case TERMINAL_ERASE_BINDING_ASCII_BACKSPACE:
      return VTE_ERASE_ASCII_BACKSPACE;

    case TERMINAL_ERASE_BINDING_ASCII_DELETE:
      return VTE_ERASE_ASCII_DELETE;

    case TERMINAL_ERASE_BINDING_DELETE_SEQUENCE:
      return VTE_ERASE_DELETE_SEQUENCE;

    case TERMINAL_ERASE_BINDING_ERASE_TTY:
      return VTE_ERASE_TTY;

    default:
      terminal_assert_not_reached ();
    }

  return VTE_ERASE_AUTO;
}



static void
terminal_screen_update_binding_backspace (TerminalScreen *screen)
{
  TerminalEraseBinding binding;

  g_object_get (G_OBJECT (screen->preferences), "binding-backspace", &binding, NULL);
  vte_terminal_set_backspace_binding (VTE_TERMINAL (screen->terminal),
      terminal_screen_binding_vte (binding));
}



static void
terminal_screen_update_binding_delete (TerminalScreen *screen)
{
  TerminalEraseBinding binding;

  g_object_get (G_OBJECT (screen->preferences), "binding-delete", &binding, NULL);
  vte_terminal_set_delete_binding (VTE_TERMINAL (screen->terminal),
                                   terminal_screen_binding_vte (binding));
}



static void
terminal_screen_update_binding_ambiguous_width (TerminalScreen *screen)
{
  TerminalAmbiguousWidthBinding binding;

  g_object_get (G_OBJECT (screen->preferences), "binding-ambiguous-width", &binding, NULL);
  vte_terminal_set_cjk_ambiguous_width (VTE_TERMINAL (screen->terminal),
                                        binding == TERMINAL_AMBIGUOUS_WIDTH_BINDING_NARROW ? 1 : 2);
}



static void
terminal_screen_update_encoding (TerminalScreen *screen)
{
  gchar *encoding;

  g_object_get (G_OBJECT (screen->preferences), "encoding", &encoding, NULL);
  terminal_screen_set_encoding (screen, encoding);
  g_free (encoding);
}



static void
terminal_screen_update_colors (TerminalScreen *screen)
{
  GdkRGBA    palette[16];
  GdkRGBA    bg;
  GdkRGBA    fg;
  GdkRGBA    cursor;
  GdkRGBA    selection;
  GdkRGBA    bold;
  gboolean   cursor_use_default;
  gboolean   selection_use_default;
  gboolean   bold_use_default;
  guint      n = 0;
  gboolean   has_bg;
  gboolean   has_fg;
  gboolean   valid_palette = FALSE;
  gchar     *palette_str;
  gchar    **colors;
  gboolean   vary_bg;
  gdouble    hsv[N_HSV];
  gdouble    sat_min, sat_max;
  gboolean   bold_is_bright;
  gboolean   use_theme;

  GtkStyleContext *context = gtk_widget_get_style_context (gtk_widget_get_toplevel (GTK_WIDGET (screen)));

  g_object_get (screen->preferences,
                "color-palette", &palette_str,
                "color-cursor-use-default", &cursor_use_default,
                "color-selection-use-default", &selection_use_default,
                "color-bold-use-default", &bold_use_default,
                "color-background-vary", &vary_bg,
                "color-bold-is-bright", &bold_is_bright,
                "color-use-theme", &use_theme,
                NULL);

  if (G_LIKELY (palette_str != NULL))
    {
      colors = g_strsplit (palette_str, ";", -1);
      g_free (palette_str);

      if (colors != NULL)
        for (; n < 16 && colors[n] != NULL; n++)
          if (!gdk_rgba_parse (palette + n, colors[n]))
            {
              g_warning ("Unable to parse color \"%s\".", colors[n]);
              break;
            }

      g_strfreev (colors);
      valid_palette = (n == 16);
    }

  if (G_LIKELY (screen->custom_fg_color == NULL))
    {
      has_fg = terminal_preferences_get_color (screen->preferences, "color-foreground", &fg);
      if (use_theme || !has_fg)
        {
          gtk_style_context_get_color (context, GTK_STATE_FLAG_ACTIVE, &fg);
          has_fg = TRUE;
        }
    }
  else
    has_fg = gdk_rgba_parse (&fg, screen->custom_fg_color);

  if (G_LIKELY (screen->custom_bg_color == NULL))
    {
      has_bg = terminal_preferences_get_color (screen->preferences, "color-background", &bg);
      if (use_theme || !has_bg)
        {
          gtk_style_context_get_background_color (context, GTK_STATE_FLAG_ACTIVE, &bg);
          has_bg = TRUE;
        }

      /* we pick a random hue value to keep readability */
      if (vary_bg && !screen->has_random_bg_color)
        {
          gtk_rgb_to_hsv (bg.red, bg.green, bg.blue,
                          NULL, &hsv[HSV_SATURATION], &hsv[HSV_VALUE]);

          /* pick random hue */
          hsv[HSV_HUE] = g_random_double_range (0.00, 1.00);

          /* saturation moving window, depending on the value */
          if (hsv[HSV_SATURATION] <= SATURATION_WINDOW)
            {
              sat_min = 0.00;
              sat_max = (2 * SATURATION_WINDOW);
            }
          else if (hsv[HSV_SATURATION] >= (1.00 - SATURATION_WINDOW))
            {
              sat_min = 1.00 - (2 * SATURATION_WINDOW);
              sat_max = 1.00;
            }
          else
            {
              sat_min = hsv[HSV_SATURATION] - SATURATION_WINDOW;
              sat_max = hsv[HSV_SATURATION] + SATURATION_WINDOW;
            }

          hsv[HSV_SATURATION] = g_random_double_range (sat_min, sat_max);

          /* and back to a rgb color */
          gtk_hsv_to_rgb (hsv[HSV_HUE], hsv[HSV_SATURATION], hsv[HSV_VALUE],
                          &bg.red, &bg.green, &bg.blue);

          /* save the color */
          screen->background_color.red = bg.red;
          screen->background_color.green = bg.green;
          screen->background_color.blue = bg.blue;

          /* it seems that random color may not be generated on the first run
           * so add a check here */
          if (bg.red != 0 && bg.green != 0 && bg.blue != 0)
            screen->has_random_bg_color = 1;
        }
      else if (vary_bg && screen->has_random_bg_color)
        {
          /* we already have a random bg color - do nothing */
        }
      else if (!vary_bg)
        {
          /* update the color if the vary setting is unchecked */
          screen->background_color.red = bg.red;
          screen->background_color.green = bg.green;
          screen->background_color.blue = bg.blue;
          screen->has_random_bg_color = 0;
        }
    }
  else if ((has_bg = gdk_rgba_parse (&bg, screen->custom_bg_color)))
    {
      /* preserve the alpha value which is responsible for transparency */
      screen->background_color.red = bg.red;
      screen->background_color.green = bg.green;
      screen->background_color.blue = bg.blue;
    }

  if (G_LIKELY (valid_palette))
    {
      vte_terminal_set_colors (VTE_TERMINAL (screen->terminal),
                               has_fg ? &fg : NULL,
                               has_bg ? &screen->background_color : NULL,
                               palette, 16);
    }
  else
    {
      vte_terminal_set_default_colors (VTE_TERMINAL (screen->terminal));
      g_warning ("One of the terminal colors was not parsed successfully. "
                 "The default palette has been applied.");
    }

  /* cursor color */
  if (!cursor_use_default)
    {
      cursor_use_default = !terminal_preferences_get_color (screen->preferences, "color-cursor-foreground", &cursor);
      vte_terminal_set_color_cursor_foreground (VTE_TERMINAL (screen->terminal), cursor_use_default ? NULL : &cursor);
      cursor_use_default = !terminal_preferences_get_color (screen->preferences, "color-cursor", &cursor);
      vte_terminal_set_color_cursor (VTE_TERMINAL (screen->terminal), cursor_use_default ? NULL : &cursor);
    }

  /* selection color */
  if (!selection_use_default)
    {
      selection_use_default = !terminal_preferences_get_color (screen->preferences, "color-selection", &selection);
      vte_terminal_set_color_highlight_foreground (VTE_TERMINAL (screen->terminal), selection_use_default ? NULL : &selection);
      selection_use_default = !terminal_preferences_get_color (screen->preferences, "color-selection-background", &selection);
      vte_terminal_set_color_highlight (VTE_TERMINAL (screen->terminal), selection_use_default ? NULL : &selection);
    }

  /* bold color */
  if (!bold_use_default)
    bold_use_default = !terminal_preferences_get_color (screen->preferences, "color-bold", &bold);
#if VTE_CHECK_VERSION (0, 52, 0)
  /* the meaning of NULL for bold color changed in vte 0.52: see bug #15019 */
  vte_terminal_set_color_bold (VTE_TERMINAL (screen->terminal), bold_use_default ? NULL : &bold);
#else
  /* avoid computed bold color for older vte versions */
  if (!bold_use_default || has_fg)
    vte_terminal_set_color_bold (VTE_TERMINAL (screen->terminal), bold_use_default ? &fg : &bold);
#endif

#if VTE_CHECK_VERSION (0, 51, 3)
  /* "bold-is-bright" supported since vte 0.51.3 */
  vte_terminal_set_bold_is_bright (VTE_TERMINAL (screen->terminal), bold_is_bright);
#endif
}



static void
terminal_screen_update_misc_bell (TerminalScreen *screen)
{
  gboolean bval;
  g_object_get (G_OBJECT (screen->preferences), "misc-bell", &bval, NULL);
  vte_terminal_set_audible_bell (VTE_TERMINAL (screen->terminal), bval);
  g_signal_connect (screen->terminal, "bell", G_CALLBACK (terminal_screen_urgent_bell), screen);
}



static void
terminal_screen_update_misc_cursor_blinks (TerminalScreen *screen)
{
  gboolean bval;
  g_object_get (G_OBJECT (screen->preferences), "misc-cursor-blinks", &bval, NULL);
  vte_terminal_set_cursor_blink_mode (VTE_TERMINAL (screen->terminal),
                                      bval ? VTE_CURSOR_BLINK_ON : VTE_CURSOR_BLINK_OFF);
}



static void
terminal_screen_update_misc_cursor_shape (TerminalScreen *screen)
{
  TerminalCursorShape val;
  VteCursorShape      shape = VTE_CURSOR_SHAPE_BLOCK;

  g_object_get (G_OBJECT (screen->preferences), "misc-cursor-shape", &val, NULL);

  switch (val)
    {
      case TERMINAL_CURSOR_SHAPE_BLOCK:
        break;

      case TERMINAL_CURSOR_SHAPE_IBEAM:
        shape = VTE_CURSOR_SHAPE_IBEAM;
        break;

      case TERMINAL_CURSOR_SHAPE_UNDERLINE:
        shape = VTE_CURSOR_SHAPE_UNDERLINE;
        break;

      default:
        terminal_assert_not_reached ();
    }

  vte_terminal_set_cursor_shape (VTE_TERMINAL (screen->terminal), shape);
}


static void
terminal_screen_update_misc_mouse_autohide (TerminalScreen *screen)
{
  gboolean bval;
  g_object_get (G_OBJECT (screen->preferences), "misc-mouse-autohide", &bval, NULL);
  vte_terminal_set_mouse_autohide (VTE_TERMINAL (screen->terminal), bval);
}



static void
terminal_screen_update_misc_rewrap_on_resize (TerminalScreen *screen)
{
  gboolean bval;
  g_object_get (G_OBJECT (screen->preferences), "misc-rewrap-on-resize", &bval, NULL);
#if !VTE_CHECK_VERSION (0, 58, 0)
  vte_terminal_set_rewrap_on_resize (VTE_TERMINAL (screen->terminal), bval);
#endif
}



static void
terminal_screen_update_scrolling_lines (TerminalScreen *screen)
{
  guint    lines;
  gboolean unlimited;
  g_object_get (G_OBJECT (screen->preferences),
                "scrolling-lines", &lines,
                "scrolling-unlimited", &unlimited,
                NULL);
  vte_terminal_set_scrollback_lines (VTE_TERMINAL (screen->terminal),
                                     unlimited ? -1 : (glong) lines);
}



static void
terminal_screen_update_scrolling_on_output (TerminalScreen *screen)
{
  gboolean scroll;
  g_object_get (G_OBJECT (screen->preferences), "scrolling-on-output", &scroll, NULL);
  terminal_screen_set_scroll_on_output (screen, scroll);
}



static void
terminal_screen_update_scrolling_on_keystroke (TerminalScreen *screen)
{
  gboolean scroll;
  g_object_get (G_OBJECT (screen->preferences), "scrolling-on-keystroke", &scroll, NULL);
  vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (screen->terminal), scroll);
}



static void
terminal_screen_update_text_blink_mode (TerminalScreen *screen)
{
#if VTE_CHECK_VERSION (0, 51, 3)
  TerminalTextBlinkMode val;
  VteTextBlinkMode      mode = VTE_TEXT_BLINK_ALWAYS;

  g_object_get (G_OBJECT (screen->preferences), "text-blink-mode", &val, NULL);

  switch (val)
    {
      case TERMINAL_TEXT_BLINK_MODE_ALWAYS:
        break;

      case TERMINAL_TEXT_BLINK_MODE_NEVER:
        mode = VTE_TEXT_BLINK_NEVER;
        break;

      case TERMINAL_TEXT_BLINK_MODE_FOCUSED:
        mode = VTE_TEXT_BLINK_FOCUSED;
        break;

      case TERMINAL_TEXT_BLINK_MODE_UNFOCUSED:
        mode = VTE_TEXT_BLINK_UNFOCUSED;
        break;

      default:
        terminal_assert_not_reached ();
    }

  vte_terminal_set_text_blink_mode (VTE_TERMINAL (screen->terminal), mode);
#endif
}



static void
terminal_screen_update_title (TerminalScreen *screen)
{
  g_object_notify (G_OBJECT (screen), "title");
}



static void
terminal_screen_update_word_chars (TerminalScreen *screen)
{
  gchar *word_chars;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (TERMINAL_IS_PREFERENCES (screen->preferences));
  terminal_return_if_fail (VTE_IS_TERMINAL (screen->terminal));

  g_object_get (G_OBJECT (screen->preferences), "word-chars", &word_chars, NULL);
  if (G_LIKELY (word_chars != NULL))
    {
      vte_terminal_set_word_char_exceptions (VTE_TERMINAL (screen->terminal), word_chars);
      g_free (word_chars);
    }
}



static void
relaunch_bar_response (GtkInfoBar     *info_bar,
                       gint            response_id,
                       TerminalScreen *screen)
{
  /* relaunch the process or remember to not show it anymore */
  if (response_id == GTK_RESPONSE_YES)
    {
      terminal_screen_launch_child (screen);
      terminal_screen_focus (screen);
    }
  else
    {
      GtkWidget       *content_area = gtk_info_bar_get_content_area (info_bar);
      GList           *children = gtk_container_get_children (GTK_CONTAINER (content_area));
      GtkToggleButton *checkbox = g_list_nth_data (children, 1);

      if (GTK_IS_TOGGLE_BUTTON (checkbox) && gtk_toggle_button_get_active (checkbox))
        g_object_set (G_OBJECT (screen->preferences), "misc-show-relaunch-dialog", FALSE, NULL);
    }

  gtk_widget_destroy (GTK_WIDGET (info_bar));
}



static void
terminal_screen_vte_child_exited (VteTerminal    *terminal,
                                  gint            status,
                                  TerminalScreen *screen)
{
  gboolean show_relaunch_dialog;

  terminal_return_if_fail (VTE_IS_TERMINAL (terminal));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  g_object_get (G_OBJECT (screen->preferences), "misc-show-relaunch-dialog", &show_relaunch_dialog, NULL);

  if (G_LIKELY (!screen->hold))
    gtk_widget_destroy (GTK_WIDGET (screen));
  else if (show_relaunch_dialog)
    {
      /* create "Relaunch" bar */
      GtkWidget *relaunch_bar, *content_area, *label, *checkbox;
      gchar *message;

      relaunch_bar = gtk_info_bar_new_with_buttons (_("_Relaunch"), GTK_RESPONSE_YES, NULL);
      gtk_info_bar_set_message_type (GTK_INFO_BAR (relaunch_bar), GTK_MESSAGE_INFO);
      gtk_info_bar_set_show_close_button (GTK_INFO_BAR (relaunch_bar), TRUE);
      content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (relaunch_bar));

      if (WIFEXITED (status))
        message = g_strdup_printf (_("The child process exited normally with status %d."), WEXITSTATUS (status));
      else if (WIFSIGNALED (status))
        message = g_strdup_printf (_("The child process was aborted by signal %d."), WTERMSIG (status));
      else
        message = g_strdup (_("The child process was aborted."));

      label = gtk_label_new (message);
      g_free (message);
      gtk_container_add (GTK_CONTAINER (content_area), label);

      checkbox = gtk_check_button_new_with_mnemonic (_("Do _not ask me again"));
      gtk_container_add (GTK_CONTAINER (content_area), checkbox);

      g_signal_connect (G_OBJECT (relaunch_bar), "response", G_CALLBACK (relaunch_bar_response), screen);

      gtk_widget_set_halign (relaunch_bar, GTK_ALIGN_FILL);
      gtk_widget_set_valign (relaunch_bar, GTK_ALIGN_START);
      gtk_overlay_add_overlay (GTK_OVERLAY (screen), relaunch_bar);
      gtk_widget_show_all (relaunch_bar);
    }
}



static void
terminal_screen_vte_eof (VteTerminal    *terminal,
                         TerminalScreen *screen)
{
  terminal_return_if_fail (VTE_IS_TERMINAL (terminal));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  if (G_LIKELY (!screen->hold))
    gtk_widget_destroy (GTK_WIDGET (screen));
}



static GtkWidget*
terminal_screen_vte_get_context_menu (TerminalWidget  *widget,
                                      TerminalScreen  *screen)
{
  GtkWidget *menu = NULL;
  g_signal_emit (G_OBJECT (screen), screen_signals[GET_CONTEXT_MENU], 0, &menu);
  return menu;
}



static void
terminal_screen_vte_selection_changed (VteTerminal    *terminal,
                                       TerminalScreen *screen)
{
  gboolean copy_on_select;

  terminal_return_if_fail (VTE_IS_TERMINAL (terminal));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  /* copy vte selection to GDK_SELECTION_CLIPBOARD if option is set */
  g_object_get (G_OBJECT (screen->preferences),
                "misc-copy-on-select", &copy_on_select, NULL);
  if (copy_on_select && vte_terminal_get_has_selection (terminal))
    terminal_screen_copy_clipboard (screen);

  g_signal_emit (G_OBJECT (screen), screen_signals[SELECTION_CHANGED], 0);
}



static void
terminal_screen_vte_window_title_changed (VteTerminal    *terminal,
                                          TerminalScreen *screen)
{
  terminal_return_if_fail (VTE_IS_TERMINAL (terminal));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  terminal_screen_update_title (screen);
}



static void
terminal_screen_vte_resize_window (VteTerminal    *terminal,
                                   guint           width,
                                   guint           height,
                                   TerminalScreen *screen)
{
  GtkWidget *toplevel;

  terminal_return_if_fail (VTE_IS_TERMINAL (terminal));
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  /* don't do anything if the window is already fullscreen/maximized */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (screen));
  if (!gtk_widget_get_realized (toplevel)
      || (gdk_window_get_state (gtk_widget_get_window (toplevel))
          & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN)) != 0)
    return;

  /* set the terminal size and resize the window if it is active */
  vte_terminal_set_size (terminal, width, height);
  if (screen == terminal_window_get_active (TERMINAL_WINDOW (toplevel)))
    terminal_screen_force_resize_window (screen, GTK_WINDOW (toplevel), width, height);
}



static gboolean
terminal_screen_reset_activity_timeout (gpointer user_data)
{
  TerminalScreen *screen = TERMINAL_SCREEN (user_data);
  GdkRGBA         active_color;
  GdkRGBA         fg_color;
  GdkRGBA         label_color;

  if (G_UNLIKELY (screen->tab_label == NULL))
    return FALSE;

  /* unset */
  if (G_LIKELY (screen->custom_title_color == NULL))
    gtk_label_set_attributes (GTK_LABEL (screen->tab_label), NULL);
  else if (gdk_rgba_parse (&label_color, screen->custom_title_color))
    terminal_screen_set_tab_label_color (screen, &label_color);

  if (terminal_preferences_get_color (screen->preferences, "tab-activity-color", &active_color))
    {
      /* calculate color between fg and active color */
      gtk_style_context_get_color (gtk_widget_get_style_context (screen->tab_label),
                                   gtk_widget_get_state_flags (screen->tab_label),
                                   &fg_color);
      active_color.red = (active_color.red + fg_color.red) / 2;
      active_color.green = (active_color.green + fg_color.green) / 2;
      active_color.blue = (active_color.blue + fg_color.blue) / 2;

      terminal_screen_set_tab_label_color (screen, &active_color);
    }

  return FALSE;
}



static void
terminal_screen_reset_activity_destroyed (gpointer user_data)
{
  TERMINAL_SCREEN (user_data)->activity_timeout_id = 0;
}



static void
terminal_screen_vte_window_contents_changed (TerminalScreen *screen)
{
  guint           timeout;
  GdkRGBA         color;
  GdkRGBA         label_color;
  gboolean        has_color;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (GTK_IS_LABEL (screen->tab_label));
  terminal_return_if_fail (TERMINAL_IS_PREFERENCES (screen->preferences));

  /* leave if we should not start an update */
  if (screen->tab_label == NULL
      || (gtk_widget_get_state_flags (screen->terminal) & GTK_STATE_FLAG_FOCUSED) != 0
      || time (NULL) - screen->activity_resize_time <= 1)
    return;

  /* get the reset time, leave if this feature is disabled */
  g_object_get (G_OBJECT (screen->preferences), "tab-activity-timeout", &timeout, NULL);
  if (timeout < 1)
    return;

  /* set label color */
  has_color = terminal_preferences_get_color (screen->preferences, "tab-activity-color", &color);
  if (G_LIKELY (has_color))
    terminal_screen_set_tab_label_color (screen, &color);
  else if (G_LIKELY (screen->custom_title_color == NULL))
    gtk_label_set_attributes (GTK_LABEL (screen->tab_label), NULL);
  else if (gdk_rgba_parse (&label_color, screen->custom_title_color))
    terminal_screen_set_tab_label_color (screen, &label_color);

  /* stop running reset timeout */
  if (screen->activity_timeout_id != 0)
    g_source_remove (screen->activity_timeout_id);

  /* start new timeout to unset the activity */
  screen->activity_timeout_id =
      gdk_threads_add_timeout_seconds_full (G_PRIORITY_DEFAULT, timeout,
                                            terminal_screen_reset_activity_timeout,
                                            screen, terminal_screen_reset_activity_destroyed);
}



static void
terminal_screen_vte_window_contents_resized (TerminalScreen *screen)
{
  /* avoid a content changed when the window is resized */
  screen->activity_resize_time = time (NULL);
}



static void
terminal_screen_update_label_orientation (TerminalScreen *screen)
{
  GtkPositionType     position;
  gdouble             angle;
  PangoEllipsizeMode  ellipsize;
  GtkWidget          *box;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (screen->tab_label == NULL || GTK_IS_LABEL (screen->tab_label));

  if (G_UNLIKELY (screen->tab_label == NULL))
    return;

  g_object_get (G_OBJECT (screen->preferences), "misc-tab-position", &position, NULL);

  if (G_LIKELY (position == GTK_POS_TOP || position == GTK_POS_BOTTOM))
    {
      angle = 0.0;
      ellipsize = PANGO_ELLIPSIZE_END;
      gtk_widget_set_halign (screen->tab_label, GTK_ALIGN_START);
      gtk_widget_set_valign (screen->tab_label, GTK_ALIGN_CENTER);

      /* reset size request, ellipsize works now */
      gtk_widget_set_size_request (screen->tab_label, -1, -1);
    }
  else
    {
      angle = position == GTK_POS_LEFT ? 90.0 : 270.0;
      ellipsize = PANGO_ELLIPSIZE_NONE;
      gtk_widget_set_halign (screen->tab_label, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (screen->tab_label, GTK_ALIGN_START);

      /* set a minimum height of 30px, because ellipsize does not work */
      gtk_widget_set_size_request (screen->tab_label, -1, 30);
    }

  gtk_label_set_angle (GTK_LABEL (screen->tab_label), angle);
  gtk_label_set_ellipsize (GTK_LABEL (screen->tab_label), ellipsize);

  box = gtk_widget_get_parent (screen->tab_label);
  terminal_return_if_fail (GTK_IS_ORIENTABLE (box));
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
    angle == 0.0 ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
}



static void
terminal_screen_urgent_bell (TerminalWidget *widget,
                             TerminalScreen *screen)
{
  gboolean enabled;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  g_object_get (G_OBJECT (screen->preferences), "misc-bell-urgent", &enabled, NULL);

  if (enabled)
    gtk_window_set_urgency_hint (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (screen))), TRUE);
}



static void
terminal_screen_set_custom_command (TerminalScreen *screen,
                                    gchar         **command)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  if (G_UNLIKELY (screen->custom_command != NULL))
    g_strfreev (screen->custom_command);

  if (G_LIKELY (command != NULL && *command != NULL))
    screen->custom_command = g_strdupv (command);
  else
    screen->custom_command = NULL;
}



static void
terminal_screen_set_tab_label_color (TerminalScreen *screen,
                                     const GdkRGBA  *color)
{
  PangoAttrList *attrs = pango_attr_list_new ();
  PangoAttribute *foreground = pango_attr_foreground_new ((guint16)(color->red*65535),
                                                          (guint16)(color->green*65535),
                                                          (guint16)(color->blue*65535));
  pango_attr_list_insert (attrs, foreground);
  gtk_label_set_attributes (GTK_LABEL (screen->tab_label), attrs);
  pango_attr_list_unref (attrs);
}



static gboolean
terminal_screen_is_text_unsafe (const gchar *text)
{
  return text != NULL && strchr (text, '\n') != NULL;
}



static GtkWidget*
terminal_screen_unsafe_paste_dialog_new (TerminalScreen *screen,
                                         const gchar    *text)
{
  GtkWindow     *parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (screen)));
  GtkTextBuffer *buffer = gtk_text_buffer_new (gtk_text_tag_table_new ());
  GtkWidget     *tv = gtk_text_view_new_with_buffer (buffer);
  GtkWidget     *sw = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget     *dialog = xfce_titled_dialog_new ();
  GtkWidget     *infobar = gtk_info_bar_new ();
  GtkWidget     *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  GtkWidget     *button, *label;
  gint           parent_w, parent_h;

  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Warning: Unsafe Paste"));

  label = gtk_label_new (_("Pasting this text to the terminal may be dangerous as it looks like "
                           "some commands may be executed, potentially involving root access ('sudo')."));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (infobar))), label);
  gtk_container_add (GTK_CONTAINER (box), infobar);
  gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 0);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), box);

#if LIBXFCE4UI_CHECK_VERSION (4, 16, 0)
  xfce_titled_dialog_create_action_area (XFCE_TITLED_DIALOG (dialog));
#endif

  button = xfce_gtk_button_new_mixed ("gtk-cancel", _("_Cancel"));
#if LIBXFCE4UI_CHECK_VERSION (4, 16, 0)
  xfce_titled_dialog_add_action_widget (XFCE_TITLED_DIALOG (dialog), button, GTK_RESPONSE_CANCEL);
#else
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_CANCEL);
#endif

  button = xfce_gtk_button_new_mixed ("gtk-ok", _("_Paste"));
#if LIBXFCE4UI_CHECK_VERSION (4, 16, 0)
  xfce_titled_dialog_add_action_widget (XFCE_TITLED_DIALOG (dialog), button, GTK_RESPONSE_YES);
#else
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_YES);
#endif

  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (tv), TRUE);
  gtk_text_view_set_monospace (GTK_TEXT_VIEW (tv), TRUE);
  gtk_text_view_set_top_margin (GTK_TEXT_VIEW (tv), 6);
  gtk_text_view_set_bottom_margin (GTK_TEXT_VIEW (tv), 6);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (tv), 6);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (tv), 6);

  gtk_window_get_size (GTK_WINDOW (parent), &parent_w, &parent_h);
  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               CLAMP (parent_w * 0.7, 300, 1050),
                               CLAMP (parent_h * 0.7, 200, 700));
  gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (sw), 300);
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (sw), 200);
  gtk_widget_set_vexpand (sw, TRUE);

  gtk_container_add (GTK_CONTAINER (sw), tv);
  gtk_container_add (GTK_CONTAINER (box), sw);

  gtk_text_buffer_set_text (buffer, text, -1);

  return dialog;
}



static void
terminal_screen_paste_unsafe_text (TerminalScreen *screen,
                                   const gchar    *text,
                                   GdkAtom         original_clipboard)
{
  GtkWidget *dialog;

  terminal_return_if_fail (original_clipboard != GDK_SELECTION_CLIPBOARD && original_clipboard != GDK_SELECTION_PRIMARY);

  dialog = terminal_screen_unsafe_paste_dialog_new (screen, text);
  gtk_widget_show_all (dialog);
  /* set focus to the Paste button */
  gtk_widget_grab_focus (gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
    {
      GtkWidget     *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
      GtkWidget     *sw = g_list_first (gtk_container_get_children (GTK_CONTAINER (content_area)))->data;
      GtkTextView   *tv = GTK_TEXT_VIEW (gtk_bin_get_child (GTK_BIN (sw)));
      GtkTextBuffer *buffer = gtk_text_view_get_buffer (tv);
      GtkTextIter    start, end;
      char          *res_text;

      gtk_text_buffer_get_bounds (buffer, &start, &end);
      res_text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

      if (res_text != NULL)
        {
          /* Modify the content of the clipboard as required, and then paste it.
           * Using the builtin pasting function enables bracketed paste mode when applicable.
           */
          GtkClipboard *clipboard = gtk_clipboard_get (original_clipboard);
          gtk_clipboard_set_text (clipboard, res_text, strlen (res_text));
          g_free (res_text);

          if (original_clipboard == GDK_SELECTION_CLIPBOARD) {
            vte_terminal_paste_clipboard (VTE_TERMINAL (screen->terminal));
          } else {
            vte_terminal_paste_primary (VTE_TERMINAL (screen->terminal));
          }

          /* restore original clipboard contents */
          gtk_clipboard_set_text (clipboard, text, strlen (text));
        }
    }

  gtk_widget_destroy (dialog);
}



#if VTE_CHECK_VERSION (0, 48, 0)
static void
terminal_screen_spawn_async_cb (VteTerminal *terminal,
                                GPid         pid,
                                GError      *error,
                                gpointer     user_data)
{
  TerminalScreen *screen = TERMINAL_SCREEN (user_data);

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (VTE_IS_TERMINAL (screen->terminal));

  screen->pid = pid;

  if (error)
    {
      xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (screen))),
                              error, _("Failed to execute child"));
    }
#ifdef HAVE_LIBUTEMPTER
  else
    {
      gboolean update_records;
      g_object_get (G_OBJECT (screen->preferences), "command-update-records", &update_records, NULL);
      if (update_records)
        utempter_add_record (vte_pty_get_fd (vte_terminal_get_pty (VTE_TERMINAL (screen->terminal))), NULL);
    }
#endif // HAVE_LIBUTEMPTER
}
#endif



/**
 * terminal_screen_new:
 * @attr    : Terminal attributes.
 * @columns : Columns (width).
 * @rows    : Rows (height).
 *
 * Creates a terminal screen object.
 **/
TerminalScreen *
terminal_screen_new (TerminalTabAttr *attr,
                     glong            columns,
                     glong            rows)
{
  TerminalScreen *screen = g_object_new (TERMINAL_TYPE_SCREEN, NULL);

  if (attr->command != NULL)
    terminal_screen_set_custom_command (screen, attr->command);
  if (attr->directory != NULL)
    terminal_screen_set_working_directory (screen, attr->directory);
  if (attr->title != NULL)
    terminal_screen_set_custom_title (screen, attr->title);
  if (attr->initial_title != NULL)
    screen->initial_title = g_strdup (attr->initial_title);
  screen->dynamic_title_mode = attr->dynamic_title_mode;
  screen->hold = attr->hold;
  vte_terminal_set_size (VTE_TERMINAL (screen->terminal), columns, rows);

  if (attr->color_text != NULL)
    screen->custom_fg_color = g_strdup (attr->color_text);
  if (attr->color_bg != NULL)
    screen->custom_bg_color = g_strdup (attr->color_bg);
  if (attr->color_text != NULL || attr->color_bg != NULL)
    terminal_screen_update_colors (screen);

  return screen;
}



/**
 * terminal_screen_launch_child:
 * @screen  : A #TerminalScreen.
 *
 * Starts the terminal child process.
 **/
void
terminal_screen_launch_child (TerminalScreen *screen)
{
  GError       *error = NULL;
  gchar        *command;
  gchar       **argv;
  gchar       **env;
  gchar       **argv2;
  guint         i, argc;
  VtePtyFlags   pty_flags = VTE_PTY_DEFAULT;
  GSpawnFlags   spawn_flags = G_SPAWN_SEARCH_PATH;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

#ifdef G_ENABLE_DEBUG
  if (!gtk_widget_get_realized (GTK_WIDGET (screen)))
    g_error ("Tried to launch command in a TerminalScreen that is not realized");
#endif

  if (!terminal_screen_get_child_command (screen, &command, &argv, &error))
    {
      /* tell the user that we were unable to execute the command */
      xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (screen))),
                              error, _("Failed to execute child"));
      g_error_free (error);
    }
  else
    {
      env = terminal_screen_get_child_environment (screen);

      argc = argv != NULL ? g_strv_length (argv) : 0;
      argv2 = g_new0 (gchar *, argc + 2);
      argv2[0] = command;

      if (argv != NULL)
        {
          for (i = 0; argv[i] != NULL; i++)
            argv2[i + 1] = argv[i];

          spawn_flags |= G_SPAWN_FILE_AND_ARGV_ZERO;
        }

#if VTE_CHECK_VERSION (0, 48, 0)
      vte_terminal_spawn_async (VTE_TERMINAL (screen->terminal),
                                pty_flags,
                                screen->working_directory, argv2, env,
                                spawn_flags,
                                NULL, NULL,
                                NULL, SPAWN_TIMEOUT,
                                NULL,
                                terminal_screen_spawn_async_cb,
                                screen);
#else
      if (!vte_terminal_spawn_sync (VTE_TERMINAL (screen->terminal),
                                    pty_flags,
                                    screen->working_directory, argv2, env,
                                    spawn_flags,
                                    NULL, NULL,
                                    &screen->pid, NULL, &error))
        {
          xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (screen))),
                                  error, _("Failed to execute child"));
          g_error_free (error);
        }
#ifdef HAVE_LIBUTEMPTER
      else
        {
          gboolean update_records;
          g_object_get (G_OBJECT (screen->preferences), "command-update-records", &update_records, NULL);
          if (update_records)
            utempter_add_record (vte_pty_get_fd (vte_terminal_get_pty (VTE_TERMINAL (screen->terminal))), NULL);
        }
#endif // HAVE_LIBUTEMPTER
#endif

      g_free (argv2);

      g_strfreev (argv);
      g_strfreev (env);
      g_free (command);
    }
}



/**
 * terminal_screen_get_custom_title:
 * @screen  : A #TerminalScreen.
 **/
const gchar *
terminal_screen_get_custom_title (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);

  return screen->custom_title;
}



/**
 * terminal_screen_set_custom_title:
 * @screen  : A #TerminalScreen.
 * @title   : Title string.
 **/
void
terminal_screen_set_custom_title (TerminalScreen *screen,
                                  const gchar    *title)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  if (g_strcmp0 (screen->custom_title, title) != 0)
    {
      g_free (screen->custom_title);
      if (IS_STRING (title))
        screen->custom_title = g_strdup (title);
      else
        screen->custom_title = NULL;
      g_object_notify (G_OBJECT (screen), "custom-title");
      terminal_screen_update_title (screen);
    }
}



/**
 **/
void
terminal_screen_get_size (TerminalScreen *screen,
                          glong          *width_chars,
                          glong          *height_chars)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  if (width_chars)
    *width_chars = vte_terminal_get_column_count (VTE_TERMINAL (screen->terminal));

  if (height_chars)
    *height_chars = vte_terminal_get_row_count (VTE_TERMINAL (screen->terminal));
}



/**
 **/
void
terminal_screen_set_size (TerminalScreen *screen,
                          glong           width_chars,
                          glong           height_chars)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  vte_terminal_set_size (VTE_TERMINAL (screen->terminal),
                         width_chars, height_chars);
}



void
terminal_screen_get_geometry (TerminalScreen *screen,
                              glong          *char_width,
                              glong          *char_height,
                              gint           *xpad,
                              gint           *ypad)
{
  GtkBorder border;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (VTE_IS_TERMINAL (screen->terminal));

  if (char_width != NULL)
    *char_width = vte_terminal_get_char_width (VTE_TERMINAL (screen->terminal));
  if (char_height != NULL)
    *char_height = vte_terminal_get_char_height (VTE_TERMINAL (screen->terminal));

  if (xpad != NULL || ypad != NULL)
    {
      gtk_style_context_get_padding (gtk_widget_get_style_context (screen->terminal),
                                     gtk_widget_get_state_flags (screen->terminal),
                                     &border);
      if (xpad != NULL)
        *xpad = border.left + border.right;
      if (ypad != NULL)
        *ypad = border.top + border.bottom;
    }
}



/**
 * terminal_screen_set_window_geometry_hints:
 *
 * I don't like this way, but its required to work-around a Gtk+
 * bug (maybe also caused by a Vte bug, not sure).
 *
 * Code for GTK > 3.19.5 borrowed from gnome-terminal
 * (terminal_window_update_geometry).
 **/
void
terminal_screen_set_window_geometry_hints (TerminalScreen *screen,
                                           GtkWindow      *window)
{
  GdkGeometry    hints;
  glong          char_width, char_height;
  GtkRequisition vbox_request;
  GtkAllocation  toplevel_allocation, vbox_allocation;
  glong          grid_width, grid_height;
  glong          chrome_width, chrome_height;
  gint           csd_width, csd_height;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (VTE_IS_TERMINAL (screen->terminal));
  terminal_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (screen)));
  terminal_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (window)));

  terminal_screen_get_geometry (screen, &char_width, &char_height, NULL, NULL);
  terminal_screen_get_size (screen, &grid_width, &grid_height);

  gtk_widget_get_preferred_size (terminal_window_get_vbox (TERMINAL_WINDOW (window)), NULL, &vbox_request);
  chrome_width = vbox_request.width - (char_width * grid_width);
  chrome_height = vbox_request.height - (char_height * grid_height);

  gtk_widget_get_allocation (terminal_window_get_vbox (TERMINAL_WINDOW (window)), &vbox_allocation);
  gtk_widget_get_allocation (GTK_WIDGET (window), &toplevel_allocation);
  csd_width = toplevel_allocation.width - vbox_allocation.width;
  csd_height = toplevel_allocation.height - vbox_allocation.height;

  hints.base_width = chrome_width + csd_width;
  hints.base_height = chrome_height + csd_height;

  hints.width_inc = char_width;
  hints.height_inc = char_height;
  hints.min_width = hints.base_width + hints.width_inc * MIN_COLUMNS;
  hints.min_height = hints.base_height + hints.height_inc * MIN_ROWS;

  gtk_window_set_geometry_hints (window,
                                 NULL,
                                 &hints,
                                 GDK_HINT_RESIZE_INC
                                 | GDK_HINT_MIN_SIZE
                                 | GDK_HINT_BASE_SIZE);
}



/**
 * terminal_screen_force_resize_window:
 *
 * I don't like this way, but its required to work-around a Gtk+
 * bug (maybe also caused by a Vte bug, not sure).
 **/
void
terminal_screen_force_resize_window (TerminalScreen *screen,
                                     GtkWindow      *window,
                                     glong           columns,
                                     glong           rows)
{
  GtkRequisition terminal_requisition;
  GtkRequisition vbox_requisition;
  gint           width;
  gint           height;
  gint           xpad, ypad;
  glong          char_width;
  glong          char_height;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (VTE_IS_TERMINAL (screen->terminal));
  terminal_return_if_fail (GTK_IS_WINDOW (window));

  terminal_screen_set_window_geometry_hints (screen, window);

  gtk_widget_get_preferred_size (terminal_window_get_vbox (TERMINAL_WINDOW (window)), NULL, &vbox_requisition);
  gtk_widget_get_preferred_size (screen->terminal, NULL, &terminal_requisition);

  if (columns < 1)
    columns = vte_terminal_get_column_count (VTE_TERMINAL (screen->terminal));
  if (rows < 1)
    rows = vte_terminal_get_row_count (VTE_TERMINAL (screen->terminal));

  terminal_screen_get_geometry (screen,
                                &char_width, &char_height,
                                &xpad, &ypad);

  width = vbox_requisition.width - terminal_requisition.width;
  if (width < 0)
    width = 0;
  width += xpad + char_width * columns;

  height = vbox_requisition.height - terminal_requisition.height;
  if (height < 0)
    height = 0;
  height += ypad + char_height * rows;

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    gtk_window_resize (window, width, height);
  else
    gtk_window_set_default_size (window, width, height);
}



/**
 * terminal_screen_get_title:
 * @screen      : A #TerminalScreen.
 *
 * Return value : The title to set for this terminal screen.
 *                The returned string should be freed when
 *                no longer needed.
 **/
gchar*
terminal_screen_get_title (TerminalScreen *screen)
{
  TerminalTitle  mode;
  const gchar   *vte_title;
  gchar         *initial, *tmp;
  gchar         *title;

  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);

  if (G_UNLIKELY (screen->custom_title != NULL))
    return terminal_screen_parse_title (screen, screen->custom_title);

  vte_title = vte_terminal_get_window_title (VTE_TERMINAL (screen->terminal));

  if (G_UNLIKELY (screen->initial_title != NULL))
    initial = terminal_screen_parse_title (screen, screen->initial_title);
  else
    {
      g_object_get (G_OBJECT (screen->preferences), "title-initial", &tmp, NULL);
      initial = terminal_screen_parse_title (screen, tmp);
      g_free (tmp);
    }

  if (G_UNLIKELY (screen->dynamic_title_mode != TERMINAL_TITLE_DEFAULT))
    mode = screen->dynamic_title_mode;
  else
    g_object_get (G_OBJECT (screen->preferences), "title-mode", &mode, NULL);

  switch (mode)
    {
    case TERMINAL_TITLE_REPLACE:
      if (G_LIKELY (vte_title != NULL))
        title = g_strdup (vte_title);
      else if (initial != NULL)
        return initial;
      else
        title = g_strdup (_("Untitled"));
      break;

    case TERMINAL_TITLE_PREPEND:
      if (G_LIKELY (vte_title != NULL))
        title = g_strconcat (vte_title, " - ", initial, NULL);
      else
        return initial;
      break;

    case TERMINAL_TITLE_APPEND:
      if (G_LIKELY (vte_title != NULL))
        title = g_strconcat (initial, " - ", vte_title, NULL);
      else
        return initial;
      break;

    case TERMINAL_TITLE_HIDE:
      return initial;
      break;

    default:
      terminal_assert_not_reached ();
      title = NULL;
    }

  g_free (initial);

  return title;
}



/**
 * terminal_screen_get_working_directory:
 * @screen      : A #TerminalScreen.
 *
 * Determinies the working directory using various OS-specific mechanisms.
 *
 * Return value : The current working directory of @screen.
 **/
const gchar*
terminal_screen_get_working_directory (TerminalScreen *screen)
{
  gchar        buffer[4096 + 1];
  gchar       *file;
  gchar       *cwd;
  const gchar *uri;
  gint         length;

  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);

  /* try to use vte functionality first: see bug #13902 */
  uri = vte_terminal_get_current_directory_uri (VTE_TERMINAL (screen->terminal));
  if (uri != NULL)
    {
      g_free (screen->working_directory);
      screen->working_directory = g_filename_from_uri (uri, NULL, NULL);
    }
  else if (screen->pid >= 0)
    {
      /* make sure that we use linprocfs on all systems */
#if defined(__FreeBSD__)
      file = g_strdup_printf ("/compat/linux/proc/%d/cwd", screen->pid);
#elif defined(__NetBSD__) || defined(__OpenBSD__)
      file = g_strdup_printf ("/emul/linux/proc/%d/cwd", screen->pid);
#else
      file = g_strdup_printf ("/proc/%d/cwd", screen->pid);
#endif

      length = readlink (file, buffer, sizeof (buffer) - 1);
      if (length > 0 && *buffer == '/')
        {
          buffer[length] = '\0';
          g_free (screen->working_directory);
          screen->working_directory = g_strdup (buffer);
        }
      else if (length == 0)
        {
          cwd = g_get_current_dir ();
          if (G_LIKELY (cwd != NULL))
            {
              if (chdir (file) == 0)
                {
                  g_free (screen->working_directory);
                  screen->working_directory = g_get_current_dir ();
                  if (chdir (cwd) == 0) {};
                }

              g_free (cwd);
            }
        }

      g_free (file);
    }

  return screen->working_directory;
}



/**
 * terminal_screen_set_working_directory:
 * @screen    : A #TerminalScreen.
 * @directory :
 **/
void
terminal_screen_set_working_directory (TerminalScreen *screen,
                                       const gchar    *directory)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (directory != NULL);

  g_free (screen->working_directory);
  screen->working_directory = g_strdup (directory);
}



/**
 * terminal_screen_has_selection:
 * @screen      : A #TerminalScreen.
 *
 * Checks if the terminal currently contains selected text. Note that this is different from
 * determining if the terminal is the owner of any GtkClipboard items.
 *
 * Return value : %TRUE if part of the text in the terminal is selected.
 **/
gboolean
terminal_screen_has_selection (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), FALSE);
  return vte_terminal_get_has_selection (VTE_TERMINAL (screen->terminal));
}



/**
 * terminal_screen_copy_clipboard:
 * @screen  : A #TerminalScreen.
 *
 * Places the selected text in the terminal in the #GDK_SELECTION_CLIPBOARD selection.
 **/
void
terminal_screen_copy_clipboard (TerminalScreen *screen)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
#if VTE_CHECK_VERSION (0, 49, 2)
  vte_terminal_copy_clipboard_format (VTE_TERMINAL (screen->terminal), VTE_FORMAT_TEXT);
#else
  vte_terminal_copy_clipboard (VTE_TERMINAL (screen->terminal));
#endif
}



/**
 * terminal_screen_copy_clipboard_html:
 * @screen  : A #TerminalScreen.
 *
 * Places the selected text in the terminal in the #GDK_SELECTION_CLIPBOARD selection
 * as HTML (preserving colors, bold font, etc).
 **/
#if VTE_CHECK_VERSION (0, 49, 2)
void
terminal_screen_copy_clipboard_html (TerminalScreen *screen)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_copy_clipboard_format (VTE_TERMINAL (screen->terminal), VTE_FORMAT_HTML);
}
#endif



/**
 * terminal_screen_paste_clipboard:
 * @screen  : A #TerminalScreen.
 *
 * Sends the contents of the #GDK_SELECTION_CLIPBOARD selection to the terminal's
 * child. If neccessary, the data is converted from UTF-8 to the terminal's current
 * encoding.
 **/
void
terminal_screen_paste_clipboard (TerminalScreen *screen)
{
  gboolean  show_dialog;
  gchar    *text = gtk_clipboard_wait_for_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  g_object_get (G_OBJECT (screen->preferences), "misc-show-unsafe-paste-dialog", &show_dialog, NULL);

  if (show_dialog && terminal_screen_is_text_unsafe (text))
    terminal_screen_paste_unsafe_text (screen, text, GDK_SELECTION_CLIPBOARD);
  else
    vte_terminal_paste_clipboard (VTE_TERMINAL (screen->terminal));

  g_free (text);
}



/**
 * terminal_screen_paste_primary:
 * @screen : A #TerminalScreen.
 *
 * Sends the contents of the #GDK_SELECTION_PRIMARY selection to the terminal's child.
 * If necessary, the data is converted from UTF-8 to the terminal's current encoding.
 * The terminal will call also paste the #GDK_SELECTION_PRIMARY selection when the user
 * clicks with the the second mouse button.
 **/
void
terminal_screen_paste_primary (TerminalScreen *screen)
{
  gboolean  show_dialog;
  gchar    *text = gtk_clipboard_wait_for_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY));

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  g_object_get (G_OBJECT (screen->preferences), "misc-show-unsafe-paste-dialog", &show_dialog, NULL);

  if (show_dialog && terminal_screen_is_text_unsafe (text))
    terminal_screen_paste_unsafe_text (screen, text, GDK_SELECTION_PRIMARY);
  else
    vte_terminal_paste_primary (VTE_TERMINAL (screen->terminal));

  g_free (text);
}



/**
 * terminal_screen_select_all:
 * @screen : A #TerminalScreen.
 *
 * Selects all text in the terminal.
 **/
void
terminal_screen_select_all (TerminalScreen *screen)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_select_all (VTE_TERMINAL (screen->terminal));
}



/**
 * terminal_screen_reset:
 * @screen  : A #TerminalScreen.
 * @clear   : %TRUE to also clear the terminal screen.
 *
 * Resets the terminal.
 **/
void
terminal_screen_reset (TerminalScreen *screen,
                       gboolean        clear)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_reset (VTE_TERMINAL (screen->terminal), TRUE, clear);

  if (clear)
    vte_terminal_search_set_regex (VTE_TERMINAL (screen->terminal), NULL, 0);
}



/**
 * terminal_screen_get_restart_command:
 * @screen  : A #TerminalScreen.
 *
 * Return value: Command to restore @screen, arguments are in reversed order.
 **/
GSList*
terminal_screen_get_restart_command (TerminalScreen *screen)
{
  const gchar *directory;
  GSList      *result = NULL;

  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);

  if (screen->custom_command != NULL)
    {
      result = g_slist_prepend (result, g_strdup ("-e"));
      result = g_slist_prepend (result, g_strjoinv (" ", screen->custom_command));
    }

  if (screen->custom_title != NULL)
    {
      result = g_slist_prepend (result, g_strdup ("--title"));
      result = g_slist_prepend (result, g_strdup (screen->custom_title));
    }

  directory = terminal_screen_get_working_directory (screen);
  if (G_LIKELY (directory != NULL))
    {
      result = g_slist_prepend (result, g_strdup ("--working-directory"));
      result = g_slist_prepend (result, g_strdup (directory));
    }

  if (G_UNLIKELY (screen->hold))
    result = g_slist_prepend (result, g_strdup ("--hold"));

  return result;
}



void
terminal_screen_reset_activity (TerminalScreen *screen)
{
  GdkRGBA label_color;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  if (screen->activity_timeout_id != 0)
    g_source_remove (screen->activity_timeout_id);

  if (screen->tab_label != NULL)
    {
      if (G_LIKELY (screen->custom_title_color == NULL))
        gtk_label_set_attributes (GTK_LABEL (screen->tab_label), NULL);
      else if (gdk_rgba_parse (&label_color, screen->custom_title_color))
        terminal_screen_set_tab_label_color (screen, &label_color);
    }
}



static void
terminal_screen_close_tab_cb (TerminalScreen *screen)
{
  g_signal_emit (G_OBJECT (screen), screen_signals[CLOSE_TAB], 0);
}



GtkWidget *
terminal_screen_get_tab_label (TerminalScreen *screen)
{
  GtkWidget *hbox, *button, *image;

  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);

  /* create the box */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);

  screen->tab_label = gtk_label_new (NULL);
  gtk_widget_set_margin_start (screen->tab_label, 2);
  gtk_box_pack_start  (GTK_BOX (hbox), screen->tab_label, TRUE, TRUE, 0);
  g_object_bind_property (G_OBJECT (screen), "title",
                          G_OBJECT (screen->tab_label), "label",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (G_OBJECT (screen->tab_label), "label",
                          G_OBJECT (screen->tab_label), "tooltip-text",
                          G_BINDING_SYNC_CREATE);
  gtk_widget_set_has_tooltip (screen->tab_label, TRUE);

  button = gtk_button_new ();
  gtk_widget_set_focus_on_click (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_widget_set_can_focus (button, FALSE);
  gtk_widget_set_can_default (button, FALSE);
  gtk_widget_set_tooltip_text (button, _("Close this tab"));
  gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect_swapped (G_OBJECT (button), "clicked",
                            G_CALLBACK (terminal_screen_close_tab_cb), screen);

  /* button image */
  image = gtk_image_new_from_icon_name ("window-close-symbolic", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);

  /* show the box and all its widgets */
  gtk_widget_show_all (hbox);

  /* update orientation */
  terminal_screen_update_label_orientation (screen);

  /* respect the show/hide buttons option */
  g_object_bind_property (G_OBJECT (screen->preferences), "misc-tab-close-buttons",
                          G_OBJECT (button), "visible",
                          G_BINDING_SYNC_CREATE);

  return hbox;
}



void
terminal_screen_focus (TerminalScreen *screen)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  gtk_widget_grab_focus (GTK_WIDGET (screen->terminal));
}



const gchar *
terminal_screen_get_encoding (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);
  return vte_terminal_get_encoding (VTE_TERMINAL (screen->terminal));
}



void
terminal_screen_set_encoding (TerminalScreen *screen,
                              const gchar    *charset)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  if (charset == NULL)
    g_get_charset (&charset);
  if (!vte_terminal_set_encoding (VTE_TERMINAL (screen->terminal), charset, NULL))
    g_printerr (_("Failed to set encoding %s\n"), charset);
}



void
terminal_screen_search_set_gregex (TerminalScreen *screen,
                                   VteRegex       *regex,
                                   gboolean        wrap_around)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_search_set_regex (VTE_TERMINAL (screen->terminal), regex, 0);
  vte_terminal_search_set_wrap_around (VTE_TERMINAL (screen->terminal), wrap_around);
}



gboolean
terminal_screen_search_has_gregex (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), FALSE);
  return vte_terminal_search_get_regex (VTE_TERMINAL (screen->terminal)) != NULL;
}



void
terminal_screen_search_find_next (TerminalScreen *screen)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_search_find_next (VTE_TERMINAL (screen->terminal));
}



void
terminal_screen_search_find_previous (TerminalScreen *screen)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_search_find_previous (VTE_TERMINAL (screen->terminal));
}



void
terminal_screen_update_scrolling_bar (TerminalScreen *screen)
{
  TerminalScrollbar  scrollbar;
  TerminalVisibility visibility = TERMINAL_VISIBILITY_DEFAULT;
  glong              grid_w = 0, grid_h = 0;
  GtkWidget         *toplevel;

  g_object_get (G_OBJECT (screen->preferences), "scrolling-bar", &scrollbar, NULL);

  if (gtk_widget_get_realized (GTK_WIDGET (screen)))
    terminal_screen_get_size (screen, &grid_w, &grid_h);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (screen));
  if (TERMINAL_IS_WINDOW (toplevel))
    visibility = terminal_window_get_scrollbar_visibility (TERMINAL_WINDOW (toplevel));

  if (G_LIKELY (visibility == TERMINAL_VISIBILITY_DEFAULT))
    {
      switch (scrollbar)
        {
        case TERMINAL_SCROLLBAR_NONE:
          gtk_widget_hide (screen->scrollbar);
          break;

        case TERMINAL_SCROLLBAR_LEFT:
          gtk_box_reorder_child (GTK_BOX (screen->hbox), screen->scrollbar, 0);
          gtk_widget_show (screen->scrollbar);
          break;

        default: /* TERMINAL_SCROLLBAR_RIGHT */
          gtk_box_reorder_child (GTK_BOX (screen->hbox), screen->scrollbar, 1);
          gtk_widget_show (screen->scrollbar);
          break;
        }
    }
  else if (visibility == TERMINAL_VISIBILITY_HIDE)
    {
      gtk_widget_hide (screen->scrollbar);
    }
  else /* show */
    {
      gtk_box_reorder_child (GTK_BOX (screen), screen->scrollbar,
                             scrollbar == TERMINAL_SCROLLBAR_LEFT ? 0 : 1);
      gtk_widget_show (screen->scrollbar);
    }

  /* update window geometry it required */
  if (grid_w > 0 && grid_h > 0)
    terminal_screen_force_resize_window (screen, GTK_WINDOW (toplevel), grid_w, grid_h);
}



void
terminal_screen_update_font (TerminalScreen *screen)
{
  GtkWidget            *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (screen));
  gboolean              font_use_system, font_allow_bold;
  gchar                *font_name = NULL;
  PangoFontDescription *font_desc;
  glong                 grid_w = 0, grid_h = 0;
  GSettings            *settings;
  XfconfChannel        *channel;
  gdouble               font_scale = PANGO_SCALE_MEDIUM;
#if VTE_CHECK_VERSION (0, 51, 3)
  gdouble cell_width_scale, cell_height_scale;
#endif

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  terminal_return_if_fail (TERMINAL_IS_PREFERENCES (screen->preferences));
  terminal_return_if_fail (VTE_IS_TERMINAL (screen->terminal));

  g_object_get (G_OBJECT (screen->preferences),
                "font-use-system", &font_use_system,
                "font-allow-bold", &font_allow_bold,
                NULL);

  if (font_use_system)
    {
      /* read Xfce settings */
      xfconf_init (NULL);
      channel = xfconf_channel_get ("xsettings");
      if (xfconf_channel_has_property (channel, "/Gtk/MonospaceFontName"))
        font_name = xfconf_channel_get_string (channel, "/Gtk/MonospaceFontName", "");
      xfconf_shutdown ();

      /* if font isn't set, read GNOME settings */
      if (!IS_STRING (font_name))
        {
          g_free (font_name);
          settings = g_settings_new ("org.gnome.desktop.interface");
          font_name = g_settings_get_string (settings, "monospace-font-name");
          g_object_unref (settings);
        }
    }
  else
    g_object_get (G_OBJECT (screen->preferences), "font-name", &font_name, NULL);

  if (TERMINAL_IS_WINDOW (toplevel))
    {
      if (terminal_window_get_font (TERMINAL_WINDOW (toplevel)))
        {
          g_free (font_name);
          font_name = g_strdup (terminal_window_get_font (TERMINAL_WINDOW (toplevel)));
        }

      switch (terminal_window_get_zoom_level (TERMINAL_WINDOW (toplevel)))
        {
          case TERMINAL_ZOOM_LEVEL_MINIMUM:     font_scale = PANGO_SCALE_XX_SMALL/1.2/1.2/1.2/1.2; break;
          case TERMINAL_ZOOM_LEVEL_XXXXX_SMALL: font_scale = PANGO_SCALE_XX_SMALL/1.2/1.2/1.2;     break;
          case TERMINAL_ZOOM_LEVEL_XXXX_SMALL:  font_scale = PANGO_SCALE_XX_SMALL/1.2/1.2;         break;
          case TERMINAL_ZOOM_LEVEL_XXX_SMALL:   font_scale = PANGO_SCALE_XX_SMALL/1.2;             break;
          case TERMINAL_ZOOM_LEVEL_XX_SMALL:    font_scale = PANGO_SCALE_XX_SMALL;                 break;
          case TERMINAL_ZOOM_LEVEL_X_SMALL:     font_scale = PANGO_SCALE_X_SMALL;                  break;
          case TERMINAL_ZOOM_LEVEL_SMALL:       font_scale = PANGO_SCALE_SMALL;                    break;
          case TERMINAL_ZOOM_LEVEL_LARGE:       font_scale = PANGO_SCALE_LARGE;                    break;
          case TERMINAL_ZOOM_LEVEL_X_LARGE:     font_scale = PANGO_SCALE_X_LARGE;                  break;
          case TERMINAL_ZOOM_LEVEL_XX_LARGE:    font_scale = PANGO_SCALE_XX_LARGE;                 break;
          case TERMINAL_ZOOM_LEVEL_XXX_LARGE:   font_scale = PANGO_SCALE_XX_LARGE*1.2;             break;
          case TERMINAL_ZOOM_LEVEL_XXXX_LARGE:  font_scale = PANGO_SCALE_XX_LARGE*1.2*1.2;         break;
          case TERMINAL_ZOOM_LEVEL_XXXXX_LARGE: font_scale = PANGO_SCALE_XX_LARGE*1.2*1.2*1.2;     break;
          case TERMINAL_ZOOM_LEVEL_MAXIMUM:     font_scale = PANGO_SCALE_XX_LARGE*1.2*1.2*1.2*1.2; break;
          default:                              font_scale = PANGO_SCALE_MEDIUM;                   break;
        }
    }

  if (gtk_widget_get_realized (GTK_WIDGET (screen)))
    terminal_screen_get_size (screen, &grid_w, &grid_h);

  if (G_LIKELY (font_name != NULL))
    {
      font_desc = pango_font_description_from_string (font_name);
      vte_terminal_set_allow_bold (VTE_TERMINAL (screen->terminal), font_allow_bold);
      vte_terminal_set_font (VTE_TERMINAL (screen->terminal), font_desc);
      vte_terminal_set_font_scale (VTE_TERMINAL (screen->terminal), font_scale);
      pango_font_description_free (font_desc);
      g_free (font_name);
    }

#if VTE_CHECK_VERSION (0, 51, 3)
  g_object_get (G_OBJECT (screen->preferences),
                "cell-width-scale", &cell_width_scale,
                "cell-height-scale", &cell_height_scale,
                NULL);

  vte_terminal_set_cell_width_scale (VTE_TERMINAL (screen->terminal), cell_width_scale);
  vte_terminal_set_cell_height_scale (VTE_TERMINAL (screen->terminal), cell_height_scale);
#endif

  /* update window geometry it required (not needed for drop-down) */
  if (TERMINAL_IS_WINDOW (toplevel) && !terminal_window_is_drop_down (TERMINAL_WINDOW (toplevel)) && grid_w > 0 && grid_h > 0)
    terminal_screen_force_resize_window (screen, GTK_WINDOW (toplevel), grid_w, grid_h);
}



gboolean
terminal_screen_get_input_enabled (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), FALSE);
  return vte_terminal_get_input_enabled (VTE_TERMINAL (screen->terminal));
}



void
terminal_screen_set_input_enabled (TerminalScreen *screen,
                                   gboolean        enabled)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_set_input_enabled (VTE_TERMINAL (screen->terminal), enabled);
}



gboolean
terminal_screen_get_scroll_on_output (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), FALSE);
#if !VTE_CHECK_VERSION (0, 51, 1)
  return screen->scroll_on_output;
#else
  return vte_terminal_get_scroll_on_output (VTE_TERMINAL (screen->terminal));
#endif
}



void
terminal_screen_set_scroll_on_output (TerminalScreen *screen,
                                      gboolean        enabled)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
#if !VTE_CHECK_VERSION (0, 51, 1)
  screen->scroll_on_output = enabled;
#endif
  vte_terminal_set_scroll_on_output (VTE_TERMINAL (screen->terminal), enabled);
}



void
terminal_screen_save_contents (TerminalScreen *screen,
                               GOutputStream  *stream,
                               GError         *error)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_write_contents_sync (VTE_TERMINAL (screen->terminal),
                                    stream, VTE_WRITE_DEFAULT, NULL, &error);
}



/**
 * terminal_screen_has_foreground_process:
 * @screen  : A #TerminalScreen.
 *
 * Return value: %TRUE if there's a foreground process running in @screen.
 **/
gboolean
terminal_screen_has_foreground_process (TerminalScreen *screen)
{
  VtePty *pty;
  int     fd;
  int     fgpid;

  if (screen == NULL || screen->pid == -1)
    return FALSE;

  pty = vte_terminal_get_pty (VTE_TERMINAL (screen->terminal));
  if (pty == NULL)
    return FALSE;

  fd = vte_pty_get_fd (pty);
  if (fd == -1)
    return FALSE;

  fgpid = tcgetpgrp (fd);
  if (fgpid == -1 || fgpid == screen->pid)
    return FALSE;

  return TRUE;
}



void
terminal_screen_feed_text (TerminalScreen *screen,
                           const char     *text)
{
  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));
  vte_terminal_feed_child (VTE_TERMINAL (screen->terminal), text, strlen (text));
}



const gchar *
terminal_screen_get_custom_fg_color (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);
  return screen->custom_fg_color;
}



const gchar *
terminal_screen_get_custom_bg_color (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);
  return screen->custom_bg_color;
}



const gchar *
terminal_screen_get_custom_title_color (TerminalScreen *screen)
{
  terminal_return_val_if_fail (TERMINAL_IS_SCREEN (screen), NULL);
  return screen->custom_title_color;
}



void
terminal_screen_set_custom_title_color (TerminalScreen *screen,
                                        const gchar    *color)
{
  GdkRGBA label_color;

  terminal_return_if_fail (TERMINAL_IS_SCREEN (screen));

  g_free (screen->custom_title_color);
  screen->custom_title_color = NULL;

  if (color == NULL)
    gtk_label_set_attributes (GTK_LABEL (screen->tab_label), NULL);
  else if (gdk_rgba_parse (&label_color, color))
    {
      screen->custom_title_color = g_strdup (color);
      terminal_screen_set_tab_label_color (screen, &label_color);
    }
}
