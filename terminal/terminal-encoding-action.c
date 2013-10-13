/*-
 * Copyright (C) 2012 Nick Schermer <nick@xfce.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <libxfce4util/libxfce4util.h>

#include <terminal/terminal-encoding-action.h>
#include <terminal/terminal-private.h>



/* Signal identifiers */
enum
{
  ENCODING_CHANGED,
  LAST_SIGNAL,
};



static void       terminal_encoding_action_finalize          (GObject                *object);
static GtkWidget *terminal_encoding_action_create_menu_item  (GtkAction              *action);
static void       terminal_encoding_action_menu_shown        (GtkWidget              *menu,
                                                              TerminalEncodingAction *encoding_action);



struct _TerminalEncodingActionClass
{
  GtkActionClass __parent__;
};

struct _TerminalEncodingAction
{
  GtkAction  __parent__;

  gchar *current;
};



/* group names for the charsets below, order matters! */
static const gchar *terminal_encodings_names[] =
{
  N_("Western European"),
  N_("Central European"),
  N_("Baltic"),
  N_("South-Eastern Europe"),
  N_("Turkish"),
  N_("Cyrillic"),
  N_("Chinese Traditional"),
  N_("Chinese Simplified"),
  N_("Korean"),
  N_("Japanese"),
  N_("Greek"),
  N_("Arabic"),
  N_("Hebrew"),
  N_("Thai"),
  N_("Vietnamese"),
  N_("Unicode"),
  N_("Other"),
};

/* charsets for the groups above, order matters! */
static const gchar *terminal_encodings_charsets[][6] =
{
  /* Western European */
  { "ISO-8859-1", "ISO-8859-15", "ISO-8859-14", "WINDOWS-1252", "IBM850", NULL },
  /* Central European */
  { "ISO-8859-2", "ISO-8859-3", "WINDOWS-1250", NULL },
  /* Baltic */
  { "ISO-8859-4", "ISO-8859-13", "WINDOWS-1257", NULL },
  /* South-Eastern Europe */
  { "ISO-8859-16", NULL },
  /* Turkish */
  { "ISO-8859-9", "WINDOWS-1254", NULL },
  /* Cyrillic */
  { "KOI8-R", "ISO-8859-5", "WINDOWS-1251", "KOI8-U", "IBM855", NULL },
  /* Chinese Traditional */
  { "BIG5", "BIG5-HKSCS", NULL },
  /* Chinese Simplified */
  { "GB18030", "GBK", "GB2312", NULL },
  /* Korean */
  { "EUC-KR", NULL },
  /* Japanese */
  { "SHIFT_JIS", "JIS7", "EUC-JP", NULL },
  /* Greek */
  { "ISO-8859-7", "WINDOWS-1253", NULL },
  /* Arabic */
  { "ISO-8859-6", "IBM864", "WINDOWS-1256", NULL },
  /* Hebrew */
  { "ISO-8859-8", "ISO-8859-8-I", "IBM862", "WINDOWS-1255", NULL },
  /* Thai */
  { "TIS-620", "ISO-8859-11", NULL },
  /* Vietnamese */
  { "WINDOWS-1258", NULL },
  /* Unicode */
  { "UTF-8", "UTF-16", "UTF-7", "UCS-2", NULL },
  /* Other */
  { "IBM874", "WINDOWS-1258", "TSCII", NULL },
};



static guint  encoding_action_signals[LAST_SIGNAL];
static GQuark encoding_action_quark = 0;



G_DEFINE_TYPE (TerminalEncodingAction, terminal_encoding_action, GTK_TYPE_ACTION)



