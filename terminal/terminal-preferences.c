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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-preferences.h>



enum
{
  PROP_0,
  PROP_ACCEL_NEW_TAB,
  PROP_ACCEL_NEW_WINDOW,
  PROP_ACCEL_CLOSE_TAB,
  PROP_ACCEL_CLOSE_WINDOW,
  PROP_ACCEL_COPY,
  PROP_ACCEL_PASTE,
  PROP_ACCEL_PREFERENCES,
  PROP_ACCEL_SHOW_MENUBAR,
  PROP_ACCEL_SHOW_TOOLBARS,
  PROP_ACCEL_SHOW_BORDERS,
  PROP_ACCEL_FULLSCREEN,
  PROP_ACCEL_PREV_TAB,
  PROP_ACCEL_NEXT_TAB,
  PROP_ACCEL_SET_TITLE,
  PROP_ACCEL_RESET,
  PROP_ACCEL_RESET_AND_CLEAR,
  PROP_ACCEL_CONTENTS,
  PROP_BACKGROUND_MODE,
  PROP_BACKGROUND_IMAGE_FILE,
  PROP_BACKGROUND_IMAGE_STYLE,
  PROP_BACKGROUND_DARKNESS,
  PROP_BINDING_BACKSPACE,
  PROP_BINDING_DELETE,
  PROP_COLOR_SYSTEM_THEME,
  PROP_COLOR_FOREGROUND,
  PROP_COLOR_BACKGROUND,
  PROP_COLOR_PALETTE1,
  PROP_COLOR_PALETTE2,
  PROP_COLOR_PALETTE3,
  PROP_COLOR_PALETTE4,
  PROP_COLOR_PALETTE5,
  PROP_COLOR_PALETTE6,
  PROP_COLOR_PALETTE7,
  PROP_COLOR_PALETTE8,
  PROP_COLOR_PALETTE9,
  PROP_COLOR_PALETTE10,
  PROP_COLOR_PALETTE11,
  PROP_COLOR_PALETTE12,
  PROP_COLOR_PALETTE13,
  PROP_COLOR_PALETTE14,
  PROP_COLOR_PALETTE15,
  PROP_COLOR_PALETTE16,
  PROP_COMMAND_UPDATE_RECORDS,
  PROP_COMMAND_LOGIN_SHELL,
  PROP_FONT_ANTI_ALIAS,
  PROP_FONT_NAME,
  PROP_MISC_BELL,
  PROP_MISC_BORDERS_DEFAULT,
  PROP_MISC_CURSOR_BLINKS,
  PROP_MISC_MENUBAR_DEFAULT,
  PROP_MISC_TOOLBARS_DEFAULT,
  PROP_MISC_CONFIRM_CLOSE,
  PROP_SCROLLING_BAR,
  PROP_SCROLLING_LINES,
  PROP_SCROLLING_ON_OUTPUT,
  PROP_SCROLLING_ON_KEYSTROKE,
  PROP_SHORTCUTS_NO_MENUKEY,
  PROP_SHORTCUTS_NO_MNEMONICS,
  PROP_TITLE_INITIAL,
  PROP_TITLE_MODE,
  PROP_TERM,
  PROP_VTE_WORKAROUND_TITLE_BUG,
  PROP_WORD_CHARS,
};

struct _TerminalPreferences
{
  GObject                  __parent__;

  gchar                   *accel_new_tab;
  gchar                   *accel_new_window;
  gchar                   *accel_close_tab;
  gchar                   *accel_close_window;
  gchar                   *accel_copy;
  gchar                   *accel_paste;
  gchar                   *accel_preferences;
  gchar                   *accel_show_menubar;
  gchar                   *accel_show_toolbars;
  gchar                   *accel_show_borders;
  gchar                   *accel_fullscreen;
  gchar                   *accel_prev_tab;
  gchar                   *accel_next_tab;
  gchar                   *accel_set_title;
  gchar                   *accel_reset;
  gchar                   *accel_reset_and_clear;
  gchar                   *accel_contents;
  TerminalBackground       background_mode;
  gchar                   *background_image_file;
  TerminalBackgroundStyle  background_image_style;
  gdouble                  background_darkness;
  TerminalEraseBinding     binding_backspace;
  TerminalEraseBinding     binding_delete;
  gboolean                 color_system_theme;
  GdkColor                 color_foreground;
  GdkColor                 color_background;
  GdkColor                 color_palette1;
  GdkColor                 color_palette2;
  GdkColor                 color_palette3;
  GdkColor                 color_palette4;
  GdkColor                 color_palette5;
  GdkColor                 color_palette6;
  GdkColor                 color_palette7;
  GdkColor                 color_palette8;
  GdkColor                 color_palette9;
  GdkColor                 color_palette10;
  GdkColor                 color_palette11;
  GdkColor                 color_palette12;
  GdkColor                 color_palette13;
  GdkColor                 color_palette14;
  GdkColor                 color_palette15;
  GdkColor                 color_palette16;
  gboolean                 command_update_records;
  gboolean                 command_login_shell;
  gboolean                 font_anti_alias;
  gchar                   *font_name;
  gboolean                 misc_bell;
  gboolean                 misc_borders_default;
  gboolean                 misc_cursor_blinks;
  gboolean                 misc_menubar_default;
  gboolean                 misc_toolbars_default;
  gboolean                 misc_confirm_close;
  TerminalScrollbar        scrolling_bar;
  guint                    scrolling_lines;
  gboolean                 scrolling_on_output;
  gboolean                 scrolling_on_keystroke;
  gboolean                 shortcuts_no_menukey;
  gboolean                 shortcuts_no_mnemonics;
  gchar                   *title_initial;
  TerminalTitle            title_mode;
  gchar                   *term;
  gboolean                 vte_workaround_title_bug;
  gchar                   *word_chars;

  guint                 store_idle_id;
  guint                 loading_in_progress : 1;
};



static void     terminal_preferences_dispose            (GObject             *object);
static void     terminal_preferences_finalize           (GObject             *object);
static void     terminal_preferences_get_property       (GObject             *object,
                                                         guint                prop_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);
