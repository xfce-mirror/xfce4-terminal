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
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <terminal/stock-icons/terminal-stock-pixbufs.h>
#include <terminal/terminal-icons.h>



static GtkIconFactory *icons_factory = NULL;



/**
 * terminal_icons_setup_helper:
 *
 * This functions sets up the helper stock icons used
 * for Terminal.
 **/
void
terminal_icons_setup_helper (void)
{
  static gboolean already_done = FALSE;
  GtkIconSource  *source;
  GtkIconSet     *set;
  GdkPixbuf      *pixbuf;

  if (already_done) return;
  already_done = TRUE;

  /* be sure to setup the main icons first */
  terminal_icons_setup_main ();
  g_assert (icons_factory != NULL);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_webbrowser_48), stock_webbrowser_48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-webbrowser", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_mailreader_48), stock_mailreader_48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-mailreader", set);
  gtk_icon_set_unref (set);
}



/**
 * terminal_icons_setup_main:
 *
 * This functions sets up the main stock icons used
 * for Terminal. These are the icons required for the
 * menubar. In addition, this function registers the
 * default window icon for Gdk.
 **/
void
terminal_icons_setup_main (void)
{
  GtkIconSource *source;
  GtkIconSet    *set;
  GdkPixbuf     *pixbuf;

  if (icons_factory != NULL)
    return;

  icons_factory = gtk_icon_factory_new ();

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_closetab_16), stock_closetab_16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-closetab", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_closewindow_16), stock_closewindow_16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-closewindow", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_general_48), stock_general_48, FALSE, NULL);
  gtk_window_set_default_icon (pixbuf); /* setup default window icon */
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-general", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_newtab_16), stock_newtab_16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-newtab", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_newwindow_16), stock_newwindow_16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-newwindow", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_reportbug_16), stock_reportbug_16, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-reportbug", set);
  gtk_icon_set_unref (set);

  gtk_icon_factory_add_default (icons_factory);
}



/**
 * terminal_icons_setup_preferences:
 *
 * Sets up the icons required for the preferences dialog.
 **/
void
terminal_icons_setup_preferences (void)
{
  static gboolean already_done = FALSE;
  GtkIconSource  *source;
  GtkIconSet     *set;
  GdkPixbuf      *pixbuf;

  if (already_done) return;
  already_done = TRUE;

  /* be sure to setup the main icons first */
  terminal_icons_setup_main ();
  g_assert (icons_factory != NULL);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_advanced_48), stock_advanced_48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-advanced", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_appearance_48), stock_appearance_48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-appearance", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_colors_48), stock_colors_48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-colors", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_compose_48), stock_compose_48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-compose", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_shortcuts_48), stock_shortcuts_48, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-shortcuts", set);
  gtk_icon_set_unref (set);
}



/**
 * terminal_icons_setup_toolbar:
 **/
void
terminal_icons_setup_toolbar (void)
{
  static gboolean already_done = FALSE;
  GtkIconSource  *source;
  GtkIconSet     *set;
  GdkPixbuf      *pixbuf;

  if (already_done) return;
  already_done = TRUE;

  /* be sure to setup the main icons first */
  terminal_icons_setup_main ();
  g_assert (icons_factory != NULL);

  set = gtk_icon_factory_lookup (icons_factory, "terminal-closetab");
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_closetab_24), stock_closetab_24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);

  set = gtk_icon_factory_lookup (icons_factory, "terminal-closewindow");
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_closewindow_24), stock_closewindow_24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_fullscreen_24), stock_fullscreen_24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-fullscreen", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_factory_lookup (icons_factory, "terminal-newtab");
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_newtab_24), stock_newtab_24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);

  set = gtk_icon_factory_lookup (icons_factory, "terminal-newwindow");
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_newwindow_24), stock_newwindow_24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);

  set = gtk_icon_factory_lookup (icons_factory, "terminal-reportbug");
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_reportbug_24), stock_reportbug_24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_showborders_24), stock_showborders_24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-showborders", set);
  gtk_icon_set_unref (set);

  set = gtk_icon_set_new ();
  source = gtk_icon_source_new ();
  pixbuf = gdk_pixbuf_new_from_inline (sizeof (stock_showmenu_24), stock_showmenu_24, FALSE, NULL);
  gtk_icon_source_set_pixbuf (source, pixbuf);
  gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_icon_set_add_source (set, source);
  g_object_unref (G_OBJECT (pixbuf));
  gtk_icon_factory_add (icons_factory, "terminal-showmenu", set);
  gtk_icon_set_unref (set);
}


