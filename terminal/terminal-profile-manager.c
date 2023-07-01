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
#include <terminal/terminal-private.h>



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
  if (no_xfconf ())
    return;

  manager->channel = xfconf_channel_get ("xfce4-terminal");

  if (!xfconf_channel_has_property (manager->channel, "/default-profile"))
  {
    g_print ("creating new profile\n");
    /* copy current user settings to a new profile */
    terminal_profile_manager_create_profile (manager, g_get_user_name (), "/", TRUE);

    xfconf_channel_set_string (manager->channel,
                               "/default-profile",
                               g_get_user_name ());
  }

  manager->default_profile = 
    xfconf_channel_get_string (manager->channel, 
                               "/default-profile",
                               g_get_user_name ());

  /* initialize with NULL; 
   * get_profiles fetches data from xfconf if NULL */
  manager->profiles = NULL;
  manager->profiles = terminal_profile_manager_get_profiles (manager);

  /* HashMap contains the following key, value pair; 
   * (profile_name (str), TerminalPreferences (obj with profile_name as active profile)) */
  manager->hashmap = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            g_free, NULL);

  /* Initialize the hashmap with the profile names with values NULL */
  for (GList *lp = manager->profiles; lp != NULL; lp = lp->next)
    g_hash_table_insert (manager->hashmap, g_strdup (lp->data), NULL);
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
  GHashTable *properties;
  GList      *keys;
  gchar     **split;
  GHashTable *set;

  if (manager->profiles != NULL)
    return manager->profiles;

  properties = xfconf_channel_get_properties (manager->channel, "/");
  keys = g_hash_table_get_keys (properties);
  set = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  for (GList *lp = keys; lp != NULL; lp = lp->next)
    {
      if (g_str_equal (lp->data, "/default-profile"))
        continue;
      split = g_strsplit (lp->data, "/", 3);
      if (!g_hash_table_contains (set, split[1]))
        {
          g_hash_table_add (set, g_strdup (split[1]));
          manager->profiles = g_list_prepend (manager->profiles, g_strdup (split[1]));
        }
      g_strfreev (split);
    }

  /* free resources */
  g_list_free (keys);
  g_hash_table_destroy (properties);
  g_hash_table_destroy (set);

  return manager->profiles;
}



TerminalPreferences*
terminal_profile_manager_get_profile (TerminalProfileManager *manager,
                                      const gchar            *name)
{
  TerminalPreferences *preferences = NULL;
  
  terminal_return_if_fail (TERMINAL_IS_PROFILE_MANAGER (manager));
  terminal_return_if_fail (name != NULL);
  
  preferences = g_hash_table_lookup (manager->hashmap, name);

  if (preferences == NULL)
    {
      preferences = terminal_preferences_new ();
      terminal_preferences_set_profile (preferences, name);
      g_hash_table_insert (manager->hashmap, g_strdup (name), preferences);
    }

  return g_object_ref (preferences);
}



