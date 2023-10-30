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
#ifdef HAVE_LIBUTEMPTER
#include <utempter.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

#include <terminal/terminal-util.h>
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-marshal.h>
#include <terminal/terminal-preferences.h>
#include <terminal/terminal-widget.h>
#include <terminal/terminal-private.h>
#include <terminal/terminal-regex.h>



#define MAILTO          "mailto:"



enum
{
  GET_CONTEXT_MENU,
  PASTE_SELECTION_REQUEST,
  PASTE_CLIPBOARD_REQUEST,

  LAST_SIGNAL,
};

typedef enum
{
  PATTERN_TYPE_NONE,
  PATTERN_TYPE_FULL_HTTP,
  PATTERN_TYPE_HTTP,
  PATTERN_TYPE_EMAIL,
  PATTERN_TYPE_FILE
} PatternType;

enum
{
  PROP_ACCEL_GROUP = 1,
  N_PROPERTIES
};

typedef struct
{
  const gchar *pattern;
  PatternType  type;
} TerminalRegexPattern;

typedef struct {
    gchar      *uri;
    PatternType type;
} TerminalHyperlink;

static const TerminalRegexPattern regex_patterns[] =
{
  { REGEX_URL_AS_IS, PATTERN_TYPE_FULL_HTTP },
  { REGEX_URL_HTTP,  PATTERN_TYPE_HTTP },
  { REGEX_URL_FILE,  PATTERN_TYPE_FILE },
  { REGEX_EMAIL,     PATTERN_TYPE_EMAIL },
  { REGEX_NEWS_MAN,  PATTERN_TYPE_FULL_HTTP },
};



static void               terminal_widget_finalize                    (GObject          *object);
static void               terminal_widget_set_property                (GObject          *object,
                                                                       guint             prop_id,
                                                                       const GValue     *value,
                                                                       GParamSpec       *pspec);
static gboolean           terminal_widget_button_press_event          (GtkWidget        *widget,
                                                                       GdkEventButton   *event);
static void               terminal_widget_drag_data_received          (GtkWidget        *widget,
                                                                       GdkDragContext   *context,
                                                                       gint              x,
                                                                       gint              y,
                                                                       GtkSelectionData *selection_data,
                                                                       guint             info,
                                                                       guint             time);
static gboolean           terminal_widget_key_press_event             (GtkWidget        *widget,
                                                                       GdkEventKey      *event);
static void               terminal_widget_open_uri                    (TerminalWidget   *widget,
                                                                       const gchar      *wlink,
                                                                       PatternType       type);
static void               terminal_widget_update_highlight_urls       (TerminalWidget   *widget);
static gboolean           terminal_widget_action_shift_scroll_up      (TerminalWidget   *widget);
static gboolean           terminal_widget_action_shift_scroll_down    (TerminalWidget   *widget);
static gboolean           terminal_widget_action_scroll_page_up       (TerminalWidget   *widget);
static gboolean           terminal_widget_action_scroll_page_down     (TerminalWidget   *widget);
static void               terminal_widget_connect_accelerators        (TerminalWidget   *widget);
static void               terminal_widget_disconnect_accelerators     (TerminalWidget   *widget);
static TerminalHyperlink  terminal_widget_get_link                    (TerminalWidget   *widget,
                                                                       GdkEvent         *event);
static gboolean           terminal_widget_link_clickable              (const gchar      *uri,
                                                                       PatternType       type);
static void               terminal_widget_hyperlink_hover_uri_changed (TerminalWidget     *widget,
                                                                       const char         *uri,
                                                                       const GdkRectangle *bbox G_GNUC_UNUSED);



struct _TerminalWidgetClass
{
  VteTerminalClass parent_class;
};

struct _TerminalWidget
{
  VteTerminal          parent_instance;

  /*< private >*/
  TerminalPreferences *preferences;
  GtkAccelGroup       *accel_group;
  gint                 regex_tags[G_N_ELEMENTS (regex_patterns)];
  pcre2_code_8        *regex_pcre[G_N_ELEMENTS (regex_patterns)];
};



static guint widget_signals[LAST_SIGNAL];



static const GtkTargetEntry targets[] =
{
  { "text/uri-list", 0, TARGET_URI_LIST },
  { "text/x-moz-url", 0, TARGET_MOZ_URL },
  { "UTF8_STRING", 0, TARGET_UTF8_STRING },
  { "TEXT", 0, TARGET_TEXT },
  { "COMPOUND_TEXT", 0, TARGET_COMPOUND_TEXT },
  { "STRING", 0, TARGET_STRING },
  { "text/plain", 0, TARGET_TEXT_PLAIN },
  { "application/x-color", 0, TARGET_APPLICATION_X_COLOR },
  { "GTK_NOTEBOOK_TAB", GTK_TARGET_SAME_APP, TARGET_GTK_NOTEBOOK_TAB },
};



