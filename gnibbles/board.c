/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

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
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>

#include <libgames-support/games-runtime.h>
#include <clutter-gtk/clutter-gtk.h>

#include "main.h"
#include "gnibbles.h"
#include "properties.h"
#include "board.h"
#include "worm.h"
#include "boni.h"

#ifdef GGZ_CLIENT
#include "ggz-network.h"
#endif

extern GnibblesWorm *worms[];
extern GnibblesProperties *properties;
extern GnibblesWarpManager *warpmanager;
extern GnibblesBoni *boni;
extern GdkPixbuf *wall_pixmaps[];
extern ClutterActor *stage;

GnibblesBoard *
gnibbles_board_new (void)
{
  gchar *filename;
  const char *dirname;
  GValue val = {0,};

  GnibblesBoard *board = g_new (GnibblesBoard, 1);
  board->width = BOARDWIDTH;
  board->height = BOARDHEIGHT;
  board->level = NULL;
  board->surface = NULL;

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  filename = g_build_filename (dirname, "wall-small-empty.svg", NULL);

  board->surface = clutter_texture_new_from_file (filename, NULL);

  clutter_actor_set_opacity (CLUTTER_ACTOR (board->surface), 100);
  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean ( &val, TRUE);

  g_object_set_property (G_OBJECT (board->surface), "repeat-y", &val);
  g_object_set_property (G_OBJECT (board->surface), "repeat-x", &val);

  clutter_actor_set_position (CLUTTER_ACTOR (board->surface), 0, 0);
  clutter_actor_set_size (CLUTTER_ACTOR (board->surface),
                          properties->tilesize * BOARDWIDTH,
                          properties->tilesize * BOARDHEIGHT);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               CLUTTER_ACTOR (board->surface));
  clutter_actor_show (CLUTTER_ACTOR (board->surface));

  return board;
}

static void
gnibbles_board_load_level (GnibblesBoard *board)
{
  gint i,j;
  gint x_pos, y_pos;
  ClutterActor *tmp;
  gboolean is_wall = TRUE;
  GError *error = NULL;

  if (board->level) {
    clutter_group_remove_all (CLUTTER_GROUP (board->level));
    clutter_container_remove_actor (CLUTTER_CONTAINER (stage), board->level);
  }

  board->level = clutter_group_new ();

  /* Load wall_pixmaps onto the surface*/
  for (i = 0; i < BOARDHEIGHT; i++) {
    y_pos = i * properties->tilesize;
    for (j = 0; j < BOARDWIDTH; j++) {
      is_wall = TRUE;
      switch (board->walls[j][i]) {
        case 'a': // empty space
          is_wall = FALSE;
          break; // break right away
        case 'b': // straight up
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[0], &error);
          break;
        case 'c': // straight side
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[1], &error);
          break;
        case 'd': // corner bottom left
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[2], &error);
          break;
        case 'e': // corner bottom right
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[3], &error);
          break;
          case 'f': // corner up left
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[4], &error);
          break;
        case 'g': // corner up right
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[5], &error);
          break;
        case 'h': // tee up
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[6], &error);
          break;
        case 'i': // tee right
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[7], &error);
          break;
        case 'j': // tee left
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[8], &error);
          break;
        case 'k': // tee down
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[9], &error);
          break;
        case 'l': // cross
          tmp = gtk_clutter_texture_new ();
          gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (tmp),
                                               wall_pixmaps[10], &error);
          break;
        default:
          is_wall = FALSE;
          break;
      }

      if (is_wall) {
        x_pos = j * properties->tilesize;

        clutter_actor_set_size (CLUTTER_ACTOR(tmp),
                                properties->tilesize,
                                properties->tilesize);

        clutter_actor_set_position (CLUTTER_ACTOR (tmp), x_pos, y_pos);
        clutter_actor_show (CLUTTER_ACTOR (tmp));
        clutter_container_add_actor (CLUTTER_CONTAINER (board->level),
                                     CLUTTER_ACTOR (tmp));
      }
    }
  }

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), board->level);
  clutter_actor_raise (board->level, board->surface);

  clutter_actor_set_opacity (board->level, 0);
  clutter_actor_set_scale (CLUTTER_ACTOR (board->level), 0.2, 0.2);
  clutter_actor_animate (board->level, CLUTTER_EASE_OUT_BOUNCE, 1210,
                         "opacity", 0xff,
                         "fixed::scale-gravity", CLUTTER_GRAVITY_CENTER,
                         "scale-x", 1.0,
                         "scale-y", 1.0,
                         NULL);
}

