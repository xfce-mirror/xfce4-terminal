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
  PROP_ACCEL_SET_TITLE,
  PROP_ACCEL_RESET,
  PROP_ACCEL_RESET_AND_CLEAR,
  PROP_ACCEL_PREV_TAB,
  PROP_ACCEL_NEXT_TAB,
  PROP_ACCEL_SWITCH_TO_TAB1,
  PROP_ACCEL_SWITCH_TO_TAB2,
  PROP_ACCEL_SWITCH_TO_TAB3,
  PROP_ACCEL_SWITCH_TO_TAB4,
  PROP_ACCEL_SWITCH_TO_TAB5,
  PROP_ACCEL_SWITCH_TO_TAB6,
  PROP_ACCEL_SWITCH_TO_TAB7,
  PROP_ACCEL_SWITCH_TO_TAB8,
  PROP_ACCEL_SWITCH_TO_TAB9,
  PROP_ACCEL_CONTENTS,
  PROP_BACKGROUND_MODE,
  PROP_BACKGROUND_IMAGE_FILE,
  PROP_BACKGROUND_IMAGE_STYLE,
  PROP_BACKGROUND_DARKNESS,
  PROP_BINDING_BACKSPACE,
  PROP_BINDING_DELETE,
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
  PROP_FONT_ALLOW_BOLD,
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
  N_PROPERTIES,
};

struct _TerminalPreferences
{
  GObject __parent__;

