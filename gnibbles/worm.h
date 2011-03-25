/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>,
 *              Guillaume Béland <guillaume.beland@gmail.com>
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

#ifndef WORM_CLUTTER_H_
#define WORM_CLUTTER_H_

#include <gtk/gtk.h>
#include <clutter/clutter.h>

#define WORMNONE  0
#define WORMRIGHT 1
#define WORMDOWN  2
#define WORMLEFT  3
#define WORMUP    4
#define SLENGTH   5
#define SLIVES    10
#define ERASESIZE 6
#define ERASETIME 500
#define CAPACITY  BOARDWIDTH * BOARDHEIGHT
#define GROWFACTOR 4

typedef struct {
  ClutterActor *actors;
  GList *list;
  gint xstart, ystart;
  guint xhead, yhead;
  guint xtail, ytail;
  gint direction, direction_start;
  gint length;
  gint lives;
  guint score;
  guint number;
  gint change;
  gint keypress;
  gboolean human;
  gboolean stop;
} GnibblesWorm;

void worm_set_direction (int worm, int dir);
void worm_handle_direction (int worm, int dir);

GnibblesWorm* gnibbles_worm_new (guint number);
void gnibbles_worm_set_start (GnibblesWorm *worm, guint t_xhead,
                              guint t_yhead, gint t_direction);
void gnibbles_worm_show (GnibblesWorm *worm);
gboolean gnibbles_worm_handle_keypress (GnibblesWorm * worm, guint keyval);
void gnibbles_worm_move_head_pointer (GnibblesWorm *worm);

void gnibbles_worm_destroy (GnibblesWorm * worm);

void gnibbles_worm_rescale (GnibblesWorm *worm, gint tilesize);

void gnibbles_worm_reset (GnibblesWorm *worm);
void gnibbles_worm_move_head (GnibblesWorm *worm);
void gnibbles_worm_move_tail (GnibblesWorm *worm);
void gnibbles_worm_reduce_tail (GnibblesWorm *worm, gint erasesize);

gboolean gnibbles_worm_lose_life (GnibblesWorm * worm);

gboolean gnibbles_worm_can_move_to (GnibblesWorm * worm, gint x, gint y);
void gnibbles_worm_position_move_head (GnibblesWorm * worm, gint *x, gint *y);
gboolean gnibbles_worm_test_move_head (GnibblesWorm * worm);
gboolean gnibbles_worm_is_move_safe (GnibblesWorm * worm);

void gnibbles_worm_ai_move (GnibblesWorm * worm);

#endif
