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

/* This code is a complete rewrite of Michael Meeks original. The data
 * structures are modelled on those he used (and the typeinfo
 * structure is identical) but the algorithm is completely different.
 * 
 * The new algorithm works by randomly descending the tree of all
 * possible games (i.e all possible sequences of pairs of tiles) and
 * tries to find a path to a solution. If it can't get there it
 * backtracks up the tree trying all the possibilities in a random order.
 * In principle this could be very expensive computationally, but in
 * practice only a few levels of backtracking should be necessary,
 * although if an unsolvable board is entered then it could fail
 * spectacularly (it will figure out that it is unsolvable, but only
 * after a very long time). This may be unavoidable, but I suspect that
 * given the contraints of the board (specifically the maximum height) the
 * largest unsolvable board that can be made is not a big problem. Once
 * it has a path it fills it with pairs of tiles chosen at random.
 *
 * The algorithm is also used to shuffle the tiles.
 *
 * - Callum, 20030819
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include "mahjongg.h"
#include "solubility.h"

#define SWAPINT(x,y) do { gint t; t = (x); (x) = (y); (y) = t;  } while (0);

struct _typeinfo {
  int type;
  int placed;
  int image[2];
  int tile;			/* A back-reference so we can figure out which pictures 
				 * are used. */
};
typedef struct _typeinfo typeinfo;

typeinfo type_info[MAX_TILES / 2] = {
  {0, 0, {0, 0}, -1},
  {0, 0, {0, 0}, -1},
  {1, 0, {1, 1}, -1},
  {1, 0, {1, 1}, -1},
  {2, 0, {2, 2}, -1},
  {2, 0, {2, 2}, -1},
  {3, 0, {3, 3}, -1},
  {3, 0, {3, 3}, -1},
  {4, 0, {4, 4}, -1},
  {4, 0, {4, 4}, -1},
  {5, 0, {5, 5}, -1},
  {5, 0, {5, 5}, -1},
  {6, 0, {6, 6}, -1},
  {6, 0, {6, 6}, -1},
  {7, 0, {7, 7}, -1},
  {7, 0, {7, 7}, -1},
  {8, 0, {8, 8}, -1},
  {8, 0, {8, 8}, -1},
  {9, 0, {9, 9}, -1},
  {9, 0, {9, 9}, -1},
  {10, 0, {10, 10}, -1},
  {10, 0, {10, 10}, -1},
  {11, 0, {11, 11}, -1},
  {11, 0, {11, 11}, -1},
  {12, 0, {12, 12}, -1},
  {12, 0, {12, 12}, -1},
  {13, 0, {13, 13}, -1},
  {13, 0, {13, 13}, -1},
  {14, 0, {14, 14}, -1},
  {14, 0, {14, 14}, -1},
  {15, 0, {15, 15}, -1},
  {15, 0, {15, 15}, -1},
  {16, 0, {16, 16}, -1},
  {16, 0, {16, 16}, -1},
  {17, 0, {17, 17}, -1},
  {17, 0, {17, 17}, -1},
  {18, 0, {18, 18}, -1},
  {18, 0, {18, 18}, -1},
  {19, 0, {19, 19}, -1},
  {19, 0, {19, 19}, -1},
  {20, 0, {20, 20}, -1},
  {20, 0, {20, 20}, -1},
  {21, 0, {21, 21}, -1},
  {21, 0, {21, 21}, -1},
  {22, 0, {22, 22}, -1},
  {22, 0, {22, 22}, -1},
  {23, 0, {23, 23}, -1},
  {23, 0, {23, 23}, -1},
  {24, 0, {24, 24}, -1},
  {24, 0, {24, 24}, -1},
  {25, 0, {25, 25}, -1},
  {25, 0, {25, 25}, -1},
  {26, 0, {26, 26}, -1},
  {26, 0, {26, 26}, -1},
  {27, 0, {27, 27}, -1},
  {27, 0, {27, 27}, -1},
  {28, 0, {28, 28}, -1},
  {28, 0, {28, 28}, -1},
  {29, 0, {29, 29}, -1},
  {29, 0, {29, 29}, -1},
  {30, 0, {30, 30}, -1},
  {30, 0, {30, 30}, -1},
  {31, 0, {31, 31}, -1},
  {31, 0, {31, 31}, -1},
  {32, 0, {32, 32}, -1},
  {32, 0, {32, 32}, -1},
  {33, 0, {33, 34}, -1},
  {33, 0, {35, 36}, -1},
  {34, 0, {37, 37}, -1},
  {34, 0, {37, 37}, -1},
  {35, 0, {38, 39}, -1},
  {35, 0, {40, 41}, -1}
};

typedef struct _dep_entry {
  gint foundation[4];		/* Up to four we build on. */
  gint left[2];			/* Up to two on the left. */
  gint right[2];		/* and two on the right. */
  gint overhead[4];		/* Up to four we support. */
} dep_entry;

dep_entry dependencies[MAX_TILES];

gint numfree;

/* These could be placed on the stack, but they're a little large, so
 * we use the depth variable as a sort of virtual stack pointer. */
