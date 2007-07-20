/* $Id$ */
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
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>

#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-helper.h>
#include <terminal/terminal-private.h>



struct _TerminalHelperClass
{
  GObjectClass __parent__;
};

struct _TerminalHelper
{
  GObject                __parent__;
  TerminalHelperCategory category;

  gboolean               hidden;

  gchar                 *id;
  gchar                 *icon;
  gchar                 *name;
  gchar                 *command;
};


static void            terminal_helper_class_init (TerminalHelperClass *klass);
static void            terminal_helper_finalize   (GObject             *object);
static TerminalHelper *terminal_helper_load       (XfceRc              *rc,
                                                   const gchar         *id);
static gint            terminal_helper_compare    (gconstpointer        a,
                                                   gconstpointer        b);



static GObjectClass *terminal_helper_parent_class;



GType
terminal_helper_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GTypeInfo info =
      {
        sizeof (TerminalHelperClass),
        NULL,
        NULL,
        (GClassInitFunc) terminal_helper_class_init,
        NULL,
        NULL,
        sizeof (TerminalHelper),
        0,
        NULL,
        NULL
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "TerminalHelper",
                                     &info, 0);
    }

  return type;
}



static void
terminal_helper_class_init (TerminalHelperClass *klass)
{
  GObjectClass *gobject_class;

  terminal_helper_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_helper_finalize;
}



static void
terminal_helper_finalize (GObject *object)
{
  TerminalHelper *helper = TERMINAL_HELPER (object);

  g_free (helper->command);
  g_free (helper->icon);
  g_free (helper->name);
  g_free (helper->id);

  (*G_OBJECT_CLASS (terminal_helper_parent_class)->finalize) (object);
}



static TerminalHelper*
terminal_helper_load (XfceRc      *rc,
                      const gchar *id)
{
  TerminalHelperCategory category;
  TerminalHelper        *helper;
  const gchar           *command;
  const gchar           *name;
  const gchar           *type;
  const gchar           *p;
  const gchar           *s;
  gchar                **binaries;
  gchar                 *path = NULL;
  gchar                 *t;
  guint                  n;

  xfce_rc_set_group (rc, "Desktop Entry");

  /* verify that we have an Application .desktop file here */
  type = xfce_rc_read_entry_untranslated (rc, "Type", NULL);
  if (!exo_str_is_equal (type, "Application"))
    return NULL;

  /* check if Command and Name are present */
  name = xfce_rc_read_entry (rc, "Name", NULL);
  command = xfce_rc_read_entry_untranslated (rc, "X-Terminal-Command", NULL);
  if (G_UNLIKELY (name == NULL || command == NULL))
    return NULL;

  /* check for a valid Category */
  s = xfce_rc_read_entry_untranslated (rc, "X-Terminal-Category", NULL);
  if (s != NULL && g_ascii_strcasecmp (s, "WebBrowser") == 0)
    category = TERMINAL_HELPER_WEBBROWSER;
  else if (s != NULL && g_ascii_strcasecmp (s, "MailReader") == 0)
    category = TERMINAL_HELPER_MAILREADER;
  else
    return NULL;

  /* check if Binaries is needed and present */
  if (strstr (command, "%B") != NULL)
    {
      binaries = xfce_rc_read_list_entry (rc, "X-Terminal-Binaries", ";");
      if (G_UNLIKELY (binaries == NULL))
        return NULL;

      /* check if atleast one of the binaries is present */
      for (n = 0; binaries[n] != NULL; ++n)
        {
          /* skip empty binaries */
          for (s = binaries[n]; g_ascii_isspace (*s); ++s);
          if (*s == '\0')
            continue;

          /* lookup binary in path */
          path = g_find_program_in_path (s);
          if (path != NULL) break;
          g_free (path);
        }

      g_strfreev (binaries);

      if (G_UNLIKELY (path == NULL))
        return NULL;
    }

  helper = g_object_new (TERMINAL_TYPE_HELPER, NULL);
  helper->category = category;
  helper->id = g_strdup (id);
  helper->name = g_strdup (name);
  helper->hidden = xfce_rc_read_bool_entry (rc, "NoDisplay", FALSE);

  s = xfce_rc_read_entry_untranslated (rc, "Icon", NULL);
  if (G_LIKELY (s != NULL))
    helper->icon = g_strdup (s);

  if (G_UNLIKELY (path == NULL || *path == '\0'))
    {
      helper->command = g_strdup (command);
    }
  else
    {
      helper->command = g_new (gchar, strlen (command) * strlen (path) + 1);
      for (s = command, t = helper->command; *s != '\0'; )
        {
          if (s[0] == '%' && g_ascii_tolower (s[1]) == 'b')
            {
              for (p = path; *p != '\0'; )
                *t++ = *p++;
              s += 2;
            }
          else
            *t++ = *s++;
        }
      *t = '\0';
    }
  g_free (path);

  return helper;
}