static XfceGtkActionEntry action_entries[] =
{
  { TERMINAL_WIDGET_ACTION_SCROLL_UP,   "<Actions>/terminal-widget/shift-up",   "<Shift>Up",   XFCE_GTK_MENU_ITEM, N_ ("Scroll one line Up"),   NULL, NULL, G_CALLBACK (terminal_widget_action_shift_scroll_up),   },
  { TERMINAL_WIDGET_ACTION_SCROLL_DOWN, "<Actions>/terminal-widget/shift-down", "<Shift>Down", XFCE_GTK_MENU_ITEM, N_ ("Scroll one line Down"), NULL, NULL, G_CALLBACK (terminal_widget_action_shift_scroll_down), },
  { TERMINAL_WIDGET_ACTION_SCROLL_PAGE_UP,   "<Actions>/terminal-widget/shift-pageup",   "<Shift>Page_Up",   XFCE_GTK_MENU_ITEM, N_ ("Scroll one Page Up"),   NULL, NULL, G_CALLBACK (terminal_widget_action_scroll_page_up),   },
  { TERMINAL_WIDGET_ACTION_SCROLL_PAGE_DOWN, "<Actions>/terminal-widget/shift-pagedown", "<Shift>Page_Down", XFCE_GTK_MENU_ITEM, N_ ("Scroll one Page Down"), NULL, NULL, G_CALLBACK (terminal_widget_action_scroll_page_down), },
};

#define get_action_entry(id) xfce_gtk_get_action_entry_by_id(action_entries, G_N_ELEMENTS(action_entries), id)

static GParamSpec *terminal_widget_props[N_PROPERTIES] = { NULL, };



G_DEFINE_TYPE (TerminalWidget, terminal_widget, VTE_TYPE_TERMINAL)