gboolean freetiles[MAX_TILES / 2][MAX_TILES];
gboolean filled[MAX_TILES];	/* This is changed incrementally with
				 * changes being undone as the program
				 * moves back towards the base of the
				 * tree. */
guchar freelist[MAX_TILES / 2][MAX_TILES];

GRand *generator;

int
tile_free (int index)
{
  dep_entry *dep;
  int i;
  gboolean free;

  dep = dependencies + index;

  if (tiles[index].visible == 0)
    return 0;

  /* Check to see we aren't covered. */
  for (i = 0; i < 4; i++) {
    if ((dep->overhead[i] != -1) && tiles[dep->overhead[i]].visible)
      return 0;
  }

  /* Look left. */
  free = TRUE;
  for (i = 0; i < 2; i++) {
    if ((dep->left[i] != -1) && tiles[dep->left[i]].visible)
      free = FALSE;
  }
  if (free)
    return 1;

  /* Look right. */
  free = TRUE;
  for (i = 0; i < 2; i++) {
    if ((dep->right[i] != -1) && tiles[dep->right[i]].visible)
      free = FALSE;
  }

  return free ? 1 : 0;

}

void
generate_dependencies (void)
{
  int i, j;
  int fc, lc, rc, oc;
  int x, y, l;
  int tx, ty, tl;
  dep_entry *dep;

  dep = dependencies;
  for (i = 0; i < MAX_TILES; i++) {
    x = pos[i].x;
    y = pos[i].y;
    l = pos[i].layer;

    dep->foundation[0] = dep->foundation[1] = dep->foundation[2]
      = dep->foundation[3] = -1;
    dep->left[0] = dep->left[1] = -1;
    dep->right[0] = dep->right[1] = -1;
    dep->overhead[0] = dep->overhead[1] = dep->overhead[2]
      = dep->overhead[3] = -1;
    fc = lc = rc = oc = 0;
    for (j = 0; j < MAX_TILES; j++) {
      ty = pos[j].y;

      /* Nothing we are interested in is outside +/- 1 units on the y axis. */
      if (abs (ty - y) > 1)
	continue;

      tl = pos[j].layer;
      tx = pos[j].x;

      /* First check if it is a foundation tile. */
      if ((tl == (l - 1)) && (abs (tx - x) < 2)) {
	dep->foundation[fc++] = j;
	continue;
      }

      /* Then do the overhead tiles. */
      if ((tl == (l + 1)) && (abs (tx - x) < 2)) {
	dep->overhead[oc++] = j;
	continue;
      }

      /* Everything else is on this layer. */
      if (tl != l)
	continue;

      /* Now look left ... */
      if (tx == (x - 2)) {
	dep->left[lc++] = j;
	continue;
      }

      /* ... and right. */
      if (tx == (x + 2)) {
	dep->right[rc++] = j;
	continue;
      }
    }
    dep++;
  }
}


/* This is the private version that additionally does some book
 * keeping for the generate game function. */
static gboolean
check_tile_is_free (gint index, gint depth)
{
  int i;
  gboolean ok;
  dep_entry *dep;

  dep = dependencies + index;

  if (!filled[index])
    return FALSE;

  if (freetiles[depth][index])
    return TRUE;

  /* First, check if we are covered above. */
  for (i = 0; i < 4; i++) {
    if (dep->overhead[i] == -1)
      break;
    if (filled[dep->overhead[i]])
      return FALSE;
  }

  /* Now check to the left. */
  ok = TRUE;
  for (i = 0; i < 2; i++) {
    if (dep->left[i] == -1)
      break;
    if (filled[dep->left[i]]) {
      ok = FALSE;
      break;
    }
  }
  if (ok) {
    freetiles[depth][index] = TRUE;
    numfree++;
    return TRUE;
  }

  /* And now the right. */
  ok = TRUE;
  for (i = 0; i < 2; i++) {
    if (dep->right[i] == -1)
      break;
    if (filled[dep->right[i]]) {
      ok = FALSE;
      break;
    }
  }
  if (ok) {
    freetiles[depth][index] = TRUE;
    numfree++;
    return TRUE;
  }
  return FALSE;
}

/* Check all the tiles around a given tile to see if they have become
 * free. This is called after a pair of tiles has been removed. */
static void
check_around (guint index, gint depth)
{
  guint i;
  guint target;

  /* See if we've freed anything below us. */
  for (i = 0; i < 4; i++) {
    target = dependencies[index].foundation[i];
    if (target == -1)
      break;
    check_tile_is_free (target, depth);
  }

  /* See if we've freed anything to the left or right. */
  for (i = 0; i < 2; i++) {
    target = dependencies[index].left[i];
    if (target != -1)
      check_tile_is_free (target, depth);
    target = dependencies[index].right[i];
    if (target != -1)
      check_tile_is_free (target, depth);
  }
}

