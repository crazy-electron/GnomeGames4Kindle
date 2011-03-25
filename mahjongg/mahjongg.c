/* -*- Mode: C; indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8 -*- */

/*
 * GNOME-Mahjongg
 * (C) 1998-1999 the Free Software Foundation
 *
 *
 * Author: Francisco Bustamante
 *
 *
 * http://www.nuclecu.unam.mx/~pancho/
 * pancho@nuclecu.unam.mx
 */

#include <config.h>

#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libgames-support/games-clock.h>
#include <libgames-support/games-conf.h>
#include <libgames-support/games-frame.h>
#include <libgames-support/games-help.h>
#include <libgames-support/games-stock.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-scores-dialog.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-fullscreen-action.h>
#include <libgames-support/games-pause-action.h>
    
#include "mahjongg.h"
#include "drawing.h"
#include "solubility.h"
#include "maps.h"

#define APPNAME "mahjongg"
#define APPNAME_LONG N_("Mahjongg")

#define KEY_TILESET       "tileset"
#define KEY_SHOW_TOOLBAR  "show_toolbar"
#define KEY_BGCOLOUR      "bgcolour"
#define KEY_MAPSET        "mapset"

/* #defines for the tile selection code. */
#define SELECTED_FLAG   1
#define HINT_FLAG       16

/* The number of half-cycles to blink during a hint (less 1) */
#define HINT_BLINK_NUM 5

#define DEFAULT_WIDTH 530
#define DEFAULT_HEIGHT 440

#define DEFAULT_TILESET "default.png"
#define DEFAULT_MAPSET "Easy"

static GtkWidget *window, *statusbar;
static GtkWidget *tiles_label;
static GtkWidget *toolbar;
static GtkWidget *moves_label, *chrono;
static GtkWidget *warn_cb = NULL, *confirm_cb = NULL;
static GtkWidget *colour_well = NULL;
static GtkWidget *pref_dialog = NULL;

static GtkAction *pause_action;
static GtkAction *hint_action;
static GtkAction *redo_action;
static GtkAction *undo_action;
static GtkAction *restart_action;
static GtkAction *scores_action;
static GtkAction *show_toolbar_action;
static GtkAction *fullscreen_action;
static GtkAction *leave_fullscreen_action;

/* Available tilsets */
static GList *tileset_list = NULL;

gchar *tileset = NULL;

tile tiles[MAX_TILES];
static gint visible_tiles; /* Count of how many tiles are active */

tilepos *pos = 0;
static gint mapset = -1, active_mapset = -1;
static gchar *selected_tileset = NULL;
static gint selected_tile;
static gint sequence_number;
static gboolean undo_state = FALSE;
static gboolean redo_state = FALSE;
static gint moves_left = 0; /* The number of moves that can be made */
gboolean paused = FALSE; /* true if the game is paused */

/* Hint animation */
static gint hint_tiles[2];
static guint timer = 0;
static guint timeout_counter = HINT_BLINK_NUM + 1;

static GamesScores *highscores;

enum {
  GAME_RUNNING = 0,
  GAME_WAITING,
  GAME_WON,
  GAME_LOST,
  GAME_DEAD
} game_over = GAME_WAITING;

static gint get_mapset_index (void);
static void clear_undo_queue (void);
void you_won (void);
void check_free (void);
void undo_tile_callback (void);
void properties_callback (void);
void shuffle_tiles_callback (void);
void pause_callback (GamesPauseAction *action);
void new_game (gboolean shuffle);

void
mahjongg_theme_warning (gchar * message)
{
  GtkWidget *dialog;
  GtkWidget *button;

  dialog = gtk_message_dialog_new (GTK_WINDOW (window),
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_WARNING,
				   GTK_BUTTONS_CLOSE,
				   _("Could not load tile set"));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
					    "%s", message);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
				  _("Preferences"), GTK_RESPONSE_ACCEPT);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  g_signal_connect (button, "clicked", G_CALLBACK (properties_callback),
		    NULL);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/* At the end of the game, hint, shuffle and pause all become unavailable. */
static void
update_menu_sensitivities (void)
{
  gtk_action_set_sensitive (pause_action,
			    game_over != GAME_WON &&
			    game_over != GAME_WAITING);
  gtk_action_set_sensitive (restart_action, undo_state);

  if (paused) {
    gtk_action_set_sensitive (hint_action, FALSE);
    gtk_action_set_sensitive (undo_action, FALSE);
    gtk_action_set_sensitive (redo_action, FALSE);
  } else {
    gtk_action_set_sensitive (hint_action, moves_left > 0);
    gtk_action_set_sensitive (undo_action, undo_state && game_over != GAME_WON);
    gtk_action_set_sensitive (redo_action, redo_state);
  }

  /* This is a workaround to make sure that all toolbar elements are
   * shown properly, due to a bug in GtkToolButton in Gtk+. 
   * See Bug #332573 for a description of the workaround.
   */
  gtk_widget_queue_draw (toolbar);
  gtk_widget_queue_resize (toolbar);
  while (gtk_events_pending ())
    gtk_main_iteration ();
  gtk_widget_queue_draw (toolbar);
  gtk_widget_queue_resize (toolbar);
  while (gtk_events_pending ())
    gtk_main_iteration ();

}

