/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>,
 *              Guillaume Beland <guillaume.beland@gmail.com>
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <clutter/clutter.h>

#include <libgames-support/games-gtk-compat.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-scores-dialog.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-sound.h>

#include "main.h"
#include "gnibbles.h"
#include "boni.h"
#include "bonus.h"
#include "warpmanager.h"
#include "properties.h"
#include "scoreboard.h"
#include "board.h"
#include "worm.h"

#ifdef GGZ_CLIENT
#include "ggz-network.h"
#endif

GnibblesWorm *worms[NUMWORMS];

GnibblesBoni *boni = NULL;
GnibblesWarpManager *warpmanager;

GdkPixbuf *logo_pixmap = NULL;

GdkPixbuf *wall_pixmaps[11] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL,
  NULL
};

GdkPixbuf *worm_pixmaps[7] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL
};

GdkPixbuf *boni_pixmaps[9] = { NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL
};

extern GtkWidget *drawing_area;

extern GnibblesBoard *board;

extern GnibblesProperties *properties;

extern GnibblesScoreboard *scoreboard;

extern ClutterActor *stage;

static GdkPixbuf *
gnibbles_load_pixmap_file (const gchar * pixmap, gint xsize, gint ysize)
{
  GdkPixbuf *image;
  gchar *filename;
  const char *dirname;

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  filename = g_build_filename (dirname, pixmap, NULL);

  if (!filename) {
    char *message =
      g_strdup_printf (_("Nibbles couldn't find pixmap file:\n%s\n\n"
                       "Please check your Nibbles installation"), pixmap);
    gnibbles_error (message);
    g_free(message);
  }

  image = gdk_pixbuf_new_from_file_at_scale (filename, xsize, ysize, TRUE, NULL);
  g_free (filename);

  return image;
}

void
gnibbles_load_pixmap (gint tilesize)
{
  gchar *bonus_files[] = {
    "blank.svg",
    "diamond.svg",
    "bonus1.svg",
    "bonus2.svg",
    "life.svg",
    "bonus3.svg",
    "bonus4.svg",
    "bonus5.svg",
    "questionmark.svg"
  };

  gchar *small_files[] = {
    "wall-straight-up.svg",
    "wall-straight-side.svg",
    "wall-corner-bottom-left.svg",
    "wall-corner-bottom-right.svg",
    "wall-corner-top-left.svg",
    "wall-corner-top-right.svg",
    "wall-tee-up.svg",
    "wall-tee-right.svg",
    "wall-tee-left.svg",
    "wall-tee-down.svg",
    "wall-cross.svg"
  };

  gchar *worm_files[] = {
    "snake-red.svg",
    "snake-green.svg",
    "snake-blue.svg",
    "snake-yellow.svg",
    "snake-cyan.svg",
    "snake-magenta.svg",
    "snake-grey.svg"
  };

  int i;

  for (i = 0; i < 9; i++) {
    if (boni_pixmaps[i])
      g_object_unref (boni_pixmaps[i]);

    boni_pixmaps[i] = gnibbles_load_pixmap_file (bonus_files[i],
                                                 2 * tilesize, 2 * tilesize);
  }

  for (i = 0; i < 11; i++) {
    if (wall_pixmaps[i])
      g_object_unref (wall_pixmaps[i]);

    wall_pixmaps[i] = gnibbles_load_pixmap_file (small_files[i],
                                                 2 * tilesize, 2 * tilesize);
  }

  for (i = 0; i < 7; i++) {
    if (worm_pixmaps[i])
      g_object_unref (worm_pixmaps[i]);

    worm_pixmaps[i] = gnibbles_load_pixmap_file (worm_files[i],
                                                 tilesize, tilesize);
  }
}

G_GNUC_NORETURN
void
gnibbles_error (gchar *message)
{
  GtkWidget *w = gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                         "%s", message);
  gtk_dialog_run (GTK_DIALOG (w));
  gtk_widget_destroy (GTK_WIDGET (w));
  exit (1);
}

void
gnibbles_load_logo (gint tilesize)
{
  if (logo_pixmap)
    g_object_unref (logo_pixmap);

  logo_pixmap = gnibbles_load_pixmap_file ("gnibbles-logo.svg",
                                           board->width * tilesize,
                                           board->height * tilesize);
}

void
gnibbles_init (void)
{
  gint i;

  if (!board)
    return;

  for (i = 0; i < properties->numworms; i++) {
    if (worms[i])
      gnibbles_worm_destroy (worms[i]);
  }

  gnibbles_scoreboard_clear (scoreboard);

  for (i = 0; i < properties->numworms; i++) {
    worms[i] = gnibbles_worm_new (i);
    gnibbles_scoreboard_register (scoreboard, worms[i],
                   colorval_name (properties->wormprops[i]->color));
  }

  for (i = 0; i < properties->human; i++)
    worms[i]->human = TRUE;

  gnibbles_scoreboard_update (scoreboard);
}

