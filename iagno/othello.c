/* -*- mode:C; indent-tabs-mode:t; tab-width:4; c-basic-offset:8; -*- */

/*
 * othello.c - Othello support routines for iagno
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

#include <config.h>

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libgames-support/games-clock.h>
#include <libgames-support/games-sound.h>

#include "othello.h"
#include "gnothello.h"

#define PERFECT   17
#define VICTORY   19
#define BEST      56
#define MAX_DEPTH  7

#define BLANK 0
#define OUTSIDE 128
#define UL -11
#define UU -10
#define UR -9
#define LL -1
#define RR +1
#define DL +9
#define DD +10
#define DR +11
#define NDIRS 8

gint8 squares[64] = { 44, 45, 54, 55, 23, 26, 73, 76,
  32, 62, 37, 67, 13, 16, 31, 61,
  38, 68, 83, 86, 14, 15, 41, 51,
  48, 58, 84, 85, 24, 25, 42, 52,
  74, 75, 47, 57, 36, 63, 33, 35,
  53, 66, 46, 64, 34, 43, 56, 65,
  12, 17, 21, 71, 28, 78, 82, 87,
  22, 27, 72, 77, 11, 18, 81, 88
};

static const gint heuristic[100] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 65, -3, 6, 4, 4, 6, -3, 65, 0,
  0, -3, -29, 3, 1, 1, 3, -29, -3, 0,
  0, 6, 3, 5, 3, 3, 5, 3, 6, 0,
  0, 4, 1, 3, 1, 1, 3, 1, 4, 0,
  0, 4, 1, 3, 1, 1, 3, 1, 4, 0,
  0, 6, 3, 5, 3, 3, 5, 3, 6, 0,
  0, -3, -29, 3, 1, 1, 3, -29, -3, 0,
  0, 65, -3, 6, 4, 4, 6, -3, 65, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const gint dirs[] = { UL, UU, UR, LL, RR, DL, DD, DR };

guint flip_final_id = 0;
gint flip_final;

gint8 board0[100];
/* This array can probably be a lot smaller (< 600). However, I am absolutely
 * certain that it cannot be more than 1200. So 1200 is what we use. */
gint8 history[1200];
gint hpointer;
gint vsquares[280][2];
gint vpointer;
gint s_kind;
GRand *rgen;

extern guint black_computer_level;
extern guint white_computer_level;

extern gint animate;
extern gint animate_stagger;
extern guint game_in_progress;
extern gint8 pixmaps[8][8];
extern gint8 board[8][8];
extern gint8 move_count;
extern guint whose_turn;
extern gint bcount;
extern gint wcount;

/* Initialization of data */
void
init (void)
{
  gint i, j;
  gint x;

  rgen = g_rand_new ();

  x = 0;
  for (j = 0; j < 10; j++)
    for (i = 0; i < 10; i++) {
      if (j == 0 || j == 9)
	board0[x] = OUTSIDE;
      else if (i == 0 || i == 9)
	board0[x] = OUTSIDE;
      else
	board0[x] = BLANK;
      x++;
    }

  board0[44] = WHITE_TURN;
  board0[45] = BLACK_TURN;
  board0[54] = BLACK_TURN;
  board0[55] = WHITE_TURN;

  hpointer = 0;
  vpointer = 0;
}

/* Copy data from board0 to board */
void
board_copy (void)
{
  gint i, j;

  for (j = 0; j < 8; j++)
    for (i = 0; i < 8; i++)
      board[i][j] = board0[(j + 1) * 10 + i + 1];
}

/* Check the validity of the move in a given direction. */
static
  gint
is_valid0 (gint xy, gint dir, gint who)
{
  gint tmp;
  gint8 not_me;

  not_me = OTHER_PLAYER (who);

  /* To be valid, the next counter should not be the current player ... */
  tmp = xy + dir;
  if (board0[tmp] != not_me)
    return FALSE;

  /* ... but eventually we must find a counter that is. */
  do {
    tmp += dir;
  } while (board0[tmp] == not_me);

  return board0[tmp] == who;
}

/* Check if a given (empty) square can provide a valid move */
static
  gint
