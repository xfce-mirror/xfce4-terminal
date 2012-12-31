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



static void       terminal_image_loader_finalize          (GObject                  *object);
static void       terminal_image_loader_check             (TerminalImageLoader      *loader);
static void       terminal_image_loader_pixbuf_destroyed  (gpointer                  data,
                                                           GObject                  *pixbuf);
static void       terminal_image_loader_tile              (TerminalImageLoader      *loader,
                                                           GdkPixbuf                *target,
                                                           gint                      width,
                                                           gint                      height);
static void       terminal_image_loader_center            (TerminalImageLoader      *loader,
                                                           GdkPixbuf                *target,
                                                           gint                      width,
                                                           gint                      height);
static void       terminal_image_loader_scale             (TerminalImageLoader      *loader,
                                                           GdkPixbuf                *target,
                                                           gint                      width,
                                                           gint                      height);
static void       terminal_image_loader_stretch           (TerminalImageLoader      *loader,
                                                           GdkPixbuf                *target,
                                                           gint                      width,
                                                           gint                      height);
static void       terminal_image_loader_saturate          (TerminalImageLoader      *loader,
                                                           GdkPixbuf                *pixbuf);


struct _TerminalImageLoaderClass
{
  GObjectClass  __parent__;
};

struct _TerminalImageLoader
{
  GObject                  __parent__;
  TerminalPreferences     *preferences;

  /* the cached image data */
  gchar                   *path;
  GSList                  *cache;
  GSList                  *cache_invalid;
  gdouble                  darkness;
  GdkColor                 bgcolor;
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

  terminal_assert (loader->cache == NULL);
  terminal_assert (loader->cache_invalid == NULL);

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
  GdkColor                selected_color;
  gboolean                invalidate = FALSE;
  gdouble                 selected_darkness;
  gchar                  *selected_color_spec;
  gchar                  *selected_path;

  terminal_return_if_fail (TERMINAL_IS_IMAGE_LOADER (loader));

  g_object_get (G_OBJECT (loader->preferences),
                "background-darkness", &selected_darkness,
                "background-image-file", &selected_path,
                "background-image-style", &selected_style,
                "color-background", &selected_color_spec,
                NULL);

  if (g_strcmp0 (selected_path, loader->path) != 0)
    {
      g_free (loader->path);
      loader->path = g_strdup (selected_path);

      if (GDK_IS_PIXBUF (loader->pixbuf))
        g_object_unref (G_OBJECT (loader->pixbuf));
      loader->pixbuf = gdk_pixbuf_new_from_file (loader->path, NULL);

      invalidate = TRUE;
    }

  if (selected_style != loader->style)
    {
      loader->style = selected_style;
      invalidate = TRUE;
    }

  if (selected_darkness != loader->darkness)
    {
      loader->darkness = selected_darkness;
      invalidate = TRUE;
    }

  gdk_color_parse (selected_color_spec, &selected_color);
  if (!gdk_color_equal (&selected_color, &loader->bgcolor))
    {
      loader->bgcolor = selected_color;
      invalidate = TRUE;
    }

  if (invalidate)
    {
      loader->cache_invalid = g_slist_concat (loader->cache_invalid,
                                              loader->cache);
      loader->cache = NULL;
    }

  g_free (selected_color_spec);
  g_free (selected_path);
}



static void
terminal_image_loader_pixbuf_destroyed (gpointer data,
                                        GObject *pixbuf)
{
  TerminalImageLoader *loader = TERMINAL_IMAGE_LOADER (data);
  GSList              *lp;

  for (lp = loader->cache; lp != NULL; lp = lp->next)
    if (lp->data == pixbuf)
      {
        loader->cache = g_slist_delete_link (loader->cache, lp);
        g_object_unref (G_OBJECT (loader));
        return;
      }

  for (lp = loader->cache_invalid; lp != NULL; lp = lp->next)
    if (lp->data == pixbuf)
      {
        loader->cache_invalid = g_slist_delete_link (loader->cache_invalid, lp);
        g_object_unref (G_OBJECT (loader));
        return;
      }

#ifdef G_ENABLE_DEBUG
  g_warning ("Pixbuf %p was freed from loader cache %p, "
             "this should not happend", pixbuf, loader);
  terminal_assert_not_reached ();
#endif
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
  rgba = (((loader->bgcolor.red & 0xff00) << 8)
        | ((loader->bgcolor.green & 0xff00))
        | ((loader->bgcolor.blue & 0xff00 >> 8))) << 8;
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
  rgba = (((loader->bgcolor.red & 0xff00) << 8)
        | ((loader->bgcolor.green & 0xff00))
        | ((loader->bgcolor.blue & 0xff00 >> 8))) << 8;
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
terminal_image_loader_saturate (TerminalImageLoader *loader,
                                GdkPixbuf           *pixbuf)
{
  guchar   *pixels;
  guchar    red[256];
  guchar    green[256];
  guchar    blue[256];
  gint      channels;
  gint      stride;
  gint      width;
  gint      height;
  gint      i;
  gint      x;
  gint      y;

  if (loader->darkness == 0)
    return;

  if (loader->darkness == 1)
    {
      for (i = 0; i < 256; ++i)
        {
          red[i] = loader->bgcolor.red >> 8;
          green[i] = loader->bgcolor.green >> 8;
          blue[i] = loader->bgcolor.blue >> 8;
        }
    }
  else
    {
      for (i = 0; i < 256; ++i)
        {
          red[i] = CLAMP ((loader->darkness * (loader->bgcolor.red >> 8))
                        + ((1.0 - loader->darkness) * i), 0, 255);
          green[i] = CLAMP ((loader->darkness * (loader->bgcolor.green >> 8))
                        + ((1.0 - loader->darkness) * i), 0, 255);
          blue[i] = CLAMP ((loader->darkness * (loader->bgcolor.blue >> 8))
                        + ((1.0 - loader->darkness) * i), 0, 255);
        }
    }

  stride = gdk_pixbuf_get_rowstride (pixbuf);
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  channels = gdk_pixbuf_get_n_channels (pixbuf);

  for (y = 0; y < height; ++y)
    {
      pixels = gdk_pixbuf_get_pixels (pixbuf) + y * stride;

      for (x = 0; x < width * channels; ++x)
        {
          switch (x % channels)
            {
            case 0:
              pixels[x] = red[pixels[x]];
              break;
            case 1:
              pixels[x] = green[pixels[x]];
              break;
            case 2:
              pixels[x] = blue[pixels[x]];
              break;
            default:
              break;
            }
        }
    }
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
           g_slist_length (loader->cache_invalid));
#endif

  /* check for a cached version */
  for (lp = loader->cache; lp != NULL; lp = lp->next)
    {
      pixbuf = GDK_PIXBUF (lp->data);
      if (gdk_pixbuf_get_height (pixbuf) == height
          && gdk_pixbuf_get_width (pixbuf) == width)
        {
          return g_object_ref (G_OBJECT (pixbuf));
        }
      else if (gdk_pixbuf_get_height (pixbuf) >= height
            && gdk_pixbuf_get_width (pixbuf) >= width
            && loader->style == TERMINAL_BACKGROUND_STYLE_TILED)
        {
          return g_object_ref (G_OBJECT (pixbuf));
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

    default:
      terminal_assert_not_reached ();
    }

  terminal_image_loader_saturate (loader, pixbuf);

  loader->cache = g_slist_prepend (loader->cache, pixbuf);
  g_object_weak_ref (G_OBJECT (pixbuf), terminal_image_loader_pixbuf_destroyed,
                     g_object_ref (G_OBJECT (loader)));

  return pixbuf;
}



