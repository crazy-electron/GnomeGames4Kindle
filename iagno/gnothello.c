/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/*
 * gnothello.c - Main GUI part of iagno
 * written by Ian Peters <itp@gnu.org>
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
#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libgames-support/games-conf.h>
#include <libgames-support/games-help.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-stock.h>

#ifdef WITH_SMCLIENT
#include <libgames-support/eggsmclient.h>
#endif /* WITH_SMCLIENT */

#ifdef GGZ_CLIENT
#include <libgames-support/games-dlg-chat.h>
#include <libgames-support/games-dlg-players.h>
#include "ggz-network.h"
#include <ggz-embed.h>
#endif /* WITH GGZ_CLIENT */

#include "gnothello.h"
#include "othello.h"
#include "properties.h"

#define APP_NAME "iagno"
#define APP_NAME_LONG N_("Iagno")

GtkWidget *window;
GtkWidget *statusbar;
GtkWidget *notebook;
GtkWidget *drawing_area;
GtkWidget *tile_dialog;
GtkWidget *black_score;
GtkWidget *white_score;

GtkAction *new_game_action;
GtkAction *new_network_action;
GtkAction *leave_network_action;
GtkAction *player_list_action;
GtkAction *undo_action;

cairo_surface_t *buffer_surface     = NULL;
cairo_surface_t *tiles_surface      = NULL;
cairo_surface_t *background_surface = NULL;

static gint flip_pixmaps_id = 0;
static gint flip_animation_speed = PIXMAP_FLIP_DELAY;
guint statusbar_id;
guint black_computer_level;
guint white_computer_level;
guint black_computer_id = 0;
guint white_computer_id = 0;
guint computer_speed = COMPUTER_MOVE_DELAY;
gint animate;
gint animate_stagger;
guint tiles_to_flip = 0;

gint64 milliseconds_total = 0;
gint64 milliseconds_current_start = 0;

guint game_in_progress;

guint tile_width = 80, tile_height = 80;
guint board_width = 648, board_height = 648;
#define GRIDWIDTH 1
double dash[1] = {4.0};

