/*
 * Gnome Robots II
 * written by Mark Rae <m.rae@inpharmatica.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * For more details see the file COPYING.
 */

#include <config.h>

#include <stdlib.h>
#include <math.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libgames-support/games-scores.h>
#include <libgames-support/games-scores-dialog.h>

#include "gbdefs.h"
#include "gameconfig.h"
#include "keyboard.h"
#include "game.h"
#include "gnobots.h"
#include "sound.h"
#include "properties.h"
#include "menu.h"
#include "statusbar.h"
#include "graphics.h"
#include "cursors.h"

/**********************************************************************/
/* Exported Variables                                                 */
/**********************************************************************/
gint game_state = STATE_PLAYING;
gint arena[GAME_WIDTH][GAME_HEIGHT];
/**********************************************************************/


/**********************************************************************/
/* File Static Variables                                              */
/**********************************************************************/
static gint num_robots1 = 0;
static gint num_robots2 = 0;
static gint endlev_counter = 0;
static gint current_level = 0;
static gint score = 0;
static gint kills = 0;
static gint score_step = 0;
static gint safe_teleports = 0;
static gint player_xpos = 0;
static gint player_ypos = 0;
static gint push_xpos = -1;
static gint push_ypos = -1;
static gint game_timer_id = -1;
static gint temp_arena[GAME_WIDTH][GAME_HEIGHT];
/**********************************************************************/


/**********************************************************************/
/* Function Prototypes                                                */
/**********************************************************************/
static void message_box (gchar * msg);
static guint log_score (gint sc);
static void add_kill (gint type);
static void clear_arena (void);
static gint check_location (gint x, gint y);
static void generate_level (void);
static void update_arena (void);
static gint timeout_cb (void *data);
static void destroy_game_timer (void);
static void create_game_timer (void);
static void move_all_robots (void);
static void move_type2_robots (void);
static void move_robots (void);
static gboolean check_safe (gint x, gint y);
static gboolean push_heap (gint x, gint y, gint dx, gint dy);
static gboolean try_player_move (gint dx, gint dy);
static gboolean safe_move_available (void);
static gboolean safe_teleport_available (void);
static gboolean player_move (gint dx, gint dy);
static gboolean random_teleport (void);
static gboolean safe_teleport (void);
/**********************************************************************/


/**********************************************************************/
/* Function Definitions                                               */
/**********************************************************************/

/**
 * message_box
 * @msg: message
 *
 * Description:
 * Displays a modal dialog box with a given message
 **/
static void
message_box (gchar * msg)
{
  GtkWidget *box;

  box = gtk_message_dialog_new (GTK_WINDOW (app), GTK_DIALOG_MODAL,
                                GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);
  gtk_dialog_run (GTK_DIALOG (box));
  gtk_widget_destroy (box);
}

/**
 * show_scores
 * @pos: score-table position
 * @endofgame: game state
 *
 * Description:
 * Displays the high-score table
 **/

gint
show_scores (gint pos, gboolean endofgame)
{
  gchar *message;
  static GtkWidget *scoresdialog = NULL;
  static GtkWidget *sorrydialog = NULL;
  GtkWidget *dialog;
  gint result;

  if (endofgame && (pos <= 0)) {
    if (sorrydialog != NULL) {
      gtk_window_present (GTK_WINDOW (sorrydialog));
    } else {
      sorrydialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (app),
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        GTK_MESSAGE_INFO,
                                                        GTK_BUTTONS_NONE,
                                                        "<b>%s</b>\n%s",
                                                        _
                                                        ("Game over!"),
                                                        _
                                                        ("Great work, but unfortunately your score did not make the top ten."));
      gtk_dialog_add_buttons (GTK_DIALOG (sorrydialog), GTK_STOCK_QUIT,
                              GTK_RESPONSE_REJECT, _("_New Game"),
                              GTK_RESPONSE_ACCEPT, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (sorrydialog),
                                       GTK_RESPONSE_ACCEPT);
      gtk_window_set_title (GTK_WINDOW (sorrydialog), "");
    }
    dialog = sorrydialog;
  } else {

    if (scoresdialog != NULL) {
      gtk_window_present (GTK_WINDOW (scoresdialog));
    } else {
      scoresdialog = games_scores_dialog_new (GTK_WINDOW (app), 
                                        highscores, _("Robots Scores"));
      games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
                                                    (scoresdialog),
                                                    _("Map:"));
    }

    if (pos > 0) {
      play_sound (SOUND_VICTORY);

      games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (scoresdialog),
                                       pos);
      message = g_strdup_printf ("<b>%s</b>\n\n%s",
                                 _("Congratulations!"),
                                 pos == 1 ? _("Your score is the best!") :
                                 _("Your score has made the top ten."));
      games_scores_dialog_set_message (GAMES_SCORES_DIALOG (scoresdialog),
                                       message);
      g_free (message);
    } else {
      games_scores_dialog_set_message (GAMES_SCORES_DIALOG (scoresdialog),
                                       NULL);
    }

    if (endofgame) {
      games_scores_dialog_set_buttons (GAMES_SCORES_DIALOG (scoresdialog),
                                       GAMES_SCORES_QUIT_BUTTON |
                                       GAMES_SCORES_NEW_GAME_BUTTON);
    } else {
      games_scores_dialog_set_buttons (GAMES_SCORES_DIALOG (scoresdialog), 0);
    }
    dialog = scoresdialog;
  }

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  return result;
}

