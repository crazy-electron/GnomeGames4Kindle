/* -*- Mode: C; indent-tabs-mode: nil; tab-width: 8; c-basic-offset: 2 -*- */

/*
 * Gnome-Mahjongg pile creation algorithm
 *
 * (C) 2003 the Free Software Foundation
 *
 * Author: Callum McKenzie, <callum@physics.otago.ac.nz>
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include <glib/gi18n.h>

#include <libgames-support/games-files.h>
#include <libgames-support/games-runtime.h>

#include "mahjongg.h"
#include "maps.h"


/* Sorted such that the bottom leftest are first, and layers decrease
 * Bottom left = high y, low x ! */

/* Easy map is compiled in, the rest of the layouts were moved to 
 * external files */

tilepos easy_map[MAX_TILES] = {
  {13, 7, 4}
  , {12, 8, 3}
  , {14, 8, 3}
  , {12, 6, 3}
  ,
  {14, 6, 3}
  , {10, 10, 2}
  , {12, 10, 2}
  , {14, 10, 2}
  ,
  {16, 10, 2}
  , {10, 8, 2}
  , {12, 8, 2}
  , {14, 8, 2}
  ,
  {16, 8, 2}
  , {10, 6, 2}
  , {12, 6, 2}
  , {14, 6, 2}
  ,
  {16, 6, 2}
  , {10, 4, 2}
  , {12, 4, 2}
  , {14, 4, 2}
  ,
  {16, 4, 2}
  , {8, 12, 1}
  , {10, 12, 1}
  , {12, 12, 1}
  ,
  {14, 12, 1}
  , {16, 12, 1}
  , {18, 12, 1}
  , {8, 10, 1}
  ,
  {10, 10, 1}
  , {12, 10, 1}
  , {14, 10, 1}
  , {16, 10, 1}
  ,
  {18, 10, 1}
  , {8, 8, 1}
  , {10, 8, 1}
  , {12, 8, 1}
  ,
  {14, 8, 1}
  , {16, 8, 1}
  , {18, 8, 1}
  , {8, 6, 1}
  ,
  {10, 6, 1}
  , {12, 6, 1}
  , {14, 6, 1}
  , {16, 6, 1}
  ,
  {18, 6, 1}
  , {8, 4, 1}
  , {10, 4, 1}
  , {12, 4, 1}
  ,
  {14, 4, 1}
  , {16, 4, 1}
  , {18, 4, 1}
  , {8, 2, 1}
  ,
  {10, 2, 1}
  , {12, 2, 1}
  , {14, 2, 1}
  , {16, 2, 1}
  ,
  {18, 2, 1}
  , {2, 14, 0}
  , {4, 14, 0}
  , {6, 14, 0}
  ,
  {8, 14, 0}
  , {10, 14, 0}
  , {12, 14, 0}
  , {14, 14, 0}
  ,
  {16, 14, 0}
  , {18, 14, 0}
  , {20, 14, 0}
  , {22, 14, 0}
  ,
  {24, 14, 0}
  , {6, 12, 0}
  , {8, 12, 0}
  , {10, 12, 0}
  ,
  {12, 12, 0}
  , {14, 12, 0}
  , {16, 12, 0}
  , {18, 12, 0}
  ,
  {20, 12, 0}
  , {4, 10, 0}
  , {6, 10, 0}
  , {8, 10, 0}
  ,
  {10, 10, 0}
  , {12, 10, 0}
  , {14, 10, 0}
  , {16, 10, 0}
  ,
  {18, 10, 0}
  , {20, 10, 0}
  , {22, 10, 0}
  , {0, 7, 0}
  ,
  {2, 8, 0}
  , {4, 8, 0}
  , {6, 8, 0}
  , {8, 8, 0}
  ,
  {10, 8, 0}
  , {12, 8, 0}
  , {14, 8, 0}
  , {16, 8, 0}
  ,
  {18, 8, 0}
  , {20, 8, 0}
  , {22, 8, 0}
  , {24, 8, 0}
  ,
  {2, 6, 0}
  , {4, 6, 0}
  , {6, 6, 0}
  , {8, 6, 0}
  ,
  {10, 6, 0}
  , {12, 6, 0}
  , {14, 6, 0}
  , {16, 6, 0}
  ,
  {18, 6, 0}
  , {20, 6, 0}
  , {22, 6, 0}
  , {24, 6, 0}
  ,
  {4, 4, 0}
  , {6, 4, 0}
  , {8, 4, 0}
  , {10, 4, 0}
  ,
  {12, 4, 0}
  , {14, 4, 0}
  , {16, 4, 0}
  , {18, 4, 0}
  ,
  {20, 4, 0}
  , {22, 4, 0}
  , {6, 2, 0}
  , {8, 2, 0}
  ,
  {10, 2, 0}
  , {12, 2, 0}
  , {14, 2, 0}
  , {16, 2, 0}
  ,
  {18, 2, 0}
  , {20, 2, 0}
  , {2, 0, 0}
  , {4, 0, 0}
  ,
  {6, 0, 0}
  , {8, 0, 0}
  , {10, 0, 0}
  , {12, 0, 0}
  ,
  {14, 0, 0}
  , {16, 0, 0}
  , {18, 0, 0}
  , {20, 0, 0}
  ,
  {22, 0, 0}
  , {24, 0, 0}
  , {26, 7, 0}
  , {28, 7, 0}
};

