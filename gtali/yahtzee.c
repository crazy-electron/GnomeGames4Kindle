/*
 * Gyatzee: Gnomified Yahtzee game.
 * (C) 1998 the Free Software Foundation
 *
 * File:   yahtzee.c
 *
 * Author: Scott Heavner
 *
 *   Window manager independent yahtzee routines.
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <glib.h>

#include <config.h>
#include "yahtzee.h"

char *ProgramHeader = "Yahtzee Version 2.00 (c)1998 SDH, (c)1992 by zorst";

GList *UndoList = NULL;
GList *RedoList = NULL;
UndoScoreElement lastRoll;

/*=== Exported variables ===*/
DiceInfo DiceValues[NUMBER_OF_DICE] = { {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0} };
Player players[MAX_NUMBER_OF_PLAYERS] = {
  {NULL, {0}, {0}, 0, 0},
  {NULL, {0}, {0}, 0, 0},
  {NULL, {0}, {0}, 0, 0},
  {NULL, {0}, {0}, 0, 0},
  {NULL, {0}, {0}, 0, 0},
  {NULL, {0}, {0}, 0, 0}
};
int NumberOfPlayers = 0;
int NumberOfComputers = 0;
int NumberOfHumans = 0;
int DoDelay = 0;
int NumberOfRolls;
int LastHumanNumberOfRolls;
int WinningScore;
int DisplayComputerThoughts = 0;
int CurrentPlayer;
GameType game_type = GAME_YAHTZEE;
int NUM_FIELDS = NUM_FIELDS_YAHTZEE;
int NUM_LOWER = NUM_LOWER_YAHTZEE;

char *DefaultPlayerNames[MAX_NUMBER_OF_PLAYERS] = { N_("Human"),
  "Wilber",
  "Bill",
  "Monica",
  "Kenneth",
  "Janet"
};

typedef struct field_info_t {
  char *label;
  int   yahtzee_row;
  int   kismet_row;
  int (*score_func)(int);
} FieldInfo;

char *FieldLabelsYahtzee[NUM_FIELDS_YAHTZEE + EXTRA_FIELDS] = {
  N_("1s [total of 1s]"),
  N_("2s [total of 2s]"),
  N_("3s [total of 3s]"),
  N_("4s [total of 4s]"),
  N_("5s [total of 5s]"),
  N_("6s [total of 6s]"),
  /* End of upper panel */
  N_("3 of a Kind [total]"),
  N_("4 of a Kind [total]"),
  N_("Full House [25]"),
  N_("Small Straight [30]"),
  N_("Large Straight [40]"),
  N_("5 of a Kind [50]"),
  N_("Chance [total]"),
  /* End of lower panel */
  N_("Lower Total"),
  N_("Grand Total"),
  /* Need to squish between upper and lower pannel */
  N_("Upper total"),
  N_("Bonus if >62"),
};

char *FieldLabelsKismet[NUM_FIELDS_KISMET+EXTRA_FIELDS] =
{
  N_("1s [total of 1s]"),
  N_("2s [total of 2s]"),
  N_("3s [total of 3s]"),
  N_("4s [total of 4s]"),
  N_("5s [total of 5s]"),
  N_("6s [total of 6s]"),
  /* End of upper panel */
  N_("2 pair Same Color [total]"),
  N_("3 of a Kind [total]"),
  N_("Full House [15 + total]"),
  N_("Full House Same Color [20 + total]"),
  N_("Flush (all same color) [35]"),
  N_("Large Straight [40]"),
  N_("4 of a Kind [25 + total]"),
  N_("5 of a Kind [50 + total]"),
  N_("Chance [total]"),
  /* End of lower panel */
  N_("Lower Total"),
  N_("Grand Total"),
  /* Need to squish between upper and lower pannel */
  N_("Upper total"),
  N_("Bonus if >62"),
};

char **FieldLabels = FieldLabelsKismet;

int
NoDiceSelected (void)
{
  int i, j = 1;
  for (i = 0; i < NUMBER_OF_DICE; i++)
    if (DiceValues[i].sel)
      j = 0;
  return j;
}

