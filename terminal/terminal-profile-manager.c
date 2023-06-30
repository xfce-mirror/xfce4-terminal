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

#include <xfconf/xfconf.h>
#include <terminal/terminal-profile-manager.h>
#include <terminal/terminal-app.h>



static void     terminal_profile_manager_finalize      (GObject             *object);



struct _TerminalProfileManagerClass
{
  GObjectClass   __parent__;
};

struct _TerminalProfileManager
{
  GObject        __parent__;

  XfconfChannel *channel;
  GList         *profiles;
  gchar         *default_profile;
  GHashTable    *hashmap;
  guint          N;
};



G_DEFINE_TYPE (TerminalProfileManager, terminal_profile_manager, G_TYPE_OBJECT)



static void
terminal_profile_manager_class_init (TerminalProfileManagerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_profile_manager_finalize;
}



static void
terminal_profile_manager_init (TerminalProfileManager *manager)
{
  if (no_xfconf())
    return;

  manager->channel = xfconf_channel_get ("xfce4-terminal");

  if (!xfconf_channel_has_property (manager->channel, "/default-profile"))
  {
    /* copy current user settings to a new profile */
    terminal_profile_manager_create_profile (manager, g_get_user_name (), "/");

    xfconf_channel_set_string (manager->channel, "/default-profile", g_get_user_name ());
  }

  manager->default_profile = xfconf_channel_get_string (manager->channel, "/default-profile", g_get_user_name ());
  manager->profiles = terminal_profile_manager_get_profiles (manager);
  manager->hashmap = g_hash_table_new (g_str_hash, g_str_equal);

  for (GList *lp = manager->profiles; lp != NULL; lp = lp->next)
    g_hash_table_add (manager->hashmap, g_strdup (lp->data));
}



static void
terminal_profile_manager_finalize (GObject *object)
{
  TerminalProfileManager *manager = TERMINAL_PROFILE_MANAGER (object);

  g_object_unref (manager->channel);

  (*G_OBJECT_CLASS (terminal_profile_manager_parent_class)->finalize) (object);
}



GList*
terminal_profile_manager_get_profiles (TerminalProfileManager *manager)
{
  return manager->profiles;
}



TerminalPreferences*
terminal_profile_manager_get_profile (TerminalProfileManager *manager,
                                      const gchar            *name)
{
  TerminalPreferences *preferences = g_hash_table_lookup (manager->hashmap, name);
  if (preferences == NULL && g_list_find_custom (manager->profiles, name, g_str_equal) != NULL)
    {
      preferences = terminal_preferences_new ();
      terminal_preferences_set_profile (preferences, name);
      g_hash_table_insert (manager->hashmap, (gpointer) name, preferences);
    }
  return g_object_ref (preferences);
}



gboolean
terminal_profile_manager_create_profile (TerminalProfileManager *manager,
                                         const gchar            *name,
                                         const gchar            *from)
{
  return TRUE;
}



gboolean
terminal_profile_manager_delete_profile(TerminalProfileManager *manager,
                                        const gchar            *name)
{
  /* TODO */
  if (g_str_equal (name, manager->default_profile))
    return FALSE;

  return TRUE;
}



gboolean
terminal_profile_manager_rename_profile (TerminalProfileManager *manager,
                                         const gchar            *from,
                                         const gchar            *to)
{
  if (G_UNLIKELY (!terminal_profile_manager_has_profile (manager, from)))
    {
      g_warning ("Profile %s not found!", from);
      return FALSE;
    }
  /* TODO */
  return TRUE;
}



gboolean
terminal_profile_manager_has_profile (TerminalProfileManager *manager,
                                      const gchar            *name)
{
  return g_list_find_custom (manager->profiles, name, g_str_equal) != NULL;
}



gchar*
terminal_profile_manager_get_default_profile (TerminalProfileManager *manager)
{
  return g_strdup (manager->default_profile);
}



gboolean
terminal_profile_manager_set_default_profile (TerminalProfileManager *manager,
                                              const gchar            *name)
{
  /* TODO */
  return TRUE;
}



TerminalProfileManager*
terminal_profile_manager_get (void)
{
  static TerminalProfileManager *manager = NULL;

  if (G_UNLIKELY (manager == NULL))
    {
      manager = g_object_new (TERMINAL_TYPE_PROFILE_MANAGER, NULL);
      g_object_add_weak_pointer (G_OBJECT (manager),
                                 (gpointer) &manager);
    }
  else
    {
      g_object_ref (G_OBJECT (manager));
    }

  return manager;
}
