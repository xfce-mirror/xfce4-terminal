/* $Id: terminal-window.h 99 2004-12-21 19:40:47Z bmeurer $ */
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
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <exo/exo.h>

#include <terminal/terminal-monitor.h>



static void     terminal_monitor_class_init   (TerminalMonitorClass *klass);
static void     terminal_monitor_finalize     (GObject              *object);
static gboolean terminal_monitor_idle         (gpointer              user_data);
static void     terminal_monitor_idle_destroy (gpointer              user_data);



typedef struct
{
  time_t  mtime;
  time_t  ctime;
  dev_t   device;
  ino_t   inode;
  gboolean  exists : 1;
} EntityInfo;

typedef struct
{
  gchar       *path;
  EntityInfo   info;
  GSList      *closures;
} MonitorEntity;

struct _TerminalMonitorClass
{
  GObjectClass __parent__;
};

struct _TerminalMonitor
{
  GObject   __parent__;
  GSList   *entities;
  guint     idle_id;
};



static GObjectClass *parent_class;



static void
entity_info_query (const gchar *path, EntityInfo *info)
{
  struct stat sb;

  if (stat (path, &sb) < 0)
    {
      info->exists = FALSE;
      info->mtime = (time_t) -1;
      info->ctime = (time_t) -1;
      info->inode = (ino_t) -1;
      info->device = (dev_t) -1;
    }
  else
    {
      info->exists  = TRUE;
      info->mtime   = sb.st_mtime;
      info->ctime   = sb.st_ctime;
      info->inode   = sb.st_ino;
      info->device  = sb.st_dev;
    }
}



static gboolean
entity_info_equals (const EntityInfo *a, 
                    const EntityInfo *b)
{
  return ((a->exists && b->exists) || (!a->exists && !b->exists))
      && (a->mtime == b->mtime)
      && (a->ctime == b->ctime)
      && (a->inode == b->inode)
      && (a->device == b->device);
}



GType
terminal_monitor_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    {
      static const GTypeInfo info =
      {
        sizeof (TerminalMonitorClass),
        NULL,
        NULL,
        (GClassInitFunc) terminal_monitor_class_init,
        NULL,
        NULL,
        sizeof (TerminalMonitor),
        0,
        NULL,
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "TerminalMonitor",
                                     &info, 0);
    }

  return type;
}



static void
terminal_monitor_class_init (TerminalMonitorClass *klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_monitor_finalize;
}



