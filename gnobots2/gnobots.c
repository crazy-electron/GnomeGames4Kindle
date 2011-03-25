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

#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <glib.h>

#include <libgames-support/games-conf.h>
#include <libgames-support/games-gridframe.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-scores-dialog.h>
#include <libgames-support/games-stock.h>

#ifdef WITH_SMCLIENT
#include <libgames-support/eggsmclient.h>
#endif /* WITH_SMCLIENT */

#include "gbdefs.h"
#include "statusbar.h"
#include "gameconfig.h"
#include "graphics.h"
#include "menu.h"
#include "sound.h"
#include "properties.h"
#include "game.h"
#include "cursors.h"

/* Minimum sizes. */
#define MINIMUM_TILE_WIDTH   8
#define MINIMUM_TILE_HEIGHT  MINIMUM_TILE_WIDTH

#define DEFAULT_WIDTH 720
#define DEFAULT_HEIGHT 566

#define KEY_GEOMETRY_GROUP "geometry"

/**********************************************************************/
/* Exported Variables                                                 */
/**********************************************************************/
GtkWidget *app = NULL;
GtkWidget *game_area = NULL;
GamesScores *highscores;
/**********************************************************************/


/**********************************************************************/
/* File Static Variables                                              */
/**********************************************************************/
static gchar *cmdline_scenario = NULL;
static gchar *cmdline_config = NULL;
static gint session_xpos = -1;
static gint session_ypos = -1;
/**********************************************************************/


/**********************************************************************/
/* File Static Variables                                              */
/**********************************************************************/
static const GOptionEntry options[] = {
  {"scenario", 's', 0, G_OPTION_ARG_STRING, &cmdline_scenario,
   N_("Set game scenario"), N_("NAME")},
  {"config", 'c', 0, G_OPTION_ARG_STRING, &cmdline_config,
   N_("Set game configuration"), N_("NAME")},
  {"x", 'x', 0, G_OPTION_ARG_INT, &session_xpos,
   N_("Initial window position"), N_("X")},
  {"y", 'y', 0, G_OPTION_ARG_INT, &session_ypos,
   N_("Initial window position"), N_("Y")},
  {NULL},
};

static const GamesScoresCategory scorecats[] = { 
  {"classic_robots", N_("Classic robots")},
  {"classic_robots-safe", N_("Classic robots with safe moves")},
  {"classic_robots-super-safe", N_("Classic robots with super-safe moves")},
  {"nightmare", N_("Nightmare")},
  {"nightmare-safe", N_("Nightmare with safe moves")},
  {"nightmare-super-safe", N_("Nightmare with super-safe moves")},
  {"robots2", N_("Robots2")},
  {"robots2-safe", N_("Robots2 with safe moves")},
  {"robots2-super-safe", N_("Robots2 with super-safe moves")},
  {"robots2_easy", N_("Robots2 easy")},
  {"robots2_easy-safe", N_("Robots2 easy with safe moves")},
  {"robots2_easy-super-safe", N_("Robots2 easy with super-safe moves")},
  {"robots_with_safe_teleport", N_("Robots with safe teleport")},
  {"robots_with_safe_teleport-safe", N_("Robots with safe teleport with safe moves")},
  {"robots_with_safe_teleport-super-safe", N_("Robots with safe teleport with super-safe moves")}
};

/**********************************************************************/


/**********************************************************************/
/* Function Prototypes                                                */
/**********************************************************************/
#ifdef WITH_SMCLIENT
static gint save_state_cb (EggSMClient *, GKeyFile *, gpointer);
static gint quit_sm_cb (EggSMClient *, gpointer);
#endif /* WITH_SMCLIENT */
/**********************************************************************/


/**********************************************************************/
/* Function Definitions                                               */
/**********************************************************************/

#ifdef WITH_SMCLIENT
/**
 * save_state
 * @client: gnome client
 * @phase: phase
 * @save_style: save style
 * @shutdown: shutdown
 * @interact_style: interact style
 * @fast: fast shutdown
 * @client_data: client data
 *
 * Description:
 * saves session info
 *
 * Returns:
 * TRUE on success, FALSE otherwise
 **/
static gint
save_state_cb (EggSMClient *client,
	    GKeyFile* keyfile,
	    gpointer client_data)
{
  char *argv[20];
  int i;
  int xpos, ypos;

  gdk_window_get_origin (gtk_widget_get_window (app), &xpos, &ypos);

  i = 0;
  argv[i++] = (char *) client_data;
  argv[i++] = "-x";
  argv[i++] = g_strdup_printf ("%d", xpos);
  argv[i++] = "-y";
  argv[i++] = g_strdup_printf ("%d", ypos);

  egg_sm_client_set_restart_command (client, i, (const char **) argv);

  g_free (argv[2]);
  g_free (argv[4]);

  return TRUE;
}

static gint
quit_sm_cb (EggSMClient *client,
         gpointer client_data)
{
  quit_game();

  return FALSE;
}
#endif /* WITH_SMCLIENT */

/**
 * main
 * @argc: number of arguments
 * @argv: arguments
 *
 * Description:
 * main
 *
 * Returns:
 * exit code
 **/
