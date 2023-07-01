/*-
 * Copyright (c) 2023 Amrit Borah <elessar1802@gmail.com>
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

#ifndef TERMINAL_PROFILE_MANAGER_H
#define TERMINAL_PROFILE_MANAGER_H

#include <glib-object.h>
#include <terminal/terminal-preferences.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_PROFILE_MANAGER             (terminal_profile_manager_get_type ())
#define TERMINAL_PROFILE_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_PROFILE_MANAGER, TerminalProfileManager))
#define TERMINAL_IS_PROFILE_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_PROFILE_MANAGER))

typedef struct _TerminalProfileManagerClass TerminalProfileManagerClass;
typedef struct _TerminalProfileManager      TerminalProfileManager;

GType                        terminal_profile_manager_get_type                  (void) G_GNUC_CONST;
TerminalProfileManager      *terminal_profile_manager_get                       (void);
GList                       *terminal_profile_manager_get_profiles              (TerminalProfileManager *manager);
TerminalPreferences         *terminal_profile_manager_get_profile               (TerminalProfileManager *manager,
                                                                                 const gchar            *name);
gboolean                     terminal_profile_manager_create_profile            (TerminalProfileManager *manager,
                                                                                 const gchar            *name,
                                                                                 const gchar            *from,
                                                                                 gboolean                delete_from);
gboolean                     terminal_profile_manager_delete_profile            (TerminalProfileManager *manager,
                                                                                 const gchar            *name);
gboolean                     terminal_profile_manager_rename_profile            (TerminalProfileManager *manager,
                                                                                 const gchar            *from,
                                                                                 const gchar            *to);
gboolean                     terminal_profile_manager_has_profile               (TerminalProfileManager *manager,
                                                                                 const gchar            *name);
TerminalPreferences         *terminal_profile_manager_get_default_profile       (TerminalProfileManager *manager);
gchar                       *terminal_profile_manager_get_default_profile_name  (TerminalProfileManager *manager);
gboolean                     terminal_profile_manager_set_default_profile       (TerminalProfileManager *manager,
                                                                                 const gchar            *name);

G_END_DECLS

#endif /* !TERMINAL_PROFILE_MANAGER_H */
