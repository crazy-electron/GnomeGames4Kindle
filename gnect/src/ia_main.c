/****************************************************************************
 *                                                                          *
 *                      Velena Source Code V1.0                             *
 *                   Written by Giuliano Bertoletti                         *
 *       Based on the knowledged approach of Louis Victor Allis             *
 *   Copyright (C) 1996-97 by Giuliano Bertoletti & GBE 32241 Software PR   *
 *                                                                          *
 ****************************************************************************

 Portable engine version.
 read the README file for further informations.

 ============================================================================

 Changes have been made to this code for inclusion with Gnect. It is
 released under the GNU General Public License with Giuliano's approval.
 The original and complete Velena Engine source code can be found at:

 http://www.ce.unipr.it/~gbe/velena.html

*/


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>

#include "connect4.h"
#include "con4vals.h"
#include "pnsearch.h"
#include "proto.h"

#define BLSTRSIZE 14

/* FIXME: These should probably go into a header file or be defined as static */
void collapse_position (unsigned char *mypos, unsigned char *blk);
short get_black_best_move (struct board *board);
void reverse_board (struct board *board);
short pentas (struct board *board, short x, short y, short side);
short check_pentas (struct board *board, short side);
short gen_odd_threat (struct board *board, short x, short side);
short check_double (struct board *board, short group, short pos, short side);
short groupeval (struct board *board);
short connected (struct board *board, short move);
short opponent_connected (struct board *board, short move);
short makemove (struct board *board, short move);
short undomove (struct board *board, short move);
int get_game_result (struct board *board);
short endgame (struct board *board);
short try_to_win (struct board *board);
short fast_try_to_win (struct board *board);
short avoid_tricks (struct board *board, short side);
short play_tricks (struct board *board, short side);
short save_position (struct board *board);
short explore_tree (struct board *board, short side, short depth);
short look_ahed (struct board *board);
short defending_function (struct board *board);
short heuristic_defend_best (struct board *board);
short mybincmp (unsigned char *p1, unsigned char *p2, short len);
short check_book (struct board *board, unsigned char *cmparray, short side);
short get_lower (short *bb, unsigned char *tp);
short use_opening_book (struct board *board, short side);
short check_presence (struct board *board, short side);
short avoid_immediate_loss (struct board *board);
short ia_compute_move (struct board *board);
short get_last_move (struct board *board);



short maxdepth;



void
collapse_position (unsigned char *mypos, unsigned char *blk)
{
  short y;


  for (y = 0; y < BOARDY; y++) {
    blk[0 + y * 2] = ((mypos[ELM (0, y)] & 0x03) << 6) |
      ((mypos[ELM (1, y)] & 0x03) << 4) |
      ((mypos[ELM (2, y)] & 0x03) << 2) | ((mypos[ELM (3, y)] & 0x03));

    blk[1 + y * 2] = ((mypos[ELM (4, y)] & 0x03) << 6) |
      ((mypos[ELM (5, y)] & 0x03) << 4) |
      ((mypos[ELM (6, y)] & 0x03) << 2) | 0x03;
  }
}



short
get_black_best_move (struct board *board)
{
  short black_str[BLSTRSIZE] = { 4, 4, 4, 4, 4, 2, 2, 2, 2, 6, 6, 6, 6, 6 };
  short x, flag = YES;


  if (board->filled >= BLSTRSIZE)
    return -1;

  for (x = 0; x < board->filled && flag; x++)
    if (board->moves[x] != (black_str[x] - 1))
      flag = NO;

  if (flag)
    return black_str[board->filled] - 1;

  return -1;
}



void
reverse_board (struct board *board)
{
  short x, y, z;

  for (y = 0; y < BOARDY; y++)
    for (x = 0; x < BOARDX / 2; x++) {
      z = board->square[ELM (x, y)];
      board->square[ELM (x, y)] = board->square[ELM (BOARDX - x - 1, y)];
      board->square[ELM (BOARDX - x - 1, y)] = z;
    }

  for (x = 0; x < BOARDX / 2; x++) {
    z = board->stack[x];
    board->stack[x] = board->stack[BOARDX - x - 1];
    board->stack[BOARDX - x - 1] = z;
  }
}