int
main (int argc, char *argv[])
{
  GtkWidget *errordialog;
  GtkWidget *vbox, *menubar, *toolbar, *statusbar, *gridframe;
  GtkUIManager *ui_manager;
  GOptionContext *context;
  struct timeval tv;
  gint i;
  gchar *config;
  gboolean retval;
  GError *error = NULL;
#ifdef WITH_SMCLIENT
  EggSMClient *sm_client;
#endif /* WITH_SMCLIENT */

  if (!games_runtime_init ("gnobots2"))
    return 1;

  gettimeofday (&tv, NULL);
  srand (tv.tv_usec);

#ifdef ENABLE_SETGID
  setgid_io_init ();
#endif

  context = g_option_context_new (NULL);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
#ifdef WITH_SMCLIENT
  g_option_context_add_group (context, egg_sm_client_get_option_group ());
#endif /* WITH_SMCLIENT */

  g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }

  g_set_application_name (_("Robots"));

  highscores = games_scores_new ("gnobots2",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 NULL, NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

  games_conf_initialise ("Gnobots2");

  gtk_window_set_default_icon_name ("gnome-robots");

#ifdef WITH_SMCLIENT
  sm_client = egg_sm_client_get ();
  g_signal_connect (sm_client, "save-state",
		    G_CALLBACK (save_state_cb), NULL);
  g_signal_connect (sm_client, "quit",
                    G_CALLBACK (quit_sm_cb), NULL);
#endif /* WITH_SMCLIENT */

  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (app), _("Robots"));

  gtk_window_set_default_size (GTK_WINDOW (app), DEFAULT_WIDTH, DEFAULT_HEIGHT);
  games_conf_add_window (GTK_WINDOW (app), KEY_GEOMETRY_GROUP);

  g_signal_connect (G_OBJECT (app), "delete_event",
		    G_CALLBACK (quit_game), NULL);

  statusbar = gnobots_statusbar_new ();
  ui_manager = gtk_ui_manager_new ();

  games_stock_prepare_for_statusbar_tooltips (ui_manager, statusbar);
  create_game_menus (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (app),
			      gtk_ui_manager_get_accel_group (ui_manager));

  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  toolbar = gtk_ui_manager_get_widget (ui_manager, "/Toolbar");

  make_cursors ();

  game_area = gtk_drawing_area_new ();
  gtk_widget_add_events (game_area, GDK_BUTTON_PRESS_MASK |
			 GDK_POINTER_MOTION_MASK);
  g_signal_connect (G_OBJECT (game_area), "button-press-event",
		    G_CALLBACK (mouse_cb), NULL);
  g_signal_connect (G_OBJECT (game_area), "motion-notify-event",
		    G_CALLBACK (move_cb), NULL);
  g_signal_connect (G_OBJECT (game_area), "configure-event",
		    G_CALLBACK (resize_cb), NULL);
  g_signal_connect (G_OBJECT (game_area), "draw",
		    G_CALLBACK (draw_cb), NULL);

  gridframe = games_grid_frame_new (GAME_WIDTH, GAME_HEIGHT);
  gtk_container_add (GTK_CONTAINER (gridframe), game_area);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), gridframe, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (app), vbox);

  gtk_widget_set_size_request (GTK_WIDGET (game_area),
			       MINIMUM_TILE_WIDTH * GAME_WIDTH,
			       MINIMUM_TILE_HEIGHT * GAME_HEIGHT);

  /* Set the window position if it was set by the session manager */
  if (session_xpos >= 0 && session_ypos >= 0) {
    gtk_window_move (GTK_WINDOW (app), session_xpos, session_ypos);
  }

  gtk_widget_show_all (app);

  if (!load_game_configs ()) {
    /* Oops, no configs, we probably haven't been installed properly. */
    errordialog = gtk_message_dialog_new_with_markup (NULL, 0, GTK_MESSAGE_ERROR,
					  GTK_BUTTONS_OK,
					  "<b>%s</b>\n\n%s",
					  _("No game data could be found."),
					  _
					  ("The program Robots was unable to find any valid game configuration files. Please check that the program is installed correctly."));
    gtk_window_set_resizable (GTK_WINDOW (errordialog), FALSE);
    gtk_dialog_run (GTK_DIALOG (errordialog));
    exit (1);
  }

  load_properties ();

  if (!load_game_graphics ()) {
    /* Oops, no graphics, we probably haven't been installed properly. */
    errordialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (app),
					  GTK_DIALOG_MODAL,
					  GTK_MESSAGE_ERROR,
					  GTK_BUTTONS_OK,
					  "<b>%s</b>\n\n%s",
					  _
					  ("Some graphics files are missing or corrupt."),
					  _
					  ("The program Robots was unable to load all the necessary graphics files. Please check that the program is installed correctly."));
    gtk_dialog_run (GTK_DIALOG (errordialog));
    exit (1);
  }

  connect_toolbar_toggle (toolbar);

  init_sound ();

  init_game ();

  if (cmdline_scenario) {
    set_game_graphics (cmdline_scenario);
  }

  if (cmdline_config) {
    for (i = 0; i < num_game_configs (); ++i) {
      config = game_config_name (i);
      if (!strcmp (cmdline_config, config)) {
	properties_set_config (i);
	g_free (config);
	break;
      }
      g_free (config);
    }
  }

  gtk_main ();

  games_conf_shutdown ();

  games_runtime_shutdown ();

  return 0;
}

/**********************************************************************/