static void
terminal_widget_class_init (TerminalWidgetClass *klass)
{
  GtkWidgetClass  *gtkwidget_class;
  GObjectClass    *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_widget_finalize;
  gobject_class->set_property = terminal_widget_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event = terminal_widget_button_press_event;
  gtkwidget_class->drag_data_received = terminal_widget_drag_data_received;
  gtkwidget_class->key_press_event    = terminal_widget_key_press_event;

  xfce_gtk_translate_action_entries (action_entries, G_N_ELEMENTS (action_entries));

  /**
   * TerminalWidget::get-context-menu:
   **/
  widget_signals[GET_CONTEXT_MENU] =
    g_signal_new (I_("context-menu"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _terminal_marshal_OBJECT__VOID,
                  GTK_TYPE_MENU, 0);

  /**
   * TerminalWidget::paste-selection-request:
   **/
  widget_signals[PASTE_SELECTION_REQUEST] =
    g_signal_new (I_("paste-selection-request"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * TerminalWidget::paste-clipboard-request:
   **/
  widget_signals[PASTE_CLIPBOARD_REQUEST] =
    g_signal_new (I_("paste-clipboard-request"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  terminal_widget_props[PROP_ACCEL_GROUP] =
    g_param_spec_object ("accel-group",
                         "accel-group",
                         "accel-group",
                         GTK_TYPE_ACCEL_GROUP,
                         G_PARAM_WRITABLE);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, terminal_widget_props);
}



static void
terminal_widget_init (TerminalWidget *widget)
{
  /* query preferences connection */
  widget->preferences = terminal_preferences_get ();

  /* unset tags */
  memset (widget->regex_tags, -1, sizeof (widget->regex_tags));

  /* setup Drag'n'Drop support */
  gtk_drag_dest_set (GTK_WIDGET (widget),
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_DROP,
                     targets, G_N_ELEMENTS (targets),
                     GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_MOVE);

  /* monitor the misc-highlight-urls setting */
  g_signal_connect_swapped (G_OBJECT (widget->preferences), "notify::misc-highlight-urls",
                            G_CALLBACK (terminal_widget_update_highlight_urls), widget);

  /* monitor the misc-hyperlinks-enabled setting */
  g_object_bind_property (G_OBJECT (widget->preferences), "misc-hyperlinks-enabled",
                          G_OBJECT (widget), "allow-hyperlink",
                          G_BINDING_SYNC_CREATE);

  /* apply the initial misc-highlight-urls setting */
  terminal_widget_update_highlight_urls (widget);

  widget->accel_group = NULL;

  for (guint i = 0; i < G_N_ELEMENTS (regex_patterns); i++)
    {
      gint         error_number;
      PCRE2_SIZE   error_offset;

      widget->regex_pcre[i] = pcre2_compile_8 ((PCRE2_SPTR8) regex_patterns[i].pattern, PCRE2_ZERO_TERMINATED, 0, &error_number, &error_offset, NULL);
      if (widget->regex_pcre[i] == NULL)
        g_warning ("Failed to compile regex, error code \"%d\".", error_number);
    }
}



static void
terminal_widget_finalize (GObject *object)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

#ifdef HAVE_LIBUTEMPTER
  VtePty *pty = vte_terminal_get_pty (VTE_TERMINAL (widget));
  if (VTE_IS_PTY (pty))
    utempter_remove_record (vte_pty_get_fd (pty));
#endif

  /* disconnect the misc-highlight-urls watch */
  g_signal_handlers_disconnect_by_func (G_OBJECT (widget->preferences), G_CALLBACK (terminal_widget_update_highlight_urls), widget);

  /* disconnect the hyperlink-hover-uri-changed callback */
  g_signal_handlers_disconnect_by_func (G_OBJECT (widget), G_CALLBACK (terminal_widget_hyperlink_hover_uri_changed), widget);

  /* disconnect from the preferences */
  g_object_unref (G_OBJECT (widget->preferences));

  /* disconnect accelerators */
  terminal_widget_disconnect_accelerators (widget);

  for (guint i = 0; i < G_N_ELEMENTS (regex_patterns); i++)
    {
      if (widget->regex_pcre[i] != NULL)
        {
          pcre2_code_free_8 (widget->regex_pcre[i]);
          widget->regex_pcre[i] = NULL;
        }
    }

  (*G_OBJECT_CLASS (terminal_widget_parent_class)->finalize) (object);
}



static void
terminal_widget_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  TerminalWidget *widget = TERMINAL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_ACCEL_GROUP:
      terminal_widget_disconnect_accelerators (widget);
      widget->accel_group = g_value_dup_object (value);
      terminal_widget_connect_accelerators (widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
terminal_widget_context_menu_copy (TerminalWidget *widget,
                                   GtkWidget      *item)
{
  GtkClipboard *clipboard;
  const gchar  *wlink;
  GdkDisplay   *display;
  gchar        *modified_wlink = NULL;

  wlink = g_object_get_data (G_OBJECT (item), "terminal-widget-link");
  if (G_LIKELY (wlink != NULL))
    {
      display = gtk_widget_get_display (GTK_WIDGET (widget));

      /* strip mailto from links, bug #7909 */
      if (g_str_has_prefix (wlink, MAILTO))
        {
          modified_wlink = g_strdup (wlink + strlen (MAILTO));
          wlink = modified_wlink;
        }

      /* copy the URI to "CLIPBOARD" */
      clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_CLIPBOARD);
      gtk_clipboard_set_text (clipboard, wlink, -1);

      /* copy the URI to "PRIMARY" */
      clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_PRIMARY);
      gtk_clipboard_set_text (clipboard, wlink, -1);

      g_free (modified_wlink);
    }
}



static void
terminal_widget_context_menu_open (TerminalWidget *widget,
                                   GtkWidget      *item)
{
  const gchar *wlink;
  PatternType *type;

  wlink = g_object_get_data (G_OBJECT (item), "terminal-widget-link");
  type = g_object_get_data (G_OBJECT (item), "terminal-widget-link-type");

  if (wlink != NULL && type != NULL && terminal_widget_link_clickable (wlink, *type))
    terminal_widget_open_uri (widget, wlink, *type);
}



static void
terminal_widget_context_menu (TerminalWidget *widget,
                              gint            button,
                              guint32         event_time,
                              GdkEvent       *event)
{
  GMainLoop        *loop;
  GtkWidget        *menu            = NULL;
  GtkWidget        *item_copy       = NULL;
  GtkWidget        *item_open       = NULL;
  GtkWidget        *item_separator  = NULL;
  GList            *children;
  guint             id;
  TerminalHyperlink link;

  g_signal_emit (G_OBJECT (widget), widget_signals[GET_CONTEXT_MENU], 0, &menu);
  if (G_UNLIKELY (menu == NULL))
    return;

  /* check if we have a match */
  link = terminal_widget_get_link (widget, (GdkEvent *) event);
  if (G_UNLIKELY (link.uri != NULL))
    {
      /* prepend a separator to the menu if it does not already contain one */
      children = gtk_container_get_children (GTK_CONTAINER (menu));
      item_separator = g_list_nth_data (children, 0);
      if (!GTK_IS_SEPARATOR_MENU_ITEM (item_separator))
        {
          item_separator = gtk_separator_menu_item_new ();
          gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item_separator);
        }
      else
        item_separator = NULL;
      g_list_free (children);

      /* create menu items with appropriate labels */
      if (link.type == PATTERN_TYPE_EMAIL)
        {
          item_copy = gtk_menu_item_new_with_label (_("Copy Email Address"));
          item_open = gtk_menu_item_new_with_label (_("Compose Email"));
        }
      else if (link.type == PATTERN_TYPE_FILE)
        {
          item_copy = gtk_menu_item_new_with_label (_("Copy Link Address"));
          if (terminal_widget_link_clickable (link.uri, link.type))
            item_open = gtk_menu_item_new_with_label (_("Open Link"));
        }
      else
        {
          item_copy = gtk_menu_item_new_with_label (_("Copy Link Address"));
          item_open = gtk_menu_item_new_with_label (_("Open Link"));
        }

      /* prepend the "COPY" menu item */
      g_object_set_data_full (G_OBJECT (item_copy), I_("terminal-widget-link"), g_strdup (link.uri), g_free);
      g_signal_connect_swapped (G_OBJECT (item_copy), "activate", G_CALLBACK (terminal_widget_context_menu_copy), widget);
      gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item_copy);

      /* prepend the "OPEN" menu item */
      if (item_open != NULL)
        {
          g_object_set_data_full (G_OBJECT (item_open), I_("terminal-widget-link"), g_strdup (link.uri), g_free);
          g_object_set_data_full (G_OBJECT (item_open), I_("terminal-widget-link-type"), g_memdup (&link.type, sizeof (link.type)), g_free);
          g_signal_connect_swapped (G_OBJECT (item_open), "activate", G_CALLBACK (terminal_widget_context_menu_open), widget);
          gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item_open);
        }

      g_free (link.uri);
    }

  gtk_widget_show_all (menu);

  /* take a reference on the menu */
  g_object_ref_sink (G_OBJECT (menu));

  loop = g_main_loop_new (NULL, FALSE);

  /* connect the deactivate handler */
  id = g_signal_connect_swapped (G_OBJECT (menu), "deactivate", G_CALLBACK (g_main_loop_quit), loop);

  /* make sure the menu is on the proper screen */
  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (GTK_WIDGET (widget)));

  /* run our custom main loop */
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  /* remove the additional items (if any) */
  if (item_separator != NULL) gtk_widget_destroy (item_separator);
  if (item_open != NULL) gtk_widget_destroy (item_open);
  if (item_copy != NULL) gtk_widget_destroy (item_copy);

  /* unlink this deactivate callback */
  g_signal_handler_disconnect (G_OBJECT (menu), id);

  /* decrease the reference count on the menu */
  g_object_unref (G_OBJECT (menu));
}