static void
clock_start (void)
{
  games_clock_start (GAMES_CLOCK (chrono));
  game_over = GAME_RUNNING;
  update_menu_sensitivities ();
}

/* Undo and redo sensitivity functionality. */
static void
set_undoredo_state (gboolean undo, gboolean redo)
{
  undo_state = undo;
  redo_state = redo;

  update_menu_sensitivities ();
}

static void
tileset_callback (GtkWidget * widget, void *data)
{
  GList *entry;

  entry = g_list_nth (tileset_list,
		      gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));

  games_conf_set_string (NULL, KEY_TILESET, entry->data);
}

static void
conf_value_changed_cb (GamesConf *conf,
                       const char *group,
                       const char *key,
                       gpointer user_data)
{
  if (group != NULL)
    return;

  if (strcmp (key, KEY_TILESET) == 0) {
    char *tile_tmp;

    tile_tmp = games_conf_get_string_with_default (NULL, KEY_TILESET, DEFAULT_TILESET);

    if (strcmp (tile_tmp, selected_tileset) != 0) {
      g_free (selected_tileset);
      selected_tileset = tile_tmp;
      load_images (selected_tileset);
      draw_all_tiles ();
    } else {
      g_free (tile_tmp);
    }
  } else if (strcmp (key, KEY_SHOW_TOOLBAR) == 0) {
    gboolean state;

    state = games_conf_get_boolean (NULL, KEY_SHOW_TOOLBAR, NULL);

    if (state)
      gtk_widget_show (toolbar);
    else
      gtk_widget_hide (toolbar);
  } else if (strcmp (key, KEY_BGCOLOUR) == 0) {
    gchar *colour;

    colour = games_conf_get_string (NULL, KEY_BGCOLOUR, NULL);
    set_background (colour);
    if (colour_well != NULL) {
      gtk_color_button_set_color (GTK_COLOR_BUTTON (colour_well), &bgcolour);
    }
    draw_all_tiles ();
  } else if (strcmp (key, KEY_MAPSET) == 0) {
    GtkWidget *dialog;
    gint response;

    mapset = get_mapset_index ();

    /* Skip the dialog if a game isn't in play. */
    if (game_over || !games_clock_get_seconds (GAMES_CLOCK (chrono))) {
      new_game (TRUE);
      return;
    }

    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_NONE,
                                                _("Do you want to start a new game with this map?"));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                               _("If you continue playing the next game will use the new map."));
    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            _("_Continue playing"), GTK_RESPONSE_REJECT,
                            _("Use _new map"), GTK_RESPONSE_ACCEPT,
                            NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    if (response == GTK_RESPONSE_ACCEPT)
      new_game (TRUE);
    gtk_widget_destroy (dialog);
  }
}

static void
show_tb_callback (void)
{
  gboolean state;

  state =
    gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (show_toolbar_action));

  games_conf_set_boolean (NULL, KEY_SHOW_TOOLBAR, state);
}

static void
bg_colour_callback (GtkWidget * widget, gpointer data)
{
  GdkColor colour;
  char str[64];

  gtk_color_button_get_color (GTK_COLOR_BUTTON (widget), &colour);

  g_snprintf (str, sizeof (str), "#%04x%04x%04x",
              colour.red, colour.green, colour.blue);

  games_conf_set_string (NULL, KEY_BGCOLOUR, str);
}

static gint
get_mapset_index (void)
{
  gchar *mapset_name;
  gint newmapset = -1;
  gint i;

  mapset_name = games_conf_get_string_with_default (NULL, KEY_MAPSET, DEFAULT_MAPSET);
  for (i = 0; i < nmaps; i++)
    if (g_utf8_collate (mapset_name, maps[i].name) == 0)
      newmapset = i;

  if (newmapset == -1) {	/* Oops: name not found. */
    if (mapset > 0)		/* If we have a valid map, use it. */
      newmapset = mapset;
    else			/* Otherwise use the default. */
      newmapset = 0;
  }

  g_free (mapset_name);

  return newmapset;
}

static void
set_map_selection (GtkWidget * widget, void *data)
{
  gint target_mapset;

  target_mapset = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

  games_conf_set_string (NULL, KEY_MAPSET, maps[target_mapset].name);
}

static void
init_config (void)
{
  g_signal_connect (games_conf_get_default (), "value-changed",
                    G_CALLBACK (conf_value_changed_cb), NULL);
}

static void
message (gchar * message)
{
  guint context_id;

  context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "message");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), context_id);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_id, message);
}

static gboolean
message_flash_remove (guint flashid)
{
  guint context_id;

  context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "flash");
  gtk_statusbar_remove (GTK_STATUSBAR (statusbar), context_id, flashid);
  return FALSE;
}

static void
message_flash (gchar * message)
{
  guint flashid;
  guint context_id;

  context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "flash");
  flashid =
    gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_id, message);
  g_timeout_add_seconds (5, (GSourceFunc) message_flash_remove,
		 GUINT_TO_POINTER (flashid));
}

static gint
update_moves_left (void)
{
  char *tmpstr;

  check_free ();
  tmpstr = g_strdup_printf ("%2d", moves_left);
  gtk_label_set_text (GTK_LABEL (moves_label), tmpstr);
  g_free (tmpstr);

  return moves_left;
}