is_valid (gint xy, gint who)
{
  int i;

  for (i = 0; i < NDIRS; i++)
    if (is_valid0 (xy, dirs[i], who))
      return (TRUE);

  return FALSE;
}

/* Check whether the supplied move is a valid one. */
gint
is_valid_move (guint x, guint y, guint who)
{
  gint xy;

  xy = (y + 1) * 10 + x + 1;
  if (board0[xy] != BLANK)
    return FALSE;

  return is_valid (xy, who);
}

static
  void
move_board0 (gint xy, gint dir)
{
  gint8 tmp;
  guint not_me;

  not_me = OTHER_PLAYER (whose_turn);
  tmp = xy + dir;
  if (board0[tmp] != not_me)
    return;
  /* Find the extent of the move in direction dir. */
  do {
    tmp += dir;
  } while (board0[tmp] == not_me);
  if (board0[tmp] != whose_turn)
    return;

  /* Now walk back to where we started flipping pieces over. */
  tmp -= dir;
  while (tmp != xy) {
    board0[tmp] = whose_turn;	/* Flip pieces */
    history[hpointer++] = tmp;	/* Record history */
    tmp -= dir;
  }
}

/* reverse some pieces */
void
move_board (gint xy)
{
  gint tmp, count;
  int i;

  count = hpointer;

  for (i = 0; i < NDIRS; i++)
    move_board0 (xy, dirs[i]);

  board0[xy] = whose_turn;
  count = hpointer - count;
  history[hpointer++] = count;	/* make history */
  history[hpointer++] = whose_turn;

  if (whose_turn == WHITE_TURN) {
    wcount += count + 1;
    bcount -= count;
  } else {
    bcount += count + 1;
    wcount -= count;
  }
  tmp = squares[move_count];
  squares[move_count++] = xy;
  if (tmp != xy) {
    count = move_count;
    while (squares[count] != xy)
      count++;
    squares[count] = tmp;
  }
  whose_turn = OTHER_PLAYER (whose_turn);
}


gint
move (guint x, guint y, guint me)
{
  gint tmp;

  tmp = (gint) ((y + 1) * 10 + x + 1);
  if (board0[tmp] != BLANK)
    return FALSE;
  if (!is_valid (tmp, whose_turn))
    return FALSE;
  move_board (tmp);
  board_copy ();

  pixmaps[x][y] = (gint8) me;
  gui_draw_pixmap (me, x, y);

  gui_status ();

  start_animation ();
  check_valid_moves ();
  check_computer_players ();

  games_sound_play ("flip-piece");

  return FALSE;
}

/* Back to before board0 */
void
undo (void)
{
  gint8 not_me, count;

  whose_turn = history[--hpointer];	/* history[t1,t2,,,tn, n,who] */
  count = history[--hpointer];

  if (whose_turn == WHITE_TURN) {
    wcount -= count + 1;
    bcount += count;
  } else {
    bcount -= count + 1;
    wcount += count;
  }

  board0[squares[--move_count]] = BLANK;	/* decrease move_count    */

  not_me = OTHER_PLAYER (whose_turn);
  while (count > 0) {
    board0[history[--hpointer]] = not_me;
    count--;
  }
}

/* Sort vsquares by small order */
static
  void
sort (gint l, gint r)
{
  gint i, j, v, w, x;

  i = l;
  j = r;
  x = vsquares[(l + r) / 2][1];
  do {
    while (vsquares[i][1] < x)
      i++;
    while (x < vsquares[j][1])
      j--;
    if (i < j) {
      v = vsquares[i][0];
      w = vsquares[i][1];
      vsquares[i][0] = vsquares[j][0];
      vsquares[i][1] = vsquares[j][1];
      vsquares[j][0] = v;
      vsquares[j][1] = w;
    }
    if (i <= j) {
      i++;
      j--;
    }
  } while (i <= j);

  if (l < j)
    sort (l, j);
  if (i < r)
    sort (i, r);
}

static
  gint
eval_heuristic (void)
{
  gint8 i, xy;
  gint count = 0;

  for (i = 0; i < move_count; i++) {
    xy = squares[i];
    count = (board0[xy] == whose_turn) ?
      count + heuristic[xy] : count - heuristic[xy];
  }

  return count;
}