static void
terminal_widget_commit (TerminalWidget *widget,
                        gchar          *data,
                        guint           length,
                        gboolean       *committed)
{
  *committed = TRUE;
}



static gboolean
terminal_widget_button_press_event (GtkWidget       *widget,
                                    GdkEventButton  *event)
{
  const GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask ();
  gboolean              committed = FALSE;
  gboolean              middle_click_opens_uri;
  guint                 signal_id = 0;

  if (event->type == GDK_BUTTON_PRESS)
    {
      /* check whether to use ctrl-click or middle click to open URI */
      g_object_get (G_OBJECT (TERMINAL_WIDGET (widget)->preferences),
                    "misc-middle-click-opens-uri", &middle_click_opens_uri, NULL);

      if (middle_click_opens_uri
            ? (event->button == 2)
            : (event->button == 1 && (event->state & modifiers) == GDK_CONTROL_MASK))
        {
          /* clicking on an URI fires the responsible application */
          TerminalHyperlink link = terminal_widget_get_link (TERMINAL_WIDGET (widget), (GdkEvent *) event);
          if (G_UNLIKELY (link.uri != NULL && terminal_widget_link_clickable (link.uri, link.type)))
            {
              terminal_widget_open_uri (TERMINAL_WIDGET (widget), link.uri, link.type);
              return TRUE;
            }
        }

      /* intercept middle button click that would paste the selection */
      if (event->button == 2)
        {
          g_signal_emit (G_OBJECT (widget), widget_signals[PASTE_SELECTION_REQUEST], 0, NULL);
          return TRUE;
        }
      else if (event->button == 3)
        {
          signal_id = g_signal_connect (G_OBJECT (widget), "commit",
                                        G_CALLBACK (terminal_widget_commit), &committed);
        }
    }

  (*GTK_WIDGET_CLASS (terminal_widget_parent_class)->button_press_event) (widget, event);

  if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
      g_signal_handler_disconnect (G_OBJECT (widget), signal_id);

      /* no data (mouse actions) was committed to the terminal application
       * which means, we can safely popup a context menu now.
       */
      if (!committed || (event->state & modifiers) == GDK_SHIFT_MASK)
        {
          TerminalRightClickAction action;

          g_object_get (G_OBJECT (TERMINAL_WIDGET (widget)->preferences), "misc-right-click-action", &action, NULL);

          if (action == TERMINAL_RIGHT_CLICK_ACTION_CONTEXT_MENU)
            terminal_widget_context_menu (TERMINAL_WIDGET (widget),
                                          event->button, event->time,
                                          (GdkEvent *) event);
          else if (action == TERMINAL_RIGHT_CLICK_ACTION_PASTE_CLIPBOARD)
            g_signal_emit (G_OBJECT (widget), widget_signals[PASTE_CLIPBOARD_REQUEST], 0, NULL);
          else if (action == TERMINAL_RIGHT_CLICK_ACTION_PASTE_SELECTION)
            g_signal_emit (G_OBJECT (widget), widget_signals[PASTE_SELECTION_REQUEST], 0, NULL);
        }
    }

  return TRUE;
}



