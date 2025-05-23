/*
 * Copyright © 1998, 2003 Jonathan Blandford <jrb@mit.edu>
 * Copyright © 2007, 2008, 2009 Christian Persch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "ar-cursor.h"

#include <gtk/gtk.h>

#ifndef HAVE_HILDON

/* These cursors borrowed from EOG */
/* FIXMEchpe use themeable cursors here! */

#if GTK_CHECK_VERSION (2, 91, 0)

#ifdef __SUNPRO_C
#pragma align 4 (hand_closed)
#endif
#ifdef __GNUC__
static const guint8 hand_closed_data[] __attribute__ ((__aligned__ (4))) =
#else
static const guint8 hand_closed_data[] =
#endif
{ ""
/* Pixbuf magic (0x47646b50) */
"GdkP"
/* length: header (24) + pixel_data (1600) */
"\0\0\6X"
/* pixdata_type (0x1010002) */
"\1\1\0\2"
/* rowstride (80) */
"\0\0\0P"
/* width (20) */
"\0\0\0\24"
/* height (20) */
"\0\0\0\24"
/* pixel_data: */
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\0"
"\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\377\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0"
"\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\0\0\0\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\377\0\0\0\377\0\0\0\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377"
"\377\377\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0"
"\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"};

#ifdef __SUNPRO_C
#pragma align 4 (hand_open)
#endif
#ifdef __GNUC__
static const guint8 hand_open_data[] __attribute__ ((__aligned__ (4))) =
#else
static const guint8 hand_open_data[] =
#endif
{ ""
/* Pixbuf magic (0x47646b50) */
"GdkP"
/* length: header (24) + pixel_data (1600) */
"\0\0\6X"
/* pixdata_type (0x1010002) */
"\1\1\0\2"
/* rowstride (80) */
"\0\0\0P"
/* width (20) */
"\0\0\0\24"
/* height (20) */
"\0\0\0\24"
/* pixel_data: */
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\0\0\0"
"\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\0\0\0\377"
"\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\377"
"\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\0\0"
"\0\377\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\377\377\377\377"
"\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377"
"\377\377\0\0\0\377\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\377"
"\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\377\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377"
"\377\377\377\377\377\377\377\0\0\0\377\377\377\377\377\377\377\377\377"
"\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\377\377\377"
"\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0\377\377\377\377"
"\377\377\377\377\377\0\0\0\377\377\377\377\377\377\377\377\377\0\0\0"
"\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\377\0\0\0\377\0\0\0\0\0\0\0\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\0\0\0\377\377\377\377\377\377\377\377"
"\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377"
"\377\377\377\377\377\0\0\0\377\0\0\0\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\0\0"
"\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\0\0\0\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\377\377"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
"\377\377\377\377\377\377\377\377\0\0\0\377\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"};

#define CURSOR_HOT_X 10
#define CURSOR_HOT_Y 10

static GdkCursor *
ar_cursor_new_from_data (GdkWindow *window,
                         const guint8 *data,
                         gsize data_len)
{
  GdkCursor *cursor;
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_inline (data_len, data, FALSE, NULL);
  cursor = gdk_cursor_new_from_pixbuf (gdk_window_get_display (window),
                                       pixbuf,
                                       CURSOR_HOT_X, CURSOR_HOT_Y);
  g_object_unref (pixbuf);

  return cursor;
}

#else

#define hand_closed_data_width 20
#define hand_closed_data_height 20
static const char hand_closed_data_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x80, 0x3f, 0x00,
  0x80, 0xff, 0x00, 0x80, 0xff, 0x00, 0xb0, 0xff, 0x00, 0xf0, 0xff, 0x00,
  0xe0, 0xff, 0x00, 0xe0, 0x7f, 0x00, 0xc0, 0x7f, 0x00, 0x80, 0x3f, 0x00,
  0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define hand_closed_mask_width 20
#define hand_closed_mask_height 20
static const char hand_closed_mask_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x80, 0x3f, 0x00, 0xc0, 0xff, 0x00,
  0xc0, 0xff, 0x01, 0xf0, 0xff, 0x01, 0xf8, 0xff, 0x01, 0xf8, 0xff, 0x01,
  0xf0, 0xff, 0x01, 0xf0, 0xff, 0x00, 0xe0, 0xff, 0x00, 0xc0, 0x7f, 0x00,
  0x80, 0x7f, 0x00, 0x80, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define hand_open_data_width 20
#define hand_open_data_height 20
static const char hand_open_data_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
  0x60, 0x36, 0x00, 0x60, 0x36, 0x00, 0xc0, 0x36, 0x01, 0xc0, 0xb6, 0x01,
  0x80, 0xbf, 0x01, 0x98, 0xff, 0x01, 0xb8, 0xff, 0x00, 0xf0, 0xff, 0x00,
  0xe0, 0xff, 0x00, 0xe0, 0x7f, 0x00, 0xc0, 0x7f, 0x00, 0x80, 0x3f, 0x00,
  0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define hand_open_mask_width 20
#define hand_open_mask_height 20
static const char hand_open_mask_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x60, 0x3f, 0x00,
  0xf0, 0x7f, 0x00, 0xf0, 0x7f, 0x01, 0xe0, 0xff, 0x03, 0xe0, 0xff, 0x03,
  0xd8, 0xff, 0x03, 0xfc, 0xff, 0x03, 0xfc, 0xff, 0x01, 0xf8, 0xff, 0x01,
  0xf0, 0xff, 0x01, 0xf0, 0xff, 0x00, 0xe0, 0xff, 0x00, 0xc0, 0x7f, 0x00,
  0x80, 0x7f, 0x00, 0x80, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static GdkCursor *
ar_cursor_new_from_data (GdkWindow *window,
                         const char *data,
                         const char *mask_data)
{
  GdkCursor *cursor;
  const GdkColor fg = { 0, 65535, 65535, 65535 };
  const GdkColor bg = { 0, 0, 0, 0 };
  GdkPixmap *source;
  GdkPixmap *mask;

  /* Yeah, hard-coded sizes are bad. */
  source = gdk_bitmap_create_from_data (window, data, 20, 20);
  mask = gdk_bitmap_create_from_data (window, mask_data, 20, 20);

  cursor = gdk_cursor_new_from_pixmap (source, mask, &fg, &bg, 10, 10);

  g_object_unref (source);
  g_object_unref (mask);

  return cursor;
}

#endif /* GTK 2.91.0 */

#endif /* !HAVE_HILDON */

GdkCursor *ar_cursor_new (GdkWindow *window,
                          ArCursorType cursor_type)
{
#ifndef HAVE_HILDON
  switch (cursor_type) {
    case AR_CURSOR_OPEN:
#if GTK_CHECK_VERSION (2, 90, 8)
      return ar_cursor_new_from_data (window, hand_open_data, sizeof (hand_open_data));
#else
      return ar_cursor_new_from_data (window, hand_open_data_bits, hand_open_mask_bits);
#endif

    case AR_CURSOR_CLOSED:
#if GTK_CHECK_VERSION (2, 90, 8)
      return ar_cursor_new_from_data (window, hand_closed_data, sizeof (hand_closed_data));
#else
      return ar_cursor_new_from_data (window, hand_closed_data_bits, hand_closed_mask_bits);
#endif

    default:
      g_assert_not_reached ();
  }
#else
  return NULL;
#endif /* !HAVE_HILDON */
}
