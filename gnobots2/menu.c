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

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libgames-support/games-help.h>
#include <libgames-support/games-stock.h>
#include <libgames-support/games-fullscreen-action.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-scores-dialog.h>

#include "gbdefs.h"
#include "menu.h"
#include "game.h"
#include "gnobots.h"
#include "properties.h"
#include "gameconfig.h"

GtkAction *scores_action;
GtkAction *pause_action;
GtkAction *teleport_action;
GtkAction *random_action;
GtkAction *wait_action;
GtkAction *toolbar_toggle_action;
GtkAction *fullscreen_action;

/**********************************************************************/
/* Function Prototypes                                                */
/**********************************************************************/
static void new_cb (GtkAction * action, gpointer data);
static void properties_cb (GtkAction * action, gpointer data);
static void scores_cb (GtkAction * action, gpointer data);
void quit_cb (GtkAction * action, gpointer data);
static void about_cb (GtkAction * action, gpointer data);
static void help_cb (GtkAction * action, gpointer data);
static void teleport_cb (GtkAction * action, gpointer data);
static void randteleport_cb (GtkAction * action, gpointer data);
static void wait_cb (GtkAction * action, gpointer data);
static void show_toolbar_cb (GtkAction * action, gpointer data);
/**********************************************************************/

const GtkActionEntry action_entry[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"ViewMenu", NULL, N_("_View")},
  {"MoveMenu", NULL, N_("_Move")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, N_("Start a new game"),
   G_CALLBACK (new_cb)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL, G_CALLBACK (scores_cb)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_cb)},
  {"Teleport", GAMES_STOCK_TELEPORT, N_("_Teleport"), NULL,
   N_("Teleport, safely if possible"), G_CALLBACK (teleport_cb)},
  {"Random", GAMES_STOCK_RTELEPORT, N_("_Random"), NULL,
   N_("Teleport randomly"), G_CALLBACK (randteleport_cb)},
  {"Wait", GTK_STOCK_STOP, N_("_Wait"), NULL, N_("Wait for the robots"),
   G_CALLBACK (wait_cb)},

  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (properties_cb)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (help_cb)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_cb)}
};


const GtkToggleActionEntry toggle_action_entry[] = {
  {"ShowToolbar", NULL, N_("_Toolbar"), NULL, N_("Show or hide the toolbar"),
   G_CALLBACK (show_toolbar_cb)}
};


const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <separator/>"
  "      <menuitem action='Scores'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
  "    </menu>"
  "    <menu action='ViewMenu'>"
  "      <menuitem action='ShowToolbar'/>"
  "      <menuitem action='Fullscreen'/>"
  "    </menu>"
  "    <menu action='MoveMenu'>"
  "      <menuitem action='Teleport'/>"
  "      <menuitem action='Random'/>"
  "      <menuitem action='Wait'/>"
  "    </menu>"
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='Preferences'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>"
  "    </menu>"
  "  </menubar>"
  "  <toolbar name='Toolbar'>"
  "    <toolitem action='NewGame'/>"
  "    <separator/>"
  "    <toolitem action='Teleport'/>"
  "    <toolitem action='Random'/>"
  "    <toolitem action='Wait'/>" "  </toolbar>" "</ui>";

/**********************************************************************/
/* Function Definitions                                               */
/**********************************************************************/

/**
 * new_cb
 * @action: Pointer to action
 * @data: Callback data
 *
 * Description:
 * Callback for new menu entry
 *
 * Returns:
 **/
static void
new_cb (GtkAction * action, gpointer data)
{
  start_new_game ();
}


/**
 * show_toolbar_cb
 * @action: Pointer to action
 * @data: Callback data
 *
 * Description:
 * Callback for toolbar menu entry. 
 * 
 * Returns:
 **/
static void
show_toolbar_cb (GtkAction * action, gpointer data)
{
  gboolean state;

  state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  conf_set_show_toolbar (state);
}


/**
 * properties_cb
 * @action: Pointer to action
 * @data: Callback data
 *
 * Description:
 * Callback for properties menu entry
 *
 * Returns:
 **/
static void
properties_cb (GtkAction * action, gpointer data)
{
  show_properties_dialog ();
}


/**
 * scores_cb
 * @action: Pointer to action
 * @data: Callback data
 *
 * Description:
 * Callback for scores menu entry
 *
 * Returns:
 **/