map hardcoded[] = {
  { NC_("mahjongg map name", "Easy"), "easy", easy_map, TRUE, 0, 0}
  ,
};

GList *maplist;

#define TAG_NAME_MAHJONGG "mahjongg"
#define TAG_NAME_MAP "map"
#define TAG_NAME_LAYER "layer"
#define TAG_NAME_ROW "row"
#define TAG_NAME_COLUMN "column"
#define TAG_NAME_BLOCK "block"
#define TAG_NAME_TILE "tile"

#define ATTR_NAME "name"
#define ATTR_SCORENAME "scorename"
#define ATTR_LEFT "left"
#define ATTR_RIGHT "right"
#define ATTR_TOP "top"
#define ATTR_BOTTOM "bottom"
#define ATTR_X "x"
#define ATTR_Y "y"
#define ATTR_Z "z"

tilepos testmap[MAX_TILES];

gboolean in_mahjongg;

static gint x1, x2, y1, y2, z;

const gchar *name, *score_name;

enum {
  ATTR_INT,
  ATTR_HALFINT,
  ATTR_STRING,
  ATTR_END
};

struct attrs {
  gchar *name;
  gint length;
  gint type;
  void *value;
} attributes[] = { {
ATTR_NAME, sizeof (ATTR_NAME), ATTR_STRING, &name}
, {
ATTR_SCORENAME, sizeof (ATTR_SCORENAME), ATTR_STRING, &score_name}
, {
ATTR_LEFT, sizeof (ATTR_LEFT), ATTR_HALFINT, &x1}
, {
ATTR_RIGHT, sizeof (ATTR_RIGHT), ATTR_HALFINT, &x2}
, {
ATTR_TOP, sizeof (ATTR_TOP), ATTR_HALFINT, &y1}
, {
ATTR_BOTTOM, sizeof (ATTR_BOTTOM), ATTR_HALFINT, &y2}
, {
ATTR_X, sizeof (ATTR_X), ATTR_HALFINT, &x1}
, {
ATTR_Y, sizeof (ATTR_Y), ATTR_HALFINT, &y1}
, {
ATTR_Z, sizeof (ATTR_Z), ATTR_INT, &z}
, {
NULL, 0, ATTR_END, NULL}
};
struct attrs *a;

enum {
  TAG_MAHJONGG,
  TAG_MAP,
  TAG_LAYER,
  TAG_BLOCK,
  TAG_ROW,
  TAG_COLUMN,
  TAG_TILE,
  TAG_END
};