static void
select_tile (gint tileno)
{
  tiles[tileno].selected |= SELECTED_FLAG;
  draw_tile (tileno);
  selected_tile = tileno;
}

static void
unselect_tile (gint tileno)
{
  selected_tile = MAX_TILES + 1;
  tiles[tileno].selected &= ~SELECTED_FLAG;
  draw_tile (tileno);
}

static void
remove_pair (gint tile1, gint tile2)
{
  gchar *tmpstr;
   
  tiles[tile1].visible = tiles[tile2].visible = 0;
  tiles[tile1].selected &= ~SELECTED_FLAG;
  tiles[tile2].selected &= ~SELECTED_FLAG;
  draw_tile (tile1);
  draw_tile (tile2);
  clear_undo_queue ();
  tiles[tile1].sequence = tiles[tile2].sequence = sequence_number;
  sequence_number++;
  selected_tile = MAX_TILES + 1;
  visible_tiles -= 2;
  tmpstr = g_strdup_printf ("%3d", visible_tiles);
  gtk_label_set_text (GTK_LABEL (tiles_label), tmpstr);
  g_free (tmpstr);
  set_undoredo_state (TRUE, FALSE);

  update_moves_left ();   
  if (visible_tiles <= 0) {
    games_clock_stop (GAMES_CLOCK (chrono));
    you_won ();
  }
}

void
tile_event (gint tileno, gint button)
{
  if (paused) {
    pause_callback (NULL);
    return;
  }

  if (!tile_free (tileno))
    return;

  if (!games_clock_get_seconds (GAMES_CLOCK (chrono)))
    clock_start ();

  switch (button) {
  case 1:
    if (tiles[tileno].selected & SELECTED_FLAG) {
      unselect_tile (tileno);
      return;
    }
    if (selected_tile >= MAX_TILES) {
      select_tile (tileno);
      return;
    }
    if ((tiles[selected_tile].type == tiles[tileno].type)) {
      remove_pair (selected_tile, tileno);
      return;
    }
    /* Note the fallthrough, if the tiles don't match,
     * we just select the new tile. */
  case 3:
    if (selected_tile < MAX_TILES)
      unselect_tile (selected_tile);
    select_tile (tileno);

  default:
    break;
  }
}

static void
fill_tile_menu (GtkWidget * menu)
{
  struct dirent *e;
  DIR *dir;
  gint itemno = 0;
  const char *dname = NULL;
  gchar **tileset_name;

  if (tileset_list) {
    g_list_foreach (tileset_list, (GFunc) g_free, NULL);
    g_list_free (tileset_list);
  }

  tileset_list = NULL;

  dname = games_runtime_get_directory(GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  dir = opendir (dname);

  if (!dir) {
    return;
  }

  while ((e = readdir (dir)) != NULL) {
    gchar *s = g_strdup (e->d_name);

    if (!(g_strrstr (s, ".xpm") ||
	  g_strrstr (s, ".svg") ||
	  g_strrstr (s, ".gif") ||
	  g_strrstr (s, ".png") ||
	  g_strrstr (s, ".jpg") || g_strrstr (s, ".xbm"))) {
      g_free (s);
      continue;
    }
      
    tileset_name = g_strsplit (s, ".", -1);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (menu), tileset_name[0]);
    g_strfreev (tileset_name);

    tileset_list = g_list_append (tileset_list, s);
    if (!strcmp (tileset, s)) {
      gtk_combo_box_set_active (GTK_COMBO_BOX (menu), itemno);
    }

    itemno++;
  }

  closedir (dir);
}

static void
fill_map_menu (GtkWidget * menu)
{
  gint lp;

  for (lp = 0; lp < nmaps; lp++) {
    const char *display_name;

    display_name = g_dpgettext2 (NULL, "mahjongg map name", maps[lp].name);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (menu), display_name);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (menu), mapset);
}

void
check_free (void)
{
  gint i;
  gint tile_count[MAX_TILES];

  moves_left = 0;

  for (i = 0; i < MAX_TILES; i++)
    tile_count[i] = 0;

  for (i = 0; i < MAX_TILES; i++) {
    if (tile_free (i))
      tile_count[tiles[i].type]++;
  }

  for (i = 0; i < MAX_TILES; i++)
    moves_left += tile_count[i] >> 1;

  if ((moves_left == 0) && (visible_tiles > 0)) {
    GtkWidget *mb;

    update_menu_sensitivities ();
    if (!game_over) {
      gint response_id;
       
      mb = gtk_message_dialog_new (GTK_WINDOW (window),
				   GTK_DIALOG_MODAL
				   | GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_NONE,
				   (_("There are no more moves.")));
      gtk_dialog_add_buttons (GTK_DIALOG (mb),
			      GTK_STOCK_UNDO, GTK_RESPONSE_REJECT,
                              _("_New game"), GTK_RESPONSE_CANCEL, NULL);

      /* Can only shuffle if two tiles are visible to be matched. This cannot occur
       * if all tiles are in a stack */
      if (visible_tiles >= 2)
        gtk_dialog_add_button(GTK_DIALOG (mb), _("_Shuffle"), GTK_RESPONSE_ACCEPT);

      gtk_dialog_set_default_response (GTK_DIALOG (mb), GTK_RESPONSE_ACCEPT);
      response_id = gtk_dialog_run (GTK_DIALOG (mb));
      switch (response_id) {
      case GTK_RESPONSE_ACCEPT:
	 shuffle_tiles_callback ();
         break;
      case GTK_RESPONSE_CANCEL:
         new_game (TRUE);
         break;
      default:
	 undo_tile_callback ();
         break;
      }       

      gtk_widget_destroy (mb);
    }
  }
}

