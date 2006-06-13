/* $Id$ */
/*-
 * Copyright (c) 2004-2006 os-cillation e.K.
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

#include <terminal/terminal-preferences.h>
#include <terminal/terminal-tab-header.h>



enum
{
  PROP_0,
  PROP_TAB_POS,
  PROP_TITLE,
};

enum
{
  CLOSE_TAB,
  DETACH_TAB,
  SET_TITLE,
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
static gboolean terminal_tab_header_button_press  (GtkWidget              *ebox,
                                                   GdkEventButton         *event,
                                                   TerminalTabHeader      *header);
static void     terminal_tab_header_close_tab     (GtkWidget              *widget,
                                                   TerminalTabHeader      *header);
static void     terminal_tab_header_detach_tab    (GtkWidget              *widget,
                                                   TerminalTabHeader      *header);



struct _TerminalTabHeaderClass
{
  GtkHBoxClass __parent__;

  /* signals */
  void (*close_tab)   (TerminalTabHeader *header);
  void (*detach_tab)  (TerminalTabHeader *header);
  void (*set_title)   (TerminalTabHeader *header);
};

struct _TerminalTabHeader
{
  GtkHBox              __parent__;

  TerminalPreferences *preferences;

  GtkTooltips         *tooltips;
  GtkWidget           *ebox;
  GtkWidget           *label;

  /* the popup menu */
  GtkWidget           *menu;
};



static guint header_signals[LAST_SIGNAL];



G_DEFINE_TYPE (TerminalTabHeader, terminal_tab_header, GTK_TYPE_HBOX);



static void
terminal_tab_header_class_init (TerminalTabHeaderClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_tab_header_finalize;
  gobject_class->get_property = terminal_tab_header_get_property;
  gobject_class->set_property = terminal_tab_header_set_property;

  /**
   * TerminalTabHeader:tab-pos:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TAB_POS,
                                   g_param_spec_enum ("tab-pos",
                                                      _("Tab position"),
                                                      _("Tab position"),
                                                      GTK_TYPE_POSITION_TYPE,
                                                      GTK_POS_TOP,
                                                      G_PARAM_CONSTRUCT | G_PARAM_WRITABLE));

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
   * TerminalTabHeader::close-tab:
   **/
  header_signals[CLOSE_TAB] =
    g_signal_new ("close-tab",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalTabHeaderClass, close_tab),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * TerminalTabHeader::detach-tab:
   **/
  header_signals[DETACH_TAB] =
    g_signal_new ("detach-tab",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalTabHeaderClass, detach_tab),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * TerminalTabHeader::set-title:
   **/
  header_signals[SET_TITLE] =
    g_signal_new ("set-title",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TerminalTabHeaderClass, set_title),
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

  header->preferences = terminal_preferences_get ();

  header->tooltips = gtk_tooltips_new ();
  g_object_ref (G_OBJECT (header->tooltips));
  gtk_object_sink (GTK_OBJECT (header->tooltips));

  gtk_widget_push_composite_child ();

  header->ebox = g_object_new (GTK_TYPE_EVENT_BOX, "border-width", 2, NULL);
  GTK_WIDGET_SET_FLAGS (header->ebox, GTK_NO_WINDOW);
  g_signal_connect (G_OBJECT (header->ebox), "button-press-event",
                    G_CALLBACK (terminal_tab_header_button_press), header);
  gtk_box_pack_start (GTK_BOX (header), header->ebox, TRUE, TRUE, 0);
  gtk_widget_show (header->ebox);

  header->label = g_object_new (GTK_TYPE_LABEL,
                                "selectable", FALSE,
                                "xalign", 0.0,
                                NULL);
  gtk_container_add (GTK_CONTAINER (header->ebox), header->label);
  gtk_widget_show (header->label);

  button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_tooltips_set_tip (header->tooltips, button,
                        _("Close this tab"), NULL);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (terminal_tab_header_close_tab), header);
  gtk_box_pack_start (GTK_BOX (header), button, FALSE, FALSE, 0);
  exo_binding_new (G_OBJECT (header->preferences), "misc-tab-close-buttons", G_OBJECT (button), "visible");

  image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, gtk_icon_size_from_name ("terminal-icon-size-tab"));
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gtk_widget_pop_composite_child ();
}



