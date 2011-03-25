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

#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgames-support/games-conf.h>
#include <libgames-support/games-gridframe.h>
#include <libgames-support/games-help.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-stock.h>
#include <libgames-support/games-pause-action.h>
#include <libgames-support/games-fullscreen-action.h>

#include "main.h"
#include "properties.h"
#include "gnibbles.h"
#include "bonus.h"
#include "boni.h"
#include "preferences.h"
#include "scoreboard.h"
#include "warp.h"

#include <clutter-gtk/clutter-gtk.h>
#include <clutter/clutter.h>

#include "board.h"
#include "worm.h"

#ifdef GGZ_CLIENT
#include <libgames-support/games-dlg-chat.h>
#include <libgames-support/games-dlg-players.h>
#include "ggz-network.h"
#include <ggz-embed.h>
#endif

#define DEFAULT_WIDTH 650
#define DEFAULT_HEIGHT 520

GtkWidget *window;
GtkWidget *statusbar;
GtkWidget *notebook;
GtkWidget *chat = NULL;

static const GamesScoresCategory scorecats[] = {
{ "4.0", NC_("game speed", "Beginner")            },
{ "3.0", NC_("game speed", "Slow")                },
{ "2.0", NC_("game speed", "Medium")              },
{ "1.0", NC_("game speed", "Fast")                },
{ "4.1", NC_("game speed", "Beginner with Fakes") },
{ "3.1", NC_("game speed", "Slow with Fakes")     },
{ "2.1", NC_("game speed", "Medium with Fakes")   },
{ "1.1", NC_("game speed", "Fast with Fakes")     }
};

GamesScores *highscores;

extern GdkPixbuf *logo_pixmap;

GnibblesProperties *properties;

GnibblesBoard *board;

GnibblesScoreboard *scoreboard;

GtkWidget *clutter_widget;
ClutterActor *stage;

extern GnibblesWorm *worms[];
extern GnibblesBoni *boni;

gint main_id = 0;
gint dummy_id = 0;
gint keyboard_id = 0;
gint add_bonus_id = 0;
gint restart_id = 0;

gint current_level;

static gint add_bonus_cb (gpointer data);

static void render_logo (void);
static gint end_game_cb (GtkAction * action, gpointer data);
static void hide_logo (void);

static GtkAction *new_game_action;
static GtkAction *new_network_action;
static GtkAction *player_list_action;
GtkAction *pause_action;
static GtkAction *end_game_action;
static GtkAction *preferences_action;
static GtkAction *scores_action;
static GtkAction *fullscreen_action;

static ClutterActor *logo;

static void
hide_cursor (void)
{
  clutter_stage_hide_cursor (CLUTTER_STAGE (stage));
}

static void
network_gui_update (void)
{

#ifdef GGZ_CLIENT
  if (ggz_network_mode) {
    gtk_widget_show (chat);
  } else {
    gtk_widget_hide (chat);
  }
  gtk_action_set_visible (new_game_action, !ggz_network_mode);
  gtk_action_set_visible (player_list_action, ggz_network_mode);
  gtk_container_check_resize (GTK_CONTAINER (window));

#endif
}

static void
show_cursor (void)
{
  clutter_stage_show_cursor (CLUTTER_STAGE (stage));
}

gint
game_running (void)
{
  return (main_id || dummy_id || restart_id || games_pause_action_get_is_paused (GAMES_PAUSE_ACTION (pause_action)));
}

static void
on_player_list (void)
{
#ifdef GGZ_CLIENT
  create_or_raise_dlg_players (GTK_WINDOW (window));
#endif
}

/* Avoid a race condition where a redraw is attempted
 * between the window being destroyed and the destroy
 * event being sent. */
static gint
delete_cb (GtkWidget * widget, gpointer data)
{
  if (main_id)
    g_source_remove (main_id);
  if (dummy_id)
    g_source_remove (dummy_id);
  if (restart_id)
    g_source_remove (restart_id);

  return FALSE;
}

static void
quit_cb (GObject * object, gpointer data)
{
  gtk_widget_destroy (window);
}

