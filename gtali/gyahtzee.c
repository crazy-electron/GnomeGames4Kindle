/* -*- mode:C; indent-tabs-mode:nil; tab-width:8; c-basic-offset:8 -*- */

/*
 * Gyatzee: Gnomified Dice game.
 * (C) 1998 the Free Software Foundation
 *
 * File:   gyahtzee.c
 *
 * Author: Scott Heavner
 *
 *   Gnome specific yahtzee routines.
 *
 *   Other gnome specific code is in setup.c and clist.c
 *
 *   Window manager independent routines are in yahtzee.c and computer.c
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
#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgames-support/games-help.h>
#include <libgames-support/games-stock.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-scores-dialog.h>
#include <libgames-support/games-conf.h>
#include <libgames-support/games-runtime.h>

#include "yahtzee.h"
#include "gyahtzee.h"

#define DELAY_MS 600

static char *appID = "gtali";
static char *appName = N_("Tali");
static guint last_timeout = 0;
static gboolean ready_to_advance_player;

#define NUMBER_OF_PIXMAPS    7
#define GAME_TYPES 2
#define DIE_SELECTED_PIXMAP  (NUMBER_OF_PIXMAPS-1)
#define SCORES_CATEGORY (game_type == GAME_KISMET ? "Colors" : NULL)

static char *dicefiles[NUMBER_OF_PIXMAPS] = { "gnome-dice-1.svg",
  "gnome-dice-2.svg",
  "gnome-dice-3.svg",
  "gnome-dice-4.svg",
  "gnome-dice-5.svg",
  "gnome-dice-6.svg",
  "gnome-dice-none.svg"
};

static char *kdicefiles[NUMBER_OF_PIXMAPS] = { "kismet1.svg",
  "kismet2.svg",
  "kismet3.svg",
  "kismet4.svg",
  "kismet5.svg",
  "kismet6.svg",
  "kismet-none.svg"
};

static GtkWidget *dicePixmaps[NUMBER_OF_DICE][NUMBER_OF_PIXMAPS][GAME_TYPES];

GtkWidget *window;
GtkWidget *ScoreList;
static GtkWidget *statusbar;
static GtkToolItem *diceBox[NUMBER_OF_DICE];
static GtkWidget *rollLabel;
static GtkWidget *mbutton;
static GtkAction *scores_action;
static GtkAction *undo_action;
static gchar *game_type_string = NULL;
static gint   test_computer_play = 0;
gint NUM_TRIALS = 0;

static const GOptionEntry yahtzee_options[] = {
  {"delay", 'd', 0, G_OPTION_ARG_NONE, &DoDelay,
   N_("Delay computer moves"), NULL},
  {"thoughts", 't', 0, G_OPTION_ARG_NONE, &DisplayComputerThoughts,
   N_("Display computer thoughts"), NULL},
  {"computers", 'n', 0, G_OPTION_ARG_INT, &NumberOfComputers,
   N_("Number of computer opponents"), N_("NUMBER")},
  {"humans", 'p', 0, G_OPTION_ARG_INT, &NumberOfHumans,
   N_("Number of human opponents"), N_("NUMBER")},
  {"game", 'g', 0, G_OPTION_ARG_STRING, &game_type_string,
   N_("Game choice: Regular or Colors"), N_("STRING")},
  {"computer-test", 'c', 0, G_OPTION_ARG_INT, &test_computer_play,
   N_("Number of computer-only games to play"), N_("NUMBER")},
  {"monte-carlo-trials", 'm', 0, G_OPTION_ARG_INT, &NUM_TRIALS,
   N_("Number of trials for each roll for the computer"), N_("NUMBER")},
  {NULL}
};

static const GamesScoresCategory category_array[] = {
  {"Regular", NC_("game type", "Regular") },
  {"Colors",  NC_("game type", "Colors")}
};

GamesScores *highscores;

static GtkWidget *dialog = NULL;

static gint modify_dice (GtkWidget * widget, gpointer data);
static gint roll_dice (GtkWidget * widget, GdkEvent * event,
			     gpointer data);
static void UpdateRollLabel (void);

static void
update_roll_button_sensitivity (void)
{
  gboolean state = FALSE;
  gint i;

  for (i = 0; i < NUMBER_OF_DICE; i++)
    state |=
      gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (diceBox[i]));

  if(!state){
    gtk_button_set_label (GTK_BUTTON (mbutton), _("Roll all!"));
    state = TRUE;
  } else {
    gtk_button_set_label (GTK_BUTTON (mbutton), _("Roll!"));
    state = TRUE;
  }

  state &= NumberOfRolls < 3;
  state &= !players[CurrentPlayer].comp;

  if(GameIsOver ()){
    state = FALSE;
  }

  gtk_widget_set_sensitive (GTK_WIDGET (mbutton), state);
}

static void
CheerWinner (void)
{
  int winner;
  int i;
  gint pos;
  gchar *message;

  ShowoffPlayer (ScoreList, CurrentPlayer, 0);

  winner = FindWinner ();

  /* draw. The score is returned as a negative value */
  if (winner < 0) {
    for (i = 0; i < NumberOfPlayers; i++) {
      if (total_score (i) == -winner) {
	ShowoffPlayer (ScoreList, i, 1);
      }
    }

    say (_("The game is a draw!"));
    return;
  }

  ShowoffPlayer (ScoreList, winner, 1);

  if (winner < NumberOfHumans) {
    pos = games_scores_add_plain_score (highscores, (guint32) WinningScore);

    if (pos > 0) {
      games_scores_update_score (highscores, players[winner].name);
      if (dialog) {
        gtk_window_present (GTK_WINDOW (dialog));
      } else {
        dialog = games_scores_dialog_new (GTK_WINDOW (window), highscores, _("Tali Scores"));
        message =
  	  g_strdup_printf ("<b>%s</b>\n\n%s", _("Congratulations!"),
                           pos == 1 ? _("Your score is the best!") :
                           _("Your score has made the top ten."));
        games_scores_dialog_set_message (GAMES_SCORES_DIALOG (dialog), message);
        g_free (message);
      }
      games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (dialog), pos);

      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_hide (dialog);
    }
  }

  if (players[winner].name)
    say (ngettext ("%s wins the game with %d point",
		   "%s wins the game with %d points", WinningScore),
	 players[winner].name, WinningScore);
  else
    say (_("Game over!"));

}
static gboolean
do_computer_turns (void)
{
  if (!players[CurrentPlayer].comp) {
    last_timeout = 0;
    return FALSE;
  }

  if (ready_to_advance_player) {
    NextPlayer ();
    return TRUE;
  }

  if (players[CurrentPlayer].finished) {
    NextPlayer ();
    return TRUE;
  }

  ComputerRolling (CurrentPlayer);
  if (NoDiceSelected () || (NumberOfRolls >= NUM_ROLLS)) {
    ComputerScoring (CurrentPlayer);
    ready_to_advance_player = TRUE;
  } else {
    RollSelectedDice ();
    UpdateRollLabel ();
  }

  if (!DoDelay)
    do_computer_turns ();

  return TRUE;
}