short
pentas (struct board *board, short x, short y, short side)
{
  if (board->stack[x] >= y)
    return NO;
  if (board->stack[x + 2] >= y)
    return NO;
  if (board->stack[x + 4] >= y)
    return NO;

  if (board->square[ELM (x + 1, y)] != side)
    return NO;
  if (board->square[ELM (x + 3, y)] != side)
    return NO;

  return YES;
}



short
check_pentas (struct board *board, short side)
{
  short x, y, px, py, flag = 0;


  for (x = 0; x < 3 && !flag; x++)
    for (y = 0; y < 2 && !flag; y++) {
      px = x;
      py = (y + 1) * 2;

      flag = (flag || pentas (board, px, py, side));
    }

  return flag;
}



short
gen_odd_threat (struct board *board, short x, short side)
{
  short y, empty = 0, fill = 0, emp[TILES], px = 0, py = 0;	/* px, py; TRM */

  for (y = 0; y < TILES; y++) {
    if (*board->groups[x][y] == EMPTY) {
      px = board->xplace[x][y];
      py = board->yplace[x][y];
      emp[empty++] = ELM (px, py);
    } else if (*board->groups[x][y] == side)
      fill++;
  }

  if (empty == 1 && fill == 3 && (py & 1) == 0 && board->stack[px] < py)
    return emp[0];
  return -1;
}



short
check_double (struct board *board, short group, short pos, short side)
{
  short x, y;

  for (x = group + 1; x < GROUPS; x++) {
    y = gen_odd_threat (board, x, side);
    if (y == pos)
      return YES;
  }

  return NO;
}


short
groupeval (struct board *board)
{
  short x;
  short z, f, p1, p2, t1, t2, score = 0;

  t1 = board->turn;
  t2 = board->turn ^ SWITCHSIDE;


  for (x = 0; x < GROUPS; x++) {

    p1 = 0;
    p2 = 0;

    if (*board->groups[x][0] == t1)
      p1++;
    else if (*board->groups[x][0] == t2)
      p2++;

    if (*board->groups[x][1] == t1)
      p1++;
    else if (*board->groups[x][1] == t2)
      p2++;

    if (*board->groups[x][2] == t1)
      p1++;
    else if (*board->groups[x][2] == t2)
      p2++;

    if (*board->groups[x][3] == t1)
      p1++;
    else if (*board->groups[x][3] == t2)
      p2++;

    if (p1 == 4)
      return GOODMOVE;
    if (p2 == 4)
      return BADMOVE;

    if (p1 == 3 && p2 == 0) {
      score += 100;
      z = gen_odd_threat (board, x, t1);
      if (z != -1) {
	f = check_double (board, x, z, t1);
	if (!f) {
	  if (t1 == WHITE)
	    score += 200;
	  else
	    score += 150;
	} else {
	  if (t1 == WHITE)
	    score += 750;
	  else
	    score += 500;
	}
      }
    }

    if (p2 == 3 && p1 == 0) {
      score -= 100;
      z = gen_odd_threat (board, x, t2);
      if (z != -1) {
	f = check_double (board, x, z, t2);
	if (!f) {
	  if (t1 == BLACK)
	    score -= 200;
	  else
	    score -= 150;
	} else {
	  if (t1 == BLACK)
	    score -= 750;
	  else
	    score -= 500;
	}
      }
    }

    if (p1 == 2 && p2 == 0)
      score += 10;
    if (p2 == 2 && p1 == 0)
      score -= 10;
  }


  if (check_pentas (board, WHITE)) {
    if (t1 == WHITE)
      score += 800;
    else
      score -= 800;
  }

  return score;
}



