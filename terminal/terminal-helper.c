/* $Id$ */
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

#include <terminal/terminal-helper.h>



struct _TerminalHelperClass
{
  GObjectClass __parent__;
};

struct _TerminalHelper
{
  GObject                __parent__;
  TerminalHelperCategory category;

  gchar                 *id;
  gchar                 *name;
  gchar                 *command;
  gchar                **icons;

  GdkPixbuf             *icon;
};


static void            terminal_helper_class_init (TerminalHelperClass *klass);
static void            terminal_helper_finalize   (GObject             *object);
static TerminalHelper *terminal_helper_load       (XfceRc              *rc,
                                                   const gchar         *id);
static gint            terminal_helper_compare    (gconstpointer        a,
                                                   gconstpointer        b);



static GObjectClass *helper_parent_class;



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

  helper_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_helper_finalize;
}



static void
terminal_helper_finalize (GObject *object)
{
  TerminalHelper *helper = TERMINAL_HELPER (object);

  if (G_LIKELY (helper->icon != NULL))
    g_object_unref (G_OBJECT (helper->icon));

  g_strfreev (helper->icons);
  g_free (helper->command);
  g_free (helper->name);
  g_free (helper->id);

  G_OBJECT_CLASS (helper_parent_class)->finalize (object);
}



static TerminalHelper*
terminal_helper_load (XfceRc      *rc,
                      const gchar *id)
{
  TerminalHelperCategory category;
  TerminalHelper        *helper;
  const gchar           *command;
  const gchar           *name;
  const gchar           *p;
  const gchar           *s;
  gchar                **binaries;
  gchar                 *path = NULL;
  gchar                 *t;
  guint                  n;

  xfce_rc_set_group (rc, id);

  /* check if Command and Name are present */
  command = xfce_rc_read_entry_untranslated (rc, "Command", NULL);
  name = xfce_rc_read_entry (rc, "Name", NULL);
  if (G_UNLIKELY (name == NULL || command == NULL))
    return NULL;

  /* check for a valid Category */
  s = xfce_rc_read_entry_untranslated (rc, "Category", NULL);
  if (exo_str_is_equal (s, "WebBrowser"))
    category = TERMINAL_HELPER_WEBBROWSER;
  else if (exo_str_is_equal (s, "MailReader"))
    category = TERMINAL_HELPER_MAILREADER;
  else
    return NULL;

  /* check if Binaries is needed and present */
  binaries = xfce_rc_read_list_entry (rc, "Binaries", ";");
  if (strstr (command, "%B") != NULL)
    {
      if (G_UNLIKELY (binaries == NULL))
        return NULL;

      /* check if atleast one of the binaries is present */
      for (n = 0; binaries[n] != NULL; ++n)
        {
          /* skip empty binaries */
          for (s = binaries[n]; isspace (*s); ++s);
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
  helper->icons = xfce_rc_read_list_entry (rc, "Icons", ";");

  if (G_UNLIKELY (path == NULL))
    {
      helper->command = g_strdup (command);
    }
  else
    {
      helper->command = g_new (gchar, strlen (command) * strlen (path) + 1);
      for (s = command, t = helper->command; *s != '\0'; )
        {
          if (s[0] == '%' && toupper (s[1]) == 'B')
            {
              for (p = path; *p != '\0'; )
                *t++ = *p++;
              s += 2;
            }
          else
            *t++ = *s++;
        }
      g_free (path);
    }

  return helper;
}



static gint
terminal_helper_compare (gconstpointer a,
                         gconstpointer b)
{
  return strcmp (TERMINAL_HELPER (a)->name,
                 TERMINAL_HELPER (b)->name);
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
  g_return_val_if_fail (TERMINAL_IS_HELPER (helper), NULL);
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
  g_return_val_if_fail (TERMINAL_IS_HELPER (helper), NULL);
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
  g_return_val_if_fail (TERMINAL_IS_HELPER (helper), NULL);
  return helper->command;
}



/**
 * terminal_helper_get_icon:
 * @helper : A #TerminalHelper.
 *
 * Return value:
 **/
GdkPixbuf*
terminal_helper_get_icon (TerminalHelper *helper)
{
  guint n;
  gint  size;

  g_return_val_if_fail (TERMINAL_IS_HELPER (helper), NULL);

  if (helper->icon == NULL && helper->icons != NULL)
    {
      if (!gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &size, &size))
        size = 16;

      for (n = 0; helper->icons[n] != NULL; ++n)
        {
          helper->icon = xfce_themed_icon_load (helper->icons[n], size);
          if (G_LIKELY (helper->icon != NULL))
            break;
        }
    }

  return helper->icon;
}




struct _TerminalHelperDatabaseClass
{
  GObjectClass __parent__;
};

struct _TerminalHelperDatabase
{
  GObject  __parent__;
  XfceRc  *helpers;
};



static void terminal_helper_database_class_init (TerminalHelperDatabaseClass  *klass);
static void terminal_helper_database_init       (TerminalHelperDatabase       *database);
static void terminal_helper_database_finalize   (GObject                      *object);


static GObjectClass *database_parent_class;



G_DEFINE_TYPE (TerminalHelperDatabase, terminal_helper_database, G_TYPE_OBJECT);



static void
terminal_helper_database_class_init (TerminalHelperDatabaseClass *klass)
{
  GObjectClass *gobject_class;

  database_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_helper_database_finalize;
}



static void
terminal_helper_database_init (TerminalHelperDatabase *database)
{
  xfce_resource_push_path (XFCE_RESOURCE_DATA, DATADIR);
  database->helpers = xfce_rc_config_open (XFCE_RESOURCE_DATA,
                                           "Terminal/helpers.ini",
                                           TRUE);
  xfce_resource_pop_path (XFCE_RESOURCE_DATA);
}



static void
terminal_helper_database_finalize (GObject *object)
{
  TerminalHelperDatabase *database = TERMINAL_HELPER_DATABASE (object);

  xfce_rc_close (database->helpers);

  G_OBJECT_CLASS (database_parent_class)->finalize (object);
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

  g_return_val_if_fail (TERMINAL_IS_HELPER_DATABASE (database), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  if (!xfce_rc_has_group (database->helpers, id))
    return NULL;

  helper = terminal_helper_load (database->helpers, id);
  if (G_UNLIKELY (helper == NULL))
    return NULL;
  
  if (terminal_helper_get_category (helper) != category)
    {
      g_object_unref (G_OBJECT (helper));
      return NULL;
    }

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
  gchar         **groups;
  guint           n;

  g_return_val_if_fail (TERMINAL_IS_HELPER_DATABASE (database), NULL);

  groups = xfce_rc_get_groups (database->helpers);
  for (n = 0; groups[n] != NULL; ++n)
    {
      helper = terminal_helper_load (database->helpers, groups[n]);
      if (G_UNLIKELY (helper == NULL))
        continue;

      if (terminal_helper_get_category (helper) == category)
        result = g_slist_insert_sorted (result, helper, terminal_helper_compare);
      else
        g_object_unref (G_OBJECT (helper));
    }
  g_strfreev (groups);

  return result;
}




