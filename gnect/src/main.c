/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/* main.c
 *
 * Four-in-a-row for GNOME
 * (C) 2000 - 2004
 * Authors: Timothy Musson <trmusson@ihug.co.nz>
 *
 * This game is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include <config.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libgames-support/games-conf.h>
#include <libgames-support/games-gridframe.h>
#include <libgames-support/games-gtk-compat.h>
#include <libgames-support/games-help.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-sound.h>
#include <libgames-support/games-stock.h>
#include <libgames-support/games-fullscreen-action.h>

#include "connect4.h"
#include "main.h"
#include "theme.h"
#include "prefs.h"
#include "gfx.h"

#define SPEED_MOVE     25
#define SPEED_DROP     20
#define SPEED_BLINK    150

#define DEFAULT_WIDTH 350
#define DEFAULT_HEIGHT 390

extern Prefs p;

GtkWidget *app;
GtkWidget *notebook;
GtkWidget *drawarea;
GtkWidget *statusbar;
GtkWidget *scorebox = NULL;
GtkWidget *chat = NULL;

GtkWidget *label_name[3];
GtkWidget *label_score[3];

PlayerID player;
PlayerID winner;
PlayerID who_starts;
gboolean gameover;
gboolean player_active;
gint moves;
gint score[3];
gint column;
gint column_moveto;
gint row;
gint row_dropto;
gint timeout;

gint gboard[7][7];
gchar vstr[SIZE_VSTR];
gchar vlevel[] = "0abc";
struct board *vboard;

typedef enum {
  ANIM_NONE,
  ANIM_MOVE,
  ANIM_DROP,
  ANIM_BLINK,
  ANIM_HINT
} AnimID;

AnimID anim;

gint blink_r1, blink_c1;
gint blink_r2, blink_c2;
gint blink_t;
gint blink_n;
gboolean blink_on;


static void game_process_move (gint c);
static void process_move2 (gint c);
static void process_move3 (gint c);


static void
clear_board (void)
{
  gint r, c, i;

  for (r = 0; r < 7; r++) {
    for (c = 0; c < 7; c++) {
      gboard[r][c] = TILE_CLEAR;
    }
  }

  for (i = 0; i < SIZE_VSTR; i++)
    vstr[i] = '\0';

  vstr[0] = vlevel[LEVEL_WEAK];
  vstr[1] = '0';
  moves = 0;
}



static gint
first_empty_row (gint c)
{
  gint r = 1;

  while (r < 7 && gboard[r][c] == TILE_CLEAR)
    r++;
  return r - 1;
}

static gint
get_n_human_players (void)
{
  if (ggz_network_mode)
    return 2;
  
  if (p.level[PLAYER1] != LEVEL_HUMAN && p.level[PLAYER2] != LEVEL_HUMAN) {
    return 0;
  }
  if (p.level[PLAYER1] != LEVEL_HUMAN || p.level[PLAYER2] != LEVEL_HUMAN) {
    return 1;
  }
  return 2;
}



static gboolean
is_player_human (void)
{
  if (player == PLAYER1) {
    return p.level[PLAYER1] == LEVEL_HUMAN;
  }
  return p.level[PLAYER2] == LEVEL_HUMAN;
}



static void
drop_marble (gint r, gint c)
{
  gint tile;

  if (player == PLAYER1)
    tile = TILE_PLAYER1;
  else
    tile = TILE_PLAYER2;

  gboard[r][c] = tile;
  gfx_draw_tile (r, c);

  column = column_moveto = c;
  row = row_dropto = r;
}



static void
drop (void)
{
  gint tile;

  if (player == PLAYER1)
    tile = TILE_PLAYER1;
  else
    tile = TILE_PLAYER2;

  gboard[row][column] = TILE_CLEAR;
  gfx_draw_tile (row, column);

  row++;
  gboard[row][column] = tile;
  gfx_draw_tile (row, column);
}



static void
move_cursor (gint c)
{
  gboard[0][column] = TILE_CLEAR;
  gfx_draw_tile (0, column);

  column = c;

  if (player == PLAYER1)
    gboard[0][c] = TILE_PLAYER1;
  else
    gboard[0][c] = TILE_PLAYER2;

  gfx_draw_tile (0, c);

  column = column_moveto = c;
  row = row_dropto = 0;
}



static void
move (gint c)
{
  gboard[0][column] = TILE_CLEAR;
  gfx_draw_tile (0, column);

  column = c;

  if (player == PLAYER1)
    gboard[0][c] = TILE_PLAYER1;
  else
    gboard[0][c] = TILE_PLAYER2;

  gfx_draw_tile (0, c);
}



static void
draw_line (gint r1, gint c1, gint r2, gint c2, gint tile)
{
  /* draw a line of 'tile' from r1,c1 to r2,c2 */

  gboolean done = FALSE;
  gint d_row = 0;
  gint d_col = 0;

  if (r1 < r2)
    d_row = 1;
  else if (r1 > r2)
    d_row = -1;

  if (c1 < c2)
    d_col = 1;
  else if (c1 > c2)
    d_col = -1;

  do {
    done = (r1 == r2 && c1 == c2);
    gboard[r1][c1] = tile;
    gfx_draw_tile (r1, c1);
    if (r1 != r2)
      r1 += d_row;
    if (c1 != c2)
      c1 += d_col;
  } while (!done);
}



