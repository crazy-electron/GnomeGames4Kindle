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

#ifndef _WARPMANAGER_H_
#define _WARPMANAGER_H_

#include <config.h>

#include <gtk/gtk.h>

#include "warp.h"
#include "worm.h"

#define MAXWARPS 200
#define WARPLETTER 'W'

typedef struct {
  GnibblesWarp *warps[MAXWARPS];
  gint numwarps;
} GnibblesWarpManager;

GnibblesWarpManager *gnibbles_warpmanager_new (void);
void gnibbles_warpmanager_destroy (GnibblesWarpManager *warpmanager);
void gnibbles_warpmanager_add_warp (GnibblesWarpManager *warpmanager,
				                            gint t_x, gint t_y, gint t_wx, gint t_wy);
void gnibbles_warpmanager_worm_change_pos (GnibblesWarpManager *warpmanager,
					                                 GnibblesWorm *worm);
void gnibbles_warpmanager_worm_change_tail_pos (GnibblesWarpManager *warpmanager,
                                                GnibblesWorm *worm);
void gnibbles_warpmanager_rescale (GnibblesWarpManager *warpmanager,
                                  gint tilesize);

#endif