short
connected (struct board *board, short move)
{
  short maxcon = 1;
  short px, py, connect;

  /* Check vertical */

  connect = 1;

  px = move;
  py = board->stack[move] - 1;

  while (py >= 0 && board->square[ELM (px, py)] == board->turn) {
    connect++;
    py--;
  }

  maxcon = max (connect, maxcon);

  /* Check horizontal */

  connect = 1;

  px = move - 1;
  py = board->stack[move];

  while (px >= 0 && board->square[ELM (px, py)] == board->turn) {
    connect++;
    px--;
  }

  px = move + 1;

  while (px < BOARDX && board->square[ELM (px, py)] == board->turn) {
    connect++;
    px++;
  }

  maxcon = max (connect, maxcon);

  /* Check NW - SE diagonal */

  connect = 1;

  px = move - 1;
  py = board->stack[move] + 1;

  while (px >= 0 && py < BOARDY && board->square[ELM (px, py)] == board->turn) {
    connect++;
    px--;
    py++;
  }

  px = move + 1;
  py = board->stack[move] - 1;

  while (px < BOARDX && py >= 0 && board->square[ELM (px, py)] == board->turn) {
    connect++;
    px++;
    py--;
  }

  maxcon = max (connect, maxcon);

  /* Check NE - SW */

  connect = 1;

  px = move - 1;
  py = board->stack[move] - 1;

  while (px >= 0 && py >= 0 && board->square[ELM (px, py)] == board->turn) {
    connect++;
    px--;
    py--;
  }

  px = move + 1;
  py = board->stack[move] + 1;

  while (px < BOARDX && py < BOARDY
	 && board->square[ELM (px, py)] == board->turn) {
    connect++;
    px++;
    py++;
  }

  maxcon = max (connect, maxcon);

  return maxcon;
}



short
opponent_connected (struct board *board, short move)
{
  short con;


  board->turn ^= SWITCHSIDE;
  con = connected (board, move);
  board->turn ^= SWITCHSIDE;

  return con;
}



short
makemove (struct board *board, short move)
{
  if (board->stack[move] >= 6)
    return 0;

  board->square[ELM (move, board->stack[move]++)] = board->turn;
  board->moves[board->filled] = move;
  board->mlist[board->filled] = ELM (move, board->stack[move]);
  board->turn ^= SWITCHSIDE;
  board->filled++;

  return 1;
}



short
undomove (struct board *board, short move)
{
  if (board->stack[move] < 1)
    return 0;

  board->square[ELM (move, --board->stack[move])] = EMPTY;
  board->turn ^= SWITCHSIDE;
  board->filled--;
  board->moves[board->filled] = -1;
  board->mlist[board->filled] = -1;

  return 1;
}



int
get_game_result (struct board *board)
{
  short answer = -1;
  short i;


  for (i = 0; i < GROUPS && answer == -1; i++) {
    if (*board->groups[i][0] != EMPTY &&
	*board->groups[i][0] == *board->groups[i][1] &&
	*board->groups[i][0] == *board->groups[i][2] &&
	*board->groups[i][0] == *board->groups[i][3]) {
      if (*board->groups[i][0] == WHITE)
	answer = WHITE;
      else
	answer = BLACK;
    }
  }

  if (board->filled == MAXSQUARES && answer == -1)
    answer = 0;
  return answer;
}



short
endgame (struct board *board)
{
  short answer = NOTHING;
  short i, j;

  j = 0;
  for (i = 0; i < GROUPS && answer == NOTHING; i++) {
    if (*board->groups[i][0] != EMPTY &&
	*board->groups[i][0] == *board->groups[i][1] &&
	*board->groups[i][0] == *board->groups[i][2] &&
	*board->groups[i][0] == *board->groups[i][3])
      answer = WIN;

    j = *board->groups[i][0];
  }

  if (board->filled == MAXSQUARES && answer == NOTHING) {
    answer = DRAW;
    board->lastwin = 0;
  }

  if (answer == WIN) {
    board->wins[j - 1]++;
    board->lastwin = j;
    if (board->oracle[2 - j] == YES)
      fatal_error ("Oracle failed to tell the truth");
  }

  else if (answer == DRAW)
    board->draws++;

  return answer;
}



short
try_to_win (struct board *board)
{
  short x, av[BOARDX], win = NO;


  for (x = 0; x < BOARDX; x++) {
    if (board->stack[x] < BOARDY && connected (board, x) >= 4) {
      av[x] = YES;
      win = YES;
    } else
      av[x] = NO;
  }

  if (win) {
    do {
      x = my_random (BOARDX);
    } while (av[x] == NO);
  } else
    x = -1;

  return x;
}



short
fast_try_to_win (struct board *board)
{
  short x, win = -1;


  for (x = 0; x < BOARDX; x++)
    if (board->stack[x] < BOARDY && connected (board, x) >= 4)
      win = x;

  return win;
}



