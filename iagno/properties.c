/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/*
 * Properties.c - Properties and preferences part of iagno
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

#include <libgames-support/games-conf.h>
#include <libgames-support/games-frame.h>
#include <libgames-support/games-files.h>
#include <libgames-support/games-sound.h>
#include <libgames-support/games-runtime.h>

#include "properties.h"
#include "gnothello.h"
#include "othello.h"

#define KEY_TILESET             "tileset"
#define KEY_BLACK_LEVEL         "black_level"
#define KEY_WHITE_LEVEL         "white_level"
#define KEY_QUICK_MOVES         "quick_moves"
#define KEY_ANIMATE             "animate"
#define KEY_ANIMATE_STAGGER     "animate_stagger"
#define KEY_SHOW_GRID           "show_grid"
#define KEY_FLIP_FINAL_RESULTS "flip_final_results"
#define KEY_SOUND               "sound"

#define DEFAULT_TILESET "classic.png"


extern GtkWidget *window;
extern guint black_computer_level;
extern guint white_computer_level;
extern guint computer_speed;
extern guint black_computer_id;
extern guint white_computer_id;
extern gchar *tile_set;
extern gchar *tile_set_tmp;
extern gint8 pixmaps[8][8];
extern gint animate;
extern gint animate_stagger;
extern gint flip_final;
gint show_grid;
gint sound;

guint t_black_computer_level;
guint t_white_computer_level;
gint t_animate;
gint t_quick_moves;
gint t_animate_stagger;
gint t_flip_final;
gint t_grid;

static GamesFileList *theme_file_list = NULL;

static void apply_changes (void);

/*
 * FIXME:
 *	It doesn't abide by the HIG.
 */

static gint
clamp_int (gint input, gint low, gint high)
{
  if (input < low)
    input = low;
  if (input > high)
    input = high;

  return input;
}

void
load_properties (void)
{
  black_computer_level = games_conf_get_integer (NULL, KEY_BLACK_LEVEL, NULL);
  black_computer_level = clamp_int (black_computer_level, 0, 3);

  white_computer_level = games_conf_get_integer (NULL, KEY_WHITE_LEVEL, NULL);
  white_computer_level = clamp_int (white_computer_level, 0, 3);

  if (games_conf_get_boolean (NULL, KEY_QUICK_MOVES, NULL))
    computer_speed = COMPUTER_MOVE_DELAY / 2;
  else
    computer_speed = COMPUTER_MOVE_DELAY;

  if (tile_set)
    g_free (tile_set);

  tile_set = games_conf_get_string_with_default (NULL, KEY_TILESET, DEFAULT_TILESET);

  animate = games_conf_get_integer (NULL, KEY_ANIMATE, NULL);
  animate = clamp_int (animate, 0, 2);

  animate_stagger = games_conf_get_boolean (NULL, KEY_ANIMATE_STAGGER, NULL);

  sound = games_conf_get_boolean (NULL, KEY_SOUND, NULL);
  games_sound_enable (sound);

  show_grid = games_conf_get_boolean (NULL, KEY_SHOW_GRID, NULL);

  flip_final = games_conf_get_boolean (NULL, KEY_FLIP_FINAL_RESULTS, NULL);

  switch (animate) {
  case 0:
    set_animation_speed (100);
    break;
  case 1:
    set_animation_speed (PIXMAP_FLIP_DELAY * 8);
    break;
  default:
  case 2:
    set_animation_speed (PIXMAP_FLIP_DELAY);
    break;
  }
}

static void
reset_properties (void)
{
  black_computer_level = games_conf_get_integer (NULL, KEY_BLACK_LEVEL, NULL);
  black_computer_level = clamp_int (black_computer_level, 0, 3);

  white_computer_level = games_conf_get_integer (NULL, KEY_WHITE_LEVEL, NULL);
  white_computer_level = clamp_int (white_computer_level, 0, 3);
      
  t_black_computer_level = black_computer_level;
  t_white_computer_level = white_computer_level;

  t_quick_moves = games_conf_get_boolean (NULL, KEY_QUICK_MOVES, NULL);

  if (tile_set_tmp)
    g_free (tile_set_tmp);

  tile_set_tmp = games_conf_get_string_with_default (NULL, KEY_TILESET, DEFAULT_TILESET);

  t_animate = animate;
  t_animate_stagger = animate_stagger;
  t_grid = show_grid;
  t_flip_final = flip_final;
}

static void
black_computer_level_select (GtkWidget * widget, gpointer data)
{
  if ((GPOINTER_TO_INT (data) != t_black_computer_level)
      && (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))) {
    t_black_computer_level = GPOINTER_TO_INT (data);
    apply_changes ();
  }
}