/* Show the current score and prompt for current player state */

void
DisplayCurrentPlayer(void) {
  ShowoffPlayer (ScoreList, CurrentPlayer, 1);

  if (players[CurrentPlayer].name) {
    if (players[CurrentPlayer].comp) {
      say (_("Computer playing for %s"), players[CurrentPlayer].name);
    } else {
      say (_("%s! -- You're up."), players[CurrentPlayer].name);
    }
  }
}

/* Display current player and refresh dice/display */
void
DisplayCurrentPlayerRefreshDice(void) {
  DisplayCurrentPlayer();
  UpdateAllDicePixmaps ();
  DeselectAllDice ();
  UpdateRollLabel();
}

void
NextPlayer (void)
{
  if (GameIsOver ()) {
    if (last_timeout) {
      g_source_remove (last_timeout);
      last_timeout = 0;
    }
    if (DoDelay && NumberOfComputers > 0)
        NumberOfRolls = NUM_ROLLS;
    else
        NumberOfRolls = LastHumanNumberOfRolls;
    /* update_roll_button_sensitivity() needs to be called in
       this context however UpdateRollLabel() also calls that method */
    UpdateRollLabel ();
    CheerWinner ();
    return;
  }

  NumberOfRolls = 0;
  ready_to_advance_player = FALSE;
  ShowoffPlayer (ScoreList, CurrentPlayer, 0);

  /* Find the next player with rolls left */
  do {
    CurrentPlayer = (CurrentPlayer + 1) % NumberOfPlayers;
  } while (players[CurrentPlayer].finished);

  DisplayCurrentPlayer();
  SelectAllDice ();
  RollSelectedDice ();
  FreeRedoList();

  /* Remember the roll count if this turn is for a
     human player for display at the end of the game */
  if (!players[CurrentPlayer].comp)
    LastHumanNumberOfRolls = NumberOfRolls;

  if (players[CurrentPlayer].comp) {
    if (DoDelay) {
      if (!last_timeout)
	last_timeout = g_timeout_add (DELAY_MS,
				      (GSourceFunc) do_computer_turns, NULL);
    } else {
      do_computer_turns ();
    }
  }
  /* Only update the roll label if we are running in
    delay mode or if this turn is for a human player */
  if (DoDelay || (!players[CurrentPlayer].comp))
    UpdateRollLabel ();
}