short
avoid_tricks (struct board *board, short side)
{
  short x, answ, cn = 0, tmp;


  maxdepth++;

  if (board->turn != side) {
    answ = GOODMOVE;

    for (x = 0; x < BOARDX; x++) {
      if (board->stack[x] < BOARDY) {
	cn++;
	if (connected (board, x) >= 4) {
	  maxdepth--;
	  return BADMOVE;
	} else {
	  makemove (board, x);
	  tmp = avoid_tricks (board, side);
	  answ = min (answ, tmp);
	  undomove (board, x);
	}
      }

    }

    if (cn == 0)
      answ = NOCOMMENT;
  }

  else {
    answ = GOODMOVE;

    for (x = 0; x < BOARDX; x++) {
      if (board->stack[x] < BOARDY) {
	cn++;
	if (connected (board, x) >= 4) {
	  maxdepth--;
	  return GOODMOVE;
	} else if (opponent_connected (board, x) >= 4) {
	  makemove (board, x);
	  tmp = avoid_tricks (board, side);
	  answ = min (answ, tmp);
	  undomove (board, x);
	}
      }
    }

    if (cn == 0)
      answ = NOCOMMENT;
  }

  maxdepth--;

  return answ;
}



short
play_tricks (struct board *board, short side)
{
  short tmp;


  tmp = avoid_tricks (board, side ^ SWITCHSIDE);

  switch (tmp) {
  case GOODMOVE:
    tmp = BADMOVE;
    break;

  case BADMOVE:
    tmp = GOODMOVE;
    break;
  }

  return tmp;
}



short
save_position (struct board *board)
{
  short x, y = GOODMOVE, z = IMPOSSIBLE, pos, score[BOARDX];


  for (x = 0; x < BOARDX; x++) {
    score[x] = IMPOSSIBLE;
    if (board->stack[x] < BOARDY) {
      makemove (board, x);
      score[x] = avoid_tricks (board, board->turn ^ SWITCHSIDE);
      y = min (score[x], y);
      z = max (score[x], z);
      undomove (board, x);
    }
  }

  if (y < GOODMOVE) {
    do {
      pos = my_random (BOARDX);
    } while (score[pos] != z);
  }

  else
    pos = -1;

  return pos;
}



short
explore_tree (struct board *board, short side, short depth)
{
  short x, answ, cn = 0, tmp;


  if (depth == 0) {
    tmp = groupeval (board);
    if (board->turn == side)
      return tmp;
    else
      return -tmp;
  }

  /*
   * if((depth==DEPTH-1) && avoid_tricks(board,side)==BADMOVE)
   * return BADMOVE;
   * 
   * 
   * if((depth==DEPTH-1) && play_tricks(board,side)==GOODMOVE)
   * return GOODMOVE;
   */

  maxdepth++;

  if (board->turn != side) {
    answ = GOODMOVE - depth;

    for (x = 0; x < BOARDX && answ != BADMOVE; x++) {

      while (gtk_events_pending ())
	gtk_main_iteration ();

      if (board->stack[x] < BOARDY) {
	cn++;
	if (connected (board, x) >= 4)
	  answ = BADMOVE;
	else {
	  makemove (board, x);
	  tmp = explore_tree (board, side, depth - 1);
	  answ = min (answ, tmp);
	  undomove (board, x);
	}
      }

    }

    if (cn == 0)
      answ = NOCOMMENT;
  }

  else {
    answ = BADMOVE + depth;

    for (x = 0; x < BOARDX; x++) {

      while (gtk_events_pending ())
	gtk_main_iteration ();

      if (board->stack[x] < BOARDY) {
	cn++;
	if (connected (board, x) >= 4)
	  answ = GOODMOVE;
	else {
	  makemove (board, x);
	  tmp = explore_tree (board, side, depth - 1);
	  answ = max (answ, tmp);
	  undomove (board, x);
	}
      }
    }

    if (cn == 0)
      answ = NOCOMMENT;
  }

  maxdepth--;

  return answ;
}



