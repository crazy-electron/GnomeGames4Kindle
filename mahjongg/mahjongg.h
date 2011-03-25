/*
 * Gnome-Mahjonggg main header
 * (C) 1998-1999 the Free Software Foundation
 *
 *
 * Author: Francisco Bustamante et al.
 *
 *
 * http://www.nuclecu.unam.mx/~pancho/
 * pancho@nuclecu.unam.mx
 *
 */

#ifndef MAHJONGG_H
#define MAHJONGG_H

#include <glib.h>

#include "maps.h"

#define MAX_TILES_STR "144"

extern tilepos *pos;

typedef struct _tile tile;

struct _tile {
  int type;
  int image;
  int visible;
  int selected;
  int sequence;
  int number;
};

void tile_event (gint tileno, gint button);

void mahjongg_theme_warning (gchar * message_format);

extern tile tiles[MAX_TILES];
extern gboolean paused;
extern gchar *tileset;

#endif /* MAHJONGG_H */