/* Go back to the previous player */

void
PreviousPlayer(void)
{
  if (UndoPossible()) {
    NumberOfRolls = 1;
    ready_to_advance_player = FALSE;
    ShowoffPlayer (ScoreList, CurrentPlayer, 0);

    /* Find the next player with rolls left */
    do {
      CurrentPlayer = (UndoLastMove() + NumberOfPlayers) % NumberOfPlayers;
    } while (players[CurrentPlayer].comp && UndoPossible());

    DisplayCurrentPlayerRefreshDice();
  }
}

void
RedoPlayer(void)
{
  if (RedoPossible()) {
    NumberOfRolls = 1;
    ready_to_advance_player = FALSE;
    ShowoffPlayer(ScoreList, CurrentPlayer, 0);
    /* The first element of the list is the undone turn, so
     * we need to remove it from the list before redoing other
     * turns.                                                    */
    FreeRedoListHead();

    /* Redo all computer players */
    do {
      CurrentPlayer = RedoLastMove();
    } while (players[CurrentPlayer].comp && RedoPossible());

    RestoreLastRoll();
    DisplayCurrentPlayerRefreshDice();
  }
}

void
ShowPlayer (int num, int field)
{
  int i;
  int line;
  int upper_tot;
  int lower_tot;
  int bonus = -1;
  int score;

  for (i = 0; i < NUM_FIELDS; ++i) {

    if (i == field || field == -1) {

      line = i;

      if (i >= NUM_UPPER)
	line += 3;

      if (players[num].used[i])
	score = players[num].score[i];
      else
	score = -1;

      if (test_computer_play == 0) update_score_cell (ScoreList, line, num + 1, score);
    }
  }

  upper_tot = upper_total (num);
  lower_tot = lower_total (num);

  if (upper_tot >= 63) {
    bonus = 35;
    if (game_type == GAME_KISMET) {
      if (upper_tot >= 78)
        bonus = 75;
      else if (upper_tot >= 71)
        bonus = 55;
    }
    upper_tot += bonus;
  }

  if (test_computer_play == 0) {
      update_score_cell (ScoreList, R_BONUS, num + 1, bonus);
      update_score_cell (ScoreList, R_UTOTAL, num + 1, upper_tot);
      update_score_cell (ScoreList, R_LTOTAL, num + 1, lower_tot);
      update_score_cell (ScoreList, R_GTOTAL, num + 1, upper_tot + lower_tot);
  }
}

static gint
quit_game (GObject * object, gpointer data)
{
  gtk_main_quit ();
  return TRUE;
}


/* This handles the keys 1..5 for the dice. */
static gint
key_press (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  gint offset;

  offset = event->keyval - GDK_KEY_1;

  if ((offset < 0) || (offset >= NUMBER_OF_DICE)) {
    return FALSE;
  }

  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (diceBox[offset]),
				     !DiceValues[offset].sel);

  return FALSE;
}