/**
 * log_score
 * @sc: score
 *
 * Description:
 * Enters a score in the high-score table
 *
 * Returns:
 * the position in the high-score table
 **/
static guint
log_score (gint sc)
{
  guint pos = 0;
  gchar *sbuf = NULL;

  if (properties_super_safe_moves ()) {
    sbuf =
      g_strdup_printf ("%s-super-safe",
                       game_config_filename (current_game_config ()));
  } else if (properties_safe_moves ()) {
    sbuf =
      g_strdup_printf ("%s-safe",
                       game_config_filename (current_game_config ()));
  } else {
    sbuf =
      g_strdup_printf ("%s", game_config_filename (current_game_config ()));
  }

  if (sc != 0) {
    games_scores_set_category (highscores, sbuf);
    pos = games_scores_add_plain_score (highscores, (guint32) sc);
  }
  g_free (sbuf);

  return pos;
}

/**
 * kill_player
 *
 * Description:
 * Ends the current game.
 **/
static void
kill_player (void)
{
  game_state = STATE_DEAD;
  play_sound (SOUND_DIE);
  arena[player_xpos][player_ypos] = OBJECT_PLAYER;
  endlev_counter = 0;
  add_aieee_bubble (player_xpos, player_ypos);
  player_animation_dead ();
  set_move_menu_sensitivity (FALSE);
}

/**
 * add_kill
 * @type: robot type
 *
 * Description:
 * registers a robot kill and updates the score
 **/
static void
add_kill (gint type)
{
  gint si;

  if ((game_state == STATE_WAITING) || (game_state == STATE_WTYPE2)) {
    if (type == OBJECT_ROBOT1) {
      si = game_config ()->score_type1_waiting;
      kills += 1;
    } else {
      si = game_config ()->score_type2_waiting;
      kills += 2;
    }
  } else {
    if (type == OBJECT_ROBOT1) {
      si = game_config ()->score_type1;
    } else {
      si = game_config ()->score_type2;
    }
  }

  score += si;
  score_step += si;

  if (game_config ()->safe_score_boundary > 0) {
    while (score_step >= game_config ()->safe_score_boundary) {
      safe_teleports += 1;
      score_step -= game_config ()->safe_score_boundary;
    }
  }

  if (game_config ()->num_robots_per_safe > 0) {
    while (kills >= game_config ()->num_robots_per_safe) {
      safe_teleports += 1;
      kills -= game_config ()->num_robots_per_safe;
    }
  }

  if (safe_teleports > game_config ()->max_safe_teleports) {
    safe_teleports = game_config ()->max_safe_teleports;
  }

  gnobots_statusbar_set (score, current_level + 1, safe_teleports,
                         num_robots1, num_robots2);
}


/**
 * clear_arena
 *
 * Description:
 * clears the arena
 **/
static void
clear_arena (void)
{
  gint i, j;

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      arena[i][j] = OBJECT_NONE;
    }
  }

  num_robots1 = 0;
  num_robots2 = 0;
}

/**
 * load_temp_arena
 *
 * Description:
 * Set up the temporary arena for processing speculative moves. 
 *
 **/