gint8 pixmaps[8][8] = { {0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
};

gint8 board[8][8] = { {0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
,
{0, 0, 0, 0, 0, 0, 0, 0}
};
guint whose_turn;
gint8 move_count;
gint bcount;
gint wcount;

extern guint flip_final_id;
extern gint8 squares[64];

int session_flag = 0;
int session_xpos = -1;
int session_ypos = -1;
int session_position = 0;

gchar *tile_set = NULL;
gchar *tile_set_tmp = NULL;

static const GOptionEntry options[] = {
  {"x", 'x', 0, G_OPTION_ARG_INT, &session_xpos, N_("X location of window"),
   N_("X")},
  {"y", 'y', 0, G_OPTION_ARG_INT, &session_ypos, N_("Y location of window"),
   N_("Y")},
  {NULL}
};

static void
undo_set_sensitive (gboolean state)
{
  gtk_action_set_sensitive (undo_action, state);
}

void
quit_game_cb (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

void
new_game_cb (GtkWidget * widget, gpointer data)
{
  init_new_game ();
}

void
on_player_list (void)
{
#ifdef GGZ_CLIENT
  create_or_raise_dlg_players (GTK_WINDOW (window));
#endif /* GGZ_CLIENT */
}

void
on_chat_window (void)
{
#ifdef GGZ_CLIENT
  create_or_raise_dlg_chat (GTK_WINDOW (window));
#endif /* GGZ_CLIENT */
}

void
new_network_game_cb (GtkWidget * widget, gpointer data)
{
#ifdef GGZ_CLIENT
  on_network_game ();
  gtk_action_set_sensitive (new_network_action, TRUE);
  gtk_action_set_sensitive (leave_network_action, TRUE);
  gtk_action_set_sensitive (player_list_action, TRUE);
#endif /* GGZ_CLIENT */
}

void
on_network_leave (GObject * object, gpointer data)
{
#ifdef GGZ_CLIENT
  ggz_embed_leave_table ();
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), NETWORK_PAGE);
#endif /* GGZ_CLIENT */
}

void
undo_move_cb (GtkWidget * widget, gpointer data)
{
  gint8 which_computer, xy;

  if ((black_computer_level && white_computer_level) || move_count == 4)
    return;
   
  /* Cancel any pending AI operations */
  if (black_computer_id) {
    g_source_remove (black_computer_id);
    black_computer_id = 0;
  }
  if (white_computer_id) {
    g_source_remove (white_computer_id);
    white_computer_id = 0;
  }

  if (flip_final_id) {
    g_source_remove (flip_final_id);
    flip_final_id = 0;
  }

  game_in_progress = 1;

  which_computer = OTHER_PLAYER (whose_turn);
  undo ();
  board_copy ();
  xy = squares[move_count];
  pixmaps[xy % 10 - 1][xy / 10 - 1] = 100;
  if ((((whose_turn == WHITE_TURN) && white_computer_level) ||
       ((whose_turn == BLACK_TURN) && black_computer_level))
      && (move_count > 4)) {
    undo ();
    board_copy ();
    xy = squares[move_count];
    pixmaps[xy % 10 - 1][xy / 10 - 1] = 100;
  }

  gui_status ();
  start_animation ();
  check_computer_players ();
}

void
about_cb (GtkAction * action, gpointer data)
{
  const gchar *authors[] = { "Ian Peters", NULL };

  const gchar *documenters[] = { "Eric Baudais", NULL };

  gchar *license = games_get_license (_(APP_NAME_LONG));

  gtk_show_about_dialog (GTK_WINDOW (window),
			 "name", _(APP_NAME_LONG),
			 "version", VERSION,
			 "copyright",
			 "Copyright \xc2\xa9 1998-2008 Ian Peters",
                         "license", license,
                         "comments", _("A disk flipping game derived from Reversi.\n\nIagno is a part of GNOME Games."),
			 "authors", authors,
                         "documenters", documenters,
			 "translator-credits", _("translator-credits"),
			 "logo-icon-name", "gnome-iagno",
			 "website-label", _("GNOME Games web site"),
                         "website", "http://www.gnome.org/projects/gnome-games/",
			 "wrap-license", TRUE,
                         NULL);
  g_free (license);
}

void
properties_cb (GtkWidget * widget, gpointer data)
{
  show_properties_dialog ();
}

gboolean
draw_event (GtkWidget * widget, cairo_t *cr)
{
  cairo_set_source_surface (cr, buffer_surface, 0, 0);
  cairo_paint (cr);
  return (FALSE);
}

gboolean
configure_event (GtkWidget * widget, GdkEventConfigure * event)
{
  static int old_width = 0, old_height = 0;

  if (old_width == event->width && old_height == event->height) {
    return FALSE;
  } else {
    old_width = event->width;
    old_height = event->height;
  }

  gui_draw_board ();
  return FALSE;
}

gint
button_press_event (GtkWidget * widget, GdkEventButton * event)
{
  guint x, y;

  if (game_in_progress == 0)
    return TRUE;

  if (!ggz_network_mode && (whose_turn == WHITE_TURN) && white_computer_level)
    return TRUE;

  if (!ggz_network_mode && (whose_turn == BLACK_TURN) && black_computer_level)
    return TRUE;

  if (event->button == 1) {
    x = event->x / (tile_width + GRIDWIDTH);
    y = event->y / (tile_height + GRIDWIDTH);
    if (ggz_network_mode && player_id == whose_turn
	&& is_valid_move (x, y, whose_turn)) {
#ifdef GGZ_CLIENT
      send_my_move (CART (x + 1, y + 1), whose_turn);
#endif /* GGZ_CLIENT */
    } else if (!ggz_network_mode && is_valid_move (x, y, whose_turn)) {
      move (x, y, whose_turn);
    } else {
      gui_message (_("Invalid move."));
    }

  }

  return TRUE;
}

static void
gui_fill_background(cairo_t *cr)
{
  cairo_pattern_t *p = cairo_pattern_create_for_surface(background_surface);
  cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);
  cairo_set_source(cr, p);
  cairo_move_to(cr, 0, 0);
  cairo_line_to (cr, 0, board_height);
  cairo_line_to (cr, board_width, board_height);
  cairo_line_to (cr, board_width, 0);
  cairo_line_to (cr, 0, 0);
  cairo_fill(cr);
}