static void
GyahtzeeNewGame (void)
{
  int i;

  say (_("Select dice to roll or choose a score slot."));

  game_type = get_new_game_type();
  games_scores_set_category(highscores, SCORES_CATEGORY);
  NewGame ();
  setup_score_list (ScoreList);
  UpdateRollLabel ();

  for (i = 0; i < NumberOfPlayers; i++)
    ShowoffPlayer (ScoreList, i, 0);
  ShowoffPlayer (ScoreList, 0, 1);
}


static gint
new_game_callback (GtkAction * action, gpointer data)
{
  GyahtzeeNewGame ();
  return FALSE;
}

static void
UpdateRollLabel (void)
{
  static GString *str = NULL;

  if (!str)
    str = g_string_sized_new (22);

  g_string_printf (str, "<b>%s %d/3</b>", _("Roll"), NumberOfRolls);
  gtk_label_set_label (GTK_LABEL (rollLabel), str->str);

  update_score_tooltips ();
  update_roll_button_sensitivity ();
  update_undo_sensitivity ();
}

static void
UpdateDiePixmap (int n, int prev_game_type)
{
  static int last_val[NUMBER_OF_DICE] = { 0 };

  gtk_widget_hide (dicePixmaps[n][last_val[n]][prev_game_type]);

  last_val[n] = DiceValues[n].sel ? DIE_SELECTED_PIXMAP :
    DiceValues[n].val - 1;
  gtk_widget_show (dicePixmaps[n][last_val[n]][game_type]);
}

void
UpdateAllDicePixmaps (void)
{
  int i;
  static int prev_game_type = 0;

  for (i = 0; test_computer_play == 0 && i < NUMBER_OF_DICE; i++) {
    UpdateDiePixmap (i, prev_game_type);
  }
  prev_game_type = game_type;
}

void
DeselectAllDice (void)
{
  int i;

  if (test_computer_play > 0) return;
  for (i = 0; i < NUMBER_OF_DICE; i++)
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (diceBox[i]),
				       FALSE);
}

/* Callback on dice press */
gint
modify_dice (GtkWidget * widget, gpointer data)
{
  DiceInfo *tmp = (DiceInfo *) data;
  GtkToggleToolButton *button = GTK_TOGGLE_TOOL_BUTTON (widget);

  /* Don't modify dice if player is marked finished or computer is playing */
  if (players[CurrentPlayer].finished || players[CurrentPlayer].comp) {
    if (gtk_toggle_tool_button_get_active (button))
      gtk_toggle_tool_button_set_active (button, FALSE);
    return TRUE;
  }

  if (NumberOfRolls >= NUM_ROLLS) {
    say (_("You are only allowed three rolls. Choose a score slot."));
    gtk_toggle_tool_button_set_active (button, FALSE);
    return TRUE;
  }

  tmp->sel = gtk_toggle_tool_button_get_active (button);

  UpdateAllDicePixmaps ();

  update_roll_button_sensitivity ();
  return TRUE;
}


/* Callback on Roll! button press */
gint
roll_dice (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  if (!players[CurrentPlayer].comp) {
    RollSelectedDice ();
    if (NumberOfRolls > 1)
        FreeUndoRedoLists();
    UpdateRollLabel ();
    LastHumanNumberOfRolls = NumberOfRolls;
  }
  return FALSE;
}

void
say (char *fmt, ...)
{
  va_list ap;
  char buf[200];
  guint context_id;

  if (test_computer_play > 0) return;
  va_start (ap, fmt);
  g_vsnprintf (buf, 200, fmt, ap);
  va_end (ap);

  context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "message");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), context_id);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_id, buf);
}


static gint
about_cb (GtkAction * action, gpointer data)
{
  const gchar *authors[] = {
    N_("GNOME version (1998):"),
    "Scott Heavner",
    "",
    N_("Console version (1992):"),
    "Orest Zborowski",
    "",
    N_("Colors game and multi-level AI (2006):"),
    "Geoff Buchan",
    NULL
  };

  const gchar *documenters[] = {
    "Scott D Heavner",
    "Callum McKenzie",
    NULL
  };
  gchar *license = games_get_license (appName);

  gtk_show_about_dialog (GTK_WINDOW (window),
			 "name", appName,
			 "version", VERSION,
			 "copyright", "Copyright \xc2\xa9 1998-2008 "
			 "Free Software Foundation, Inc.",
			 "license", license,
			 "comments", _("A variation on poker with "
				       "dice and less money.\n\n"
				       "Tali is a part of GNOME Games."),
			 "authors", authors,
			 "documenters", documenters,
			 "translator-credits", _("translator-credits"),
			 "logo-icon-name", "gnome-tali",
			 "website",
			 "http://www.gnome.org/projects/gnome-games/",
			 "website-label", _("GNOME Games web site"),
			 "wrap-license", TRUE, NULL);
  g_free (license);

  return FALSE;
}