static gboolean
on_animate (gint c)
{
  if (anim == ANIM_NONE)
    return FALSE;

  switch (anim) {
  case ANIM_NONE:
    break;
  case ANIM_HINT:
  case ANIM_MOVE:
    if (column < column_moveto) {
      move (column + 1);
    } else if (column > column_moveto) {
      move (column - 1);
    } else {
      timeout = 0;
      if (anim == ANIM_MOVE) {
	anim = ANIM_NONE;
	process_move2 (c);
      } else {
	anim = ANIM_NONE;
      }
      return FALSE;
    }
    break;
  case ANIM_DROP:
    if (row < row_dropto) {
      drop ();
    } else {
      anim = ANIM_NONE;
      timeout = 0;
      process_move3 (c);
      return FALSE;
    }
    break;
  case ANIM_BLINK:
    if (blink_on)
      draw_line (blink_r1, blink_c1, blink_r2, blink_c2, blink_t);
    else
      draw_line (blink_r1, blink_c1, blink_r2, blink_c2, TILE_CLEAR);
    blink_n--;
    if (blink_n <= 0 && blink_on) {
      anim = ANIM_NONE;
      timeout = 0;
      return FALSE;
    }
    blink_on = !blink_on;
    break;
  }
  return TRUE;
}



static void
blink_tile (gint r, gint c, gint t, gint n)
{
  if (timeout)
    return;
  blink_r1 = r;
  blink_c1 = c;
  blink_r2 = r;
  blink_c2 = c;
  blink_t = t;
  blink_n = n;
  blink_on = FALSE;
  anim = ANIM_BLINK;
  timeout = g_timeout_add (SPEED_BLINK, (GSourceFunc) on_animate, NULL);
}



static void
swap_player (void)
{
  player = (player == PLAYER1) ? PLAYER2 : PLAYER1;
  move_cursor (column);
  prompt_player ();
}



void
set_status_message (const gchar * message)
{
  guint context_id;
  context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "message");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), context_id);
  if (message)
    gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_id, message);
}


GtkAction *new_game_action;
GtkAction *new_network_action;
GtkAction *leave_network_action;
GtkAction *player_list_action;
GtkAction *undo_action;
GtkAction *hint_action;
GtkAction *fullscreen_action;


static void
stop_anim (void)
{
  if (timeout == 0)
    return;
  anim = ANIM_NONE;
  g_source_remove (timeout);
  timeout = 0;
}

static void
game_init (void)
{
  g_random_set_seed ((guint) time (NULL));
  vboard = veleng_init ();

  anim = ANIM_NONE;
  gameover = TRUE;
  player_active = FALSE;
  player = PLAYER1;
  winner = NOBODY;
  score[PLAYER1] = 0;
  score[PLAYER2] = 0;
  score[NOBODY] = 0;

  who_starts = PLAYER2;		/* This gets reversed immediately. */

  clear_board ();
}



void
game_reset (void)
{
  stop_anim ();
  gtk_action_set_sensitive (undo_action, FALSE);
  gtk_action_set_sensitive (hint_action, FALSE);

  who_starts = (who_starts == PLAYER1) ? PLAYER2 : PLAYER1;
  player = who_starts;

  gameover = TRUE;
  player_active = FALSE;
  winner = NOBODY;
  column = 3;
  column_moveto = 3;
  row = 0;
  row_dropto = 0;

  clear_board ();
  set_status_message (NULL);
  gfx_draw_all ();

  move_cursor (column);
  gameover = FALSE;
  prompt_player ();
  if (!is_player_human ()) {
    if (player == PLAYER1) {
      vstr[0] = vlevel[p.level[PLAYER1]];
    } else {
      vstr[0] = vlevel[p.level[PLAYER2]];
    }
    game_process_move (playgame (vstr, vboard) - 1);
  }
}



static void
game_free (void)
{
  veleng_free (vboard);
  gfx_free ();
}