static void
terminal_widget_drag_data_received (GtkWidget        *widget,
                                    GdkDragContext   *context,
                                    gint              x,
                                    gint              y,
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    guint             time)
{
  const gunichar2 *ucs;
  GdkRGBA        color;
  GString       *str;
  GValue         value = { 0, };
  gchar        **uris;
  gchar         *filename;
  gchar         *text;
  gint           n;
  GtkWidget     *screen;

  switch (info)
    {
    case TARGET_STRING:
    case TARGET_UTF8_STRING:
    case TARGET_COMPOUND_TEXT:
    case TARGET_TEXT:
      text = (gchar *) gtk_selection_data_get_text (selection_data);
      if (G_LIKELY (text != NULL))
        {
          if (G_LIKELY (*text != '\0'))
            vte_terminal_feed_child (VTE_TERMINAL (widget), text, strlen (text));
          g_free (text);
        }
      break;

    case TARGET_TEXT_PLAIN:
      if (gtk_selection_data_get_format (selection_data) != 8 || gtk_selection_data_get_length (selection_data) == 0)
        {
          g_warning ("Unable to drop selection of type text/plain to terminal: Wrong format (%d) or length (%d)",
                     gtk_selection_data_get_format (selection_data), gtk_selection_data_get_length (selection_data));
        }
      else
        {
          vte_terminal_feed_child (VTE_TERMINAL (widget),
                                   (const gchar *) gtk_selection_data_get_data (selection_data),
                                   gtk_selection_data_get_length (selection_data));
        }
      break;

    case TARGET_MOZ_URL:
      if (gtk_selection_data_get_format (selection_data) != 8
          || gtk_selection_data_get_length (selection_data) == 0
          || (gtk_selection_data_get_length (selection_data) % 2) != 0)
        {
          g_warning ("Unable to drop Mozilla URL on terminal: Wrong format (%d) or length (%d)",
                     gtk_selection_data_get_format (selection_data), gtk_selection_data_get_length (selection_data));
        }
      else
        {
          str = g_string_new (NULL);
          ucs = (const gunichar2 *) gtk_selection_data_get_data (selection_data);
          for (n = 0; n < gtk_selection_data_get_length (selection_data) / 2 && ucs[n] != '\n'; ++n)
            g_string_append_unichar (str, ucs[n]);
          filename = g_filename_from_uri (str->str, NULL, NULL);
          if (filename != NULL)
            {
              vte_terminal_feed_child (VTE_TERMINAL (widget), filename, strlen (filename));
              g_free (filename);
            }
          else
            {
              vte_terminal_feed_child (VTE_TERMINAL (widget), str->str, str->len);
            }
          vte_terminal_feed_child (VTE_TERMINAL (widget), " ", 1);
          g_string_free (str, TRUE);
        }
      break;

    case TARGET_URI_LIST:
      if (gtk_selection_data_get_format (selection_data) != 8 || gtk_selection_data_get_length (selection_data) == 0)
        {
          g_warning ("Unable to drop URI list on terminal: Wrong format (%d) or length (%d)",
                     gtk_selection_data_get_format (selection_data), gtk_selection_data_get_length (selection_data));
        }
      else
        {
          /* split the text/uri-list */
          text = g_strndup ((const gchar *) gtk_selection_data_get_data (selection_data), gtk_selection_data_get_length (selection_data));
          uris = g_uri_list_extract_uris (text);
          g_free (text);

          /* translate all file:-URIs to quoted file names */
          for (n = 0; uris[n] != NULL; ++n)
            {
              /* check if we have a local file here */
              filename = g_filename_from_uri (uris[n], NULL, NULL);
              if (G_LIKELY (filename != NULL))
                {
                  /* release the file:-URI */
                  g_free (uris[n]);

                  /* quote the file name (for the shell) */
                  uris[n] = g_shell_quote (filename);
                  g_free (filename);
                }
            }

          text = g_strjoinv (" ", uris);
          vte_terminal_feed_child (VTE_TERMINAL (widget), text, strlen (text));
          vte_terminal_feed_child (VTE_TERMINAL (widget), " ", 1);
          g_strfreev (uris);
          g_free (text);
        }
      break;

    case TARGET_APPLICATION_X_COLOR:
      if (gtk_selection_data_get_format (selection_data) != 16 || gtk_selection_data_get_length (selection_data) != 8)
        {
          g_warning ("Received invalid color data: Wrong format (%d) or length (%d)",
                     gtk_selection_data_get_format (selection_data), gtk_selection_data_get_length (selection_data));
        }
      else
        {
          /* get the color from the selection data (ignoring the alpha setting) */
          const guchar *data = gtk_selection_data_get_data (selection_data);
          color.red   = (gdouble) data[0] / 65535.;
          color.green = (gdouble) data[1] / 65535.;
          color.blue  = (gdouble) data[2] / 65535.;
          color.alpha = 1.;

          /* prepare the value */
          g_value_init (&value, GDK_TYPE_RGBA);
          g_value_set_boxed (&value, &color);

          /* change the background to the specified color */
          g_object_set_property (G_OBJECT (TERMINAL_WIDGET (widget)->preferences), "color-background", &value);

          /* release the value */
          g_value_unset (&value);
        }
      break;

    case TARGET_GTK_NOTEBOOK_TAB:
      /* 'send' the drag to the parent's parent widget (TerminalWidget -> GtkBox -> TerminalScreen) */
      screen = gtk_widget_get_parent (gtk_widget_get_parent (widget));
      if (G_LIKELY (screen))
        {
          g_signal_emit_by_name (G_OBJECT (screen), "drag-data-received", context,
                                 x, y, selection_data, info, time);
        }
      break;

    default:
      /* never finish the drag */
      return;
    }

  if (info != TARGET_GTK_NOTEBOOK_TAB)
    gtk_drag_finish (context, TRUE, FALSE, time);
}



