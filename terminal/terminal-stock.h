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
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TERMINAL_STOCK_H__
#define __TERMINAL_STOCK_H__

G_BEGIN_DECLS

#define TERMINAL_STOCK_CLOSETAB     "terminal-closetab"
#define TERMINAL_STOCK_NEWTAB       "terminal-newtab"
#define TERMINAL_STOCK_NEWWINDOW    "terminal-newwindow"
#define TERMINAL_STOCK_REPORTBUG    "terminal-reportbug"
#define TERMINAL_STOCK_SHOWBORDERS  "terminal-showborders"
#define TERMINAL_STOCK_SHOWMENU     "terminal-showmenu"
#define TERMINAL_STOCK_COMPOSE      "terminal-compose"

gboolean terminal_stock_init (void);

G_END_DECLS

#endif /* !__TERMINAL_STOCK_H__ */