static void
load_temp_arena (void)
{
  gint i, j;

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if (arena[i][j] != OBJECT_PLAYER) {
        temp_arena[i][j] = arena[i][j];
      } else {
        temp_arena[i][j] = OBJECT_NONE;
      }
    }
  }
}

/**
 * check_location
 * @x: x position
 * @y: y position
 *
 * Description:
 * checks for an object at a given location
 *
 * Returns:
 * type of object if present or OBJECT_NONE
 **/
static gint
check_location (gint x, gint y)
{
  if ((x < 0) || (y < 0) || (x >= GAME_WIDTH) || (y >= GAME_HEIGHT)) {
    return OBJECT_NONE;
  }

  return arena[x][y];
}

/**
 * generate_level
 *
 * Description:
 * Creates a new level and populates it with robots
 **/
static void
generate_level (void)
{
  gint i;
  gint xp, yp;

  clear_arena ();

  arena[PLAYER_DEF_XPOS][PLAYER_DEF_YPOS] = OBJECT_PLAYER;
  player_xpos = PLAYER_DEF_XPOS;
  player_ypos = PLAYER_DEF_YPOS;

  num_robots1 = game_config ()->initial_type1 +
    game_config ()->increment_type1 * current_level;

  if (num_robots1 > game_config ()->maximum_type1) {
    num_robots1 = game_config ()->maximum_type1;
  }
  if (num_robots1 > MAX_ROBOTS) {
    current_level = 0;
    num_robots1 = game_config ()->initial_type1;
    message_box (_
                 ("Congratulations, You Have Defeated the Robots!! \nBut Can You do it Again?"));
    play_sound (SOUND_VICTORY);
  }

  num_robots2 = game_config ()->initial_type2 +
    game_config ()->increment_type2 * current_level;

  if (num_robots2 > game_config ()->maximum_type2) {
    num_robots2 = game_config ()->maximum_type2;
  }

  if ((num_robots1 + num_robots2) > MAX_ROBOTS) {
    current_level = 0;
    num_robots1 = game_config ()->initial_type1;
    num_robots2 = game_config ()->initial_type2;
    message_box (_
                 ("Congratulations, You Have Defeated the Robots!! \nBut Can You do it Again?"));
    play_sound (SOUND_VICTORY);
  }

  safe_teleports += game_config ()->free_safe_teleports;

  if (safe_teleports > game_config ()->max_safe_teleports) {
    safe_teleports = game_config ()->max_safe_teleports;
  }

  for (i = 0; i < num_robots1; ++i) {
    while (1) {
      xp = rand () % GAME_WIDTH;
      yp = rand () % GAME_HEIGHT;

      if (check_location (xp, yp) == OBJECT_NONE) {
        arena[xp][yp] = OBJECT_ROBOT1;
        break;
      }
    }
  }


  for (i = 0; i < num_robots2; ++i) {
    while (1) {
      xp = rand () % GAME_WIDTH;
      yp = rand () % GAME_HEIGHT;

      if (check_location (xp, yp) == OBJECT_NONE) {
        arena[xp][yp] = OBJECT_ROBOT2;
        break;
      }
    }
  }

}

/**
 * update_arena
 *
 * Description:
 * Copies the temporary arena into the main game arena
 **/
static void
update_arena (void)
{
  gint i, j;

  num_robots1 = 0;
  num_robots2 = 0;

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {


      if ((temp_arena[i][j] == OBJECT_HEAP) &&
          (push_xpos == i) && (push_ypos == j)) {
        if (arena[i][j] == OBJECT_ROBOT1) {
          add_splat_bubble (i, j);
          play_sound (SOUND_SPLAT);
          push_xpos = push_ypos = -1;
          score += game_config ()->score_type1_splatted;
        }
        if (arena[i][j] == OBJECT_ROBOT2) {
          add_splat_bubble (i, j);
          play_sound (SOUND_SPLAT);
          push_xpos = push_ypos = -1;
          score += game_config ()->score_type2_splatted;
        }
      }


      arena[i][j] = temp_arena[i][j];
      if (arena[i][j] == OBJECT_ROBOT1) {
        num_robots1 += 1;
      } else if (arena[i][j] == OBJECT_ROBOT2) {
        num_robots2 += 1;
      }
    }
  }

  if (arena[player_xpos][player_ypos] != OBJECT_PLAYER) {
    kill_player ();
  } else {
    /* This is in the else statement to catch the case where the last
     * two robots collide on top of the human. Without the "else" this
     * leads to the player being ressurected and winning. */
    if ((num_robots1 + num_robots2) <= 0) {
      game_state = STATE_COMPLETE;
      play_sound (SOUND_YAHOO);
      endlev_counter = 0;
      add_yahoo_bubble (player_xpos, player_ypos);
      reset_player_animation ();
      set_move_menu_sensitivity (FALSE);
    }
  }

  gnobots_statusbar_set (score, current_level + 1, safe_teleports,
                         num_robots1, num_robots2);

}


