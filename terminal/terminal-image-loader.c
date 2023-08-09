/*-
 * Copyright (c) 2004-2007 os-cillation e.K.
 * Copyright (c) 2003      Red Hat, Inc.
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

#include <terminal/terminal-image-loader.h>
#include <terminal/terminal-private.h>

/* max image resolution is 8K */
#define MAX_IMAGE_WIDTH  7680
#define MAX_IMAGE_HEIGHT 4320
#define CACHE_SIZE 10



static void terminal_image_loader_finalize (GObject             *object);
static void terminal_image_loader_check    (TerminalImageLoader *loader);
static void terminal_image_loader_tile     (TerminalImageLoader *loader,
                                            GdkPixbuf           *target,
                                            gint                 width,
                                            gint                 height);
static void terminal_image_loader_center   (TerminalImageLoader *loader,
                                            GdkPixbuf           *target,
                                            gint                 width,
                                            gint                 height);
static void terminal_image_loader_scale    (TerminalImageLoader *loader,
                                            GdkPixbuf           *target,
                                            gint                 width,
                                            gint                 height);
static void terminal_image_loader_stretch  (TerminalImageLoader *loader,
                                            GdkPixbuf           *target,
                                            gint                 width,
                                            gint                 height);
static void terminal_image_loader_fill     (TerminalImageLoader *loader,
                                            GdkPixbuf           *target,
                                            gint                 width,
                                            gint                 height);


struct _TerminalImageLoaderClass
{
  GObjectClass parent_class;
};

struct _TerminalImageLoader
{
  GObject                  parent_instance;
  TerminalPreferences     *preferences;

  /* the cached image data */
  gchar                   *path;
  GSList                  *cache;
  GdkRGBA                  bgcolor;
  GdkPixbuf               *pixbuf;
  TerminalBackgroundStyle  style;
};



G_DEFINE_TYPE (TerminalImageLoader, terminal_image_loader, G_TYPE_OBJECT)



static void
terminal_image_loader_class_init (TerminalImageLoaderClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = terminal_image_loader_finalize;
}



static void
terminal_image_loader_init (TerminalImageLoader *loader)
{
  loader->preferences = terminal_preferences_get ();
}



static void
terminal_image_loader_finalize (GObject *object)
{
  TerminalImageLoader *loader = TERMINAL_IMAGE_LOADER (object);

  g_slist_free_full (loader->cache, g_object_unref);

  g_object_unref (G_OBJECT (loader->preferences));

  if (G_LIKELY (loader->pixbuf != NULL))
    g_object_unref (G_OBJECT (loader->pixbuf));
  g_free (loader->path);

  (*G_OBJECT_CLASS (terminal_image_loader_parent_class)->finalize) (object);
}



static void
terminal_image_loader_check (TerminalImageLoader *loader)
{
  TerminalBackgroundStyle selected_style;
  GdkRGBA                 selected_color;
  gboolean                invalidate = FALSE;
  gchar                  *selected_color_spec;
  gchar                  *selected_path;

  terminal_return_if_fail (TERMINAL_IS_IMAGE_LOADER (loader));

  g_object_get (G_OBJECT (loader->preferences),
                "background-image-file", &selected_path,
                "background-image-style", &selected_style,
                "color-background", &selected_color_spec,
                NULL);

  if (g_strcmp0 (selected_path, loader->path) != 0)
    {
      gint width, height;

      g_free (loader->path);
      loader->path = g_strdup (selected_path);

      if (GDK_IS_PIXBUF (loader->pixbuf))
        g_object_unref (G_OBJECT (loader->pixbuf));

      if (gdk_pixbuf_get_file_info (loader->path, &width, &height) == NULL)
        {
          g_warning ("Unable to load background image file \"%s\"", loader->path);
          loader->pixbuf = NULL;
        }
      else if (width <= MAX_IMAGE_WIDTH && height <= MAX_IMAGE_HEIGHT)
        loader->pixbuf = gdk_pixbuf_new_from_file (loader->path, NULL);
      else
        loader->pixbuf = gdk_pixbuf_new_from_file_at_size (loader->path,
                                                           MAX_IMAGE_WIDTH, MAX_IMAGE_WIDTH,
                                                           NULL);

      invalidate = TRUE;
    }

  if (selected_style != loader->style)
    {
      loader->style = selected_style;
      invalidate = TRUE;
    }

  gdk_rgba_parse (&selected_color, selected_color_spec);
  if (!gdk_rgba_equal (&selected_color, &loader->bgcolor))
    {
      loader->bgcolor = selected_color;
      invalidate = TRUE;
    }

  if (invalidate)
    {
      g_slist_free_full (loader->cache, g_object_unref);
      loader->cache = NULL;
    }

  g_free (selected_color_spec);
  g_free (selected_path);
}



