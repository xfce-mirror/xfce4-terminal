/* $Id$ */
/*-
 * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
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

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <exo/exo.h>

#include <terminal/terminal-gtk-extensions.h>
#include <terminal/terminal-private.h>



/**
 * terminal_gtk_widget_set_tooltip:
 * @widget : a #GtkWidget for which to set the tooltip.
 * @format : a printf(3)-style format string.
 * @...    : additional arguments for @format.
 *
 * Sets the tooltip for the @widget to a string generated
 * from the @format and the additional arguments in @...<!--->,
 * utilizing the shared #GtkTooltips instance.
 **/
void
terminal_gtk_widget_set_tooltip (GtkWidget   *widget,
                                 const gchar *format,
                                 ...)
{
  static GtkTooltips *tooltips = NULL;
  va_list             var_args;
  gchar              *tooltip;

  _terminal_return_if_fail (GTK_IS_WIDGET (widget));
  _terminal_return_if_fail (g_utf8_validate (format, -1, NULL));

  /* allocate the shared tooltips on-demand */
  if (G_UNLIKELY (tooltips == NULL))
    tooltips = gtk_tooltips_new ();

  /* determine the tooltip */
  va_start (var_args, format);
  tooltip = g_strdup_vprintf (format, var_args);
  va_end (var_args);

  /* setup the tooltip for the widget */
  gtk_tooltips_set_tip (tooltips, widget, tooltip, NULL);

  /* release the tooltip */
  g_free (tooltip);
}