short
look_ahed (struct board *board)
{
  long def_nodes = 0L;
  short x, y, score[BOARDX], sc = IMPOSSIBLE, p;
  short depth = DEPTH, oracle, set = NO, def_depth = 0, cpu_level;


  maxdepth = 0;

  if (board->turn == WHITE)
    cpu_level = board->white_lev;
  else
    cpu_level = board->black_lev;

  for (x = 0; x < BOARDX; x++) {
    score[x] = IMPOSSIBLE;
    if (board->stack[x] < BOARDY) {
      makemove (board, x);

      p = heuristic_play_best (board, 2800L);
      def_nodes += board->nodes_visited;
      def_depth = max (def_depth, board->maxtreedepth);

      if (p >= 0)
	score[x] = BADMOVE + board->maxtreedepth;
      else if (p == -2)
	score[x] = GOODMOVE - board->maxtreedepth;
      else {
	score[x] = explore_tree (board, board->turn ^ SWITCHSIDE, depth - 1);
	if (score[x] > -3500 && score[x] < 3500) {
	  if (board->autotest)
	    score[x] = my_random (RANDVARIANCE);
	  else
	    score[x] += my_random (RANDVARIANCE);
	}

	if (cpu_level > 1)
	  oracle = evaluation_function (board);
	else
	  oracle = 0;

	score[x] += (oracle << 13);
	if (oracle) {
	  set = YES;
	  if (!board->oracle[2 - board->turn]) {
	    board->lastguess = board->filled + 1;
	    board->oracle[2 - board->turn] = YES;
	    if (board->lastguess < board->bestguess)
	      board->bestguess = board->lastguess;
	  }
	}
      }
      undomove (board, x);
    }

    sc = max (score[x], sc);
  }

  for (x = 0, y = 0; x < BOARDX; x++)
    if (score[x] == sc)
      y++;

  board->choices[board->filled] = y;

  do {
    x = my_random (BOARDX);
  } while (score[x] != sc);

  return x;
}



short
defending_function (struct board *board)
{
  int x, y = 0;


  for (x = 0; x < BOARDX; x++) {
    if (board->stack[x] < BOARDY) {
      makemove (board, x);
      if (evaluation_function (board) ||
	  heuristic_play_best (board, 2800L) >= 0)
	y++;
      undomove (board, x);
    } else
      y++;
  }

  if (y < BOARDX)
    return 0;
  return 1;
}



short
heuristic_defend_best (struct board *board)
{
  int x, y = 0;


  for (x = 0; x < BOARDX; x++) {
    if (board->stack[x] < BOARDY) {
      makemove (board, x);
      if (heuristic_play_best (board, 2800L) >= 0)
	y++;
      undomove (board, x);
    } else
      y++;
  }

  if (y < BOARDX)
    return -1;
  return 1;
}



short
mybincmp (unsigned char *p1, unsigned char *p2, short len)
{
  short x;


  for (x = 0; x < len; x++) {
    if (p1[x] < p2[x])
      return -1;
    else if (p1[x] > p2[x])
      return +1;
  }

  return 0;
}



short
check_book (struct board *board, unsigned char *cmparray, short side)
{
  short res, value;
  long x, head, tail;


  head = 0;
  tail = board->wbposit;

  if (side == WHITE)
    value = PROVED;
  else
    value = DISPROVED;

  cmparray[12] = (value) & 255;
  cmparray[13] = (value) >> 8;

  do {
    x = (head + tail) / 2;
    res = mybincmp (cmparray, &board->white_book[14 * x], 14);
    if (res == 1)
      head = x + 1;
    else if (res == -1)
      tail = x;
  } while (res != 0 && head != tail);

  if (res == 0) {
    board->lastob = x;
    return YES;
  }
  return NO;
}



short
get_lower (short *bb, unsigned char *tp)
{
  unsigned char tp2[64];	/* bugfix from GBE, 2001-05-20) */
  short x, y, flag = 0;


  memset (tp, 0xff, 64);
  memset (tp2, 0xff, 64);

  for (y = 0; y < BOARDY; y++)
    for (x = 0; x < BOARDX; x++) {
      tp[ELM (x, y)] = bb[ELM (x, y)];
      tp2[ELM (x, y)] = bb[ELM (BOARDX - x - 1, y)];
    }

  for (x = 0; x < 64 && flag == 0; x++) {
    if (tp[x] < tp2[x])
      flag = -1;
    else if (tp[x] > tp2[x])
      flag = 1;
  }

  if (flag == 1) {
    memcpy (tp, tp2, 64);
    return 0;
  }

  return 1;
}



