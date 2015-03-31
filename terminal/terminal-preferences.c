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

#define TERMINALRC     "xfce4/terminal/terminalrc"
#define TERMINALRC_OLD "Terminal/terminalrc"


enum
{
  PROP_0,
  PROP_BACKGROUND_MODE,
  PROP_BACKGROUND_IMAGE_FILE,
  PROP_BACKGROUND_IMAGE_STYLE,
  PROP_BACKGROUND_DARKNESS,
  PROP_BINDING_BACKSPACE,
  PROP_BINDING_DELETE,
  PROP_COLOR_FOREGROUND,
  PROP_COLOR_BACKGROUND,
  PROP_COLOR_BACKGROUND_VARY,
  PROP_COLOR_CURSOR,
  PROP_COLOR_SELECTION,
  PROP_COLOR_SELECTION_USE_DEFAULT,
  PROP_COLOR_BOLD,
  PROP_COLOR_BOLD_USE_DEFAULT,
  PROP_COLOR_PALETTE,
  PROP_COMMAND_UPDATE_RECORDS,
  PROP_COMMAND_LOGIN_SHELL,
  PROP_DROPDOWN_ANIMATION_TIME,
  PROP_DROPDOWN_KEEP_OPEN_DEFAULT,
  PROP_DROPDOWN_KEEP_ABOVE,
  PROP_DROPDOWN_TOGGLE_FOCUS,
  PROP_DROPDOWN_STATUS_ICON,
  PROP_DROPDOWN_WIDTH,
  PROP_DROPDOWN_HEIGHT,
  PROP_DROPDOWN_OPACITY,
  PROP_DROPDOWN_POSITION,
  PROP_DROPDOWN_MOVE_TO_ACTIVE,
  PROP_DROPDOWN_ALWAYS_SHOW_TABS,
  PROP_ENCODING,
  PROP_FONT_ALLOW_BOLD,
  PROP_FONT_NAME,
  PROP_MISC_ALWAYS_SHOW_TABS,
  PROP_MISC_BELL,
  PROP_MISC_BORDERS_DEFAULT,
  PROP_MISC_CURSOR_BLINKS,
  PROP_MISC_CURSOR_SHAPE,
  PROP_MISC_DEFAULT_GEOMETRY,
  PROP_MISC_INHERIT_GEOMETRY,
  PROP_MISC_MENUBAR_DEFAULT,
  PROP_MISC_MOUSE_AUTOHIDE,
  PROP_MISC_TOOLBAR_DEFAULT,
  PROP_MISC_CONFIRM_CLOSE,
  PROP_MISC_CYCLE_TABS,
  PROP_MISC_TAB_CLOSE_BUTTONS,
  PROP_MISC_TAB_CLOSE_MIDDLE_CLICK,
  PROP_MISC_TAB_POSITION,
  PROP_MISC_HIGHLIGHT_URLS,
  PROP_SCROLLING_BAR,
  PROP_SCROLLING_LINES,
  PROP_SCROLLING_ON_OUTPUT,
  PROP_SCROLLING_ON_KEYSTROKE,
  PROP_SCROLLING_SINGLE_LINE,
  PROP_SHORTCUTS_NO_MENUKEY,
  PROP_SHORTCUTS_NO_MNEMONICS,
  PROP_TITLE_INITIAL,
  PROP_TITLE_MODE,
  PROP_EMULATION,
  PROP_WORD_CHARS,
  PROP_TAB_ACTIVITY_COLOR,
  PROP_TAB_ACTIVITY_TIMEOUT,
  N_PROPERTIES,
};

struct _TerminalPreferences
{
  GObject __parent__;

  GValue           values[N_PROPERTIES];

  GFile           *file;
  GFileMonitor    *monitor;
  guint32          last_mtime;