static
  gint
mobility (void)
{
  gint8 i, xy;
  gint count = 0;

  for (i = move_count; i < 64; i++) {
    xy = squares[i];
    if (is_valid (xy, whose_turn))
      count++;
  }

  return count;
}

static
  gint
around0 (gint xy)
{
  gint count = 0;
  int i;

  for (i = 0; i < NDIRS; i++)
    if (board0[xy + dirs[i]] == BLANK)
      count--;

  if (!count)
    count = 2;

  return count;
}

static
  gint
around (void)
{
  gint8 i, xy;
  gint count = 0;

  for (i = 0; i < move_count; i++) {
    xy = squares[i];
    count = (board0[xy] == whose_turn) ?
      count + around0 ((gint) xy) : count - around0 ((gint) xy);
  }

  return count;
}

/* Evaluate the board   */
static
  gint
b_evaluation (void)
{
  gint score1, score2, score3;

  score1 = mobility ();
  whose_turn = OTHER_PLAYER (whose_turn);
  score1 -= mobility ();
  whose_turn = OTHER_PLAYER (whose_turn);

  score2 = around ();
  score3 = eval_heuristic ();

  return score1 + score2 + score3;
}

/* Victory evaluation */
static
  gint
v_evaluation (void)
{
  gint aa;

  aa = wcount - bcount;
  if (whose_turn == BLACK_TURN)
    aa = -aa;

  if (aa > 0)
    return 1;			/* win  */
  if (aa < 0)
    return -1;			/* lose */
  return 0;			/* draw */
}

static
  gint
p_evaluation (void)
{

  if (whose_turn == WHITE_TURN)
    return wcount - bcount;

  return bcount - wcount;
}

static
  gint
w_evaluation (void)
{
  gint aa;

  if (!bcount || !wcount) {
    if (whose_turn == WHITE_TURN && !bcount)
      return 10000;
    if (whose_turn == BLACK_TURN && !wcount)
      return 10000;
    return -10000;
  }

  aa = wcount - bcount;
  if (whose_turn == BLACK_TURN)
    aa = -aa;

  if (aa > 0)
    return aa + 100;
  if (aa < 0)
    return aa - 100;
  return (0);
}

/* alpha-beta search */
static
  gint
search (gint n, gint a, gint b)
{
  gint aa, bb, xy, i, j;

  if (!n) {
    switch (s_kind) {
    case PERFECT:
      return (p_evaluation ());
    case VICTORY:
      return (v_evaluation ());
    default:
      return (b_evaluation ());
    }
  }

  aa = a;

  j = vpointer;
  for (i = (gint) move_count; i < 64; i++) {	/* entry valid squares */
    xy = (gint) squares[i];
    if (is_valid (xy, whose_turn)) {
      vsquares[++j][0] = xy;
      move_board (xy);
      vsquares[j][1] = mobility ();
      undo ();
    }
  }

  if (j == vpointer) {		/* pass ?  */
    whose_turn = (whose_turn == WHITE_TURN) ? BLACK_TURN : WHITE_TURN;
    if (!mobility ()) {
      whose_turn = OTHER_PLAYER (whose_turn);

      switch (s_kind) {
      case PERFECT:
	return (p_evaluation ());
      case VICTORY:
	return (v_evaluation ());
      default:
	return (w_evaluation ());
      }
    }
    aa = -search (n, -b, -aa);
    whose_turn = OTHER_PLAYER (whose_turn);
    vsquares[j][0] = 0;		/* mark pass              */
    return aa;
  }

  sort (vpointer + 1, j);

  i = vpointer;			/* save old vpointer      */
  vpointer = j + 1;		/* new vpointer           */

  j = i + 1;
  while (aa < b && j < vpointer) {
    xy = vsquares[j][0];
    move_board (xy);
    bb = -search (n - 1, -b, -aa);	/* evaluate this square  */
    undo ();
    if (aa < bb) {
      aa = bb;
      vsquares[i][0] = xy;	/* save this square       */
    }
    j++;
  }
  vpointer = i;			/* pop vpointer           */
  return aa;
}

static
  void