static void
gui_draw_pixmap_buffer (cairo_t *cr, gint which, gint x, gint y)
{
  int tile_surface_x = x * (tile_width + GRIDWIDTH) - (which % 8) * tile_width;
  int tile_surface_y = y * (tile_height + GRIDWIDTH) - (which / 8) * tile_height;

  cairo_set_source_surface (cr, tiles_surface, tile_surface_x, tile_surface_y);
  cairo_rectangle (cr, x * (tile_width + GRIDWIDTH), y * (tile_height + GRIDWIDTH), tile_width, tile_height);
  cairo_fill (cr);
}

static void
gui_draw_grid (cairo_t *cr)
{
  int i;
  if (!show_grid)
    return;

  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);
  cairo_set_dash(cr, dash, 1, 2.5);
  cairo_set_line_width(cr, GRIDWIDTH);
  for (i = 1; i < 8; i++) {
    cairo_move_to (cr, i * board_width / 8 - 0.5, 0);
    cairo_line_to (cr, i * board_width / 8 - 0.5, board_height);

    cairo_move_to (cr, 0, i * board_height / 8 - 0.5);
    cairo_line_to (cr, board_width, i * board_height / 8 - 0.5);
  }
  cairo_stroke(cr);
}

void
gui_draw_pixmap (gint which, gint x, gint y)
{
  cairo_t *cr;
  GdkRectangle rect;

  cr = cairo_create (buffer_surface);
  gui_draw_pixmap_buffer (cr, which, x, y);
  cairo_destroy (cr);

  rect.x = x * (tile_width + GRIDWIDTH);
  rect.y = y * (tile_height + GRIDWIDTH);
  rect.width = tile_width;
  rect.height = tile_height;
  gdk_window_invalidate_rect (gtk_widget_get_window (drawing_area), &rect, FALSE);
}

void
gui_draw_board() {
  cairo_t *cr;
  guint i, j;
  GdkRectangle rect;

  cr = cairo_create (buffer_surface);
  gui_fill_background(cr);

  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      if (pixmaps[i][j] >= BLACK_TURN && pixmaps[i][j] <= WHITE_TURN)
        gui_draw_pixmap_buffer (cr, pixmaps[i][j], i, j);
      else
        gui_draw_pixmap_buffer (cr, 0, i, j);

  gui_draw_grid(cr);
  cairo_destroy (cr);

  rect.x = 0;
  rect.y = 0;
  rect.width = board_width;
  rect.height = board_height;
  gdk_window_invalidate_rect (gtk_widget_get_window (drawing_area), &rect, FALSE);
}

