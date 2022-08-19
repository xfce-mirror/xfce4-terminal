/*-
 * Copyright 2022 Amrit Borah <Elessar1802@xfce.org>
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

#ifndef TERMINAL_PROFILE_SELECTOR_H
#define TERMINAL_PROFILE_SELECTOR_H

#include <libxfce4ui/libxfce4ui.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_PROFILE_SELECTOR            (terminal_profile_selector_get_type ())
#define TERMINAL_PROFILE_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_PROFILE_SELECTOR, TerminalProfileSelector))
#define TERMINAL_PROFILE_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_PROFILE_SELECTOR, TerminalProfileSelectorClass))
#define TERMINAL_IS_PROFILE_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_PROFILE_SELECTOR))
#define TERMINAL_IS_PROFILE_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_PROFILE_SELECTOR))
#define TERMINAL_PROFILE_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_PROFILE_SELECTOR, TerminalProfileSelectorClass))

typedef struct _TerminalProfileSelector      TerminalProfileSelector;
typedef struct _TerminalProfileSelectorClass TerminalProfileSelectorClass;

GtkWidget      *terminal_profile_selector_new     (void);

G_END_DECLS

#endif /* !TERMINAL_SEARCH_DIALOG_H */