void
you_won (void)
{
  gint pos;
  time_t seconds;
  static GtkWidget *dialog = NULL;
  gchar *message;

  game_over = GAME_WON;

  seconds = games_clock_get_seconds (GAMES_CLOCK (chrono));

  pos = games_scores_add_time_score (highscores, (seconds / 60) * 1.0 + (seconds % 60) / 100.0);
  update_menu_sensitivities ();
  if (pos > 0) {
    if (dialog) {
      gtk_window_present (GTK_WINDOW (dialog));
    } else {
      dialog = games_scores_dialog_new (GTK_WINDOW (window), highscores, _("Mahjongg Scores"));
      games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
						    (dialog), _("Map:"));
      message =
	g_strdup_printf ("<b>%s</b>\n\n%s", _("Congratulations!"),
                         pos == 1 ? _("Your score is the best!") :
			 _("Your score has made the top ten."));
      games_scores_dialog_set_message (GAMES_SCORES_DIALOG (dialog), message);
      g_free (message);
    }
    games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (dialog), pos);
    /* FIXME: Quit / New Game choice. */

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_hide (dialog);
  }
}

static void
pref_dialog_response (GtkDialog * dialog, gint response, gpointer data)
{
  gtk_widget_destroy (pref_dialog);
  pref_dialog = NULL;
  warn_cb = NULL;
  confirm_cb = NULL;
  colour_well = NULL;
}

void
properties_callback (void)
{
  GtkWidget *omenu;
  GtkWidget *frame, *table, *widget, *label;
  GtkSizeGroup *group;
  GtkWidget *top_table;

  if (pref_dialog) {
    gtk_window_present (GTK_WINDOW (pref_dialog));
    return;
  }

  pref_dialog = gtk_dialog_new_with_buttons (_("Mahjongg Preferences"),
					     GTK_WINDOW (window),
					     GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_CLOSE,
					     GTK_RESPONSE_CLOSE, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (pref_dialog), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))), 2);
  gtk_window_set_resizable (GTK_WINDOW (pref_dialog), FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (pref_dialog),
				   GTK_RESPONSE_CLOSE);
  g_signal_connect (G_OBJECT (pref_dialog), "response",
		    G_CALLBACK (pref_dialog_response), NULL);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  top_table = gtk_table_new (4, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (top_table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (top_table), 18);
  gtk_table_set_col_spacings (GTK_TABLE (top_table), 0);

  frame = games_frame_new (_("Tiles"));
  gtk_table_attach_defaults (GTK_TABLE (top_table), frame, 0, 1, 0, 1);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);

  label = gtk_label_new_with_mnemonic (_("_Tile set:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions) 0, 0, 0);

  omenu = gtk_combo_box_text_new ();
  fill_tile_menu (omenu);
  g_signal_connect (G_OBJECT (omenu), "changed",
		    G_CALLBACK (tileset_callback), NULL);
  gtk_table_attach_defaults (GTK_TABLE (table), omenu, 1, 2, 0, 1);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), omenu);

  gtk_container_add (GTK_CONTAINER (frame), table);

  frame = games_frame_new (_("Maps"));
  gtk_table_attach_defaults (GTK_TABLE (top_table), frame, 0, 1, 1, 2);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);

  label = gtk_label_new_with_mnemonic (_("_Select map:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions) 0, 0, 0);

  omenu = gtk_combo_box_text_new ();
  fill_map_menu (omenu);
  g_signal_connect (G_OBJECT (omenu), "changed",
		    G_CALLBACK (set_map_selection), NULL);
  gtk_table_attach_defaults (GTK_TABLE (table), omenu, 1, 2, 0, 1);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), omenu);

  gtk_container_add (GTK_CONTAINER (frame), table);

  frame = games_frame_new (_("Colors"));
  gtk_table_attach_defaults (GTK_TABLE (top_table), frame, 0, 1, 2, 3);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);

  label = gtk_label_new_with_mnemonic (_("_Background color:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions) 0, 0, 0);

  widget = gtk_color_button_new ();
  gtk_color_button_set_color (GTK_COLOR_BUTTON (widget), &bgcolour);
  g_signal_connect (G_OBJECT (widget), "color_set",
		    G_CALLBACK (bg_colour_callback), NULL);
  gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 0, 1);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);

  gtk_container_add (GTK_CONTAINER (frame), table);

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))),
			       top_table, TRUE, TRUE, 0);

  g_object_unref (group);
  gtk_widget_show_all (pref_dialog);
}

