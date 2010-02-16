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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>

#include <terminal/terminal-stock.h>


typedef struct
{
  const gchar *name;
  const gchar *stock;
} TerminalStockIcon;



static const TerminalStockIcon terminal_stock_icons[] =
{
  { TERMINAL_STOCK_CLOSETAB,    "tab-close" },
  { TERMINAL_STOCK_NEWTAB,      "tab-new" },
  { TERMINAL_STOCK_NEWWINDOW,   "window-new" },
  { TERMINAL_STOCK_REPORTBUG,   NULL },
  { TERMINAL_STOCK_SHOWBORDERS, NULL },
  { TERMINAL_STOCK_SHOWMENU,    NULL },
  { TERMINAL_STOCK_COMPOSE,     NULL }
};



/**
 * terminal_stock_init:
 *
 * This function sets up the terminal stock icons.
 **/
gboolean
terminal_stock_init (void)
{
  GtkIconTheme   *icon_theme;
  GtkIconFactory *icon_factory;
  GtkIconSource  *icon_source;
  GtkIconSet     *icon_set;
  gchar           icon_name[50];
  guint           n;

  /* allocate a new icon factory for the terminal icons */
  icon_factory = gtk_icon_factory_new ();

  /* we try to avoid allocating multiple icon sources */
  icon_source = gtk_icon_source_new ();

  /* get default icon theme */
  icon_theme = gtk_icon_theme_get_default ();

  /* register our stock icons */
  for (n = 0; n < G_N_ELEMENTS (terminal_stock_icons); ++n)
    {
      /* set the new icon name for the icon source */
      g_snprintf (icon_name, sizeof (icon_name), "stock_%s",
                  terminal_stock_icons[n].name);

      /* allocate the icon set */
      icon_set = gtk_icon_set_new ();
      gtk_icon_source_set_icon_name (icon_source, icon_name);
      gtk_icon_set_add_source (icon_set, icon_source);

      /* add an alternative stock name if there is one */
      if (terminal_stock_icons[n].stock != NULL
          && gtk_icon_theme_has_icon (icon_theme,
                 terminal_stock_icons[n].stock))
        {
          gtk_icon_source_set_icon_name (icon_source,
              terminal_stock_icons[n].stock);
          gtk_icon_set_add_source (icon_set, icon_source);
        }

      /* add to the factory */
      gtk_icon_factory_add (icon_factory,
          terminal_stock_icons[n].name, icon_set);
      gtk_icon_set_unref (icon_set);
    }

  /* register the icon factory as default */
  gtk_icon_factory_add_default (icon_factory);

  /* cleanup */
  g_object_unref (G_OBJECT (icon_factory));
  gtk_icon_source_free (icon_source);

  return gtk_icon_theme_has_icon (icon_theme, "utilities-terminal");
}