static void
scores_cb (GtkAction * action, gpointer data)
{
  show_scores (0, FALSE);
}


/**
 * quit_cb
 * @action: Pointer to action
 * @data: Callback data
 *
 * Description:
 * Callback for exit menu entry
 *
 * Returns:
 **/
void
quit_cb (GtkAction * action, gpointer data)
{
  quit_game ();
}


/**
 * help_cb
 * @action: Pointer to action
 * @data: Callback data
 *
 * Description:
 * Callback for help menu entry
 *
 * Returns:
 **/
static void
help_cb (GtkAction * action, gpointer data)
{
  games_help_display (app, "gnobots2", NULL);
}


/**
 * about_cb
 * @action: Pointer to action
 * @data: Callback data
 *
 * Description:
 * Callback for about menu entry
 *
 * Returns:
 **/
static void
about_cb (GtkAction * action, gpointer data)
{
  const gchar *authors[] = { "Mark Rae <m.rae@inpharmatica.co.uk>", NULL };

  const gchar *documenters[] =
    { "Patanjali Somayaji", "Mark Rae <m.rae@inpharmatica.co.uk>", NULL };

  gchar *license = games_get_license (_("Robots"));

  gtk_show_about_dialog (GTK_WINDOW (app),
			 "name", _("Robots"),
			 "version", VERSION,
			 "copyright", "Copyright \xc2\xa9 1998-2008 Mark Rae",
			 "license", license,
		         "website-label", _("GNOME Games web site"),
			 "comments", _("Based on classic BSD Robots.\n\nRobots is a part of GNOME Games."),
			 "authors", authors,
			 "documenters", documenters,
			 "translator-credits", _("translator-credits"),
			 "logo-icon-name", "gnome-robots",
			 "website",
			 "http://www.gnome.org/projects/gnome-games/",
			 "wrap-license", TRUE, NULL);
  g_free (license);
}

static void
teleport_cb (GtkAction * action, gpointer data)
{
  game_keypress (KBD_TELE);
}

static void
randteleport_cb (GtkAction * action, gpointer data)
{
  game_keypress (KBD_RTEL);
}

static void
wait_cb (GtkAction * action, gpointer data)
{
  game_keypress (KBD_WAIT);
}

/**
 * create_game_menus
 * @ap: application pointer
 *
 * Description:
 * Creates the menus for application @ap
 *
 **/
void
create_game_menus (GtkUIManager * ui_manager)
{

  GtkActionGroup *action_group;
  games_stock_init ();

  action_group = gtk_action_group_new ("actions");

  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entry,
				G_N_ELEMENTS (action_entry), app);
  gtk_action_group_add_toggle_actions (action_group, toggle_action_entry,
				       G_N_ELEMENTS (toggle_action_entry),
				       app);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

  scores_action = gtk_action_group_get_action (action_group, "Scores");
  teleport_action = gtk_action_group_get_action (action_group, "Teleport");
  random_action = gtk_action_group_get_action (action_group, "Random");
  wait_action = gtk_action_group_get_action (action_group, "Wait");
  toolbar_toggle_action =
    gtk_action_group_get_action (action_group, "ShowToolbar");
  fullscreen_action = GTK_ACTION (games_fullscreen_action_new ("Fullscreen", GTK_WINDOW (app)));
  gtk_action_group_add_action_with_accel (action_group, fullscreen_action, NULL);

  return;
}

/**********************************************************************/

void
set_move_menu_sensitivity (gboolean state)
{
  gtk_action_set_sensitive (teleport_action, state);
  gtk_action_set_sensitive (random_action, state);
  gtk_action_set_sensitive (wait_action, state);
}

static void
toggle_toolbar_cb (GtkAction * action, GtkWidget * toolbar)
{
  gboolean state;

  state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
  if (state) {
    gtk_widget_hide (toolbar); /* hack to unfocus the toolbar */
    gtk_widget_show (toolbar);
  } else {
    gtk_widget_hide (toolbar);
  }
}

void
connect_toolbar_toggle (GtkWidget * toolbar)
{
  g_signal_connect (toolbar_toggle_action, "activate",
		    G_CALLBACK (toggle_toolbar_cb), toolbar);
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (toolbar_toggle_action),
				properties_show_toolbar ());

  toggle_toolbar_cb (GTK_ACTION (toolbar_toggle_action), toolbar);
}