  guint            store_idle_id;
  guint            loading_in_progress : 1;
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
static void     terminal_preferences_monitor_changed    (GFileMonitor        *monitor,
                                                         GFile               *file,
                                                         GFile               *other_file,
                                                         GFileMonitorEvent    event_type,
                                                         TerminalPreferences *preferences);
static void     terminal_preferences_monitor_disconnect (TerminalPreferences *preferences);
static void     terminal_preferences_monitor_connect    (TerminalPreferences *preferences,
                                                         const gchar         *filename,
                                                         gboolean             update_mtime);



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
  g_value_set_boolean (dst, g_strcmp0 (g_value_get_string (src), "FALSE") != 0);
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



G_DEFINE_TYPE (TerminalPreferences, terminal_preferences, G_TYPE_OBJECT)



static GParamSpec *preferences_props[N_PROPERTIES] = { NULL, };



static void
terminal_preferences_class_init (TerminalPreferencesClass *klass)
{
  GObjectClass *gobject_class;

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
  if (!g_value_type_transformable (G_TYPE_STRING, GTK_TYPE_POSITION_TYPE))
    g_value_register_transform_func (G_TYPE_STRING, GTK_TYPE_POSITION_TYPE, transform_string_to_enum);
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
  if (!g_value_type_transformable (G_TYPE_STRING, TERMINAL_TYPE_CURSOR_SHAPE))
    g_value_register_transform_func (G_TYPE_STRING, TERMINAL_TYPE_CURSOR_SHAPE, transform_string_to_enum);

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
   * TerminalPreferences:color-cursor:
   **/
  preferences_props[PROP_COLOR_CURSOR] =
      g_param_spec_string ("color-cursor",
                           NULL,
                           "ColorCursor",
                           "#00aa00",
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:color-selection:
   **/
  preferences_props[PROP_COLOR_SELECTION] =
      g_param_spec_string ("color-selection",
                           NULL,
                           "ColorSelection",
                           "#ffffff",
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
                           "#ffffff",
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
   * TerminalPreferences:command-update-records:
   **/
  preferences_props[PROP_COMMAND_UPDATE_RECORDS] =
      g_param_spec_boolean ("command-update-records",
                            NULL,
                            "CommandUpdateRecords",
                            TRUE,
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
                            TRUE,
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
   * TerminalPreferences:scrolling-lines:
   **/
  preferences_props[PROP_SCROLLING_LINES] =
      g_param_spec_uint ("scrolling-lines",
                         NULL,
                         "ScrollingLines",
                         0u, 1024u * 1024u, 1000u,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:scrolling-on-output:
   **/
  preferences_props[PROP_SCROLLING_ON_OUTPUT] =
      g_param_spec_boolean ("scrolling-on-output",
                            NULL,
                            "ScrollingOnOutput",
                            TRUE,
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
   * TerminalPreferences:scrolling-single-line:
   *
   * Whether to enable scrolling single lines using Shift-Up/-Down.
   **/
  preferences_props[PROP_SCROLLING_SINGLE_LINE] =
      g_param_spec_boolean ("scrolling-single-line",
                            NULL,
                            "ScrollingSingleLine",
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
                         G_PARAM_READWRITE| G_PARAM_STATIC_STRINGS);

  /**
   * TerminalPreferences:emulation:
   **/
  preferences_props[PROP_EMULATION] =
      g_param_spec_string ("emulation",
                           NULL,
                           "Emulation",
                           "xterm",
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

  /* install all properties */
  g_object_class_install_properties (gobject_class, N_PROPERTIES, preferences_props);
}



static void
terminal_preferences_init (TerminalPreferences *preferences)
{
  /* load settings */
  terminal_preferences_load (preferences);
}



static void
terminal_preferences_dispose (GObject *object)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);

  /* stop file monitoring */
  terminal_preferences_monitor_disconnect (preferences);

  /* flush preferences */
  if (G_UNLIKELY (preferences->store_idle_id != 0))
    {
      g_source_remove (preferences->store_idle_id);
      terminal_preferences_store_idle (preferences);
    }

  (*G_OBJECT_CLASS (terminal_preferences_parent_class)->dispose) (object);
}



static void
terminal_preferences_finalize (GObject *object)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  guint                n;

  for (n = 1; n < N_PROPERTIES; ++n)
    if (G_IS_VALUE (preferences->values + n))
      g_value_unset (preferences->values + n);

  (*G_OBJECT_CLASS (terminal_preferences_parent_class)->finalize) (object);
}



static void
terminal_preferences_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  GValue              *src;

  terminal_return_if_fail (prop_id < N_PROPERTIES);

  src = preferences->values + prop_id;
  if (G_VALUE_HOLDS (src, pspec->value_type))
    {
      if (G_LIKELY (pspec->value_type == G_TYPE_STRING))
        g_value_set_static_string (value, g_value_get_string (src));
      else
        g_value_copy (src, value);
    }
  else
    {
      g_param_value_set_default (pspec, value);
    }
}



static void
terminal_preferences_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  TerminalPreferences *preferences = TERMINAL_PREFERENCES (object);
  GValue              *dst;

  terminal_return_if_fail (prop_id < N_PROPERTIES);
  terminal_return_if_fail (preferences_props[prop_id] == pspec);

  dst = preferences->values + prop_id;
  if (!G_IS_VALUE (dst))
    {
      g_value_init (dst, pspec->value_type);
      g_param_value_set_default (pspec, dst);
    }

  if (g_param_values_cmp (pspec, value, dst) != 0)
    {
      g_value_copy (value, dst);

      /* don't schedule a store if loading */
      if (!preferences->loading_in_progress)
        {
          /* notify */
          g_object_notify_by_pspec (object, pspec);

          /* store new value */
          terminal_preferences_schedule_store (preferences);
        }
    }
}



#ifdef G_ENABLE_DEBUG
static void
terminal_preferences_check_blurb (GParamSpec *spec)
{
  const gchar *s, *name;
  gboolean     upper = TRUE;
  gchar       *option, *t;

  /* generate the option name */
  name = g_param_spec_get_name (spec);
  option = g_new (gchar, strlen (name) + 1);
  for (s = name, t = option; *s != '\0'; ++s)
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

  /* check if the generated option name is equal to the blurb */
  if (g_strcmp0 (option, g_param_spec_get_blurb (spec)) != 0)
    g_error ("The blurb of property \"%s\" does not match option name", name);

  /* cleanup */
  g_free (option);
}
#endif



static void
terminal_preferences_load (TerminalPreferences *preferences)
{
  gchar        *filename;
  const gchar  *string, *name;
  GParamSpec   *pspec;
  XfceRc       *rc;
  GValue        dst = { 0, };
  GValue        src = { 0, };
  GValue       *value;
  guint         n;
  gboolean      migrate_colors = FALSE;
  gchar         color_name[16];
  GString      *array;

  filename = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, TERMINALRC);
  if (G_UNLIKELY (filename == NULL))
    {
      /* old location of the Terminal days */
      filename = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, TERMINALRC_OLD);
      migrate_colors = TRUE;
      if (G_UNLIKELY (filename == NULL))
        return;
    }