/**
 * timeout_cb
 * @data: callback data
 *
 * Description:
 * Game timer callback function
 **/
static gint
timeout_cb (void *data)
{
  gint sp;

  animate_game_graphics ();

  clear_game_area ();

  if ((game_state == STATE_TYPE2) || (game_state == STATE_WTYPE2)) {
    move_type2_robots ();
    update_arena ();
    if (game_state == STATE_TYPE2) {
      game_state = STATE_PLAYING;
    } else if (game_state == STATE_WTYPE2) {
      game_state = STATE_WAITING;
    }
  } else if (game_state == STATE_WAITING) {
    remove_splat_bubble ();
    move_robots ();
  } else if (game_state == STATE_COMPLETE) {
    ++endlev_counter;
    if (endlev_counter >= CHANGE_DELAY) {
      ++current_level;
      remove_bubble ();
      reset_player_animation ();
      clear_game_area ();
      generate_level ();
      game_state = STATE_PLAYING;
      set_move_menu_sensitivity (TRUE);
      gnobots_statusbar_set (score, current_level + 1, safe_teleports,
                             num_robots1, num_robots2);
    }
  } else if (game_state == STATE_DEAD) {
    ++endlev_counter;
    if (endlev_counter >= DEAD_DELAY) {
      if (score > 0) {
        sp = log_score (score);
        if (show_scores (sp, TRUE) == GTK_RESPONSE_REJECT) {
          quit_game ();
        }
      }
      start_new_game ();
    }
  }

  return TRUE;
}


/**
 * destroy_game_timer
 *
 * Description:
 * Destroys the game timer
 **/
static void
destroy_game_timer (void)
{
  if (game_timer_id != -1) {
    g_source_remove (game_timer_id);
    game_timer_id = -1;
  }
}


/**
 * create_game_timer
 *
 * Description:
 * Creates the game timer
 **/
static void
create_game_timer (void)
{
  if (game_timer_id != -1) {
    destroy_game_timer ();
  }

  game_timer_id = g_timeout_add (ANIMATION_DELAY, timeout_cb, 0);
}


/**
 * init_game
 *
 * Description:
 * Initialises everything when game first starts up
 **/
void
init_game (void)
{
  create_game_timer ();

  g_signal_connect (GTK_WIDGET (app), "key_press_event",
                    G_CALLBACK (keyboard_cb), 0);

  start_new_game ();
}


/**
 * quit_game
 *
 * Description:
 * Stop animation timeouts and exit.
 **/
void
quit_game (void)
{
  destroy_game_timer ();
  gtk_main_quit ();
}


/**
 * start_new_game
 *
 * Description:
 * Initialises everything needed to start a new game
 **/
void
start_new_game (void)
{
  GameConfig *conf;
  current_level = 0;
  score = 0;
  kills = 0;
  score_step = 0;

  if (game_state == STATE_PLAYING)
    log_score (score);

  conf = game_config ();
  g_return_if_fail (conf != NULL);

  safe_teleports = conf->initial_safe_teleports;

  remove_bubble ();
  reset_player_animation ();
  generate_level ();
  clear_game_area ();

  if (game_config ()->maximum_type2 > 0) {
    gnobots_statusbar_show_both (TRUE);
  } else {
    gnobots_statusbar_show_both (FALSE);
  }

  game_state = STATE_PLAYING;

  gnobots_statusbar_set (score, current_level + 1, safe_teleports,
                         num_robots1, num_robots2);
  set_move_menu_sensitivity (TRUE);
}


/**
 * move_all_robots
 *
 * Description:
 * Moves all of the robots and checks for collisions
 **/
