/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>,
 *                 Guillaume Beland <guillaume.beland@gmail.com>
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

#ifndef BOARD_H_
#define BOARD_H_

#include <config.h>

#include <gtk/gtk.h>
#include <clutter/clutter.h>

#define BOARDWIDTH 92
#define BOARDHEIGHT 66

#define EMPTYCHAR 'a'
#define WORMCHAR 'w'

typedef struct {
  gint width;
  gint height;
  ClutterActor *surface;
  ClutterActor *level;

  gchar walls[BOARDWIDTH][BOARDHEIGHT];
  gint current_level;
} GnibblesBoard;

GnibblesBoard* gnibbles_board_new (void);
void gnibbles_board_rescale (GnibblesBoard *board, gint tilesize);
void gnibbles_board_level_new (GnibblesBoard *board, gint level);
void gnibbles_board_level_add_bonus (GnibblesBoard *board, gint regular);

#endif