static void
about_cb (GtkAction * action, gpointer data)
{
  const gchar *authors[] = { "Sean MacIsaac", "Ian Peters", "Andreas Røsdal",
                             "Guillaume Beland", NULL };

  const gchar *documenters[] = { "Kevin Breit", NULL };
  gchar *license = games_get_license (_("Nibbles"));

  gtk_show_about_dialog (GTK_WINDOW (window),
       "program-name", _("Nibbles"),
       "version", VERSION,
       "copyright",
       "Copyright \xc2\xa9 1999-2008 Sean MacIsaac, Ian Peters, Andreas Røsdal"
       "2009 Guillaume Beland",
       "license", license, "comments",
       _("A worm game for GNOME.\n\nNibbles is a part of GNOME Games."),
       "authors", authors,
       "documenters", documenters, "translator-credits",
       _("translator-credits"), "logo-icon-name",
       "gnome-gnibbles", "website",
       "http://www.gnome.org/projects/gnome-games/",
       "website-label", _("GNOME Games web site"),
       "wrap-license", TRUE, NULL);
  g_free (license);
}

static gboolean
key_press_cb (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  hide_cursor ();

  if (!(event->type == CLUTTER_KEY_PRESS))
    return FALSE;

  return gnibbles_keypress_worms (event->key.keyval);
}

static gboolean
configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
  int tilesize, ts_x, ts_y;
  int i;

  /* Compute the new tile size based on the size of the
   * drawing area, rounded down. */
  ts_x = event->width / BOARDWIDTH;
  ts_y = event->height / BOARDHEIGHT;
  if (ts_x * BOARDWIDTH > event->width)
    ts_x--;
  if (ts_y * BOARDHEIGHT > event->height)
    ts_y--;
  tilesize = MIN (ts_x, ts_y);

  gnibbles_load_pixmap (tilesize);
  gnibbles_load_logo (tilesize);

  clutter_actor_set_size (CLUTTER_ACTOR (stage),
                          BOARDWIDTH * tilesize,
                          BOARDHEIGHT * tilesize);
  if (game_running ()) {
    if (board) {
      gnibbles_board_rescale (board, tilesize);
      gnibbles_boni_rescale (boni, tilesize);

      for (i=0; i<properties->numworms; i++)
        gnibbles_worm_rescale (worms[i], tilesize);

      if (warpmanager)
        gnibbles_warpmanager_rescale (warpmanager, tilesize);
    }
  } else {
    if (logo)
      hide_logo ();

    if (board)
      gnibbles_board_rescale (board, tilesize);

    render_logo ();
  }

  /* But, has the tile size changed? */
  if (properties->tilesize == tilesize) {
    /* We must always re-load the logo. */
    gnibbles_load_logo (tilesize);
    return FALSE;
  }

  properties->tilesize = tilesize;
  gnibbles_properties_set_tile_size (tilesize);

  return FALSE;
}

#ifdef GGZ_CLIENT
static gboolean
network_loop (gpointer data)
{
  if (ggz_network_mode) {
    network_move_worms ();
  }
  return TRUE;
}
#endif

