/*-
 * Copyright (c) 2004-2005 os-cillation e.K.
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

#ifndef TERMINAL_WIDGET_H
#define TERMINAL_WIDGET_H

#include <libxfce4ui/libxfce4ui.h>
#include <vte/vte.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_WIDGET (terminal_widget_get_type ())
G_DECLARE_FINAL_TYPE (TerminalWidget, terminal_widget, TERMINAL, WIDGET, VteTerminal)

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
  TARGET_GTK_NOTEBOOK_TAB
};

typedef enum
{
  TERMINAL_WIDGET_ACTION_SCROLL_UP,
  TERMINAL_WIDGET_ACTION_SCROLL_DOWN,
  TERMINAL_WIDGET_ACTION_SCROLL_PAGE_UP,
  TERMINAL_WIDGET_ACTION_SCROLL_PAGE_DOWN,
  TERMINAL_WIDGET_ACTION_N
} TerminalWidgetAction;

XfceGtkActionEntry *
terminal_widget_get_action_entries (void);

G_END_DECLS

#endif /* !TERMINAL_WIDGET_H */