static gint
hint_timeout (gpointer data)
{
  timeout_counter++;

  if (timeout_counter > HINT_BLINK_NUM) {
    if (selected_tile < MAX_TILES)
      tiles[selected_tile].selected = 1;
    return 0;
  }

  tiles[hint_tiles[0]].selected ^= HINT_FLAG;
  tiles[hint_tiles[1]].selected ^= HINT_FLAG;
  draw_tile (hint_tiles[0]);
  draw_tile (hint_tiles[1]);

  return 1;
}

static void
stop_hints (void)
{
  if (timeout_counter > HINT_BLINK_NUM)
    return;

  timeout_counter = HINT_BLINK_NUM + 1;
  tiles[hint_tiles[0]].selected &= ~HINT_FLAG;
  tiles[hint_tiles[1]].selected &= ~HINT_FLAG;
  draw_tile (hint_tiles[0]);
  draw_tile (hint_tiles[1]);
  g_source_remove (timer);
}

static void
hint_callback (void)
{
  gint i, j, type;
  gboolean have_match = FALSE;

  if (paused || (game_over != GAME_RUNNING && game_over != GAME_WAITING))
    return;

  /* This prevents the flashing speeding up if the hint button is
   * pressed multiple times. */
  if (timeout_counter <= HINT_BLINK_NUM)
    return;

  /* Snarfed from check free
   * Tile Free is now _so_ much quicker, it is more elegant to do a
   * British Library search, and safer. */
  /* Note: British Library should probably read British Museum.  */
   
  /* Check if the selected tile has a match */
  if (selected_tile < MAX_TILES) {
    type = tiles[selected_tile].type;
    for (i = 0; i < MAX_TILES && !have_match; i++) {
       if (tiles[i].type == type && i != selected_tile && tile_free (i)) {
         have_match = TRUE;
         hint_tiles[0] = selected_tile;
         hint_tiles[1] = i;
       }
    }
  }

  /* Check if any tiles match */
  for (i = 0; i < MAX_TILES && !have_match; i++) {
    if (tile_free (i)) {
      type = tiles[i].type;
      for (j = 0; j < MAX_TILES && !have_match; j++) {
	if (tiles[j].type == type && i != j && tile_free (j)) {
          have_match = TRUE;
          hint_tiles[0] = i;
          hint_tiles[1] = j;
        }
      }
    }
  }
   
  if (!have_match)
    return;

  /* Clear selection if no part of the hint */
  if (selected_tile < MAX_TILES && hint_tiles[0] != selected_tile) {
    tiles[selected_tile].selected &= ~SELECTED_FLAG;
    draw_tile (selected_tile);
    selected_tile = MAX_TILES + 1;
  }

  tiles[hint_tiles[0]].selected |= HINT_FLAG;
  tiles[hint_tiles[1]].selected |= HINT_FLAG;
  draw_tile (hint_tiles[0]);
  draw_tile (hint_tiles[1]);

  /* This is a good way to test check_free
   * for (i=0;i<MAX_TILES;i++)
   * if (tiles[i].selected == 17)
   * tiles[i].visible = 0 ; */

  timeout_counter = 0;
  timer = g_timeout_add (250, (GSourceFunc) hint_timeout, NULL);

  /* 30s penalty */
  clock_start ();
  games_clock_add_seconds (GAMES_CLOCK (chrono), 30);
}

static void
about_callback (void)
{
  const gchar *authors[] = {
    _("Main game:"),
    "Francisco Bustamante",
    "Max Watson",
    "Heinz Hempe",
    "Michael Meeks",
    "Philippe Chavin",
    "Callum McKenzie",
    "",
    _("Maps:"),
    "Rexford Newbould",
    "Krzysztof Foltman",
    NULL
  };

  const gchar *artists[] = {
    _("Tiles:"),
    "Jonathan Buzzard",
    "Jim Evans",
    "Richard Hoelscher",
    "Gonzalo Odiard",
    "Max Watson",
    NULL
  };

  const gchar *documenters[] = {
    "Eric Baudais",
    NULL
  };
  gchar *license = games_get_license (_("Mahjongg"));

  gtk_show_about_dialog (GTK_WINDOW (window),
                         "program-name", _("Mahjongg"),
			 "version", VERSION,
			 "comments",
			 _("A matching game played with Mahjongg tiles.\n\nMahjongg is a part of GNOME Games."),
			 "copyright", "Copyright \xc2\xa9 1998-2008 Free Software Foundation, Inc.",
			 "license", license,
                         "wrap-license", TRUE,
                         "authors", authors,
                         "artists", artists,
                         "documenters", documenters,
			 "translator-credits", _("translator-credits"),
			 "logo-icon-name", "gnome-mahjongg",
                         "website", "http://www.gnome.org/projects/gnome-games",
                         "website-label", _("GNOME Games web site"),
                         NULL);
  g_free (license);
}

void
pause_callback (GamesPauseAction *action)
{
  static gboolean noloops = FALSE;

  /* The calls to set the menu bar toggle-button will
   * trigger another callback, which will trigger another
   * callback ... this must be stopped. */
  if (noloops)
    return;

  noloops = TRUE;
  stop_hints ();
  if (action)
    paused = games_pause_action_get_is_paused (action);
  draw_all_tiles ();
  update_menu_sensitivities ();
  if (paused) {
    games_clock_stop (GAMES_CLOCK (chrono));
    message (_("Game paused"));
  } else {
    clock_start ();
    message ("");
  }
  noloops = FALSE;
}