static void
terminal_image_loader_tile (TerminalImageLoader *loader,
                            GdkPixbuf           *target,
                            gint                 width,
                            gint                 height)
{
  GdkRectangle area;
  gint         source_width;
  gint         source_height;
  gint         i;
  gint         j;

  source_width = gdk_pixbuf_get_width (loader->pixbuf);
  source_height = gdk_pixbuf_get_height (loader->pixbuf);

  for (i = 0; (i * source_width) < width; ++i)
    for (j = 0; (j * source_height) < height; ++j)
      {
        area.x = i * source_width;
        area.y = j * source_height;
        area.width = source_width;
        area.height = source_height;

        if (area.x + area.width > width)
          area.width = width - area.x;
        if (area.y + area.height > height)
          area.height = height - area.y;

        gdk_pixbuf_copy_area (loader->pixbuf, 0, 0,
                              area.width, area.height,
                              target, area.x, area.y);
      }
}



static void
terminal_image_loader_center (TerminalImageLoader *loader,
                              GdkPixbuf           *target,
                              gint                 width,
                              gint                 height)
{
  guint32 rgba;
  gint    source_width;
  gint    source_height;
  gint    dx;
  gint    dy;
  gint    x0;
  gint    y0;

  /* fill with background color */
  rgba = ((((guint)(loader->bgcolor.red * 65535) & 0xff00) << 8)
        | (((guint)(loader->bgcolor.green * 65535) & 0xff00))
        | (((guint)(loader->bgcolor.blue * 65535) & 0xff00) >> 8)) << 8;
  gdk_pixbuf_fill (target, rgba);

  source_width = gdk_pixbuf_get_width (loader->pixbuf);
  source_height = gdk_pixbuf_get_height (loader->pixbuf);

  dx = MAX ((width - source_width) / 2, 0);
  dy = MAX ((height - source_height) / 2, 0);
  x0 = MIN ((width - source_width) / 2, dx);
  y0 = MIN ((height - source_height) / 2, dy);

  gdk_pixbuf_composite (loader->pixbuf, target, dx, dy,
                        MIN (width, source_width),
                        MIN (height, source_height),
                        x0, y0, 1.0, 1.0,
                        GDK_INTERP_BILINEAR, 255);
}



static void
terminal_image_loader_scale (TerminalImageLoader *loader,
                             GdkPixbuf           *target,
                             gint                 width,
                             gint                 height)
{
  gdouble xscale;
  gdouble yscale;
  guint32 rgba;
  gint    source_width;
  gint    source_height;
  gint    x;
  gint    y;

  /* fill with background color */
  rgba = ((((guint)(loader->bgcolor.red * 65535) & 0xff00) << 8)
        | (((guint)(loader->bgcolor.green * 65535) & 0xff00))
        | (((guint)(loader->bgcolor.blue * 65535) & 0xff00) >> 8)) << 8;
  gdk_pixbuf_fill (target, rgba);

  source_width = gdk_pixbuf_get_width (loader->pixbuf);
  source_height = gdk_pixbuf_get_height (loader->pixbuf);

  xscale = (gdouble) width / source_width;
  yscale = (gdouble) height / source_height;

  if (xscale < yscale)
    {
      yscale = xscale;
      x = 0;
      y = (height - (source_height * yscale)) / 2;
    }
  else
    {
      xscale = yscale;
      x = (width - (source_width * xscale)) / 2;
      y = 0;
    }

  gdk_pixbuf_composite (loader->pixbuf, target, x, y,
                        source_width * xscale,
                        source_height * yscale,
                        x, y, xscale, yscale,
                        GDK_INTERP_BILINEAR, 255);
}



static void
terminal_image_loader_stretch (TerminalImageLoader *loader,
                               GdkPixbuf           *target,
                               gint                 width,
                               gint                 height)
{
  gdouble xscale;
  gdouble yscale;
  gint    source_width;
  gint    source_height;

  source_width = gdk_pixbuf_get_width (loader->pixbuf);
  source_height = gdk_pixbuf_get_height (loader->pixbuf);

  xscale = (gdouble) width / source_width;
  yscale = (gdouble) height / source_height;

  gdk_pixbuf_composite (loader->pixbuf, target,
                        0, 0, width, height,
                        0, 0, xscale, yscale,
                        GDK_INTERP_BILINEAR, 255);
}



