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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>

#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-helper-dialog.h>
#include <terminal/terminal-helper.h>
#include <terminal/terminal-icons.h>
#include <terminal/terminal-preferences.h>



enum
{
  PROP_0,
  PROP_ACTIVE,
  PROP_CATEGORY,
};

static const gchar *category_names[] =
{
  "browser",
  "mailer",
};

static const gchar *category_titles[] =
{
  N_ ("Web Browser"),
  N_ ("Mail Reader"),
};

static const gchar *category_descriptions[] =
{
  N_ ("Choose your preferred Web Browser. The preferred\n"
      "Web Browser opens when you right-click on a URL\n"
      "and select Open URL from the context menu."),
  N_ ("Choose your preferred Mail Reader. The preferred\n"
      "Mail Reader opens when you right-click on a mail\n"
      "address and select Compose E-Mail from the context\n"
      "menu."),
};



static void     terminal_helper_chooser_class_init          (TerminalHelperChooserClass *klass);
static void     terminal_helper_chooser_init                (TerminalHelperChooser      *chooser);
static void     terminal_helper_chooser_finalize            (GObject                    *object);
static void     terminal_helper_chooser_get_property        (GObject                    *object,
                                                             guint                       prop_id,
                                                             GValue                     *value,
                                                             GParamSpec                 *pspec);
static void     terminal_helper_chooser_set_property        (GObject                    *object,
                                                             guint                       prop_id,
                                                             const GValue               *value,
                                                             GParamSpec                 *pspec);
static void     terminal_helper_chooser_pressed             (GtkButton                  *button,
                                                             TerminalHelperChooser      *chooser);
static void     terminal_helper_chooser_schedule_update     (TerminalHelperChooser      *chooser);
static gboolean terminal_helper_chooser_update_idle         (gpointer                    user_data);
static void     terminal_helper_chooser_update_idle_destroy (gpointer                    user_data);



struct _TerminalHelperChooserClass
{
  GtkHBoxClass __parent__;
};

struct _TerminalHelperChooser
{
  GtkHBox                 __parent__;

  GtkWidget              *image;
  GtkWidget              *label;

  TerminalHelperDatabase *database;

  gchar                  *active;
  TerminalHelperCategory  category;

  guint                   update_idle_id;
};



static GObjectClass *chooser_parent_class;



G_DEFINE_TYPE (TerminalHelperChooser, terminal_helper_chooser, GTK_TYPE_HBOX);



