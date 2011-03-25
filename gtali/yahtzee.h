#ifndef _yahtzee_H_
#define _yahtzee_H_
/*
 * Gyatzee: Gnomified Yahtzee game.
 * (C) 1998 the Free Software Foundation
 *
 * File:   yahtzee.h
 *
 * Author: Scott Heavner
 *
 * This program is based on based on orest zborowski's curses based
 * yahtze (c)1992.
 */

#include <glib/gi18n.h>

#define COMPUTER_DELAY 1	/* sec */

#define NUM_ROLLS 3
#define NUMBER_OF_DICE       5

#define MAX_NUMBER_OF_PLAYERS 6
#define LAST_COL (MAX_NUMBER_OF_PLAYERS+2)
#define MAX_NAME_LENGTH 8
#define NUM_UPPER 6
#define NUM_LOWER_YAHTZEE 7
#define NUM_LOWER_KISMET  9
#define NUM_FIELDS_YAHTZEE (NUM_UPPER + NUM_LOWER_YAHTZEE)
#define NUM_FIELDS_KISMET  (NUM_UPPER + NUM_LOWER_KISMET)
#define MAX_FIELDS (NUM_UPPER + NUM_LOWER_KISMET)

#define EXTRA_FIELDS 4

/* Locations of fields containing totals */
#define F_LOWERT (NUM_FIELDS)
#define F_GRANDT (F_LOWERT+1)
#define F_UPPERT (F_GRANDT+1)
#define F_BONUS  (F_UPPERT+1)

#define H_2P (game_type == GAME_YAHTZEE ? -1 : 6)  /* 2 pair same color     */
#define H_3  (game_type == GAME_YAHTZEE ?  6 : 7)  /* 3 of a kind           */
#define H_4  (game_type == GAME_YAHTZEE ?  7 : 12) /* 4 of a kind           */
#define H_FH 8                                     /* Full house            */
#define H_FS (game_type == GAME_YAHTZEE ? -1 : 9)  /* Full house same color */
#define H_FL (game_type == GAME_YAHTZEE ? -1 : 10) /* Flush: all came color */
#define H_SS (game_type == GAME_YAHTZEE ?  9 : -1) /* Small straight        */
#define H_LS (game_type == GAME_YAHTZEE ? 10 : 11) /* Large straight        */
#define H_YA (game_type == GAME_YAHTZEE ? 11 : 13) /* 5 of a kind           */
#define H_CH (game_type == GAME_YAHTZEE ? 12 : 14) /* Chance                */

typedef struct {
  char *name;
  short used[MAX_FIELDS];
  int score[MAX_FIELDS];
  int finished;
  int comp;
} Player;

typedef struct {
  int val;
  int sel;
} DiceInfo;

typedef struct undo_score_element_t {
  int player;
  int field;
  int score;
  int DiceValues[NUMBER_OF_DICE];
  int roll;
} UndoScoreElement;

/* yahtzee.c */
extern DiceInfo DiceValues[];
extern Player players[];
extern int NumberOfPlayers;
extern int NumberOfHumans;
extern int NumberOfComputers;
extern int DoDelay;
extern int NumberOfRolls;
extern int LastHumanNumberOfRolls;
extern int WinningScore;
extern int DisplayComputerThoughts;
extern int OnlyShowScores;
extern int CurrentPlayer;
extern int NUM_FIELDS;
extern int NUM_LOWER;
extern char *ProgramHeader;
extern char **FieldLabels;
extern char *DefaultPlayerNames[MAX_NUMBER_OF_PLAYERS];

extern void YahtzeeInit (void);
extern void NewGame (void);
extern int upper_total (int num);
extern int lower_total (int num);
extern int total_score (int num);
extern int count (int val);
extern int find_n_of_a_kind (int n, int but_not);
extern int find_straight (int run, int notstart, int notrun);
extern int find_yahtzee (void);
extern int add_dice (void);
extern gint field_score (gint field);
extern gint player_field_score (gint player, gint field);
extern int play_score (int player, int field);
extern void handle_play (int player);
extern void play (void);
extern void calc_random (void);
extern void say (char *fmt, ...);
extern void SelectAllDice (void);
extern int NoDiceSelected (void);
extern int RollDie (void);
extern void RollSelectedDice (void);
extern int GameIsOver (void);
extern int FindWinner (void);
extern int UndoLastMove(void);
extern int RedoLastMove(void);
void PrependUndoList(gint player, gint field, gint score);
void FreeUndoList(void);
void FreeRedoList(void);
void FreeUndoRedoLists(void);
void FreeRedoListHead(void);
extern int  UndoPossible(void);
extern int  UndoVisible(void);
extern int  RedoPossible(void);
extern void ResetDiceState(UndoScoreElement *elem);
extern void RestoreLastRoll(void);
/* Computer.c */
extern void ComputerRolling (int player);
extern void ComputerScoring (int player);

/* Specific to a windowing system: gyahtzee.c/cyahtzee.c */
extern void NewGame (void);
extern void UpdateAllDicePixmaps (void);
extern void DeselectAllDice (void);
extern void ShowPlayer (int num, int field);
extern void DisplayCurrentPlayer(void);
extern void DisplayCurrentPlayerRefreshDice(void);
extern void NextPlayer (void);
extern void PreviousPlayer (void);
extern void RedoPlayer     (void);
extern void ShowHighScores (void);
extern UndoScoreElement *RedoHead(void);

enum { SCORE_OK = 0, SLOT_USED, PLAYER_DONE, YAHTZEE_NEWGAME };

typedef  enum { GAME_YAHTZEE = 0, GAME_KISMET } GameType;
extern GameType game_type;

#endif /* _yahtzee_H_ */


/* Arrgh - lets all use the same tabs under emacs: 
Local Variables:
tab-width: 8
c-basic-offset: 8
indent-tabs-mode: nil
End:
*/
