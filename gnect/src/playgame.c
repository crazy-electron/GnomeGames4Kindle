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

#include "connect4.h"
#include "pnsearch.h"
#include "proto.h"


/* FIXME: These should probably go into a header file or be defined as static */
short parse_input_string (char *str, struct board *board);
short playgame (char *input_str, struct board *board);



short
parse_input_string (char *str, struct board *board)
{
  short cnt = 0, move;
  char *p;

  p = str;

  board->cpu = 3;

  switch (*p) {
  case 'a':
    board->white_lev = 1;
    board->black_lev = 1;
    break;

  case 'b':
    board->white_lev = 2;
    board->black_lev = 2;
    break;

  case 'c':
    board->white_lev = 3;
    board->black_lev = 3;
    break;

  default:
    return -1;
  }

  p++;

  while (p[cnt] != '0') {
    move = (short) (p[cnt] - '1');
    if (move < 0 || move > 6)
      return -1;
    if (board->stack[move] < BOARDY)
      makemove (board, move);
    else
      return -2;

    if (cnt >= 42 || endgame (board) != 0)
      return -2;
    cnt++;
  }

  return 1;
}



short
playgame (char *input_str, struct board *board)
{
  int flag;

  board->oracle[0] = 0;
  board->oracle[1] = 0;

  initboard (board);
  flag = parse_input_string (input_str, board);
  if (flag <= 0)
    return flag;

  return ia_compute_move (board) + 1;
}
