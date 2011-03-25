/*
 * Gyatzee: Gnomified Yahtzee game.
 * (C) 1998 the Free Software Foundation
 *
 * File:   computer.c
 *
 * Author: Scott Heavner
 *
 *   Window manager independent yahtzee routines for computer opponents.
 *
 *   Variables are exported in yahtzee.h
 *
 *   This program is based on based on orest zborowski's curses based
 *   yahtze (c)1992.
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
#include <stdio.h>
#include <string.h>
#include <config.h>
#include "yahtzee.h"


/*
**	questions are:
**		0:	none
**		1:	"what dice to roll again?"
**		2:	"where do you want to put that?"
*/

static int bc_table[MAX_FIELDS];
extern int NUM_TRIALS;

static void
BuildTable (int player)
{
  int i;
  int d;

  for (i = 0; i < NUM_FIELDS; ++i) {
    bc_table[i] = 0;
    if (players[player].used[i]) {
      bc_table[i] = -99;
    }
  }

/*
**	HANDLING UPPER SLOTS
*/

  for (i = 0; i < NUM_UPPER; ++i) {
    if (players[player].used[i])
      continue;

/*
**	ok. now we set a base value on the roll based on its count and
**	how much it is worth to us.
*/
    bc_table[i] = (count(i + 1) - 2) * (i + 1) * 4 - (i + 1);
  }

/*
**	HANDLING LOWER SLOTS
*/

/*
**	now we look hard at what we got
*/

  /* Small straight */
  if (H_SS > 0 && !players[player].used[H_SS]) {
    d = field_score (H_SS);
    if (d) bc_table[H_SS] = d;
  }

  /* Large straight */
  if (!players[player].used[H_LS]) {
    bc_table[H_LS] = field_score (H_LS);
  }

  /* chance - sum of all dice */
  /* Lower this, because we'd rather score somewhere else when we can */
  if (!players[player].used[H_CH] && NumberOfRolls > 2) {
    bc_table[H_CH] = field_score (H_CH) / 2;
  }

  /* Full house */
  if (!players[player].used[H_FH]) {
    bc_table[H_FH]  = field_score (H_FH);
  }

  /* Two pair same color */
  if (H_2P > 0 && !players[player].used[H_2P]) {
    bc_table[H_2P] = field_score (H_2P);
  }

  /* Full house same color */
  if (H_FS > 0 && !players[player].used[H_FS]) {
    bc_table[H_FS]  = field_score (H_FS);
  }

  /* Flush - all same color */
  if (H_FL > 0 && !players[player].used[H_FL]) {
    bc_table[H_FL]  = field_score (H_FL);
  }

  /* 3 of a kind */
  if (!players[player].used[H_3]) {
    bc_table[H_3]  = field_score (H_3);
  }

  /* 4 of a kind */
  if (!players[player].used[H_4]) {
    /* Add one to break tie with 3 of a kind */
    bc_table[H_4] = field_score (H_4) + 1;
  }

  /* 5 of a kind */

  if (players[player].used[H_YA] && (players[player].score[H_YA] == 0 || game_type == GAME_KISMET)) {
      bc_table[H_YA] = -99;
  }
  else if (find_n_of_a_kind (5, 0)) {
      bc_table[H_YA] = 150;	/* so he will use it! */
  }

  if (DisplayComputerThoughts) {
    for (i = 0; i < NUM_FIELDS; ++i) {
      printf ("%s : SCORE = %d\n", _(FieldLabels[i]), bc_table[i]);
    }
  }
}

/*
**	The idea here is to use a Monte Carlo simulation.
**	For each possible set of dice to roll, try NUM_TRIALS random rolls,
**	and average the scores. The highest average score will be the set
**	that we decide to roll.
**	Currently, this ignores the number of rolls a player has left.
**  We could try to do a second set of trials when there are two rolls
**  left, but that would take a lot more CPU time.
*/

