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
 * terminal_gtk_label_set_a11y_relation:
 * @label  : a #GtkLabel.
 * @widget : a #GtkWidget.
 *
 * Sets the %ATK_RELATION_LABEL_FOR relation on @label for @widget, which means
 * accessiblity tools will identify @label as descriptive item for the specified
 * @widget.
 **/
void
terminal_gtk_label_set_a11y_relation (GtkLabel  *label,
                                      GtkWidget *widget)
{
  AtkRelationSet *relations;
  AtkRelation    *relation;
  AtkObject      *object;

  _terminal_return_if_fail (GTK_IS_WIDGET (widget));
  _terminal_return_if_fail (GTK_IS_LABEL (label));

  object = gtk_widget_get_accessible (widget);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (GTK_WIDGET (label)));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));
}
