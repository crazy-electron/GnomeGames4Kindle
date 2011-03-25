/*
 * othello.h - Header for othello.c
 * written by Ian Peters <itp@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * For more details see the file COPYING.
 */

#ifndef _OTHELLO_H_
#define _OTHELLO_H_

gint is_valid_move (guint, guint, guint);
gint move (guint, guint, guint);
void move_board (gint);
gboolean flip_final_results (gpointer data);
gint check_valid_moves (void);
gint computer_move_1 (guint);
gint computer_move_2 (guint);
gint computer_move_3 (guint);
gint computer_move_4 (guint);
void init (void);
void board_copy (void);
void undo (void);

#endif