static gboolean
new_game_2_cb (GtkWidget * widget, gpointer data)
{
  if (!games_pause_action_get_is_paused (GAMES_PAUSE_ACTION (pause_action))) {
    if (!keyboard_id)
      keyboard_id = g_signal_connect (G_OBJECT (stage),
                                      "key-press-event",
                                      G_CALLBACK (key_press_cb), NULL);
#ifdef GGZ_CLIENT
    if (!main_id && ggz_network_mode && network_is_host ()) {
      main_id = g_timeout_add (GAMEDELAY * (properties->gamespeed + NETDELAY),
                               (GSourceFunc) network_loop, NULL);
    } else
#endif
    if (!main_id && !ggz_network_mode) {
      main_id = g_timeout_add (GAMEDELAY * properties->gamespeed,
                               (GSourceFunc) main_loop, NULL);
    }
#ifdef GGZ_CLIENT
    if (!add_bonus_id && network_is_host ()) {
#else
    if (!add_bonus_id) {
#endif
      add_bonus_id = g_timeout_add (BONUSDELAY *
                                    properties->gamespeed,
                                    (GSourceFunc) add_bonus_cb, NULL);
    }
  }

  dummy_id = 0;

  return FALSE;
}

gboolean
new_game (void)
{
  int i;

  gtk_action_set_sensitive (new_network_action, FALSE);

  if (ggz_network_mode) {
    gtk_action_set_sensitive (pause_action, FALSE);
  } else {
    gtk_action_set_sensitive (pause_action, TRUE);
  }
  gtk_action_set_sensitive (end_game_action, TRUE);
  gtk_action_set_sensitive (preferences_action, !ggz_network_mode);

  if (game_running ()) {
    end_game (FALSE);
    main_id = 0;
  }

  if (ggz_network_mode || !properties->random) {
    current_level = properties->startlevel;
  } else {
    current_level = rand () % MAXLEVEL + 1;
  }

  hide_logo ();
  gnibbles_init ();
  gnibbles_board_level_new (board, current_level);
  gnibbles_board_level_add_bonus (board, 1);

  for (i = 0; i < properties->numworms; i++) {
    if (!clutter_actor_get_stage (worms[i]->actors))
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), worms[i]->actors);
    gnibbles_worm_show (worms[i]);
  }

  games_pause_action_set_is_paused (GAMES_PAUSE_ACTION (pause_action), FALSE);
  gtk_action_set_visible (player_list_action, ggz_network_mode);

  if (restart_id) {
    g_source_remove (restart_id);
    restart_id = 0;
  }

  if (add_bonus_id) {
    g_source_remove (add_bonus_id);
    add_bonus_id = 0;
  }

  if (dummy_id)
    g_source_remove (dummy_id);

  dummy_id = g_timeout_add_seconds (1, (GSourceFunc) new_game_2_cb, NULL);

  network_gui_update ();

  return TRUE;
}

static void
new_game_cb (GtkAction * action, gpointer data)
{
  new_game ();
}

static void
pause_game_cb (GtkAction * action, gpointer data)
{
  if (games_pause_action_get_is_paused (GAMES_PAUSE_ACTION (action))) {
    if (main_id || restart_id || dummy_id) {
      if (main_id) {
        g_source_remove (main_id);
        main_id = 0;
      }
      if (keyboard_id) {
        g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);
        keyboard_id = 0;
      }
      if (add_bonus_id) {
        g_source_remove (add_bonus_id);
        add_bonus_id = 0;
      }
    }
  }
  else {
    dummy_id = g_timeout_add (500, (GSourceFunc) new_game_2_cb, NULL);
  }
}

static void
show_scores_cb (GtkAction * action, gpointer data)
{
  gnibbles_show_scores (window, 0);
}

void
end_game (gboolean show_splash)
{
  if (main_id) {
    g_source_remove (main_id);
    main_id = 0;
  }

  if (keyboard_id) {
    g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);
    keyboard_id = 0;
  }

  if (add_bonus_id) {
    g_source_remove (add_bonus_id);
    add_bonus_id = 0;
  }

  if (dummy_id) {
    g_source_remove (dummy_id);
    dummy_id = 0;
  }

  if (restart_id) {
    g_source_remove (restart_id);
    restart_id = 0;
  }

  if (show_splash) {
    render_logo ();
    gtk_action_set_sensitive (new_network_action, TRUE);
    gtk_action_set_sensitive (pause_action, FALSE);
    gtk_action_set_sensitive (end_game_action, FALSE);
    gtk_action_set_sensitive (preferences_action, TRUE);
  }

  network_gui_update ();
  games_pause_action_set_is_paused (GAMES_PAUSE_ACTION (pause_action), FALSE);
}

static gboolean
end_game_cb (GtkAction * action, gpointer data)
{

#ifdef GGZ_CLIENT
  if (ggz_network_mode) {
    ggz_embed_leave_table ();
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), NETWORK_PAGE);
  }
#endif

  end_game (TRUE);
  return FALSE;
}

static gboolean
add_bonus_cb (gpointer data)
{
  gnibbles_board_level_add_bonus (board, 0);
  return TRUE;
}

