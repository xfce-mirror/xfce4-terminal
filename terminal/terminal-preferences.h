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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TERMINAL_PREFERENCES_H
#define TERMINAL_PREFERENCES_H

#include <libxfce4ui/libxfce4ui.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_PREFERENCES             (terminal_preferences_get_type ())
#define TERMINAL_PREFERENCES(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_PREFERENCES, TerminalPreferences))
#define TERMINAL_PREFERENCES_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_PREFERENCES, TerminalPreferencesClass))
#define TERMINAL_IS_PREFERENCES(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_PREFERENCES))
#define TERMINAL_IS_PREFERENCES_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_PREFERENCES))
#define TERMINAL_PREFERENCES_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_PREFERENCES, TerminalPreferencesClass))

typedef struct _TerminalPreferencesClass TerminalPreferencesClass;
typedef struct _TerminalPreferences      TerminalPreferences;

typedef enum /*< enum,prefix=TERMINAL_SCROLLBAR >*/
{
  TERMINAL_SCROLLBAR_NONE,
  TERMINAL_SCROLLBAR_LEFT,
  TERMINAL_SCROLLBAR_RIGHT
} TerminalScrollbar;

typedef enum /*< enum,prefix=TERMINAL_TITLE >*/
{
  TERMINAL_TITLE_REPLACE,
  TERMINAL_TITLE_PREPEND,
  TERMINAL_TITLE_APPEND,
  TERMINAL_TITLE_HIDE,
  TERMINAL_TITLE_DEFAULT
} TerminalTitle;

typedef enum /*< enum,prefix=TERMINAL_BACKGROUND >*/
{
  TERMINAL_BACKGROUND_SOLID,
  TERMINAL_BACKGROUND_IMAGE,
  TERMINAL_BACKGROUND_TRANSPARENT
} TerminalBackground;

typedef enum /*< enum,prefix=TERMINAL_BACKGROUND_STYLE >*/
{
  TERMINAL_BACKGROUND_STYLE_TILED,
  TERMINAL_BACKGROUND_STYLE_CENTERED,
  TERMINAL_BACKGROUND_STYLE_SCALED,
  TERMINAL_BACKGROUND_STYLE_STRETCHED,
  TERMINAL_BACKGROUND_STYLE_FILLED
} TerminalBackgroundStyle;

typedef enum /*< enum,prefix=TERMINAL_ERASE_BINDING >*/
{
  TERMINAL_ERASE_BINDING_AUTO,
  TERMINAL_ERASE_BINDING_ASCII_DELETE,    /* ASCII DEL */
  TERMINAL_ERASE_BINDING_DELETE_SEQUENCE, /* Escape Sequence */
  TERMINAL_ERASE_BINDING_ASCII_BACKSPACE, /* Control-H */
  TERMINAL_ERASE_BINDING_ERASE_TTY        /* TTY Erase */
} TerminalEraseBinding;

typedef enum /*< enum,prefix=TERMINAL_AMBIGUOUS_WIDTH_BINDING >*/
{
  TERMINAL_AMBIGUOUS_WIDTH_BINDING_NARROW,
  TERMINAL_AMBIGUOUS_WIDTH_BINDING_WIDE
} TerminalAmbiguousWidthBinding;

typedef enum /*< enum,prefix=TERMINAL_CURSOR_SHAPE >*/
{
  TERMINAL_CURSOR_SHAPE_BLOCK,
  TERMINAL_CURSOR_SHAPE_IBEAM,
  TERMINAL_CURSOR_SHAPE_UNDERLINE
} TerminalCursorShape;

typedef enum /*< enum,prefix=TERMINAL_TEXT_BLINK_MODE >*/
{
  TERMINAL_TEXT_BLINK_MODE_NEVER,
  TERMINAL_TEXT_BLINK_MODE_FOCUSED,
  TERMINAL_TEXT_BLINK_MODE_UNFOCUSED,
  TERMINAL_TEXT_BLINK_MODE_ALWAYS
} TerminalTextBlinkMode;

typedef enum /*< enum,prefix=TERMINAL_RIGHT_CLICK_ACTION >*/
{
    TERMINAL_RIGHT_CLICK_ACTION_CONTEXT_MENU,
    TERMINAL_RIGHT_CLICK_ACTION_PASTE_CLIPBOARD,
    TERMINAL_RIGHT_CLICK_ACTION_PASTE_SELECTION
} TerminalRightClickAction;

GType                terminal_preferences_get_type            (void) G_GNUC_CONST;

TerminalPreferences *terminal_preferences_get                 (void);

gboolean             terminal_preferences_get_color           (TerminalPreferences *preferences,
                                                               const gchar         *property,
                                                               GdkRGBA             *color_return);
void                 terminal_preferences_xfconf_init_failed  (void);


G_END_DECLS

#endif /* !TERMINAL_PREFERENCES_H */