static void
play_sound (SoundID id)
{
 /* if (!p.do_sound)
    return;*/

  switch (id) {
  case SOUND_DROP:
    games_sound_play ("slide");
    break;
  case SOUND_I_WIN:
    games_sound_play ("reverse");
    break;
  case SOUND_YOU_WIN:
    games_sound_play ("bonus");
    break;
  case SOUND_PLAYER_WIN:
    games_sound_play ("bonus");
    break;
  case SOUND_DRAWN_GAME:
    games_sound_play ("reverse");
    break;
  case SOUND_COLUMN_FULL:
    games_sound_play ("bad");
    break;
  }
}


void
prompt_player (void)
{
  gint players = get_n_human_players ();
  gint human = is_player_human ();
  const gchar *who = NULL;
  gchar *str = NULL;

#ifdef GGZ_CLIENT
  if (ggz_network_mode) {
    gtk_widget_show (chat);
  } else {
    gtk_widget_hide (chat);
  }
#endif

  gtk_action_set_visible (new_game_action, !ggz_network_mode);
  gtk_action_set_visible (hint_action, !ggz_network_mode);
  gtk_action_set_visible (undo_action, !ggz_network_mode);
  gtk_action_set_visible (new_network_action, !ggz_network_mode);
  gtk_action_set_visible (player_list_action, ggz_network_mode);
  gtk_action_set_visible (leave_network_action, ggz_network_mode);

  gtk_action_set_sensitive (new_game_action, (human || gameover));
  gtk_action_set_sensitive (hint_action, (human || gameover));

  switch (players) {
  case 0:
    gtk_action_set_sensitive (undo_action, FALSE);
    break;
  case 1:
    gtk_action_set_sensitive (undo_action,
			      ((human && moves > 1) || (!human && gameover)));
    break;
  case 2:
    gtk_action_set_sensitive (undo_action, (moves > 0));
    break;
  }

  if (gameover && winner == NOBODY) {
    if (score[NOBODY] == 0) {
      set_status_message ("");
    } else {
      set_status_message (_("It's a draw!"));
    }
    return;
  }

  switch (players) {
  case 1:
    if (human) {
      if (gameover)
	set_status_message (_("You win!"));
      else
	set_status_message (_("It is your move."));
    } else {
      if (gameover)
	set_status_message (_("I win!"));
      else
	set_status_message (_("Thinking..."));
    }
    break;
  case 2:
  case 0:
    if (ggz_network_mode) {
#ifdef GGZ_CLIENT
      who = variables.name[(variables.num + 1) % 2];
      if (!who)
	return;
#endif
    } else {
      if (player == PLAYER1)
	who = _(theme_get_player (PLAYER1));
      else
	who = _(theme_get_player (PLAYER2));
    }

    if (gameover) {
      if (ggz_network_mode) {
#ifdef GGZ_CLIENT
	str = g_strdup_printf (_("%s wins!"),
			       variables.name[(int) variables.winner]);
#endif
      } else {
	str = g_strdup_printf (_("%s wins!"), who);
      }
    } else if (player_active && ggz_network_mode) {
      set_status_message (_("It is your move."));
      return;
    } else {
      str = g_strdup_printf (_("Waiting for %s to move."), who);
    }

    set_status_message (str);
    g_free (str);
    break;
  }
}



static void
on_game_new (void)
{
  stop_anim ();
  game_reset ();
}

static gboolean
on_network_leave (GObject * object, gpointer data)
{
#ifdef GGZ_CLIENT
  ggz_embed_leave_table ();
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), NETWORK_PAGE);
#endif
  return TRUE;
}

static gboolean
on_game_exit (GObject * object, gpointer data)
{

  stop_anim ();
  gtk_main_quit ();
  return TRUE;
}

static void
on_player_list (void)
{
#ifdef GGZ_CLIENT
  create_or_raise_dlg_players (GTK_WINDOW (app));
#endif
}

static void
on_game_undo (GtkMenuItem * m, gpointer data)
{
  gint r, c;

  if (timeout)
    return;
  c = vstr[moves] - '0' - 1;
  r = first_empty_row (c) + 1;
  vstr[moves] = '0';
  vstr[moves + 1] = '\0';
  moves--;

  if (gameover) {
    score[winner]--;
    scorebox_update ();
    gameover = FALSE;
    prompt_player ();
  } else {
    swap_player ();
  }
  move_cursor (c);

  gboard[r][c] = TILE_CLEAR;
  gfx_draw_tile (r, c);

  if (get_n_human_players () == 1 && !is_player_human ()) {
    if (moves > 0) {
      c = vstr[moves] - '0' - 1;
      r = first_empty_row (c) + 1;
      vstr[moves] = '0';
      vstr[moves + 1] = '\0';
      moves--;
      swap_player ();
      move_cursor (c);
      gboard[r][c] = TILE_CLEAR;
      gfx_draw_tile (r, c);
    }
  }
}



