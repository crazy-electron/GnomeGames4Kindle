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

#ifndef _BONUS_H_
#define _BONUS_H_

#include <config.h>

#include <gtk/gtk.h>
#include <clutter/clutter.h>

#define BONUSNONE	0
#define BONUSREGULAR	1
#define BONUSHALF	2
#define BONUSDOUBLE	3
#define BONUSLIFE	4
#define BONUSREVERSE	5
#define BONUSCUT	6
#define BONUSSWITCH	7

typedef struct {
  gint x, y;
  guint type;
  gint fake;
  gint countdown;
  ClutterActor *actor;
} GnibblesBonus;

GnibblesBonus *gnibbles_bonus_new (gint t_x, gint t_y, gint t_type,
				   gint t_fake, gint t_countdown);

void gnibbles_bonus_draw (GnibblesBonus * bonus);

void gnibbles_bonus_erase (GnibblesBonus * bonus);

#endif