static gboolean
restart_game (gpointer data)
{
  int i;

  gnibbles_board_level_new (board, current_level);
  gnibbles_board_level_add_bonus (board, 1);

  for (i = 0; i < properties->numworms; i++) {
    if (!clutter_actor_get_stage (worms[i]->actors))
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), worms[i]->actors);
    gnibbles_worm_show (worms[i]);
  }

  for (i = 0; i < properties->human; i++)
    worms[i]->human = TRUE;

  dummy_id = g_timeout_add_seconds (1, (GSourceFunc) new_game_2_cb, NULL);
  restart_id = 0;

  return FALSE;
}

static void
end_game_anim_cb (ClutterAnimation *animation, ClutterActor *actor)
{
  if (!restart_id)
    end_game (TRUE);
}

static void
animate_end_game (void)
{
  int i;
  for (i = 0; i < properties->numworms; i++) {
    clutter_actor_animate (worms[i]->actors, CLUTTER_EASE_IN_QUAD, 500,
                           "opacity", 0,
                           "scale-x", 0.4, "scale-y", 0.4,
                           "fixed::scale-center-x",
                           (gfloat) worms[i]->xhead * properties->tilesize,
                           "fixed::scale-center-y",
                           (gfloat) worms[i]->yhead * properties->tilesize,
                           NULL);
  }

  for ( i = 0; i < boni->numbonuses; i++) {
    clutter_actor_animate (boni->bonuses[i]->actor, CLUTTER_EASE_IN_QUAD, 500,
                           "opacity", 0,
                           "scale-x", 0.4, "scale-y", 0.4,
                           "fixed::scale-gravity", CLUTTER_GRAVITY_CENTER,
                           NULL);

  }

  g_signal_connect_after (
    clutter_actor_animate (board->level, CLUTTER_EASE_IN_QUAD, 700,
                           "scale-x", 0.4, "scale-y", 0.4,
                           "fixed::scale-gravity", CLUTTER_GRAVITY_CENTER,
                           "opacity", 0,
                           NULL),
    "completed", G_CALLBACK (end_game_anim_cb), NULL);

}

gboolean
main_loop (gpointer data)
{
  gint status;
  gint tmp, winner;
  gchar *str = NULL;

  status = gnibbles_move_worms ();
  gnibbles_scoreboard_update (scoreboard);

  if (status == VICTORY) {
    end_game (TRUE);
    winner = gnibbles_get_winner ();

    if (winner == -1)
      return FALSE;

    str = g_strdup_printf (_("Game over! The game has been won by %s!"),
                             names[winner]);
#ifdef GGZ_CLIENT
    add_chat_text (str);
#endif
    g_free (str);

    if (keyboard_id) {
      g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);
      keyboard_id = 0;
    }
    if (main_id) {
      g_source_remove (main_id);
      main_id = 0;
    }
    if (add_bonus_id)
      g_source_remove (add_bonus_id);

    add_bonus_id = 0;

    animate_end_game ();
    gnibbles_log_score (window);

    return FALSE;
  }

  if (status == GAMEOVER) {

    if (keyboard_id) {
      g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);
      keyboard_id = 0;
    }
    main_id = 0;
    if (add_bonus_id)
      g_source_remove (add_bonus_id);

    add_bonus_id = 0;

    animate_end_game ();
    gnibbles_log_score (window);

    return FALSE;
  }

  if (status == NEWROUND) {
#ifdef GGZ_CLIENT
    if (ggz_network_mode) {
      end_game (TRUE);
      add_chat_text (_("The game is over."));
      return FALSE;
    }
#endif

    if (keyboard_id) {
      g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);
      keyboard_id = 0;
    }
    if (add_bonus_id)
      g_source_remove (add_bonus_id);

    if (main_id) {
      g_source_remove (main_id);
      main_id = 0;
    }
    add_bonus_id = 0;

    animate_end_game ();
    restart_id = g_timeout_add_seconds (1, (GSourceFunc) restart_game, NULL);

    return FALSE;
  }

  if (boni->numleft == 0) {
    if (restart_id)
      return TRUE;

    if (keyboard_id)
      g_signal_handler_disconnect (G_OBJECT (stage), keyboard_id);

    keyboard_id = 0;

    if (add_bonus_id)
      g_source_remove (add_bonus_id);

    add_bonus_id = 0;
    if (main_id) {
      g_source_remove (main_id);
      main_id = 0;
    }
    if ((current_level < MAXLEVEL) && (!properties->random
                                       || ggz_network_mode)) {
      current_level++;
    } else if (properties->random && !ggz_network_mode) {
      tmp = rand () % MAXLEVEL + 1;
      while (tmp == current_level)
        tmp = rand () % MAXLEVEL + 1;
      current_level = tmp;
    }
    animate_end_game ();
    restart_id = g_timeout_add_seconds (1, (GSourceFunc) restart_game, NULL);
    return FALSE;
  }

  return TRUE;
}