void
gnibbles_board_rescale (GnibblesBoard *board, gint tilesize)
{
  gint i, count;
  gfloat x_pos, y_pos;
  ClutterActor *tmp;

  if (!board->level)
    return;
  if (!board->surface)
    return;

  board->width = BOARDWIDTH * tilesize;
  board->height = BOARDHEIGHT * tilesize;

  clutter_actor_set_size (CLUTTER_ACTOR (board->surface),
                          board->width,
                          board->height);

  count = clutter_group_get_n_children (CLUTTER_GROUP (board->level));

  for (i = 0; i < count; i++) {
    tmp = clutter_group_get_nth_child (CLUTTER_GROUP (board->level), i);
    clutter_actor_get_position (CLUTTER_ACTOR (tmp), &x_pos, &y_pos);
    clutter_actor_set_position (CLUTTER_ACTOR (tmp),
                                (x_pos / properties->tilesize) * tilesize,
                                (y_pos / properties->tilesize) * tilesize);
    clutter_actor_set_size (CLUTTER_ACTOR (tmp), tilesize, tilesize);
  }
}

void
gnibbles_board_level_new (GnibblesBoard *board, gint level)
{

  gchar *tmp = NULL;
  const char *dirname;
  gchar *filename;
  FILE *in;
  gchar tmpboard [BOARDWIDTH +2];
  gint i,j;
  gint count = 0;

  board->current_level = level;

  tmp = g_strdup_printf("level%03d.gnl", level);

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_GAMES_DIRECTORY);
  filename = g_build_filename (dirname, tmp, NULL);

  g_free (tmp);

  if ((in = fopen (filename, "r")) == NULL) {
    char *message =
      g_strdup_printf (_("Nibbles couldn't load level file:\n%s\n\n"
                        "Please check your Nibbles installation"), filename);
    gnibbles_error (message);
    g_free (message);
  }

  if (warpmanager)
    gnibbles_warpmanager_destroy (warpmanager);

  warpmanager = gnibbles_warpmanager_new ();

  if (boni)
    gnibbles_boni_destroy (boni);

  boni = gnibbles_boni_new ();

  for (i = 0; i < BOARDHEIGHT; i++) {
    if (!fgets (tmpboard, sizeof (tmpboard), in)) {
      char *message =
        g_strdup_printf (_("Level file appears to be damaged:\n%s\n\n"
                         "Please check your Nibbles installation"), filename);
      gnibbles_error (message);
      g_free (message);
      break;
    }

    for (j = 0; j < BOARDWIDTH; j++) {
      board->walls[j][i] = tmpboard[j];
      switch (board->walls[j][i]) {
        case 'm':
          board->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms)
            gnibbles_worm_set_start (worms[count++], j, i, WORMUP);
          break;
        case 'n':
          board->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms)
            gnibbles_worm_set_start(worms[count++], j, i, WORMLEFT);
          break;
        case 'o':
          board->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms)
            gnibbles_worm_set_start (worms[count++], j, i, WORMDOWN);
          break;
        case 'p':
          board->walls[j][i] = EMPTYCHAR;
          if (count < properties->numworms)
            gnibbles_worm_set_start (worms[count++], j, i, WORMRIGHT);
          break;
        case 'Q':
          gnibbles_warpmanager_add_warp (warpmanager, j - 1, i - 1, -1, -1);
          break;
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
          gnibbles_warpmanager_add_warp
            (warpmanager, j - 1, i - 1, -(board->walls[j][i]), 0);
          break;
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
          gnibbles_warpmanager_add_warp
            (warpmanager, -(board->walls[j][i] - 'a' + 'A'), 0, j, i);
          board->walls[j][i] = EMPTYCHAR;
          break;
       }
    }
  }

  g_free (filename);
  fclose (in);

  for (i = 0; i < count; i++) {
    if (worms[i]->direction == WORMRIGHT) {
      for (j = 0; j < worms[i]->length; j++)
        gnibbles_worm_move_head_pointer (worms[i]);
      worms[i]->xtail++;
    } else if ( worms[i]->direction == WORMLEFT) {
      for (j = 0; j < worms[i]->length; j++)
        gnibbles_worm_move_head_pointer (worms[i]);
      worms[i]->xtail--;
    } else if (worms[i]->direction == WORMDOWN) {
      for (j = 0; j < worms[i]->length; j++)
        gnibbles_worm_move_head_pointer (worms[i]);
      worms[i]->ytail++;
    } else if (worms[i]->direction == WORMUP) {
      for (j = 0; j < worms[i]->length; j++)
        gnibbles_worm_move_head_pointer (worms[i]);
      worms[i]->ytail--;
    }
    board->walls[worms[i]->xtail][worms[i]->ytail] = EMPTYCHAR;
  }
  gnibbles_board_load_level (board);
}