static void
white_computer_level_select (GtkWidget * widget, gpointer data)
{
  if ((GPOINTER_TO_INT (data) != t_white_computer_level)
      && (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))) {
    t_white_computer_level = GPOINTER_TO_INT (data);
    apply_changes ();
  }
}

static void
sound_select (GtkWidget * widget, gpointer data)
{
  sound = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  apply_changes ();
}

static void
quick_moves_select (GtkWidget * widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    t_quick_moves = 1;
  else
    t_quick_moves = 0;
  apply_changes ();
}

static void
flip_final_select (GtkWidget * widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    t_flip_final = 1;
  else
    t_flip_final = 0;
  apply_changes ();
}

static void
animate_stagger_select (GtkWidget * widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    t_animate_stagger = 1;
  else
    t_animate_stagger = 0;
  apply_changes ();
}

static void
grid_select (GtkWidget * widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    t_grid = 1;
  else
    t_grid = 0;
  apply_changes ();
}

static void
animate_select (GtkWidget * widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
    t_animate = GPOINTER_TO_INT (data);
  }
  apply_changes ();
}

static void
save_properties (void)
{
  games_conf_set_integer (NULL, KEY_BLACK_LEVEL, black_computer_level);
  games_conf_set_integer (NULL, KEY_WHITE_LEVEL, white_computer_level);

  games_conf_set_boolean (NULL, KEY_QUICK_MOVES, t_quick_moves);

  games_conf_set_string (NULL, KEY_TILESET, tile_set_tmp);

  games_conf_set_integer (NULL, KEY_ANIMATE, animate);

  games_conf_set_boolean (NULL, KEY_ANIMATE_STAGGER, animate_stagger);
  games_conf_set_boolean (NULL, KEY_SHOW_GRID, show_grid);
  games_conf_set_boolean (NULL, KEY_FLIP_FINAL_RESULTS, flip_final);
  games_conf_set_boolean (NULL, KEY_SOUND, sound);
}

static void
apply_changes (void)
{
  guint redraw = 0;

  black_computer_level = t_black_computer_level;
  white_computer_level = t_white_computer_level;

  if (black_computer_id) {
    g_source_remove (black_computer_id);
    black_computer_id = 0;
  }

  if (white_computer_id) {
    g_source_remove (white_computer_id);
    white_computer_id = 0;
  }

  if (t_quick_moves)
    computer_speed = COMPUTER_MOVE_DELAY / 2;
  else
    computer_speed = COMPUTER_MOVE_DELAY;

  if (strcmp (tile_set, tile_set_tmp)) {
    g_free (tile_set);
    tile_set = g_strdup (tile_set_tmp);
    redraw = 1;
  }

  animate = t_animate;

  switch (animate) {
  case 0:
    set_animation_speed (100);
    break;
  case 1:
    set_animation_speed (PIXMAP_FLIP_DELAY * 8);
    break;
  default:
  case 2:
    set_animation_speed (PIXMAP_FLIP_DELAY);
    break;
  }

  animate_stagger = t_animate_stagger;

  flip_final = t_flip_final;

  if (show_grid != t_grid) {
    show_grid = t_grid;
    redraw = 1;
  }

  if (redraw) {
    load_pixmaps ();
    gui_draw_board ();
  }

  games_sound_enable (sound);

  check_computer_players ();

  save_properties ();
}

static gboolean
close_cb (GtkWidget * widget)
{
  gtk_widget_hide (widget);

  return TRUE;
}

void
set_selection (GtkWidget * widget, gpointer data)
{
  gchar *filename;

  if (tile_set_tmp)
    g_free (tile_set_tmp);

  filename = games_file_list_get_nth (theme_file_list,
				      gtk_combo_box_get_active (GTK_COMBO_BOX
								(widget)));

  tile_set_tmp = g_strdup (filename);

  apply_changes ();
}

void
free_str (GtkWidget * widget, void *data)
{
  g_free (data);
}

static GtkWidget *
fill_menu (void)
{
  const char *dir;

  /* FIXME: we need to check that both dname is valid and that
   * games_file_list_new_images returns something. */

  dir = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);

  if (theme_file_list)
    g_object_unref (theme_file_list);

  theme_file_list = games_file_list_new_images (dir, NULL);

  games_file_list_transform_basename (theme_file_list);
 
  return games_file_list_create_widget (theme_file_list, tile_set,
					GAMES_FILE_LIST_REMOVE_EXTENSION |
					GAMES_FILE_LIST_REPLACE_UNDERSCORES);
}

