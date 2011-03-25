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
#include <config.h>

#include <glib/gprintf.h>
#include <ctype.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <math.h>

#include <libgames-support/games-sound.h>
#include <libgames-support/games-runtime.h>
#include <clutter-gtk/clutter-gtk.h>

#include "main.h"
#include "gnibbles.h"
#include "boni.h"
#include "bonus.h"
#include "warpmanager.h"
#include "properties.h"
#include "board.h"
#ifdef GGZ_CLIENT
#include "ggz-network.h"
#endif

#include "worm.h"

extern GnibblesProperties *properties;
extern GdkPixbuf *worm_pixmaps[];
extern GnibblesBoni *boni;
extern GnibblesWarpManager *warpmanager;
extern GnibblesWorm *worms[NUMWORMS];
extern GnibblesBoard *board;

extern ClutterActor *stage;

extern gint current_level;

typedef struct _key_queue_entry {
  GnibblesWorm *worm;
  guint dir;
} key_queue_entry;

static GQueue *key_queue[NUMWORMS] = { NULL, NULL, NULL, NULL };

static void
gnibbles_worm_queue_keypress (GnibblesWorm * worm, guint dir)
{
  key_queue_entry *entry;
  int n = worm->number;

  if (key_queue[n] == NULL)
    key_queue[n] = g_queue_new ();

  /* Ignore duplicates in normal movement mode. This resolves the
   * key repeat issue. We ignore this in relative mode because then
   * you do want two keys that are the same in quick succession. */
  if ((!properties->wormprops[worm->number]->relmove) &&
      (!g_queue_is_empty (key_queue[n])) &&
      (dir == ((key_queue_entry *) g_queue_peek_tail (key_queue[n]))->dir))
    return;

  entry = g_new (key_queue_entry, 1);
  entry->worm = worm;
  entry->dir = dir;
  g_queue_push_tail (key_queue[n], (gpointer) entry);
}

void
worm_set_direction (int worm, int dir)
{
  if (!worms[worm]->human)
    return;

  if (worms[worm]) {

    if (dir > 4)
      dir = 1;
    if (dir < 1)
      dir = 4;

    if (worms[worm]->keypress) {
      gnibbles_worm_queue_keypress (worms[worm], dir);
      return;
    }
    worms[worm]->direction = dir;
    worms[worm]->keypress = 1;
  }
}

void
worm_handle_direction (int worm, int dir)
{
  if (ggz_network_mode) {
#ifdef GGZ_CLIENT
    network_game_move (dir);

    worms[0]->direction = dir;
    worms[0]->keypress = 1;
#endif
  } else {
    worm_set_direction (worm, dir);
  }
}

static void
gnibbles_worm_queue_empty (GnibblesWorm * worm)
{
  key_queue_entry *entry;
  int n = worm->number;

  if (!key_queue[n])
    return;
  while (!g_queue_is_empty (key_queue[n])) {
    entry = g_queue_pop_head (key_queue[n]);
    g_free (entry);
  }
}

static void
gnibbles_worm_dequeue_keypress (GnibblesWorm * worm)
{
  key_queue_entry *entry;
  int n = worm->number;

  entry = (key_queue_entry *) g_queue_pop_head (key_queue[n]);

  worm_set_direction (entry->worm->number, entry->dir);

  g_free (entry);
}

static ClutterActor*
gnibbles_worm_get_head_actor (GnibblesWorm *worm)
{
  return CLUTTER_ACTOR (g_list_first (worm->list)->data);
}

static ClutterActor*
gnibbles_worm_get_tail_actor (GnibblesWorm *worm)
{
  return CLUTTER_ACTOR (g_list_last (worm->list)->data);
}

static void
gnibbles_worm_add_actor (GnibblesWorm *worm)
{
  ClutterActor *actor;
  GError *error = NULL;

  actor = gtk_clutter_texture_new ();
  gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (actor),
                                       worm_pixmaps[properties->wormprops[worm->number]->color - 12],
                                       &error);
  clutter_actor_set_size (actor, properties->tilesize, properties->tilesize);
  clutter_actor_set_position (actor,
                              worm->xhead * properties->tilesize,
                              worm->yhead * properties->tilesize);

  clutter_container_add_actor (CLUTTER_CONTAINER (worm->actors), actor);
  worm->list = g_list_prepend (worm->list, actor);
  board->walls[worm->xhead][worm->yhead] = WORMCHAR + worm->number;
}

