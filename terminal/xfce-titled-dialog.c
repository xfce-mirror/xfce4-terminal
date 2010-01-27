/*-
 * Copyright (c) 2006 Benedikt Meurer <benny@xfce.org>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <gdk/gdkkeysyms.h>

#include <terminal/xfce-heading.h>
#include <terminal/xfce-titled-dialog.h>


#define XFCE_TITLED_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFCE_TYPE_TITLED_DIALOG, XfceTitledDialogPrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_SUBTITLE,
};



static void xfce_titled_dialog_finalize       (GObject                *object);
static void xfce_titled_dialog_get_property   (GObject                *object,
                                               guint                   prop_id,
                                               GValue                 *value,
                                               GParamSpec             *pspec);
static void xfce_titled_dialog_set_property   (GObject                *object,
                                               guint                   prop_id,
                                               const GValue           *value,
                                               GParamSpec             *pspec);
static void xfce_titled_dialog_close          (GtkDialog              *dialog);
static void xfce_titled_dialog_update_heading (XfceTitledDialog       *titled_dialog);
static void xfce_titled_dialog_set_subtitle   (XfceTitledDialog       *titled_dialog,
                                               const gchar            *subtitle);


struct _XfceTitledDialogPrivate
{
  GtkWidget *heading;
  gchar     *subtitle;
};



G_DEFINE_TYPE (XfceTitledDialog, xfce_titled_dialog, GTK_TYPE_DIALOG)



static void
xfce_titled_dialog_class_init (XfceTitledDialogClass *klass)
{
  GtkDialogClass *gtkdialog_class;
  GtkBindingSet  *binding_set;
  GObjectClass   *gobject_class;

  /* add our private data to the class */
  g_type_class_add_private (klass, sizeof (XfceTitledDialogPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_titled_dialog_finalize;
  gobject_class->get_property = xfce_titled_dialog_get_property;
  gobject_class->set_property = xfce_titled_dialog_set_property;

  gtkdialog_class = GTK_DIALOG_CLASS (klass);
  gtkdialog_class->close = xfce_titled_dialog_close;

  /**
   * XfceTitledDialog:subtitle:
   *
   * The subtitle displayed below the main dialog title.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SUBTITLE,
                                   g_param_spec_string ("subtitle",
                                                        "subtitle",
                                                        "subtitle",
                                                        NULL,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_STATIC_STRINGS));

  /* connect additional key bindings to the GtkDialog::close action signal */
  binding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (binding_set, GDK_w, GDK_CONTROL_MASK, "close", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_W, GDK_CONTROL_MASK, "close", 0);
}



static void
xfce_titled_dialog_init (XfceTitledDialog *titled_dialog)
{
  GtkWidget *line;
  GtkWidget *vbox;

  /* connect the private data */
  titled_dialog->priv = XFCE_TITLED_DIALOG_GET_PRIVATE (titled_dialog);

  /* remove the main dialog box from the window */
  g_object_ref (G_OBJECT (GTK_DIALOG (titled_dialog)->vbox));
  gtk_container_remove (GTK_CONTAINER (titled_dialog), GTK_DIALOG (titled_dialog)->vbox);

  /* add a new vbox w/o border to the main window */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (titled_dialog), vbox);
  gtk_widget_show (vbox);

  /* add the heading to the window */
  titled_dialog->priv->heading = xfce_heading_new ();
  gtk_box_pack_start (GTK_BOX (vbox), titled_dialog->priv->heading, FALSE, FALSE, 0);
  gtk_widget_show (titled_dialog->priv->heading);

  /* add the separator between header and content */
  line = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), line, FALSE, FALSE, 0);
  gtk_widget_show (line);

  /* add the main dialog box to the new vbox */
  gtk_box_pack_start (GTK_BOX (vbox), GTK_DIALOG (titled_dialog)->vbox, TRUE, TRUE, 0);
  g_object_unref (G_OBJECT (GTK_DIALOG (titled_dialog)->vbox));

  /* make sure to update the heading whenever one of the relevant window properties changes */
  g_signal_connect (G_OBJECT (titled_dialog), "notify::icon", G_CALLBACK (xfce_titled_dialog_update_heading), NULL);
  g_signal_connect (G_OBJECT (titled_dialog), "notify::icon-name", G_CALLBACK (xfce_titled_dialog_update_heading), NULL);
  g_signal_connect (G_OBJECT (titled_dialog), "notify::title", G_CALLBACK (xfce_titled_dialog_update_heading), NULL);

  /* initially update the heading properties */
  xfce_titled_dialog_update_heading (titled_dialog);
}