static gboolean
terminal_widget_key_press_event (GtkWidget    *widget,
                                 GdkEventKey  *event)
{
  gboolean       shortcuts_no_menukey;

  /* determine current settings */
  g_object_get (G_OBJECT (TERMINAL_WIDGET (widget)->preferences),
                "shortcuts-no-menukey", &shortcuts_no_menukey,
                NULL);

  /* popup context menu if "Menu" or "<Shift>F10" is pressed */
  if (event->keyval == GDK_KEY_Menu ||
      (!shortcuts_no_menukey && (event->state & GDK_SHIFT_MASK) != 0 && event->keyval == GDK_KEY_F10))
    {
      terminal_widget_context_menu (TERMINAL_WIDGET (widget), 0, event->time, (GdkEvent *) event);
      return TRUE;
    }

  return (*GTK_WIDGET_CLASS (terminal_widget_parent_class)->key_press_event) (widget, event);
}



static void
terminal_widget_open_uri (TerminalWidget *widget,
                          const gchar    *wlink,
                          PatternType     type)
{
  GtkWindow *window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (widget)));
  GError    *error = NULL;
  gchar     *uri;

  /* handle the pattern type */
  switch (type)
    {
      case PATTERN_TYPE_FULL_HTTP:
      case PATTERN_TYPE_FILE:
        uri = g_strdup (wlink);
        break;

      case PATTERN_TYPE_HTTP:
        uri = g_strconcat ("http://", wlink, NULL);
        break;

      case PATTERN_TYPE_EMAIL:
        uri = strncmp (wlink, MAILTO, strlen (MAILTO)) == 0
            ? g_strdup (wlink) : g_strconcat (MAILTO, wlink, NULL);
        break;

      default:
        g_warning ("Invalid tag specified while trying to open link \"%s\".", wlink);
        return;
    }

  /* try to open the URI with the responsible application */
  if (gtk_show_uri_on_window (window, uri, gtk_get_current_event_time (), &error) == FALSE)
    {
      /* tell the user that we were unable to open the responsible application */
      xfce_dialog_show_error (window, error, _("Failed to open the URL '%s'"), uri);
      g_error_free (error);
    }

  g_free (uri);
}



