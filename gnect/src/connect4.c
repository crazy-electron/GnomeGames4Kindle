/* -*-mode:c; c-style:k&r; c-basic-offset:4; -*- */


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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <zlib.h>
#include <gtk/gtk.h>

#include <libgames-support/games-runtime.h>

#include "connect4.h"
#include "pnsearch.h"
#include "proto.h"
#include "main.h"

#define PLAYER1 0
#define PLAYER2 1



struct board *brd;




G_GNUC_NORETURN
void
fatal_error (char *str)
{
  g_printerr ("velena: %s\n", str);
  exit (1);
}

int
get_random_int (int n)
{
  /* Return a random integer in the range 1..n */
  return (int) g_random_int_range (1, n + 1);
}


int
my_random (unsigned short maxval)
{
  /* range: 0..maxval-1 */
  return (get_random_int (maxval) - 1);
}



static short
check_solution_groups (struct board *board)
{
  short x, y, z, q, c, answer = YES;

  for (y = 0; y < BOARDY && answer; y++) {
    for (x = 0; x < BOARDX && answer; x++) {
      for (z = 0; z < board->solvable_groups->sqpnt[ELM (x, y)] && answer;
	   z++) {
	answer = NO;
	c = board->solvable_groups->square[ELM (x, y)][z];
	for (q = 0; q < TILES; q++) {
	  if (*board->groups[c][q] == ELM (x, y)) {
	    answer = YES;
	  }
	}
      }
    }
  }
  return (answer);
}


/* Initialize the program data structures, it reads and builds (if needed)
 * the opening book.
 */

static void
init_prg (struct board *board)
{
  uLong crc = crc32 (0L, Z_NULL, 0);
  long ob_size, len;
  FILE *h1;
  short x;
  const char *tmp = games_runtime_get_directory (GAMES_RUNTIME_GAME_DATA_DIRECTORY);
  char *bookdata = g_build_filename (tmp, WHITE_BOOK, NULL);

  if (!g_file_test (bookdata, G_FILE_TEST_EXISTS)) {
    g_printerr ("velena: required file not found (%s)\n", bookdata);
    exit (1);
  }

  brd = board;

  board->wins[PLAYER1] = 0;
  board->wins[PLAYER2] = 0;
  board->draws = 0;
  board->lastguess = 0;
  board->bestguess = MAXMEN;
  board->lastwin = EMPTY;

  board->white_lev = 0;		/* Human */
  board->black_lev = 3;		/* Computer-Strong */
  board->videotype = CHARS;
  board->enablegr = NO;
  board->autotest = NO;

  for (x = 0; x < 3; x++) {
    board->rule[x] = 0L;
  }

  board->oracle_guesses = 0;


  h1 = gzopen (bookdata, "rb");
  if (!h1) {
    g_printerr ("velena: could not open required file (%s)\n", bookdata);
    exit (1);
  }

  ob_size = OPENINGBOOK_LENGTH;

  board->wbposit = ob_size / 14;

  board->white_book = (unsigned char *) malloc (ob_size);
  if (!board->white_book) {
    fatal_error ("Not enough memory to allocate opening book");
  }

  len = 0;			/* We read all the position from disk */
  while (!gzeof (h1) && (len < ob_size)) {
    /* each position takes 14 bytes of storage */
    gzread (h1, &board->white_book[len], 14);
    crc = crc32 (crc, &board->white_book[len], 14);
    len += 14;
  }

  if (crc != OPENINGBOOK_CRC) {
    fatal_error ("Opening book is corrupt");
  }

  gzclose (h1);
  board->bbposit = 0;
  g_free(bookdata);

}