static gint
terminal_helper_compare (gconstpointer a,
                         gconstpointer b)
{
  return g_utf8_collate (TERMINAL_HELPER (a)->name,
                         TERMINAL_HELPER (b)->name);
}



/**
 * terminal_helper_is_hidden:
 * @helper  : A #TerminalHelper.
 *
 * Return value:
 **/
gboolean
terminal_helper_is_hidden (TerminalHelper *helper)
{
  _terminal_return_val_if_fail (TERMINAL_IS_HELPER (helper), TRUE);
  return helper->hidden;
}



/**
 * terminal_helper_get_category:
 * @helper : A #TerminalHelper.
 *
 * Return value:
 **/
TerminalHelperCategory
terminal_helper_get_category (TerminalHelper *helper)
{
  _terminal_return_val_if_fail (TERMINAL_IS_HELPER (helper), TERMINAL_HELPER_WEBBROWSER);
  return helper->category;
}



/**
 * terminal_helper_get_id:
 * @helper : A #TerminalHelper.
 *
 * Return value:
 **/
const gchar*
terminal_helper_get_id (TerminalHelper *helper)
{
  _terminal_return_val_if_fail (TERMINAL_IS_HELPER (helper), NULL);
  return helper->id;
}



/**
 * terminal_helper_get_name:
 * @helper : A #TerminalHelper.
 *
 * Return value:
 **/
const gchar*
terminal_helper_get_name (TerminalHelper *helper)
{
  _terminal_return_val_if_fail (TERMINAL_IS_HELPER (helper), NULL);
  return helper->name;
}



/**
 * terminal_helper_get_command
 * @helper : A #TerminalHelper.
 *
 * Return value: the command associated with @helper.
 **/
const gchar*
terminal_helper_get_command (TerminalHelper *helper)
{
  _terminal_return_val_if_fail (TERMINAL_IS_HELPER (helper), NULL);
  return helper->command;
}



/**
 * terminal_helper_get_icon:
 * @helper : A #TerminalHelper.
 *
 * Return value:
 **/
const gchar*
terminal_helper_get_icon (TerminalHelper *helper)
{
  _terminal_return_val_if_fail (TERMINAL_IS_HELPER (helper), NULL);
  return helper->icon;
}



/**
 * terminal_helper_execute:
 * @helper  : A #TerminalHelper.
 * @screen  : A #GdkScreen or %NULL.
 * @uri     : The URI for the command.
 **/
void
terminal_helper_execute (TerminalHelper *helper,
                         GdkScreen      *screen,
                         const gchar    *uri)
{
  const gchar *s;
  const gchar *u;
  gchar       *argv[4];
  gchar       *command;
  gchar       *t;
  guint        n;

  _terminal_return_if_fail (TERMINAL_IS_HELPER (helper));
  _terminal_return_if_fail (GDK_IS_SCREEN (screen));
  _terminal_return_if_fail (uri != NULL);

  for (n = 0, s = helper->command; *s != '\0'; ++s)
    if (s[0] == '%' && g_ascii_tolower (s[1]) == 'u')
      ++n;

  if (n > 0)
    {
      command = g_new (gchar, strlen (helper->command) + n * strlen (uri) + 1);
      for (s = helper->command, t = command; *s != '\0'; )
        {
          if (s[0] == '%' && g_ascii_tolower (s[1]) == 'u')
            {
              for (u = uri; *u != '\0'; )
                *t++ = *u++;
              s += 2;
            }
          else
            {
              *t++ = *s++;
            }
        }
      *t = '\0';
    }
  else
    {
      command = g_strconcat (helper->command, " ", uri, NULL);
    }

  argv[0] = "/bin/sh";
  argv[1] = "-c";
  argv[2] = command;
  argv[3] = NULL;

  gdk_spawn_on_screen (screen, NULL, argv, NULL,
                       0, NULL, NULL, NULL, NULL);

  g_free (command);
}




