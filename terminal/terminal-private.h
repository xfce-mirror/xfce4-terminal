/*-
 * Copyright (c) 2006-2007 Benedikt Meurer <benny@xfce.org>
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

#ifndef __TERMINAL_PRIVATE_H__
#define __TERMINAL_PRIVATE_H__

#include <glib-object.h>

G_BEGIN_DECLS

/* returns true if string contains text */
#define IS_STRING(string) ((string) != NULL && *(string) != '\0')

/* macro for some debugging */
#define PRINT_TIME(desc) \
  G_BEGIN_DECLS { \
    GTimeVal __tv; \
    g_get_current_time (&__tv); \
    g_print ("%ld.%ld: %s\n", __tv.tv_sec, __tv.tv_usec, desc); \
  } G_END_DECLS

/* support macros for debugging */
#ifdef G_ENABLE_DEBUG
#define terminal_assert(expr)                  g_assert (expr)
#define terminal_assert_not_reached()          g_assert_not_reached ()
#define terminal_return_if_fail(expr)          g_return_if_fail (expr)
#define terminal_return_val_if_fail(expr, val) g_return_val_if_fail (expr, (val))
#else
#define terminal_assert(expr)                  G_STMT_START{ (void)0; }G_STMT_END
#define terminal_assert_not_reached()          G_STMT_START{ (void)0; }G_STMT_END
#define terminal_return_if_fail(expr)          G_STMT_START{ (void)0; }G_STMT_END
#define terminal_return_val_if_fail(expr, val) G_STMT_START{ (void)0; }G_STMT_END
#endif

/* avoid trivial g_value_get_*() function calls */
#ifdef G_ENABLE_DEBUG
#define g_value_get_boolean(v)  (((const GValue *) (v))->data[0].v_int)
#define g_value_get_char(v)     (((const GValue *) (v))->data[0].v_int)
#define g_value_get_uchar(v)    (((const GValue *) (v))->data[0].v_uint)
#define g_value_get_int(v)      (((const GValue *) (v))->data[0].v_int)
#define g_value_get_uint(v)     (((const GValue *) (v))->data[0].v_uint)
#define g_value_get_long(v)     (((const GValue *) (v))->data[0].v_long)
#define g_value_get_ulong(v)    (((const GValue *) (v))->data[0].v_ulong)
#define g_value_get_int64(v)    (((const GValue *) (v))->data[0].v_int64)
#define g_value_get_uint64(v)   (((const GValue *) (v))->data[0].v_uint64)
#define g_value_get_enum(v)     (((const GValue *) (v))->data[0].v_long)
#define g_value_get_flags(v)    (((const GValue *) (v))->data[0].v_ulong)
#define g_value_get_float(v)    (((const GValue *) (v))->data[0].v_float)
#define g_value_get_double(v)   (((const GValue *) (v))->data[0].v_double)
#define g_value_get_string(v)   (((const GValue *) (v))->data[0].v_pointer)
#define g_value_get_param(v)    (((const GValue *) (v))->data[0].v_pointer)
#define g_value_get_boxed(v)    (((const GValue *) (v))->data[0].v_pointer)
#define g_value_get_pointer(v)  (((const GValue *) (v))->data[0].v_pointer)
#define g_value_get_object(v)   (((const GValue *) (v))->data[0].v_pointer)
#endif

#define I_(string) (g_intern_static_string ((string)))

G_END_DECLS

#endif /* !__TERMINAL_PRIVATE_H__ */
