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

#ifndef TERMINAL_IMAGE_LOADER_H
#define TERMINAL_IMAGE_LOADER_H

#include "terminal-preferences.h"

G_BEGIN_DECLS

#define TERMINAL_TYPE_IMAGE_LOADER (terminal_image_loader_get_type ())
G_DECLARE_FINAL_TYPE (TerminalImageLoader, terminal_image_loader, TERMINAL, IMAGE_LOADER, GObject)

TerminalImageLoader *
terminal_image_loader_get (void);

GdkPixbuf *
terminal_image_loader_load (TerminalImageLoader *loader,
                            gint width,
                            gint height);

G_END_DECLS

#endif /* !TERMINAL_IMAGE_LOADER_H */