struct _TerminalHelperDatabaseClass
{
  GObjectClass __parent__;
};

struct _TerminalHelperDatabase
{
  GObject     __parent__;
  GHashTable *helpers;
};



static void terminal_helper_database_class_init (TerminalHelperDatabaseClass *klass);
static void terminal_helper_database_init       (TerminalHelperDatabase      *database);
static void terminal_helper_database_finalize   (GObject                     *object);



G_DEFINE_TYPE (TerminalHelperDatabase, terminal_helper_database, G_TYPE_OBJECT);



static void
terminal_helper_database_class_init (TerminalHelperDatabaseClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_helper_database_finalize;
}



static void
terminal_helper_database_init (TerminalHelperDatabase *database)
{
  database->helpers = g_hash_table_new_full (g_str_hash,
                                             g_str_equal,
                                             g_free,
                                             g_object_unref);
}



static void
terminal_helper_database_finalize (GObject *object)
{
  TerminalHelperDatabase *database = TERMINAL_HELPER_DATABASE (object);

  g_hash_table_destroy (database->helpers);

  (*G_OBJECT_CLASS (terminal_helper_database_parent_class)->finalize) (object);
}



/**
 * terminal_helper_database_get:
 *
 * Return value: 
 **/
TerminalHelperDatabase*
terminal_helper_database_get (void)
{
  static TerminalHelperDatabase *default_database = NULL;

  if (default_database == NULL)
    {
      default_database = g_object_new (TERMINAL_TYPE_HELPER_DATABASE, NULL);
      g_object_add_weak_pointer (G_OBJECT (default_database),
                                 (gpointer) &default_database);
    }
  else
    {
      g_object_ref (G_OBJECT (default_database));
    }

  return default_database;
}



/**
 * terminal_helper_database_lookup:
 * @database  : A #TerminalHelperDabase.
 * @category  : Helper category.
 * @name      : Helper id.
 *
 * Return value:
 **/
TerminalHelper*
terminal_helper_database_lookup (TerminalHelperDatabase *database,
                                 TerminalHelperCategory  category,
                                 const gchar            *id)
{
  TerminalHelper *helper;
  XfceRc         *rc;
  gchar          *file;
  gchar          *spec;

  _terminal_return_val_if_fail (TERMINAL_IS_HELPER_DATABASE (database), NULL);

  if (G_UNLIKELY (id == NULL))
    return NULL;

  spec = g_strconcat ("Terminal/apps/", id, ".desktop", NULL);

  /* try to find a cached version */
  helper = g_hash_table_lookup (database->helpers, spec);

  /* load helper from file */
  if (G_LIKELY (helper == NULL))
    {
      xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
      file = xfce_resource_lookup (XFCE_RESOURCE_DATA, spec);
      xfce_resource_pop_path (XFCE_RESOURCE_DATA);

      if (G_LIKELY (file != NULL))
        {
          rc = xfce_rc_simple_open (file, TRUE);
          helper = terminal_helper_load (rc, id);
          xfce_rc_close (rc);
          g_free (file);
        }

      /* add the loaded helper to the cache */
      if (G_LIKELY (helper != NULL))
        {
          g_hash_table_insert (database->helpers,
                               g_strdup (spec),
                               helper);
        }
    }

  if (G_LIKELY (helper != NULL))
    {
      if (terminal_helper_get_category (helper) == category)
        g_object_ref (G_OBJECT (helper));
      else
        helper = NULL;
    }

  g_free (spec);

  return helper;
}



/**
 * terminal_helper_database_lookup_all:
 * @database  : A #TerminalHelperDatabase.
 * @category  : Helper category.
 *
 * Returns all available helpers in @category, sorted in
 * alphabetic order.
 *
 * The returned list keeps references on the included
 * helpers, so be sure to run
 * <programlisting>
 *  g_slist_foreach (list, (GFunc) g_object_unref, NULL);
 *  g_slist_free (list);
 * </programlisting>
 * when you are done.
 *
 * Return value: The list of all helpers available
 *               in @category.
 **/
