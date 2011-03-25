/* -*- mode:C; indent-tabs-mode:nil; tab-width:8; c-basic-offset:8 -*- */

/*
 * Gyatzee: Gnomified Yahtzee game.
 * (C) 1998 the Free Software Foundation
 *
 * File:   setup.c
 *
 * Author: Scott Heavner
 *
 *   Contains parser for command line arguments and 
 *   code for properties window.
 *
 *   Variables are exported in gyahtzee.h
 *
 *   This file is largely based upon GTT code by Eckehard Berns.
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

#include <gtk/gtk.h>

#include <libgames-support/games-frame.h>
#include <libgames-support/games-conf.h>

#include "yahtzee.h"
#include "gyahtzee.h"

static gint setupdialog_destroy (GtkWidget * widget, gint mode);
static GtkWidget *setupdialog = NULL;
static GtkWidget *HumanSpinner, *ComputerSpinner;
static GtkWidget *PlayerNames[MAX_NUMBER_OF_PLAYERS];
static GtkAdjustment *HumanAdj, *ComputerAdj;

static int OriginalNumberOfComputers = -1;
static int OriginalNumberOfHumans = -1;
static GameType NewGameType  = GAME_YAHTZEE;

static int tmpDoDelay = -1, tmpDisplayComputerThoughts;
static int skill_level;
extern int NUM_TRIALS;

extern GtkWidget *window;
typedef struct game_type_table_t {
    GameType type;
    const gchar *name;
} GameTypeTable;

static GameTypeTable game_type_table[] = {
    { GAME_YAHTZEE, "Regular" },
    { GAME_KISMET,  "Colors"  }
};

#define GAME_TYPE_TABLE_SIZE (sizeof(game_type_table) / sizeof(GameTypeTable))

typedef struct skill_level_table_t {
    int level;
    int trials;
} SkillLevelTable;

static SkillLevelTable skill_level_table[] = {
    { 0, 10 },
    { 1, 100 },
    { 2, 1000 }
};

#define SKILL_LEVEL_TABLE_SIZE (sizeof(skill_level_table) / sizeof(SkillLevelTable))

static const gchar *game_type_name(GameType type)
{
    gint ii = 0;
    for (ii = 0; ii < GAME_TYPE_TABLE_SIZE; ii++) {
        if (type == game_type_table[ii].type)
            return game_type_table[ii].name;
    }

    return NULL;
}

GameType game_type_from_string(const gchar *string)
{
    if (string) {
        gint ii =0;
        for (ii = 0; ii < GAME_TYPE_TABLE_SIZE; ii++)
            if (!strcmp(string, game_type_table[ii].name))
                return game_type_table[ii].type;
    }

    return GAME_YAHTZEE;
}

GameType get_new_game_type(void)
{
    return NewGameType;
}

void set_new_game_type(GameType type)
{
    NewGameType = type;
}

static void
WarnNumPlayersChanged (void)
{
  GtkWidget *mb;

  mb = gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_DESTROY_WITH_PARENT,
			       GTK_MESSAGE_INFO,
			       GTK_BUTTONS_OK,
			       _("Current game will complete"
				 " with original number of players."));
  gtk_window_set_position (GTK_WINDOW (mb), GTK_WIN_POS_MOUSE);
  gtk_dialog_run (GTK_DIALOG (mb));
  gtk_widget_destroy (mb);
}


static void
do_setup (GtkWidget * widget, gpointer data)
{
  const gchar  *type_name = NULL;
  int i;
  char **player_names;
  gsize n_player_names;

  NumberOfComputers =
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ComputerSpinner));
  NumberOfHumans =
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (HumanSpinner));
    
  if (tmpDoDelay == -1)
    tmpDoDelay = DoDelay;
  else
    DoDelay = tmpDoDelay;

  DisplayComputerThoughts = tmpDisplayComputerThoughts;

  for (i = 0; i < MAX_NUMBER_OF_PLAYERS; i++) {
    if (players[i].name != _(DefaultPlayerNames[i]))
      g_free (players[i].name);
    players[i].name =
      g_strdup (gtk_entry_get_text (GTK_ENTRY (PlayerNames[i])));

    if (i < NumberOfPlayers)
      score_list_set_column_title (ScoreList, i + 1, players[i].name);
  }

  setupdialog_destroy (setupdialog, 1);

  n_player_names = MAX_NUMBER_OF_PLAYERS;
  player_names = g_newa (char *, n_player_names);
  for (i = 0; i < MAX_NUMBER_OF_PLAYERS; ++i) {
    player_names[i] = players[i].name;
  }

  games_conf_set_string_list (NULL, KEY_PLAYER_NAMES,
                              (const char * const *) player_names,
                              n_player_names);

  games_conf_set_integer (NULL, KEY_NUMBER_OF_COMPUTERS, NumberOfComputers);

  games_conf_set_integer (NULL, KEY_NUMBER_OF_HUMANS, NumberOfHumans);

  games_conf_set_boolean (NULL, KEY_DELAY_BETWEEN_ROLLS, DoDelay);

  type_name = game_type_name(NewGameType);
  if (type_name) {
    games_conf_set_string (NULL, KEY_GAME_TYPE, type_name);
  }

  games_conf_set_integer (NULL, KEY_NUMTRIALS, NUM_TRIALS);

  if (((NumberOfComputers != OriginalNumberOfComputers)
       || (NumberOfHumans != OriginalNumberOfHumans) 
       || (NewGameType != game_type))
      && !GameIsOver ())
    WarnNumPlayersChanged ();
}

static gint
setupdialog_destroy (GtkWidget * widget, gint mode)
{
  if (mode == 1) {
    gtk_widget_destroy (setupdialog);
  }
  setupdialog = NULL;

  return FALSE;
}


static gint
set_as_int (GtkWidget * widget, gpointer * data)
{
  *((int *) data) = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  return FALSE;
}

static gint
MaxPlayersCheck (GtkWidget * widget, gpointer * data)
{
  int numc, numh;

  numc = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ComputerSpinner));
  numh = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (HumanSpinner));

  if ((numc + numh) > MAX_NUMBER_OF_PLAYERS) {
    if (GTK_ADJUSTMENT (data) == GTK_ADJUSTMENT (HumanAdj)) {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (ComputerAdj),
				(gfloat) (numc - 1));
    } else {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (HumanAdj),
				(gfloat) (numh - 1));
    }

  }

  return FALSE;
}

static gint
SetGameType (GtkWidget *widget, gpointer *data)
{
    NewGameType = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    return FALSE;
}

static gint
SetSkillLevel (GtkWidget *widget, gpointer *data)
{
    skill_level = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    if (skill_level >= 0 && skill_level < SKILL_LEVEL_TABLE_SIZE) {
        NUM_TRIALS = skill_level_table[skill_level].trials;
    }
    return FALSE;
}

gint
setup_game (GtkAction * action, gpointer data)
{
  GtkWidget *box, *box2, *label, *button, *frame;
  GtkWidget *table;
  GtkWidget *combo;
  gchar *ts;
  int i;

  if (setupdialog) {
    gtk_window_present (GTK_WINDOW (setupdialog));
    return FALSE;
  }

  setupdialog = gtk_dialog_new_with_buttons (_("Tali Preferences"),
					     GTK_WINDOW (window),
					     GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_CLOSE,
					     GTK_RESPONSE_CLOSE, NULL);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (setupdialog))), 2);
  gtk_window_set_resizable (GTK_WINDOW (setupdialog), FALSE);
  g_signal_connect (G_OBJECT (setupdialog), "delete_event",
		    G_CALLBACK (setupdialog_destroy), NULL);
  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  g_signal_connect (G_OBJECT (setupdialog), "response",
		    G_CALLBACK (do_setup), NULL);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 18);
  gtk_table_set_col_spacings (GTK_TABLE (table), 18);

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (setupdialog))), 
                      table, FALSE, FALSE, 0);

  frame = games_frame_new (_("Human Players"));
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
		    0, 0);

  box = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), box);

	/*--- Spinner (number of humans) ---*/
  OriginalNumberOfHumans = NumberOfHumans;
  box2 = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (box), box2, FALSE, FALSE, 0);
  label = gtk_label_new_with_mnemonic (_("_Number of players:"));

  gtk_box_pack_start (GTK_BOX (box2), label, FALSE, FALSE, 0);
  HumanAdj = gtk_adjustment_new ((gfloat) NumberOfHumans, 1.0,
				 6.0, 1.0, 6.0, 0.0);
  HumanSpinner = gtk_spin_button_new (GTK_ADJUSTMENT (HumanAdj), 10, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), HumanSpinner);

  g_signal_connect (G_OBJECT (HumanAdj), "value_changed",
		    G_CALLBACK (MaxPlayersCheck), HumanAdj);

  gtk_box_pack_start (GTK_BOX (box2), HumanSpinner, TRUE, TRUE, 0);


  frame = games_frame_new (_("Computer Opponents"));
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 1, 2, GTK_FILL,
		    GTK_FILL | GTK_EXPAND, 0, 0);

  box = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), box);

	/*--- Button ---*/
  button = gtk_check_button_new_with_mnemonic (_("_Delay between rolls"));
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), DoDelay);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (set_as_int), &tmpDoDelay);

	/*--- Spinner (number of computers) ---*/
  OriginalNumberOfComputers = NumberOfComputers;
  box2 = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (box), box2, FALSE, FALSE, 0);
  label = gtk_label_new_with_mnemonic (_("N_umber of opponents:"));
  gtk_box_pack_start (GTK_BOX (box2), label, FALSE, FALSE, 0);

  ComputerAdj = gtk_adjustment_new ((gfloat) NumberOfComputers,
				    0.0, 5.0, 1.0, 5.0, 0.0);
  ComputerSpinner = gtk_spin_button_new (GTK_ADJUSTMENT (ComputerAdj), 10, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), ComputerSpinner);

  g_signal_connect (G_OBJECT (ComputerAdj), "value_changed",
		    G_CALLBACK (MaxPlayersCheck), ComputerAdj);
  gtk_box_pack_start (GTK_BOX (box2), ComputerSpinner, TRUE, TRUE, 0);

  box2 = gtk_hbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(box), box2, FALSE, FALSE, 0);
  label = gtk_label_new_with_mnemonic (_("_Difficulty:"));
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);
  combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), C_("difficulty", "Easy"));
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), C_("difficulty", "Medium"));
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), C_("difficulty", "Hard"));
  skill_level = 0;
  while (NUM_TRIALS > skill_level_table[skill_level].trials &&
         skill_level < (SKILL_LEVEL_TABLE_SIZE - 1)) skill_level++;
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), skill_level);
  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (SetSkillLevel),
                    combo);
  gtk_box_pack_start(GTK_BOX(box2), combo, FALSE, FALSE, 0);

    /*--- Combo (yahtzee or kismet style ----*/

  frame = games_frame_new (_("Game Type"));
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 2, 3, GTK_FILL,
                    GTK_FILL | GTK_EXPAND, 0, 0);
  combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), C_("game type", "Regular"));
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), C_("game type", "Colors"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), game_type);
  NewGameType = game_type;
  g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (SetGameType),
                    combo);
  gtk_container_add (GTK_CONTAINER (frame), combo);

	/*--- PLAYER NAMES FRAME ----*/
  frame = games_frame_new (_("Player Names"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 1, 2, 0, 3);

  box = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), box);

  for (i = 0; i < MAX_NUMBER_OF_PLAYERS; i++) {
    box2 = gtk_hbox_new (FALSE, 12);

    gtk_box_pack_start (GTK_BOX (box), box2, FALSE, FALSE, 0);
    ts = g_strdup_printf ("_%1d:", i + 1);
    label = gtk_label_new_with_mnemonic (ts);
    g_free (ts);
    gtk_box_pack_start (GTK_BOX (box2), label, FALSE, FALSE, 0);

    PlayerNames[i] = gtk_entry_new ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), PlayerNames[i]);
    ts = g_strdup_printf ("PlayerName%1d", i + 1);
    gtk_widget_set_name (PlayerNames[i], ts);
    g_free (ts);
    gtk_entry_set_text (GTK_ENTRY (PlayerNames[i]), players[i].name);
    gtk_box_pack_start (GTK_BOX (box2), PlayerNames[i], FALSE, FALSE, 0);
  }

  gtk_widget_show_all (setupdialog);

  return FALSE;
}