static void
move_all_robots (void)
{
  gint i, j;
  gint nx, ny;

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if ((arena[i][j] == OBJECT_PLAYER) || (arena[i][j] == OBJECT_HEAP)) {
        temp_arena[i][j] = arena[i][j];
      } else {
        temp_arena[i][j] = OBJECT_NONE;
      }
    }
  }

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if ((arena[i][j] == OBJECT_ROBOT1) || (arena[i][j] == OBJECT_ROBOT2)) {
        nx = i;
        ny = j;
        if (player_xpos < nx)
          nx -= 1;
        if (player_xpos > nx)
          nx += 1;
        if (player_ypos < ny)
          ny -= 1;
        if (player_ypos > ny)
          ny += 1;

        if (temp_arena[nx][ny] == OBJECT_HEAP) {
          add_kill (arena[i][j]);
        } else if ((temp_arena[nx][ny] == OBJECT_ROBOT1) ||
                   (temp_arena[nx][ny] == OBJECT_ROBOT2)) {
          add_kill (arena[i][j]);
          add_kill (temp_arena[nx][ny]);
          temp_arena[nx][ny] = OBJECT_HEAP;
        } else {
          temp_arena[nx][ny] = arena[i][j];
        }
      }
    }
  }

}


/**
 * move_type2_robots
 *
 * Description:
 * Makes the extra move for all of the type2 robots
 **/
static void
move_type2_robots (void)
{
  gint i, j;
  gint nx, ny;

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if ((arena[i][j] == OBJECT_PLAYER) ||
          (arena[i][j] == OBJECT_ROBOT1) || (arena[i][j] == OBJECT_HEAP)) {
        temp_arena[i][j] = arena[i][j];
      } else {
        temp_arena[i][j] = OBJECT_NONE;
      }
    }
  }

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if (arena[i][j] == OBJECT_ROBOT2) {
        nx = i;
        ny = j;
        if (player_xpos < nx)
          nx -= 1;
        if (player_xpos > nx)
          nx += 1;
        if (player_ypos < ny)
          ny -= 1;
        if (player_ypos > ny)
          ny += 1;

        if (temp_arena[nx][ny] == OBJECT_HEAP) {
          add_kill (arena[i][j]);
        } else if ((temp_arena[nx][ny] == OBJECT_ROBOT1) ||
                   (temp_arena[nx][ny] == OBJECT_ROBOT2)) {
          add_kill (arena[i][j]);
          add_kill (temp_arena[nx][ny]);
          temp_arena[nx][ny] = OBJECT_HEAP;
        } else {
          temp_arena[nx][ny] = arena[i][j];
        }
      }
    }
  }

}


/**
 * move_robots
 *
 * Description:
 * Starts the process of moving robots
 **/
static void
move_robots (void)
{
  move_all_robots ();

  if (num_robots2 > 0) {
    if (game_state == STATE_WAITING) {
      game_state = STATE_WTYPE2;
    } else if (game_state == STATE_PLAYING) {
      game_state = STATE_TYPE2;
    }
  }

  update_arena ();
}


/**
 * check_safe
 * @x: x position
 * @y: y position
 *
 * Description:
 * checks whether a given location is safe
 *
 * Returns:
 * TRUE if location is safe, FALSE otherwise
 **/