/* Assign a tile pair to a given pair of positions on the pile. */
static void
place_tiles (guint a, guint b, gint depth)
{
  tiles[a].visible = tiles[b].visible = TRUE;
  tiles[a].selected = tiles[b].selected = FALSE;
  tiles[a].type = tiles[b].type = type_info[depth].type;
  tiles[a].image = type_info[depth].image[0];
  tiles[b].image = type_info[depth].image[1];
  type_info[depth].tile = a;
}

static gboolean
walk_tree (gint depth)
{
  gint i;
  gint j;
  guchar swap;
  gint oldnumfree;
  guchar a, b;

  /* The termination condition. */
  if (numfree < 2) {		/* We don't have enough tiles to continue. */
    return FALSE;
  }

  /* Get a list of free tiles remaining. This is a compacted version
   * of freetiles. freetiles should have been constructed by the previous
   * iteration. */
  j = 0;
  for (i = 0; i < MAX_TILES; i++)
    if (freetiles[depth][i]) {
      freelist[depth][j++] = i;
    }

  if (depth == MAX_TILES / 2 - 1) {	/* We have reached the end with
					 * precisely two tiles to place. */
    place_tiles (freelist[depth][0], freelist[depth][1], depth);
    return TRUE;
  }

  /* If we aren't at the end we make an exhaustive search for a walk down
   * the tree of possible games. While in principle this could take a very
   * long time, it will in general only affect the very end of any walk. */

  /* Scramble the freelist. */
  for (i = 0; i < numfree; i++) {
    j = g_rand_int_range (generator, i, numfree);
    swap = freelist[depth][i];
    freelist[depth][i] = freelist[depth][j];
    freelist[depth][j] = swap;
  }


  /* Try out all possibilities, bailing out if we can't find a path of
   * suitable depth. */
  for (i = 0; i < numfree - 1; i++) {
    for (j = i + 1; j < numfree; j++) {
      a = freelist[depth][i];
      b = freelist[depth][j];
      /* This is all a bit messy, but has a distinct advantage over the
       * cleaner, cleverer method I tried: it works. */
      oldnumfree = numfree;
      numfree -= 2;
      memcpy (freetiles[depth + 1], freetiles[depth],
	      MAX_TILES * sizeof (gboolean));
      freetiles[depth + 1][a] = FALSE;
      freetiles[depth + 1][b] = FALSE;
      filled[a] = FALSE;
      filled[b] = FALSE;
      check_around (a, depth + 1);
      check_around (b, depth + 1);
      if (walk_tree (depth + 1)) {
	/* We have found a path to the end. Place a pair of tiles
	 * and go up to the next level. */
	place_tiles (a, b, depth);
	return TRUE;
      }
      filled[a] = TRUE;
      filled[b] = TRUE;
      numfree = oldnumfree;
    }
  }

  /* We failed to find a route. */
  return FALSE;
}

void
generate_game (guint32 seed)
{
  guint i, j;
  typeinfo tile;

  generator = g_rand_new_with_seed (seed);

  /* Scramble the tiles */
  for (i = 0; i < MAX_TILES; i++) {
    j = g_rand_int_range (generator, 0, MAX_TILES / 2);
    tile = type_info[0];
    type_info[0] = type_info[j];
    type_info[j] = tile;
  }

  /* Find which tiles are initially free. */
  numfree = 0;
  for (i = 0; i < MAX_TILES; i++) {
    filled[i] = TRUE;
    tiles[i].sequence = 0;
  }
  for (i = 0; i < MAX_TILES; i++) {
    freetiles[0][i] = FALSE;
    check_tile_is_free (i, 0);
  }

  /* Now find a random path through the tree of all possible games. */
  if (walk_tree (0))
    return;

  /* FIXME: we should report the error that the pile cannot be solved. */
}

int
shuffle (void)
{
  int n = 0;
  int i, j;
  typeinfo temp;

  /* Shuffle the images around so that we use the same set of tile images
   * as last time. */
  for (i = 0; i < MAX_TILES / 2 - 1; i++) {
    if (tiles[type_info[i].tile].visible)
      type_info[i].tile = 1;
    else
      type_info[i].tile = 0;
  }
  i = 0;
  j = MAX_TILES / 2 - 1;
  while (1) {
    while (j && type_info[j].tile)
      j--;
    while ((i < MAX_TILES / 2) && (type_info[i].tile == 0))
      i++;
    if (j < i)
      break;
    temp = type_info[i];
    type_info[i] = type_info[j];
    type_info[j] = temp;
    j--;
    i++;
  }

  /* Figure out which ones are available and which aren't. 
   * Also figure out what depth in the tree we have to start
   * at, i.e. how far through the game we are. */
  for (i = 0; i < MAX_TILES; i++) {
    if (tiles[i].visible) {
      filled[i] = TRUE;
    } else {
      filled[i] = FALSE;
      n++;
    }
  }
  n >>= 1;

  /* Check which ones are playable. */
  numfree = 0;
  for (i = 0; i < MAX_TILES; i++) {
    freetiles[n][i] = FALSE;
    if (tiles[i].visible) {
      check_tile_is_free (i, n);
    }
  }

  /* Work out a new, random, solution. */
  return walk_tree (n);
}