static void
gnibbles_worm_remove_actor (GnibblesWorm *worm)
{
  ClutterActor *actor = gnibbles_worm_get_tail_actor (worm);
  board->walls[worm->xtail][worm->ytail] = EMPTYCHAR;
  clutter_actor_hide (actor);
  worm->list = g_list_delete_link (worm->list, g_list_last (worm->list));
  clutter_container_remove_actor (CLUTTER_CONTAINER (worm->actors), actor);
}

gboolean
gnibbles_worm_handle_keypress (GnibblesWorm * worm, guint keyval)
{
  GnibblesWormProps *props;
  guint propsUp, propsLeft, propsDown, propsRight, keyvalUpper;

  if (worm->lives <= 0)
    return FALSE;

  props = properties->wormprops[ggz_network_mode ? 0 : worm->number];
  propsUp = toupper(props->up);
  propsLeft = toupper(props->left);
  propsDown = toupper(props->down);
  propsRight = toupper(props->right);
  keyvalUpper = toupper(keyval);

  if (properties->wormprops[worm->number]->relmove) {
    if (keyvalUpper == propsLeft) {
      worm_handle_direction (worm->number, worm->direction - 1);
    } else if (keyvalUpper == propsRight) {
      worm_handle_direction (worm->number, worm->direction + 1);
    } else {
      return FALSE;
    }
    return TRUE;
  } else {
    if ((keyvalUpper == propsUp) && (worm->direction != WORMDOWN)) {
      worm_handle_direction (worm->number, WORMUP);
      return TRUE;
    }
    if ((keyvalUpper == propsRight) && (worm->direction != WORMLEFT)) {
      worm_handle_direction (worm->number, WORMRIGHT);
      return TRUE;
    }
    if ((keyvalUpper == propsDown) && (worm->direction != WORMUP)) {
      worm_handle_direction (worm->number, WORMDOWN);
      return TRUE;
    }
    if ((keyvalUpper == propsLeft) && (worm->direction != WORMRIGHT)) {
      worm_handle_direction (worm->number, WORMLEFT);
      return TRUE;
    }
  }
  return FALSE;
}

static gint
gnibbles_worm_get_tail_direction (GnibblesWorm *worm)
{
  gfloat x1,y1,x2,y2;
  gfloat xdiff, ydiff;

  ClutterActor *next = NULL;
  ClutterActor *tail = gnibbles_worm_get_tail_actor (worm);

  if (g_list_length (worm->list) >= 2)
    next = CLUTTER_ACTOR (g_list_previous (g_list_last (worm->list))->data);
  else
    return worm->direction;

  clutter_actor_get_position (CLUTTER_ACTOR (next), &x2, &y2);
  clutter_actor_get_position (CLUTTER_ACTOR (tail), &x1, &y1);

  xdiff = MAX (x2,x1) - MIN (x2,x1);
  ydiff = MAX (y2,y1) - MIN (y2,y1);

  if (x2 > x1 && fabs (y1 - y2) < 0.0001)
    return xdiff > properties->tilesize ? WORMLEFT : WORMRIGHT;
  else if (x2 < x1 && fabs (y1 - y2) < 0.0001)
    return xdiff > properties->tilesize ? WORMRIGHT : WORMLEFT;
  else if (y2 > y1 && fabs (x1 - x2) < 0.0001)
    return ydiff > properties->tilesize ? WORMUP: WORMDOWN;
  else if (y2 < y1 && fabs (x1 - x2) < 0.0001)
    return ydiff > properties->tilesize ? WORMDOWN : WORMUP;
  else
    return -1;
}

static gboolean
gnibbles_worm_reverse (gpointer data)
{
  GnibblesWorm *worm = (GnibblesWorm *) data;
  gint tmp, old_dir;

  old_dir = gnibbles_worm_get_tail_direction (worm);

  worm->list = g_list_reverse (worm->list);

  tmp = worm->xhead;
  worm->xhead = worm->xtail;
  worm->xtail = tmp;
  tmp = worm->yhead;
  worm->yhead = worm->ytail;
  worm->ytail = tmp;
  tmp = worm->yhead;

  if (old_dir == WORMRIGHT)
    worm->direction = WORMLEFT;
  else if (old_dir == WORMLEFT)
    worm->direction = WORMRIGHT;
  else if (old_dir == WORMUP)
    worm->direction = WORMDOWN;
  else if (old_dir == WORMDOWN)
    worm->direction = WORMUP;

  return FALSE;
}

