#ifndef _Gyahtzee_H_
#define _Gyahtzee_H_
/*
 * Gyatzee: Gnomified Yahtzee game.
 * (C) 1998 the Free Software Foundation
 *
 * File:   gyahtzee.h
 *
 * Author: Scott Heavner
 *
 */
#define SCOREROWS (NUM_FIELDS+5)

/* Screen row numbers containing totals */
#define R_UTOTAL (NUM_UPPER+1)
#define R_BONUS  (R_UTOTAL-1)
#define R_BLANK1 (R_UTOTAL+1)
#define R_GTOTAL (SCOREROWS-1)
#define R_LTOTAL (R_GTOTAL-1)

#define KEY_NUMBER_OF_COMPUTERS       "NumberOfComputerOpponents"
#define KEY_NUMBER_OF_HUMANS          "NumberOfHumanOpponents"
#define KEY_GAME_TYPE                 "GameType"
#define KEY_NUMTRIALS                 "MonteCarloTrials"
#define KEY_DELAY_BETWEEN_ROLLS       "DelayBetweenRolls"
#define KEY_DISPLAY_COMPUTER_THOUGHTS "DisplayComputerThoughts"
#define KEY_PLAYER_NAMES              "PlayerNames"

/* clist.c */
extern GtkWidget *create_score_list (void);
extern void setup_score_list (GtkWidget * scorelist);
extern void update_score_cell (GtkWidget * scorelist, int row, int col,
			       int val);
extern void ShowoffPlayerColumn (GtkWidget * scorelist, int player, int so);
extern void ShowoffPlayer (GtkWidget * scorelist, int player, int so);
extern void score_list_set_column_title (GtkWidget * scorelist, int column,
					 const char *str);
extern void update_score_tooltips (void);
/* setup.c */
extern gint setup_game (GtkAction * action, gpointer data);
extern void GRenamePlayer (gint playerno);
extern GameType game_type_from_string(const gchar *string);
extern GameType get_new_game_type(void);
extern void set_new_game_type(GameType type);

/* gyahtzee.c */
extern int GyahtzeeAbort;
extern GtkWidget *ScoreList;
extern void update_undo_sensitivity(void);
#endif /* _Gyahtzee_H_ */

/* Arrgh - lets all use the same tabs under emacs: 
Local Variables:
tab-width: 8
c-basic-offset: 8
indent-tabs-mode: nil
*/