short
use_opening_book (struct board *board, short side)
{
  char flags[BOARDX], flagno = 0;
  unsigned char cmparray[14];
  unsigned char tempboard[64];
  short x;


  if (!board->white_book)
    return -1;

  for (x = 0; x < BOARDX; x++) {
    flags[x] = NO;
    if (makemove (board, x)) {
      get_lower (board->square, tempboard);
      collapse_position (tempboard, cmparray);
      if (check_book (board, cmparray, side)) {
	flags[x] = YES;
	flagno++;
      }
      undomove (board, x);
    }
  }

  if (flagno)
    while (1) {
      x = my_random (BOARDX);
      if (flags[x])
	return x;
    }

  return -1;
}



short
check_presence (struct board *board, short side)
{
  unsigned char tempboard[64], cmparray[14];


  get_lower (board->square, tempboard);
  collapse_position (tempboard, cmparray);
  return check_book (board, cmparray, side);
}



short
avoid_immediate_loss (struct board *board)
{
  short move;


  board->turn ^= SWITCHSIDE;
  move = try_to_win (board);
  board->turn ^= SWITCHSIDE;
  return move;
}



short
ia_compute_move (struct board *board)
{
  short move, x;


  board->choices[board->filled] = 1;

  if (board->filled == 0) {
    move = 3;
    goto IA_RETURN_MOVE;
  }

  if (board->filled == 1) {
    move = 3;
    if (board->moves[0] == 1)
      move = 2;
    else if (board->moves[0] == 5)
      move = 4;
    goto IA_RETURN_MOVE;
  }

  if (board->filled == MAXMEN - 1) {
    for (x = 0; x < BOARDX; x++) {
      if (board->stack[x] < BOARDY) {
	move = x;
	goto IA_RETURN_MOVE;
      }
    }
    fatal_error ("I shouldn't have come here...");
  }

  move = try_to_win (board);
  if (move >= 0)
    goto IA_RETURN_MOVE;

  move = avoid_immediate_loss (board);
  if (move >= 0)
    goto IA_RETURN_MOVE;

  if (board->turn == BLACK && board->black_lev == 3) {
    move = get_black_best_move (board);
    if (move >= 0)
      goto IA_RETURN_MOVE;
  }

  else if (board->filled == 1 && board->stack[3] == 1) {
    if (board->autotest) {
      short sel;

      /* The probability should not be uniform, but triangular */
      sel = my_random (15);

      if (sel == 0)
	move = 0;
      else if (sel <= 2)
	move = 1;
      else if (sel <= 5)
	move = 2;
      else if (sel <= 9)
	move = 3;
      else if (sel <= 12)
	move = 4;
      else if (sel <= 14)
	move = 5;
      else
	move = 6;
    } else
      move = 3;

    goto IA_RETURN_MOVE;
  }

  /* Let's look in the opening book */
  switch (board->turn) {
  case WHITE:
    if (board->white_lev == 3)
      move = use_opening_book (board, WHITE);
    else
      move = -1;
    break;

  case BLACK:
    if (board->black_lev == 3)
      move = use_opening_book (board, BLACK);
    else
      move = -1;
    break;

  default:
    fatal_error ("I don't know who's to move...");
    break;
  }

  if (move >= 0)
    goto IA_RETURN_MOVE;

  fight (NO);
  move = heuristic_play_best (board, 2800L);

  /* Here we handle the fact that Black could also look for a win
   * improving the algorithm.
   */

  if (board->turn == BLACK) {
    short temp;


    fight (YES);
    temp = heuristic_play_best (board, 2800L);
    fight (NO);

    if (temp >= 0)
      move = temp;
  }

  if (move >= 0)
    goto IA_RETURN_MOVE;

  move = look_ahed (board);

IA_RETURN_MOVE:
  return move;
}



/* *************************************************************************** */



short
get_last_move (struct board *board)
{
  short x, y = -1;


  for (x = 0; x < BOARDX && y == -1; x++)
    if (board->stack[x] < BOARDY)
      y = x;

  if (y == -1)
    fatal_error ("Problem finding a solution...");

  if (connected (board, y) >= 4)
    return 10;
  return 11;
}