static void
gnibbles_worm_grok_bonus (GnibblesWorm *worm)
{
  int i;

  if (gnibbles_boni_fake (boni, worm->xhead, worm->yhead)) {
    g_timeout_add (1, (GSourceFunc) gnibbles_worm_reverse, worm);
    games_sound_play ("reverse");
    return;
  }

  switch (board->walls[worm->xhead][worm->yhead] - 'A') {
    case BONUSREGULAR:
      boni->numleft--;
      worm->change += (NUMBONI - boni->numleft) * GROWFACTOR;
      worm->score += (NUMBONI - boni->numleft) * current_level;
      games_sound_play ("gobble");
      break;
    case BONUSDOUBLE:
      worm->score += (worm->length + worm->change) * current_level;
      worm->change += worm->length + worm->change;
      games_sound_play ("bonus");
      break;
    case BONUSHALF:
      if (worm->length + worm->change > 2) {
        worm->score += ((worm->length + worm->change) / 2) * current_level;
        gnibbles_worm_reduce_tail (worm,
                                  (g_list_length (worm->list)
                                  + worm->change) / 2);
        worm->change -= (g_list_length (worm->list) + worm->change) / 2;
        games_sound_play ("bonus");
      }
      break;
    case BONUSLIFE:
      worm->lives += 1;
      games_sound_play ("life");
      break;
    case BONUSREVERSE:
      for (i = 0; i < properties->numworms; i++)
        if (worm != worms[i])
          g_timeout_add (1, (GSourceFunc)
                         gnibbles_worm_reverse, worms[i]);
      games_sound_play ("reverse");
      break;
  }
}

static void
worm_grok_scale_down (ClutterAnimation *animation, ClutterActor *actor)
{
  clutter_actor_animate (actor, CLUTTER_EASE_OUT_QUINT, 420,
                         "scale-x", 1.0, "scale-y", 1.0,
                         "fixed::scale-gravity", CLUTTER_GRAVITY_CENTER,
                         NULL);
}

static void
gnibbles_worm_handle_bonus (GnibblesWorm *worm)
{
  ClutterActor *actor = NULL;

  if ((board->walls[worm->xhead][worm->yhead] != EMPTYCHAR) &&
    (board->walls[worm->xhead][worm->yhead] != WARPLETTER)) {
    actor = gnibbles_worm_get_head_actor (worm);
    g_signal_connect_after (
      clutter_actor_animate (actor, CLUTTER_EASE_OUT_QUINT, 420,
                            "scale-x", 1.45, "scale-y", 1.45,
                            "fixed::scale-gravity", CLUTTER_GRAVITY_CENTER,
                            NULL),
      "completed", G_CALLBACK (worm_grok_scale_down), actor);
    gnibbles_worm_grok_bonus (worm);

    if ((board->walls[worm->xhead][worm->yhead] == BONUSREGULAR + 'A') &&
        !gnibbles_boni_fake (boni, worm->xhead, worm->yhead)) {

      gnibbles_boni_remove_bonus_final (boni, worm->xhead, worm->yhead);

      if (boni->numleft != 0)
        gnibbles_board_level_add_bonus (board, 1);

    } else
        gnibbles_boni_remove_bonus_final (boni, worm->xhead, worm->yhead);
  }

  if (board->walls[worm->xhead][worm->yhead] == WARPLETTER) {
    gnibbles_warpmanager_worm_change_pos (warpmanager, worm);
    games_sound_play ("teleport");
  }
}

void
gnibbles_worm_move_head_pointer (GnibblesWorm *worm)
{
  switch (worm->direction) {
    case WORMRIGHT:
      worm->xhead++;
      break;
    case WORMDOWN:
      worm->yhead++;
      break;
    case WORMLEFT:
      worm->xhead--;
      break;
    case WORMUP:
      worm->yhead--;
      break;
    default:
      break;
  }

  if (worm->xhead <= 0)
    worm->xhead = BOARDWIDTH - 1;
  if (worm->yhead <= 0)
    worm->yhead = BOARDHEIGHT - 1;
  if (worm->xhead >= BOARDWIDTH)
    worm->xhead = 0;
  if (worm->yhead >= BOARDHEIGHT)
    worm->yhead = 0;

  gnibbles_worm_handle_bonus (worm);
  gnibbles_worm_add_actor (worm);
}