static void     terminal_preferences_set_property       (GObject             *object,
                                                         guint                prop_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void     terminal_preferences_load               (TerminalPreferences *preferences);
static void     terminal_preferences_schedule_store     (TerminalPreferences *preferences);
static gboolean terminal_preferences_store_idle         (gpointer             user_data);
static void     terminal_preferences_store_idle_destroy (gpointer             user_data);



static GObjectClass *parent_class;
static TerminalPreferences *default_preferences = NULL;



static void
transform_color_to_string (const GValue *src,
                           GValue       *dst)
{
  GdkColor *color;
  gchar     buffer[32];

  color = g_value_get_boxed (src);
  g_snprintf (buffer, 32, "#%04x%04x%04x",
              (guint) color->red,
              (guint) color->green,
              (guint) color->blue);
  g_value_set_string (dst, buffer);
}



static void
transform_string_to_boolean (const GValue *src,
                             GValue       *dst)
{
  g_value_set_boolean (dst, !exo_str_is_equal (g_value_get_string (src), "FALSE"));
}



static void
transform_string_to_color (const GValue *src,
                           GValue       *dst)
{
  GdkColor color;

  gdk_color_parse (g_value_get_string (src), &color);
  g_value_set_boxed (dst, &color);
}



static void
transform_string_to_double (const GValue *src,
                            GValue       *dst)
{
  const gchar *sval;
  gdouble      dval;
  gchar       *endptr;

  sval = g_value_get_string (src);
  dval = strtod (sval, &endptr);

  if (*endptr != '\0')
    dval = g_ascii_strtod (sval, NULL);

  g_value_set_double (dst, dval);
}



static void
transform_string_to_uint (const GValue *src,
                          GValue       *dst)
{
  g_value_set_uint (dst, strtoul (g_value_get_string (src), NULL, 10));
}



static void
transform_string_to_enum (const GValue *src,
                          GValue       *dst)
{
  GEnumClass *genum_class;
  GEnumValue *genum_value;

  genum_class = g_type_class_peek (G_VALUE_TYPE (dst));
  genum_value = g_enum_get_value_by_name (genum_class, g_value_get_string (src));
  if (G_UNLIKELY (genum_value == NULL))
    genum_value = genum_class->values;
  g_value_set_enum (dst, genum_value->value);
}



G_DEFINE_TYPE (TerminalPreferences, terminal_preferences, G_TYPE_OBJECT);



static void
terminal_preferences_class_init (TerminalPreferencesClass *klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = terminal_preferences_dispose;
  gobject_class->finalize = terminal_preferences_finalize;
  gobject_class->get_property = terminal_preferences_get_property;
  gobject_class->set_property = terminal_preferences_set_property;

  /* register transform functions */
  if (!g_value_type_transformable (GDK_TYPE_COLOR, G_TYPE_STRING))
    g_value_register_transform_func (GDK_TYPE_COLOR, G_TYPE_STRING, transform_color_to_string);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_BOOLEAN))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, transform_string_to_boolean);
  if (!g_value_type_transformable (G_TYPE_STRING, GDK_TYPE_COLOR))
    g_value_register_transform_func (G_TYPE_STRING, GDK_TYPE_COLOR, transform_string_to_color);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_DOUBLE))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE, transform_string_to_double);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_UINT))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT, transform_string_to_uint);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_BACKGROUND_STYLE))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_BACKGROUND_STYLE, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_BACKGROUND))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_BACKGROUND, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_SCROLLBAR))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_SCROLLBAR, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_TITLE))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_TITLE, transform_string_to_enum);
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_ERASE_BINDING))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_ERASE_BINDING, transform_string_to_enum);

  /**
   * TerminalPreferences:accel-new-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_NEW_TAB,
                                   g_param_spec_string ("accel-new-tab",
                                                        _("Open Tab"),
                                                        _("Open Tab"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-new-window:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_NEW_WINDOW,
                                   g_param_spec_string ("accel-new-window",
                                                        _("Open Terminal"),
                                                        _("Open Terminal"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-close-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CLOSE_TAB,
                                   g_param_spec_string ("accel-close-tab",
                                                        _("Close Tab"),
                                                        _("Close Tab"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-close-window:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CLOSE_WINDOW,
                                   g_param_spec_string ("accel-close-window",
                                                        _("Close Window"),
                                                        _("Close Window"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-copy:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_COPY,
                                   g_param_spec_string ("accel-copy",
                                                        _("Copy"),
                                                        _("Copy"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-paste:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PASTE,
                                   g_param_spec_string ("accel-paste",
                                                        _("Paste"),
                                                        _("Paste"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-preferences:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PREFERENCES,
                                   g_param_spec_string ("accel-preferences",
                                                        _("Preferences"),
                                                        _("Preferences"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-menubar:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_MENUBAR,
                                   g_param_spec_string ("accel-show-menubar",
                                                        _("Show menubar"),
                                                        _("Show menubar"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-toolbars:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_TOOLBARS,
                                   g_param_spec_string ("accel-show-toolbars",
                                                        _("Show toolbars"),
                                                        _("Show toolbars"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-borders:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_BORDERS,
                                   g_param_spec_string ("accel-show-borders",
                                                        _("Show borders"),
                                                        _("Show borders"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-fullscreen:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_FULLSCREEN,
                                   g_param_spec_string ("accel-fullscreen",
                                                        _("Fullscreen"),
                                                        _("Fullscreen"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-prev-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PREV_TAB,
                                   g_param_spec_string ("accel-prev-tab",
                                                        _("Previous Tab"),
                                                        _("Previous Tab"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-next-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_NEXT_TAB,
                                   g_param_spec_string ("accel-next-tab",
                                                        _("Next Tab"),
                                                        _("Next Tab"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-set-title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SET_TITLE,
                                   g_param_spec_string ("accel-set-title",
                                                        _("Set Title"),
                                                        _("Set Title"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-reset:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_RESET,
                                   g_param_spec_string ("accel-reset",
                                                        _("Reset"),
                                                        _("Reset"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-reset-and-clear:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_RESET_AND_CLEAR,
                                   g_param_spec_string ("accel-reset-and-clear",
                                                        _("Reset and Clear"),
                                                        _("Reset and Clear"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-contents:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CONTENTS,
                                   g_param_spec_string ("accel-contents",
                                                        _("Contents"),
                                                        _("Contents"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:background-mode:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_MODE,
                                   g_param_spec_enum ("background-mode",
                                                      _("Background mode"),
                                                      _("Background mode"),
                                                      TERMINAL_TYPE_BACKGROUND,
                                                      TERMINAL_BACKGROUND_SOLID,
                                                      G_PARAM_READWRITE));

  /**
   * TerminalPreferences:background-image-file:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_IMAGE_FILE,
                                   g_param_spec_string ("background-image-file",
                                                        _("Background image file"),
                                                        _("Background image file"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:background-image-style:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_IMAGE_STYLE,
                                   g_param_spec_enum ("background-image-style",
                                                      _("Background image style"),
                                                      _("Background image style"),
                                                      TERMINAL_TYPE_BACKGROUND_STYLE,
                                                      TERMINAL_BACKGROUND_STYLE_TILED,
                                                      G_PARAM_READWRITE));

  /**
   * TerminalPreferences:background-darkness:
   *
   * If a background image has been set (either an image file or a transparent background), the
   * terminal will adjust the brightness of the image before drawing the image. To do so, the
   * terminal will create a copy of the background image (or snapshot of the root window) and
   * modify its pixel values.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_DARKNESS,
                                   g_param_spec_double ("background-darkness",
                                                        _("Background darkness"),
                                                        _("Background darkness"),
                                                        0.0, 1.0, 0.5,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:binding-backspace:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BINDING_BACKSPACE,
                                   g_param_spec_enum ("binding-backspace",
                                                      _("Backspace binding"),
                                                      _("Backspace binding"),
                                                      TERMINAL_TYPE_ERASE_BINDING,
                                                      TERMINAL_ERASE_BINDING_ASCII_DELETE,
                                                      G_PARAM_READWRITE));

  /**
   * TerminalPreferences:binding-delete:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_BINDING_DELETE,
                                   g_param_spec_enum ("binding-delete",
                                                      _("Delete binding"),
                                                      _("Delete binding"),
                                                      TERMINAL_TYPE_ERASE_BINDING,
                                                      TERMINAL_ERASE_BINDING_DELETE_SEQUENCE,
                                                      G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-system-theme:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_SYSTEM_THEME,
                                   g_param_spec_boolean ("color-system-theme",
                                                         _("Use colors from system theme"),
                                                         _("Use colors from system theme"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-foreground:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_FOREGROUND,
                                   g_param_spec_boxed ("color-foreground",
                                                       _("Foreground color"),
                                                       _("Terminal foreground color"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-background:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_BACKGROUND,
                                   g_param_spec_boxed ("color-background",
                                                       _("Background color"),
                                                       _("Terminal background color"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette1:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE1,
                                   g_param_spec_boxed ("color-palette1",
                                                       _("Palette entry 1"),
                                                       _("Palette entry 1"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette2:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE2,
                                   g_param_spec_boxed ("color-palette2",
                                                       _("Palette entry 2"),
                                                       _("Palette entry 2"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette3:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE3,
                                   g_param_spec_boxed ("color-palette3",
                                                       _("Palette entry 3"),
                                                       _("Palette entry 3"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette4:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE4,
                                   g_param_spec_boxed ("color-palette4",
                                                       _("Palette entry 4"),
                                                       _("Palette entry 4"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette5:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE5,
                                   g_param_spec_boxed ("color-palette5",
                                                       _("Palette entry 5"),
                                                       _("Palette entry 5"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette6:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE6,
                                   g_param_spec_boxed ("color-palette6",
                                                       _("Palette entry 6"),
                                                       _("Palette entry 6"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette7:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE7,
                                   g_param_spec_boxed ("color-palette7",
                                                       _("Palette entry 7"),
                                                       _("Palette entry 7"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette8:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE8,
                                   g_param_spec_boxed ("color-palette8",
                                                       _("Palette entry 8"),
                                                       _("Palette entry 8"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette9:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE9,
                                   g_param_spec_boxed ("color-palette9",
                                                       _("Palette entry 9"),
                                                       _("Palette entry 9"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette10:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE10,
                                   g_param_spec_boxed ("color-palette10",
                                                       _("Palette entry 10"),
                                                       _("Palette entry 10"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette11:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE11,
                                   g_param_spec_boxed ("color-palette11",
                                                       _("Palette entry 11"),
                                                       _("Palette entry 11"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette12:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE12,
                                   g_param_spec_boxed ("color-palette12",
                                                       _("Palette entry 12"),
                                                       _("Palette entry 12"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette13:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE13,
                                   g_param_spec_boxed ("color-palette13",
                                                       _("Palette entry 13"),
                                                       _("Palette entry 13"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette14:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE14,
                                   g_param_spec_boxed ("color-palette14",
                                                       _("Palette entry 14"),
                                                       _("Palette entry 14"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette15:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE15,
                                   g_param_spec_boxed ("color-palette15",
                                                       _("Palette entry 15"),
                                                       _("Palette entry 15"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette16:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE16,
                                   g_param_spec_boxed ("color-palette16",
                                                       _("Palette entry 16"),
                                                       _("Palette entry 16"),
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  /**
   * TerminalPreferences:command-update-records:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COMMAND_UPDATE_RECORDS,
                                   g_param_spec_boolean ("command-update-records",
                                                         _("Update records"),
                                                         _("Update wtmp/utmp/lastlog records"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:command-login-shell:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COMMAND_LOGIN_SHELL,
                                   g_param_spec_boolean ("command-login-shell",
                                                         _("Run shell as login shell"),
                                                         _("Run shell as login shell"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:font-anti-alias:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FONT_ANTI_ALIAS,
                                   g_param_spec_boolean ("font-anti-alias",
                                                         _("Font anti-aliasing"),
                                                         _("Font anti-aliasing"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:font-name:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FONT_NAME,
                                   g_param_spec_string ("font-name",
                                                        _("Font name"),
                                                        _("Terminal font name"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-bell:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_BELL,
                                   g_param_spec_boolean ("misc-bell",
                                                         _("Terminal bell"),
                                                         _("Terminal bell"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-borders-default:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_BORDERS_DEFAULT,
                                   g_param_spec_boolean ("misc-borders-default",
                                                         _("Show window borders by default"),
                                                         _("Show window borders by default"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-cursor-blinks:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_CURSOR_BLINKS,
                                   g_param_spec_boolean ("misc-cursor-blinks",
                                                         _("Cursor blinks"),
                                                         _("Cursor blinks"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-menubar-default:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_MENUBAR_DEFAULT,
                                   g_param_spec_boolean ("misc-menubar-default",
                                                         _("Show menubar by default"),
                                                         _("Show menubar by default"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-toolbars-default:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_TOOLBARS_DEFAULT,
                                   g_param_spec_boolean ("misc-toolbars-default",
                                                         _("Show toolbars by default"),
                                                         _("Show toolbars by default"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:misc-confirm-close:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MISC_CONFIRM_CLOSE,
                                   g_param_spec_boolean ("misc-confirm-close",
                                                         _("Confirm closing a window with multiple tabs"),
                                                         _("Confirm closing a window with multiple tabs"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:scrolling-bar:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLLING_BAR,
                                   g_param_spec_enum ("scrolling-bar",
                                                      _("Scrollbar setting"),
                                                      _("Whether and where to display a scrollbar"),
                                                      TERMINAL_TYPE_SCROLLBAR,
                                                      TERMINAL_SCROLLBAR_RIGHT,
                                                      G_PARAM_READWRITE));

  /**
   * TerminalPreferences:scrolling-lines:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLLING_LINES,
                                   g_param_spec_uint ("scrolling-lines",
                                                      _("Scrollback lines"),
                                                      _("Number of lines to keep in history"),
                                                      0u, 1024u * 1024u, 1000u,
                                                      G_PARAM_READWRITE));

  /**
   * TerminalPreferences:scrolling-on-output:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLLING_ON_OUTPUT,
                                   g_param_spec_boolean ("scrolling-on-output",
                                                         _("Scroll on output"),
                                                         _("Scroll on output"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:scrolling-on-keystroke:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SCROLLING_ON_KEYSTROKE,
                                   g_param_spec_boolean ("scrolling-on-keystroke",
                                                         _("Scroll on keystroke"),
                                                         _("Scroll on keystroke"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:shortcuts-no-menukey:
   *
   * Disable menu shortcut key (F10 by default).
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SHORTCUTS_NO_MENUKEY,
                                   g_param_spec_boolean ("shortcuts-no-menukey",
                                                         _("Disable menu shortcut key"),
                                                         _("Disable menu shortcut key"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:shortcuts-no-mnemonics:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SHORTCUTS_NO_MNEMONICS,
                                   g_param_spec_boolean ("shortcuts-no-mnemonics",
                                                         _("Disable all mnemonics"),
                                                         _("Disable all mnemonics"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:title-initial:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE_INITIAL,
                                   g_param_spec_string ("title-initial",
                                                        _("Initial title"),
                                                        _("Initial Terminal title"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:title-mode:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE_MODE,
                                   g_param_spec_enum ("title-mode",
                                                      _("Title mode"),
                                                      _("Dynamic-title mode"),
                                                      TERMINAL_TYPE_TITLE,
                                                      TERMINAL_TITLE_APPEND,
                                                      G_PARAM_READWRITE));

  /**
   * TerminalPreferences:term:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TERM,
                                   g_param_spec_string ("term",
                                                        _("Term"),
                                                        _("Term"),
                                                        "xterm",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:vte-workaround-title-bug:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_VTE_WORKAROUND_TITLE_BUG,
                                   g_param_spec_boolean ("vte-workaround-title-bug",
                                                         _("Workaround VTE title bug"),
                                                         _("Workaround VTE title bug"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * TerminalPreferences:word-chars:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_WORD_CHARS,
                                   g_param_spec_string ("word-chars",
                                                        _("Word chars"),
                                                        _("Word characters"),
                                                        NULL,
                                                        G_PARAM_READWRITE));
}



static void
terminal_preferences_init (TerminalPreferences *preferences)
{
  preferences->accel_new_tab          = g_strdup ("<control><shift>t");
  preferences->accel_new_window       = g_strdup ("<control><shift>n");
  preferences->accel_close_tab        = g_strdup ("<control><shift>w");
  preferences->accel_close_window     = g_strdup ("<control><shift>q");
  preferences->accel_copy             = g_strdup ("<control><shift>c");
  preferences->accel_paste            = g_strdup ("<control><shift>p");
  preferences->accel_preferences      = g_strdup (_("Disabled"));
  preferences->accel_show_menubar     = g_strdup (_("Disabled"));
  preferences->accel_show_toolbars    = g_strdup (_("Disabled"));
  preferences->accel_show_borders     = g_strdup (_("Disabled"));
  preferences->accel_fullscreen       = g_strdup ("F11");
  preferences->accel_prev_tab         = g_strdup ("<control>Page_Up");
  preferences->accel_next_tab         = g_strdup ("<control>Page_Down");
  preferences->accel_set_title        = g_strdup (_("Disabled"));
  preferences->accel_reset            = g_strdup (_("Disabled"));
  preferences->accel_reset_and_clear  = g_strdup (_("Disabled"));
  preferences->accel_contents         = g_strdup ("F1");

  preferences->background_mode        = TERMINAL_BACKGROUND_SOLID;
  preferences->background_image_file  = g_strdup ("");
  preferences->background_image_style = TERMINAL_BACKGROUND_STYLE_TILED;
  preferences->background_darkness    = 0.5;

  preferences->binding_backspace      = TERMINAL_ERASE_BINDING_ASCII_DELETE;
  preferences->binding_delete         = TERMINAL_ERASE_BINDING_DELETE_SEQUENCE;

  preferences->color_system_theme     = FALSE;
  gdk_color_parse ("White", &preferences->color_foreground);
  gdk_color_parse ("Black", &preferences->color_background);
  gdk_color_parse ("#000000000000", &preferences->color_palette1);
  gdk_color_parse ("#aaaa00000000", &preferences->color_palette2);
  gdk_color_parse ("#0000aaaa0000", &preferences->color_palette3);
  gdk_color_parse ("#aaaa55550000", &preferences->color_palette4);
  gdk_color_parse ("#00000000aaaa", &preferences->color_palette5);
  gdk_color_parse ("#aaaa0000aaaa", &preferences->color_palette6);
  gdk_color_parse ("#0000aaaaaaaa", &preferences->color_palette7);
  gdk_color_parse ("#aaaaaaaaaaaa", &preferences->color_palette8);
  gdk_color_parse ("#555555555555", &preferences->color_palette9);
  gdk_color_parse ("#ffff55555555", &preferences->color_palette10);
  gdk_color_parse ("#5555ffff5555", &preferences->color_palette11);
  gdk_color_parse ("#ffffffff5555", &preferences->color_palette12);
  gdk_color_parse ("#55555555ffff", &preferences->color_palette13);
  gdk_color_parse ("#ffff5555ffff", &preferences->color_palette14);
  gdk_color_parse ("#5555ffffffff", &preferences->color_palette15);
  gdk_color_parse ("#ffffffffffff", &preferences->color_palette16);

  preferences->command_update_records   = TRUE;
  preferences->command_login_shell      = FALSE;

  preferences->misc_bell                = FALSE;
  preferences->misc_borders_default     = TRUE;
  preferences->misc_cursor_blinks       = FALSE;
  preferences->misc_menubar_default     = TRUE;
  preferences->misc_toolbars_default    = FALSE;
  preferences->misc_confirm_close       = TRUE;

  preferences->font_anti_alias          = TRUE;
  preferences->font_name                = g_strdup ("Monospace 12");

  preferences->scrolling_bar            = TERMINAL_SCROLLBAR_RIGHT;
  preferences->scrolling_lines          = 1000;
  preferences->scrolling_on_output      = TRUE;
  preferences->scrolling_on_keystroke   = TRUE;

  preferences->shortcuts_no_menukey     = FALSE;
  preferences->shortcuts_no_mnemonics   = FALSE;

  preferences->title_initial            = g_strdup (_("Terminal"));
  preferences->title_mode               = TERMINAL_TITLE_APPEND;

  preferences->term                     = g_strdup ("xterm");

  preferences->vte_workaround_title_bug = TRUE;

  preferences->word_chars               = g_strdup ("-A-Za-z0-9,./?%&#:_");

  terminal_preferences_load (preferences);
}



static void
terminal_preferences_dispose (GObject *object)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);

  /* flush preferences */
  if (G_UNLIKELY (preferences->store_idle_id != 0))
    {
      terminal_preferences_store_idle (preferences);
      g_source_remove (preferences->store_idle_id);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}