static void
on_game_hint (GtkMenuItem * m, gpointer data)
{
  gchar *s;
  gint c;

  if (timeout)
    return;
  if (gameover)
    return;

  gtk_action_set_sensitive (hint_action, FALSE);
  gtk_action_set_sensitive (undo_action, FALSE);

  set_status_message (_("Thinking..."));

  vstr[0] = vlevel[LEVEL_STRONG];
  c = playgame (vstr, vboard) - 1;

  column_moveto = c;
  if (p.do_animate) {
    while (timeout)
      gtk_main_iteration ();
    anim = ANIM_HINT;
    timeout = g_timeout_add (SPEED_MOVE, (GSourceFunc) on_animate, NULL);
  } else {
    move_cursor (column_moveto);
  }

  blink_tile (0, c, gboard[0][c], 6);

  s = g_strdup_printf (_("Hint: Column %d"), c + 1);
  set_status_message (s);
  g_free (s);

  gtk_action_set_sensitive (hint_action, TRUE);
  gtk_action_set_sensitive (undo_action, (moves > 0));
}



void
on_dialog_close (GtkWidget * w, int response_id, gpointer data)
{
  gtk_widget_hide (w);
}



void
scorebox_update (void)
{
  gchar *s;

  if (scorebox == NULL)
    return;

  if (get_n_human_players () == 1) {
    if (p.level[PLAYER1] == LEVEL_HUMAN) {
      gtk_label_set_text (GTK_LABEL (label_name[PLAYER1]), _("You:"));
      gtk_label_set_text (GTK_LABEL (label_name[PLAYER2]), _("Me:"));
    } else {
      gtk_label_set_text (GTK_LABEL (label_name[PLAYER1]), _("Me:"));
      gtk_label_set_text (GTK_LABEL (label_name[PLAYER2]), _("You:"));
    }
  } else {
    gtk_label_set_text (GTK_LABEL (label_name[PLAYER1]),
			_(theme_get_player (PLAYER1)));
    gtk_label_set_text (GTK_LABEL (label_name[PLAYER2]),
			_(theme_get_player (PLAYER2)));
  }

  s = g_strdup_printf ("%d", score[PLAYER1]);
  gtk_label_set_text (GTK_LABEL (label_score[PLAYER1]), s);
  g_free (s);

  s = g_strdup_printf ("%d", score[PLAYER2]);
  gtk_label_set_text (GTK_LABEL (label_score[PLAYER2]), s);
  g_free (s);

  s = g_strdup_printf ("%d", score[NOBODY]);
  gtk_label_set_text (GTK_LABEL (label_score[NOBODY]), s);
  g_free (s);
}



void
scorebox_reset (void)
{
  score[PLAYER1] = 0;
  score[PLAYER2] = 0;
  score[NOBODY] = 0;
  scorebox_update ();
}



static void
on_game_scores (GtkMenuItem * m, gpointer data)
{
  GtkWidget *table, *vbox, *icon;

  if (scorebox != NULL) {
    gtk_window_present (GTK_WINDOW (scorebox));
    return;
  }

  scorebox = gtk_dialog_new_with_buttons (_("Scores"),
					  GTK_WINDOW (app),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_CLOSE,
					  GTK_RESPONSE_CLOSE, NULL);

  gtk_window_set_resizable (GTK_WINDOW (scorebox), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (scorebox), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (scorebox))), 2);

  g_signal_connect (scorebox, "destroy",
		    G_CALLBACK (gtk_widget_destroyed), &scorebox);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (scorebox))),
		      vbox, TRUE, TRUE, 0);

  icon = gtk_image_new_from_icon_name ("gnome-gnect", 48);
  gtk_box_pack_start (GTK_BOX (vbox), icon, FALSE, FALSE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  label_name[PLAYER1] = gtk_label_new (NULL);
  gtk_table_attach (GTK_TABLE (table), label_name[PLAYER1], 0, 1, 0, 1,
		    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) 0, 0,
		    0);
  gtk_misc_set_alignment (GTK_MISC (label_name[PLAYER1]), 0, 0.5);

  label_score[PLAYER1] = gtk_label_new (NULL);
  gtk_table_attach (GTK_TABLE (table), label_score[PLAYER1], 1, 2, 0, 1,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label_score[PLAYER1]), 1, 0.5);

  label_name[PLAYER2] = gtk_label_new (NULL);
  gtk_table_attach (GTK_TABLE (table), label_name[PLAYER2], 0, 1, 1, 2,
		    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) 0, 0,
		    0);
  gtk_misc_set_alignment (GTK_MISC (label_name[PLAYER2]), 0, 0.5);

  label_score[PLAYER2] = gtk_label_new (NULL);
  gtk_table_attach (GTK_TABLE (table), label_score[PLAYER2], 1, 2, 1, 2,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label_score[PLAYER2]), 1, 0.5);

  label_name[NOBODY] = gtk_label_new (_("Drawn:"));
  gtk_table_attach (GTK_TABLE (table), label_name[NOBODY], 0, 1, 2, 3,
		    (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) 0, 0,
		    0);
  gtk_misc_set_alignment (GTK_MISC (label_name[NOBODY]), 0, 0.5);

  label_score[NOBODY] = gtk_label_new (NULL);
  gtk_table_attach (GTK_TABLE (table), label_score[NOBODY], 1, 2, 2, 3,
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label_score[NOBODY]), 1, 0.5);

  g_signal_connect (GTK_DIALOG (scorebox), "response",
		    G_CALLBACK (on_dialog_close), NULL);

  gtk_widget_show_all (scorebox);

  scorebox_update ();
}