struct tgs {
  gchar *name;
  gint length;
  gint value;
} tags[] = { {
TAG_NAME_MAHJONGG, sizeof (TAG_NAME_MAHJONGG), TAG_MAHJONGG}
, {
TAG_NAME_MAP, sizeof (TAG_NAME_MAP), TAG_MAP}
, {
TAG_NAME_LAYER, sizeof (TAG_NAME_LAYER), TAG_LAYER}
, {
TAG_NAME_BLOCK, sizeof (TAG_NAME_BLOCK), TAG_BLOCK}
, {
TAG_NAME_ROW, sizeof (TAG_NAME_ROW), TAG_ROW}
, {
TAG_NAME_COLUMN, sizeof (TAG_NAME_COLUMN), TAG_COLUMN}
, {
TAG_NAME_TILE, sizeof (TAG_NAME_TILE), TAG_TILE}
, {
NULL, TAG_END}
};
struct tgs *t;

GMarkupParser parser;

map *maps = NULL;
gint nmaps = 0;

static void
parse_start_el (GMarkupParseContext * context,
		const gchar * element_name,
		const gchar ** attribute_names,
		const gchar ** attribute_values,
		gpointer user_data, GError ** error)
{
  map *m;
  gint tag;
  gint y;

  /* Identify the tag. */
  tag = TAG_END;
  t = tags;
  while (t->value != TAG_END) {
    if (g_ascii_strncasecmp (element_name, t->name, t->length) == 0) {
      tag = t->value;
      break;
    }
    t++;
  }

  /* Ignore anything outside of a pair of <mahjongg> tags. */
  if (!in_mahjongg) {
    in_mahjongg = tag == TAG_MAHJONGG;
    return;
  }

  /* Scan the attributes. */
  x1 = x2 = y1 = y2 = z = 0;
  name = score_name = NULL;

  m = NULL;

  if (maplist) {
    m = maplist->data;
    z = m->layer_t;
  }

  /* NOTE: We can't use atof() since it uses the locale to decide on the format.
   * For instance in Russian the number "1.5" will be decoded as "1" (Bug #386213) */
  while (*attribute_names) {
    a = attributes;
    while (a->type != ATTR_END) {
      if (g_ascii_strncasecmp (*attribute_names, a->name, a->length) == 0) {
	switch (a->type) {
	case ATTR_INT:
	  *(gint *) (a->value) = (gint) (g_ascii_strtod (*attribute_values, NULL));
	  break;
	case ATTR_HALFINT:
	  *(gint *) (a->value) = (gint) (2 * g_ascii_strtod (*attribute_values, NULL));
	  break;
	case ATTR_STRING:
	  *(const gchar **) (a->value) = *attribute_values;
	  break;
	}
	break;
      }
      a++;
    }

    attribute_names++;
    attribute_values++;
  }

  /* Handle setting up the maplist separately. */
  if (tag == TAG_MAP) {
    m = g_malloc0 (sizeof (map));
    maplist = g_list_prepend (maplist, m);
    m->map = g_malloc0 (sizeof (tilepos) * MAX_TILES);
    m->current = 0;
    m->hardcoded = FALSE;
    m->name = g_strdup (name);
    m->score_name = g_strdup (score_name);
    return;
  }

  /* This only happens if the input file is malformed. */
  if (!maplist)
    return;

  /* Now handle the rest of the tags. */
  switch (tag) {
  case TAG_LAYER:
    m->layer_t = CLAMP (z, 0, 255);
    break;
  case TAG_ROW:
    for (; x1 <= x2; x1 += 2) {
      m->map[m->current].x = x1;
      m->map[m->current].y = y1;
      m->map[m->current].layer = z;
      if (m->current < MAX_TILES)
	m->current++;
    }
    break;
  case TAG_COLUMN:
    for (; y1 <= y2; y1 += 2) {
      m->map[m->current].x = x1;
      m->map[m->current].y = y1;
      m->map[m->current].layer = z;
      if (m->current < MAX_TILES)
	m->current++;
    }
    break;
  case TAG_BLOCK:
    for (; x1 <= x2; x1 += 2) {
      for (y = y1; y <= y2; y += 2) {
	m->map[m->current].x = x1;
	m->map[m->current].y = y;
	m->map[m->current].layer = z;
	if (m->current < MAX_TILES)
	  m->current++;
      }
    }
    break;
  case TAG_TILE:
    m->map[m->current].x = x1;
    m->map[m->current].y = y1;
    m->map[m->current].layer = z;
    if (m->current < MAX_TILES)
      m->current++;
    break;
  }

}

