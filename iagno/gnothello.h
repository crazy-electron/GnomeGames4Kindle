/*
 * gnothello.h - Header for gnothello.c
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

#ifndef _GNOTHELLO_H_
#define _GNOTHELLO_H_

#define BLACK_TURN 1
#define WHITE_TURN 31
#define PIXMAP_FLIP_DELAY 20
#define PIXMAP_STAGGER_DELAY 3
#define COMPUTER_MOVE_DELAY 1000

#define OTHER_PLAYER(w) (((w) == WHITE_TURN) ? BLACK_TURN : WHITE_TURN)

#define MAIN_PAGE           	0
#define NETWORK_PAGE           	1

gboolean ggz_network_mode;
int player_id;
int seat;
int seats[2];
char names[2][17];

extern GtkWidget *notebook;
extern GtkWidget *window;

void new_game_cb (GtkWidget * widget, gpointer data);
void new_network_game_cb (GtkWidget * widget, gpointer data);
void on_network_leave (GObject * object, gpointer data);
void on_player_list (void);
void on_chat_window (void);
void quit_game_cb (GtkWidget * widget, gpointer data);
void undo_move_cb (GtkWidget * widget, gpointer data);
void redo_move_cb (GtkWidget * widget, gpointer data);
void about_cb (GtkAction * action, gpointer data);
void comp_black_cb (GtkWidget * widget, gpointer data);
void comp_white_cb (GtkWidget * widget, gpointer data);
void quick_moves_cb (GtkWidget * widget, gpointer data);
void anim_cb (GtkWidget * widget, gpointer data);
void anim_stagger_cb (GtkWidget * widget, gpointer data);
void load_tiles_cb (GtkWidget * widget, gpointer data);
void set_selection (GtkWidget * widget, void *data);
void free_str (GtkWidget * widget, void *data);
void load_tiles_callback (GtkWidget * widget, void *data);
void cancel (GtkWidget * widget, void *data);
gboolean expose_event (GtkWidget * widget, GdkEventExpose * event, gpointer data);
gint configure_event (GtkWidget * widget, GdkEventConfigure * event);
gint button_press_event (GtkWidget * widget, GdkEventButton * event);
void gui_draw_pixmap (gint which, gint x, gint y);
void gui_draw_board (void);
void set_animation_speed (gint speed);
void start_animation (void);
void stop_animation (void);
gint flip_pixmaps (gpointer data);
void init_new_game (void);
void clear_board (void);
void create_window (void);
void gui_status (void);
void gui_message (gchar * message);
guint check_computer_players (void);
guint add_timeout (guint time, GSourceFunc func, gpointer turn);
void load_pixmaps (void);
void properties_cb (GtkWidget * widget, gpointer data);

#endif