static void
scores_callback (GtkAction * action, gpointer data)
{
  static GtkWidget *dialog = NULL;

  if (dialog) {
    gtk_window_present (GTK_WINDOW (dialog));
  } else {
    dialog = games_scores_dialog_new (GTK_WINDOW (window), highscores, _("Mahjongg Scores"));
    games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
						  (dialog), _("Map:"));
  }

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);
}

static void
new_game_cb (GtkAction * action, gpointer data)
{
  new_game (TRUE);
}

static void
restart_game_cb (GtkAction * action, gpointer data)
{
  new_game (FALSE);
}

static void
quit_cb (GObject * object, gpointer data)
{
  gtk_main_quit ();
}

static void
redo_tile_callback (GtkAction * action, gpointer data)
{
  gint i, change;
  char *tmpstr;
  gboolean found;

  if (paused)
    return;
  if (sequence_number > (MAX_TILES / 2))
    return;

  stop_hints ();

  if (selected_tile < MAX_TILES) {
    tiles[selected_tile].selected = 0;
    draw_tile (selected_tile);
    selected_tile = MAX_TILES + 1;
  }
  change = 0;
  for (i = 0; i < MAX_TILES; i++)
    if (tiles[i].sequence == sequence_number) {
      tiles[i].selected = 0;
      tiles[i].visible = 0;
      draw_tile (i);
      visible_tiles--;
      change = 1;
    }
  if (change) {
    if (sequence_number < MAX_TILES)
      sequence_number++;
  }

  tmpstr = g_strdup_printf ("%3d", visible_tiles);
  gtk_label_set_text (GTK_LABEL (tiles_label), tmpstr);

  found = FALSE;
  for (i = 0; i < MAX_TILES; i++) {
    if (tiles[i].sequence == sequence_number) {
      found = TRUE;
      break;
    }
  }

  set_undoredo_state (TRUE, found);

  update_moves_left ();
}

void
undo_tile_callback (void)
{
  gint i;
  char *tmpstr;

  if (paused || game_over == GAME_WON)
    return;
  if (game_over == GAME_LOST)
    game_over = GAME_RUNNING;
  stop_hints ();

  if (sequence_number > 1)
    sequence_number--;
  else
    return;

  if (selected_tile < MAX_TILES) {
    tiles[selected_tile].selected = 0;
    draw_tile (selected_tile);
    selected_tile = MAX_TILES + 1;
  }

  for (i = 0; i < MAX_TILES; i++)
    if (tiles[i].sequence == sequence_number) {
      tiles[i].selected = 0;
      tiles[i].visible = 1;
      visible_tiles++;
      draw_tile (i);
    }

  tmpstr = g_strdup_printf ("%3d", visible_tiles);
  gtk_label_set_text (GTK_LABEL (tiles_label), tmpstr);
  g_free (tmpstr);

  set_undoredo_state (sequence_number > 1, TRUE);
  update_moves_left ();
}

/* You lose your re-do queue when you make a move */
static void
clear_undo_queue (void)
{
  gint lp;

  for (lp = 0; lp < MAX_TILES; lp++)
    if (tiles[lp].sequence >= sequence_number)
      tiles[lp].sequence = 0;
}

static void
load_preferences (void)
{
  gchar *buf;

  mapset = get_mapset_index ();

  buf = games_conf_get_string (NULL, KEY_BGCOLOUR, NULL);
  set_background (buf);
  g_free (buf);

  selected_tileset = games_conf_get_string_with_default (NULL, KEY_TILESET, DEFAULT_TILESET);

  load_images (selected_tileset);
}

void
new_game (gboolean shuffle)
{
  gint i;
  gchar *title;
  const char *display_name;

  /* Reset state */
  game_over = GAME_WAITING;
  sequence_number = 1;
  visible_tiles = MAX_TILES;
  selected_tile = MAX_TILES + 1;
  undo_state = FALSE;
  redo_state = FALSE;
  hint_tiles[0] = MAX_TILES + 1;
  hint_tiles[1] = MAX_TILES + 1;   
  if (timer)
    g_source_remove (timer);
  timer = 0;
  timeout_counter = HINT_BLINK_NUM + 1;
  moves_left = 0;
  paused = FALSE;
  for (i = 0; i < MAX_TILES; i++) {
    tiles[i].visible = 1;
    tiles[i].selected = 0;
    tiles[i].sequence = 0;
  }

  /* Load map */
  if (active_mapset != mapset) {
    active_mapset = mapset;
    pos = maps[mapset].map;
    generate_dependencies ();
    calculate_view_geometry ();
    configure_pixmaps ();
    games_scores_set_category (highscores, maps[mapset].score_name);
  }

  /* Generate layout */
  if (shuffle) {
     generate_game (g_random_int ());
  }
   
  /* Analyse layout */
  update_moves_left ();
  
  /* Update menus */
  update_menu_sensitivities ();

  /* Set window title */
  /* Translators: This is the window title for Mahjongg which contains the map name, e.g. 'Mahjongg - Red Dragon' */

  display_name = g_dpgettext2 (NULL, "mahjongg map name", maps[mapset].name);
  title = g_strdup_printf (_("Mahjongg - %s"), display_name);
  gtk_window_set_title (GTK_WINDOW (window), title);
  g_free (title);
  gtk_label_set_text (GTK_LABEL (tiles_label), MAX_TILES_STR);

  /* Prepare clock */
  games_clock_stop (GAMES_CLOCK (chrono));
  games_clock_reset (GAMES_CLOCK (chrono));

  /* Redraw */
  draw_all_tiles ();
}