void
SelectAllDice (void)
{
  int i;
  for (i = 0; i < NUMBER_OF_DICE; i++)
    DiceValues[i].sel = 1;
}

void
YahtzeeInit (void)
{
  int i;

  srand (time (NULL));

  for (i = 0; i < MAX_NUMBER_OF_PLAYERS; ++i) {
    players[i].name = _(DefaultPlayerNames[i]);
    players[i].comp = 1;
  }

  /* Make player number one human */
  players[0].comp = 0;

}

/* Must be called after window system is initted */
void
NewGame (void)
{
  int i, j;

  if (game_type == GAME_YAHTZEE) {
    FieldLabels = FieldLabelsYahtzee;
    NUM_FIELDS = NUM_FIELDS_YAHTZEE;
    NUM_LOWER = NUM_LOWER_YAHTZEE;
  }
  else if (game_type == GAME_KISMET) {
    FieldLabels = FieldLabelsKismet;
    NUM_FIELDS = NUM_FIELDS_KISMET;
    NUM_LOWER = NUM_LOWER_KISMET;
  }

  CurrentPlayer = 0;
  NumberOfRolls = 0;
  LastHumanNumberOfRolls = 0;
  FreeUndoRedoLists();

  NumberOfPlayers = NumberOfComputers + NumberOfHumans;

  for (i = 0; i < MAX_NUMBER_OF_PLAYERS; ++i) {
    players[i].finished = 0;
    players[i].comp = 1;

    for (j = 0; j < NUM_FIELDS; ++j) {
      players[i].score[j] = 0;
      players[i].used[j] = 0;
    }
  }

  /* Possibly 0 humans? */
  for (i = 0; i < NumberOfHumans; i++)
    players[i].comp = 0;

  SelectAllDice ();
  RollSelectedDice ();
}

int
RollDie (void)
{
  double r = (double) rand() / RAND_MAX;
  return (int)(r * 6.0) + 1;
}

void
RollSelectedDice (void)
{
  int i, cnt = 0;

  if (NumberOfRolls >= NUM_ROLLS) {
    return;
  }

  for (i = 0; i < NUMBER_OF_DICE; i++) {
    if (DiceValues[i].sel) {
      DiceValues[i].val = RollDie ();
      DiceValues[i].sel = 0;
      cnt++;
      lastRoll.DiceValues[i] = DiceValues[i].val;
    }
  }

  /* If no dice is selcted roll them all */
  if(cnt == 0){
    for (i = 0; i < NUMBER_OF_DICE; i++) {
      DiceValues[i].val = RollDie ();
      lastRoll.DiceValues[i] = DiceValues[i].val;
    }
  }

  UpdateAllDicePixmaps ();
  DeselectAllDice ();

  NumberOfRolls++;

  if (NumberOfRolls >= NUM_ROLLS) {
    say (_("Choose a score slot."));
  }

  lastRoll.roll = NumberOfRolls;
  lastRoll.player = CurrentPlayer;
}

int
GameIsOver (void)
{
  int i;

  for (i = 0; i < NumberOfPlayers; i++)
    if (players[i].finished == 0)
      return 0;
  return 1;
}

int
upper_total (int num)
{
  int val;
  int i;

  val = 0;

  for (i = 0; i < NUM_UPPER; ++i)
    val += players[num].score[i];

  return (val);
}

int
lower_total (int num)
{
  int val;
  int i;

  val = 0;

  for (i = 0; i < NUM_LOWER; ++i)
    val += players[num].score[i + NUM_UPPER];

  return (val);
}

int
total_score (int num)
{
  int upper_tot;
  int lower_tot;

  upper_tot = 0;
  lower_tot = 0;

  lower_tot = lower_total (num);
  upper_tot = upper_total (num);

  if (game_type == GAME_KISMET && upper_tot >= 78)
    upper_tot += 75;
  else if (game_type == GAME_KISMET && upper_tot >= 71)
    upper_tot += 55;
  else if (upper_tot >= 63)
    upper_tot += 35;

  return (upper_tot + lower_tot);
}

