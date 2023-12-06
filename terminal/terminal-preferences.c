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
#include <terminal/terminal-private.h>
#include <xfconf/xfconf.h>

#define TERMINALRC     "xfce4/terminal/terminalrc"
#define TERMINALRC_OLD "Terminal/terminalrc"


enum
{
  PROP_0,
  PROP_BACKGROUND_MODE,
  PROP_BACKGROUND_IMAGE_FILE,
  PROP_BACKGROUND_IMAGE_STYLE,
  PROP_BACKGROUND_DARKNESS,
  PROP_BACKGROUND_IMAGE_SHADING,
  PROP_BINDING_BACKSPACE,
  PROP_BINDING_DELETE,
  PROP_BINDING_AMBIGUOUS_WIDTH,
  PROP_COLOR_FOREGROUND,
  PROP_COLOR_BACKGROUND,
  PROP_COLOR_BACKGROUND_VARY,
  PROP_COLOR_CURSOR_FOREGROUND,
  PROP_COLOR_CURSOR,
  PROP_COLOR_CURSOR_USE_DEFAULT,
  PROP_COLOR_SELECTION,
  PROP_COLOR_SELECTION_BACKGROUND,
  PROP_COLOR_SELECTION_USE_DEFAULT,
  PROP_COLOR_BOLD,
  PROP_COLOR_BOLD_USE_DEFAULT,
  PROP_COLOR_PALETTE,
  PROP_COLOR_BOLD_IS_BRIGHT,
  PROP_COLOR_USE_THEME,
  PROP_COMMAND_LOGIN_SHELL,
  PROP_COMMAND_UPDATE_RECORDS,
  PROP_RUN_CUSTOM_COMMAND,
  PROP_CUSTOM_COMMAND,
  PROP_DROPDOWN_ANIMATION_TIME,
  PROP_DROPDOWN_KEEP_OPEN_DEFAULT,
  PROP_DROPDOWN_KEEP_ABOVE,
  PROP_DROPDOWN_TOGGLE_FOCUS,
  PROP_DROPDOWN_STATUS_ICON,
  PROP_DROPDOWN_WIDTH,
  PROP_DROPDOWN_HEIGHT,
  PROP_DROPDOWN_OPACITY,
  PROP_DROPDOWN_POSITION,
  PROP_DROPDOWN_POSITION_VERTICAL,
  PROP_DROPDOWN_MOVE_TO_ACTIVE,
  PROP_DROPDOWN_ALWAYS_SHOW_TABS,
  PROP_DROPDOWN_SHOW_BORDERS,
  PROP_DROPDOWN_PARAMETERS_ONCE,
  PROP_ENCODING,
  PROP_FONT_ALLOW_BOLD,
  PROP_FONT_NAME,
  PROP_FONT_USE_SYSTEM,
  PROP_MISC_ALWAYS_SHOW_TABS,
  PROP_MISC_BELL,
  PROP_MISC_BELL_URGENT,
  PROP_MISC_BORDERS_DEFAULT,
  PROP_MISC_CURSOR_BLINKS,
  PROP_MISC_CURSOR_SHAPE,
  PROP_MISC_DEFAULT_GEOMETRY,
  PROP_MISC_INHERIT_GEOMETRY,
  PROP_MISC_MENUBAR_DEFAULT,
  PROP_MISC_MOUSE_AUTOHIDE,
  PROP_MISC_MOUSE_WHEEL_ZOOM,
  PROP_MISC_TOOLBAR_DEFAULT,
  PROP_MISC_CONFIRM_CLOSE,
  PROP_MISC_CYCLE_TABS,
  PROP_MISC_TAB_CLOSE_BUTTONS,
  PROP_MISC_TAB_CLOSE_MIDDLE_CLICK,
  PROP_MISC_TAB_POSITION,
  PROP_MISC_HIGHLIGHT_URLS,
  PROP_MISC_MIDDLE_CLICK_OPENS_URI,
  PROP_MISC_COPY_ON_SELECT,
  PROP_MISC_SHOW_RELAUNCH_DIALOG,
  PROP_USE_DEFAULT_WORKING_DIR,
  PROP_DEFAULT_WORKING_DIR,
  PROP_MISC_REWRAP_ON_RESIZE,
  PROP_MISC_SLIM_TABS,
  PROP_MISC_NEW_TAB_ADJACENT,
  PROP_MISC_SEARCH_DIALOG_OPACITY,
  PROP_MISC_SHOW_UNSAFE_PASTE_DIALOG,
  PROP_MISC_RIGHT_CLICK_ACTION,
  PROP_MISC_HYPERLINKS_ENABLED,
  PROP_SCROLLING_BAR,
  PROP_OVERLAY_SCROLLING,
  PROP_SCROLLING_LINES,
  PROP_SCROLLING_ON_OUTPUT,
  PROP_SCROLLING_ON_KEYSTROKE,
  PROP_SCROLLING_UNLIMITED,
  PROP_KINETIC_SCROLLING,
  PROP_SHORTCUTS_NO_MENUKEY,
  PROP_SHORTCUTS_NO_MNEMONICS,
  PROP_TITLE_INITIAL,
  PROP_TITLE_MODE,
  PROP_WORD_CHARS,
  PROP_TAB_ACTIVITY_COLOR,
  PROP_TAB_ACTIVITY_TIMEOUT,
  PROP_TEXT_BLINK_MODE,
  PROP_CELL_WIDTH_SCALE,
  PROP_CELL_HEIGHT_SCALE,
  PROP_ENABLE_SIXEL,
  N_PROPERTIES,
};