static void
terminal_preferences_finalize (GObject *object)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);

  g_free (preferences->accel_new_tab);
  g_free (preferences->accel_new_window);
  g_free (preferences->accel_close_tab);
  g_free (preferences->accel_close_window);
  g_free (preferences->accel_copy);
  g_free (preferences->accel_paste);
  g_free (preferences->accel_preferences);
  g_free (preferences->accel_show_menubar);
  g_free (preferences->accel_show_toolbars);
  g_free (preferences->accel_show_borders);
  g_free (preferences->accel_fullscreen);
  g_free (preferences->accel_prev_tab);
  g_free (preferences->accel_next_tab);
  g_free (preferences->accel_set_title);
  g_free (preferences->accel_reset);
  g_free (preferences->accel_reset_and_clear);
  g_free (preferences->accel_contents);
  g_free (preferences->background_image_file);
  g_free (preferences->font_name);
  g_free (preferences->title_initial);
  g_free (preferences->word_chars);

  parent_class->finalize (object);
}



static void
terminal_preferences_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);

  switch (prop_id)
    {
    case PROP_ACCEL_NEW_TAB:
      g_value_set_string (value, preferences->accel_new_tab);
      break;

    case PROP_ACCEL_NEW_WINDOW:
      g_value_set_string (value, preferences->accel_new_window);
      break;

    case PROP_ACCEL_CLOSE_TAB:
      g_value_set_string (value, preferences->accel_close_tab);
      break;

    case PROP_ACCEL_CLOSE_WINDOW:
      g_value_set_string (value, preferences->accel_close_window);
      break;

    case PROP_ACCEL_COPY:
      g_value_set_string (value, preferences->accel_copy);
      break;

    case PROP_ACCEL_PASTE:
      g_value_set_string (value, preferences->accel_paste);
      break;

    case PROP_ACCEL_PREFERENCES:
      g_value_set_string (value, preferences->accel_preferences);
      break;

    case PROP_ACCEL_SHOW_MENUBAR:
      g_value_set_string (value, preferences->accel_show_menubar);
      break;

    case PROP_ACCEL_SHOW_TOOLBARS:
      g_value_set_string (value, preferences->accel_show_toolbars);
      break;

    case PROP_ACCEL_SHOW_BORDERS:
      g_value_set_string (value, preferences->accel_show_borders);
      break;

    case PROP_ACCEL_FULLSCREEN:
      g_value_set_string (value, preferences->accel_fullscreen);
      break;

    case PROP_ACCEL_PREV_TAB:
      g_value_set_string (value, preferences->accel_prev_tab);
      break;

    case PROP_ACCEL_NEXT_TAB:
      g_value_set_string (value, preferences->accel_next_tab);
      break;

    case PROP_ACCEL_SET_TITLE:
      g_value_set_string (value, preferences->accel_set_title);
      break;

    case PROP_ACCEL_RESET:
      g_value_set_string (value, preferences->accel_reset);
      break;

    case PROP_ACCEL_RESET_AND_CLEAR:
      g_value_set_string (value, preferences->accel_reset_and_clear);
      break;

    case PROP_ACCEL_CONTENTS:
      g_value_set_string (value, preferences->accel_contents);
      break;

    case PROP_BACKGROUND_MODE:
      g_value_set_enum (value, preferences->background_mode);
      break;

    case PROP_BACKGROUND_IMAGE_FILE:
      g_value_set_string (value, preferences->background_image_file);
      break;

    case PROP_BACKGROUND_IMAGE_STYLE:
      g_value_set_enum (value, preferences->background_image_style);
      break;

    case PROP_BACKGROUND_DARKNESS:
      g_value_set_double (value, preferences->background_darkness);
      break;

    case PROP_BINDING_BACKSPACE:
      g_value_set_enum (value, preferences->binding_backspace);
      break;

    case PROP_BINDING_DELETE:
      g_value_set_enum (value, preferences->binding_delete);
      break;

    case PROP_COLOR_SYSTEM_THEME:
      g_value_set_boolean (value, preferences->color_system_theme);
      break;

    case PROP_COLOR_FOREGROUND:
      g_value_set_boxed (value, &preferences->color_foreground);
      break;

    case PROP_COLOR_PALETTE1:
      g_value_set_boxed (value, &preferences->color_palette1);
      break;

    case PROP_COLOR_PALETTE2:
      g_value_set_boxed (value, &preferences->color_palette2);
      break;

    case PROP_COLOR_PALETTE3:
      g_value_set_boxed (value, &preferences->color_palette3);
      break;

    case PROP_COLOR_PALETTE4:
      g_value_set_boxed (value, &preferences->color_palette4);
      break;

    case PROP_COLOR_PALETTE5:
      g_value_set_boxed (value, &preferences->color_palette5);
      break;

    case PROP_COLOR_PALETTE6:
      g_value_set_boxed (value, &preferences->color_palette6);
      break;

    case PROP_COLOR_PALETTE7:
      g_value_set_boxed (value, &preferences->color_palette7);
      break;

    case PROP_COLOR_PALETTE8:
      g_value_set_boxed (value, &preferences->color_palette8);
      break;

    case PROP_COLOR_PALETTE9:
      g_value_set_boxed (value, &preferences->color_palette9);
      break;

    case PROP_COLOR_PALETTE10:
      g_value_set_boxed (value, &preferences->color_palette10);
      break;

    case PROP_COLOR_PALETTE11:
      g_value_set_boxed (value, &preferences->color_palette11);
      break;

    case PROP_COLOR_PALETTE12:
      g_value_set_boxed (value, &preferences->color_palette12);
      break;

    case PROP_COLOR_PALETTE13:
      g_value_set_boxed (value, &preferences->color_palette13);
      break;

    case PROP_COLOR_PALETTE14:
      g_value_set_boxed (value, &preferences->color_palette14);
      break;

    case PROP_COLOR_PALETTE15:
      g_value_set_boxed (value, &preferences->color_palette15);
      break;

    case PROP_COLOR_PALETTE16:
      g_value_set_boxed (value, &preferences->color_palette16);
      break;

    case PROP_COLOR_BACKGROUND:
      g_value_set_boxed (value, &preferences->color_background);
      break;

    case PROP_COMMAND_UPDATE_RECORDS:
      g_value_set_boolean (value, preferences->command_update_records);
      break;

    case PROP_COMMAND_LOGIN_SHELL:
      g_value_set_boolean (value, preferences->command_login_shell);
      break;

    case PROP_FONT_ANTI_ALIAS:
      g_value_set_boolean (value, preferences->font_anti_alias);
      break;

    case PROP_FONT_NAME:
      g_value_set_string (value, preferences->font_name);
      break;

    case PROP_MISC_BELL:
      g_value_set_boolean (value, preferences->misc_bell);
      break;

    case PROP_MISC_BORDERS_DEFAULT:
      g_value_set_boolean (value, preferences->misc_borders_default);
      break;

    case PROP_MISC_CURSOR_BLINKS:
      g_value_set_boolean (value, preferences->misc_cursor_blinks);
      break;

    case PROP_MISC_MENUBAR_DEFAULT:
      g_value_set_boolean (value, preferences->misc_menubar_default);
      break;

    case PROP_MISC_TOOLBARS_DEFAULT:
      g_value_set_boolean (value, preferences->misc_toolbars_default);
      break;

    case PROP_MISC_CONFIRM_CLOSE:
      g_value_set_boolean (value, preferences->misc_confirm_close);
      break;

    case PROP_SCROLLING_BAR:
      g_value_set_enum (value, preferences->scrolling_bar);
      break;

    case PROP_SCROLLING_LINES:
      g_value_set_uint (value, preferences->scrolling_lines);
      break;

    case PROP_SCROLLING_ON_OUTPUT:
      g_value_set_boolean (value, preferences->scrolling_on_output);
      break;

    case PROP_SCROLLING_ON_KEYSTROKE:
      g_value_set_boolean (value, preferences->scrolling_on_keystroke);
      break;

    case PROP_SHORTCUTS_NO_MENUKEY:
      g_value_set_boolean (value, preferences->shortcuts_no_menukey);
      break;

    case PROP_SHORTCUTS_NO_MNEMONICS:
      g_value_set_boolean (value, preferences->shortcuts_no_mnemonics);
      break;

    case PROP_TITLE_INITIAL:
      g_value_set_string (value, preferences->title_initial);
      break;

    case PROP_TITLE_MODE:
      g_value_set_enum (value, preferences->title_mode);
      break;

    case PROP_TERM:
      g_value_set_string (value, preferences->term);
      break;

    case PROP_VTE_WORKAROUND_TITLE_BUG:
      g_value_set_boolean (value, preferences->vte_workaround_title_bug);
      break;

    case PROP_WORD_CHARS:
      g_value_set_string (value, preferences->word_chars);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
terminal_preferences_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  const gchar         *sval;
  GdkColor            *color;
  gboolean             bval;
  gdouble              dval;
  guint                uval;
  gint                 ival;

  switch (prop_id)
    {
    case PROP_ACCEL_NEW_TAB:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_new_tab))
        {
          g_free (preferences->accel_new_tab);
          preferences->accel_new_tab = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-new-tab");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_NEW_WINDOW:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_new_window))
        {
          g_free (preferences->accel_new_window);
          preferences->accel_new_window = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-new-window");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_CLOSE_TAB:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_close_tab))
        {
          g_free (preferences->accel_close_tab);
          preferences->accel_close_tab = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-close-tab");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_CLOSE_WINDOW:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_close_window))
        {
          g_free (preferences->accel_close_window);
          preferences->accel_close_window = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-close-window");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_COPY:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_copy))
        {
          g_free (preferences->accel_copy);
          preferences->accel_copy = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-copy");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_PASTE:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_paste))
        {
          g_free (preferences->accel_paste);
          preferences->accel_paste = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-paste");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_PREFERENCES:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_preferences))
        {
          g_free (preferences->accel_preferences);
          preferences->accel_preferences = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-preferences");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_SHOW_MENUBAR:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_show_menubar))
        {
          g_free (preferences->accel_show_menubar);
          preferences->accel_show_menubar = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-show-menubar");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_SHOW_TOOLBARS:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_show_toolbars))
        {
          g_free (preferences->accel_show_toolbars);
          preferences->accel_show_toolbars = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-show-toolbars");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_SHOW_BORDERS:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_show_borders))
        {
          g_free (preferences->accel_show_borders);
          preferences->accel_show_borders = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-show-borders");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_FULLSCREEN:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_fullscreen))
        {
          g_free (preferences->accel_fullscreen);
          preferences->accel_fullscreen = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-fullscreen");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_PREV_TAB:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_prev_tab))
        {
          g_free (preferences->accel_prev_tab);
          preferences->accel_prev_tab = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-prev-tab");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_NEXT_TAB:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_next_tab))
        {
          g_free (preferences->accel_next_tab);
          preferences->accel_next_tab = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-next-tab");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_SET_TITLE:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_set_title))
        {
          g_free (preferences->accel_set_title);
          preferences->accel_set_title = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-set-title");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_RESET:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_reset))
        {
          g_free (preferences->accel_reset);
          preferences->accel_reset = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-reset");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_RESET_AND_CLEAR:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_reset_and_clear))
        {
          g_free (preferences->accel_reset_and_clear);
          preferences->accel_reset_and_clear = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-reset-and-clear");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_ACCEL_CONTENTS:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->accel_contents))
        {
          g_free (preferences->accel_contents);
          preferences->accel_contents = (sval != NULL) ? g_strdup (sval) : g_strdup (_("Disabled"));
          g_object_notify (object, "accel-contents");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_BACKGROUND_MODE:
      ival = g_value_get_enum (value);
      if (ival != preferences->background_mode)
        {
          preferences->background_mode = ival;
          g_object_notify (object, "background-mode");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_BACKGROUND_IMAGE_FILE:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->background_image_file))
        {
          g_free (preferences->background_image_file);
          preferences->background_image_file = g_strdup (sval);
          g_object_notify (object, "background-image-file");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_BACKGROUND_IMAGE_STYLE:
      ival = g_value_get_enum (value);
      if (ival != preferences->background_image_style)
        {
          preferences->background_image_style = ival;
          g_object_notify (object, "background-image-style");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_BACKGROUND_DARKNESS:
      dval = g_value_get_double (value);
      if (dval != preferences->background_darkness)
        {
          preferences->background_darkness = dval;
          g_object_notify (object, "background-darkness");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_BINDING_BACKSPACE:
      ival = g_value_get_enum (value);
      if (ival != preferences->binding_backspace)
        {
          preferences->binding_backspace = ival;
          g_object_notify (object, "binding-backspace");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_BINDING_DELETE:
      ival = g_value_get_enum (value);
      if (ival != preferences->binding_delete)
        {
          preferences->binding_delete = ival;
          g_object_notify (object, "binding-delete");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_SYSTEM_THEME:
      bval = g_value_get_boolean (value);
      if (bval != preferences->color_system_theme)
        {
          preferences->color_system_theme = bval;
          g_object_notify (object, "color-system-theme");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_FOREGROUND:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_foreground))
        {
          preferences->color_foreground = *color;
          g_object_notify (object, "color-foreground");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_BACKGROUND:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_background))
        {
          preferences->color_background = *color;
          g_object_notify (object, "color-background");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE1:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette1))
        {
          preferences->color_palette1 = *color;
          g_object_notify (object, "color-palette1");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE2:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette2))
        {
          preferences->color_palette2 = *color;
          g_object_notify (object, "color-palette2");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE3:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette3))
        {
          preferences->color_palette3 = *color;
          g_object_notify (object, "color-palette3");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE4:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette4))
        {
          preferences->color_palette4 = *color;
          g_object_notify (object, "color-palette4");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE5:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette5))
        {
          preferences->color_palette5 = *color;
          g_object_notify (object, "color-palette5");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE6:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette6))
        {
          preferences->color_palette6 = *color;
          g_object_notify (object, "color-palette6");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE7:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette7))
        {
          preferences->color_palette7 = *color;
          g_object_notify (object, "color-palette7");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE8:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette8))
        {
          preferences->color_palette8 = *color;
          g_object_notify (object, "color-palette8");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE9:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette9))
        {
          preferences->color_palette9 = *color;
          g_object_notify (object, "color-palette9");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE10:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette10))
        {
          preferences->color_palette10 = *color;
          g_object_notify (object, "color-palette10");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE11:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette11))
        {
          preferences->color_palette11 = *color;
          g_object_notify (object, "color-palette11");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE12:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette12))
        {
          preferences->color_palette12 = *color;
          g_object_notify (object, "color-palette12");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE13:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette13))
        {
          preferences->color_palette13 = *color;
          g_object_notify (object, "color-palette13");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE14:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette14))
        {
          preferences->color_palette14 = *color;
          g_object_notify (object, "color-palette14");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE15:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette15))
        {
          preferences->color_palette15 = *color;
          g_object_notify (object, "color-palette15");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COLOR_PALETTE16:
      color = g_value_get_boxed (value);
      if (!gdk_color_equal (color, &preferences->color_palette16))
        {
          preferences->color_palette16 = *color;
          g_object_notify (object, "color-palette16");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COMMAND_UPDATE_RECORDS:
      bval = g_value_get_boolean (value);
      if (bval != preferences->command_update_records)
        {
          preferences->command_update_records = bval;
          g_object_notify (object, "command-update-records");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_COMMAND_LOGIN_SHELL:
      bval = g_value_get_boolean (value);
      if (bval != preferences->command_login_shell)
        {
          preferences->command_login_shell = bval;
          g_object_notify (object, "command-login-shell");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_FONT_ANTI_ALIAS:
      bval = g_value_get_boolean (value);
      if (bval != preferences->font_anti_alias)
        {
          preferences->font_anti_alias = bval;
          g_object_notify (object, "font-anti-alias");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_FONT_NAME:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->font_name))
        {
          g_free (preferences->font_name);
          preferences->font_name = (sval != NULL) ? g_strdup (sval) : NULL;
          g_object_notify (object, "font-name");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_MISC_BELL:
      bval = g_value_get_boolean (value);
      if (bval != preferences->misc_bell)
        {
          preferences->misc_bell = bval;
          g_object_notify (object, "misc-bell");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_MISC_BORDERS_DEFAULT:
      bval = g_value_get_boolean (value);
      if (bval != preferences->misc_borders_default)
        {
          preferences->misc_borders_default = bval;
          g_object_notify (object, "misc-borders-default");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_MISC_CURSOR_BLINKS:
      bval = g_value_get_boolean (value);
      if (bval != preferences->misc_cursor_blinks)
        {
          preferences->misc_cursor_blinks = bval;
          g_object_notify (object, "misc-cursor-blinks");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_MISC_MENUBAR_DEFAULT:
      bval = g_value_get_boolean (value);
      if (bval != preferences->misc_menubar_default)
        {
          preferences->misc_menubar_default = bval;
          g_object_notify (object, "misc-menubar-default");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_MISC_TOOLBARS_DEFAULT:
      bval = g_value_get_boolean (value);
      if (bval != preferences->misc_toolbars_default)
        {
          preferences->misc_toolbars_default = bval;
          g_object_notify (object, "misc-toolbars-default");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_MISC_CONFIRM_CLOSE:
      bval = g_value_get_boolean (value);
      if (bval != preferences->misc_confirm_close)
        {
          preferences->misc_confirm_close = bval;
          g_object_notify (object, "misc-confirm-close");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_SCROLLING_BAR:
      ival = g_value_get_enum (value);
      if (ival != preferences->scrolling_bar)
        {
          preferences->scrolling_bar = ival;
          g_object_notify (object, "scrolling-bar");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_SCROLLING_LINES:
      uval = g_value_get_uint (value);
      if (uval != preferences->scrolling_lines)
        {
          preferences->scrolling_lines = uval;
          g_object_notify (object, "scrolling-lines");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_SCROLLING_ON_OUTPUT:
      bval = g_value_get_boolean (value);
      if (bval != preferences->scrolling_on_output)
        {
          preferences->scrolling_on_output = bval;
          g_object_notify (object, "scrolling-on-output");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_SCROLLING_ON_KEYSTROKE:
      bval = g_value_get_boolean (value);
      if (bval != preferences->scrolling_on_keystroke)
        {
          preferences->scrolling_on_keystroke = bval;
          g_object_notify (object, "scrolling-on-keystroke");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_SHORTCUTS_NO_MENUKEY:
      bval = g_value_get_boolean (value);
      if (bval != preferences->shortcuts_no_menukey)
        {
          preferences->shortcuts_no_menukey = bval;
          g_object_notify (object, "shortcuts-no-menukey");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_SHORTCUTS_NO_MNEMONICS:
      bval = g_value_get_boolean (value);
      if (bval != preferences->shortcuts_no_mnemonics)
        {
          preferences->shortcuts_no_mnemonics = bval;
          g_object_notify (object, "shortcuts-no-mnemonics");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_TITLE_INITIAL:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->title_initial))
        {
          g_free (preferences->title_initial);
          preferences->title_initial = g_strdup (sval);
          g_object_notify (object, "title-initial");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_TITLE_MODE:
      ival = g_value_get_enum (value);
      if (ival != preferences->title_mode)
        {
          preferences->title_mode = ival;
          g_object_notify (object, "title-mode");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_TERM:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->term))
        {
          g_free (preferences->term);
          preferences->term = g_strdup (sval);
          g_object_notify (object, "term");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_VTE_WORKAROUND_TITLE_BUG:
      bval = g_value_get_boolean (value);
      if (bval != preferences->vte_workaround_title_bug)
        {
          preferences->vte_workaround_title_bug = bval;
          g_object_notify (object, "vte-workaround-title-bug");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    case PROP_WORD_CHARS:
      sval = g_value_get_string (value);
      if (!exo_str_is_equal (sval, preferences->word_chars))
        {
          g_free (preferences->word_chars);
          preferences->word_chars = g_strdup (sval);
          g_object_notify (object, "word-chars");
          terminal_preferences_schedule_store (preferences);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gchar*
property_name_to_option_name (const gchar *property_name)
{
  const gchar *s;
  gboolean     upper = TRUE;
  gchar       *option;
  gchar       *t;

  option = g_strdup (property_name);
  for (s = property_name, t = option; *s != '\0'; ++s)
    {
      if (*s == '-')
        {
          upper = TRUE;
        }
      else if (upper)
        {
          *t++ = g_ascii_toupper (*s);
          upper = FALSE;
        }
      else
        {
          *t++ = *s;
        }
    }
  *t = '\0';

  return option;
}



static void
terminal_preferences_load (TerminalPreferences *preferences)
{
  const gchar         *string;
  GParamSpec         **specs;
  GParamSpec          *spec;
  XfceRc              *rc;
  GValue               dst = { 0, };
  GValue               src = { 0, };
  gchar               *option;
  guint                nspecs;
  guint                n;

  rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG, "Terminal/terminalrc", TRUE);
  if (G_UNLIKELY (rc == NULL))
    {
      g_warning ("Unable to load terminal preferences.");
      return;
    }

  xfce_rc_set_group (rc, "Configuration");

  preferences->loading_in_progress = TRUE;

  specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (preferences), &nspecs);
  for (n = 0; n < nspecs; ++n)
    {
      spec = specs[n];

      option = property_name_to_option_name (spec->name);
      string = xfce_rc_read_entry (rc, option, NULL);
      g_free (option);

      /* retry with old config name, to get smooth transition */
      if (G_UNLIKELY (string == NULL))
        string = xfce_rc_read_entry (rc, spec->name, NULL);

      if (G_UNLIKELY (string == NULL))
        continue;

      g_value_init (&src, G_TYPE_STRING);
      g_value_set_static_string (&src, string);

      if (spec->value_type == G_TYPE_STRING)
        {
          g_object_set_property (G_OBJECT (preferences), spec->name, &src);
        }
      else if (g_value_type_transformable (G_TYPE_STRING, spec->value_type))
        {
          g_value_init (&dst, spec->value_type);
          if (g_value_transform (&src, &dst))
            g_object_set_property (G_OBJECT (preferences), spec->name, &dst);
          g_value_unset (&dst);
        }
      else
        {
          g_warning ("Unable to load property \"%s\"", spec->name);
        }

      g_value_unset (&src);
    }
  g_free (specs);

  preferences->loading_in_progress = FALSE;

  xfce_rc_close (rc);
}



static void
terminal_preferences_schedule_store (TerminalPreferences *preferences)
{
  if (preferences->store_idle_id == 0 && !preferences->loading_in_progress)
    {
      preferences->store_idle_id = g_idle_add_full (G_PRIORITY_LOW, terminal_preferences_store_idle,
                                                    preferences, terminal_preferences_store_idle_destroy);
    }
}



static gboolean
terminal_preferences_store_idle (gpointer user_data)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (user_data);
  const gchar         *string;
  GParamSpec         **specs;
  GParamSpec          *spec;
  XfceRc              *rc;
  GValue               dst = { 0, };
  GValue               src = { 0, };
  gchar               *option;
  guint                nspecs;
  guint                n;

  rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG, "Terminal/terminalrc", FALSE);
  if (G_UNLIKELY (rc == NULL))
    {
      g_warning ("Unable to store terminal preferences.");
      return FALSE;
    }

  xfce_rc_set_group (rc, "Configuration");

  specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (preferences), &nspecs);
  for (n = 0; n < nspecs; ++n)
    {
      spec = specs[n];

      g_value_init (&dst, G_TYPE_STRING);

      if (spec->value_type == G_TYPE_STRING)
        {
          g_object_get_property (G_OBJECT (preferences), spec->name, &dst);
        }
      else
        {
          g_value_init (&src, spec->value_type);
          g_object_get_property (G_OBJECT (preferences), spec->name, &src);
          g_value_transform (&src, &dst);
          g_value_unset (&src);
        }

      option = property_name_to_option_name (spec->name);

      string = g_value_get_string (&dst);

      if (G_LIKELY (string != NULL))
        xfce_rc_write_entry (rc, option, string);
      
      g_free (option);

      g_value_unset (&dst);
    }
  g_free (specs);

  xfce_rc_close (rc);

  return FALSE;
}



static void
terminal_preferences_store_idle_destroy (gpointer user_data)
{
  TERMINAL_PREFERENCES (user_data)->store_idle_id = 0;
}



/**
 * terminal_preferences_get:
 *
 * Return value :
 **/
TerminalPreferences*
terminal_preferences_get (void)
{
  if (G_UNLIKELY (default_preferences == NULL))
    {
      default_preferences = g_object_new (TERMINAL_TYPE_PREFERENCES, NULL);
      g_object_add_weak_pointer (G_OBJECT (default_preferences),
                                 (gpointer) &default_preferences);
    }
  else
    {
      g_object_ref (G_OBJECT (default_preferences));
    }

  return default_preferences;
}