static void
terminal_helper_chooser_class_init (TerminalHelperChooserClass *klass)
{
  GObjectClass   *gobject_class;

  chooser_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_helper_chooser_finalize;
  gobject_class->get_property = terminal_helper_chooser_get_property;
  gobject_class->set_property = terminal_helper_chooser_set_property;

  /**
   * TerminalHelperChooser:active:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_string ("active",
                                                        "Active helper",
                                                        "The currently selected helper",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalHelperChooser:category:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_CATEGORY,
                                   g_param_spec_enum ("category",
                                                      "Helper category",
                                                      "Helper category",
                                                      TERMINAL_TYPE_HELPER_CATEGORY,
                                                      TERMINAL_HELPER_WEBBROWSER,
                                                      G_PARAM_READWRITE));
}



static void
terminal_helper_chooser_init (TerminalHelperChooser *chooser)
{
  GtkWidget *separator;
  GtkWidget *button;
  GtkWidget *arrow;
  GtkWidget *hbox;

  chooser->database = terminal_helper_database_get ();

  gtk_widget_push_composite_child ();

  button = gtk_button_new ();
  g_signal_connect (G_OBJECT (button), "pressed",
                    G_CALLBACK (terminal_helper_chooser_pressed), chooser);
  gtk_container_add (GTK_CONTAINER (chooser), button);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (button), hbox);
  gtk_widget_show (hbox);

  chooser->image = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), chooser->image, FALSE, FALSE, 0);
  gtk_widget_show (chooser->image);

  chooser->label = g_object_new (GTK_TYPE_LABEL, "xalign", 0.0, "yalign", 0.0, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), chooser->label, TRUE, TRUE, 0);
  gtk_widget_show (chooser->label);

  separator = g_object_new (GTK_TYPE_VSEPARATOR, "height-request", 16, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), separator, FALSE, FALSE, 0);
  gtk_widget_show (separator);

  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (hbox), arrow, FALSE, FALSE, 0);
  gtk_widget_show (arrow);

  gtk_widget_pop_composite_child ();
}



static void
terminal_helper_chooser_finalize (GObject *object)
{
  TerminalHelperChooser *chooser = TERMINAL_HELPER_CHOOSER (object);

  terminal_helper_chooser_set_active (chooser, NULL);

  if (G_LIKELY (chooser->update_idle_id != 0))
    g_source_remove (chooser->update_idle_id);

  g_object_unref (G_OBJECT (chooser->database));

  G_OBJECT_CLASS (chooser_parent_class)->finalize (object);
}



static void
terminal_helper_chooser_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  TerminalHelperChooser *chooser = TERMINAL_HELPER_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_string (value, terminal_helper_chooser_get_active (chooser));
      break;

    case PROP_CATEGORY:
      g_value_set_enum (value, terminal_helper_chooser_get_category (chooser));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
   }
}



static void
terminal_helper_chooser_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  TerminalHelperChooser *chooser = TERMINAL_HELPER_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      terminal_helper_chooser_set_active (chooser, g_value_get_string (value));
      break;

    case PROP_CATEGORY:
      terminal_helper_chooser_set_category (chooser, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
   }
}



static void
menu_activate (GtkWidget             *item,
               TerminalHelperChooser *chooser)
{
  TerminalHelper *helper;
  gchar          *active;

  helper = g_object_get_data (G_OBJECT (item), "terminal-helper");
  if (G_LIKELY (helper != NULL))
    {
      active = g_strconcat ("predefined:", terminal_helper_get_id (helper), NULL);
      terminal_helper_chooser_set_active (chooser, active);
      g_free (active); 
    }
  else
    {
      terminal_helper_chooser_set_active (chooser, "disabled");
    }
}



static void
entry_changed (GtkEditable *editable,
               GtkDialog   *dialog)
{
  gchar *text;

  text = gtk_editable_get_chars (editable, 0, -1);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_OK,
                                     *text != '\0');
  g_free (text);
}



static void
browse_clicked (GtkWidget *button,
                GtkWidget *entry)
{
  GtkWidget *toplevel;
  GtkWidget *dialog;
  gchar     *filename;

  toplevel = gtk_widget_get_toplevel (entry);
  dialog = gtk_file_chooser_dialog_new (_("Select application"),
                                        GTK_WINDOW (toplevel),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

  filename = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  if (G_LIKELY (filename != NULL))
    {
      if (*filename != '\0')
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), filename);
      g_free (filename);
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      gtk_entry_set_text (GTK_ENTRY (entry), filename);
      g_free (filename);
    }

  gtk_widget_destroy (dialog);
}



static void
menu_activate_other (GtkWidget             *item,
                     TerminalHelperChooser *chooser)
{
  static const gchar *browse_titles[] =
  {
    N_ ("Choose a custom Web Browser"),
    N_ ("Choose a custom Mail Reader"),
  };

  static const gchar *browse_messages[] =
  {
    N_ ("Specify the application you want to use as\nWeb Browser in Terminal:"),
    N_ ("Specify the application you want to use as\nMail Reader in Terminal:"),
  };

  GtkWidget *toplevel;
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  gchar     *active;
  gchar     *stock;

  /* sanity check the category values */
  g_assert (TERMINAL_HELPER_WEBBROWSER == 0);
  g_assert (TERMINAL_HELPER_MAILREADER == 1);

  /* make sure the helper specific stock icons are loaded */
  terminal_icons_setup_helper ();

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));

  dialog = gtk_dialog_new_with_buttons (dgettext (GETTEXT_PACKAGE, browse_titles[chooser->category]),
                                        GTK_WINDOW (toplevel),
                                        GTK_DIALOG_DESTROY_WITH_PARENT
                                        | GTK_DIALOG_NO_SEPARATOR
                                        | GTK_DIALOG_MODAL,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 12);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  hbox = g_object_new (GTK_TYPE_HBOX, "border-width", 5, "spacing", 12, NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  stock = g_strconcat ("terminal-", dgettext (GETTEXT_PACKAGE, category_names[chooser->category]), NULL);
  image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);
  g_free (stock);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = g_object_new (GTK_TYPE_LABEL, 
                        "label", dgettext (GETTEXT_PACKAGE, browse_messages[chooser->category]),
                        "xalign", 0.0,
                        "yalign", 0.0,
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  entry = g_object_new (GTK_TYPE_ENTRY, "activates-default", TRUE, NULL);
  g_signal_connect (G_OBJECT (entry), "changed",
                    G_CALLBACK (entry_changed), dialog);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (browse_clicked), entry);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  if (strncmp (chooser->active, "custom:", 7) == 0)
    gtk_entry_set_text (GTK_ENTRY (entry), chooser->active + 7);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      active = g_strconcat ("custom:", gtk_entry_get_text (GTK_ENTRY (entry)), NULL);
      terminal_helper_chooser_set_active (TERMINAL_HELPER_CHOOSER (chooser), active);
      g_free (active);
    }

  gtk_widget_destroy (dialog);
}