int
count (int val)
{
  int i;
  int num;

  num = 0;

  for (i = 0; i < NUMBER_OF_DICE; ++i)
    if (DiceValues[i].val == val)
      ++num;

  return (num);
}

int
find_n_of_a_kind (int n, int but_not)
{
  int i;

  for (i = 0; i < NUMBER_OF_DICE; ++i) {
    if (DiceValues[i].val == but_not)
      continue;

    if (count (DiceValues[i].val) >= n)
      return (DiceValues[i].val);
  }

  return (0);
}

int
find_straight (int run, int notstart, int notrun)
{
  int i;
  int j;

  for (i = 1; i < 7; ++i) {
    if (i >= notstart && i < notstart + notrun)
      continue;

    for (j = 0; j < run; ++j)
      if (!count (i + j))
	break;

    if (j == run)
      return (i);
  }

  return (0);
}

int
find_yahtzee (void)
{
  int i;

  for (i = 1; i < 7; ++i)
    if (count (i) == 5)
      return (i);

  return (0);
}

int
add_dice (void)
{
  int i;
  int val;

  val = 0;

  for (i = 0; i < NUMBER_OF_DICE; ++i)
    val += DiceValues[i].val;

  return (val);
}

static int
score_basic(int field) {
  field++;
  return count (field) * field;
}

static int
score_3_of_a_kind(int field) {
  if (find_n_of_a_kind (3, 0))
    return add_dice ();
  return 0;
}

static int
score_4_of_a_kind(int field) {
  if (find_n_of_a_kind (4, 0))
    return add_dice ();
  return 0;
}

static int
score_full_house(int field) {
  int i = find_n_of_a_kind (3, 0);
  if (i) {
    if (find_n_of_a_kind (2, i) || find_n_of_a_kind (5, 0))
      return 25;
  }

  return 0;
}

static int
score_small_straight(int field) {
  if (find_straight (4, 0, 0))
    return 30;

  return 0;
}

static int
score_large_straight(int field) {
  if (find_straight (5, 0, 0))
    return 40;

  return 0;
}

static int
score_yahtzee(int field) {
  if (find_n_of_a_kind (5, 0))
    return 50;

  return 0;
}

static int
score_chance(int field) {
  return add_dice ();
}

static int
score_2_pair_same_color(int field) {
  int i = find_n_of_a_kind (2, 0);
  if (i) {
     if (find_n_of_a_kind (2, i) + i == 7 || find_n_of_a_kind (4, 0))
       return add_dice ();
  }

  return 0;
}

static int
score_full_house_kismet(int field) {
  int i = find_n_of_a_kind (3, 0);
  if (i) {
    if (find_n_of_a_kind (2, i) || find_n_of_a_kind (5, 0))
      return 15 + add_dice ();
  }

  return 0;
}

static int
score_full_house_same_color(int field) {
  int i = find_n_of_a_kind (3, 0);
  if (i) {
    if (find_n_of_a_kind (2, i) + i == 7 || find_n_of_a_kind (5, 0))
      return 20 + add_dice ();
  }

  return 0;
}

static int
score_flush(int field) {
  int i = find_n_of_a_kind (3, 0);

  if (i && i + find_n_of_a_kind (2, i) == 7) return 35;
  i = find_n_of_a_kind (4, 0);
  if (i && i + find_n_of_a_kind (1, i) == 7) return 35;
  if (find_n_of_a_kind (5, 0)) return 35;

  return 0;
}

static int
score_4_of_a_kind_kismet(int field) {
  if (find_n_of_a_kind (4, 0))
    return 25 + add_dice ();
  return 0;
}

static int
score_kismet(int field) {
  if (find_n_of_a_kind (5, 0))
    return 50 + add_dice ();
  return 0;
}

