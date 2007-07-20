/* $Id$ */
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
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-private.h>



enum
{
  PROP_0,
  PROP_ACTIVE,
  PROP_CATEGORY,
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
      "and select Open Link from the context menu."),
  N_ ("Choose your preferred Mail Reader. The preferred\n"
      "Mail Reader opens when you right-click on an email\n"
      "address and select Compose Email from the context\n"
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
static void     terminal_helper_chooser_update_state        (TerminalHelperChooser      *chooser);



struct _TerminalHelperChooserClass
{
  GtkHBoxClass __parent__;
};

struct _TerminalHelperChooser
{
  GtkHBox                 __parent__;

  GtkWidget              *image;
  GtkWidget              *label;
  GtkTooltips            *tooltips;

  TerminalHelperDatabase *database;

  gchar                  *active;
  TerminalHelperCategory  category;
};



G_DEFINE_TYPE (TerminalHelperChooser, terminal_helper_chooser, GTK_TYPE_HBOX);



static void
terminal_helper_chooser_class_init (TerminalHelperChooserClass *klass)
{
  GObjectClass   *gobject_class;

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
                                                        "active",
                                                        "active",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  /**
   * TerminalHelperChooser:category:
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_CATEGORY,
                                   g_param_spec_enum ("category",
                                                      "category",
                                                      "category",
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

  chooser->tooltips = gtk_tooltips_new ();
  g_object_ref (G_OBJECT (chooser->tooltips));
  gtk_object_sink (GTK_OBJECT (chooser->tooltips));

  gtk_widget_push_composite_child ();

  button = gtk_button_new ();
  g_signal_connect (G_OBJECT (button), "pressed",
                    G_CALLBACK (terminal_helper_chooser_pressed), chooser);
  gtk_tooltips_set_tip (chooser->tooltips, button,
                        _("Click here to change the selected application "
                          "or to disable this feature."), NULL);
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

  terminal_helper_chooser_update_state (chooser);
}



static void
terminal_helper_chooser_finalize (GObject *object)
{
  TerminalHelperChooser *chooser = TERMINAL_HELPER_CHOOSER (object);

  g_object_unref (G_OBJECT (chooser->database));
  g_object_unref (G_OBJECT (chooser->tooltips));
  g_free (chooser->active);

  (*G_OBJECT_CLASS (terminal_helper_chooser_parent_class)->finalize) (object);
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
  const gchar    *id;

  helper = g_object_get_data (G_OBJECT (item), "terminal-helper");
  if (G_LIKELY (helper != NULL))
    {
      id = terminal_helper_get_id (helper);
      terminal_helper_chooser_set_active (chooser, id);
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
                                        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), TRUE);

  filename = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  if (G_LIKELY (filename != NULL && *filename != '\0'))
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), filename);
    }
  else
    {
      /* set Terminal's bindir as default folder */
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), BINDIR);
    }
  g_free (filename);

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

  TerminalHelper  *helper;
  const gchar     *command;
  const gchar     *id;
  GEnumClass      *enum_class;
  GEnumValue      *enum_value;
  GtkWidget       *toplevel;
  GtkWidget       *dialog;
  GtkWidget       *hbox;
  GtkWidget       *image;
  GtkWidget       *vbox;
  GtkWidget       *label;
  GtkWidget       *entry;
  GtkWidget       *button;
  gchar           *stock;

  /* sanity check the category values */
  g_assert (TERMINAL_HELPER_WEBBROWSER == 0);
  g_assert (TERMINAL_HELPER_MAILREADER == 1);

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

  /* determine the stock icon for the helper category */
  enum_class = g_type_class_ref (TERMINAL_TYPE_HELPER_CATEGORY);
  enum_value = g_enum_get_value (enum_class, chooser->category);
  stock = g_strconcat ("terminal-", enum_value->value_nick, NULL);
  g_type_class_unref (enum_class);

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
  g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (entry_changed), dialog);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (browse_clicked), entry);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* set the current custom command (if any) */
  helper = terminal_helper_database_get_custom (chooser->database, chooser->category);
  if (G_LIKELY (helper != NULL))
    {
      command = terminal_helper_get_command (helper);
      gtk_entry_set_text (GTK_ENTRY (entry), command);
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      /* change the custom command in the database */
      command = gtk_entry_get_text (GTK_ENTRY (entry));
      terminal_helper_database_set_custom (chooser->database, chooser->category, command);

      /* reload the custom helper */
      helper = terminal_helper_database_get_custom (chooser->database, chooser->category);
      if (G_LIKELY (helper != NULL))
        {
          id = terminal_helper_get_id (helper);
          terminal_helper_chooser_set_active (chooser, id);
        }
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
  guint           n_helpers;

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

  helpers = terminal_helper_database_lookup_all (chooser->database, chooser->category);
  for (lp = helpers, n_helpers = 0; lp != NULL; lp = lp->next)
    {
      /* skip hidden helpers (like custom ones) */
      helper = TERMINAL_HELPER (lp->data);
      if (terminal_helper_is_hidden (helper))
        {
          g_object_unref (G_OBJECT (helper));
          continue;
        }

      item = gtk_image_menu_item_new_with_label (terminal_helper_get_name (helper));
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_activate), chooser);
      g_object_set_data_full (G_OBJECT (item), "terminal-helper", helper, g_object_unref);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      image = gtk_image_new_from_icon_name (terminal_helper_get_icon (helper), GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      gtk_widget_show (image);

      ++n_helpers;
    }
  g_slist_free (helpers);

  if (G_LIKELY (n_helpers > 0))
    {
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);
    }

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
terminal_helper_chooser_update_state (TerminalHelperChooser *chooser)
{
  TerminalHelper *helper;

  if (exo_str_is_equal (chooser->active, "disabled"))
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (chooser->image), NULL);
      gtk_label_set_text (GTK_LABEL (chooser->label), _("Disabled"));
      return;
    }
  else if (chooser->active != NULL)
    {
      helper = terminal_helper_database_lookup (chooser->database,
                                                chooser->category,
                                                chooser->active);
      if (G_LIKELY (helper != NULL))
        {
          gtk_image_set_from_icon_name (GTK_IMAGE (chooser->image), terminal_helper_get_icon (helper), GTK_ICON_SIZE_MENU);
          gtk_label_set_text (GTK_LABEL (chooser->label), terminal_helper_get_name (helper));
          return;
        }
    }

  gtk_image_set_from_pixbuf (GTK_IMAGE (chooser->image), NULL);
  gtk_label_set_text (GTK_LABEL (chooser->label), _("No application selected"));
}



