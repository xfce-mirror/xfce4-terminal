/* $Id$ */
/*-
 * Copyright (c) 2004 os-cillation e.K.
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

#include <exo/exo.h>
#include <terminal/terminal-tab-header.h>



enum
{
  PROP_0,
  PROP_TITLE,
};

enum
{
  CLOSE,
  DOUBLE_CLICKED,
  LAST_SIGNAL,
};



static void     terminal_tab_header_class_init    (TerminalTabHeaderClass *klass);
static void     terminal_tab_header_init          (TerminalTabHeader      *header);
static void     terminal_tab_header_finalize      (GObject                *object);
static void     terminal_tab_header_get_property  (GObject                *object,
                                                   guint                   prop_id,
                                                   GValue                 *value,
                                                   GParamSpec             *pspec);
static void     terminal_tab_header_set_property  (GObject                *object,
                                                   guint                   prop_id,
                                                   const GValue           *value,
                                                   GParamSpec             *pspec);
static void     terminal_tab_header_clicked       (GtkWidget              *button,
                                                   TerminalTabHeader      *header);
static gboolean terminal_tab_header_button_press  (GtkWidget              *ebox,
                                                   GdkEventButton         *event,
                                                   TerminalTabHeader      *header);



struct _TerminalTabHeader
{
  GtkHBox      __parent__;

  GtkTooltips *tooltips;
  GtkWidget   *ebox;
  GtkWidget   *label;
};



static GObjectClass *parent_class;
static guint         header_signals[LAST_SIGNAL];



G_DEFINE_TYPE (TerminalTabHeader, terminal_tab_header, GTK_TYPE_HBOX);



static void
terminal_tab_header_class_init (TerminalTabHeaderClass *klass)
{
  GObjectClass *gobject_class;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_tab_header_finalize;
  gobject_class->get_property = terminal_tab_header_get_property;
  gobject_class->set_property = terminal_tab_header_set_property;

  /**
   * TerminalTabHeader:title:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        _("Tab title"),
                                                        _("Tab title"),
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalTabHeader::close
   **/
  header_signals[CLOSE] =
    g_signal_new ("close",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalTabHeaderClass, close),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * TerminalTabHeader::double-clicked:
   **/
  header_signals[DOUBLE_CLICKED] =
    g_signal_new ("double-clicked",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalTabHeaderClass, double_clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /* register custom icon size for the tab close buttons */
  if (gtk_icon_size_from_name ("terminal-icon-size-tab") == GTK_ICON_SIZE_INVALID)
    gtk_icon_size_register ("terminal-icon-size-tab", 14, 14);
}



static void
terminal_tab_header_init (TerminalTabHeader *header)
{
  GtkWidget *button;
  GtkWidget *image;

  header->tooltips = gtk_tooltips_new ();
  g_object_ref (G_OBJECT (header->tooltips));
  gtk_object_sink (GTK_OBJECT (header->tooltips));

  header->ebox = gtk_event_box_new ();
  g_signal_connect (G_OBJECT (header->ebox), "button-press-event",
                    G_CALLBACK (terminal_tab_header_button_press), header);
  gtk_box_pack_start (GTK_BOX (header), header->ebox, TRUE, TRUE, 0);
  gtk_widget_show (header->ebox);

  header->label = exo_ellipsized_label_new ("");
  exo_ellipsized_label_set_mode (EXO_ELLIPSIZED_LABEL (header->label),
                                 EXO_PANGO_ELLIPSIZE_END);
  gtk_misc_set_alignment (GTK_MISC (header->label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (header->ebox), header->label);
  gtk_widget_show (header->label);

  button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_tooltips_set_tip (header->tooltips, button,
                        _("Close this tab"), NULL);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (terminal_tab_header_clicked), header);
  gtk_box_pack_start (GTK_BOX (header), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, gtk_icon_size_from_name ("terminal-icon-size-tab"));
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);
}



static void
terminal_tab_header_finalize (GObject *object)
{
  TerminalTabHeader *header = TERMINAL_TAB_HEADER (object);

  g_object_unref (G_OBJECT (header->tooltips));

  parent_class->finalize (object);
}



static void
terminal_tab_header_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  TerminalTabHeader *header = TERMINAL_TAB_HEADER (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_object_get_property (G_OBJECT (header->label), "full-text", value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
terminal_tab_header_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  TerminalTabHeader *header = TERMINAL_TAB_HEADER (object);
  const gchar       *title;

  switch (prop_id)
    {
    case PROP_TITLE:
      title = g_value_get_string (value);
      gtk_tooltips_set_tip (header->tooltips, header->ebox, title, NULL);
      exo_ellipsized_label_set_full_text (EXO_ELLIPSIZED_LABEL (header->label), title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
terminal_tab_header_clicked (GtkWidget         *button,
                             TerminalTabHeader *header)
{
  g_signal_emit (G_OBJECT (header), header_signals[CLOSE], 0);
}



static gboolean
terminal_tab_header_button_press (GtkWidget              *ebox,
                                  GdkEventButton         *event,
                                  TerminalTabHeader      *header)
{
  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      g_signal_emit (G_OBJECT (header), header_signals[DOUBLE_CLICKED], 0);
      return TRUE;
    }

  return FALSE;
}



/**
 * terminal_tab_header_new:
 * 
 * Allocates a new #TerminalTabHeader object.
 *
 * Return value : Pointer to the allocated #TerminalTabHeader object.
 **/
GtkWidget*
terminal_tab_header_new (void)
{
  return g_object_new (TERMINAL_TYPE_TAB_HEADER, NULL);
}