static gboolean
check_safe (gint x, gint y)
{
  static gint temp2_arena[GAME_WIDTH][GAME_HEIGHT];
  static gint temp3_arena[GAME_WIDTH][GAME_HEIGHT];
  gint i, j;
  gint nx, ny;

  if (temp_arena[x][y] != OBJECT_NONE)
    return FALSE;

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if ((temp_arena[i][j] == OBJECT_PLAYER) ||
          (temp_arena[i][j] == OBJECT_HEAP)) {
        temp2_arena[i][j] = temp_arena[i][j];
      } else {
        temp2_arena[i][j] = OBJECT_NONE;
      }
    }
  }

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if ((temp_arena[i][j] == OBJECT_ROBOT1) ||
          (temp_arena[i][j] == OBJECT_ROBOT2)) {
        nx = i;
        ny = j;
        if (x < nx)
          nx -= 1;
        if (x > nx)
          nx += 1;
        if (y < ny)
          ny -= 1;
        if (y > ny)
          ny += 1;

        if ((temp2_arena[nx][ny] == OBJECT_ROBOT1) ||
            (temp2_arena[nx][ny] == OBJECT_ROBOT2) ||
            (temp2_arena[nx][ny] == OBJECT_HEAP)) {
          temp2_arena[nx][ny] = OBJECT_HEAP;
        } else {
          temp2_arena[nx][ny] = temp_arena[i][j];
        }
      }
    }
  }

  if (temp2_arena[x][y] != OBJECT_NONE)
    return FALSE;

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if ((temp2_arena[i][j] == OBJECT_PLAYER) ||
          (temp2_arena[i][j] == OBJECT_HEAP)) {
        temp3_arena[i][j] = temp2_arena[i][j];
      } else {
        temp3_arena[i][j] = OBJECT_NONE;
      }
    }
  }

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if (temp2_arena[i][j] == OBJECT_ROBOT2) {
        nx = i;
        ny = j;
        if (x < nx)
          nx -= 1;
        if (x > nx)
          nx += 1;
        if (y < ny)
          ny -= 1;
        if (y > ny)
          ny += 1;

        if ((temp3_arena[nx][ny] == OBJECT_ROBOT1) ||
            (temp3_arena[nx][ny] == OBJECT_ROBOT2) ||
            (temp3_arena[nx][ny] == OBJECT_HEAP)) {
          temp3_arena[nx][ny] = OBJECT_HEAP;
        } else {
          temp3_arena[nx][ny] = temp2_arena[i][j];
        }
      }
    }
  }

  if (temp3_arena[x][y] != OBJECT_NONE)
    return FALSE;

  return TRUE;
}

/**
 * push_heap
 * @x: x position
 * @y: y position
 * @dx: x direction
 * @dy: y direction
 *
 * Description:
 * pushes a heap in a given direction
 *
 * Returns:
 * TRUE if heap can be pushed, FALSE otherwise
 **/
static gboolean
push_heap (gint x, gint y, gint dx, gint dy)
{
  gint nx = x + dx;
  gint ny = y + dy;

  if (temp_arena[x][y] != OBJECT_HEAP)
    return FALSE;

  if ((nx < 0) || (nx >= GAME_WIDTH) || (ny < 0) || (ny >= GAME_HEIGHT)) {
    return FALSE;
  }

  if (temp_arena[nx][ny] == OBJECT_HEAP)
    return FALSE;

  push_xpos = nx;
  push_ypos = ny;

  temp_arena[nx][ny] = OBJECT_HEAP;
  temp_arena[x][y] = OBJECT_NONE;

  return TRUE;
}


/**
 * try_player_move
 * @dx: x direction
 * @dy: y direction
 *
 * Description:
 * tries to move the player in a given direction
 *
 * Returns:
 * TRUE if the player can move, FALSE otherwise
 **/
static gboolean
try_player_move (gint dx, gint dy)
{
  gint nx, ny;

  nx = player_xpos + dx;
  ny = player_ypos + dy;

  if ((nx < 0) || (nx >= GAME_WIDTH) || (ny < 0) || (ny >= GAME_HEIGHT)) {
    return FALSE;
  }

  load_temp_arena ();

  if (temp_arena[nx][ny] == OBJECT_HEAP) {
    if (game_config ()->moveable_heaps) {
      if (!push_heap (nx, ny, dx, dy)) {
        push_xpos = push_ypos = -1;
        return FALSE;
      }
    } else {
      return FALSE;
    }
  }

  return TRUE;
}

/**
 * safe_move_available
 *
 * Description:
 * checks to see if a safe move was available to the player
 *
 * Returns:
 * TRUE if there is a possible safe move, FALSE otherwise
 **/