static void
terminal_encoding_action_class_init (TerminalEncodingActionClass *klass)
{
  GtkActionClass *gtkaction_class;
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_encoding_action_finalize;

  gtkaction_class = GTK_ACTION_CLASS (klass);
  gtkaction_class->create_menu_item = terminal_encoding_action_create_menu_item;

  encoding_action_quark = g_quark_from_static_string ("encoding-action-quark");

  encoding_action_signals[ENCODING_CHANGED] =
    g_signal_new (I_("encoding-changed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
}



static void
terminal_encoding_action_init (TerminalEncodingAction *action)
{

}



static void
terminal_encoding_action_finalize (GObject *object)
{
  TerminalEncodingAction *action = TERMINAL_ENCODING_ACTION (object);

  g_free (action->current);

  (*G_OBJECT_CLASS (terminal_encoding_action_parent_class)->finalize) (object);
}



static GtkWidget *
terminal_encoding_action_create_menu_item (GtkAction *action)
{
  GtkWidget *item;
  GtkWidget *menu;

  terminal_return_val_if_fail (TERMINAL_IS_ENCODING_ACTION (action), NULL);

  /* let GtkAction allocate the menu item */
  item = (*GTK_ACTION_CLASS (terminal_encoding_action_parent_class)->create_menu_item) (action);

  /* associate an empty submenu with the item (will be filled when shown) */
  menu = gtk_menu_new ();
  g_signal_connect (G_OBJECT (menu), "show", G_CALLBACK (terminal_encoding_action_menu_shown), action);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

  return item;
}



static void
terminal_encoding_action_activated (GtkWidget              *item,
                                    TerminalEncodingAction *encoding_action)
{
  const gchar *charset;

  terminal_return_if_fail (GTK_IS_CHECK_MENU_ITEM (item));

  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item)))
    return;

  /* menu charset or null to reset */
  charset = g_object_get_qdata (G_OBJECT (item), encoding_action_quark);
  g_signal_emit (G_OBJECT (encoding_action),
                 encoding_action_signals[ENCODING_CHANGED], 0, charset);
}



static void
terminal_encoding_action_menu_shown (GtkWidget              *menu,
                                     TerminalEncodingAction *action)
{
  GList         *children;
  guint          n, k;
  GtkWidget     *item;
  GtkWidget     *item2;
  GtkWidget     *submenu;
  const gchar   *charset;
  GSList        *groups = NULL;
  gboolean       found;
  GtkWidget     *label;
  GtkWidget     *bold_item = NULL;
  PangoAttrList *attrs;
  const gchar   *default_charset;
  gchar         *default_label;

  terminal_return_if_fail (TERMINAL_IS_ENCODING_ACTION (action));
  terminal_return_if_fail (GTK_IS_MENU_SHELL (menu));

  /* drop all existing children of the menu first */
  children = gtk_container_get_children (GTK_CONTAINER (menu));
  g_list_free_full (children, (GDestroyNotify) gtk_widget_destroy);

  g_get_charset (&default_charset);
  found = (action->current == NULL
           || g_strcmp0 (default_charset, action->current) == 0);

  /* action to reset to the default */
  default_label = g_strdup_printf (_("Default (%s)"), default_charset);
  item = gtk_radio_menu_item_new_with_label (groups, default_label);
  groups = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), found);
  g_signal_connect (G_OBJECT (item), "activate",
      G_CALLBACK (terminal_encoding_action_activated), action);
  gtk_widget_show (item);
  g_free (default_label);

  /*add the groups */
  for (n = 0; n < G_N_ELEMENTS (terminal_encodings_names); n++)
    {
      /* category item */
      item = gtk_menu_item_new_with_label (_(terminal_encodings_names[n]));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      submenu = gtk_menu_new ();
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

      /* submenu with charset */
      for (k = 0; k < G_N_ELEMENTS (*terminal_encodings_charsets); k++)
        {
          charset = terminal_encodings_charsets[n][k];
          if (charset == NULL)
            break;

          item2 = gtk_radio_menu_item_new_with_label (groups, charset);
          groups = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item2));
          gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item2);
          g_object_set_qdata (G_OBJECT (item2), encoding_action_quark, (gchar *) charset);
          gtk_widget_show (item2);

          if (!found
              && strcmp (action->current, charset) == 0)
            {
              gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item2), TRUE);

              found = TRUE;
              bold_item = item;
            }

          g_signal_connect (G_OBJECT (item2), "activate",
              G_CALLBACK (terminal_encoding_action_activated), action);
        }
    }

  if (!found)
    {
      /* add an action with the unknown charset */
      item2 = gtk_radio_menu_item_new_with_label (groups, action->current);
      groups = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item2));
      g_object_set_qdata_full (G_OBJECT (item2), encoding_action_quark,
                               g_strdup (action->current), g_free);
      gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item2);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item2), TRUE);
      g_signal_connect (G_OBJECT (item2), "activate",
        G_CALLBACK (terminal_encoding_action_activated), action);
      gtk_widget_show (item2);

      /* other group */
      bold_item = item;
    }

  if (bold_item != NULL)
    {
      attrs = pango_attr_list_new ();
      pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
      label = gtk_bin_get_child (GTK_BIN (bold_item));
      gtk_label_set_attributes (GTK_LABEL (label), attrs);
      pango_attr_list_unref (attrs);
    }
}