static void
on_help_about (GtkAction * action, gpointer data)
{
  const gchar *authors[] = { "Four-in-a-row:",
    "  Tim Musson <trmusson@ihug.co.nz>",
    "  David Neary <bolsh@gimp.org>",
    "",
    "Velena Engine V1.07:",
    "  AI engine written by Giuliano Bertoletti",
    "  Based on the knowledged approach of Victor Allis",
    "  Copyright (C) 1996-97 ",
    "  Giuliano Bertoletti and GBE 32241 Software PR.",
    NULL
  };

  const gchar *artists[] = { "Alan Horkan",
    "Tim Musson",
    NULL
  };

  const gchar *documenters[] = { "Timothy Musson",
    NULL
  };
  gchar *license = games_get_license (_(APPNAME_LONG));

  gtk_show_about_dialog (GTK_WINDOW (app),
			 "name", _(APPNAME_LONG),
			 "version", VERSION,
			 "copyright",
			 "Copyright \xc2\xa9 1999-2008, Tim Musson and David Neary",
			 "license", license, "comments",
		         _("\"Four in a Row\" for GNOME, with a computer player driven by Giuliano Bertoletti's Velena Engine.\n\n\"Four in a Row\" is a part of GNOME Games."),
		         "website-label", _("GNOME Games web site"),
			 "authors", authors, "documenters", documenters,
			 "artists", artists, "translator-credits",
			 _("translator-credits"),
			 "logo-icon-name", "gnome-gnect",
			 "website", "http://www.gnome.org/projects/gnome-games/",
			 "wrap-license", TRUE, NULL);
  g_free (license);
}


static void
on_help_contents (GtkAction * action, gpointer data)
{
  games_help_display (app, "gnect", NULL);
}


#if 0
static void
on_settings_toggle_sound (GtkMenuItem * m, gpointer user_data)
{
  p.do_sound = GTK_CHECK_MENU_ITEM (m)->active;
  games_conf_set_boolean (NULL, KEY_DO_SOUND, p.do_sound);
}
#endif



static void
on_settings_preferences (GtkAction * action, gpointer user_data)
{
  prefsbox_open ();
}



static gboolean
is_hline_at (PlayerID p, gint r, gint c, gint * r1, gint * c1, gint * r2,
	     gint * c2)
{
  *r1 = *r2 = r;
  *c1 = *c2 = c;
  while (*c1 > 0 && gboard[r][*c1 - 1] == p)
    *c1 = *c1 - 1;
  while (*c2 < 6 && gboard[r][*c2 + 1] == p)
    *c2 = *c2 + 1;
  if (*c2 - *c1 >= 3)
    return TRUE;
  return FALSE;
}



static gboolean
is_vline_at (PlayerID p, gint r, gint c, gint * r1, gint * c1, gint * r2,
	     gint * c2)
{
  *r1 = *r2 = r;
  *c1 = *c2 = c;
  while (*r1 > 1 && gboard[*r1 - 1][c] == p)
    *r1 = *r1 - 1;
  while (*r2 < 6 && gboard[*r2 + 1][c] == p)
    *r2 = *r2 + 1;
  if (*r2 - *r1 >= 3)
    return TRUE;
  return FALSE;
}



static gboolean
is_dline1_at (PlayerID p, gint r, gint c, gint * r1, gint * c1, gint * r2,
	      gint * c2)
{
  /* upper left to lower right */
  *r1 = *r2 = r;
  *c1 = *c2 = c;
  while (*c1 > 0 && *r1 > 1 && gboard[*r1 - 1][*c1 - 1] == p) {
    *r1 = *r1 - 1;
    *c1 = *c1 - 1;
  }
  while (*c2 < 6 && *r2 < 6 && gboard[*r2 + 1][*c2 + 1] == p) {
    *r2 = *r2 + 1;
    *c2 = *c2 + 1;
  }
  if (*r2 - *r1 >= 3)
    return TRUE;
  return FALSE;
}