static void
xfce_titled_dialog_finalize (GObject *object)
{
  XfceTitledDialog *titled_dialog = XFCE_TITLED_DIALOG (object);

  /* release the subtitle */
  g_free (titled_dialog->priv->subtitle);

  (*G_OBJECT_CLASS (xfce_titled_dialog_parent_class)->finalize) (object);
}



static void
xfce_titled_dialog_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  XfceTitledDialog *titled_dialog = XFCE_TITLED_DIALOG (object);

  switch (prop_id)
    {
    case PROP_SUBTITLE:
      g_value_set_string (value, titled_dialog->priv->subtitle);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_titled_dialog_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  XfceTitledDialog *titled_dialog = XFCE_TITLED_DIALOG (object);

  switch (prop_id)
    {
    case PROP_SUBTITLE:
      xfce_titled_dialog_set_subtitle (titled_dialog, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_titled_dialog_close (GtkDialog *dialog)
{
  GdkEvent *event;

  /* verify that the dialog is realized */
  if (G_LIKELY (GTK_WIDGET_REALIZED (dialog)))
    {
      /* send a delete event to the dialog */
      event = gdk_event_new (GDK_DELETE);
      event->any.window = g_object_ref (GTK_WIDGET (dialog)->window);
      event->any.send_event = TRUE;
      gtk_main_do_event (event);
      gdk_event_free (event);
    }
}



static void
xfce_titled_dialog_update_heading (XfceTitledDialog *titled_dialog)
{
  /* update the heading properties using the window property values */
  xfce_heading_set_icon (XFCE_HEADING (titled_dialog->priv->heading), gtk_window_get_icon (GTK_WINDOW (titled_dialog)));
  xfce_heading_set_icon_name (XFCE_HEADING (titled_dialog->priv->heading), gtk_window_get_icon_name (GTK_WINDOW (titled_dialog)));
  xfce_heading_set_title (XFCE_HEADING (titled_dialog->priv->heading), gtk_window_get_title (GTK_WINDOW (titled_dialog)));
}



/**
 * xfce_titled_dialog_set_subtitle:
 * @titled_dialog : a #XfceTitledDialog.
 * @subtitle      : the new subtitle for the @titled_dialog, or %NULL.
 *
 * Sets the subtitle displayed by @titled_dialog to @subtitle; if
 * @subtitle is %NULL no subtitle will be displayed by the @titled_dialog.
 **/
static void
xfce_titled_dialog_set_subtitle (XfceTitledDialog *titled_dialog,
                                 const gchar      *subtitle)
{
  g_return_if_fail (XFCE_IS_TITLED_DIALOG (titled_dialog));
  g_return_if_fail (subtitle == NULL || g_utf8_validate (subtitle, -1, NULL));

  /* release the previous subtitle */
  g_free (titled_dialog->priv->subtitle);

  /* activate the new subtitle */
  titled_dialog->priv->subtitle = g_strdup (subtitle);

  /* update the subtitle for the heading */
  xfce_heading_set_subtitle (XFCE_HEADING (titled_dialog->priv->heading), subtitle);

  /* notify listeners */
  g_object_notify (G_OBJECT (titled_dialog), "subtitle");
}