void
ComputerRolling (int player)
{
  int i;
  int best;
  int bestv;
  int num_options = 1;
  int die_comp[NUMBER_OF_DICE];
  double best_score;
  int ii, jj, kk;

  for (i = 0; i < NUMBER_OF_DICE; i++) {
      die_comp[i] = num_options;
      num_options *= 2;
  }

  best = 0;
  bestv = -99;
  {
  double avg_score[num_options];
  DiceInfo sav_DiceValues[NUMBER_OF_DICE];
  memset(avg_score, 0, sizeof(avg_score));
  memcpy(sav_DiceValues, DiceValues, sizeof(sav_DiceValues));

  for (ii = 0; ii < num_options; ii++) {
      for (jj = 0; jj < NUM_TRIALS; jj++) {
          DiceInfo loc_info[NUMBER_OF_DICE];
          memcpy(loc_info, sav_DiceValues, sizeof(loc_info));
          for (kk = 0; kk < NUMBER_OF_DICE; kk++) {
              if (die_comp[kk] & ii) {
                  loc_info[kk].val = RollDie();
              }
          }
          memcpy(DiceValues, loc_info, sizeof(sav_DiceValues));
          BuildTable(player);
          bestv = -99;
          best  = 0;
          for (i = NUM_FIELDS - 1; i >= 0; i--) {
              if (bc_table[i] >= bestv) {
                  best = i;
                  bestv = bc_table[i];
              }
          }
          avg_score[ii] += bestv;
      }
      avg_score[ii] /= NUM_TRIALS;
      if (DisplayComputerThoughts) printf ("Set avg score for %d to %f\n", ii, avg_score[ii]);
  }

  best  = 0;
  best_score = -99;
  for (ii = 0; ii < num_options; ii++) {
      if (avg_score[ii] >= best_score) {
          best = ii;
          best_score = avg_score[ii];
      }
  }
  if (DisplayComputerThoughts) printf("Best choice is %d val %f for dice ", best, best_score);

  /* Restore DiceValues */
  memcpy(DiceValues, sav_DiceValues, sizeof(sav_DiceValues));
  for (ii = 0; ii < NUMBER_OF_DICE; ii++) {
      if (die_comp[ii] & best) {
          DiceValues[ii].sel = 1;
          if (DisplayComputerThoughts) printf("Reset to roll die %d value %d bit %d comp %d test %d\n", ii, DiceValues[ii].val, ii, best, ii & best);
      }
      else {
          DiceValues[ii].sel = 0;
          if (DisplayComputerThoughts) printf("Reset NOT to roll die %d value %d bit %d comp %d test %d\n", ii, DiceValues[ii].val, ii, best, ii & best);
      }
  }
  }
}

/*
**	what we do here is generate a table of all the choices then choose
**	the highest value one.  in case of a tie, we go for the lower choice
**	cause higher choices tend to be easier to better
*/

void
ComputerScoring (int player)
{
  int i;
  int best;
  int bestv;

  NumberOfRolls = 3;		/* in case skipped middle */

  BuildTable (player);

  best = 0;

  bestv = -99;

  for (i = NUM_FIELDS - 1; i >= 0; --i) {
    if (player % 2) {
      if (bc_table[i] > bestv) {
	best = i;

	bestv = bc_table[i];
      }
    }

    else {
      if (bc_table[i] >= bestv) {
	best = i;

	bestv = bc_table[i];
      }
    }

    if (DisplayComputerThoughts) {
      fprintf (stderr, "<<BEST>> %s : VALUE = %d\n",
	       _(FieldLabels[best]),
	       bc_table[best]);
    }

  }

  if (DisplayComputerThoughts)
      fprintf(stderr, "I choose category %d as best %d score name %s\n",
              best, bc_table[best], _(FieldLabels[best]));

  play_score (player, best);

}

/* Arrgh - lets all use the same tabs under emacs: 
Local Variables:
tab-width: 8
c-basic-offset: 8
indent-tabs-mode: nil
*/