GtkAction*
terminal_encoding_action_new (const gchar *name,
                              const gchar *label)
{
  terminal_return_val_if_fail (name != NULL, NULL);
  terminal_return_val_if_fail (label != NULL, NULL);

  return g_object_new (TERMINAL_TYPE_ENCODING_ACTION,
                       "hide-if-empty", FALSE,
                       "label", label,
                       "name", name,
                       NULL);
}



void
terminal_encoding_action_set_charset (GtkAction   *gtkaction,
                                      const gchar *charset)
{
  TerminalEncodingAction *action = TERMINAL_ENCODING_ACTION (gtkaction);

  terminal_return_if_fail (TERMINAL_IS_ENCODING_ACTION (action));

  g_free (action->current);
  action->current = g_strdup (charset);
}







GtkTreeModel *
terminal_encoding_model_new (const gchar *current,
                             GtkTreeIter *current_iter)
{
  GtkTreeStore *store;
  guint         n;
  guint         k;
  GtkTreeIter   parent;
  const gchar  *charset;
  gboolean      found;
  GtkTreeIter   iter;
  gchar        *default_label;

  store = gtk_tree_store_new (N_ENCODING_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN,
                              G_TYPE_STRING);

  /* default */
  g_get_charset (&charset);
  default_label = g_strdup_printf (_("Default (%s)"), charset);
  gtk_tree_store_insert_with_values (store, &iter, NULL, 0,
                                     ENCODING_COLUMN_TITLE, default_label,
                                     ENCODING_COLUMN_VALUE, NULL,
                                     ENCODING_COLUMN_IS_CHARSET, TRUE,
                                     -1);
  g_free (default_label);
  found = (current == NULL || g_strcmp0 (current, charset) == 0);
  if (found)
    *current_iter = iter;

  /*add the groups */
  for (n = 0; n < G_N_ELEMENTS (terminal_encodings_names); n++)
    {
      /* category item */
      gtk_tree_store_insert_with_values (store, &parent, NULL, n + 1,
                                         ENCODING_COLUMN_TITLE, _(terminal_encodings_names[n]),
                                         -1);

      /* submenu with charset */
      for (k = 0; k < G_N_ELEMENTS (*terminal_encodings_charsets); k++)
        {
          charset = terminal_encodings_charsets[n][k];
          if (charset == NULL)
            break;

          gtk_tree_store_insert_with_values (store, &iter, &parent, k,
                                             ENCODING_COLUMN_TITLE, charset,
                                             ENCODING_COLUMN_VALUE, charset,
                                             -1);

          if (!found && g_strcmp0 (charset, current) == 0)
            {
              *current_iter = iter;
              found = TRUE;
            }
        }
    }

  if (!found)
    {
      /* add custom in other menu */
      gtk_tree_store_insert_with_values (store, current_iter, &parent, k,
                                         ENCODING_COLUMN_TITLE, current,
                                         ENCODING_COLUMN_IS_CHARSET, TRUE,
                                         ENCODING_COLUMN_VALUE, current,
                                         -1);
    }

  return GTK_TREE_MODEL (store);
}