static void
menu_position (GtkMenu    *menu,
               gint       *x,
               gint       *y,
               gboolean   *push_in,
               gpointer    chooser)
{
  GtkRequisition chooser_request;
  GtkRequisition menu_request;
  GdkRectangle   geometry;
  GdkScreen     *screen;
  GtkWidget     *toplevel = gtk_widget_get_toplevel (chooser);
  gint           monitor;
  gint           x0;
  gint           y0;

  gtk_widget_translate_coordinates (GTK_WIDGET (chooser), toplevel, 0, 0, &x0, &y0);

  gtk_widget_size_request (GTK_WIDGET (chooser), &chooser_request);
  gtk_widget_size_request (GTK_WIDGET (menu), &menu_request);

  gdk_window_get_position (GTK_WIDGET (chooser)->window, x, y);

  *y += y0;
  *x += x0;

  /* verify the the menu is on-screen */
  screen = gtk_widget_get_screen (GTK_WIDGET (chooser));
  if (G_LIKELY (screen != NULL))
    {
      monitor = gdk_screen_get_monitor_at_point (screen, *x, *y);
      gdk_screen_get_monitor_geometry (screen, monitor, &geometry);
      if (*y + menu_request.height > geometry.y + geometry.height)
        *y -= menu_request.height - chooser_request.height;
    }

  *push_in = TRUE;
}



static void
terminal_helper_chooser_pressed (GtkButton             *button,
                                 TerminalHelperChooser *chooser)
{
  TerminalHelper *helper;
  GMainLoop      *loop;
  GtkWidget      *menu;
  GtkWidget      *item;
  GtkWidget      *image;
  GdkCursor      *cursor;
  GSList         *helpers;
  GSList         *lp;

  /* set a watch cursor while loading the menu */
  if (G_LIKELY (GTK_WIDGET (button)->window != NULL))
    {
      cursor = gdk_cursor_new (GDK_WATCH);
      gdk_window_set_cursor (GTK_WIDGET (button)->window, cursor);
      gdk_cursor_unref (cursor);
      gdk_flush ();
    }

  menu = gtk_menu_new ();

  item = gtk_menu_item_new_with_label (_("Disable this feature"));
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_activate), chooser);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  helpers = terminal_helper_database_lookup_all (chooser->database,
                                                 chooser->category);
  for (lp = helpers; lp != NULL; lp = lp->next)
    {
      helper = TERMINAL_HELPER (lp->data);

      item = gtk_image_menu_item_new_with_label (terminal_helper_get_name (helper));
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_activate), chooser);
      g_object_set_data_full (G_OBJECT (item), "terminal-helper",
                              helper, g_object_unref);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      image = gtk_image_new_from_pixbuf (terminal_helper_get_icon (helper));
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      gtk_widget_show (image);
    }

  if (G_LIKELY (helpers != NULL))
    {
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
    }

  g_slist_free (helpers);

  item = gtk_menu_item_new_with_label (_("Other..."));
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_activate_other), chooser);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* reset the watch cursor on the chooser */
  if (G_LIKELY (GTK_WIDGET (button)->window != NULL))
    gdk_window_set_cursor (GTK_WIDGET (button)->window, NULL);

  loop = g_main_loop_new (NULL, FALSE);

  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (GTK_WIDGET (chooser)));
  g_signal_connect_swapped (G_OBJECT (menu), "deactivate", G_CALLBACK (g_main_loop_quit), loop);

  /* make sure the menu has atleast the same width as the chooser */
  if (menu->allocation.width < GTK_WIDGET (chooser)->allocation.width)
    gtk_widget_set_size_request (menu, GTK_WIDGET (chooser)->allocation.width, -1);

  gtk_grab_add (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, menu_position,
                  GTK_WIDGET (button), 0, gtk_get_current_event_time ());
  g_main_loop_run (loop);
  gtk_grab_remove (menu);
  g_main_loop_unref (loop);

  gtk_object_sink (GTK_OBJECT (menu));

  gtk_button_released (button);
}