void
shuffle_tiles_callback (void)
{
  if (paused || game_over == GAME_DEAD || game_over == GAME_WON)
    return;

  stop_hints ();

  /* Make sure no tiles are selected. */
  if (selected_tile < MAX_TILES) {
    unselect_tile (selected_tile);
  }

  /* Shuffle the tiles - this should always succeed, the option should not be
   * available if they cannot be shuffled */
  if (!shuffle ())
    return;

  draw_all_tiles ();

  game_over = GAME_RUNNING;

  /* 60s penalty */
  games_clock_add_seconds (GAMES_CLOCK (chrono), 60);

  update_moves_left ();
  /* Disable undo/redo after a shuffle. */
  sequence_number = 1;
  clear_undo_queue ();
  set_undoredo_state (FALSE, FALSE);

  update_menu_sensitivities ();
}

static void
help_cb (GtkAction * action, gpointer data)
{
  games_help_display (window, APPNAME, NULL);
}

static const GtkActionEntry actions[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, N_("Start a new game"),
   G_CALLBACK (new_game_cb)},
  {"RestartGame", GAMES_STOCK_RESTART_GAME, NULL, NULL,
   N_("Restart the current game"), G_CALLBACK (restart_game_cb)},
  {"UndoMove", GAMES_STOCK_UNDO_MOVE, NULL, NULL, N_("Undo the last move"),
   G_CALLBACK (undo_tile_callback)},
  {"RedoMove", GAMES_STOCK_REDO_MOVE, NULL, NULL, N_("Redo the last move"),
   G_CALLBACK (redo_tile_callback)},
  {"Hint", GAMES_STOCK_HINT, NULL, NULL, N_("Show a hint"),
   G_CALLBACK (hint_callback)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL,
   G_CALLBACK (scores_callback)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_cb)},
  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (properties_callback)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (help_cb)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_callback)}
};

static const GtkToggleActionEntry toggle_actions[] = {
  {"ShowToolbar", NULL, N_("_Toolbar"), NULL, N_("Show or hide the toolbar"),
   G_CALLBACK (show_tb_callback)}
};

static const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <menuitem action='RestartGame'/>"
  "      <menuitem action='PauseGame'/>"
  "      <separator/>"
  "      <menuitem action='UndoMove'/>"
  "      <menuitem action='RedoMove'/>"
  "      <menuitem action='Hint'/>"
  "      <separator/>"
  "      <menuitem action='Scores'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
  "    </menu>"
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='Fullscreen'/>"
  "      <menuitem action='ShowToolbar'/>"
  "      <separator/>"
  "      <menuitem action='Preferences'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>"
  "    </menu>"
  "  </menubar>"
  "  <toolbar name='Toolbar'>"
  "    <toolitem action='NewGame'/>"
  "    <toolitem action='UndoMove'/>"
  "    <toolitem action='Hint'/>"
  "    <toolitem action='PauseGame'/>"
  "    <toolitem action='LeaveFullscreen'/>"
  "  </toolbar>"
  "</ui>";


static void
create_menus (GtkUIManager * ui_manager)
{
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new ("group");

  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, actions, G_N_ELEMENTS (actions),
				window);
  gtk_action_group_add_toggle_actions (action_group, toggle_actions,
				       G_N_ELEMENTS (toggle_actions), window);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);
  restart_action = gtk_action_group_get_action (action_group, "RestartGame");
  pause_action = GTK_ACTION (games_pause_action_new ("PauseGame"));
  gtk_action_set_is_important (pause_action, TRUE);
  g_signal_connect (G_OBJECT (pause_action), "state-changed", G_CALLBACK (pause_callback), NULL);
  gtk_action_group_add_action_with_accel (action_group, pause_action, NULL);
  hint_action = gtk_action_group_get_action (action_group, "Hint");
  gtk_action_set_is_important (hint_action, TRUE);
  undo_action = gtk_action_group_get_action (action_group, "UndoMove");
  gtk_action_set_is_important (undo_action, TRUE);
  redo_action = gtk_action_group_get_action (action_group, "RedoMove");
  scores_action = gtk_action_group_get_action (action_group, "Scores");
  show_toolbar_action =
    gtk_action_group_get_action (action_group, "ShowToolbar");

  fullscreen_action = GTK_ACTION (games_fullscreen_action_new ("Fullscreen", GTK_WINDOW (window)));
  gtk_action_group_add_action_with_accel (action_group, fullscreen_action, NULL);

  leave_fullscreen_action = GTK_ACTION (games_fullscreen_action_new ("LeaveFullscreen", GTK_WINDOW (window)));
  gtk_action_group_add_action_with_accel (action_group, leave_fullscreen_action, NULL);

  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (show_toolbar_action),
                                games_conf_get_boolean (NULL, KEY_SHOW_TOOLBAR, NULL));  
}