static gboolean
is_dline2_at (PlayerID p, gint r, gint c, gint * r1, gint * c1, gint * r2,
	      gint * c2)
{
  /* upper right to lower left */
  *r1 = *r2 = r;
  *c1 = *c2 = c;
  while (*c1 < 6 && *r1 > 1 && gboard[*r1 - 1][*c1 + 1] == p) {
    *r1 = *r1 - 1;
    *c1 = *c1 + 1;
  }
  while (*c2 > 0 && *r2 < 6 && gboard[*r2 + 1][*c2 - 1] == p) {
    *r2 = *r2 + 1;
    *c2 = *c2 - 1;
  }
  if (*r2 - *r1 >= 3)
    return TRUE;
  return FALSE;
}



static gboolean
is_line_at (PlayerID p, gint r, gint c)
{
  gint r1, r2, c1, c2;

  return is_hline_at (p, r, c, &r1, &c1, &r2, &c2) ||
    is_vline_at (p, r, c, &r1, &c1, &r2, &c2) ||
    is_dline1_at (p, r, c, &r1, &c1, &r2, &c2) ||
    is_dline2_at (p, r, c, &r1, &c1, &r2, &c2);
}



static void
blink_winner (gint n)
{
  /* blink the winner's line(s) n times */

  if (winner == NOBODY)
    return;

  blink_t = winner;
  if (is_hline_at
      (winner, row, column, &blink_r1, &blink_c1, &blink_r2, &blink_c2)) {
    anim = ANIM_BLINK;
    blink_on = FALSE;
    blink_n = n;
    timeout = g_timeout_add (SPEED_BLINK, (GSourceFunc) on_animate, NULL);
    while (timeout)
      gtk_main_iteration ();
  }

  if (is_vline_at
      (winner, row, column, &blink_r1, &blink_c1, &blink_r2, &blink_c2)) {
    anim = ANIM_BLINK;
    blink_on = FALSE;
    blink_n = n;
    timeout = g_timeout_add (SPEED_BLINK, (GSourceFunc) on_animate, NULL);
    while (timeout)
      gtk_main_iteration ();
  }

  if (is_dline1_at
      (winner, row, column, &blink_r1, &blink_c1, &blink_r2, &blink_c2)) {
    anim = ANIM_BLINK;
    blink_on = FALSE;
    blink_n = n;
    timeout = g_timeout_add (SPEED_BLINK, (GSourceFunc) on_animate, NULL);
    while (timeout)
      gtk_main_iteration ();
  }

  if (is_dline2_at
      (winner, row, column, &blink_r1, &blink_c1, &blink_r2, &blink_c2)) {
    anim = ANIM_BLINK;
    blink_on = FALSE;
    blink_n = n;
    timeout = g_timeout_add (SPEED_BLINK, (GSourceFunc) on_animate, NULL);
    while (timeout)
      gtk_main_iteration ();
  }
}



static void
check_game_state (void)
{
  if (is_line_at (player, row, column)) {
    gameover = TRUE;
    winner = player;
    switch (get_n_human_players ()) {
    case 1:
      if (is_player_human ()) {
	play_sound (SOUND_YOU_WIN);
      } else {
	play_sound (SOUND_I_WIN);
      }
      break;
    case 0:
    case 2:
      play_sound (SOUND_PLAYER_WIN);
      break;
    }
    blink_winner (6);
  } else if (moves == 42) {
    gameover = TRUE;
    winner = NOBODY;
    play_sound (SOUND_DRAWN_GAME);
  }
}

static gint
next_move (gint c)
{
  process_move (c);
  return FALSE;
}

static void
game_process_move (gint c)
{

  if (ggz_network_mode) {
#ifdef GGZ_CLIENT
    network_send_move (c);
#endif
    return;
  } else {
    process_move (c);
  }
}

void
process_move (gint c)
{
  if (timeout) {
    g_timeout_add (SPEED_DROP,
	           (GSourceFunc) next_move, GINT_TO_POINTER (c));
    return;

  }

  if (!p.do_animate) {
    move_cursor (c);
    column_moveto = c;
    process_move2 (c);
  } else {
    column_moveto = c;
    anim = ANIM_MOVE;
    timeout = g_timeout_add (SPEED_MOVE,
                             (GSourceFunc) on_animate, GINT_TO_POINTER (c));
  }
}