FieldInfo field_table[] = { 
  { N_("1s [total of 1s]"),                   0,  0, score_basic },
  { N_("2s [total of 2s]"),                   1,  1, score_basic },
  { N_("3s [total of 3s]"),                   2,  2, score_basic },
  { N_("4s [total of 4s]"),                   3,  3, score_basic },
  { N_("5s [total of 5s]"),                   4,  4, score_basic },
  { N_("6s [total of 6s]"),                   5,  5, score_basic },
  { N_("3 of a Kind [total]"),                6,  7, score_3_of_a_kind },
  { N_("4 of a Kind [total]"),                7, -1, score_4_of_a_kind },
  { N_("Full House [25]"),                    8, -1, score_full_house },
  { N_("Small Straight [30]"),                9, -1, score_small_straight },
  { N_("Large Straight [40]"),               10, 11, score_large_straight },
  { N_("5 of a Kind [total]"),               11, -1, score_yahtzee },
  { N_("Chance [total]"),                    12, 14, score_chance },
  { N_("2 pair Same Color [total]"),         -1,  6, score_2_pair_same_color },
  { N_("Full House [15 + total]"),            -1,  8, score_full_house_kismet },
  { N_("Full House Same Color [20 + total]"), -1,  9, score_full_house_same_color },
  { N_("Flush (all same color) [35]"),       -1, 10, score_flush },
  { N_("4 of a Kind [25 + total]"),          -1, 12, score_4_of_a_kind_kismet },
  { N_("5 of a Kind [50 + total]"),          -1, 13, score_kismet },
};

#define FIELD_TABLE_SIZE (sizeof(field_table) / sizeof(FieldInfo))

static FieldInfo
*get_field_info(int field)
{
  gint ii;

  for (ii = 0; ii < FIELD_TABLE_SIZE; ii++)
    if (field == (game_type == GAME_KISMET ? field_table[ii].kismet_row :
                                             field_table[ii].yahtzee_row))
      return &field_table[ii];

  return NULL;
}

gint
field_score(gint field)
{
  FieldInfo *info = get_field_info(field);
  gint rval = 0;

  if (info) {
    return info->score_func(field);
  }

  return rval;
}

gint
player_field_score(gint player, gint field)
{
  /* A player can still score in H_YA even if it's used in the
   * regular game, but only if they have a non-zero value there */
  if (field == H_YA && game_type == GAME_YAHTZEE) {
    if (players[player].used[field]) {
      if (field_score(field) > 0 && players[player].score[field] > 0)
        return field_score(field);
      else
        return -1;
    }
  }
  else if (players[player].used[field])
    return -1;

  return field_score (field);
}

void
PrependUndoList(gint player, gint field, gint score) {
  UndoScoreElement *elem = g_new0(UndoScoreElement, 1);
  gint ii;
  elem->player = player;
  elem->field  = field;
  elem->score  = score;

  for (ii = 0; ii < NUMBER_OF_DICE; ii++)
    elem->DiceValues[ii] = DiceValues[ii].val;
  elem->roll = NumberOfRolls;

  if (!players[player].comp)
    FreeUndoList();
  UndoList = g_list_prepend(UndoList, elem);
}

void
ResetDiceState(UndoScoreElement *elem) {
  gint ii;
  for (ii = 0; ii < NUMBER_OF_DICE; ii++) {
    DiceValues[ii].val = elem->DiceValues[ii];
    DiceValues[ii].sel = 0;
  }
  NumberOfRolls = elem->roll;
  ShowPlayer (elem->player, elem->field);
}

gint
UndoLastMove(void) {
  if (UndoList) {
    UndoScoreElement *elem = UndoList->data;
    if (elem->field == H_YA && game_type == GAME_YAHTZEE) {
      if (players[elem->player].score[elem->field] != 0)
        players[elem->player].score[elem->field] -= 50;
      players[elem->player].used [elem->field] = players[elem->player].score[elem->field] > 0;
    } else {
      players[elem->player].score[elem->field] = 0;
      players[elem->player].used [elem->field] = 0;
    }

    ResetDiceState(elem);
    UndoList = g_list_remove(UndoList, elem);
    RedoList = g_list_prepend(RedoList, elem);
    return elem->player;
  }

  return CurrentPlayer;
}

