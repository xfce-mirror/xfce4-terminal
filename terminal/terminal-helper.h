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

#ifndef __TERMINAL_HELPER_H__
#define __TERMINAL_HELPER_H__

#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS;

typedef enum /*< enum,prefix=TERMINAL_HELPER >*/
{
  TERMINAL_HELPER_WEBBROWSER,
  TERMINAL_HELPER_MAILREADER,
} TerminalHelperCategory;


#define TERMINAL_TYPE_HELPER            (terminal_helper_get_type ())
#define TERMINAL_HELPER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_HELPER, TerminalHelper))
#define TERMINAL_HELPER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_HELPER, TerminalHelperClass))
#define TERMINAL_IS_HELPER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_HELPER))
#define TERMINAL_IS_HELPER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_HELPER))
#define TERMINAL_HELPER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_HELPER, TerminalHelperClass))

typedef struct _TerminalHelperClass TerminalHelperClass;
typedef struct _TerminalHelper      TerminalHelper;

GType                   terminal_helper_get_type      (void) G_GNUC_CONST;
TerminalHelperCategory  terminal_helper_get_category  (TerminalHelper *helper);
const gchar            *terminal_helper_get_id        (TerminalHelper *helper);
const gchar            *terminal_helper_get_name      (TerminalHelper *helper);
const gchar            *terminal_helper_get_command   (TerminalHelper *helper);
GdkPixbuf              *terminal_helper_get_icon      (TerminalHelper *helper);


#define TERMINAL_TYPE_HELPER_DATABASE             (terminal_helper_database_get_type ())
#define TERMINAL_HELPER_DATABASE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_HELPER_DATABASE, TerminalHelperDatabase))
#define TERMINAL_HELPER_DATABASE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_HELPER_DATABASE, TerminalHelperDatabaseClass))
#define TERMINAL_IS_HELPER_DATABASE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_HELPER_DATABASE))
#define TERMINAL_IS_HELPER_DATABASE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_HELPER_DATABASE))
#define TERMINAL_HELPER_DATABASE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_HELPER_DATABASE, TerminalHelperDatabaseClass))

typedef struct _TerminalHelperDatabaseClass TerminalHelperDatabaseClass;
typedef struct _TerminalHelperDatabase      TerminalHelperDatabase;

TerminalHelperDatabase  *terminal_helper_database_get         (void);
TerminalHelper          *terminal_helper_database_lookup      (TerminalHelperDatabase *database,
                                                               TerminalHelperCategory  category,
                                                               const gchar            *name);
GSList                  *terminal_helper_database_lookup_all  (TerminalHelperDatabase *database,
                                                               TerminalHelperCategory  category);


G_END_DECLS;

#endif /* !__TERMINAL_HELPER_H__ */