  rc = xfce_rc_simple_open (filename, TRUE);
  if (G_UNLIKELY (rc == NULL))
    goto connect_monitor;

  preferences->loading_in_progress = TRUE;

  g_object_freeze_notify (G_OBJECT (preferences));

  xfce_rc_set_group (rc, "Configuration");

  g_value_init (&src, G_TYPE_STRING);

  for (n = PROP_0 + 1; n < N_PROPERTIES; ++n)
    {
      pspec = preferences_props[n];
      name = g_param_spec_get_name (pspec);

#ifdef G_ENABLE_DEBUG
      terminal_assert (g_param_spec_get_nick (pspec) != NULL);
      terminal_preferences_check_blurb (pspec);
#endif

      string = xfce_rc_read_entry (rc, g_param_spec_get_blurb (pspec), NULL);
      if (G_UNLIKELY (string == NULL))
        {
          /* check if we need to reset to the default value */
          value = preferences->values + n;
          if (G_IS_VALUE (value))
            {
              g_value_unset (value);
              g_object_notify_by_pspec (G_OBJECT (preferences), pspec);
            }
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

connect_monitor:
  /* startup file monitoring */
  terminal_preferences_monitor_connect (preferences, filename, FALSE);

  preferences->loading_in_progress = FALSE;

  g_free (filename);
}



static void
terminal_preferences_schedule_store (TerminalPreferences *preferences)
{
  if (preferences->store_idle_id == 0 && !preferences->loading_in_progress)
    {
      preferences->store_idle_id =
          g_timeout_add_seconds_full (G_PRIORITY_LOW, 1, terminal_preferences_store_idle,
                                      preferences, terminal_preferences_store_idle_destroy);
    }
}



static void
terminal_preferences_store_value (const GValue *value,
                                  const gchar  *property,
                                  XfceRc       *rc)
{
  GValue       dst = { 0, };
  const gchar *string;

  terminal_return_if_fail (G_IS_VALUE (value));

  if (G_VALUE_HOLDS_STRING (value))
    {
      /* write */
      string = g_value_get_string (value);
      if (G_LIKELY (string != NULL))
        xfce_rc_write_entry (rc, property, string);
    }
  else
    {
      /* transform the property to a string */
      g_value_init (&dst, G_TYPE_STRING);
      if (!g_value_transform (value, &dst))
        terminal_assert_not_reached ();

      /* write */
      string = g_value_get_string (&dst);
      if (G_LIKELY (string != NULL))
        xfce_rc_write_entry (rc, property, string);

      /* cleanup */
      g_value_unset (&dst);
    }
}



static gboolean
terminal_preferences_store_idle (gpointer user_data)
{
  TerminalPreferences  *preferences = TERMINAL_PREFERENCES (user_data);
  const gchar          *blurb;
  GParamSpec           *pspec;
  XfceRc               *rc = NULL;
  GValue               *value;
  GValue                src = { 0, };
  guint                 n;
  gchar                *filename;

  /* try again later if we're loading */
  if (G_UNLIKELY (preferences->loading_in_progress))
    return TRUE;

  filename = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, TERMINALRC, TRUE);
  if (G_UNLIKELY (filename == NULL))
    goto error;

  rc = xfce_rc_simple_open (filename, FALSE);
  if (G_UNLIKELY (rc == NULL))
    goto error;

  xfce_rc_set_group (rc, "Configuration");

  for (n = PROP_0 + 1; n < N_PROPERTIES; ++n)
    {
      pspec = preferences_props[n];
      value = preferences->values + n;
      blurb = g_param_spec_get_blurb (pspec);

      if (G_IS_VALUE (value)
          && !g_param_value_defaults (pspec, value))
        {
          /* always save non-default values */
          terminal_preferences_store_value (value, blurb, rc);
          continue;
        }

      if (g_str_has_prefix (blurb, "Misc"))
        {
          /* store the hidden-properties' default value */
          g_value_init (&src, G_PARAM_SPEC_VALUE_TYPE (pspec));
          g_param_value_set_default (pspec, &src);
          terminal_preferences_store_value (&src, blurb, rc);
          g_value_unset (&src);
        }
      else
        {
          /* remove from the configuration */
          xfce_rc_delete_entry (rc, blurb, FALSE);
        }
    }

  /* check if verything has been written */
  xfce_rc_flush (rc);
  if (xfce_rc_is_dirty (rc))
    goto error;

  /* check if we need to update the monitor */
  terminal_preferences_monitor_connect (preferences, filename, TRUE);

  if (G_LIKELY (FALSE))
    {
error:
      g_warning ("Unable to store terminal preferences to \"%s\".", filename);
    }

  /* cleanup */
  g_free (filename);
  if (G_LIKELY (rc != NULL))
    xfce_rc_close (rc);

  return FALSE;
}



static void
terminal_preferences_store_idle_destroy (gpointer user_data)
{
  TERMINAL_PREFERENCES (user_data)->store_idle_id = 0;
}



static void
terminal_preferences_monitor_changed (GFileMonitor        *monitor,
                                      GFile               *file,
                                      GFile               *other_file,
                                      GFileMonitorEvent    event_type,
                                      TerminalPreferences *preferences)
{
  GFileInfo *info;
  guint64    mtime = 0;

  terminal_return_if_fail (G_IS_FILE_MONITOR (monitor));
  terminal_return_if_fail (TERMINAL_IS_PREFERENCES (preferences));
  terminal_return_if_fail (G_IS_FILE (file));

  /* xfce rc rewrites the file, so skip other events */
  if (G_UNLIKELY (preferences->loading_in_progress))
    return;

  /* get the last modified timestamp from the file */
  info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                            G_FILE_QUERY_INFO_NONE, NULL, NULL);
  if (G_LIKELY (info != NULL))
    {
      mtime = g_file_info_get_attribute_uint64 (info,
          G_FILE_ATTRIBUTE_TIME_MODIFIED);
      g_object_unref (G_OBJECT (info));
    }