random_select (void)
{
  gint xy, i, j;

  vsquares[0][0] = 0;
  j = 0;
  for (i = move_count; i < 64; i++) {
    xy = squares[i];
    if (is_valid (xy, whose_turn))
      vsquares[j++][0] = xy;
  }
  if (j) {
    i = g_rand_int_range (rgen, 0, j);
    vsquares[0][0] = vsquares[i][0];
  }
}

static
  gint
computer_move (gint level)
{
  gint nn, aa, kind, best_xy;
    
  vsquares[0][0] = -1;
  nn = 64 - move_count;
  if (nn > BEST)
    random_select ();
  else {
    if (nn <= PERFECT - level) {
      kind = PERFECT;
      aa = 127;
    }
    if (PERFECT - level < nn && nn <= VICTORY - level) {
      kind = VICTORY;
      aa = 2;
    }
    if (VICTORY - level < nn && nn <= BEST) {
      kind = BEST;
      aa = 32767;
      nn = MAX_DEPTH - level;
    }
    s_kind = kind;
    vsquares[0][1] = search (nn, -aa, aa);
  }

  best_xy = vsquares[0][0];

  if (best_xy == -1) {		/* If I can't win, then search the best again. */
    s_kind = BEST;
    vsquares[0][1] = search (MAX_DEPTH - level, -32767, 32767);
    best_xy = vsquares[0][0];
  }

  if (best_xy)
    move (best_xy % 10 - 1, best_xy / 10 - 1, whose_turn);

  return FALSE;
}

gint
computer_move_1 (guint me)
{
  return computer_move (6);
}

gint
computer_move_2 (guint me)
{
  return computer_move (4);
}

gint
computer_move_3 (guint me)
{
  return computer_move (2);
}

gint
computer_move_4 (guint me)
{
  return computer_move (0);
}

gboolean
flip_final_results (gpointer data)
{
  guint i;
  guint white_pieces;
  guint black_pieces;
  guint adder = 0;
    
  white_pieces = wcount;
  black_pieces = bcount;

  i = 0;
  for (; i < black_pieces; i++) {
    board[i % 8][i / 8] = BLACK_TURN;
    if (pixmaps[i % 8][i / 8] < 1)
      pixmaps[i % 8][i / 8] = WHITE_TURN;
    if (pixmaps[i % 8][i / 8] == WHITE_TURN) {
      pixmaps[i % 8][i / 8] += adder;
      if (animate_stagger)
	adder++;
    }
  }
  for (; i < 64 - white_pieces; i++) {
    board[i % 8][i / 8] = 0;
    pixmaps[i % 8][i / 8] = 100;
  }
  for (; i < 64; i++) {
    board[i % 8][i / 8] = WHITE_TURN;
    if (pixmaps[i % 8][i / 8] == 0)
      pixmaps[i % 8][i / 8] = BLACK_TURN;
    if (pixmaps[i % 8][i / 8] == BLACK_TURN) {
      pixmaps[i % 8][i / 8] -= adder;
      if (animate_stagger)
	adder++;
    }
  }

  start_animation ();

  return FALSE;
}

gint
check_valid_moves (void)
{
  guint white_moves = 0;
  guint black_moves = 0;

  if (!game_in_progress)
    return TRUE;
  if (mobility ())
    return TRUE;

  whose_turn = OTHER_PLAYER (whose_turn);

  if (!mobility ()) {
    white_moves = wcount;
    black_moves = bcount;
    if (white_moves > black_moves)
      gui_message (_("Light player wins!"));
    if (black_moves > white_moves)
      gui_message (_("Dark player wins!"));
    if (white_moves == black_moves)
      gui_message (_("The game was a draw."));
    whose_turn = 0;
    game_in_progress = 0;
    if (flip_final)
      flip_final_id = g_timeout_add (100, flip_final_results, NULL);

    games_sound_play ("gameover");

    return TRUE;
  }

  if (whose_turn == BLACK_TURN) {
    gui_message (_("Light must pass, Dark's move"));
    return TRUE;
  }

  if (whose_turn == WHITE_TURN) {
    gui_message (_("Dark must pass, Light's move"));
    return TRUE;
  }

  return TRUE;
}