gint
gnibbles_move_worms (void)
{
  gint i, j, olddir;
  gint status = 1, nlives = 1;
  gint *dead;

  dead = g_new (gint, properties->numworms);

  for (i = 0; i < properties->numworms; i++) {
    olddir = worms[i]->direction;
    if (!worms[i]->human) {
      gnibbles_worm_ai_move (worms[i]);
    }
  }

  if (boni->missed > MAXMISSED)
    for (i = 0; i < properties->numworms; i++)
      if (worms[i]->score)
        worms[i]->score--;

  for (i = 0; i < boni->numbonuses; i++) {
    if (!(boni->bonuses[i]->countdown--)) {
      if (boni->bonuses[i]->type == BONUSREGULAR && !boni->bonuses[i]->fake) {
        gnibbles_boni_remove_bonus (boni,
                                    boni->bonuses[i]->x,
                                    boni->bonuses[i]->y);
        boni->missed++;
        gnibbles_board_level_add_bonus (board, 1);
      } else {
        gnibbles_boni_remove_bonus (boni,
                                    boni->bonuses[i]->x,
                                    boni->bonuses[i]->y);
      }
    }
  }

  for (i = 0; i < properties->numworms; i++) {
    dead[i] = !gnibbles_worm_test_move_head (worms[i]);
    status &= !dead[i];
  }

  for (i = 0; i < properties->numworms; i++) {
    if (!dead[i] && worms[i]->lives > 0 && !worms[i]->stop)
      gnibbles_worm_move_tail (worms[i]);
  }

  for (i = 0; i < properties->numworms; i++) {
    if (!dead[i] && worms[i]->lives > 0 && !worms[i]->stop)
      gnibbles_worm_move_head (worms[i]);
  }

  for (i = 0; i < properties->numworms; i++) {
    for (j = 0; j < properties->numworms; j++) {
      if (i != j
          && worms[i]->xhead == worms[j]->xhead
          && worms[i]->yhead == worms[j]->yhead
          && worms[i]->lives > 0
          && worms[j]->lives > 0
          && !worms[i]->stop)
        dead[i] = TRUE;
    }
  }

  for (i = 0; i < properties->numworms; i++) {
    if (dead[i]) {
      if (properties->numworms > 1)
        worms[i]->score *= .7;
      if (!gnibbles_worm_lose_life (worms[i])) {
        /* One of the worms lost one life, but the round continues. */
        gnibbles_worm_reset (worms[i]);
        games_sound_play ("crash");
      }
    }
  }

  if (status & GAMEOVER) {
    games_sound_play ("crash");
    games_sound_play ("gameover");
    return GAMEOVER;
  }

  for (i = 0; i < properties->numworms; i++) {
    if (worms[i]->human && worms[i]->lives <= 0)
      return GAMEOVER;
  }

  for (i = 0; i < properties->numworms; i++) {
    if (worms[i]->lives > 0)
      nlives += 1;
  }

  if (nlives == 1 && (properties->ai + properties->human > 1)) {
    /* There is one player left, the other AI players are dead,
     * and that player has won! */
    return VICTORY;
  } else if (nlives == 0) {
    /* There was only one worm, and it died. */
    return GAMEOVER;
  }
   /* Noone died, so the round can continue. */

  g_free (dead);
  return CONTINUE;
}

gint
gnibbles_get_winner (void)
{
  int i;

  for (i = 0; i < properties->numworms; i++) {
    if (worms[i]->lives > 0) {
      return i;
    }
  }
  return -1;
}

gboolean
gnibbles_keypress_worms (guint keyval)
{
  gint i;
  gint numworms = ggz_network_mode ? 1 : properties->numworms;

  for (i = 0; i < numworms; i++) {
    if (worms[i]->human)
      if (gnibbles_worm_handle_keypress (worms[i], keyval)) {
        return TRUE;
      }
  }

  return FALSE;
}

void
gnibbles_show_scores (GtkWidget * window, gint pos)
{
  static GtkWidget *scoresdialog = NULL;
  gchar *message;

  if (!scoresdialog) {
    scoresdialog = games_scores_dialog_new (GTK_WINDOW (window),
                                            highscores,
                                            _("Nibbles Scores"));
    games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
                                                  (scoresdialog),
                                                  _("Speed:"));
  }
  if (pos > 0) {
    games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (scoresdialog), pos);
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

  gtk_dialog_run (GTK_DIALOG (scoresdialog));
  gtk_widget_hide (scoresdialog);
}

void
gnibbles_log_score (GtkWidget * window)
{
  gint pos;

  if (properties->numworms > 1)
    return;

  if (properties->human != 1)
    return;

  if (properties->startlevel != 1)
    return;

  if (!worms[0]->score)
    return;

  pos = games_scores_add_plain_score (highscores, worms[0]->score);

  gnibbles_show_scores (window, pos);
}


void
gnibbles_add_spec_bonus (gint t_x, gint t_y,
                         gint t_type, gint t_fake, gint t_countdown)
{
  gnibbles_boni_add_bonus_final (boni, t_x, t_y, t_type, t_fake, t_countdown);
}

void
gnibbles_remove_spec_bonus (gint x, gint y)
{
  gnibbles_boni_remove_bonus_final (boni, x, y);
}