GSList*
terminal_helper_database_lookup_all (TerminalHelperDatabase *database,
                                     TerminalHelperCategory  category)
{
  TerminalHelper *helper;
  GSList         *result = NULL;
  gchar         **specs;
  gchar          *id;
  gchar          *s;
  guint           n;

  _terminal_return_val_if_fail (TERMINAL_IS_HELPER_DATABASE (database), NULL);

  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  specs = xfce_resource_match (XFCE_RESOURCE_DATA, "Terminal/apps/*.desktop", TRUE);
  xfce_resource_pop_path (XFCE_RESOURCE_DATA);

  for (n = 0; specs[n] != NULL; ++n)
    {
      s = strrchr (specs[n], '.');
      if (G_LIKELY (s != NULL))
        *s = '\0';

      id = strrchr (specs[n], '/');
      id = (id != NULL) ? id + 1 : specs[n];

      helper = terminal_helper_database_lookup (database, category, id);
      if (G_LIKELY (helper != NULL))
        result = g_slist_insert_sorted (result, helper, terminal_helper_compare);
    }
  g_strfreev (specs);

  return result;
}



/**
 * terminal_helper_database_get_custom:
 * @database  : A #TerminalHelperDatabase.
 * @category  : A #TerminalHelperCategory.
 **/
TerminalHelper*
terminal_helper_database_get_custom (TerminalHelperDatabase *database,
                                     TerminalHelperCategory  category)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  gchar       id[64];

  _terminal_return_val_if_fail (TERMINAL_IS_HELPER_DATABASE (database), NULL);
  _terminal_return_val_if_fail (category >= TERMINAL_HELPER_WEBBROWSER
                     && category <= TERMINAL_HELPER_MAILREADER, NULL);

  /* determine the id for the custom helper */
  enum_class = g_type_class_ref (TERMINAL_TYPE_HELPER_CATEGORY);
  enum_value = g_enum_get_value (enum_class, category);
  g_snprintf (id, 64, "custom-%s", enum_value->value_nick);
  g_type_class_unref (enum_class);

  return terminal_helper_database_lookup (database, category, id);
}



/**
 * terminal_helper_database_set_custom:
 * @database  : A #TerminalHelperDatabase.
 * @category  : A #TerminalHelperCategory.
 * @command   : The custom command.
 **/
void
terminal_helper_database_set_custom (TerminalHelperDatabase *database,
                                     TerminalHelperCategory  category,
                                     const gchar            *command)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  XfceRc     *rc;
  gchar       spec[128];
  gchar      *file;

  _terminal_return_if_fail (TERMINAL_IS_HELPER_DATABASE (database));
  _terminal_return_if_fail (command != NULL && *command != '\0');

  /* determine the spec for the custom helper */
  enum_class = g_type_class_ref (TERMINAL_TYPE_HELPER_CATEGORY);
  enum_value = g_enum_get_value (enum_class, category);
  g_snprintf (spec, 128, "Terminal/apps/custom-%s.desktop", enum_value->value_nick);

  /* lookup the resource save location */
  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  file = xfce_resource_save_location (XFCE_RESOURCE_DATA, spec, TRUE);
  xfce_resource_pop_path (XFCE_RESOURCE_DATA);

  /* write the custom helper file */
  rc = xfce_rc_simple_open (file, FALSE);
  if (G_LIKELY (rc != NULL))
    {
      xfce_rc_set_group (rc, "Desktop Entry");
      xfce_rc_write_bool_entry (rc, "NoDisplay", TRUE);
      xfce_rc_write_entry (rc, "Type", "Application");
      xfce_rc_write_entry (rc, "Name", command);
      xfce_rc_write_entry (rc, "X-Terminal-Category", enum_value->value_nick);
      xfce_rc_write_entry (rc, "X-Terminal-Command", command);
      xfce_rc_close (rc);
    }

  /* ditch any cached object */
  g_hash_table_remove (database->helpers, spec);

  g_type_class_unref (enum_class);
  g_free (file);
}