void
initboard (struct board *board)
{
  short x, y, i, j, p;

  /* randomize(); *//* see main.c, game_init() */

  for (i = 0; i < 10; i++) {
    board->instances[i] = 0L;
  }

  /* Groups initializations */

  /* Step one. Horizontal lines. */

  i = 0;
  for (y = 0; y < BOARDY; y++) {
    for (x = 0; x < BOARDX - 3; x++) {
      board->groups[i][0] = &board->square[ELM (x + 0, y)];
      board->groups[i][1] = &board->square[ELM (x + 1, y)];
      board->groups[i][2] = &board->square[ELM (x + 2, y)];
      board->groups[i][3] = &board->square[ELM (x + 3, y)];

      board->xplace[i][0] = x;
      board->xplace[i][1] = x + 1;
      board->xplace[i][2] = x + 2;
      board->xplace[i][3] = x + 3;

      board->yplace[i][0] = y;
      board->yplace[i][1] = y;
      board->yplace[i][2] = y;
      board->yplace[i][3] = y;

      i++;
    }
  }

  /* Step two. Vertical lines */

  for (y = 0; y < BOARDY - 3; y++) {
    for (x = 0; x < BOARDX; x++) {
      board->groups[i][0] = &board->square[ELM (x, y + 0)];
      board->groups[i][1] = &board->square[ELM (x, y + 1)];
      board->groups[i][2] = &board->square[ELM (x, y + 2)];
      board->groups[i][3] = &board->square[ELM (x, y + 3)];

      board->xplace[i][0] = x;
      board->xplace[i][1] = x;
      board->xplace[i][2] = x;
      board->xplace[i][3] = x;

      board->yplace[i][0] = y + 0;
      board->yplace[i][1] = y + 1;
      board->yplace[i][2] = y + 2;
      board->yplace[i][3] = y + 3;

      i++;
    }
  }

  /* Step three. Diagonal (north east) lines */

  for (y = 0; y < BOARDY - 3; y++) {
    for (x = 0; x < BOARDX - 3; x++) {
      board->groups[i][0] = &board->square[ELM (x + 0, y + 0)];
      board->groups[i][1] = &board->square[ELM (x + 1, y + 1)];
      board->groups[i][2] = &board->square[ELM (x + 2, y + 2)];
      board->groups[i][3] = &board->square[ELM (x + 3, y + 3)];

      board->xplace[i][0] = x + 0;
      board->xplace[i][1] = x + 1;
      board->xplace[i][2] = x + 2;
      board->xplace[i][3] = x + 3;

      board->yplace[i][0] = y + 0;
      board->yplace[i][1] = y + 1;
      board->yplace[i][2] = y + 2;
      board->yplace[i][3] = y + 3;

      i++;
    }
  }

  /* Step four. Diagonal (south east) lines */

  for (y = 3; y < BOARDY; y++) {
    for (x = 0; x < BOARDX - 3; x++) {
      board->groups[i][0] = &board->square[ELM (x + 0, y - 0)];
      board->groups[i][1] = &board->square[ELM (x + 1, y - 1)];
      board->groups[i][2] = &board->square[ELM (x + 2, y - 2)];
      board->groups[i][3] = &board->square[ELM (x + 3, y - 3)];

      board->xplace[i][0] = x + 0;
      board->xplace[i][1] = x + 1;
      board->xplace[i][2] = x + 2;
      board->xplace[i][3] = x + 3;

      board->yplace[i][0] = y - 0;
      board->yplace[i][1] = y - 1;
      board->yplace[i][2] = y - 2;
      board->yplace[i][3] = y - 3;

      i++;
    }
  }

  if (i != GROUPS) {
    fatal_error ("Implementation error!");
  }

  for (x = 0; x < 64; x++) {
    board->solvable_groups->sqpnt[x] = 0;
    for (y = 0; y < 16; y++) {
      board->solvable_groups->square[x][y] = -1;
    }
  }

  for (x = 0; x < BOARDX; x++) {
    for (y = 0; y < BOARDY; y++) {
      board->square[ELM (x, y)] = ELM (x, y);
    }
  }

  for (i = 0; i < GROUPS; i++) {
    for (j = 0; j < TILES; j++) {
      p = *board->groups[i][j];
      board->solvable_groups->square[p][board->solvable_groups->sqpnt[p]++] =
	i;
    }
  }

  if (!check_solution_groups (board)) {
    fatal_error ("Implementation error!");
  }

  /* Here we set all out squares to a default value to detect problems */

  for (i = 0; i < 8; i++) {
    board->square[ELM (7, i)] = FULL;
    board->square[ELM (i, 6)] = board->square[ELM (i, 7)] = FULL;
  }

  board->stack[7] = FULL;

  for (y = 0; y < BOARDY; y++) {
    for (x = 0; x < BOARDX; x++) {
      board->square[ELM (x, y)] = EMPTY;
    }
  }

  for (x = 0; x < BOARDX; x++) {
    board->stack[x] = 0;
  }

  board->turn = WHITE;
  board->filled = 0;
  board->cpu = 0x01;

  return;
}



struct board *
veleng_init (void)
{
  short x;
  struct board *board;

  /* Here we initialize our environment and the call
   * command_line_input in file cmdline.c to process data from the
   * outside world.
   */


  fight (NO);

  brd = NULL;
  board = (struct board *) malloc (sizeof (struct board));
  if (!board) {
    fatal_error ("Cannot allocate memory!");
  }

  board->solvable_groups =
    (struct solvable_groups *) malloc (sizeof (struct solvable_groups));
  if (!board->solvable_groups) {
    fatal_error ("Cannot allocate memory!");
  }

  board->debug = 0;

  init_prg (board);		/* Initialize data structures */

  for (x = 0; x < ALLOC_SOLUTIONS; x++) {
    board->solution[x] =
      (struct solution *) malloc (sizeof (struct solution));
    if (!board->solution[x]) {
      fatal_error ("Not enough memory for solutions");
    }
  }

  board->usegraphics = NO;

  return (board);
}



void
veleng_free (struct board *board)
{
  int x;

  if (board != NULL) {

    for (x = 0; x < ALLOC_SOLUTIONS; x++) {
      if (board->solution[x] != NULL) {
	free (board->solution[x]);
      }
    }

    free (board->solvable_groups);
    free (board);

  }

}