static gboolean
show_cursor_cb (GtkWidget * widget, GdkEventMotion *event, gpointer data)
{
  show_cursor ();
  return FALSE;
}

static void
help_cb (GtkAction * action, gpointer data)
{
  games_help_display (window, "gnibbles", NULL);
}

static const GtkActionEntry action_entry[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"ViewMenu", NULL, N_("_View")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL,
   G_CALLBACK (new_game_cb)},
#ifdef GGZ_CLIENT
  {"NewNetworkGame", GAMES_STOCK_NETWORK_GAME, NULL, NULL, NULL,
   G_CALLBACK (on_network_game)},
#else
  {"NewNetworkGame", GAMES_STOCK_NETWORK_GAME, NULL, NULL, NULL, NULL},
#endif
  {"PlayerList", GAMES_STOCK_PLAYER_LIST, NULL, NULL, NULL,
   G_CALLBACK (on_player_list)},
  {"EndGame", GAMES_STOCK_END_GAME, NULL, NULL, NULL,
   G_CALLBACK (end_game_cb)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL,
   G_CALLBACK (show_scores_cb)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_cb)},
  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (gnibbles_preferences_cb)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (help_cb)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_cb)}
};

static const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <menuitem action='NewNetworkGame'/>"
  "      <menuitem action='PlayerList'/>"
  "      <menuitem action='EndGame'/>"
  "      <separator/>"
  "      <menuitem action='Pause'/>"
  "      <separator/>"
  "      <menuitem action='Scores'/>"
  "      <separator/>"
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
  "      <menuitem action='About'/>"
  "    </menu>"
  "  </menubar>"
  "</ui>";

static void
create_menus (GtkUIManager * ui_manager)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new ("MenuActions");

  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entry,
                                G_N_ELEMENTS (action_entry), window);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

  new_game_action = gtk_action_group_get_action (action_group, "NewGame");
  scores_action = gtk_action_group_get_action (action_group, "Scores");
  end_game_action = gtk_action_group_get_action (action_group, "EndGame");
  pause_action = GTK_ACTION (games_pause_action_new ("Pause"));
  g_signal_connect (G_OBJECT (pause_action), "state-changed", G_CALLBACK (pause_game_cb), NULL);
  gtk_action_group_add_action_with_accel (action_group, pause_action, NULL);

  preferences_action = gtk_action_group_get_action (action_group,
                                                    "Preferences");
  fullscreen_action = GTK_ACTION (games_fullscreen_action_new ("Fullscreen", GTK_WINDOW(window)));
  gtk_action_group_add_action_with_accel (action_group, fullscreen_action, NULL);
  new_network_action = gtk_action_group_get_action (action_group,
                                                    "NewNetworkGame");
#ifndef GGZ_CLIENT
  gtk_action_set_sensitive (new_network_action, FALSE);
#endif
  player_list_action = gtk_action_group_get_action (action_group, "PlayerList");

}