static void
gnibbles_worm_move_tail_pointer (GnibblesWorm *worm)
{
  gint tail_dir = gnibbles_worm_get_tail_direction (worm);
  gnibbles_worm_remove_actor (worm);

  switch (tail_dir) {
    case WORMRIGHT:
      worm->xtail++;
      break;
    case WORMDOWN:
      worm->ytail++;
      break;
    case WORMLEFT:
      worm->xtail--;
      break;
    case WORMUP:
      worm->ytail--;
      break;
    default:
      break;
  }

  if (worm->xtail <= 0)
    worm->xtail = BOARDWIDTH - 1;
  if (worm->ytail <= 0)
    worm->ytail = BOARDHEIGHT - 1;
  if (worm->xtail >= BOARDWIDTH)
    worm->xtail = 0;
  if (worm->ytail >= BOARDHEIGHT)
    worm->ytail = 0;

  if (board->walls[worm->xtail][worm->ytail] == WARPLETTER) {
    gnibbles_warpmanager_worm_change_tail_pos (warpmanager, worm);
    tail_dir = gnibbles_worm_get_tail_direction (worm);
    board->walls[worm->xtail][worm->ytail] = EMPTYCHAR;
    switch (tail_dir) {
      case WORMRIGHT:
        worm->xtail++;
        break;
      case WORMDOWN:
        worm->ytail++;
        break;
      case WORMLEFT:
        worm->xtail--;
        break;
      case WORMUP:
        worm->ytail--;
        break;
      default:
        break;
      }
  }

}

static void
gnibbles_worm_animate_death (GnibblesWorm *worm)
{
  ClutterActor *group = clutter_group_new ();
  ClutterActor *tmp = NULL;
  GError *error = NULL;

  int i;
  gfloat x,y;

  for (i = 0; i < g_list_length (worm->list); i++) {
    tmp = gtk_clutter_texture_new ();
    gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                         worm_pixmaps[properties->wormprops[worm->number]->color - 12],
                                         &error);

    clutter_actor_get_position (CLUTTER_ACTOR (g_list_nth_data (worm->list, i)),
                                &x, &y);

    clutter_actor_set_position (CLUTTER_ACTOR (tmp), x, y);
    clutter_actor_set_size (CLUTTER_ACTOR (tmp),
                            properties->tilesize,
                            properties->tilesize);
    clutter_container_add_actor (CLUTTER_CONTAINER (group), tmp);
  }

  worm->length = g_list_length (worm->list);
  for (i = 0; i < worm->length ; i++)
    worm->list = g_list_remove (worm->list, g_list_nth_data (worm->list, i));

  clutter_actor_set_opacity (CLUTTER_ACTOR (worm->actors), 0x00);

  clutter_group_remove_all (CLUTTER_GROUP (worm->actors));
  g_list_free (worm->list);
  worm->list = NULL;

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), group);

  clutter_actor_animate (group, CLUTTER_EASE_OUT_QUAD, 310,
                         "opacity", 0,
                         "scale-x", 2.0,
                         "scale-y", 2.0,
                         "fixed::scale-center-x",
                         (gfloat) worm->xhead * properties->tilesize,
                         "fixed::scale-center-y",
                         (gfloat) worm->yhead * properties->tilesize,
                         NULL);
}

GnibblesWorm*
gnibbles_worm_new (guint number)
{
  GnibblesWorm *worm = g_new (GnibblesWorm, 1);

  worm->actors = clutter_group_new ();
  worm->list = NULL;
  worm->number = number;
  worm->lives = SLIVES;
  worm->human = FALSE;
  worm->score = 0;

  return worm;
}

void
gnibbles_worm_set_start (GnibblesWorm *worm, guint t_xhead,
                         guint t_yhead, gint t_direction)
{
  int i;
  worm->length = g_list_length (worm->list);
  for (i = 0; i < worm->length ; i++)
    worm->list = g_list_remove (worm->list, g_list_nth_data (worm->list, i));
  g_list_free (worm->list);
  worm->list = NULL;

  clutter_group_remove_all (CLUTTER_GROUP (worm->actors));

  worm->xhead = t_xhead;
  worm->yhead = t_yhead;
  worm->xtail = t_xhead;
  worm->ytail = t_yhead;
  worm->xstart = t_xhead;
  worm->ystart = t_yhead;

  worm->direction = t_direction;
  worm->direction_start = t_direction;
  worm->length = SLENGTH;
  worm->change = 0;
  worm->stop = FALSE;

  gnibbles_worm_queue_empty (worm);
}