void
load_pixmaps (void)
{
  GdkPixbuf *image;
  GError *error = NULL;
  gchar *fname;
  const char *dname;
  cairo_t *cr;
  int dash_count;

  g_return_if_fail (tile_set != NULL && tile_set[0] != '0');

  dname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);

  fname = g_build_filename (dname, tile_set, NULL);

  /* fall back to default tileset "classic.png" if tile_set not found*/
  if (!g_file_test (fname, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
    g_free (fname);
    fname = g_build_filename (dname, "classic.png", NULL);
  }

  if (!g_file_test (fname, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
    g_print (_("Could not find \'%s\' pixmap file\n"), fname);
    exit (1);
  }

  image = gdk_pixbuf_new_from_file (fname, &error);
  if (error) {
    g_warning (G_STRLOC ": gdk-pixbuf error %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  tile_width = gdk_pixbuf_get_width (image) / 8;
  tile_height = gdk_pixbuf_get_height (image) / 4;

  /* Make sure the dash width evenly subdivides the tile height, and is at least 4 pixels long.
   * This makes the dash crossings always cross in the same place, which looks nicer. */
  dash_count = (tile_height + GRIDWIDTH)/4;
  if (dash_count%2 != 0)
    dash_count--;
  dash[0] = ((double)(tile_height + GRIDWIDTH))/dash_count;

  board_width = (tile_width+GRIDWIDTH) * 8;
  board_height = (tile_height+GRIDWIDTH) * 8;
  if (buffer_surface)
    cairo_surface_destroy (buffer_surface);
  gtk_widget_realize (drawing_area);

  buffer_surface = gdk_window_create_similar_surface (gtk_widget_get_window (drawing_area),
                                                      CAIRO_CONTENT_COLOR_ALPHA,
                                                      board_width, board_height);
  gtk_widget_set_size_request (GTK_WIDGET (drawing_area),
			       board_width, board_height);

  if (tiles_surface)
    cairo_surface_destroy (tiles_surface);
  tiles_surface = gdk_window_create_similar_surface (gtk_widget_get_window (drawing_area),
                                                     CAIRO_CONTENT_COLOR_ALPHA,
                                                     gdk_pixbuf_get_width (image),
                                                     gdk_pixbuf_get_height (image));
  cr = cairo_create (tiles_surface);
  gdk_cairo_set_source_pixbuf (cr, image, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);

  if (background_surface)
    cairo_surface_destroy (background_surface);
  background_surface = gdk_window_create_similar_surface (gtk_widget_get_window (drawing_area),
                                                     CAIRO_CONTENT_COLOR_ALPHA,
                                                     1, 1);
  cr = cairo_create (background_surface);
  gdk_cairo_set_source_pixbuf (cr, image, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);

  g_object_unref (image);
  g_free (fname);
}

void
set_animation_speed (gint speed)
{
    flip_animation_speed = speed;
    if (flip_pixmaps_id) {
        g_source_remove (flip_pixmaps_id);
        flip_pixmaps_id = g_timeout_add (flip_animation_speed, flip_pixmaps, NULL);
    }
}

void
start_animation (void)
{
    if (flip_pixmaps_id == 0)
        flip_pixmaps_id = g_timeout_add (flip_animation_speed, flip_pixmaps, NULL);
    tiles_to_flip = 1;
}

void
stop_animation (void)
{
    tiles_to_flip = 0;
}

gint
flip_pixmaps (gpointer data)
{
  guint i, j;
  guint flipped_tiles = 0;

  if (!tiles_to_flip) {
      flip_pixmaps_id = 0;
      return FALSE;
  }

  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++) {
      /* This first case only happens when undoing the "final flip". */
      if ((pixmaps[i][j] == 101) && (board[i][j] != 0)) {
	pixmaps[i][j] = board[i][j];
	gui_draw_pixmap (pixmaps[i][j], i, j);
	flipped_tiles = 1;
      } else if ((pixmaps[i][j] == 100)
		 || ((pixmaps[i][j] != 101) && (board[i][j] == 0))) {
	pixmaps[i][j] = 101;
	gui_draw_pixmap (0, i, j);
	flipped_tiles = 1;
      } else if (pixmaps[i][j] < board[i][j]) {
	if (animate == 0) {
	  if (pixmaps[i][j] == BLACK_TURN)
	    pixmaps[i][j] = board[i][j];
	  else
	    pixmaps[i][j]++;
	} else if (animate == 1) {
	  if (pixmaps[i][j] < 1)
	    pixmaps[i][j] += 2;
	  else if (pixmaps[i][j] >= 1 && pixmaps[i][j] < 8)
	    pixmaps[i][j] = 8;
	  else if (pixmaps[i][j] >= 8 && pixmaps[i][j] < 16)
	    pixmaps[i][j] = 16;
	  else if (pixmaps[i][j] >= 16 && pixmaps[i][j] < 23)
	    pixmaps[i][j] = 23;
	  else if (pixmaps[i][j] >= 23 && pixmaps[i][j] < 31)
	    pixmaps[i][j] = 31;
	  else if (pixmaps[i][j] > 31)
	    pixmaps[i][j] = 31;
	} else if (animate == 2)
	  pixmaps[i][j]++;
	if (pixmaps[i][j] > 0)
	  gui_draw_pixmap (pixmaps[i][j], i, j);
	flipped_tiles = 1;
      } else if (pixmaps[i][j] > board[i][j] && pixmaps[i][j] != 101) {
	if (animate == 0) {
	  if (pixmaps[i][j] == WHITE_TURN)
	    pixmaps[i][j] = board[i][j];
	  else
	    pixmaps[i][j]--;
	} else if (animate == 1) {
	  if (pixmaps[i][j] > 31)
	    pixmaps[i][j] -= 2;
	  else if (pixmaps[i][j] <= 31 && pixmaps[i][j] > 23)
	    pixmaps[i][j] = 23;
	  else if (pixmaps[i][j] <= 23 && pixmaps[i][j] > 16)
	    pixmaps[i][j] = 16;
	  else if (pixmaps[i][j] <= 16 && pixmaps[i][j] > 8)
	    pixmaps[i][j] = 8;
	  else if (pixmaps[i][j] <= 8 && pixmaps[i][j] > 1)
	    pixmaps[i][j] = 1;
	  else if (pixmaps[i][j] < 1)
	    pixmaps[i][j] = 1;
	} else if (animate == 2)
	  pixmaps[i][j]--;
	if (pixmaps[i][j] < 32)
	  gui_draw_pixmap (pixmaps[i][j], i, j);
	flipped_tiles = 1;
      }
    }

  if (!flipped_tiles)
    stop_animation ();

  return TRUE;
}

static void
redraw_board (void)
{
  gui_status ();
  gui_draw_board();
}

void
clear_board (void)
{
  guint i, j;

  if (flip_final_id) {
    g_source_remove (flip_final_id);
    flip_final_id = 0;
  }

  if (black_computer_id) {
    g_source_remove (black_computer_id);
    black_computer_id = 0;
  }

  if (white_computer_id) {
    g_source_remove (white_computer_id);
    white_computer_id = 0;
  }

  game_in_progress = 0;
  move_count = 0;
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      board[i][j] = 0;

  memcpy (pixmaps, board, sizeof (gint8) * 8 * 8);

  bcount = 0;
  wcount = 0;

  redraw_board ();
}

void
init_new_game (void)
{
  clear_board ();
  game_in_progress = 1;
  move_count = 4;

  undo_set_sensitive (FALSE);

  board[3][3] = WHITE_TURN;
  board[3][4] = BLACK_TURN;
  board[4][3] = BLACK_TURN;
  board[4][4] = WHITE_TURN;

  bcount = 2;
  wcount = 2;
  init ();

  memcpy (pixmaps, board, sizeof (gint8) * 8 * 8);

  redraw_board ();

  whose_turn = BLACK_TURN;
  gui_status ();

  check_computer_players ();
}

void
gui_status (void)
{
  gchar message[3];

  sprintf (message, _("%.2d"), bcount);
  gtk_label_set_text (GTK_LABEL (black_score), message);
  sprintf (message, _("%.2d"), wcount);
  gtk_label_set_text (GTK_LABEL (white_score), message);
  undo_set_sensitive (move_count > 0);

  if (ggz_network_mode) {
    gtk_action_set_sensitive(new_game_action, FALSE);
    gtk_action_set_sensitive(new_network_action, FALSE);
    gtk_action_set_sensitive(leave_network_action, FALSE);
    gtk_action_set_sensitive(player_list_action, FALSE);
  } else {
    gtk_action_set_sensitive(new_game_action, TRUE);
    gtk_action_set_sensitive(new_network_action, TRUE);
    gtk_action_set_sensitive(leave_network_action, TRUE);
    gtk_action_set_sensitive(player_list_action, TRUE);
  }

#ifndef GGZ_CLIENT
    gtk_action_set_sensitive(new_network_action, FALSE);
#endif /* GGZ_CLIENT */

  if (ggz_network_mode) {
    if (whose_turn == player_id && whose_turn == BLACK_TURN) {
      gui_message (_("It is your turn to place a dark piece"));
    } else if (whose_turn == player_id && whose_turn == WHITE_TURN) {
      gui_message (_("It is your turn to place a light piece"));
    } else {
      gchar *str;
      str = g_strdup_printf (_("Waiting for %s to move"),
			     names[(seat + 1) % 2]);
      gui_message (str);
      g_free (str);
    }
  } else {
    if (whose_turn == BLACK_TURN) {
      gui_message (_("Dark's move"));
    } else if (whose_turn == WHITE_TURN) {
      gui_message (_("Light's move"));
    }

  }
}

void
gui_message (gchar * message)
{
  gtk_statusbar_pop (GTK_STATUSBAR(statusbar), statusbar_id);
  gtk_statusbar_push (GTK_STATUSBAR(statusbar), statusbar_id, message);
}

guint
check_computer_players (void)
{

  if (ggz_network_mode) {
    black_computer_id = 0;
    white_computer_id = 0;
    return TRUE;
  }
  if (black_computer_level && whose_turn == BLACK_TURN)
    switch (black_computer_level) {
    case 1:
      black_computer_id =
	add_timeout (computer_speed, (GSourceFunc) computer_move_1,
		       (gpointer) BLACK_TURN);
      break;
    case 2:
      black_computer_id =
	add_timeout (computer_speed, (GSourceFunc) computer_move_2,
		       (gpointer) BLACK_TURN);
      break;
    case 3:
      black_computer_id =
	add_timeout (computer_speed, (GSourceFunc) computer_move_3,
		       (gpointer) BLACK_TURN);
      break;
    }

  if (white_computer_level && whose_turn == WHITE_TURN)
    switch (white_computer_level) {
    case 1:
      white_computer_id =
	add_timeout (computer_speed, (GSourceFunc) computer_move_1,
		       (gpointer) WHITE_TURN);
      break;
    case 2:
      white_computer_id =
	add_timeout (computer_speed, (GSourceFunc) computer_move_2,
		       (gpointer) WHITE_TURN);
      break;
    case 3:
      white_computer_id =
	add_timeout (computer_speed, (GSourceFunc) computer_move_3,
		       (gpointer) WHITE_TURN);
      break;
    }

  return TRUE;
}

guint
add_timeout (guint time, GSourceFunc func, gpointer turn)
{
  if (time % 1000) {
    return g_timeout_add (time, func, turn);
  } else {
    time = time / 1000;
    return g_timeout_add_seconds (time, func, turn);
  }
}

#ifdef WITH_SMCLIENT
static int
save_state_cb (EggSMClient *client,
	    GKeyFile* keyfile,
	    gpointer client_data)
{
  gchar *argv[20];
  gint argc;
  gint xpos, ypos;

  gdk_window_get_origin (gtk_widget_get_window (window), &xpos, &ypos);

  argc = 0;
  argv[argc++] = g_get_prgname ();
  argv[argc++] = "-x";
  argv[argc++] = g_strdup_printf ("%d", xpos);
  argv[argc++] = "-y";
  argv[argc++] = g_strdup_printf ("%d", ypos);

  egg_sm_client_set_restart_command (client, argc, (const char **) argv);

  g_free (argv[2]);
  g_free (argv[4]);

  return TRUE;
}

static gint
quit_cb (EggSMClient *client,
         gpointer client_data)
{
  gtk_main_quit ();

  return FALSE;
}

#endif /* WITH_SMCLIENT */

static void
help_cb (GtkAction * action, gpointer data)
{
  games_help_display (window, APP_NAME, NULL);
}

static const GtkActionEntry actions[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL, 
   G_CALLBACK (new_game_cb)},
#ifdef GGZ_CLIENT
  {"NewNetworkGame", GAMES_STOCK_NETWORK_GAME, NULL, NULL, NULL,
   G_CALLBACK (new_network_game_cb)},
#else
  {"NewNetworkGame", GAMES_STOCK_NETWORK_GAME, NULL, NULL, NULL, NULL},
#endif
  {"LeaveNetworkGame", GAMES_STOCK_NETWORK_LEAVE, NULL, NULL, NULL,
   G_CALLBACK (on_network_leave)},
  {"PlayerList", GAMES_STOCK_PLAYER_LIST, NULL, NULL, NULL,
   G_CALLBACK (on_player_list)},
  {"UndoMove", GAMES_STOCK_UNDO_MOVE, NULL, NULL, NULL,
   G_CALLBACK (undo_move_cb)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_game_cb)},
  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (properties_cb)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL,
   G_CALLBACK (help_cb)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_cb)}
};