void
ShowHighScores (void)
{
  if (!dialog)
    dialog = games_scores_dialog_new (GTK_WINDOW (window), highscores, _("Tali Scores"));

  gtk_dialog_run (GTK_DIALOG (dialog));    
  gtk_widget_hide (dialog);
}

static gint
score_callback (GtkAction * action, gpointer data)
{
  ShowHighScores ();
  return FALSE;
}

static gint undo_callback(GtkAction *action, gpointer data)
{
  PreviousPlayer();
  return FALSE;
}

static void
LoadDicePixmaps (void)
{
  int i, j;
  char *path, *path_kismet;
  const char *dir;
  GError *error = NULL;

  dir = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);

  for (i = 0; i < NUMBER_OF_PIXMAPS; i++) {
    /* This is not efficient, but it lets us load animated types,
     * there is no way for us to copy a general GtkImage (the old 
     * code had a way for static images). */
    path = g_build_filename (dir, dicefiles[i], NULL);
    path_kismet = g_build_filename (dir, kdicefiles[i], NULL);

    if (g_file_test (path, G_FILE_TEST_EXISTS) && 
          g_file_test (path_kismet, G_FILE_TEST_EXISTS)) {

      for (j = 0; j < NUMBER_OF_DICE; j++) {
        GdkPixbuf *pixbuf;

        pixbuf = gdk_pixbuf_new_from_file_at_size (path, 60, 60, &error);
        dicePixmaps[j][i][GAME_YAHTZEE] = gtk_image_new_from_pixbuf (pixbuf);
        if (error) {
          g_warning ("Loading dice image %s: %s", path, error->message);
          g_clear_error (&error);
        }
        g_object_unref (pixbuf);

        pixbuf = gdk_pixbuf_new_from_file_at_size (path_kismet, 60, 60, &error);
        dicePixmaps[j][i][GAME_KISMET] = gtk_image_new_from_pixbuf (pixbuf);
        if (error) {
          g_warning ("Loading dice image %s: %s", path_kismet, error->message);
          g_clear_error (&error);
        }
        g_object_unref (pixbuf);
      }

    } /* FIXME: What happens if the file isn't found. */
    g_free(path);
    g_free(path_kismet);
  }
}

void
update_undo_sensitivity (void)
{
  gtk_action_set_sensitive(undo_action, UndoVisible());
}

static void
help_cb (GtkAction * action, gpointer data)
{
  games_help_display (window, "gtali", NULL);
}


static const GtkActionEntry action_entry[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL,
   G_CALLBACK (new_game_callback)},
  {"Undo", GAMES_STOCK_UNDO_MOVE, NULL, NULL, NULL,
   G_CALLBACK (undo_callback)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL,
   G_CALLBACK (score_callback)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_game)},
  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (setup_game)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (help_cb)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_cb)},
  /* Roll is just an accelerator */
  {"Roll", GTK_STOCK_REFRESH, NULL, "r", NULL, G_CALLBACK (roll_dice)}
};


static const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <menuitem action='Undo'/>"
  "      <menuitem action='Scores'/>"
  "      <menuitem action='Quit'/>"
  "    </menu>"
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='Preferences'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>"
  "    </menu>" "  </menubar>" "  <accelerator action='Roll' />" "</ui>";


static void
create_menus (GtkUIManager * ui_manager)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new ("group");

  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entry,
				G_N_ELEMENTS (action_entry), window);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);
  scores_action = gtk_action_group_get_action (action_group, "Scores");
  undo_action   = gtk_action_group_get_action (action_group, "Undo");
  update_undo_sensitivity();
}