  /* reload the preferences if the new mtime is newer */
  if (G_UNLIKELY (mtime > preferences->last_mtime))
    {
      terminal_preferences_load (preferences);

      /* set new mtime */
      preferences->last_mtime = mtime;
    }
}



static void
terminal_preferences_monitor_disconnect (TerminalPreferences *preferences)
{
  /* release the old file and monitor */
  if (G_LIKELY (preferences->file != NULL))
    {
      g_object_unref (G_OBJECT (preferences->file));
      preferences->file = NULL;
    }

  if (G_LIKELY (preferences->monitor != NULL))
    {
      g_file_monitor_cancel (preferences->monitor);
      g_object_unref (G_OBJECT (preferences->monitor));
      preferences->monitor = NULL;
    }
}



static void
terminal_preferences_monitor_connect (TerminalPreferences *preferences,
                                      const gchar         *filename,
                                      gboolean             update_mtime)
{
  GError    *error = NULL;
  GFileInfo *info;
  GFile     *new_file;

  /* get new file location */
  new_file = g_file_new_for_path (filename);

  /* check if we need to start or update file monitoring */
  if (preferences->file == NULL
      || !g_file_equal (new_file, preferences->file))
    {
      /* disconnect old monitor */
      terminal_preferences_monitor_disconnect (preferences);

      /* create new local file */
      preferences->file = g_object_ref (new_file);

      /* monitor the file */
      preferences->monitor = g_file_monitor_file (preferences->file,
                                                  G_FILE_MONITOR_NONE,
                                                  NULL, &error);
      if (G_LIKELY (preferences->monitor != NULL))
        {
          /* connect signal */
#ifdef G_ENABLE_DEBUG
          g_debug ("Monitoring \"%s\" for changes.", filename);
#endif
          g_signal_connect (G_OBJECT (preferences->monitor), "changed",
                            G_CALLBACK (terminal_preferences_monitor_changed),
                            preferences);
        }
      else
        {
          g_critical ("Failed to setup monitoring for file \"%s\": %s",
                      filename, error->message);
          g_error_free (error);
        }
    }

  g_object_unref (new_file);

  preferences->last_mtime = 0;

  /* store the last known mtime */
  if (preferences->file != NULL
      && update_mtime)
    {
      info = g_file_query_info (preferences->file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                G_FILE_QUERY_INFO_NONE, NULL, NULL);
      if (G_LIKELY (info != NULL))
        {
          preferences->last_mtime = g_file_info_get_attribute_uint64 (info,
              G_FILE_ATTRIBUTE_TIME_MODIFIED);
          g_object_unref (G_OBJECT (info));
        }
    }
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

  terminal_return_val_if_fail (TERMINAL_IS_PREFERENCES (preferences), FALSE);

  g_object_get (G_OBJECT (preferences), property, &spec, NULL);
  if (G_LIKELY (spec != NULL))
    succeed = gdk_rgba_parse (color_return, spec);
  g_free (spec);

  return succeed;
}