static const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <menuitem action='NewNetworkGame'/>"
  "      <separator/>"
  "      <menuitem action='UndoMove'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
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


  action_group = gtk_action_group_new ("group");

  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, actions, G_N_ELEMENTS (actions),
				window);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

  gtk_window_add_accel_group (GTK_WINDOW (window),
			      gtk_ui_manager_get_accel_group (ui_manager));

  new_game_action = gtk_action_group_get_action (action_group, "NewGame");
  new_network_action = gtk_action_group_get_action (action_group,
						    "NewNetworkGame");
#ifndef GGZ_CLIENT
  gtk_action_set_sensitive (new_network_action, FALSE);
#endif /* GGZ_CLIENT */
  leave_network_action =
    gtk_action_group_get_action (action_group, "LeaveNetworkGame");
  player_list_action =
    gtk_action_group_get_action (action_group, "PlayerList");
  undo_action = gtk_action_group_get_action (action_group, "UndoMove");
}

void
create_window (void)
{
  GtkWidget *table;
  GtkUIManager *ui_manager;
  GtkWidget *menubar;
  GtkWidget *vbox;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), _(APP_NAME_LONG));

  games_conf_add_window (GTK_WINDOW (window), NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  ui_manager = gtk_ui_manager_new ();
  create_menus (ui_manager);
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (quit_game_cb), NULL);

  drawing_area = gtk_drawing_area_new ();

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), drawing_area, NULL);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), MAIN_PAGE);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (drawing_area), "draw",
		    G_CALLBACK (draw_event), NULL);
  g_signal_connect (G_OBJECT (window), "configure_event",
		    G_CALLBACK (configure_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
		    G_CALLBACK (button_press_event), NULL);

  gtk_widget_set_events (drawing_area,
			 GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  /* We do our own double-buffering. */
  gtk_widget_set_double_buffered (drawing_area, FALSE);

  statusbar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  table = gtk_table_new (1, 8, FALSE);

  black_score = gtk_label_new (_("Dark:"));
  gtk_widget_show (black_score);

  gtk_table_attach (GTK_TABLE (table), black_score, 1, 2, 0, 1, 0, 0, 3, 1);

  black_score = gtk_label_new ("00");
  gtk_widget_show (black_score);

  gtk_table_attach (GTK_TABLE (table), black_score, 2, 3, 0, 1, 0, 0, 3, 1);

  white_score = gtk_label_new (_("Light:"));
  gtk_widget_show (white_score);

  gtk_table_attach (GTK_TABLE (table), white_score, 4, 5, 0, 1, 0, 0, 3, 1);

  white_score = gtk_label_new ("00");
  gtk_widget_show (white_score);

  gtk_table_attach (GTK_TABLE (table), white_score, 5, 6, 0, 1, 0, 0, 3, 1);
  undo_set_sensitive (FALSE);

  gtk_widget_show (vbox);
  gtk_widget_show (drawing_area);
  gtk_widget_show (notebook);
  gtk_widget_show (table);
  gtk_widget_show (statusbar);

  gtk_box_pack_start (GTK_BOX (statusbar), table, FALSE, TRUE, 0);

  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  statusbar_id = gtk_statusbar_get_context_id (GTK_STATUSBAR(statusbar),
                                               "iagno");
  gtk_statusbar_push (GTK_STATUSBAR(statusbar), statusbar_id, 
                      _("Welcome to Iagno!"));
}

int
main (int argc, char **argv)
{
  GOptionContext *context;
  gboolean retval;
  GError *error = NULL;
#ifdef WITH_SMCLIENT
  EggSMClient *sm_client;
#endif /* WITH_SMCLIENT */

  if (!games_runtime_init ("iagno"))
    return 1;

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

  g_set_application_name (_(APP_NAME_LONG));

  games_conf_initialise (APP_NAME);

  games_stock_init ();

  gtk_window_set_default_icon_name ("gnome-iagno");

#ifdef WITH_SMCLIENT
  sm_client = egg_sm_client_get ();
  g_signal_connect (sm_client, "save-state",
		    G_CALLBACK (save_state_cb), NULL);
  g_signal_connect (sm_client, "quit",
                    G_CALLBACK (quit_cb), NULL);
#endif /* WITH_SMCLIENT */

  create_window ();

  load_properties ();

  load_pixmaps ();

  gtk_widget_show (window);

#ifdef GGZ_CLIENT
  network_init ();
#endif /* GGZ_CLIENT */

  if (session_xpos >= 0 && session_ypos >= 0) {
    gdk_window_move (gtk_widget_get_window (window), session_xpos, session_ypos);
  }

  init_new_game ();

  gtk_main ();

  games_conf_shutdown ();

  games_runtime_shutdown ();

  return 0;
}