static void
terminal_widget_update_highlight_urls (TerminalWidget *widget)
{
  guint                       i;
  gboolean                    highlight_urls;
  VteRegex                   *regex;
  const TerminalRegexPattern *pattern;
  GError                     *error;

  g_object_get (G_OBJECT (widget->preferences),
                "misc-highlight-urls", &highlight_urls, NULL);

  if (!highlight_urls)
    {
      /* remove all our regex tags */
      for (i = 0; i < G_N_ELEMENTS (regex_patterns); i++)
        if (widget->regex_tags[i] != -1)
          {
            vte_terminal_match_remove (VTE_TERMINAL (widget), widget->regex_tags[i]);
            widget->regex_tags[i] = -1;
          }
    }
  else
    {
      /* set all our patterns */
      for (i = 0; i < G_N_ELEMENTS (regex_patterns); i++)
        {
          /* continue if already set */
          if (G_UNLIKELY (widget->regex_tags[i] != -1))
            continue;

          /* get the pattern */
          pattern = &regex_patterns[i];

          /* build the regex */
          error = NULL;
          regex = vte_regex_new_for_match (pattern->pattern, -1,
                                           PCRE2_CASELESS | PCRE2_UTF | PCRE2_NO_UTF_CHECK | PCRE2_MULTILINE,
                                           &error);

          if (error == NULL && (!vte_regex_jit (regex, PCRE2_JIT_COMPLETE, &error) ||
                                !vte_regex_jit (regex, PCRE2_JIT_PARTIAL_SOFT, &error)))
            {
              g_critical ("Failed to JIT regular expression '%s': %s\n", pattern->pattern, error->message);
              g_clear_error (&error);
            }
          if (G_UNLIKELY (error != NULL))
            {
              g_critical ("Failed to parse regular expression pattern %d: %s", i, error->message);
              g_error_free (error);
              continue;
            }

          /* set the new regular expression */
          widget->regex_tags[i] = vte_terminal_match_add_regex (VTE_TERMINAL (widget), regex, 0);
#if VTE_CHECK_VERSION (0, 53, 0)
          vte_terminal_match_set_cursor_name (VTE_TERMINAL (widget), widget->regex_tags[i], "hand2");
#else
          vte_terminal_match_set_cursor_type (VTE_TERMINAL (widget), widget->regex_tags[i], GDK_HAND2);
#endif
          /* release the regex owned by vte now */
          vte_regex_unref (regex);
        }
    }
}



static gboolean
terminal_widget_action_scroll_page_up (TerminalWidget *widget)
{
  GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (widget));

  gtk_adjustment_set_value (adjustment, gtk_adjustment_get_value (adjustment) - gtk_adjustment_get_page_size (adjustment));
  return TRUE;
}



static gboolean
terminal_widget_action_scroll_page_down (TerminalWidget *widget)
{
  GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (widget));

  gtk_adjustment_set_value (adjustment, gtk_adjustment_get_value (adjustment) + gtk_adjustment_get_page_size (adjustment));
  return TRUE;
}



static gboolean
terminal_widget_action_shift_scroll_up (TerminalWidget *widget)
{
  GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (widget));

  gtk_adjustment_set_value (adjustment, gtk_adjustment_get_value (adjustment) - 1);
  return TRUE;
}



static gboolean
terminal_widget_action_shift_scroll_down (TerminalWidget *widget)
{
  GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (widget));
  gdouble        value;

  value = MIN (gtk_adjustment_get_value (adjustment) + 1, gtk_adjustment_get_upper (adjustment) - gtk_adjustment_get_page_size (adjustment));
  gtk_adjustment_set_value (adjustment, value);
  return TRUE;
}



