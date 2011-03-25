/*
 * Gnome-Mahjonggg-solubility header file
 * (C) 1998-2002 the Free Software Foundation
 *
 *
 * Author: Callum McKenzie.
 *         callum@physics.otago.ac.nz
 */

#ifndef MAPS_H
#define MAPS_H

#include <glib.h>

#define MAX_TILES 144

typedef struct _tilepos {
  int x;
  int y;
  int layer;
} tilepos;

typedef struct _map {
  gchar *name;
  gchar *score_name;
  tilepos *map;
  gboolean hardcoded;
  /* FIXME: These two are only used while building the map. */
  gint current;
  gint layer_t;
} map;

extern map *maps;
extern gint nmaps;

void load_maps (void);

#endif /* MAPS_H */
