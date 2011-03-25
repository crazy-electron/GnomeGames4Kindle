/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>,
 *              Guillaume Beland <guillaume.beland@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>

#include <stdlib.h>

#include <gtk/gtk.h>
#include <clutter-gtk/clutter-gtk.h>

#include "gnibbles.h"
#include "warp.h"
#include "warpmanager.h"
#include "boni.h"
#include "main.h"
#include "board.h"
#include "properties.h"

extern GnibblesBoard *board;
extern GnibblesBoni *boni;

extern GnibblesProperties *properties;
extern GdkPixbuf *boni_pixmaps[];

GnibblesWarpManager *
gnibbles_warpmanager_new (void)
{
  int i;
  GnibblesWarpManager *tmp;

  tmp = g_new (GnibblesWarpManager, 1);
  for (i = 0; i < MAXWARPS; i++)
    tmp->warps[i] = NULL;
  tmp->numwarps = 0;

  return tmp;
}

void
gnibbles_warpmanager_destroy (GnibblesWarpManager * warpmanager)
{
  gint i;

  for (i = 0; i < warpmanager->numwarps; i++) {
    clutter_actor_hide (warpmanager->warps[i]->actor);
    free (warpmanager->warps[i]);
  }
  warpmanager->numwarps = 0;
  free (warpmanager);
}

void
gnibbles_warpmanager_add_warp (GnibblesWarpManager * warpmanager, gint t_x,
                               gint t_y, gint t_wx, gint t_wy)
{
  gint i, add = 1, draw = 0;

  if (t_x < 0) {
    for (i = 0; i < warpmanager->numwarps; i++) {
      if (warpmanager->warps[i]->wx == t_x) {
        warpmanager->warps[i]->wx = t_wx;
        warpmanager->warps[i]->wy = t_wy;
        return;
      }
    }

    if (warpmanager->numwarps == MAXWARPS)
      return;
    warpmanager->warps[warpmanager->numwarps] =
                gnibbles_warp_new (t_x, t_y, t_wx, t_wy);
    warpmanager->numwarps++;
  } else {
    for (i = 0; i < warpmanager->numwarps; i++) {
      if (warpmanager->warps[i]->x == t_wx) {
        warpmanager->warps[i]->x = t_x;
        warpmanager->warps[i]->y = t_y;
        draw = i;
        add = 0;
      }
    }
    if (add) {
      if (warpmanager->numwarps == MAXWARPS)
        return;
      warpmanager->warps[warpmanager->numwarps] =
                    gnibbles_warp_new (t_x, t_y, t_wx, t_wy);
      draw = warpmanager->numwarps;
      warpmanager->numwarps++;
    }

    board->walls[t_x][t_y] = WARPLETTER;
    board->walls[t_x + 1][t_y] = WARPLETTER;
    board->walls[t_x][t_y + 1] = WARPLETTER;
    board->walls[t_x + 1][t_y + 1] = WARPLETTER;

    gnibbles_warp_draw (warpmanager->warps[draw]);
  }
}