gint
RedoLastMove(void) {
  gint rval = (CurrentPlayer + 1) % NumberOfPlayers;
  if (RedoList) {
    gint ii;
    UndoScoreElement *elem = RedoList->data;

    for (ii = 0; ii < NUMBER_OF_DICE; ii++) {
      DiceValues[ii].val = elem->DiceValues[ii];
      DiceValues[ii].sel = 0;
    }

    RedoList = g_list_remove(RedoList, elem);
    play_score(elem->player, elem->field);
    rval = (elem->player + 1) % NumberOfPlayers;
    g_free(elem);
    if (RedoList) {
      elem = RedoList->data;
      rval = elem->player;
    }
  }

  return rval;
}

void
RestoreLastRoll(void) {
  ResetDiceState(&lastRoll);
}

UndoScoreElement*
RedoHead(void) {
  if (RedoList) {
    UndoScoreElement *elem = RedoList->data;
    return elem;
  }

  return NULL;
}

void
FreeUndoList(void) {
  while (UndoList) {
    UndoScoreElement *elem = UndoList->data;
    UndoList = g_list_remove(UndoList, elem);
    g_free(elem);
  }
}

void
FreeRedoList(void) {
  while (RedoList) {
    UndoScoreElement *elem = RedoList->data;
    RedoList = g_list_remove(RedoList, elem);
    g_free(elem);
  }
}

void
FreeUndoRedoLists(void) {
  FreeUndoList();
  FreeRedoList();
}

void
FreeRedoListHead(void) {
  if (RedoList) {
    UndoScoreElement *elem = RedoList->data;
    RedoList = g_list_remove(RedoList, elem);
    g_free(elem);
  }
}

/* Test if we can use suggested score slot */
int
play_score (int player, int field)
{
  int i;

  /* Special case for yahtzee, allow multiple calls if 1st wasn't 0 */

  /* This, however, was broken: it didn't actually check to see if the
   * user had a yahtzee if this wasn't their first time clicking on it.
   * Bad. -- pschwan@cmu.edu */
  if (field == 11 && game_type == GAME_YAHTZEE) {
    if ((players[player].used[11] && (players[player].score[11] == 0)))
      return SLOT_USED;

    if ((players[player].used[11] && !find_yahtzee ()))
      return SLOT_USED;
  } else if (players[player].used[field])
    return SLOT_USED;

  players[player].used[field] = 1;
  players[player].score[field] = players[player].score[field] + field_score(field);
  PrependUndoList(player, field, players[player].score[field]);

  ShowPlayer (player, field);

  for (i = 0; i < NUM_FIELDS; ++i)
    if (!players[player].used[i])
      return SCORE_OK;

  players[player].finished = 1;

  return SCORE_OK;
}

int
FindWinner (void)
{
  int i;
  int winner = 0;
  int total;

  WinningScore = 0;
  FreeUndoRedoLists();

  for (i = 0; i < NumberOfPlayers; ++i) {
    total = total_score (i);
    if (total > WinningScore) {
      WinningScore = total_score (i);
      winner = i;
    }
  }

  /* Detect a drawn game. Returning the negative of the score 
   * is a bit of a hack, but it allows us to find out who the winners
   * were without having to pass around a list. */
  for (i = 0; i < NumberOfPlayers; ++i) {
    total = total_score (i);
    if ((total == WinningScore) && (i != winner))
      return -total;
  }

  return winner;
}

/* Undo is possible when the Undo List isn't NULL */
int
UndoPossible(void)
{
  return UndoList != NULL;
}

/* Undo option should be visible only when the player is human */
int
UndoVisible(void)
{
  return UndoPossible() && !players[CurrentPlayer].comp;
}

int
RedoPossible(void)
{
  return RedoList != NULL;
}

/* Arrgh - lets all use the same tabs under emacs: 
Local Variables:
tab-width: 8
c-basic-offset: 8
indent-tabs-mode: nil
*/