static void     terminal_preferences_finalize      (GObject             *object);
static void     terminal_preferences_get_property  (GObject             *object,
                                                    guint                prop_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);
static void     terminal_preferences_set_property  (GObject             *object,
                                                    guint                prop_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void     terminal_preferences_prop_changed  (XfconfChannel       *channel,
                                                    const gchar         *prop_name,
                                                    const GValue        *value,
                                                    TerminalPreferences *preferences);
static void     terminal_preferences_load_rc_file  (TerminalPreferences *preferences);



struct _TerminalPreferencesClass
{
  GObjectClass __parent__;
};

struct _TerminalPreferences
{
  GObject        __parent__;

  XfconfChannel *channel;
};



static void
transform_color_to_string (const GValue *src,
                           GValue       *dst)
{
  GdkRGBA *color;
  gchar    buffer[16];

  color = g_value_get_boxed (src);
  g_snprintf (buffer, sizeof (buffer), "#%04x%04x%04x", (guint) (color->red * 65535), (guint) (color->green * 65535), (guint) (color->blue * 65535));
  g_value_set_string (dst, buffer);
}



static void
transform_string_to_boolean (const GValue *src,
                             GValue       *dst)
{
  g_value_set_boolean (dst, g_strcmp0 (g_value_get_string (src), "FALSE") != 0);
}



static void
transform_string_to_color (const GValue *src,
                           GValue       *dst)
{
  GdkRGBA color;

  gdk_rgba_parse (&color, g_value_get_string (src));
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



G_DEFINE_TYPE (TerminalPreferences, terminal_preferences, G_TYPE_OBJECT)



static GParamSpec *preferences_props[N_PROPERTIES] = { NULL, };



static void
terminal_preferences_class_init (TerminalPreferencesClass *klass)
{
  GObjectClass *gobject_class;
  guint         i;
  const GType   enum_types[] =
    {
        GTK_TYPE_POSITION_TYPE,
        TERMINAL_TYPE_BACKGROUND_STYLE,
        TERMINAL_TYPE_BACKGROUND,
        TERMINAL_TYPE_SCROLLBAR,
        TERMINAL_TYPE_TITLE,
        TERMINAL_TYPE_ERASE_BINDING,
        TERMINAL_TYPE_AMBIGUOUS_WIDTH_BINDING,
        TERMINAL_TYPE_CURSOR_SHAPE,
        TERMINAL_TYPE_TEXT_BLINK_MODE,
        TERMINAL_TYPE_RIGHT_CLICK_ACTION
    };

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_preferences_finalize;
  gobject_class->get_property = terminal_preferences_get_property;
  gobject_class->set_property = terminal_preferences_set_property;

  /* register transform functions */
  if (!g_value_type_transformable (GDK_TYPE_RGBA, G_TYPE_STRING))
    g_value_register_transform_func (GDK_TYPE_RGBA, G_TYPE_STRING, transform_color_to_string);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_BOOLEAN))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, transform_string_to_boolean);
  if (!g_value_type_transformable (G_TYPE_STRING, GDK_TYPE_RGBA))
    g_value_register_transform_func (G_TYPE_STRING, GDK_TYPE_RGBA, transform_string_to_color);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_DOUBLE))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE, transform_string_to_double);
  if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_UINT))
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT, transform_string_to_uint);
  /* enum types */
  for (i = 0; i < G_N_ELEMENTS (enum_types); ++i)
    if (!g_value_type_transformable (G_TYPE_STRING, enum_types[i]))
      g_value_register_transform_func (G_TYPE_STRING, enum_types[i], transform_string_to_enum);

  /**
   * TerminalPreferences:background-mode:
   **/
  preferences_props[PROP_BACKGROUND_MODE] =
      g_param_spec_enum ("background-mode",
                         NULL,
                         "BackgroundMode",
                         TERMINAL_TYPE_BACKGROUND,
                         TERMINAL_BACKGROUND_SOLID,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:background-image-file:
   **/
  preferences_props[PROP_BACKGROUND_IMAGE_FILE] =
      g_param_spec_string ("background-image-file",
                           NULL,
                           "BackgroundImageFile",
                           NULL,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:background-image-style:
   **/
  preferences_props[PROP_BACKGROUND_IMAGE_STYLE] =
      g_param_spec_enum ("background-image-style",
                         NULL,
                         "BackgroundImageStyle",
                         TERMINAL_TYPE_BACKGROUND_STYLE,
                         TERMINAL_BACKGROUND_STYLE_TILED,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:background-darkness:
   *
   * If a background image has been set (either an image file or a transparent background), the
   * terminal will adjust the brightness of the image before drawing the image. To do so, the
   * terminal will create a copy of the background image (or snapshot of the root window) and
   * modify its pixel values.
   **/
  preferences_props[PROP_BACKGROUND_DARKNESS] =
      g_param_spec_double ("background-darkness",
                           NULL,
                           "BackgroundDarkness",
                           0.0, 1.0, 0.5,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:background-image-shading:
   **/
  preferences_props[PROP_BACKGROUND_IMAGE_SHADING] =
      g_param_spec_double ("background-image-shading",
                           NULL,
                           "BackgroundImageShading",
                           0.0, 1.0, 0.5,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:binding-backspace:
   **/
  preferences_props[PROP_BINDING_BACKSPACE] =
      g_param_spec_enum ("binding-backspace",
                         NULL,
                         "BindingBackspace",
                         TERMINAL_TYPE_ERASE_BINDING,
                         TERMINAL_ERASE_BINDING_AUTO,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:binding-delete:
   **/
  preferences_props[PROP_BINDING_DELETE] =
      g_param_spec_enum ("binding-delete",
                         NULL,
                         "BindingDelete",
                         TERMINAL_TYPE_ERASE_BINDING,
                         TERMINAL_ERASE_BINDING_AUTO,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:binding-ambiguous-width:
   **/
  preferences_props[PROP_BINDING_AMBIGUOUS_WIDTH] =
      g_param_spec_enum ("binding-ambiguous-width",
                         NULL,
                         "BindingAmbiguousWidth",
                         TERMINAL_TYPE_AMBIGUOUS_WIDTH_BINDING,
                         TERMINAL_AMBIGUOUS_WIDTH_BINDING_NARROW,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-foreground:
   **/
  preferences_props[PROP_COLOR_FOREGROUND] =
      g_param_spec_string ("color-foreground",
                           NULL,
                           "ColorForeground",
                           "#ffffff",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-background:
   **/
  preferences_props[PROP_COLOR_BACKGROUND] =
      g_param_spec_string ("color-background",
                           NULL,
                           "ColorBackground",
                           "#000000",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-background-vary:
   **/
  preferences_props[PROP_COLOR_BACKGROUND_VARY] =
      g_param_spec_boolean ("color-background-vary",
                            NULL,
                            "ColorBackgroundVary",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-cursor-foreground:
   **/
  preferences_props[PROP_COLOR_CURSOR_FOREGROUND] =
      g_param_spec_string ("color-cursor-foreground",
                           NULL,
                           "ColorCursorForeground",
                           "",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-cursor:
   **/
  preferences_props[PROP_COLOR_CURSOR] =
      g_param_spec_string ("color-cursor",
                           NULL,
                           "ColorCursor",
                           "",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-cursor-use-default:
   **/
  preferences_props[PROP_COLOR_CURSOR_USE_DEFAULT] =
      g_param_spec_boolean ("color-cursor-use-default",
                            NULL,
                            "ColorCursorUseDefault",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-selection:
   **/
  preferences_props[PROP_COLOR_SELECTION] =
      g_param_spec_string ("color-selection",
                           NULL,
                           "ColorSelection",
                           "",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-selection-background:
   **/
  preferences_props[PROP_COLOR_SELECTION_BACKGROUND] =
      g_param_spec_string ("color-selection-background",
                           NULL,
                           "ColorSelectionBackground",
                           "",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-selection-use-default:
   **/
  preferences_props[PROP_COLOR_SELECTION_USE_DEFAULT] =
      g_param_spec_boolean ("color-selection-use-default",
                            NULL,
                            "ColorSelectionUseDefault",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-bold:
   **/
  preferences_props[PROP_COLOR_BOLD] =
      g_param_spec_string ("color-bold",
                           NULL,
                           "ColorBold",
                           "",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-bold-use-default:
   **/
  preferences_props[PROP_COLOR_BOLD_USE_DEFAULT] =
      g_param_spec_boolean ("color-bold-use-default",
                            NULL,
                            "ColorBoldUseDefault",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-palette:
   **/
  preferences_props[PROP_COLOR_PALETTE] =
      g_param_spec_string ("color-palette",
                           NULL,
                           "ColorPalette",
                           "#000000;"
                           "#aa0000;"
                           "#00aa00;"
                           "#aa5500;"
                           "#0000aa;"
                           "#aa00aa;"
                           "#00aaaa;"
                           "#aaaaaa;"
                           "#555555;"
                           "#ff5555;"
                           "#55ff55;"
                           "#ffff55;"
                           "#5555ff;"
                           "#ff55ff;"
                           "#55ffff;"
                           "#ffffff",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-bold-is-bright:
   **/
  preferences_props[PROP_COLOR_BOLD_IS_BRIGHT] =
      g_param_spec_boolean ("color-bold-is-bright",
                            NULL,
                            "ColorBoldIsBright",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-use-theme:
   **/
  preferences_props[PROP_COLOR_USE_THEME] =
      g_param_spec_boolean ("color-use-theme",
                            NULL,
                            "ColorUseTheme",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:tab-activity-color:
   **/
  preferences_props[PROP_TAB_ACTIVITY_COLOR] =
      g_param_spec_string ("tab-activity-color",
                           NULL,
                           "TabActivityColor",
                           "#aa0000",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:tab-activity-timeout:
   **/
  preferences_props[PROP_TAB_ACTIVITY_TIMEOUT] =
      g_param_spec_uint ("tab-activity-timeout",
                         NULL,
                         "TabActivityTimeout",
                         0, 30, 2,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:command-login-shell:
   **/
  preferences_props[PROP_COMMAND_LOGIN_SHELL] =
      g_param_spec_boolean ("command-login-shell",
                            NULL,
                            "CommandLoginShell",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:command-update-records:
   **/
  preferences_props[PROP_COMMAND_UPDATE_RECORDS] =
      g_param_spec_boolean ("command-update-records",
                            NULL,
                            "CommandUpdateRecords",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:run-custom-command:
   **/
  preferences_props[PROP_RUN_CUSTOM_COMMAND] =
      g_param_spec_boolean ("run-custom-command",
                            NULL,
                            "RunCustomCommand",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:custom-command:
   **/
  preferences_props[PROP_CUSTOM_COMMAND] =
      g_param_spec_string ("custom-command",
                           NULL,
                           "CustomCommand",
                           "",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:use-default-working-dir:
   **/
  preferences_props[PROP_USE_DEFAULT_WORKING_DIR] =
      g_param_spec_boolean ("use-default-working-dir",
                            NULL,
                            "UseDefaultWorkingDir",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:default-working-dir:
   **/
  preferences_props[PROP_DEFAULT_WORKING_DIR] =
      g_param_spec_string ("default-working-dir",
                           NULL,
                           "DefaultWorkingDir",
                           "",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-animation-time:
   **/
  preferences_props[PROP_DROPDOWN_ANIMATION_TIME] =
      g_param_spec_uint ("dropdown-animation-time",
                         NULL,
                         "DropdownAnimationTime",
                         0, 500, 0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-keep-open-default:
   **/
  preferences_props[PROP_DROPDOWN_KEEP_OPEN_DEFAULT] =
      g_param_spec_boolean ("dropdown-keep-open-default",
                            NULL,
                            "DropdownKeepOpenDefault",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);


  /**
   * TerminalPreferences:dropdown-keep-above:
   **/
  preferences_props[PROP_DROPDOWN_KEEP_ABOVE] =
      g_param_spec_boolean ("dropdown-keep-above",
                            NULL,
                            "DropdownKeepAbove",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);


  /**
   * TerminalPreferences:dropdown-toggle-focus:
   **/
  preferences_props[PROP_DROPDOWN_TOGGLE_FOCUS] =
      g_param_spec_boolean ("dropdown-toggle-focus",
                            NULL,
                            "DropdownToggleFocus",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-status-icon:
   **/
  preferences_props[PROP_DROPDOWN_STATUS_ICON] =
      g_param_spec_boolean ("dropdown-status-icon",
                            NULL,
                            "DropdownStatusIcon",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);


  /**
   * TerminalPreferences:dropdown-width:
   **/
  preferences_props[PROP_DROPDOWN_WIDTH] =
      g_param_spec_uint ("dropdown-width",
                         NULL,
                         "DropdownWidth",
                         10, 100, 80,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-height:
   **/
  preferences_props[PROP_DROPDOWN_HEIGHT] =
      g_param_spec_uint ("dropdown-height",
                         NULL,
                         "DropdownHeight",
                         10, 100, 50,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-opacity:
   **/
  preferences_props[PROP_DROPDOWN_OPACITY] =
      g_param_spec_uint ("dropdown-opacity",
                         NULL,
                         "DropdownOpacity",
                         0, 100, 100,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-position:
   **/
  preferences_props[PROP_DROPDOWN_POSITION] =
      g_param_spec_uint ("dropdown-position",
                         NULL,
                         "DropdownPosition",
                         0, 100, 50,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-position-vertical:
   **/
  preferences_props[PROP_DROPDOWN_POSITION_VERTICAL] =
      g_param_spec_uint ("dropdown-position-vertical",
                         NULL,
                         "DropdownPositionVertical",
                         0, 100, 0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:move-to-active:
   **/
  preferences_props[PROP_DROPDOWN_MOVE_TO_ACTIVE] =
      g_param_spec_boolean ("dropdown-move-to-active",
                            NULL,
                            "DropdownMoveToActive",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-always-show-tabs:
   **/
  preferences_props[PROP_DROPDOWN_ALWAYS_SHOW_TABS] =
      g_param_spec_boolean ("dropdown-always-show-tabs",
                            NULL,
                            "DropdownAlwaysShowTabs",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-show-borders:
   **/
  preferences_props[PROP_DROPDOWN_SHOW_BORDERS] =
      g_param_spec_boolean ("dropdown-show-borders",
                            NULL,
                            "DropdownShowBorders",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:dropdown-parameters-once:
   *
   * If set to TRUE the drop-down window accepts command line parameters only the first time, i.e. when it is created.
   **/
  preferences_props[PROP_DROPDOWN_PARAMETERS_ONCE] =
      g_param_spec_boolean ("dropdown-parameters-once",
                            NULL,
                            "DropdownParametersOnce",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:encoding:
   **/
  preferences_props[PROP_ENCODING] =
      g_param_spec_string ("encoding",
                           NULL,
                           "Encoding",
                           NULL,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:font-allow-bold:
   **/
  preferences_props[PROP_FONT_ALLOW_BOLD] =
      g_param_spec_boolean ("font-allow-bold",
                            NULL,
                            "FontAllowBold",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:font-name:
   **/
  preferences_props[PROP_FONT_NAME] =
      g_param_spec_string ("font-name",
                           NULL,
                           "FontName",
                           "Monospace 12",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:font-allow-bold:
   **/
  preferences_props[PROP_FONT_USE_SYSTEM] =
      g_param_spec_boolean ("font-use-system",
                            NULL,
                            "FontUseSystem",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-always-show-tabs:
   **/
  preferences_props[PROP_MISC_ALWAYS_SHOW_TABS] =
      g_param_spec_boolean ("misc-always-show-tabs",
                            NULL,
                            "MiscAlwaysShowTabs",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-bell:
   **/
  preferences_props[PROP_MISC_BELL] =
      g_param_spec_boolean ("misc-bell",
                            NULL,
                            "MiscBell",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-bell-urgent:
   **/
  preferences_props[PROP_MISC_BELL_URGENT] =
      g_param_spec_boolean ("misc-bell-urgent",
                            NULL,
                            "MiscBellUrgent",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-borders-default:
   **/
  preferences_props[PROP_MISC_BORDERS_DEFAULT] =
      g_param_spec_boolean ("misc-borders-default",
                            NULL,
                            "MiscBordersDefault",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-cursor-blinks:
   **/
  preferences_props[PROP_MISC_CURSOR_BLINKS] =
      g_param_spec_boolean ("misc-cursor-blinks",
                            NULL,
                            "MiscCursorBlinks",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-cursor-shape:
   **/
  preferences_props[PROP_MISC_CURSOR_SHAPE] =
      g_param_spec_enum ("misc-cursor-shape",
                         NULL,
                         "MiscCursorShape",
                         TERMINAL_TYPE_CURSOR_SHAPE,
                         TERMINAL_CURSOR_SHAPE_BLOCK,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-default-geometry:
   **/
  preferences_props[PROP_MISC_DEFAULT_GEOMETRY] =
      g_param_spec_string ("misc-default-geometry",
                           NULL,
                           "MiscDefaultGeometry",
                           "80x24",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-inherit-geometry:
   **/
  preferences_props[PROP_MISC_INHERIT_GEOMETRY] =
      g_param_spec_boolean ("misc-inherit-geometry",
                            NULL,
                            "MiscInheritGeometry",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-menubar-default:
   **/
  preferences_props[PROP_MISC_MENUBAR_DEFAULT] =
      g_param_spec_boolean ("misc-menubar-default",
                            NULL,
                            "MiscMenubarDefault",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-mouse-autohide:
   **/
  preferences_props[PROP_MISC_MOUSE_AUTOHIDE] =
      g_param_spec_boolean ("misc-mouse-autohide",
                            NULL,
                            "MiscMouseAutohide",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-mouse-wheel-zoom:
   **/
  preferences_props[PROP_MISC_MOUSE_WHEEL_ZOOM] =
      g_param_spec_boolean ("misc-mouse-wheel-zoom",
                            NULL,
                            "MiscMouseWheelZoom",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-toolbar-default:
   **/
  preferences_props[PROP_MISC_TOOLBAR_DEFAULT] =
      g_param_spec_boolean ("misc-toolbar-default",
                            NULL,
                            "MiscToolbarDefault",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-confirm-close:
   **/
  preferences_props[PROP_MISC_CONFIRM_CLOSE] =
      g_param_spec_boolean ("misc-confirm-close",
                            NULL,
                            "MiscConfirmClose",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-cycle-tabs:
   **/
  preferences_props[PROP_MISC_CYCLE_TABS] =
      g_param_spec_boolean ("misc-cycle-tabs",
                            NULL,
                            "MiscCycleTabs",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-tab-close-buttons:
   **/
  preferences_props[PROP_MISC_TAB_CLOSE_BUTTONS] =
      g_param_spec_boolean ("misc-tab-close-buttons",
                            NULL,
                            "MiscTabCloseButtons",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-tab-close-middle-click:
   **/
  preferences_props[PROP_MISC_TAB_CLOSE_MIDDLE_CLICK] =
      g_param_spec_boolean ("misc-tab-close-middle-click",
                            NULL,
                            "MiscTabCloseMiddleClick",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-tab-position:
   **/
  preferences_props[PROP_MISC_TAB_POSITION] =
      g_param_spec_enum ("misc-tab-position",
                         NULL,
                         "MiscTabPosition",
                         GTK_TYPE_POSITION_TYPE,
                         GTK_POS_TOP,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-highlight-urls:
   **/
  preferences_props[PROP_MISC_HIGHLIGHT_URLS] =
      g_param_spec_boolean ("misc-highlight-urls",
                            NULL,
                            "MiscHighlightUrls",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-middle-click-open-uri:
   **/
  preferences_props[PROP_MISC_MIDDLE_CLICK_OPENS_URI] =
      g_param_spec_boolean ("misc-middle-click-opens-uri",
                            NULL,
                            "MiscMiddleClickOpensUri",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-copy-on-select:
   **/
  preferences_props[PROP_MISC_COPY_ON_SELECT] =
      g_param_spec_boolean ("misc-copy-on-select",
                            NULL,
                            "MiscCopyOnSelect",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-rewrap-on-resize:
   **/
  preferences_props[PROP_MISC_REWRAP_ON_RESIZE] =
      g_param_spec_boolean ("misc-rewrap-on-resize",
                            NULL,
                            "MiscRewrapOnResize",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-slim-tabs:
   **/
  preferences_props[PROP_MISC_SLIM_TABS] =
      g_param_spec_boolean ("misc-slim-tabs",
                            NULL,
                            "MiscSlimTabs",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-new-tab-adjacent:
   **/
  preferences_props[PROP_MISC_NEW_TAB_ADJACENT] =
      g_param_spec_boolean ("misc-new-tab-adjacent",
                            NULL,
                            "MiscNewTabAdjacent",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-show-relaunch-dialog:
   **/
  preferences_props[PROP_MISC_SHOW_RELAUNCH_DIALOG] =
      g_param_spec_boolean ("misc-show-relaunch-dialog",
                            NULL,
                            "MiscShowRelaunchDialog",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-search-dialog-opacity:
   **/
  preferences_props[PROP_MISC_SEARCH_DIALOG_OPACITY] =
      g_param_spec_uint ("misc-search-dialog-opacity",
                         NULL,
                         "MiscSearchDialogOpacity",
                         0, 100, 100,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-show-unsafe-paste-dialog:
   **/
  preferences_props[PROP_MISC_SHOW_UNSAFE_PASTE_DIALOG] =
      g_param_spec_boolean ("misc-show-unsafe-paste-dialog",
                            NULL,
                            "MiscShowUnsafePasteDialog",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-right-click-action:
   **/
  preferences_props[PROP_MISC_RIGHT_CLICK_ACTION] =
      g_param_spec_enum ("misc-right-click-action",
                         NULL,
                         "MiscRightClickAction",
                         TERMINAL_TYPE_RIGHT_CLICK_ACTION,
                         TERMINAL_RIGHT_CLICK_ACTION_CONTEXT_MENU,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:misc-hyperlinks-enabled:
   **/
  preferences_props[PROP_MISC_HYPERLINKS_ENABLED] =
      g_param_spec_boolean ("misc-hyperlinks-enabled",
                            NULL,
                            "MiscHyperlinksEnabled",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:scrolling-bar:
   **/
  preferences_props[PROP_SCROLLING_BAR] =
      g_param_spec_enum ("scrolling-bar",
                         NULL,
                         "ScrollingBar",
                         TERMINAL_TYPE_SCROLLBAR,
                         TERMINAL_SCROLLBAR_RIGHT,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:overlay-scrolling:
   **/
  preferences_props[PROP_OVERLAY_SCROLLING] =
      g_param_spec_boolean ("overlay-scrolling",
                            NULL,
                            "OverlayScrolling",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:scrolling-lines:
   **/
  preferences_props[PROP_SCROLLING_LINES] =
      g_param_spec_uint ("scrolling-lines",
                         NULL,
                         "ScrollingLines",
                         0u, 1024u * 1024u, 1000u,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:scrolling-unlimited:
   **/
  preferences_props[PROP_SCROLLING_UNLIMITED] =
      g_param_spec_boolean ("scrolling-unlimited",
                            NULL,
                            "ScrollingUnlimited",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:kinetic-scrolling:
   **/
  preferences_props[PROP_KINETIC_SCROLLING] =
      g_param_spec_boolean ("kinetic-scrolling",
                            NULL,
                            "KineticScrolling",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:scrolling-on-output:
   **/
  preferences_props[PROP_SCROLLING_ON_OUTPUT] =
      g_param_spec_boolean ("scrolling-on-output",
                            NULL,
                            "ScrollingOnOutput",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:scrolling-on-keystroke:
   **/
  preferences_props[PROP_SCROLLING_ON_KEYSTROKE] =
      g_param_spec_boolean ("scrolling-on-keystroke",
                            NULL,
                            "ScrollingOnKeystroke",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:shortcuts-no-menukey:
   *
   * Disable menu shortcut key (F10 by default).
   **/
  preferences_props[PROP_SHORTCUTS_NO_MENUKEY] =
      g_param_spec_boolean ("shortcuts-no-menukey",
                            NULL,
                            "ShortcutsNoMenukey",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:shortcuts-no-mnemonics:
   **/
  preferences_props[PROP_SHORTCUTS_NO_MNEMONICS] =
      g_param_spec_boolean ("shortcuts-no-mnemonics",
                            NULL,
                            "ShortcutsNoMnemonics",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:title-initial:
   **/
  preferences_props[PROP_TITLE_INITIAL] =
      g_param_spec_string ("title-initial",
                           NULL,
                           "TitleInitial",
                           _("Terminal"),
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:title-mode:
   **/
  preferences_props[PROP_TITLE_MODE] =
      g_param_spec_enum ("title-mode",
                         NULL,
                         "TitleMode",
                         TERMINAL_TYPE_TITLE,
                         TERMINAL_TITLE_APPEND,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:word-chars:
   **/
  preferences_props[PROP_WORD_CHARS] =
      g_param_spec_string ("word-chars",
                           NULL,
                           "WordChars",
                           "-A-Za-z0-9,./?%&#:_=+@~",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:text-blink-mode:
   **/
  preferences_props[PROP_TEXT_BLINK_MODE] =
      g_param_spec_enum ("text-blink-mode",
                         NULL,
                         "TextBlinkMode",
                         TERMINAL_TYPE_TEXT_BLINK_MODE,
                         TERMINAL_TEXT_BLINK_MODE_ALWAYS,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:cell-width-scale:
   **/
  preferences_props[PROP_CELL_WIDTH_SCALE] =
      g_param_spec_double ("cell-width-scale",
                           NULL,
                           "CellWidthScale",
                           1.0, 2.0, 1.0,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:cell-height-scale:
   **/
  preferences_props[PROP_CELL_HEIGHT_SCALE] =
      g_param_spec_double ("cell-height-scale",
                           NULL,
                           "CellHeightScale",
                           1.0, 2.0, 1.0,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:enable-sixel:
   **/
  preferences_props[PROP_ENABLE_SIXEL] =
      g_param_spec_boolean ("enable-sixel",
                            NULL,
                            "EnableSixel",
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /* install all properties */
  g_object_class_install_properties (gobject_class, N_PROPERTIES, preferences_props);
}



static void
terminal_preferences_init (TerminalPreferences *preferences)
{
  GError *error = NULL;
  gchar **channels;

  /* don't set a channel if xfconf init failed */
  if (!xfconf_init (&error))
    {
      g_warning ("Failed to initialize Xfconf: %s", error->message);
      g_error_free (error);
      return;
    }

  /* load the channel */
  preferences->channel = xfconf_channel_get ("xfce4-terminal");

  channels = xfconf_list_channels ();
  if (!g_strv_contains ((const gchar * const *) channels, "xfce4-terminal"))
    {
      /* try to load the old config file & save changes */
      terminal_preferences_load_rc_file (preferences);
    }
  g_strfreev (channels);

  g_signal_connect (G_OBJECT (preferences->channel), "property-changed",
                    G_CALLBACK (terminal_preferences_prop_changed), preferences);
}



static void
terminal_preferences_finalize (GObject *object)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);

  if (G_LIKELY (preferences->channel != NULL))
    xfconf_shutdown ();

  (*G_OBJECT_CLASS (terminal_preferences_parent_class)->finalize) (object);
}



static void
terminal_preferences_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  TerminalPreferences  *preferences = TERMINAL_PREFERENCES (object);
  GValue                src = { 0, };
  gchar                 prop_name[64];
  gchar               **array;

  g_return_if_fail (prop_id < N_PROPERTIES);

  /* only set defaults if channel is not set */
  if (G_UNLIKELY (preferences->channel == NULL))
    {
      g_param_value_set_default (pspec, value);
      return;
    }

  /* build property name */
  g_snprintf (prop_name, sizeof (prop_name), "/%s", g_param_spec_get_name (pspec));

  if (G_VALUE_TYPE (value) == G_TYPE_STRV)
    {
      /* handle arrays directly since we cannot transform those */
      array = xfconf_channel_get_string_list (preferences->channel, prop_name);
      g_value_take_boxed (value, array);
    }
  else if (xfconf_channel_get_property (preferences->channel, prop_name, &src))
    {
      if (G_VALUE_TYPE (value) == G_VALUE_TYPE (&src))
        g_value_copy (&src, value);
      else if (!g_value_transform (&src, value))
        g_warning ("Failed to transform property %s", prop_name);
      g_value_unset (&src);
    }
  else
    {
      /* value is not found, return default */
      g_param_value_set_default (pspec, value);
    }
}



static void
terminal_preferences_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  TerminalPreferences  *preferences = TERMINAL_PREFERENCES (object);
  GValue                dst = { 0, };
  gchar                 prop_name[64];
  gchar               **array;

  /* leave if the channel is not set */
  if (G_UNLIKELY (preferences->channel == NULL))
    return;

  /* build property name */
  g_snprintf (prop_name, sizeof (prop_name), "/%s", g_param_spec_get_name (pspec));

  if (G_VALUE_HOLDS_ENUM (value))
    {
      /* convert into a string */
      g_value_init (&dst, G_TYPE_STRING);
      if (g_value_transform (value, &dst))
        xfconf_channel_set_property (preferences->channel, prop_name, &dst);
      g_value_unset (&dst);
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
      /* convert to a GValue GPtrArray in xfconf */
      array = g_value_get_boxed (value);
      if (array != NULL && *array != NULL)
        xfconf_channel_set_string_list (preferences->channel, prop_name, (const gchar *const *) array);
      else
        xfconf_channel_reset_property (preferences->channel, prop_name, FALSE);
    }
  else
    {
      /* other types we support directly */
      xfconf_channel_set_property (preferences->channel, prop_name, value);
    }
}



static void
terminal_preferences_prop_changed (XfconfChannel       *channel,
                                   const gchar         *prop_name,
                                   const GValue        *value,
                                   TerminalPreferences *preferences)
{
  GParamSpec *pspec;

  /* check if the property exists and emit change */
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (preferences), prop_name + 1);
  if (G_LIKELY (pspec != NULL))
    g_object_notify_by_pspec (G_OBJECT (preferences), pspec);
}



static void
terminal_preferences_load_rc_file (TerminalPreferences *preferences)
{
  gchar       *filename;
  const gchar *string, *name;
  GParamSpec  *pspec;
  XfceRc      *rc;
  GValue       dst = { 0, };
  GValue       src = { 0, };
  guint        n;
  gboolean     migrate_colors = FALSE;
  gchar        color_name[16];
  GString     *array;

  /* find file */
  filename = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, TERMINALRC);
  if (G_UNLIKELY (filename == NULL))
    {
      /* old location of the Terminal days */
      filename = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, TERMINALRC_OLD);
      migrate_colors = TRUE;
      if (G_UNLIKELY (filename == NULL))
        return;
    }

  /* look for preferences */
  rc = xfce_rc_simple_open (filename, TRUE);
  if (G_UNLIKELY (rc == NULL))
    return;

  g_object_freeze_notify (G_OBJECT (preferences));

  xfce_rc_set_group (rc, "Configuration");

  g_value_init (&src, G_TYPE_STRING);

  for (n = PROP_0 + 1; n < N_PROPERTIES; ++n)
    {
      pspec = preferences_props[n];
      name = g_param_spec_get_name (pspec);

      string = xfce_rc_read_entry (rc, g_param_spec_get_blurb (pspec), NULL);
      if (G_UNLIKELY (string == NULL))
        {
          g_object_notify_by_pspec (G_OBJECT (preferences), pspec);
        }
      else
        {
          g_value_set_static_string (&src, string);

          if (G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_STRING)
            {
              /* set the string property */
              g_object_set_property (G_OBJECT (preferences), name, &src);
            }
          else
            {
              g_value_init (&dst, G_PARAM_SPEC_VALUE_TYPE (pspec));
              if (G_LIKELY (g_value_transform (&src, &dst)))
                g_object_set_property (G_OBJECT (preferences), name, &dst);
              else
                g_warning ("Unable to load property \"%s\"", name);
              g_value_unset (&dst);
            }
        }
    }

  /* migrate old terminal color properties into a single string */
  if (G_UNLIKELY (migrate_colors))
    {
      /* concat all values */
      array = g_string_sized_new (225);
      for (n = 1; n <= 16; n++)
        {
          g_snprintf (color_name, sizeof (color_name), "ColorPalette%d", n);
          string = xfce_rc_read_entry (rc, color_name, NULL);
          if (string == NULL)
            break;

          g_string_append (array, string);
          if (n != 16)
            g_string_append_c (array, ';');
        }

      /* set property if 16 colors were found */
      if (n >= 16)
        g_object_set (G_OBJECT (preferences), "color-palette", array->str, NULL);
      g_string_free (array, TRUE);
    }

  g_value_unset (&src);

  xfce_rc_close (rc);

  g_object_thaw_notify (G_OBJECT (preferences));

  g_message ("Your Terminal settings have been migrated to Xfconf."
             " The config file \"%s\" is not used anymore.", filename);

  g_free (filename);
}



/**
 * terminal_preferences_get:
 *
 * Return value :
 **/
TerminalPreferences*
terminal_preferences_get (void)
{
  static TerminalPreferences *preferences = NULL;

  if (G_UNLIKELY (preferences == NULL))
    {
      preferences = g_object_new (TERMINAL_TYPE_PREFERENCES, NULL);
      g_object_add_weak_pointer (G_OBJECT (preferences),
                                 (gpointer) &preferences);
    }
  else
    {
      g_object_ref (G_OBJECT (preferences));
    }

  return preferences;
}



gboolean
terminal_preferences_get_color (TerminalPreferences *preferences,
                                const gchar         *property,
                                GdkRGBA             *color_return)
{
  gchar    *spec;
  gboolean  succeed = FALSE;

  g_return_val_if_fail (TERMINAL_IS_PREFERENCES (preferences), FALSE);

  g_object_get (G_OBJECT (preferences), property, &spec, NULL);
  if (G_LIKELY (spec != NULL))
    succeed = gdk_rgba_parse (color_return, spec);
  g_free (spec);

  return succeed;
}