static void
terminal_helper_chooser_schedule_update (TerminalHelperChooser *chooser)
{
  if (G_LIKELY (chooser->update_idle_id == 0))
    {
      chooser->update_idle_id = g_idle_add_full (G_PRIORITY_HIGH, terminal_helper_chooser_update_idle,
                                                 chooser, terminal_helper_chooser_update_idle_destroy);
    }
}



static gboolean
terminal_helper_chooser_update_idle (gpointer user_data)
{
  TerminalHelperChooser *chooser = TERMINAL_HELPER_CHOOSER (user_data);
  TerminalHelper        *helper;
  const gchar           *id;
  GSList                *helpers;

  g_object_freeze_notify (G_OBJECT (chooser));

  for (;;)
    {
      if (G_UNLIKELY (chooser->active == NULL))
        {
          helpers = terminal_helper_database_lookup_all (chooser->database,
                                                         chooser->category);
          if (G_LIKELY (helpers != NULL))
            {
              id = terminal_helper_get_id (helpers->data);
              chooser->active = g_strconcat ("predefined:", id, NULL);
              g_slist_foreach (helpers, (GFunc) g_object_unref, NULL);
              g_slist_free (helpers);
            }
          else
            {
              chooser->active = g_strdup ("disabled");
            }

          g_object_notify (G_OBJECT (chooser), "active");
        }

      if (exo_str_is_equal (chooser->active, "disabled"))
        {
          gtk_image_set_from_pixbuf (GTK_IMAGE (chooser->image), NULL);
          gtk_label_set_text (GTK_LABEL (chooser->label), _("Disabled"));
          break;
        }
      else if (strncmp (chooser->active, "predefined:", 11) == 0 && strlen (chooser->active) > 11)
        {
          helper = terminal_helper_database_lookup (chooser->database,
                                                    chooser->category,
                                                    chooser->active + 11);
          if (G_LIKELY (helper != NULL))
            {
              gtk_image_set_from_pixbuf (GTK_IMAGE (chooser->image), terminal_helper_get_icon (helper));
              gtk_label_set_text (GTK_LABEL (chooser->label), terminal_helper_get_name (helper));
              break;
            }
        }
      else if (strncmp (chooser->active, "custom:", 7) == 0 && strlen (chooser->active) > 7)
        {
          gtk_image_set_from_pixbuf (GTK_IMAGE (chooser->image), NULL);
          gtk_label_set_text (GTK_LABEL (chooser->label), chooser->active + 7);
          break;
        }

      /* once again... */
      g_free (chooser->active);
      chooser->active = NULL;
    }

  g_object_thaw_notify (G_OBJECT (chooser));

  return FALSE;
}



static void
terminal_helper_chooser_update_idle_destroy (gpointer user_data)
{
  TERMINAL_HELPER_CHOOSER (user_data)->update_idle_id = 0;
}



/**
 * terminal_helper_chooser_get_active:
 * @chooser : A #TerminalHelperChooser.
 *
 * This function returns the currently active helper, encoded as a
 * string.
 *
 *  predefined:name
 *  custom:path
 *  disabled
 *
 * Return value: the active helper selection encoded into a string.
 **/
const gchar*
terminal_helper_chooser_get_active (TerminalHelperChooser *chooser)
{
  g_return_val_if_fail (TERMINAL_IS_HELPER_CHOOSER (chooser), NULL);
  return chooser->active;
}



/**
 * terminal_helper_chooser_set_active:
 * @chooser : A #TerminalHelperChooser.
 * @active  : 
 **/
void
terminal_helper_chooser_set_active (TerminalHelperChooser *chooser,
                                    const gchar           *active)
{
  g_return_if_fail (TERMINAL_IS_HELPER_CHOOSER (chooser));

  if (!exo_str_is_equal (chooser->active, active))
    {
      g_free (chooser->active);
      chooser->active = g_strdup (active);
      g_object_notify (G_OBJECT (chooser), "active");
      terminal_helper_chooser_schedule_update (chooser);
    }
}



/**
 * terminal_helper_chooser_get_category:
 * @chooser : A #TerminalHelperChooser.
 *
 * Return value: 
 **/