static void
terminal_monitor_finalize (GObject *object)
{
  TerminalMonitor *monitor = TERMINAL_MONITOR (object);
  MonitorEntity   *entity;
  GSList          *lp;

  for (lp = monitor->entities; lp != NULL; lp = lp->next)
    {
      entity = lp->data;

      g_slist_foreach (entity->closures, (GFunc) g_closure_unref, NULL);
      g_slist_free (entity->closures);

      g_free (entity->path);
      g_free (entity);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}



static gboolean
terminal_monitor_idle (gpointer user_data)
{
  typedef struct
  {
    gchar    *path;
    GClosure *closure;
  } CallerData;

  TerminalMonitor *monitor = TERMINAL_MONITOR (user_data);
  MonitorEntity   *entity;
  CallerData      *cdata;
  EntityInfo       info;
  GValue           values[2] = { { 0, }, { 0, } };
  GSList          *scheduled_calls = NULL;
  GSList          *elp;
  GSList          *clp;

  for (elp = monitor->entities; elp != NULL; elp = elp->next)
    {
      entity = elp->data;

      entity_info_query (entity->path, &info);

      if (!entity_info_equals (&info, &entity->info))
        {
          for (clp = entity->closures; clp != NULL; clp = clp->next)
            {
              cdata = g_new (CallerData, 1);
              cdata->path = g_strdup (entity->path);
              cdata->closure = clp->data;
              g_closure_ref (cdata->closure);
              scheduled_calls = g_slist_prepend (scheduled_calls, cdata);
            }
        }

      entity->info = info;
    }

  if (G_UNLIKELY (scheduled_calls != NULL))
    {
      g_value_init (&values[0], TERMINAL_TYPE_MONITOR);
      g_value_init (&values[1], G_TYPE_STRING);

      g_value_set_object (&values[0], monitor);

      for (clp = scheduled_calls; clp != NULL; clp = clp->next)
        {
          cdata = clp->data;

          g_value_take_string (&values[1], cdata->path);

          /* Make sure the closure is still registered with the monitor */
          for (elp = monitor->entities; elp != NULL; elp = elp->next)
            {
              entity = elp->data;
              if (exo_str_is_equal (entity->path, cdata->path)
                  && g_slist_find (entity->closures, cdata->closure) != NULL)
                {
                  g_closure_invoke (cdata->closure, NULL, 2, values, NULL);
                  break;
                }
            }

          g_closure_unref (cdata->closure);
          g_value_reset (&values[1]);
          g_free (cdata);
        }

      g_value_unset (&values[1]);
      g_value_unset (&values[0]);

      g_slist_free (scheduled_calls);
    }

  return (monitor->entities != NULL);
}



static void
terminal_monitor_idle_destroy (gpointer user_data)
{
  TERMINAL_MONITOR (user_data)->idle_id = 0;
}



/**
 * terminal_monitor_get:
 *
 * Return value:
 **/
TerminalMonitor*
terminal_monitor_get (void)
{
  static TerminalMonitor *monitor = NULL;

  if (G_UNLIKELY (monitor == NULL))
    {
      monitor = g_object_new (TERMINAL_TYPE_MONITOR, NULL);
      g_object_add_weak_pointer (G_OBJECT (monitor), (gpointer) &monitor);
    }
  else
    {
      g_object_ref (G_OBJECT (monitor));
    }

  return monitor;
}



/**
 * terminal_monitor_add:
 * @monitor : A #TerminalMonitor.
 * @path    : A path.
 * @closure : A #GClosure.
 **/
void
terminal_monitor_add (TerminalMonitor *monitor,
                      const gchar     *path,
                      GClosure        *closure)
{
  MonitorEntity *entity = NULL;
  GSList        *lp;

  g_return_if_fail (TERMINAL_IS_MONITOR (monitor));
  g_return_if_fail (path != NULL);
  g_return_if_fail (closure != NULL);

  for (lp = monitor->entities; lp != NULL; lp = lp->next)
    {
      entity = lp->data;
      if (exo_str_is_equal (entity->path, path))
        break;
    }

  if (G_LIKELY (entity == NULL))
    {
      entity = g_new (MonitorEntity, 1);
      entity->path = g_strdup (path);
      entity->closures = NULL;

      entity_info_query (path, &entity->info);

      monitor->entities = g_slist_prepend (monitor->entities, entity);
    }

  if (g_slist_find (entity->closures, closure) == NULL)
    {
      entity->closures = g_slist_prepend (entity->closures, closure);
      g_closure_ref (closure);
    }

  if (G_UNLIKELY (monitor->idle_id == 0))
    {
      monitor->idle_id = g_timeout_add_full (G_PRIORITY_LOW, 1000, terminal_monitor_idle,
                                             monitor, terminal_monitor_idle_destroy);
    }
}


/**
 * terminal_monitor_remove_by_closure:
 * @monitor : A #TerminalMonitor.
 * @closure : A #GClosure.
 **/
void
terminal_monitor_remove_by_closure (TerminalMonitor *monitor,
                                    GClosure        *closure)
{
  MonitorEntity *entity;
  GSList        *entities = NULL;
  GSList        *link;
  GSList        *lp;

  g_return_if_fail (TERMINAL_IS_MONITOR (monitor));
  g_return_if_fail (closure != NULL);

  for (lp = monitor->entities; lp != NULL; lp = lp->next)
    {
      entity = lp->data;

      link = g_slist_find (entity->closures, closure);
      if (link != NULL)
        {
          entity->closures = g_slist_delete_link (entity->closures, link);
          g_closure_unref (closure);

          if (entity->closures != NULL)
            {
              entities = g_slist_append (entities, entity);
            }
          else
            {
              g_free (entity->path);
              g_free (entity);
            }
        }
    }

  g_slist_free (monitor->entities);
  monitor->entities = entities;
}