static gboolean
safe_move_available (void)
{
  if (try_player_move (-1, -1)) {
    if (check_safe (player_xpos - 1, player_ypos - 1)) {
      return TRUE;
    }
  }
  if (try_player_move (0, -1)) {
    if (check_safe (player_xpos, player_ypos - 1)) {
      return TRUE;
    }
  }
  if (try_player_move (1, -1)) {
    if (check_safe (player_xpos + 1, player_ypos - 1)) {
      return TRUE;
    }
  }

  if (try_player_move (-1, 0)) {
    if (check_safe (player_xpos - 1, player_ypos)) {
      return TRUE;
    }
  }
  if (try_player_move (0, 0)) {
    if (check_safe (player_xpos, player_ypos)) {
      return TRUE;
    }
  }
  if (try_player_move (1, 0)) {
    if (check_safe (player_xpos + 1, player_ypos)) {
      return TRUE;
    }
  }

  if (try_player_move (-1, 1)) {
    if (check_safe (player_xpos - 1, player_ypos + 1)) {
      return TRUE;
    }
  }
  if (try_player_move (0, 1)) {
    if (check_safe (player_xpos, player_ypos + 1)) {
      return TRUE;
    }
  }
  if (try_player_move (1, 1)) {
    if (check_safe (player_xpos + 1, player_ypos + 1)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
 * safe_teleport_available
 *
 * Description:
 * Check for a safe teleport.
 *
 * Returns:
 * TRUE is a safe teleport is possible, FALSE otherwise.
 *
 */
static gboolean
safe_teleport_available (void)
{
  int x, y;

  load_temp_arena ();

  for (x = 0; x < GAME_WIDTH; x++) {
    for (y = 0; y < GAME_HEIGHT; y++) {
      if (check_safe (x, y))
        return TRUE;
    }
  }

  return FALSE;
}

/**
 * player_move
 * @dx: x direction
 * @dy: y direction
 *
 * Description:
 * moves the player in a given direction
 *
 * Returns:
 * TRUE if the player can move, FALSE otherwise
 **/
static gboolean
player_move (gint dx, gint dy)
{
  gint opx, opy;
  gint nx, ny;

  nx = player_xpos + dx;
  ny = player_ypos + dy;

  opx = player_xpos;
  opy = player_ypos;

  if (properties_safe_moves ()) {
    if (!try_player_move (dx, dy)) {
      play_sound (SOUND_BAD);
      return FALSE;
    } else {
      if (!check_safe (nx, ny)) {
        if (properties_super_safe_moves () || safe_move_available ()) {
          play_sound (SOUND_BAD);
          return FALSE;
        }
      }
    }
  } else {
    if (!try_player_move (dx, dy)) {
      play_sound (SOUND_BAD);
      return FALSE;
    }
  }

  player_xpos = nx;
  player_ypos = ny;

  if (temp_arena[player_xpos][player_ypos] == OBJECT_NONE) {
    temp_arena[player_xpos][player_ypos] = OBJECT_PLAYER;
  }

  reset_player_animation ();

  remove_splat_bubble ();

  update_arena ();

  return TRUE;
}


/**
 * random_teleport
 *
 * Description:
 * randomly teleports the player
 *
 * Returns:
 * TRUE if the player can be teleported, FALSE otherwise
 **/
static gboolean
random_teleport (void)
{
  gint xp, yp, ixp, iyp;
  gint i, j;

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if (arena[i][j] != OBJECT_PLAYER) {
        temp_arena[i][j] = arena[i][j];
      } else {
        temp_arena[i][j] = OBJECT_NONE;
      }
    }
  }


  ixp = xp = rand () % GAME_WIDTH;
  iyp = yp = rand () % GAME_HEIGHT;

  while (1) {
    if (temp_arena[xp][yp] == OBJECT_NONE) {
      player_xpos = xp;
      player_ypos = yp;
      temp_arena[player_xpos][player_ypos] = OBJECT_PLAYER;

      reset_player_animation ();

      update_arena ();
      break;
    }

    ++xp;
    if (xp >= GAME_WIDTH) {
      xp = 0;
      ++yp;
      if (yp >= GAME_HEIGHT) {
        yp = 0;
      }
    }

    if ((xp == ixp) && (yp == iyp)) {
      /* This should never happen. */
      message_box (_("There are no teleport locations left!!"));
      return FALSE;
    }
  }

  remove_splat_bubble ();
  play_sound (SOUND_TELEPORT);

  return TRUE;
}


/**
 * safe_teleport
 *
 * Description:
 * teleports the player to safe location
 *
 * Returns:
 * TRUE if player can be teleported, FALSE otherwise
 **/
static gboolean
safe_teleport (void)
{
  gint xp, yp, ixp, iyp;
  gint i, j;

  if (!safe_teleport_available ()) {
    message_box (_("There are no safe locations to teleport to!!"));
    kill_player ();
    return FALSE;
  }

  if (safe_teleports <= 0)
    return random_teleport ();

  for (i = 0; i < GAME_WIDTH; ++i) {
    for (j = 0; j < GAME_HEIGHT; ++j) {
      if (arena[i][j] != OBJECT_PLAYER) {
        temp_arena[i][j] = arena[i][j];
      } else {
        temp_arena[i][j] = OBJECT_NONE;
      }
    }
  }

  ixp = xp = rand () % GAME_WIDTH;
  iyp = yp = rand () % GAME_HEIGHT;

  while (1) {

    if ((temp_arena[xp][yp] == OBJECT_NONE) && check_safe (xp, yp)) {
      player_xpos = xp;
      player_ypos = yp;
      temp_arena[player_xpos][player_ypos] = OBJECT_PLAYER;

      reset_player_animation ();

      safe_teleports -= 1;

      update_arena ();
      break;
    }

    ++xp;
    if (xp >= GAME_WIDTH) {
      xp = 0;
      ++yp;
      if (yp >= GAME_HEIGHT) {
        yp = 0;
      }
    }
  }

  remove_splat_bubble ();
  play_sound (SOUND_TELEPORT);

  return TRUE;
}


/**
 * game_keypress
 * @key: keycode
 *
 * Description:
 * handles keyboard commands
 **/
void
game_keypress (gint key)
{
  if (game_state == STATE_PLAYING) {
    switch (key) {
    case KBD_NW:
      if (player_move (-1, -1)) {
        move_robots ();
      }
      break;
    case KBD_N:
      if (player_move (0, -1)) {
        move_robots ();
      }
      break;
    case KBD_NE:
      if (player_move (1, -1)) {
        move_robots ();
      }
      break;
    case KBD_W:
      if (player_move (-1, 0)) {
        move_robots ();
      }
      break;
    case KBD_STAY:
      if (player_move (0, 0)) {
        move_robots ();
      }
      break;
    case KBD_E:
      if (player_move (1, 0)) {
        move_robots ();
      }
      break;
    case KBD_SW:
      if (player_move (-1, 1)) {
        move_robots ();
      }
      break;
    case KBD_S:
      if (player_move (0, 1)) {
        move_robots ();
      }
      break;
    case KBD_SE:
      if (player_move (1, 1)) {
        move_robots ();
      }
      break;
    case KBD_TELE:
      if (safe_teleport ()) {
        move_robots ();
      }
      break;
    case KBD_RTEL:
      if (random_teleport ()) {
        move_robots ();
      }
      break;
    case KBD_WAIT:
      game_state = STATE_WAITING;
      break;
    }
  }

}

static void
get_dir (int ix, int iy, int *odx, int *ody)
{
  int x, y, idx, idy;
  double dx, dy, angle;
  int octant;
  const int movetable[8][2] = { {-1, 0}, {-1, -1}, {0, -1}, {1, -1},
  {1, 0}, {1, 1}, {0, 1}, {-1, 1}
  };
  x = CLAMP (ix / tile_width, 0, GAME_WIDTH);
  y = CLAMP (iy / tile_height, 0, GAME_HEIGHT);

  /* If we click on our man then we assume we hold. */
  if ((x == player_xpos) && (y == player_ypos)) {
    *odx = 0;
    *ody = 0;
    return;
  }

  /* If the square clicked on is a valid move, go there. */
  idx = x - player_xpos;
  idy = y - player_ypos;
  if ((ABS (idx) < 2) && (ABS (idy) < 2)) {
    *odx = idx;
    *ody = idy;
    return;
  }

  /* Otherwise go in the general direction of the mouse click. */
  dx = ix - (player_xpos + 0.5) * tile_width;
  dy = iy - (player_ypos + 0.5) * tile_height;

  angle = atan2 (dy, dx);

  /* Note the adjustment we have to make (+9, not +8) because atan2's idea 
   * of octants and the ones we want are shifted by PI/8. */
  octant = (((int) floor (8.0 * angle / M_PI) + 9) / 2) % 8;

  *odx = movetable[octant][0];
  *ody = movetable[octant][1];
}

gboolean
mouse_cb (GtkWidget * widget, GdkEventButton * e, gpointer data)
{
  int dx, dy;

  if (game_state != STATE_PLAYING)
    return TRUE;

  get_dir (e->x, e->y, &dx, &dy);

  if (player_move (dx, dy)) {
    move_robots ();
  }

  return TRUE;
}

gboolean
move_cb (GtkWidget * widget, GdkEventMotion * e, gpointer data)
{
  int dx, dy;

  get_dir (e->x, e->y, &dx, &dy);

  set_cursor_by_direction (gtk_widget_get_window (widget), dx, dy);

  return TRUE;
}

/**********************************************************************/