static void
init_scores (void)
{
  int i;

  highscores = games_scores_new ("mahjongg",
                                 NULL, 0,
                                 NULL, NULL,
                                 0 /* default category; FIXMEchpe: was: "Easy" */,
                                 GAMES_SCORES_STYLE_TIME_ASCENDING);

  for (i = 0; i < nmaps; i++) {
    const char *display_name;

    display_name = g_dpgettext2 (NULL, "mahjongg map name", maps[i].name);
    games_scores_add_category (highscores, maps[i].score_name, display_name);
  }
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox;
  GtkWidget *box;
  GtkWidget *board;
  GtkWidget *chrono_label;
  GtkWidget *status_box;
  GtkWidget *group_box;
  GtkWidget *spacer;

  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;

  GOptionContext *context;

  gboolean retval;
  GError *error = NULL;

  if (!games_runtime_init ("mahjongg"))
    return 1;

#ifdef ENABLE_SETGID
  setgid_io_init ();
#endif

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

  g_set_application_name (_(APPNAME_LONG));

  if (!games_conf_initialise (APPNAME)) {
    /* Set the defaults */
    games_conf_set_boolean (NULL, KEY_SHOW_TOOLBAR, TRUE);
    games_conf_set_string (NULL, KEY_TILESET, "postmodern.svg");
    games_conf_set_string (NULL, KEY_MAPSET, "Easy");
    games_conf_set_string (NULL, KEY_BGCOLOUR, "#34385b");
  }

  games_stock_init ();

  gtk_window_set_default_icon_name ("gnome-mahjongg");

  load_maps ();
  init_scores ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), _(APPNAME_LONG));

  gtk_window_set_default_size (GTK_WINDOW (window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
  games_conf_add_window (GTK_WINDOW (window), NULL);

  load_preferences ();

  /* Statusbar for a chrono, Tiles left and Moves left */
  status_box = gtk_hbox_new (FALSE, 10);

  group_box = gtk_hbox_new (FALSE, 0);
  tiles_label = gtk_label_new (_("Tiles Left:"));
  gtk_box_pack_start (GTK_BOX (group_box), tiles_label, FALSE, FALSE, 0);
  spacer = gtk_label_new (" ");
  gtk_box_pack_start (GTK_BOX (group_box), spacer, FALSE, FALSE, 0);
  tiles_label = gtk_label_new (MAX_TILES_STR);
  gtk_box_pack_start (GTK_BOX (group_box), tiles_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_box), group_box, FALSE, FALSE, 0);

  group_box = gtk_hbox_new (FALSE, 0);
  moves_label = gtk_label_new (_("Moves Left:"));
  gtk_box_pack_start (GTK_BOX (group_box), moves_label, FALSE, FALSE, 0);
  spacer = gtk_label_new (" ");
  gtk_box_pack_start (GTK_BOX (group_box), spacer, FALSE, FALSE, 0);
  moves_label = gtk_label_new (MAX_TILES_STR);
  gtk_box_pack_start (GTK_BOX (group_box), moves_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_box), group_box, FALSE, FALSE, 0);

  group_box = gtk_hbox_new (FALSE, 0);
  chrono_label = gtk_label_new (_("Time:"));
  gtk_box_pack_start (GTK_BOX (group_box), chrono_label, FALSE, FALSE, 0);
  spacer = gtk_label_new (" ");
  gtk_box_pack_start (GTK_BOX (group_box), spacer, FALSE, FALSE, 0);
  chrono = games_clock_new ();
  gtk_box_pack_start (GTK_BOX (group_box), chrono, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_box), group_box, FALSE, FALSE, 0);

  /* show the status bar items */
  statusbar = gtk_statusbar_new ();
  ui_manager = gtk_ui_manager_new ();

  games_stock_prepare_for_statusbar_tooltips (ui_manager, statusbar);

  create_menus (ui_manager);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);  
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  box = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (quit_cb), NULL);

  board = create_mahjongg_board ();

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/Toolbar");

  vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), board, TRUE, TRUE, 0);

  gtk_box_pack_end (GTK_BOX (statusbar), status_box, FALSE, FALSE, 6);
  gtk_box_pack_end (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox);

  /* FIXME: get these in the best place (as per the comment below. */
  init_config ();
   
  new_game (TRUE);

  /* Don't leave the keyboard focus on the toolbar */
  gtk_widget_grab_focus (board);

  /* Note: we have to have a layout loaded before here so that the
   * window knows how big to make the tiles. */
  gtk_widget_show_all (window);

  games_fullscreen_action_set_visible_policy (GAMES_FULLSCREEN_ACTION (leave_fullscreen_action), GAMES_FULLSCREEN_ACTION_VISIBLE_ON_FULLSCREEN);
  if (!games_conf_get_boolean (NULL, KEY_SHOW_TOOLBAR, NULL))
    gtk_widget_hide (toolbar);

  message_flash (_("Remove matching pairs of tiles."));

  gtk_main ();

  games_conf_shutdown ();

  games_runtime_shutdown ();

  return 0;
}