/* Sorted such that the bottom leftest are first, and layers decrease
 * Bottom left = high y, low x ! */
static int
compare_tiles (tilepos * a, tilepos * b)
{
  int dx, dy;

  dx = b->x - a->x;
  dy = b->y - a->y;

  if (b->layer < a->layer)
    return -1;
  if (b->layer > a->layer)
    return +1;
  if (dx > dy)
    return -1;
  if (dx < dy)
    return +1;

  return 0;
}

static void
parse_end_el (GMarkupParseContext * context,
	      const gchar * element_name, gpointer user_data, GError ** error)
{
  map *m;

  if (!in_mahjongg)
    return;

  /* FIXME: What if we end mahjongg without closing <map> ? */

  if (g_ascii_strncasecmp (TAG_NAME_MAHJONGG, element_name,
			   sizeof (TAG_NAME_MAHJONGG)) == 0) {
    in_mahjongg = FALSE;
    return;
  }

  if (g_ascii_strncasecmp (TAG_NAME_MAP, element_name,
			   sizeof (TAG_NAME_MAP)) == 0) {
    m = maplist->data;
    if (m->current < MAX_TILES)
      g_warning ("Too few tiles in map %s", m->name);
    qsort (m->map, MAX_TILES, sizeof (tilepos),
	   (int (*)(const void *, const void *)) compare_tiles);
  }
}

static void
load_map_from_file (gchar * filename)
{
  GMarkupParseContext *parse_context;
  gboolean ok;
  gchar *file;
  gsize length;

  /* Read in a map file. */
  ok = g_file_get_contents (filename, &file, &length, NULL);

  if (!ok) {
    g_warning ("Could not read file %s\n", filename);
    return;
  }

  /* Parse the map file. */
  in_mahjongg = FALSE;

  parse_context = g_markup_parse_context_new (&parser, 0, NULL, NULL);
  g_markup_parse_context_parse (parse_context, file, length, NULL);

  /* FIXME: Handle bad maps (i.e. throw them off the list) */
  /* FIXME: Check for duplicates. */
}

void
load_maps (void)
{
  int i;
  map *m;
  GamesFileList *filelist;
  const char *dname;

  /* Initialise the parser. */
  parser.start_element = parse_start_el;
  parser.end_element = parse_end_el;
  parser.text = NULL;
  parser.passthrough = NULL;
  parser.error = NULL;

  maplist = NULL;

  /* Load up the hard-coded games. */
  for (i = 0; i < G_N_ELEMENTS (hardcoded); i++) {
    maplist = g_list_prepend (maplist, hardcoded + i);
  }

  /* Now read the remainder in from file. */
  dname =  games_runtime_get_directory (GAMES_RUNTIME_GAME_GAMES_DIRECTORY);
  filelist = games_file_list_new ("*.map", ".", dname, NULL);
  games_file_list_for_each (filelist, (GFunc) load_map_from_file, NULL);

  /* FIXME: Ideally we would do this transformation, but the old code 
   * expects an array, so we give it one. */
  g_free (maps);

  nmaps = g_list_length (maplist);

  maps = g_malloc0 (sizeof (map) * nmaps);

  for (i = 0; i < nmaps; i++) {
    m = maplist->data;
    g_memmove (maps + i, maplist->data, sizeof (map));
    if (!m->hardcoded)
      g_free (maplist->data);
    maplist = g_list_delete_link (maplist, maplist);
  }
}