static void
setup_window (void)
{
  GtkWidget *vbox;
  GtkWidget *packing;
  GtkWidget *menubar;

  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  ClutterColor stage_color = {0x00,0x00,0x00,0xff};

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  clutter_widget = gtk_clutter_embed_new ();
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter_widget));

  clutter_stage_set_color (CLUTTER_STAGE(stage), &stage_color);

  clutter_actor_set_size (CLUTTER_ACTOR (stage),
                          properties->tilesize * BOARDWIDTH,
                          properties->tilesize * BOARDHEIGHT);
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), FALSE);

  board = gnibbles_board_new ();

  gtk_window_set_title (GTK_WINDOW (window), _("Nibbles"));

  gtk_window_set_default_size (GTK_WINDOW (window),
                               DEFAULT_WIDTH, DEFAULT_HEIGHT);
  games_conf_add_window (GTK_WINDOW (window), KEY_PREFERENCES_GROUP);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (delete_cb), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);

  games_stock_init ();
  ui_manager = gtk_ui_manager_new ();
  create_menus (ui_manager);
  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

  packing = games_grid_frame_new (BOARDWIDTH, BOARDHEIGHT);
  gtk_widget_show (packing);

  gtk_container_add (GTK_CONTAINER (packing), clutter_widget);

#ifdef GGZ_CLIENT
  chat = create_chat_widget ();
  gtk_box_pack_start (GTK_BOX (vbox), chat, FALSE, TRUE, 0);
#endif

  g_signal_connect (G_OBJECT (clutter_widget), "configure_event",
                    G_CALLBACK (configure_event_cb), NULL);

  g_signal_connect (G_OBJECT (window), "focus_out_event",
                    G_CALLBACK (show_cursor_cb), NULL);

  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), packing, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), MAIN_PAGE);

  statusbar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_show_all (window);
#ifdef GGZ_CLIENT
  gtk_widget_hide (chat);
#endif

  scoreboard = gnibbles_scoreboard_new (statusbar);
}

static void
render_logo (void)
{
  ClutterActor *image;
  ClutterActor *text, *text_shadow;
  ClutterActor *desc, *desc_shadow;
  ClutterColor actor_color = {0xff,0xff,0xff,0xff};
  ClutterColor shadow_color = {0x00, 0x00, 0x00, 0x88};
  ClutterActor *text_group;
  static gint width, height;
  gint size;
  gfloat stage_w, stage_h;
  PangoFontDescription *pfd;
  PangoLayout *layout;
  PangoContext *context;
  GError *error = NULL;

  gchar *nibbles = _("Nibbles");
  /* Translators: This string will be included in the intro screen, so don't make sure it fits! */
  gchar *description = _("A worm game for GNOME.");

  logo = clutter_group_new ();
  text_group = clutter_group_new ();

  if (!logo_pixmap)
    gnibbles_load_logo (properties->tilesize);

  image = gtk_clutter_texture_new ();
  gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (image), logo_pixmap, &error);

  stage_w = board->width * properties->tilesize;
  stage_h = board->height * properties->tilesize;

  clutter_actor_set_size (CLUTTER_ACTOR (image), stage_w, stage_h);

  clutter_actor_set_position (CLUTTER_ACTOR (image), 0, 0);
  clutter_actor_show (image);

  text = clutter_text_new ();
  clutter_text_set_color (CLUTTER_TEXT (text), &actor_color);

  context = gdk_pango_context_get ();
  layout = clutter_text_get_layout (CLUTTER_TEXT (text));
  pfd = pango_context_get_font_description (context);
  size = pango_font_description_get_size (pfd);

  pango_font_description_set_size (pfd, (size * stage_w) / 100);
  pango_font_description_set_family (pfd, "Sans");
  pango_font_description_set_weight(pfd, PANGO_WEIGHT_BOLD);
  pango_layout_set_font_description (layout, pfd);
  pango_layout_set_text (layout, nibbles, -1);
  pango_layout_get_pixel_size (layout, &width, &height);

  text_shadow = clutter_text_new ();
  clutter_text_set_color (CLUTTER_TEXT (text_shadow), &shadow_color);

  layout = clutter_text_get_layout (CLUTTER_TEXT (text_shadow));
  pango_layout_set_font_description (layout, pfd);
  pango_layout_set_text (layout, nibbles, -1);

  clutter_actor_set_position (CLUTTER_ACTOR (text),
                              (stage_w - width) * 0.5 ,
                              stage_h * .72);
  clutter_actor_set_position (CLUTTER_ACTOR (text_shadow),
                              (stage_w - width) * 0.5 + 5,
                              stage_h * .72 + 5);

  desc = clutter_text_new ();
  layout = clutter_text_get_layout (CLUTTER_TEXT (desc));

  clutter_text_set_color (CLUTTER_TEXT (desc), &actor_color);
  pango_font_description_set_size (pfd, (size * stage_w) / 400);
  pango_layout_set_font_description (layout, pfd);
  pango_layout_set_text (layout, description, -1);
  pango_layout_get_pixel_size(layout, &width, &height);

  desc_shadow = clutter_text_new ();
  layout = clutter_text_get_layout (CLUTTER_TEXT (desc_shadow));
  clutter_text_set_color (CLUTTER_TEXT (desc_shadow), &shadow_color);

  pango_font_description_set_size (pfd, (size * stage_w) / 400);
  pango_layout_set_font_description (layout, pfd);
  pango_layout_set_text (layout, description, -1);

  clutter_actor_set_position (CLUTTER_ACTOR (desc),
                              (stage_w - width) * 0.5,
                              stage_h* .93);
  clutter_actor_set_position (CLUTTER_ACTOR (desc_shadow),
                              (stage_w - width) * 0.5 + 3,
                              stage_h * .93 + 3);

  clutter_container_add (CLUTTER_CONTAINER (text_group),
                         CLUTTER_ACTOR (text_shadow),
                         CLUTTER_ACTOR (text),
                         CLUTTER_ACTOR (desc_shadow),
                         CLUTTER_ACTOR (desc),
                         NULL);
  clutter_container_add (CLUTTER_CONTAINER (logo),
                         CLUTTER_ACTOR (image),
                         CLUTTER_ACTOR (text_group),
                         NULL);

  clutter_actor_set_opacity (CLUTTER_ACTOR (text_group), 0);
  clutter_actor_set_scale (CLUTTER_ACTOR (text_group), 0.0, 0.0);
  clutter_actor_animate (text_group, CLUTTER_EASE_OUT_CIRC, 800,
                          "opacity", 0xff,
                          "scale-x", 1.0,
                          "scale-y", 1.0,
                          "fixed::scale-center-y", stage_w / 2,
                          "fixed::scale-center-x", stage_h / 2,
                          NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               CLUTTER_ACTOR (logo));
}

