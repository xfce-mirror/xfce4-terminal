/* $Id$ */
/*-
 * Copyright (c) 2004-2006 os-cillation e.K.
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

#include <terminal/terminal-accel-map.h>
#include <terminal/terminal-preferences.h>



static void terminal_accel_map_class_init   (TerminalAccelMapClass  *klass);
static void terminal_accel_map_init         (TerminalAccelMap       *map);
static void terminal_accel_map_finalize     (GObject                *object);
static void terminal_accel_map_notify       (TerminalPreferences    *preferences,
                                             GParamSpec             *pspec,
                                             TerminalAccelMap       *map);



struct _TerminalAccelMap
{
  GObject              __parent__;
  TerminalPreferences *preferences;
};



G_DEFINE_TYPE (TerminalAccelMap, terminal_accel_map, G_TYPE_OBJECT);



static void
terminal_accel_map_class_init (TerminalAccelMapClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_accel_map_finalize;
}



static void
terminal_accel_map_init (TerminalAccelMap *map)
{
  GParamSpec **specs;
  GParamSpec  *spec;
  gchar       *signal_name;
  guint        nspecs;
  guint        n;

  map->preferences = terminal_preferences_get ();

  specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (map->preferences), &nspecs);
  for (n = 0; n < nspecs; ++n)
    {
      spec = specs[n];
      if (!g_str_has_prefix (spec->name, "accel-"))
        continue;

      signal_name = g_strconcat ("notify::", spec->name, NULL);
      g_signal_connect (G_OBJECT (map->preferences), signal_name,
                        G_CALLBACK (terminal_accel_map_notify), map);
      g_free (signal_name);

      terminal_accel_map_notify (map->preferences, spec, map);
    }
  g_free (specs);
}



static void
terminal_accel_map_finalize (GObject *object)
{
  TerminalAccelMap *map = TERMINAL_ACCEL_MAP (object);

  g_signal_handlers_disconnect_by_func (G_OBJECT (map->preferences), G_CALLBACK (terminal_accel_map_notify), map);
  g_object_unref (G_OBJECT (map->preferences));

  (*G_OBJECT_CLASS (terminal_accel_map_parent_class)->finalize) (object);
}



static void
terminal_accel_map_notify (TerminalPreferences *preferences,
                           GParamSpec          *pspec,
                           TerminalAccelMap    *map)
{
  GdkModifierType accelerator_mods;
  gchar          *accelerator_path;
  guint           accelerator_key;
  gchar          *accelerator;

  g_return_if_fail (g_str_has_prefix (pspec->name, "accel-"));

  accelerator_path = g_strconcat ("<Actions>/terminal-window/", pspec->name + 6, NULL);

  g_object_get (G_OBJECT (preferences), pspec->name, &accelerator, NULL);
  if (G_LIKELY (accelerator != NULL))
    {
      gtk_accelerator_parse (accelerator, &accelerator_key, &accelerator_mods);
      gtk_accel_map_change_entry (accelerator_path,
                                  accelerator_key,
                                  accelerator_mods,
                                  TRUE);
      g_free (accelerator);
    }
  else
    {
      gtk_accel_map_change_entry (accelerator_path, 0, 0, TRUE);
    }

  g_free (accelerator_path);
}



/**
 * terminal_accel_map_new:
 *
 * Return value :
 **/
TerminalAccelMap*
terminal_accel_map_new (void)
{
  return g_object_new (TERMINAL_TYPE_ACCEL_MAP, NULL);
}