static void
terminal_tab_header_finalize (GObject *object)
{
  TerminalTabHeader *header = TERMINAL_TAB_HEADER (object);

  /* ensure that the open popup menu is
   * destroyed now, or we risk to run into
   * a crash when the user selects anything
   * from the menu after this point.
   */
  if (G_UNLIKELY (header->menu != NULL))
    gtk_widget_destroy (header->menu);

  g_object_unref (G_OBJECT (header->preferences));
  g_object_unref (G_OBJECT (header->tooltips));

  (*G_OBJECT_CLASS (terminal_tab_header_parent_class)->finalize) (object);
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
      g_object_get_property (G_OBJECT (header->label), "label", value);
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
  GtkPositionType    position;
  const gchar       *title;

  switch (prop_id)
    {
    case PROP_TAB_POS:
      position = g_value_get_enum (value);
      if (position == GTK_POS_TOP || position == GTK_POS_BOTTOM)
        {
          gtk_label_set_angle (GTK_LABEL (header->label), 0.0);
          gtk_label_set_ellipsize (GTK_LABEL (header->label), PANGO_ELLIPSIZE_END);
          gtk_widget_set_size_request (header->label, -1, -1);
        }
      else if (position == GTK_POS_LEFT)
        {
          gtk_label_set_angle (GTK_LABEL (header->label), 90.0);
          gtk_label_set_ellipsize (GTK_LABEL (header->label), PANGO_ELLIPSIZE_NONE);
          gtk_widget_set_size_request (header->label, -1, 1);
        }
      else
        {
          gtk_label_set_angle (GTK_LABEL (header->label), 270.0);
          gtk_label_set_ellipsize (GTK_LABEL (header->label), PANGO_ELLIPSIZE_NONE);
          gtk_widget_set_size_request (header->label, -1, 1);
        }
      break;

    case PROP_TITLE:
      title = g_value_get_string (value);
      gtk_tooltips_set_tip (header->tooltips, header->ebox, title, NULL);
      gtk_label_set_text (GTK_LABEL (header->label), title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
terminal_tab_header_button_press (GtkWidget              *ebox,
                                  GdkEventButton         *event,
                                  TerminalTabHeader      *header)
{
  GtkWidget *item;

  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      g_signal_emit (G_OBJECT (header), header_signals[SET_TITLE], 0);
      return TRUE;
    }
  else if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      if (G_LIKELY (header->menu == NULL))
        {
          header->menu = gtk_menu_new ();
          g_object_add_weak_pointer (G_OBJECT (header->menu),
                                     (gpointer) &header->menu);

          item = gtk_menu_item_new_with_mnemonic (_("_Detach Tab"));
          g_signal_connect (G_OBJECT (item), "activate",
                            G_CALLBACK (terminal_tab_header_detach_tab), header);
          gtk_menu_shell_append (GTK_MENU_SHELL (header->menu), item);
          gtk_widget_show (item);

          item = gtk_menu_item_new_with_mnemonic (_("C_lose Tab"));
          g_signal_connect (G_OBJECT (item), "activate",
                            G_CALLBACK (terminal_tab_header_close_tab), header);
          gtk_menu_shell_append (GTK_MENU_SHELL (header->menu), item);
          gtk_widget_show (item);
        }

      gtk_menu_popup (GTK_MENU (header->menu), NULL, NULL, NULL,
                      NULL, event->button, event->time);

      return TRUE;
    }

  return FALSE;
}



static void
terminal_tab_header_close_tab (GtkWidget         *widget,
                               TerminalTabHeader *header)
{
  g_signal_emit (G_OBJECT (header), header_signals[CLOSE_TAB], 0);
}



static void
terminal_tab_header_detach_tab (GtkWidget         *widget,
                                TerminalTabHeader *header)
{
  g_signal_emit (G_OBJECT (header), header_signals[DETACH_TAB], 0);
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