void
gnibbles_warpmanager_worm_change_pos (GnibblesWarpManager * warpmanager,
                                      GnibblesWorm * worm)
{
  int i, x, y, good;

  for (i = 0; i < warpmanager->numwarps; i++) {
    if ((worm->xhead == warpmanager->warps[i]->x &&
        worm->yhead == warpmanager->warps[i]->y) ||
        (worm->xhead == warpmanager->warps[i]->x + 1 &&
        worm->yhead == warpmanager->warps[i]->y) ||
        (worm->xhead == warpmanager->warps[i]->x &&
        worm->yhead == warpmanager->warps[i]->y + 1) ||
        (worm->xhead == warpmanager->warps[i]->x + 1 &&
        worm->yhead == warpmanager->warps[i]->y + 1)) {

      if (warpmanager->warps[i]->wx == -1) {
         good = 0;
        while (!good) {
        // In network games, warps should be fair.
          if (ggz_network_mode) {
            x = 10 % BOARDWIDTH;
            y = 10 % BOARDHEIGHT;
          } else {
            x = rand () % BOARDWIDTH;
            y = rand () % BOARDHEIGHT;
          }
          if (board->walls[x][y] == EMPTYCHAR)
            good = 1;
        }
      } else {
        x = warpmanager->warps[i]->wx;
        y = warpmanager->warps[i]->wy;
        if (board->walls[x][y] != EMPTYCHAR)
          gnibbles_boni_remove_bonus (boni, x, y);
      }
      //reset warps
      board->walls
        [warpmanager->warps[i]->x][warpmanager->warps[i]->y] = WARPLETTER;
      board->walls
        [warpmanager->warps[i]->x + 1][warpmanager->warps[i]->y] = WARPLETTER;
      board->walls
        [warpmanager->warps[i]->x][warpmanager->warps[i]->y + 1] = WARPLETTER;
      board->walls
        [warpmanager->warps[i]->x+1][warpmanager->warps[i]->y+1] = WARPLETTER;

      worm->xhead = x;
      worm->yhead = y;
    }
  }
}

void
gnibbles_warpmanager_worm_change_tail_pos (GnibblesWarpManager * warpmanager,
                                           GnibblesWorm * worm)
{
  int i, x, y, good;

  for (i = 0; i < warpmanager->numwarps; i++) {
    if ((worm->xtail == warpmanager->warps[i]->x &&
        worm->ytail == warpmanager->warps[i]->y) ||
        (worm->xtail == warpmanager->warps[i]->x + 1 &&
        worm->ytail == warpmanager->warps[i]->y) ||
        (worm->xtail == warpmanager->warps[i]->x &&
        worm->ytail == warpmanager->warps[i]->y + 1) ||
        (worm->xtail == warpmanager->warps[i]->x + 1 &&
        worm->ytail == warpmanager->warps[i]->y + 1)) {

      if (warpmanager->warps[i]->wx == -1) {
         good = 0;
        while (!good) {
        // In network games, warps should be fair.
          if (ggz_network_mode) {
            x = 10 % BOARDWIDTH;
            y = 10 % BOARDHEIGHT;
          } else {
            x = rand () % BOARDWIDTH;
            y = rand () % BOARDHEIGHT;
          }
          if (board->walls[x][y] == EMPTYCHAR)
            good = 1;
        }
      } else {
        x = warpmanager->warps[i]->wx;
        y = warpmanager->warps[i]->wy;
        if (board->walls[x][y] != EMPTYCHAR)
          gnibbles_boni_remove_bonus (boni, x, y);
      }
      //reset warps
      board->walls
        [warpmanager->warps[i]->x][warpmanager->warps[i]->y] = WARPLETTER;
      board->walls
        [warpmanager->warps[i]->x + 1][warpmanager->warps[i]->y] = WARPLETTER;
      board->walls
        [warpmanager->warps[i]->x][warpmanager->warps[i]->y + 1] = WARPLETTER;
      board->walls
        [warpmanager->warps[i]->x+1][warpmanager->warps[i]->y+1] = WARPLETTER;

      worm->xtail = x;
      worm->ytail = y;
    }
  }
}

void
gnibbles_warpmanager_rescale (GnibblesWarpManager *warpmanager, gint tilesize)
{
  int i;
  gfloat x_pos, y_pos;
  GError *err = NULL;

  for (i = 0; i < warpmanager->numwarps; i++) {
    clutter_actor_get_position (warpmanager->warps[i]->actor, &x_pos, &y_pos);
    clutter_actor_set_position (warpmanager->warps[i]->actor,
                                (x_pos / properties->tilesize) * tilesize,
                                (y_pos / properties->tilesize) * tilesize);
    gtk_clutter_texture_set_from_pixbuf
      (GTK_CLUTTER_TEXTURE (warpmanager->warps[i]->actor), boni_pixmaps[WARP], &err);
    if (err)
      gnibbles_error (err->message);
  }
}