TerminalHelperCategory
terminal_helper_chooser_get_category (TerminalHelperChooser *chooser)
{
  g_return_val_if_fail (TERMINAL_IS_HELPER_CHOOSER (chooser),
                        TERMINAL_HELPER_WEBBROWSER);
  return chooser->category;
}



/**
 * terminal_helper_chooser_set_category:
 * @chooser : A #TerminalHelperChooser.
 * @category:
 **/
void
terminal_helper_chooser_set_category (TerminalHelperChooser *chooser,
                                      TerminalHelperCategory category)
{
  g_return_if_fail (TERMINAL_IS_HELPER_CHOOSER (chooser));
  g_return_if_fail (category >= TERMINAL_HELPER_WEBBROWSER
                 && category <= TERMINAL_HELPER_MAILREADER);
  
  if (chooser->category != category)
    {
      chooser->category = category;
      terminal_helper_chooser_schedule_update (chooser);
    }
}




struct _TerminalHelperDialogClass
{
  GtkDialogClass __parent__;
};

struct _TerminalHelperDialog
{
  GtkDialog               __parent__;
  TerminalPreferences    *preferences;
};



static void terminal_helper_dialog_class_init (TerminalHelperDialogClass *klass);
static void terminal_helper_dialog_init       (TerminalHelperDialog      *dialog);
static void terminal_helper_dialog_finalize   (GObject                   *object);
static void terminal_helper_dialog_response   (GtkDialog                 *dialog,
                                               gint                       response_id);



static GObjectClass *dialog_parent_class;



G_DEFINE_TYPE (TerminalHelperDialog, terminal_helper_dialog, GTK_TYPE_DIALOG);



static void
terminal_helper_dialog_class_init (TerminalHelperDialogClass *klass)
{
  GtkDialogClass *gtkdialog_class;
  GObjectClass   *gobject_class;

  dialog_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_helper_dialog_finalize;

  gtkdialog_class = GTK_DIALOG_CLASS (klass);
  gtkdialog_class->response = terminal_helper_dialog_response;
}



static void
terminal_helper_dialog_init (TerminalHelperDialog *dialog)
{
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *box;
  GtkWidget *chooser;
  gchar     *name;
  guint      n;

  /* sanity check the category values */
  g_assert (TERMINAL_HELPER_WEBBROWSER == 0);
  g_assert (TERMINAL_HELPER_MAILREADER == 1);

  dialog->preferences = terminal_preferences_get ();

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Preferred Applications"));

  vbox = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 12, NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  for (n = 0; n < 2; ++n)
    {
      frame = g_object_new (GTK_TYPE_FRAME,
                            "border-width", 0,
                            "shadow-type", GTK_SHADOW_NONE,
                            NULL);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
      gtk_widget_show (frame);

      name = g_strdup_printf ("<b>%s</b>", dgettext (GETTEXT_PACKAGE, category_titles[n]));
      label = g_object_new (GTK_TYPE_LABEL,
                            "label", name,
                            "use-markup", TRUE,
                            NULL);
      gtk_frame_set_label_widget (GTK_FRAME (frame), label);
      gtk_widget_show (label);
      g_free (name);

      box = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 12, NULL);
      gtk_container_add (GTK_CONTAINER (frame), box);
      gtk_widget_show (box);

      label = gtk_label_new (dgettext (GETTEXT_PACKAGE, category_descriptions[n]));
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      chooser = g_object_new (TERMINAL_TYPE_HELPER_CHOOSER, "category", n, NULL);
      gtk_box_pack_start (GTK_BOX (box), chooser, FALSE, FALSE, 0);
      gtk_widget_show (chooser);

      name = g_strconcat ("helper-", category_names[n], NULL);
      exo_mutual_binding_new (G_OBJECT (dialog->preferences), name,
                              G_OBJECT (chooser), "active");
      g_free (name);
    }
}



static void
terminal_helper_dialog_finalize (GObject *object)
{
  TerminalHelperDialog *dialog = TERMINAL_HELPER_DIALOG (object);

  g_object_unref (G_OBJECT (dialog->preferences));

  G_OBJECT_CLASS (dialog_parent_class)->finalize (object);
}



static void
terminal_helper_dialog_response (GtkDialog *dialog,
                                 gint       response_id)
{
  if (response_id == GTK_RESPONSE_CLOSE)
    gtk_widget_destroy (GTK_WIDGET (dialog));
}