static void
GyahtzeeCreateMainWindow (void)
{
  GtkWidget *hbox, *vbox;
  GtkWidget *toolbar;
  GtkWidget *tmp;
  GtkWidget *dicebox;
  GtkWidget *menubar;
  GtkAccelGroup *accel_group;
  GtkUIManager *ui_manager;
  int i, j;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), _(appName));

  games_conf_add_window (GTK_WINDOW (window), NULL);

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (quit_game), NULL);
  g_signal_connect (G_OBJECT (window), "key_press_event",
		    G_CALLBACK (key_press), NULL);

  statusbar = gtk_statusbar_new ();
  ui_manager = gtk_ui_manager_new ();

  games_stock_prepare_for_statusbar_tooltips (ui_manager, statusbar);

	/*---- Menus ----*/
  create_menus (ui_manager);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

	/*---- Content ----*/

  hbox = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  gtk_widget_show (statusbar);
  /* Retreive dice pixmaps from memory or files */
  LoadDicePixmaps ();

  /* Put all the dice in a vertical column */
  dicebox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), dicebox, FALSE, TRUE, 0);
  gtk_widget_show (dicebox);

  rollLabel = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (rollLabel), TRUE);
  gtk_widget_show (rollLabel);
  gtk_box_pack_start (GTK_BOX (dicebox), rollLabel, FALSE, TRUE, 5);

  mbutton = gtk_button_new_with_label (_("Roll!"));
  gtk_box_pack_end (GTK_BOX (dicebox), mbutton, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (mbutton), "clicked",
		    G_CALLBACK (roll_dice), NULL);
  gtk_widget_show (GTK_WIDGET (mbutton));

  toolbar = gtk_toolbar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar),
			       GTK_ORIENTATION_VERTICAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), FALSE);
  gtk_box_pack_end (GTK_BOX (dicebox), toolbar, TRUE, TRUE, 0);

  for (i = 0; i < NUMBER_OF_DICE; i++) {
    tmp = gtk_vbox_new (FALSE, 0);

    for (j = 0; j < NUMBER_OF_PIXMAPS; j++) {
      gtk_box_pack_start (GTK_BOX (tmp), dicePixmaps[i][j][GAME_YAHTZEE], FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (tmp), dicePixmaps[i][j][GAME_KISMET], FALSE, FALSE, 0);
    }

    diceBox[i] = gtk_toggle_tool_button_new ();
    gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (diceBox[i]), tmp);
    g_signal_connect (G_OBJECT (diceBox[i]), "clicked",
		      G_CALLBACK (modify_dice), &DiceValues[i]);

    gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
			GTK_TOOL_ITEM (diceBox[i]), -1);

    gtk_widget_show (GTK_WIDGET (diceBox[i]));
    gtk_widget_show (tmp);
  /*gtk_widget_show (dicePixmaps[i][0][game_type]);*/
  }
  gtk_widget_show (toolbar);

  /* Scores displayed in score list */
  ScoreList = create_score_list ();
  gtk_box_pack_end (GTK_BOX (hbox), ScoreList, TRUE, TRUE, 0);
  setup_score_list (ScoreList);
  gtk_widget_show (ScoreList);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);

  gtk_widget_show (window);
}

