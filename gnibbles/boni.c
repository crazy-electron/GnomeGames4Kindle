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

#include <libgames-support/games-sound.h>
#include <clutter-gtk/clutter-gtk.h>

#include "gnibbles.h"
#include "main.h"
#include "bonus.h"
#include "boni.h"
#include "board.h"
#include "properties.h"

extern GnibblesBoard *board;
extern GnibblesProperties *properties;
extern GdkPixbuf *boni_pixmaps[];

GnibblesBoni *
gnibbles_boni_new (void)
{
  int i;
  GnibblesBoni *tmp;

  tmp = g_new (GnibblesBoni, 1);
  for (i = 0; i < MAXBONUSES; i++)
    tmp->bonuses[i] = NULL;
  tmp->numbonuses = 0;
  tmp->numleft = NUMBONI;
  tmp->missed = 0;

  return tmp;
}

void
gnibbles_boni_destroy (GnibblesBoni * boni)
{
  int i;

  for (i = 0; i < boni->numbonuses; i++) {
    clutter_actor_hide (boni->bonuses[i]->actor);
    free (boni->bonuses[i]);
  }
  boni->numbonuses = 0;
  free (boni);
}

void
gnibbles_boni_add_bonus (GnibblesBoni * boni, gint t_x, gint t_y,
                         gint t_type, gint t_fake, gint t_countdown)
{
  if (boni->numbonuses == MAXBONUSES)
    return;
  boni->bonuses[boni->numbonuses] = gnibbles_bonus_new (t_x, t_y,
                                                        t_type, t_fake,
                                                        t_countdown);
  board->walls[t_x][t_y] = (gchar) t_type + 'A';
  board->walls[t_x + 1][t_y] = (gchar) t_type + 'A';
  board->walls[t_x][t_y + 1] = (gchar) t_type + 'A';
  board->walls[t_x + 1][t_y + 1] = (gchar) t_type + 'A';

  gnibbles_bonus_draw (boni->bonuses[boni->numbonuses]);

  boni->numbonuses++;
  if (t_type != BONUSREGULAR)
    games_sound_play ("appear");
#ifdef GGZ_CLIENT
  if (ggz_network_mode) {
    network_add_bonus (t_x, t_y, t_type, t_fake, t_countdown);
  }
#endif
}

void
gnibbles_boni_add_bonus_final (GnibblesBoni * boni, gint t_x, gint t_y,
                               gint t_type, gint t_fake, gint t_countdown)
{
  if (boni->numbonuses == MAXBONUSES)
    return;
  boni->bonuses[boni->numbonuses] = gnibbles_bonus_new (t_x, t_y,
                                                        t_type, t_fake,
                                                        t_countdown);
  board->walls[t_x][t_y] = (gchar) t_type + 'A';
  board->walls[t_x + 1][t_y] = (gchar) t_type + 'A';
  board->walls[t_x][t_y + 1] = (gchar) t_type + 'A';
  board->walls[t_x + 1][t_y + 1] = (gchar) t_type + 'A';

  gnibbles_bonus_draw (boni->bonuses[boni->numbonuses]);
  boni->numbonuses++;
  if (t_type != BONUSREGULAR)
    games_sound_play ("appear");
}

int
gnibbles_boni_fake (GnibblesBoni * boni, gint x, gint y)
{
  int i;

  for (i = 0; i < boni->numbonuses; i++) {
    if ((x == boni->bonuses[i]->x &&
        y == boni->bonuses[i]->y) ||
        (x == boni->bonuses[i]->x + 1 &&
        y == boni->bonuses[i]->y) ||
        (x == boni->bonuses[i]->x &&
        y == boni->bonuses[i]->y + 1) ||
        (x == boni->bonuses[i]->x + 1 && y == boni->bonuses[i]->y + 1)) {
      return (boni->bonuses[i]->fake);
    }
  }

  return 0;
}

void
gnibbles_boni_remove_bonus (GnibblesBoni * boni, gint x, gint y)
{
  int i;

#ifdef GGZ_CLIENT
  if (ggz_network_mode) {
    network_remove_bonus (x, y);
  }
#endif
  for (i = 0; i < boni->numbonuses; i++) {
    if ((x == boni->bonuses[i]->x &&
        y == boni->bonuses[i]->y) ||
        (x == boni->bonuses[i]->x + 1 &&
        y == boni->bonuses[i]->y) ||
        (x == boni->bonuses[i]->x &&
        y == boni->bonuses[i]->y + 1) ||
        (x == boni->bonuses[i]->x + 1 && y == boni->bonuses[i]->y + 1)) {

      board->walls[boni->bonuses[i]->x][boni->bonuses[i]->y] = EMPTYCHAR;
      board->walls[boni->bonuses[i]->x + 1][boni->bonuses[i]->y] = EMPTYCHAR;
      board->walls[boni->bonuses[i]->x][boni->bonuses[i]->y + 1] = EMPTYCHAR;
      board->walls[boni->bonuses[i]->x + 1][boni->bonuses[i]->y + 1] = EMPTYCHAR;

      gnibbles_bonus_erase (boni->bonuses[i]);
      boni->bonuses[i] = boni->bonuses[--boni->numbonuses];
      return;
    }
  }
}

void
gnibbles_boni_remove_bonus_final (GnibblesBoni * boni, gint x, gint y)
{
  int i;

  for (i = 0; i < boni->numbonuses; i++) {
    if ((x == boni->bonuses[i]->x &&
        y == boni->bonuses[i]->y) ||
        (x == boni->bonuses[i]->x + 1 &&
        y == boni->bonuses[i]->y) ||
        (x == boni->bonuses[i]->x &&
        y == boni->bonuses[i]->y + 1) ||
        (x == boni->bonuses[i]->x + 1 && y == boni->bonuses[i]->y + 1)) {

      board->walls[boni->bonuses[i]->x][boni->bonuses[i]->y] = EMPTYCHAR;
      board->walls[boni->bonuses[i]->x + 1][boni->bonuses[i]->y] = EMPTYCHAR;
      board->walls[boni->bonuses[i]->x][boni->bonuses[i]->y + 1] = EMPTYCHAR;
      board->walls[boni->bonuses[i]->x + 1][boni->bonuses[i]->y + 1] = EMPTYCHAR;

      gnibbles_bonus_erase (boni->bonuses[i]);
      boni->bonuses[i] = boni->bonuses[--boni->numbonuses];
      return;
    }
  }
}

void
gnibbles_boni_rescale (GnibblesBoni *boni, gint tilesize)
{
  int i;
  gfloat x_pos, y_pos;
  GError *err = NULL;

  for (i = 0; i < boni->numbonuses; i++) {
    clutter_actor_get_position (boni->bonuses[i]->actor, &x_pos, &y_pos);
    clutter_actor_set_position (boni->bonuses[i]->actor,
                                (x_pos / properties->tilesize) * tilesize,
                                (y_pos / properties->tilesize) * tilesize);
    gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE(boni->bonuses[i]->actor),
                                         boni_pixmaps[boni->bonuses[i]->type],
                                         &err);
    if (err)
      gnibbles_error (err->message);
  }
}