void
show_properties_dialog (void)
{
  GtkWidget *notebook;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *label2;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *vbox, *vbox2;
  GtkWidget *propbox = NULL;
  GtkWidget *option_menu;

  if (propbox) {
    gtk_window_present (GTK_WINDOW (propbox));
    return;
  }

  reset_properties ();

  propbox = gtk_dialog_new_with_buttons (_("Iagno Preferences"),
					 GTK_WINDOW (window),
					 0,
					 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					 NULL);

  gtk_container_set_border_width (GTK_CONTAINER (propbox), 5);
  gtk_box_set_spacing
    (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (propbox))), 2);
  gtk_window_set_resizable (GTK_WINDOW (propbox), FALSE);
  notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 5);
  gtk_container_add
    (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (propbox))),
     notebook);

  label = gtk_label_new (_("Game"));

  vbox = gtk_vbox_new (FALSE, 18);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 18);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

  button = gtk_check_button_new_with_mnemonic (_("_Use quick moves"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				(computer_speed == COMPUTER_MOVE_DELAY / 2));
  g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK
		    (quick_moves_select), NULL);

  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);

  button = gtk_check_button_new_with_mnemonic (_("E_nable sounds"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), sound);
  g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK
		    (sound_select), NULL);

  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);

  frame = games_frame_new (_("Dark"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 0, 1);

  vbox = gtk_vbox_new (FALSE, 6);

  button = gtk_radio_button_new_with_label (NULL, _("Human"));
  if (black_computer_level == 0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (black_computer_level_select), (gpointer) 0);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
					    (GTK_RADIO_BUTTON (button)),
					    _("Level one"));
  if (black_computer_level == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (black_computer_level_select), (gpointer) 1);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
					    (GTK_RADIO_BUTTON (button)),
					    _("Level two"));
  if (black_computer_level == 2)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (black_computer_level_select), (gpointer) 2);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
					    (GTK_RADIO_BUTTON (button)),
					    _("Level three"));
  if (black_computer_level == 3)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (black_computer_level_select), (gpointer) 3);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  frame = games_frame_new (_("Light"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 1, 2, 0, 1);

  vbox = gtk_vbox_new (FALSE, 6);

  button = gtk_radio_button_new_with_label (NULL, _("Human"));
  if (white_computer_level == 0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (white_computer_level_select), (gpointer) 0);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
					    (GTK_RADIO_BUTTON (button)),
					    _("Level one"));
  if (white_computer_level == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (white_computer_level_select), (gpointer) 1);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
					    (GTK_RADIO_BUTTON (button)),
					    _("Level two"));
  if (white_computer_level == 2)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (white_computer_level_select), (gpointer) 2);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
					    (GTK_RADIO_BUTTON (button)),
					    _("Level three"));
  if (white_computer_level == 3)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (white_computer_level_select), (gpointer) 3);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (frame), vbox);


  label = gtk_label_new (_("Appearance"));

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 18);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);

  frame = games_frame_new (_("Animation"));

  vbox = gtk_vbox_new (FALSE, 6);

  button = gtk_radio_button_new_with_label (NULL, _("None"));
  if (animate == 0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (animate_select), (gpointer) 0);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
					    (GTK_RADIO_BUTTON (button)),
					    _("Partial"));
  if (animate == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (animate_select), (gpointer) 1);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
					    (GTK_RADIO_BUTTON (button)),
					    _("Complete"));
  if (animate == 2)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (animate_select), (gpointer) 2);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 0, 1, 0, 1);

  frame = games_frame_new (_("Options"));
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  button = gtk_check_button_new_with_mnemonic (_("_Stagger flips"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				t_animate_stagger);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (animate_stagger_select), NULL);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = gtk_check_button_new_with_mnemonic (_("S_how grid"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), t_grid);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (grid_select), NULL);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = gtk_check_button_new_with_mnemonic (_("_Flip final results"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), t_flip_final);
  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (flip_final_select), NULL);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 12);

  label2 = gtk_label_new_with_mnemonic (_("_Tile set:"));

  gtk_box_pack_start (GTK_BOX (hbox), label2, FALSE, FALSE, 0);

  option_menu = fill_menu ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), option_menu);
  g_signal_connect (G_OBJECT (option_menu), "changed",
		    G_CALLBACK (set_selection), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), option_menu, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  gtk_table_attach_defaults (GTK_TABLE (table), frame, 1, 2, 0, 1);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  g_signal_connect (G_OBJECT (propbox), "response", G_CALLBACK
		    (close_cb), NULL);
  g_signal_connect (G_OBJECT (propbox), "delete_event", G_CALLBACK
		    (close_cb), NULL);

  gtk_widget_show_all (propbox);
}