void
gnibbles_worm_show (GnibblesWorm *worm)
{
  clutter_actor_set_opacity (worm->actors, 0);
  clutter_actor_set_scale (worm->actors, 3.0, 3.0);
  clutter_actor_animate (worm->actors, CLUTTER_EASE_OUT_CIRC, 910,
                         "scale-x", 1.0,
                         "scale-y", 1.0,
                         "fixed::scale-gravity", CLUTTER_GRAVITY_CENTER,
                         "opacity", 0xff,
                         NULL);
  worm->stop = FALSE;
}

void
gnibbles_worm_reset (GnibblesWorm *worm)
{
  gint i,j;

  worm->stop = TRUE;
  gnibbles_worm_animate_death (worm);

  for (i = 0; i < BOARDHEIGHT; i++)
    for (j = 0; j < BOARDWIDTH; j++)
      if (board->walls[j][i] == WORMCHAR + worm->number)
        board->walls[j][i] = EMPTYCHAR;

  worm->xhead = worm->xstart;
  worm->yhead = worm->ystart;
  worm->xtail = worm->xhead;
  worm->ytail = worm->yhead;
  worm->direction = worm->direction_start;
  worm->length = 1;
  worm->change = SLENGTH - 1;

  switch (worm->direction) {
    case WORMRIGHT:
      worm->xtail++;
      break;
    case WORMLEFT:
      worm->xtail--;
      break;
    case WORMDOWN:
      worm->ytail++;
      break;
    case WORMUP:
      worm->ytail--;
      break;
    default:
      break;
  }

  gnibbles_worm_queue_empty (worm);
  clutter_actor_set_opacity (CLUTTER_ACTOR (worm->actors), 0xFF);
  board->walls[worm->xtail][worm->ytail] = EMPTYCHAR;

  worm->stop = FALSE;
}

void
gnibbles_worm_destroy (GnibblesWorm *worm)
{
  while (worm->list)
    gnibbles_worm_remove_actor (worm);

  clutter_group_remove_all (CLUTTER_GROUP (worm->actors));

  g_free (worm);
}

void
gnibbles_worm_rescale (GnibblesWorm *worm, gint tilesize)
{
  int i;
  gfloat x_pos, y_pos;
  gint count;
  ClutterActor *tmp;
  GError *err = NULL;

  if (!worm)
    return;
  if (!worm->actors)
    return;

  count = clutter_group_get_n_children (CLUTTER_GROUP (worm->actors));

  for (i = 0; i < count; i++) {
    tmp = clutter_group_get_nth_child (CLUTTER_GROUP (worm->actors), i);
    clutter_actor_get_position (tmp, &x_pos, &y_pos);

    clutter_actor_set_position (tmp,
                                (x_pos / properties->tilesize) * tilesize,
                                (y_pos / properties->tilesize) * tilesize);

    gtk_clutter_texture_set_from_pixbuf (
       GTK_CLUTTER_TEXTURE (tmp),
       worm_pixmaps[properties->wormprops[worm->number]->color - 12],
       &err);
    if (err)
      gnibbles_error (err->message);
  }

}

void
gnibbles_worm_move_head (GnibblesWorm *worm)
{
  if (worm->human)
    worm->keypress = 0;

  gnibbles_worm_move_head_pointer (worm);

  if (key_queue[worm->number] && !g_queue_is_empty (key_queue[worm->number])) {
    gnibbles_worm_dequeue_keypress (worm);
  }
}

void
gnibbles_worm_move_tail (GnibblesWorm *worm)
{
  if (g_list_length (worm->list) <= 1)
    return;

  if (worm->change <= 0) {
    gnibbles_worm_move_tail_pointer (worm);
  } else {
    worm->change--;
    worm->length++;
  }
}