/**
 * terminal_helper_chooser_get_active:
 * @chooser : A #TerminalHelperChooser.
 *
 * Return value:
 **/
const gchar*
terminal_helper_chooser_get_active (TerminalHelperChooser *chooser)
{
  _terminal_return_val_if_fail (TERMINAL_IS_HELPER_CHOOSER (chooser), NULL);
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
  _terminal_return_if_fail (TERMINAL_IS_HELPER_CHOOSER (chooser));

  if (!exo_str_is_equal (chooser->active, active))
    {
      g_free (chooser->active);
      chooser->active = g_strdup (active);
      g_object_notify (G_OBJECT (chooser), "active");
    }

  terminal_helper_chooser_update_state (chooser);
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
  _terminal_return_val_if_fail (TERMINAL_IS_HELPER_CHOOSER (chooser),
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
  _terminal_return_if_fail (TERMINAL_IS_HELPER_CHOOSER (chooser));
  _terminal_return_if_fail (category >= TERMINAL_HELPER_WEBBROWSER
                         && category <= TERMINAL_HELPER_MAILREADER);
  
  if (chooser->category != category)
    {
      chooser->category = category;
      g_object_notify (G_OBJECT (chooser), "category");
    }

  terminal_helper_chooser_update_state (chooser);
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



G_DEFINE_TYPE (TerminalHelperDialog, terminal_helper_dialog, GTK_TYPE_DIALOG);



static void
terminal_helper_dialog_class_init (TerminalHelperDialogClass *klass)
{
  GtkDialogClass *gtkdialog_class;
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_helper_dialog_finalize;

  gtkdialog_class = GTK_DIALOG_CLASS (klass);
  gtkdialog_class->response = terminal_helper_dialog_response;
}



static void
terminal_helper_dialog_init (TerminalHelperDialog *dialog)
{
  TerminalHelperCategory category;
  const gchar           *text;
  GEnumClass            *enum_class;
  GEnumValue            *enum_value;
  GtkWidget             *vbox;
  GtkWidget             *frame;
  GtkWidget             *label;
  GtkWidget             *box;
  GtkWidget             *chooser;
  gchar                 *name;

  /* sanity check the category values */
  g_assert (TERMINAL_HELPER_WEBBROWSER == 0);
  g_assert (TERMINAL_HELPER_MAILREADER == 1);

  dialog->preferences = terminal_preferences_get ();

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Preferred Applications"));

  vbox = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 12, NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  for (category = 0; category < 2; ++category)
    {
      frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
      gtk_widget_show (frame);

      text = dgettext (GETTEXT_PACKAGE, category_titles[category]);
      name = g_strdup_printf ("<b>%s</b>", text);
      label = g_object_new (GTK_TYPE_LABEL, "label", name, "use-markup", TRUE, NULL);
      gtk_frame_set_label_widget (GTK_FRAME (frame), label);
      gtk_widget_show (label);
      g_free (name);

      box = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 12, NULL);
      gtk_container_add (GTK_CONTAINER (frame), box);
      gtk_widget_show (box);

      text = dgettext (GETTEXT_PACKAGE, category_descriptions[category]);
      label = g_object_new (GTK_TYPE_LABEL, "label", text, "xalign", 0.0, "yalign", 0.0, NULL);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      chooser = g_object_new (TERMINAL_TYPE_HELPER_CHOOSER, "category", category, NULL);
      gtk_box_pack_start (GTK_BOX (box), chooser, FALSE, FALSE, 0);
      gtk_widget_show (chooser);

      /* determine the name for the helper setting name */
      enum_class = g_type_class_ref (TERMINAL_TYPE_HELPER_CATEGORY);
      enum_value = g_enum_get_value (enum_class, category);
      name = g_strconcat ("helper-", enum_value->value_nick, NULL);
      exo_mutual_binding_new (G_OBJECT (dialog->preferences), name, G_OBJECT (chooser), "active");
      g_type_class_unref (enum_class);
      g_free (name);
    }
}



static void
terminal_helper_dialog_finalize (GObject *object)
{
  TerminalHelperDialog *dialog = TERMINAL_HELPER_DIALOG (object);

  g_object_unref (G_OBJECT (dialog->preferences));

  (*G_OBJECT_CLASS (terminal_helper_dialog_parent_class)->finalize) (object);
}



static void
terminal_helper_dialog_response (GtkDialog *dialog,
                                 gint       response_id)
{
  if (response_id == GTK_RESPONSE_CLOSE)
    gtk_widget_destroy (GTK_WIDGET (dialog));
}