void
gnibbles_board_level_add_bonus (GnibblesBoard *board, gint regular)
{
  gint x, y, good;

#ifdef GGZ_CLIENT
  if (!network_is_host ()) {
    return;
  }
#endif

  if (regular) {
    good = 0;
  } else {
    good = rand () % 50;
    if (good)
      return;
  }

  do {
    good = 1;
    x = rand () % (BOARDWIDTH - 1);
    y = rand () % (BOARDHEIGHT - 1);
    if (board->walls[x][y] != EMPTYCHAR)
      good = 0;
    if (board->walls[x + 1][y] != EMPTYCHAR)
      good = 0;
    if (board->walls[x][y + 1] != EMPTYCHAR)
      good = 0;
    if (board->walls[x + 1][y + 1] != EMPTYCHAR)
      good = 0;
  } while (!good);

  if (regular) {
    if ((rand () % 7 == 0) && properties->fakes)
      gnibbles_boni_add_bonus (boni, x, y, BONUSREGULAR, 1, 300);
    good = 0;
    while (!good) {
      good = 1;
      x = rand () % (BOARDWIDTH - 1);
      y = rand () % (BOARDHEIGHT - 1);
      if (board->walls[x][y] != EMPTYCHAR)
        good = 0;
      if (board->walls[x + 1][y] != EMPTYCHAR)
        good = 0;
      if (board->walls[x][y + 1] != EMPTYCHAR)
        good = 0;
      if (board->walls[x + 1][y + 1] != EMPTYCHAR)
        good = 0;
    }
    gnibbles_boni_add_bonus (boni, x, y, BONUSREGULAR, 0, 300);
  } else if (boni->missed <= MAXMISSED) {
    good = rand () % 7;

    if (good)
      good = 0;
    else
      good = 1;

    if (good && !properties->fakes)
      return;

    switch (rand () % 21) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      gnibbles_boni_add_bonus (boni, x, y, BONUSHALF, good, 200);
      break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      gnibbles_boni_add_bonus (boni, x, y, BONUSDOUBLE, good, 150);
      break;
    case 15:
      gnibbles_boni_add_bonus (boni, x, y, BONUSLIFE, good, 100);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      if (properties->numworms > 1)
        gnibbles_boni_add_bonus (boni, x, y, BONUSREVERSE, good, 150);
      break;
    }
  }
}