  /*< private >*/
  GValue *values;
  guint   store_idle_id;
  guint   loading_in_progress : 1;
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



static GObjectClass        *parent_class;
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
                                                        "<control><shift>t",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-new-window:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_NEW_WINDOW,
                                   g_param_spec_string ("accel-new-window",
                                                        _("Open Terminal"),
                                                        _("Open Terminal"),
                                                        "<control><shift>n",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-close-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CLOSE_TAB,
                                   g_param_spec_string ("accel-close-tab",
                                                        _("Close Tab"),
                                                        _("Close Tab"),
                                                        "<control><shift>w",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-close-window:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CLOSE_WINDOW,
                                   g_param_spec_string ("accel-close-window",
                                                        _("Close Window"),
                                                        _("Close Window"),
                                                        "<control><shift>q",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-copy:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_COPY,
                                   g_param_spec_string ("accel-copy",
                                                        _("Copy"),
                                                        _("Copy"),
                                                        "<control><shift>c",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-paste:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PASTE,
                                   g_param_spec_string ("accel-paste",
                                                        _("Paste"),
                                                        _("Paste"),
                                                        "<control><shift>p",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-preferences:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PREFERENCES,
                                   g_param_spec_string ("accel-preferences",
                                                        _("Preferences"),
                                                        _("Preferences"),
                                                        _("Disabled"),
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-menubar:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_MENUBAR,
                                   g_param_spec_string ("accel-show-menubar",
                                                        _("Show menubar"),
                                                        _("Show menubar"),
                                                        _("Disabled"),
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-toolbars:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_TOOLBARS,
                                   g_param_spec_string ("accel-show-toolbars",
                                                        _("Show toolbars"),
                                                        _("Show toolbars"),
                                                        _("Disabled"),
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-show-borders:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SHOW_BORDERS,
                                   g_param_spec_string ("accel-show-borders",
                                                        _("Show borders"),
                                                        _("Show borders"),
                                                        _("Disabled"),
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-fullscreen:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_FULLSCREEN,
                                   g_param_spec_string ("accel-fullscreen",
                                                        _("Fullscreen"),
                                                        _("Fullscreen"),
                                                        "F11",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-set-title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SET_TITLE,
                                   g_param_spec_string ("accel-set-title",
                                                        _("Set Title"),
                                                        _("Set Title"),
                                                        _("Disabled"),
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-reset:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_RESET,
                                   g_param_spec_string ("accel-reset",
                                                        _("Reset"),
                                                        _("Reset"),
                                                        _("Disabled"),
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-reset-and-clear:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_RESET_AND_CLEAR,
                                   g_param_spec_string ("accel-reset-and-clear",
                                                        _("Reset and Clear"),
                                                        _("Reset and Clear"),
                                                        _("Disabled"),
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-prev-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_PREV_TAB,
                                   g_param_spec_string ("accel-prev-tab",
                                                        _("Previous Tab"),
                                                        _("Previous Tab"),
                                                        "<control>Page_Up",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-next-tab:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_NEXT_TAB,
                                   g_param_spec_string ("accel-next-tab",
                                                        _("Next Tab"),
                                                        _("Next Tab"),
                                                        "<control>Page_Down",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab1:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB1,
                                   g_param_spec_string ("accel-switch-to-tab1",
                                                        _("Switch to Tab 1"),
                                                        _("Switch to Tab 1"),
                                                        "<Alt>1",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab2:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB2,
                                   g_param_spec_string ("accel-switch-to-tab2",
                                                        _("Switch to Tab 2"),
                                                        _("Switch to Tab 2"),
                                                        "<Alt>2",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab3:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB3,
                                   g_param_spec_string ("accel-switch-to-tab3",
                                                        _("Switch to Tab 3"),
                                                        _("Switch to Tab 3"),
                                                        "<Alt>3",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab4:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB4,
                                   g_param_spec_string ("accel-switch-to-tab4",
                                                        _("Switch to Tab 4"),
                                                        _("Switch to Tab 4"),
                                                        "<Alt>4",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab5:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB5,
                                   g_param_spec_string ("accel-switch-to-tab5",
                                                        _("Switch to Tab 5"),
                                                        _("Switch to Tab 5"),
                                                        "<Alt>5",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab6:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB6,
                                   g_param_spec_string ("accel-switch-to-tab6",
                                                        _("Switch to Tab 6"),
                                                        _("Switch to Tab 6"),
                                                        "<Alt>6",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab7:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB7,
                                   g_param_spec_string ("accel-switch-to-tab7",
                                                        _("Switch to Tab 7"),
                                                        _("Switch to Tab 7"),
                                                        "<Alt>7",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab8:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB8,
                                   g_param_spec_string ("accel-switch-to-tab8",
                                                        _("Switch to Tab 8"),
                                                        _("Switch to Tab 8"),
                                                        "<Alt>8",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-switch-to-tab9:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_SWITCH_TO_TAB9,
                                   g_param_spec_string ("accel-switch-to-tab9",
                                                        _("Switch to Tab 9"),
                                                        _("Switch to Tab 9"),
                                                        "<Alt>9",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:accel-contents:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACCEL_CONTENTS,
                                   g_param_spec_string ("accel-contents",
                                                        _("Contents"),
                                                        _("Contents"),
                                                        "F1",
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
                                                        "",
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
                                                      TERMINAL_ERASE_BINDING_AUTO,
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
                                                      TERMINAL_ERASE_BINDING_AUTO,
                                                      G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-foreground:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_FOREGROUND,
                                   g_param_spec_string ("color-foreground",
                                                        _("Foreground color"),
                                                        _("Terminal foreground color"),
                                                        "White",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-background:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_BACKGROUND,
                                   g_param_spec_string ("color-background",
                                                        _("Background color"),
                                                        _("Terminal background color"),
                                                        "Black",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette1:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE1,
                                   g_param_spec_string ("color-palette1",
                                                        _("Palette entry 1"),
                                                        _("Palette entry 1"),
                                                        "#000000000000",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette2:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE2,
                                   g_param_spec_string ("color-palette2",
                                                        _("Palette entry 2"),
                                                        _("Palette entry 2"),
                                                        "#aaaa00000000",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette3:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE3,
                                   g_param_spec_string ("color-palette3",
                                                        _("Palette entry 3"),
                                                        _("Palette entry 3"),
                                                        "#0000aaaa0000",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette4:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE4,
                                   g_param_spec_string ("color-palette4",
                                                        _("Palette entry 4"),
                                                        _("Palette entry 4"),
                                                        "#aaaa55550000",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette5:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE5,
                                   g_param_spec_string ("color-palette5",
                                                        _("Palette entry 5"),
                                                        _("Palette entry 5"),
                                                        "#00000000aaaa",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette6:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE6,
                                   g_param_spec_string ("color-palette6",
                                                        _("Palette entry 6"),
                                                        _("Palette entry 6"),
                                                        "#aaaa0000aaaa",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette7:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE7,
                                   g_param_spec_string ("color-palette7",
                                                        _("Palette entry 7"),
                                                        _("Palette entry 7"),
                                                        "#0000aaaaaaaa",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette8:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE8,
                                   g_param_spec_string ("color-palette8",
                                                        _("Palette entry 8"),
                                                        _("Palette entry 8"),
                                                        "#aaaaaaaaaaaa",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette9:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE9,
                                   g_param_spec_string ("color-palette9",
                                                        _("Palette entry 9"),
                                                        _("Palette entry 9"),
                                                        "#555555555555",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette10:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE10,
                                   g_param_spec_string ("color-palette10",
                                                        _("Palette entry 10"),
                                                        _("Palette entry 10"),
                                                        "#ffff55555555",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette11:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE11,
                                   g_param_spec_string ("color-palette11",
                                                        _("Palette entry 11"),
                                                        _("Palette entry 11"),
                                                        "#5555ffff5555",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette12:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE12,
                                   g_param_spec_string ("color-palette12",
                                                        _("Palette entry 12"),
                                                        _("Palette entry 12"),
                                                        "#ffffffff5555",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette13:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE13,
                                   g_param_spec_string ("color-palette13",
                                                        _("Palette entry 13"),
                                                        _("Palette entry 13"),
                                                        "#55555555ffff",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette14:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE14,
                                   g_param_spec_string ("color-palette14",
                                                        _("Palette entry 14"),
                                                        _("Palette entry 14"),
                                                        "#ffff5555ffff",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette15:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE15,
                                   g_param_spec_string ("color-palette15",
                                                        _("Palette entry 15"),
                                                        _("Palette entry 15"),
                                                        "#5555ffffffff",
                                                        G_PARAM_READWRITE));

  /**
   * TerminalPreferences:color-palette16:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLOR_PALETTE16,
                                   g_param_spec_string ("color-palette16",
                                                        _("Palette entry 16"),
                                                        _("Palette entry 16"),
                                                        "#ffffffffffff",
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
   * TerminalPreferences:font-allow-bold:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FONT_ALLOW_BOLD,
                                   g_param_spec_boolean ("font-allow-bold",
                                                         _("Allow bold fonts"),
                                                         _("Allow bold fonts"),
                                                         TRUE,
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
                                                        "Monospace 12",
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
                                                        _("Terminal"),
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
                                                        "-A-Za-z0-9,./?%&#:_",
                                                        G_PARAM_READWRITE));
}



static void
terminal_preferences_init (TerminalPreferences *preferences)
{
  preferences->values = g_new0 (GValue, N_PROPERTIES);

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
  guint                n;

  for (n = 1; n < N_PROPERTIES; ++n)
    if (G_IS_VALUE (preferences->values + n))
      g_value_unset (preferences->values + n);
  g_free (preferences->values);

  parent_class->finalize (object);
}



static void
terminal_preferences_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  GValue              *src;

  g_return_if_fail (prop_id < N_PROPERTIES);

  src = preferences->values + prop_id;
  if (G_VALUE_HOLDS (src, pspec->value_type))
    g_value_copy (src, value);
  else
    g_param_value_set_default (pspec, value);
}



static void
terminal_preferences_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  GValue              *dst;

  g_return_if_fail (prop_id < N_PROPERTIES);

  dst = preferences->values + prop_id;
  if (!G_IS_VALUE (dst))
    g_value_init (dst, pspec->value_type);

  if (g_param_values_cmp (pspec, value, dst) != 0)
    {
      g_value_copy (value, dst);
      g_object_notify (object, pspec->name);
      terminal_preferences_schedule_store (preferences);
    }
}



static gchar*
property_name_to_option_name (const gchar *property_name)
{
  const gchar *s;
  gboolean     upper = TRUE;
  gchar       *option;
  gchar       *t;

  option = g_new (gchar, strlen (property_name) + 1);
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

  g_object_freeze_notify (G_OBJECT (preferences));

  xfce_rc_set_group (rc, "Configuration");

  preferences->loading_in_progress = TRUE;

  specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (preferences), &nspecs);
  for (n = 0; n < nspecs; ++n)
    {
      spec = specs[n];

      option = property_name_to_option_name (spec->name);
      string = xfce_rc_read_entry (rc, option, NULL);
      g_free (option);

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

  g_object_thaw_notify (G_OBJECT (preferences));
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