static void
process_move2 (gint c)
{
  gint r;

  r = first_empty_row (c);
  if (r > 0) {

    if (!p.do_animate) {
      drop_marble (r, c);
      process_move3 (c);
    } else {
      row = 0;
      row_dropto = r;
      anim = ANIM_DROP;
      timeout = g_timeout_add (SPEED_DROP,
                               (GSourceFunc) on_animate,
                               GINT_TO_POINTER (c));
    }
  } else {
    play_sound (SOUND_COLUMN_FULL);
  }
}

static void
process_move3 (gint c)
{
  play_sound (SOUND_DROP);

  vstr[++moves] = '1' + c;
  vstr[moves + 1] = '0';

  check_game_state ();

  if (gameover) {
    score[winner]++;
    scorebox_update ();
    prompt_player ();
  } else {
    swap_player ();
    if (!is_player_human ()) {
      if (player == PLAYER1) {
	vstr[0] = vlevel[p.level[PLAYER1]];
      } else {
	vstr[0] = vlevel[p.level[PLAYER2]];
      }
      c = playgame (vstr, vboard) - 1;
      if (c < 0)
	gameover = TRUE;
      g_timeout_add (SPEED_DROP,
                     (GSourceFunc) next_move, GINT_TO_POINTER (c));
    }
  }
}

static gint
on_drawarea_resize (GtkWidget * w, GdkEventConfigure * e, gpointer data)
{
  gfx_resize (w);

  return TRUE;
}

static gboolean
on_drawarea_draw (GtkWidget * w, cairo_t *cr, gpointer data)
{
  gfx_expose (cr);

  return FALSE;
}

static gboolean
on_key_press (GtkWidget * w, GdkEventKey * e, gpointer data)
{
  if ((!player_active && ggz_network_mode) || timeout ||
      (e->keyval != p.keypress[MOVE_LEFT] &&
       e->keyval != p.keypress[MOVE_RIGHT] &&
       e->keyval != p.keypress[MOVE_DROP])) {
    return FALSE;
  }

  if (gameover) {
    blink_winner (2);
    return TRUE;
  }

  if (e->keyval == p.keypress[MOVE_LEFT] && column) {
    column_moveto--;
    move_cursor (column_moveto);
  } else if (e->keyval == p.keypress[MOVE_RIGHT] && column < 6) {
    column_moveto++;
    move_cursor (column_moveto);
  } else if (e->keyval == p.keypress[MOVE_DROP]) {
    game_process_move (column);
  }
  return TRUE;
}

static gboolean
on_button_press (GtkWidget * w, GdkEventButton * e, gpointer data)
{
  gint x, y;
  if (!player_active && ggz_network_mode) {
    return FALSE;
  }

  if (gameover && !timeout) {
    blink_winner (2);
  } else if (is_player_human () && !timeout) {
    gtk_widget_get_pointer (w, &x, &y);
    game_process_move (gfx_get_column (x));
  }

  return TRUE;
}

static const GtkActionEntry action_entry[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"ViewMenu", NULL, N_("_View")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL,
   G_CALLBACK (on_game_new)},
#ifdef GGZ_CLIENT
  {"NewNetworkGame", GAMES_STOCK_NETWORK_GAME, NULL, NULL, NULL,
   G_CALLBACK (on_network_game)},
#else
  {"NewNetworkGame", GAMES_STOCK_NETWORK_GAME, NULL, NULL, NULL, NULL},
#endif
  {"LeaveNetworkGame", GAMES_STOCK_NETWORK_LEAVE, NULL, NULL, NULL,
   G_CALLBACK (on_network_leave)},
  {"PlayerList", GAMES_STOCK_PLAYER_LIST, NULL, NULL, NULL,
   G_CALLBACK (on_player_list)},
  {"UndoMove", GAMES_STOCK_UNDO_MOVE, NULL, NULL, NULL,
   G_CALLBACK (on_game_undo)},
  {"Hint", GAMES_STOCK_HINT, NULL, NULL, NULL, G_CALLBACK (on_game_hint)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL,
   G_CALLBACK (on_game_scores)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (on_game_exit)},
  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (on_settings_preferences)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL,
   G_CALLBACK (on_help_contents)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (on_help_about)}
};

static const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <menuitem action='NewNetworkGame'/>"
  "      <menuitem action='PlayerList'/>"
  "      <separator/>"
  "      <menuitem action='UndoMove'/>"
  "      <menuitem action='Hint'/>"
  "      <separator/>"
  "      <menuitem action='Scores'/>"
  "      <separator/>"
  "      <menuitem action='LeaveNetworkGame'/>"
  "      <menuitem action='Quit'/>"
  "    </menu>"
  "    <menu action='ViewMenu'>"
  "      <menuitem action='Fullscreen'/>"
  "    </menu>"
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='Preferences'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>" "    </menu>" "  </menubar>" "</ui>";