void
gnibbles_worm_reduce_tail (GnibblesWorm *worm, gint erasesize)
{
  gint i;
  gfloat x,y;
  ClutterActor *tmp = NULL;
  ClutterActor *group = clutter_group_new ();
  GError *error = NULL;

  if (erasesize) {
    if (g_list_length (worm->list) <= erasesize) {
      gnibbles_worm_reset (worm);
      return;
    }

    for (i = 0; i < erasesize; i++) {
      tmp = gtk_clutter_texture_new ();
      gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                           worm_pixmaps[properties->wormprops[worm->number]->color - 12],
                                           &error);
      clutter_actor_get_position
        (CLUTTER_ACTOR (g_list_last (worm->list)->data), &x, &y);
      clutter_actor_set_position (CLUTTER_ACTOR (tmp), x, y);
      clutter_actor_set_size (CLUTTER_ACTOR (tmp),
                              properties->tilesize,
                              properties->tilesize);
      clutter_container_add_actor (CLUTTER_CONTAINER (group), tmp);

      gnibbles_worm_move_tail_pointer (worm);
    }
    worm->length -= erasesize;
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), group);

    clutter_actor_animate (group, CLUTTER_EASE_OUT_EXPO, 850,
                           "opacity", 0,
                           NULL);
  }
}

gboolean
gnibbles_worm_lose_life (GnibblesWorm * worm)
{
  worm->lives--;
  if (worm->lives < 0)
    return TRUE;

  return FALSE;
}

gboolean
gnibbles_worm_can_move_to (GnibblesWorm * worm, gint x, gint y)
{
  if (worm->xhead == x)
    return worm->yhead - 1 == y || worm->yhead + 1 == y;
  if (worm->yhead == y)
    return worm->xhead - 1 == x || worm->xhead + 1 == x;
  return FALSE;
}

void
gnibbles_worm_position_move_head (GnibblesWorm * worm, gint *x, gint *y)
{
  *x = worm->xhead;
  *y = worm->yhead;

  switch (worm->direction) {
    case WORMUP:
      *y = worm->yhead - 1;
      break;
    case WORMDOWN:
      *y = worm->yhead + 1;
      break;
    case WORMLEFT:
      *x = worm->xhead - 1;
      break;
    case WORMRIGHT:
      *x = worm->xhead + 1;
      break;
  }

  if (*x == BOARDWIDTH)
    *x = 0;
  if (*x < 0)
    *x = BOARDWIDTH - 1;
  if (*y == BOARDHEIGHT)
    *y = 0;
  if (*y < 0)
    *y = BOARDHEIGHT - 1;
}

gboolean
gnibbles_worm_test_move_head (GnibblesWorm * worm)
{
  int x, y;

  gnibbles_worm_position_move_head(worm, &x, &y);

  if (board->walls[x][y] > EMPTYCHAR
      && board->walls[x][y] < 'z' + properties->numworms)
    return FALSE;

  return TRUE;
}

gboolean
gnibbles_worm_is_move_safe (GnibblesWorm * worm)
{
  int x, y, i;

  gnibbles_worm_position_move_head(worm, &x, &y);

  for (i = 0; i < properties->numworms; i++) {
    if (i != worm->number) {
      if (gnibbles_worm_can_move_to (worms[i], x, y))
        return (FALSE);
    }
  }

  return TRUE;
}

/* Check whether the worm will be trapped in a dead end. A location
   within the dead end and the length of the worm is given. This
   prevents worms getting trapped in a spiral, or in a corner sharper
   than 90 degrees.  runnumber is a unique number used to update the
   deadend board. The principle of the deadend board is that it marks
   all squares previously checked, so the exact size of the deadend
   can be calculated in O(n) time; to prevent the need to clear it
   afterwards, a different number is stored in the board each time
   (the number will not have been previously used, so the board will
   appear empty). Although in theory deadend_runnumber may wrap round,
   after 4 billion steps the entire board is likely to have been
   overwritten anyway. */
static guint deadendboard[BOARDWIDTH][BOARDHEIGHT] = {{0}};
static guint deadend_runnumber = 0;

static gint
gnibbles_worm_ai_deadend (gint x, gint y, gint lengthleft)
{
  gint cdir, cx, cy;

  if (x >= BOARDWIDTH)
    x = 0;
  if (x < 0)
    x = BOARDWIDTH - 1;
  if (y >= BOARDHEIGHT)
    y = 0;
  if (y < 0)
    y = BOARDHEIGHT - 1;

  if (! lengthleft)
    return 0;

  cdir = 5;
  while (--cdir) {
    cx = x;
    cy = y;
    switch (cdir) {
      case WORMUP:
        cy -= 1;
        break;
      case WORMDOWN:
        cy += 1;
        break;
      case WORMLEFT:
        cx -= 1;
        break;
      case WORMRIGHT:
        cx += 1;
        break;
    }

    if (cx >= BOARDWIDTH)
      cx = 0;
    if (cx < 0)
      cx = BOARDWIDTH - 1;
    if (cy >= BOARDHEIGHT)
      cy = 0;
    if (cy < 0)
      cy = BOARDHEIGHT - 1;

    if ((board->walls[cx][cy] <= EMPTYCHAR
        || board->walls[x][y] >= 'z' + properties->numworms)
        && deadendboard[cx][cy] != deadend_runnumber) {

      deadendboard[cx][cy] = deadend_runnumber;
      lengthleft = gnibbles_worm_ai_deadend(cx, cy, lengthleft - 1);
      if (!lengthleft)
        return 0;
    }
  }
  return lengthleft;
}

