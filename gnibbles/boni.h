/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>
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

#ifndef _BONI_H_
#define _BONI_H_

#include <config.h>

#include <gtk/gtk.h>

#include "bonus.h"

#define MAXBONUSES 100
#define MAXMISSED 2

#define NUMBONI 10

typedef struct {
  GnibblesBonus *bonuses[MAXBONUSES];
  gint numbonuses;
  gint numleft;
  gint missed;
} GnibblesBoni;

GnibblesBoni *gnibbles_boni_new (void);

void gnibbles_boni_destroy (GnibblesBoni * boni);

void gnibbles_boni_add_bonus (GnibblesBoni * boni, gint t_x, gint t_y,
                              gint t_type, gint t_fake, gint t_countdown);
void gnibbles_boni_remove_bonus (GnibblesBoni * boni, gint x, gint y);
void gnibbles_boni_add_bonus_final (GnibblesBoni * boni, gint t_x, gint t_y,
                                    gint t_type, gint t_fake, gint t_countdown);
void gnibbles_boni_remove_bonus_final (GnibblesBoni * boni, gint x, gint y);

int gnibbles_boni_fake (GnibblesBoni * boni, gint x, gint y);

void gnibbles_boni_rescale (GnibblesBoni *boni, gint tilesize);

#endif