static void
create_game_menus (GtkUIManager * ui_manager)
{
  GtkActionGroup *action_group;
  games_stock_init ();

  action_group = gtk_action_group_new ("actions");

  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entry,
				G_N_ELEMENTS (action_entry), app);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

  gtk_window_add_accel_group (GTK_WINDOW (app),
			      gtk_ui_manager_get_accel_group (ui_manager));

  new_game_action = gtk_action_group_get_action (action_group, "NewGame");
  new_network_action = gtk_action_group_get_action (action_group,
						    "NewNetworkGame");
#ifndef GGZ_CLIENT
  gtk_action_set_sensitive (new_network_action, FALSE);
#endif

  player_list_action =
    gtk_action_group_get_action (action_group, "PlayerList");
  leave_network_action =
    gtk_action_group_get_action (action_group, "LeaveNetworkGame");
  hint_action = gtk_action_group_get_action (action_group, "Hint");
  undo_action = gtk_action_group_get_action (action_group, "UndoMove");
  fullscreen_action = GTK_ACTION (games_fullscreen_action_new ("Fullscreen", GTK_WINDOW (app)));
  gtk_action_group_add_action_with_accel (action_group, fullscreen_action, NULL);
}


static gboolean
create_app (void)
{
  GtkWidget *menubar;
  GtkWidget *gridframe;
  GtkWidget *vbox;
  GtkWidget *vpaned;
  GtkUIManager *ui_manager;

  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (app), _(APPNAME_LONG));

  gtk_window_set_default_size (GTK_WINDOW (app), DEFAULT_WIDTH, DEFAULT_HEIGHT);
  games_conf_add_window (GTK_WINDOW (app), NULL);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  g_signal_connect (G_OBJECT (app), "delete_event",
		    G_CALLBACK (on_game_exit), NULL);

  gtk_window_set_default_icon_name ("gnome-gnect");

  statusbar = gtk_statusbar_new ();
  ui_manager = gtk_ui_manager_new ();

  games_stock_prepare_for_statusbar_tooltips (ui_manager, statusbar);
  create_game_menus (ui_manager);
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

  vpaned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);

  vbox = gtk_vbox_new (FALSE, 0);
  gridframe = games_grid_frame_new (7, 7);


  gtk_paned_pack1 (GTK_PANED (vpaned), gridframe, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
#ifdef GGZ_CLIENT
  chat = create_chat_widget ();
  gtk_paned_pack2 (GTK_PANED (vpaned), chat, FALSE, TRUE);
#endif
  gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (app), notebook);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), MAIN_PAGE);

  drawarea = gtk_drawing_area_new ();

  /* set a min size to avoid pathological behavior of gtk when scaling down */
  gtk_widget_set_size_request (drawarea, 200, 200);

  gtk_container_add (GTK_CONTAINER (gridframe), drawarea);

  gtk_widget_set_events (drawarea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT (drawarea), "configure_event",
		    G_CALLBACK (on_drawarea_resize), NULL);
  g_signal_connect (G_OBJECT (drawarea), "draw",
		    G_CALLBACK (on_drawarea_draw), NULL);
  g_signal_connect (G_OBJECT (drawarea), "button_press_event",
		    G_CALLBACK (on_button_press), NULL);
  g_signal_connect (G_OBJECT (app), "key_press_event",
		    G_CALLBACK (on_key_press), NULL);

  /* We do our own double-buffering. */
  gtk_widget_set_double_buffered (GTK_WIDGET (drawarea), FALSE);

  gtk_action_set_sensitive (hint_action, FALSE);
  gtk_action_set_sensitive (undo_action, FALSE);

  gtk_widget_show_all (app);
#ifdef GGZ_CLIENT
  gtk_widget_hide (chat);
#endif

  gfx_refresh_pixmaps ();
  gfx_draw_all ();

  scorebox_update ();		/* update visible player descriptions */
  prompt_player ();

  return TRUE;
}



int
main (int argc, char *argv[])
{
  GOptionContext *context;
  gboolean retval;
  GError *error = NULL;

  if (!games_runtime_init ("gnect"))
    return 1;

  context = g_option_context_new (NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }

  g_set_application_name (_(APPNAME_LONG));

  games_conf_initialise (APPNAME);

  prefs_init ();
  game_init ();

  /* init gfx */
  if (!gfx_load_pixmaps ()) {
    games_conf_shutdown ();
    exit (1);
  }

#ifdef GGZ_CLIENT
  network_init ();
#endif

  if (create_app ()) {
    game_reset ();
    gtk_main ();
  }

  game_free ();

  games_conf_shutdown ();

  games_runtime_shutdown ();

  return 0;
}