static void
on_hide_logo_completed (ClutterAnimation *animation, ClutterActor *actor)
{
  clutter_actor_hide (actor);
  clutter_group_remove_all (CLUTTER_GROUP (actor));
}

static void
hide_logo (void)
{
  g_signal_connect_after (
    clutter_actor_animate (logo, CLUTTER_EASE_IN_QUAD, 150,
                           "opacity", 0,
                           NULL),
    "completed", G_CALLBACK (on_hide_logo_completed), logo);
}

int
main (int argc, char **argv)
{
  GOptionContext *context;
  gboolean retval;
  GError *error = NULL;

  if (!games_runtime_init ("gnibbles"))
    return 1;

#ifdef ENABLE_SETGID
  setgid_io_init ();
#endif

  g_set_application_name (_("Nibbles"));

  if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS) {
    GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_NONE,
                                                "%s", "Unable to initialize Clutter.");
    gtk_window_set_title (GTK_WINDOW (dialog), g_get_application_name ());
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    exit (1);
  }

  context = g_option_context_new (NULL);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }

  gtk_window_set_default_icon_name ("gnome-gnibbles");
  srand (time (NULL));

  highscores = games_scores_new ("gnibbles",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 "game speed", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

  games_conf_initialise ("Gnibbles");
  properties = gnibbles_properties_new ();
  setup_window ();
  gnibbles_load_pixmap (properties->tilesize);
  gnibbles_load_logo (properties->tilesize);

#ifdef GGZ_CLIENT
  network_init ();
  network_gui_update ();
#endif

  gtk_action_set_sensitive (pause_action, FALSE);
  gtk_action_set_sensitive (end_game_action, FALSE);
  gtk_action_set_visible (new_game_action, !ggz_network_mode);
  gtk_action_set_visible (player_list_action, ggz_network_mode);

  gtk_main ();

  gnibbles_properties_destroy (properties);
  games_conf_shutdown ();

  games_runtime_shutdown ();

  return 0;
}