/* Check a deadend starting from the next square in this direction,
   rather than from this square. Also block off the squares near worm
   heads, so that humans can't kill AI players by trapping them
   against a wall.  The given length is quartered and squared; this
   allows for the situation where the worm has gone round in a square
   and is about to get trapped in a spiral. However, it's set to at
   least BOARDWIDTH, so that on the levels with long thin paths a worm
   won't start down the path if it'll crash at the other end. */
static gint
gnibbles_worm_ai_deadend_after (gint x, gint y, gint dir, gint length)
{
  gint cx, cy, cl, i;

  if (x < 0 || x >= BOARDWIDTH || y < 0 || y >= BOARDHEIGHT) {
    return 0;
  }

  ++deadend_runnumber;

  if (dir > 4)
    dir = 1;
  if (dir < 1)
    dir = 4;

  i = properties->numworms;
  while(i--) {
    cx = worms[i]->xhead;
    cy = worms[i]->yhead;
    if(cx != x || cy != y) {
      if(cx > 0)
        deadendboard[cx-1][cy] = deadend_runnumber;
      if(cy > 0)
        deadendboard[cx][cy-1] = deadend_runnumber;
      if(cx < BOARDWIDTH-1)
        deadendboard[cx+1][cy] = deadend_runnumber;
      if(cy < BOARDHEIGHT-1)
        deadendboard[cx][cy+1] = deadend_runnumber;
    }
  }

  cx = x;
  cy = y;
  switch (dir) {
    case WORMUP:
      cy -= 1;
      break;
    case WORMDOWN:
      cy += 1;
      break;
    case WORMLEFT:
      cx -= 1;
      break;
    case WORMRIGHT:
      cx += 1;
      break;
  }

  if (cx >= BOARDWIDTH)
    cx = 0;
  if (cx < 0)
    cx = BOARDWIDTH - 1;
  if (cy >= BOARDHEIGHT)
    cy = 0;
  if (cy < 0)
    cy = BOARDHEIGHT - 1;

  deadendboard[x][y] = deadend_runnumber;
  deadendboard[cx][cy] = deadend_runnumber;

  cl = (length * length) / 16;
  if (cl < BOARDWIDTH)
    cl = BOARDWIDTH;
  return gnibbles_worm_ai_deadend (cx, cy, cl);
}

/* Check to see if another worm's head is too close in front of us;
   that is, that it's within 3 in the direction we're going and within
   1 to the side. */
static gint
gnibbles_worm_ai_tooclose (GnibblesWorm * worm)
{
  gint i = properties->numworms;
  gint dx, dy;

  while (i--) {
    dx = worm->xhead - worms[i]->xhead;
    dy = worm->yhead - worms[i]->yhead;
    switch (worm->direction) {
      case WORMUP:
        if (dy > 0 && dy <= 3 && dx >= -1 && dx <= 1)
          return 1;
        break;
      case WORMDOWN:
        if (dy < 0 && dy >= -3 && dx >= -1 && dx <= 1)
          return 1;
        break;
      case WORMLEFT:
        if (dx > 0 && dx <= 3 && dy >= -1 && dy <= 1)
          return 1;
        break;
      case WORMRIGHT:
        if (dx < 0 && dx >= -3 && dy >= -1 && dy <= 1)
          return 1;
        break;
    }
  }

  return 0;
}