static void
terminal_image_loader_fill    (TerminalImageLoader *loader,
                               GdkPixbuf           *target,
                               gint                 width,
                               gint                 height)
{
  gdouble xscale;
  gdouble yscale;
  gdouble scale;
  gdouble xoff;
  gdouble yoff;
  gint    source_width;
  gint    source_height;

  source_width = gdk_pixbuf_get_width (loader->pixbuf);
  source_height = gdk_pixbuf_get_height (loader->pixbuf);

  xscale = (gdouble) width / source_width;
  yscale = (gdouble) height / source_height;
  if (xscale < yscale)
    {
       scale = yscale;
       xoff = ((scale - xscale) * source_width) * -0.5;
       yoff = 0;
    }
  else
    {
       scale = xscale;
       xoff = 0;
       yoff = ((scale - yscale) * source_height) * -0.5;
    }
  scale = MAX(xscale,yscale);

  gdk_pixbuf_scale (loader->pixbuf, target,
                        0, 0, width, height,
                        xoff, yoff, scale, scale,
                        GDK_INTERP_BILINEAR);
}



/**
 * terminal_image_loader_get:
 *
 * Returns the default #TerminalImageLoader instance. The returned
 * pointer is already ref'ed, call g_object_unref() if you don't
 * need it any longer.
 *
 * Return value : The default #TerminalImageLoader instance.
 **/
TerminalImageLoader*
terminal_image_loader_get (void)
{
  static TerminalImageLoader *loader = NULL;

  if (G_UNLIKELY (loader == NULL))
    {
      loader = g_object_new (TERMINAL_TYPE_IMAGE_LOADER, NULL);
      g_object_add_weak_pointer (G_OBJECT (loader), (gpointer) &loader);
    }
  else
    {
      g_object_ref (G_OBJECT (loader));
    }

  return loader;
}



/**
 * terminal_image_loader_load:
 * @loader      : A #TerminalImageLoader.
 * @width       : The image width.
 * @height      : The image height.
 *
 * Return value : The image in the given @width and @height drawn with
 *                the configured style or %NULL on error.
 **/
GdkPixbuf*
terminal_image_loader_load (TerminalImageLoader *loader,
                            gint                 width,
                            gint                 height)
{
  GdkPixbuf *pixbuf;
  GSList    *lp;

  terminal_return_val_if_fail (TERMINAL_IS_IMAGE_LOADER (loader), NULL);
  terminal_return_val_if_fail (width > 0, NULL);
  terminal_return_val_if_fail (height > 0, NULL);

  terminal_image_loader_check (loader);

  if (G_UNLIKELY (loader->pixbuf == NULL || width <= 1 || height <= 1))
    return NULL;

#ifdef G_ENABLE_DEBUG
  g_debug ("Image Loader Memory Status: %d images in valid "
           "cache, %d in invalid cache",
           g_slist_length (loader->cache),
#endif

  /* check for a cached version */
  for (lp = loader->cache; lp != NULL; lp = lp->next)
    {
      gint w, h;
      pixbuf = GDK_PIXBUF (lp->data);
      w = gdk_pixbuf_get_width (pixbuf);
      h = gdk_pixbuf_get_height (pixbuf);

      if ((w == width && h == height) ||
          (w >= width && h >= height && loader->style == TERMINAL_BACKGROUND_STYLE_TILED))
        {
          return GDK_PIXBUF (g_object_ref (G_OBJECT (pixbuf)));
        }
    }

  pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (loader->pixbuf),
                           gdk_pixbuf_get_has_alpha (loader->pixbuf),
                           gdk_pixbuf_get_bits_per_sample (loader->pixbuf),
                           width, height);

  switch (loader->style)
    {
    case TERMINAL_BACKGROUND_STYLE_TILED:
      terminal_image_loader_tile (loader, pixbuf, width, height);
      break;

    case TERMINAL_BACKGROUND_STYLE_CENTERED:
      terminal_image_loader_center (loader, pixbuf, width, height);
      break;

    case TERMINAL_BACKGROUND_STYLE_SCALED:
      terminal_image_loader_scale (loader, pixbuf, width, height);
      break;

    case TERMINAL_BACKGROUND_STYLE_STRETCHED:
      terminal_image_loader_stretch (loader, pixbuf, width, height);
      break;

    case TERMINAL_BACKGROUND_STYLE_FILLED:
      terminal_image_loader_fill (loader, pixbuf, width, height);
      break;

    default:
      terminal_assert_not_reached ();
    }

  loader->cache = g_slist_prepend (loader->cache, pixbuf);
  lp = g_slist_nth (loader->cache, CACHE_SIZE - 1);
  if (lp != NULL)
    {
      g_object_unref (lp->data);
      loader->cache = g_slist_delete_link (loader->cache, lp);
    }

  return GDK_PIXBUF (g_object_ref (G_OBJECT (pixbuf)));
}