static void
terminal_widget_connect_accelerators (TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  if (widget->accel_group == NULL)
    return;

  xfce_gtk_accel_map_add_entries (action_entries, G_N_ELEMENTS (action_entries));
  xfce_gtk_accel_group_connect_action_entries (widget->accel_group,
                                               action_entries,
                                               G_N_ELEMENTS (action_entries),
                                               widget);
}



static void
terminal_widget_disconnect_accelerators (TerminalWidget *widget)
{
  g_return_if_fail (TERMINAL_IS_WIDGET (widget));

  if (widget->accel_group == NULL)
    return;

  /* Don't listen to the accel keys defined by the action entries any more */
  xfce_gtk_accel_group_disconnect_action_entries (widget->accel_group,
                                                  action_entries,
                                                  G_N_ELEMENTS (action_entries));

  /* and release the accel group */
  g_object_unref (widget->accel_group);
  widget->accel_group = NULL;
}



XfceGtkActionEntry*
terminal_widget_get_action_entries (void)
{
  return action_entries;
}



static TerminalHyperlink
terminal_widget_get_link (TerminalWidget *widget,
                          GdkEvent       *event)
{
  guint               i;
  gint                tag;
  gchar              *uri;
  pcre2_match_data_8 *match_data;
  TerminalHyperlink   result = {NULL, PATTERN_TYPE_NONE};
  gboolean            hyperlinks_enabled;

  g_object_get (G_OBJECT (TERMINAL_WIDGET (widget)->preferences), "misc-hyperlinks-enabled", &hyperlinks_enabled, NULL);

  /* check if we have an OSC 8 uri */
  if (hyperlinks_enabled && (uri = vte_terminal_hyperlink_check_event (VTE_TERMINAL (widget), (GdkEvent *) event)) != NULL)
    {
      gint rc;

      for (i = 0; i < G_N_ELEMENTS (widget->regex_pcre); i++)
        {
          if (widget->regex_pcre[i] == NULL)
            continue;

          match_data = pcre2_match_data_create_from_pattern_8 (widget->regex_pcre[i], NULL);
          rc = pcre2_match_8 (widget->regex_pcre[i], (PCRE2_SPTR8) uri, strlen (uri), 0, 0, match_data, NULL);
          if (rc >= 0)
            {
              result.uri = uri;
              result.type = regex_patterns[i].type;
              return result;
            }
          else if (rc != PCRE2_ERROR_NOMATCH)
            g_warning ("pcre2_match returned error code \"%d\".", rc);

          pcre2_match_data_free_8(match_data);
        }
    }

  /* check if we have a regex match */
  if ((uri = vte_terminal_match_check_event (VTE_TERMINAL (widget), event, &tag)) != NULL)
    {
      for (i = 0; i < G_N_ELEMENTS (regex_patterns); i++)
        {
          /* lookup the tag in our tags */
          if (widget->regex_tags[i] == tag)
            {
              result.uri = uri;
              result.type = regex_patterns[i].type;
              return result;
            }
        }
    }

  /* freeing the uri if regex didn't match */
  if (uri != NULL && result.uri == NULL)
    g_free (uri);

  return result;
}



/*
 * Ensures that hostname component of a link with a file:// schema
 * matches the current hostname or is "localhost".
 *
 * See: https://gist.github.com/egmontkob/eb114294efbcd5adb1944c9f3cb5feda
 */
static gboolean
terminal_widget_link_clickable (const gchar *uri,
                                PatternType  type)
{
  gboolean result = FALSE;
  gchar   *filename;
  gchar   *hostname;

  if (type != PATTERN_TYPE_FILE)
    return TRUE;

  filename = g_filename_from_uri (uri, &hostname, NULL);

  if (hostname != NULL)
    result = g_ascii_strcasecmp (hostname, "localhost") == 0 || g_ascii_strcasecmp (hostname, g_get_host_name ()) == 0;
  else
    result = TRUE; /* consider it a local link */

  g_free(filename);
  g_free(hostname);

  return result;
}



static void
terminal_widget_hyperlink_hover_uri_changed (TerminalWidget     *widget,
                                             const char         *uri,
                                             const GdkRectangle *bbox G_GNUC_UNUSED)
{
  if (gtk_widget_get_realized (GTK_WIDGET (widget)) == FALSE)
    return;

  gtk_widget_set_tooltip_text (GTK_WIDGET (widget), uri);
}