gboolean
terminal_profile_manager_create_profile (TerminalProfileManager *manager,
                                         const gchar            *name,
                                         const gchar            *from,
                                         gboolean                delete_from)
{
  GValue value = G_VALUE_INIT;
  gchar *to_prop_name;
  gchar *to_prop_basename;
  gchar *from_prop_name;
  GHashTable *properties;
  GList *keys;
  gboolean success;

  terminal_return_if_fail (TERMINAL_IS_PROFILE_MANAGER (manager));
  terminal_return_if_fail (name != NULL);

  to_prop_basename = g_strdup_printf ("/%s", name);
  success = xfconf_channel_set_string (manager->channel, to_prop_basename, "Profile");
  g_free (to_prop_basename);

  if (from == NULL || !g_str_equal (from, "/"))
    return success;

  /* create a new profile from existing user preferences;
   * port old format to new format & delete the old data */
  from_prop_name = g_strdup_printf ("/%s", from);
  properties = xfconf_channel_get_properties (manager->channel,
                                              g_str_equal (from, "/") ? "/" : from_prop_name);
  g_free (from_prop_name);
  keys = g_hash_table_get_keys (properties);

  to_prop_basename = g_strdup_printf ("/%s", name);
  for (GList *lp = keys; lp != NULL; lp = lp->next)
  {
    if (g_str_equal (lp->data, to_prop_basename))
      continue;
    g_print ("%s\n", (gchar *) lp->data);
    /* lp->data is of the format "/prop_name" */
    to_prop_name = g_strdup_printf ("/%s%s", name, (gchar *) lp->data);
    g_value_init (&value, G_TYPE_NONE);
    xfconf_channel_get_property (manager->channel, lp->data, &value);
    xfconf_channel_set_property (manager->channel, to_prop_name, &value);
    if (delete_from)
      xfconf_channel_reset_property (manager->channel, lp->data, TRUE);
    g_value_unset (&value);
    g_free (to_prop_name);
  }
  g_free (to_prop_basename);

  g_list_free (keys);
  g_hash_table_destroy (properties);

  /* validate that the new profile was indeed created */
  to_prop_basename = g_strdup_printf ("/%s", name);
  success = xfconf_channel_has_property (manager->channel, to_prop_basename);
  g_free (to_prop_basename);

  return success;
}



gboolean
terminal_profile_manager_delete_profile(TerminalProfileManager *manager,
                                        const gchar            *name)
{
  GList *lp;
  TerminalPreferences *preferences;

  if (G_UNLIKELY (!terminal_profile_manager_has_profile (manager, name)))
    {
      g_warning ("Profile %s not found!", name);
      return FALSE;
    }

  if (g_str_equal (name, manager->default_profile))
    {
      g_warning (
        "Profile %s is set as default profile!\n"
        "A default profile cannot be deleted.\n"
        "Please set another profile as the default profile\n"
        "if you wish to delete this profile.", name);
      return FALSE;
    }
  
  lp = g_list_find_custom (manager->profiles, name, g_str_equal);
  g_assert (lp != NULL);
  manager->profiles = g_list_delete_link (manager->profiles, lp);

  preferences = g_hash_table_lookup (manager->hashmap, name);
  if (preferences != NULL)
    {
      terminal_preferences_set_profile (preferences, manager->default_profile);
      g_object_unref (preferences);
    }
  g_hash_table_remove (manager->hashmap, name);
  
  return TRUE;
}



gboolean
terminal_profile_manager_rename_profile (TerminalProfileManager *manager,
                                         const gchar            *from,
                                         const gchar            *to)
{
  TerminalPreferences *preferences;
  GList               *lp;

  if (G_UNLIKELY (!terminal_profile_manager_has_profile (manager, from)))
    {
      g_warning ("Profile %s not found!", from);
      return FALSE;
    }
  
  preferences = g_hash_table_lookup (manager->hashmap, from);
  g_hash_table_replace (manager->hashmap, g_strdup (to), preferences);
  lp = g_list_find_custom (manager->profiles, from, g_str_equal);
  terminal_profile_manager_create_profile (manager, to, from, TRUE);
  g_assert (lp != NULL);
  manager->profiles = g_list_delete_link (manager->profiles, lp);

  return TRUE;
}



gboolean
terminal_profile_manager_has_profile (TerminalProfileManager *manager,
                                      const gchar            *name)
{
  return g_hash_table_contains (manager->hashmap, name);
}



TerminalPreferences*
terminal_profile_manager_get_default_profile (TerminalProfileManager *manager)
{
  return terminal_profile_manager_get_profile (manager, manager->default_profile);
}



gchar*
terminal_profile_manager_get_default_profile_name (TerminalProfileManager *manager)
{
  return g_strdup (manager->default_profile);
}



gboolean
terminal_profile_manager_set_default_profile (TerminalProfileManager *manager,
                                              const gchar            *name)
{
  gboolean success;
  success = xfconf_channel_set_string (manager->channel, "/default-profile", name);
  if (!success) return FALSE;
  g_free (manager->default_profile);
  manager->default_profile = g_strdup (name);
  return success;
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