int
main (int argc, char *argv[])
{
  char **player_names;
  gsize n_player_names;
  guint i;
  GOptionContext *context;
  gboolean retval;
  GError *error = NULL;

  if (!games_runtime_init ("gtali"))
    return 1;

#ifdef ENABLE_SETGID
  setgid_io_init ();
#endif

  /* Reset all yahtzee variables before parsing args */
  YahtzeeInit ();

  context = g_option_context_new (NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  g_option_context_add_main_entries (context, yahtzee_options,
				     GETTEXT_PACKAGE);
  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }

  g_set_application_name (_(appName));

  games_conf_initialise (appID);

  games_stock_init ();

  /* If we're in computer test mode, just run some tests, no GUI */
  if (test_computer_play > 0) {
      gint ii, jj, kk;
      gdouble sum_scores = 0.0;
      game_type = GAME_YAHTZEE;
      if (game_type_string)
          game_type = game_type_from_string(game_type_string);
      g_message("In test computer play section - Using %d trials for simulation", NUM_TRIALS);
      for (ii = 0; ii < test_computer_play; ii++) {
          int num_rolls = 0;
          NumberOfHumans = 0;
          NumberOfComputers = 1;
          NewGame ();

          while (!GameIsOver() && num_rolls < 100) {
              ComputerRolling (CurrentPlayer);
              if (NoDiceSelected () || (NumberOfRolls >= NUM_ROLLS)) {
                ComputerScoring (CurrentPlayer);
                NumberOfRolls = 0;
                SelectAllDice ();
                RollSelectedDice ();
              } else {
                RollSelectedDice ();
              }
              num_rolls++;
          }
          for (kk = NumberOfHumans; kk < NumberOfPlayers; kk++) {
              printf("Computer score: %d\n", total_score(kk));
              sum_scores += total_score(kk);
              if (num_rolls > 98) {
                  for (jj = 0; jj < NUM_FIELDS; jj++)
                      g_message("Category %d is score %d", jj, players[kk].score[jj]);
              }
          }
      }
      printf("Computer average: %.2f for %d trials\n", sum_scores / test_computer_play, NUM_TRIALS);
      exit(0);
  }

  highscores = games_scores_new ("gtali",
                                 category_array, G_N_ELEMENTS (category_array),
                                 "game type", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

  gtk_window_set_default_icon_name ("gnome-tali");

  if (NumberOfComputers == 0)	/* Not set on the command-line. */
    NumberOfComputers = games_conf_get_integer (NULL, KEY_NUMBER_OF_COMPUTERS, NULL);

  if (NumberOfHumans == 0)	/* Not set on the command-line. */
    NumberOfHumans = games_conf_get_integer (NULL, KEY_NUMBER_OF_HUMANS, NULL);

  if (NumberOfHumans < 1)
    NumberOfHumans = 1;
  if (NumberOfComputers < 0)
    NumberOfComputers = 0;

  if (NumberOfHumans > MAX_NUMBER_OF_PLAYERS)
    NumberOfHumans = MAX_NUMBER_OF_PLAYERS;
  if ((NumberOfHumans + NumberOfComputers) > MAX_NUMBER_OF_PLAYERS)
    NumberOfComputers = MAX_NUMBER_OF_PLAYERS - NumberOfHumans;

  if (game_type_string)
    game_type = game_type_from_string(game_type_string);
  else {
    char *type;

    type = games_conf_get_string (NULL, KEY_GAME_TYPE, NULL);
    game_type = game_type_from_string(type);
  }

  set_new_game_type(game_type);

  if (NUM_TRIALS <= 0)
      NUM_TRIALS = games_conf_get_integer (NULL, KEY_NUMTRIALS, NULL);

  if (DoDelay == 0)		/* Not set on the command-line */
    DoDelay = games_conf_get_boolean (NULL, KEY_DELAY_BETWEEN_ROLLS, NULL);
  if (DisplayComputerThoughts == 0)	/* Not set on the command-line */
    DisplayComputerThoughts = games_conf_get_boolean (NULL, KEY_DISPLAY_COMPUTER_THOUGHTS, NULL);
  
  /* Read in new player names */
  player_names = games_conf_get_string_list (NULL, KEY_PLAYER_NAMES, &n_player_names, NULL);
  if (player_names) {
    n_player_names = MIN (n_player_names, MAX_NUMBER_OF_PLAYERS);

    for (i = 0; i < n_player_names; ++i) {
      if (i == 0 && strcasecmp (player_names[i], _("Human")) == 0) {
	const char *realname;

	realname = g_get_real_name ();
        if (realname && realname[0] && strcmp (realname, "Unknown") != 0) {
          players[i].name = g_locale_to_utf8 (realname, -1, NULL, NULL, NULL);
        }
        if (!players[i].name) {
	  players[i].name = g_strdup (player_names[i]);
	}
      } else {
        players[i].name = g_strdup (player_names[i]);
      }
    }

    g_strfreev (player_names);
  }

  GyahtzeeCreateMainWindow ();

  /* Need to roll the dice once */
  GyahtzeeNewGame ();

  gtk_main ();

  games_conf_shutdown ();

  games_runtime_shutdown ();

  exit(0);
}

/* Arrgh - lets all use the same tabs under emacs: 
Local Variables:
tab-width: 8
c-basic-offset: 8
indent-tabs-mode: nil
End:
*/