static gint
gnibbles_worm_ai_wander (gint x, gint y, gint dir, gint ox, gint oy)
{
  if (dir > 4)
    dir = 1;
  if (dir < 1)
    dir = 4;

  switch (dir) {
    case WORMUP:
      y -= 1;
      break;
    case WORMDOWN:
      y += 1;
      break;
    case WORMLEFT:
      x -= 1;
      break;
    case WORMRIGHT:
      x += 1;
      break;
  }

  if (x >= BOARDWIDTH)
    x = 0;
  if (x < 0)
    x = BOARDWIDTH - 1;
  if (y >= BOARDHEIGHT)
    y = 0;
  if (y < 0)
    y = BOARDHEIGHT - 1;

  switch (board->walls[x][y] - 'A') {
    case BONUSREGULAR:
    case BONUSDOUBLE:
    case BONUSLIFE:
    case BONUSREVERSE:
      return 1;
      break;
    case BONUSHALF:
      return 0;
      break;
    default:
      if (board->walls[x][y] > EMPTYCHAR
          && board->walls[x][y] < 'z' + properties->numworms) {
        return 0;
      } else {
        if (ox == x && oy == y)
          return 0;
        return gnibbles_worm_ai_wander (x, y, dir, ox, oy);
      }
    break;
  }
}

/* Determines the direction of the AI worm. */
void
gnibbles_worm_ai_move (GnibblesWorm * worm)
{
  int opposite, dir, left, right, front;
  gint bestyet, bestdir, thislen, olddir;

  opposite = (worm->direction + 1) % 4 + 1;

  front = gnibbles_worm_ai_wander (worm->xhead, worm->yhead,
                                   worm->direction, worm->xhead, worm->yhead);
  left = gnibbles_worm_ai_wander (worm->xhead, worm->yhead,
                                  worm->direction - 1,
                                  worm->xhead, worm->yhead);
  right = gnibbles_worm_ai_wander (worm->xhead, worm->yhead,
                                   worm->direction + 1,
                                   worm->xhead, worm->yhead);

  if (!front) {
    if (left) {
      // Found a bonus to the left
      dir = worm->direction - 1;
      if (dir < 1)
        dir = 4;
      worm->direction = dir;
    } else if (right) {
      // Found a bonus to the right
      dir = worm->direction + 1;
      if (dir > 4)
        dir = 1;
      worm->direction = dir;
    } else {
      // Else move in random direction at random time intervals
      if (rand () % 30 == 1) {
        dir = worm->direction + (rand() % 2 ? 1 : -1);
        if (dir != opposite) {
          if (dir > 4)
            dir = 1;
          if (dir < 1)
            dir = 4;
          worm->direction = dir;

        }
      }
    }
  }

  /* Avoid walls, dead-ends and other worm's heads. This is done using
     an evalution function which is CAPACITY for a wall, 4 if another
     worm's head is in the tooclose area, 4 if another worm's head
     could move to the same location as ours, plus 0 if there's no
     dead-end, or the amount that doesn't fit for a deadend. olddir's
     score is reduced by 100, to favour it, but only if its score is 0
     otherwise; this is so that if we're currently trapped in a dead
     end, the worm will move in a space-filling manner in the hope
     that the dead end will disappear (e.g. if it's made from the tail
     of some worm, as often happens). */
  olddir = worm->direction;
  bestyet = CAPACITY * 2;
  bestdir = -1;

  for (dir = 1; dir <= 4; dir++) {
    worm->direction = dir;

    if (dir == opposite)
      continue;
    thislen = 0;

    if(!gnibbles_worm_test_move_head (worm))
      thislen += CAPACITY;

    if(gnibbles_worm_ai_tooclose (worm))
      thislen += 4;

    if(!gnibbles_worm_is_move_safe (worm))
      thislen += 4;

    thislen += gnibbles_worm_ai_deadend_after (worm->xhead, worm->yhead, dir,
                                               worm->length + worm->change);

    if (dir == olddir && !thislen)
      thislen -= 100;
    /* If the favoured direction isn't appropriate, then choose
       another direction at random rather than favouring one in
       particular, to stop the worms bunching in the bottom-
       right corner of the board. */
    if (thislen <= 0)
      thislen -= random() % 100;
    if (thislen < bestyet) {
      bestyet = thislen;
      bestdir = dir;
    }
  }

  if (bestdir == -1) /* this should never happen, but just in case... */
    bestdir = olddir;

  worm->direction = bestdir;

  /* Make sure we are at least avoiding walls.
   * Mostly other snakes should avoid our head. */
  for (dir = 1; dir <= 4; dir++) {
    if (dir == opposite)
      continue;
    if (!gnibbles_worm_test_move_head (worm))
      worm->direction = dir;
    else
      continue;
  }
}
