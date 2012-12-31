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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TERMINAL_IMAGE_LOADER_H__
#define __TERMINAL_IMAGE_LOADER_H__

#include <terminal/terminal-preferences.h>

G_BEGIN_DECLS

#define TERMINAL_TYPE_IMAGE_LOADER            (terminal_image_loader_get_type ())
#define TERMINAL_IMAGE_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TERMINAL_TYPE_IMAGE_LOADER, TerminalImageLoader))
#define TERMINAL_IMAGE_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TERMINAL_TYPE_IMAGE_LOADER, TerminalImageLoaderClass))
#define TERMINAL_IS_IMAGE_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TERMINAL_TYPE_IMAGE_LOADER))
#define TERMINAL_IS_IMAGE_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TERMINAL_TYPE_IMAGE_LOADER))
#define TERMINAL_IMAGE_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TERMINAL_TYPE_IMAGE_LOADER, TerminalImageLoaderClass))

typedef struct _TerminalImageLoaderClass TerminalImageLoaderClass;
typedef struct _TerminalImageLoader      TerminalImageLoader;

GType                terminal_image_loader_get_type   (void) G_GNUC_CONST;

TerminalImageLoader *terminal_image_loader_get        (void);

GdkPixbuf           *terminal_image_loader_load (TerminalImageLoader *loader,
                                                 gint                 width,
                                                 gint                 height);

G_END_DECLS

#endif /* !__TERMINAL_IMAGE_LOADER_H__ */
