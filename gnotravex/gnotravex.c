/* -*- mode:C; indent-tabs-mode: nil; tab-width: 8; c-basic-offset: 2; -*- */

/*
 *   Gnome Tetravex: Tetravex clone
 *   Written by Lars Rydlinge <lars.rydlinge@hig.se>
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
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgames-support/games-clock.h>
#include <libgames-support/games-conf.h>
#include <libgames-support/games-gtk-compat.h>
#include <libgames-support/games-help.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-scores-dialog.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-stock.h>
#include <libgames-support/games-pause-action.h>
#include <libgames-support/games-fullscreen-action.h>

#ifdef WITH_SMCLIENT
#include <libgames-support/eggsmclient.h>
#endif /* WITH_SMCLIENT */

#define APPNAME "gnotravex"
#define APPNAME_LONG N_("Tetravex")

#define RELEASE 4
#define PRESS 3
#define MOVING 2
#define UNUSED 1
#define USED 0

#define LONG_COUNT 15
#define SHORT_COUNT 5
#define DELAY 10

#define KEY_GRID_SIZE     "grid_size"
#define KEY_CLICK_MOVE    "click_to_move"

#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240

/* The sector of a tile to mark quads with */
#define NORTH 0
#define SOUTH 1
#define EAST  2
#define WEST  3

#define HIGHLIGHT 0
#define BASE      1
#define SHADOW    2
#define TEXT      3

typedef struct
{
  gint n, w, e, s;
  gint status;
} Tile;

typedef struct
{
  GdkWindow *window;
  cairo_surface_t *surface;
  Tile heldtile;
  gint xstart, ystart;
  gint xend, yend;
  gint xoff, yoff;
  gint x, y;
} Mover;

typedef enum
{
  GAME_OVER,
  PAUSED,
  PLAYING,
} GameState;

static GtkWidget *window = NULL;
static GtkWidget *statusbar = NULL;
static GtkWidget *space = NULL;
static GtkWidget *timer = NULL;
static GdkPixbuf *background_pixbuf = NULL;
static GamesScores *highscores = NULL;
static gint xborder = 0;
static gint yborder = 0;
static gint gap = 0;
static cairo_surface_t *buffer = NULL;
static Tile tiles[9][18];
static Tile orig_tiles[9][9];
static GdkWindowAttr windowattrib;
static Mover mousemover;
static Mover automover;
static const GamesScoresCategory scorecats[] = {
  { "2x2", N_("2\303\2272") },
  { "3x3", N_("3\303\2273") },
  { "4x4", N_("4\303\2274") },
  { "5x5", N_("5\303\2275") },
  { "6x6", N_("6\303\2276") }
};

static gint size = -1;
static GameState game_state = GAME_OVER;
static gint have_been_hinted = 0;
static gint solve_me = 0;
static gint moving = 0;
static gint session_xpos = 0;
static gint session_ypos = 0;
static guint timer_timeout = 0;
static gint tile_size = 0;
static gdouble tile_border_size = 3.0;
static gdouble arrow_border_size = 1.5;
static gboolean click_to_move = FALSE;
static gint button_down = 0;

static gint animcount = 0;
static gboolean swapanim = FALSE;
static gint move_src_x = 0, move_src_y = 0, move_dest_x = 0, move_dest_y = 0;

/* The vertices used in the tiles/sockets. These are built using gui_build_vertices() */
static gdouble vertices[27][2];
static gboolean rebuild_vertices = TRUE;

/* The faces used to build a socket */
static int socket_faces[4][7] = {
  {NORTH, SHADOW,    4, 0, 1, 18, 17},
  {WEST,  SHADOW,    4, 0, 3, 20, 17},
  {EAST,  HIGHLIGHT, 4, 1, 2, 19, 18},
  {SOUTH, HIGHLIGHT, 4, 2, 3, 20, 19}
};

/* The faces used to build the arrow */
static int arrow_faces[3][7] = {
  {NORTH, SHADOW,    4, 21, 24, 25, 22},
  {NORTH, SHADOW,    4, 21, 23, 26, 24},
  {NORTH, HIGHLIGHT, 4, 23, 22, 25, 26}
};

/* The faces used to build a tile */
static int tile_faces[16][7] = {
  {NORTH, BASE,      3,  4,  5, 12,  0},
  {SOUTH, BASE,      3,  8,  9, 14,  0},
  {EAST,  BASE,      3,  6,  7, 13,  0},
  {WEST,  BASE,      3, 10, 11, 15,  0},
  {EAST,  SHADOW,    4,  1,  2,  7,  6},
  {SOUTH, SHADOW,    4,  2,  3,  9,  8},
  {WEST,  SHADOW,    4,  0, 16, 15, 11},
  {NORTH, SHADOW,    4,  1, 16, 12,  5},
  {SOUTH, SHADOW,    4,  2, 16, 14,  8},
  {WEST,  SHADOW,    4,  3, 16, 15, 10},
  {NORTH, HIGHLIGHT, 4,  0,  1,  5,  4},
  {WEST,  HIGHLIGHT, 4,  0,  3, 10, 11},
  {NORTH, HIGHLIGHT, 4,  0, 16, 12,  4},
  {EAST,  HIGHLIGHT, 4,  1, 16, 13,  6},
  {EAST,  HIGHLIGHT, 4,  2, 16, 13,  7},
  {SOUTH, HIGHLIGHT, 4,  3, 16, 14,  9}
};

/* Tile segment colours (this is the resistor colour code) */
static gdouble tile_colours[11][4][4] = {
  {{ 46,  52,  54, 255}, {  0,   0,   0, 255}, {  0,   0,   0, 255}, {255, 255, 255, 255}}, /* 0  = black */
  {{233, 185, 110, 255}, {193, 125,  17, 255}, {143,  89,   2, 255}, {255, 255, 255, 255}}, /* 1  = brown */
  {{239,  41,  41, 255}, {204,   0,   0, 255}, {164,   0,   0, 255}, {255, 255, 255, 255}}, /* 2  = red */
  {{252, 175,  62, 255}, {245, 121,   0, 255}, {206,  92,   0, 255}, {255, 255, 255, 255}}, /* 3  = orange */
  {{252, 233,  79, 255}, {237, 212,   0, 255}, {196, 160,   0, 255}, {  0,   0,   0, 255}}, /* 4  = yellow */
  {{138, 226,  52, 255}, {115, 210,  22, 255}, { 78, 154,   6, 255}, {  0,   0,   0, 255}}, /* 5  = green */
  {{114, 159, 207, 255}, { 52, 101, 164, 255}, { 32,  74, 135, 255}, {255, 255, 255, 255}}, /* 6  = blue */
  {{173, 127, 168, 255}, {117,  80, 123, 255}, { 92,  53, 102, 255}, {255, 255, 255, 255}}, /* 7  = violet */
  {{211, 215, 207, 255}, {186, 189, 182, 255}, {136, 138, 133, 255}, {  0,   0,   0, 255}}, /* 8  = grey */
  {{255, 255, 255, 255}, {255, 255, 255, 255}, {238, 238, 236, 255}, {  0,   0,   0, 255}}, /* 9  = white */
  {{255, 255, 255,  32}, {  0,   0,   0,  32}, {  0,   0,   0,  64}, {  0,   0,   0,   0}}  /* 10 = shadows */
};

/* The colour to use when drawing the sockets */
#define SOCKET_COLOUR 10

const GtkRadioActionEntry size_action_entry[] = {
  {"Size2x2", NULL, N_("_2\303\2272"), NULL, N_("Play on a 2\303\2272 board"),
   2},
  {"Size3x3", NULL, N_("_3\303\2273"), NULL, N_("Play on a 3\303\2273 board"),
   3},
  {"Size4x4", NULL, N_("_4\303\2274"), NULL, N_("Play on a 4\303\2274 board"),
   4},
  {"Size5x5", NULL, N_("_5\303\2275"), NULL, N_("Play on a 5\303\2275 board"),
   5},
  {"Size6x6", NULL, N_("_6\303\2276"), NULL, N_("Play on a 6\303\2276 board"),
   6}
};

static GtkAction *new_game_action;
static GtkAction *pause_action;
static GtkAction *hint_action;
static GtkAction *solve_action;
static GtkAction *scores_action;
static GtkAction *move_up_action;
static GtkAction *move_left_action;
static GtkAction *move_right_action;
static GtkAction *move_down_action;
static GtkAction *fullscreen_action;
static GtkAction *size_action[G_N_ELEMENTS (size_action_entry)];

static const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='NewGame'/>"
  "      <menuitem action='PauseGame'/>"
  "      <separator/>"
  "      <menu action='MoveMenu'>"
  "        <menuitem action='MoveUp'/>"
  "        <menuitem action='MoveLeft'/>"
  "        <menuitem action='MoveRight'/>"
  "        <menuitem action='MoveDown'/>"
  "      </menu>"
  "      <menuitem action='Hint'/>"
  "      <menuitem action='Solve'/>"
  "      <separator/>"
  "      <menuitem action='Scores'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
  "    </menu>"
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='Fullscreen'/>"
  "      <menuitem action='ClickToMove'/>"
  "      <separator/>"
  "      <menuitem action='Size2x2'/>"
  "      <menuitem action='Size3x3'/>"
  "      <menuitem action='Size4x4'/>"
  "      <menuitem action='Size5x5'/>"
  "      <menuitem action='Size6x6'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>"
  "    </menu>"
  "  </menubar>"
  "</ui>";


static const GOptionEntry options[] = {
  {"x", 'x', 0, G_OPTION_ARG_INT, &session_xpos, N_("X location of window"),
   N_("X")},
  {"y", 'y', 0, G_OPTION_ARG_INT, &session_ypos, N_("Y location of window"),
   N_("Y")},
  {"size", 's', 0, G_OPTION_ARG_INT, &size, N_("Size of board (2-6)"),
   N_("SIZE")},
  {NULL}
};

/* ------------------------------------------------------- */

static void
gui_build_vertices (void)
{
  gdouble z, midx, midy, offset, far_offset;
  gdouble z2, dx, dy, w, h, xoffset, yoffset;

  /* Vertices 0-3 are the border of the square */
  vertices[0][0] = 0;
  vertices[0][1] = 0;
  vertices[1][0] = tile_size;
  vertices[1][1] = 0;
  vertices[2][0] = tile_size;
  vertices[2][1] = tile_size;
  vertices[3][0] = 0;
  vertices[3][1] = tile_size;

  /* Calculate the intersection between the edge and the diagonal grooves */
  z = 0.70711 * tile_border_size;
  offset = tile_border_size + z;
  far_offset = tile_size - offset;

  /* Top edge */
  vertices[4][0] = offset;
  vertices[4][1] = tile_border_size;
  vertices[5][0] = far_offset;
  vertices[5][1] = tile_border_size;

  /* Right edge */
  vertices[6][0] = tile_size - tile_border_size;
  vertices[6][1] = offset;
  vertices[7][0] = tile_size - tile_border_size;
  vertices[7][1] = far_offset;

  /* Bottom edge */
  vertices[8][0] = far_offset;
  vertices[8][1] = tile_size - tile_border_size;
  vertices[9][0] = offset;
  vertices[9][1] = tile_size - tile_border_size;

  /* Left edge */
  vertices[10][0] = tile_border_size;
  vertices[10][1] = far_offset;
  vertices[11][0] = tile_border_size;
  vertices[11][1] = offset;

  midx = tile_size / 2.0;
  midy = tile_size / 2.0;

  /* Inner edges */
  vertices[12][0] = midx;
  vertices[12][1] = midy - z;
  vertices[13][0] = midx + z;
  vertices[13][1] = midy;
  vertices[14][0] = midx;
  vertices[14][1] = midy + z;
  vertices[15][0] = midx - z;
  vertices[15][1] = midy;

  /* Centre point */
  vertices[16][0] = midx;
  vertices[16][1] = midy;

  /* Edges for socket */
  vertices[17][0] = tile_border_size;
  vertices[17][1] = tile_border_size;
  vertices[18][0] = tile_size - tile_border_size;
  vertices[18][1] = tile_border_size;
  vertices[19][0] = tile_size - tile_border_size;
  vertices[19][1] = tile_size - tile_border_size;
  vertices[20][0] = tile_border_size;
  vertices[20][1] = tile_size - tile_border_size;

  /* Edges for the arrow */
  w = gap;
  h = size * tile_size;
  xoffset = w * 0.25;
  yoffset = 0.5 * (h - 1.5 * tile_size);
  vertices[21][0] = xoffset;
  vertices[21][1] = h * 0.5;
  vertices[22][0] = w - xoffset;
  vertices[22][1] = yoffset;
  vertices[23][0] = vertices[22][0];
  vertices[23][1] = h - yoffset;

  /* Arrow inner edges */
  dx = w - 2*xoffset;
  dy = (h - 2*yoffset) * 0.5;
  z = arrow_border_size * dy / sqrt(dx*dx + dy*dy);
  z2 = (dy / dx) * (dx - arrow_border_size - z);
  vertices[24][0] = vertices[21][0] + z;
  vertices[24][1] = vertices[21][1];
  vertices[25][0] = vertices[22][0] - arrow_border_size;
  vertices[25][1] = vertices[21][1] - z2;
  vertices[26][0] = vertices[25][0];
  vertices[26][1] = vertices[21][1] + z2;
}

static void
gui_draw_faces (cairo_t * context, gint xadd, gint yadd, int quads[][7],
                int count, guint colours[4], gboolean prelight)
{
  int i, j, k;
  int *quad;
  guint face, level, n_vertices;
  gdouble *colour;

  for (i = 0; i < count; i += 1) {
    quad = quads[i];

    /* Set the face colour */
    face = quad[0];
    level = quad[1];
    if (prelight && level == BASE)
       level = HIGHLIGHT;
    n_vertices = quad[2];
    colour = tile_colours[colours[face]][level];
    cairo_set_source_rgba (context, colour[0] / 255.0, colour[1] / 255.0,
                           colour[2] / 255.0, colour[3] / 255.0);

    k = quad[3];
    cairo_move_to (context, xadd + vertices[k][0], yadd + vertices[k][1]);
    for (j = 1; j < n_vertices; j += 1) {
      k = quad[j + 3];
      cairo_line_to (context, xadd + vertices[k][0], yadd + vertices[k][1]);
    }

    cairo_close_path (context);
    cairo_fill (context);
  }
}

static void
gui_draw_arrow (cairo_surface_t * target)
{
  cairo_t *context;
  gdouble x, y;
  guint colours[4] = { SOCKET_COLOUR, SOCKET_COLOUR, SOCKET_COLOUR, SOCKET_COLOUR };

  context = cairo_create (buffer);

  x = xborder + size * tile_size;
  y = yborder;
  gui_draw_faces (context, x, y, arrow_faces, 3, colours, FALSE);

  cairo_destroy (context);
}

static void
gui_draw_socket (cairo_surface_t * target, GtkStateType state, gint xadd, gint yadd)
{
  cairo_t *context;
  guint colours[4] = { SOCKET_COLOUR, SOCKET_COLOUR, SOCKET_COLOUR, SOCKET_COLOUR };
  gdouble *colour;

  context = cairo_create (target);

  /* Only draw inside the allocated space */
  cairo_rectangle (context, xadd, yadd, tile_size, tile_size);
  cairo_clip_preserve (context);

  /* Tile the background */
  gdk_cairo_set_source_pixbuf (context, background_pixbuf, 0, 0);
  cairo_pattern_set_extend (cairo_get_source (context), CAIRO_EXTEND_REPEAT);
  cairo_fill_preserve (context);

  /* Blank the piece */
  colour = tile_colours[SOCKET_COLOUR][BASE];
  cairo_set_source_rgba (context, colour[0] / 255.0, colour[1] / 255.0, colour[2] / 255.0, colour[3] / 255.0);
  cairo_fill (context);

  /* Build the co-ordinates used by the tiles */
  if (rebuild_vertices) {
    gui_build_vertices ();
    rebuild_vertices = FALSE;
  }

  gui_draw_faces (context, xadd, yadd, socket_faces, 4, colours, FALSE);

  cairo_destroy (context);
}

static void
gui_draw_number (cairo_t * context, gdouble x, gdouble y, guint number, gdouble *colour)
{
  const gchar *text;
  cairo_text_extents_t extents;
  static const char *translatable_number[10] = {
    /* Translators: in-game numbers, replaceable with single-character local ideograms */
    NC_("number", "0"),
    NC_("number", "1"),
    NC_("number", "2"),
    NC_("number", "3"),
    NC_("number", "4"),
    NC_("number", "5"),
    NC_("number", "6"),
    NC_("number", "7"),
    NC_("number", "8"),
    NC_("number", "9")
  };

  text = g_dpgettext2 (NULL, "number", translatable_number[number]);

  cairo_set_source_rgba (context, colour[0] / 255.0, colour[1] / 255.0,
                         colour[2] / 255.0, colour[3] / 255.0);

  cairo_text_extents (context, text, &extents);
  cairo_move_to (context, x - extents.width / 2.0, y + extents.height / 2.0);
  cairo_show_text (context, text);
}

static void
gui_draw_tile (cairo_surface_t * target, GtkStateType state, gint xadd, gint yadd,
               gint north, gint south, gint east, gint west, gboolean prelight)
{
  cairo_t *context;
  guint colours[4];

  context = cairo_create (target);

  /* Use per sector colours */
  colours[NORTH] = north;
  colours[SOUTH] = south;
  colours[EAST] = east;
  colours[WEST] = west;

  /* Only draw inside the allocated space */
  cairo_rectangle (context, xadd, yadd, tile_size, tile_size);
  cairo_clip (context);

  /* Clear background */
  cairo_set_source_rgba (context, 0.0, 0.0, 0.0, 1.0);
  cairo_paint (context);

  /* Build the co-ordinates used by the tiles */
  if (rebuild_vertices) {
    gui_build_vertices ();
    rebuild_vertices = FALSE;
  }

  gui_draw_faces (context, xadd, yadd, tile_faces, 16, colours, prelight);

  /* Draw outline */
  cairo_set_line_width (context, 1.0);
  cairo_set_source_rgba (context, 0.0, 0.0, 0.0, 1.0);
  cairo_rectangle (context, xadd + 0.5, yadd + 0.5, tile_size - 1.0, tile_size - 1.0);
  cairo_stroke (context);

  cairo_select_font_face (context, "Sans", CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (context, tile_size / 3.5);

  gui_draw_number (context, xadd + tile_size / 2, yadd + tile_size / 5, north, tile_colours[colours[NORTH]][TEXT]);
  gui_draw_number (context, xadd + tile_size / 2, yadd + tile_size * 4 / 5, south, tile_colours[colours[SOUTH]][TEXT]);
  gui_draw_number (context, xadd + tile_size * 4 / 5, yadd + tile_size / 2, east, tile_colours[colours[EAST]][TEXT]);
  gui_draw_number (context, xadd + tile_size / 5, yadd + tile_size / 2, west, tile_colours[colours[WEST]][TEXT]);

  cairo_destroy (context);
}

static void
gui_draw_surface (cairo_surface_t * target, gint x, gint y, gboolean prelight, Mover *mover)
{
  gint which, xadd = 0, yadd = 0;
  GtkStateType state;

  which = tiles[y][x].status;
  state = GTK_STATE_NORMAL;

  if (target == buffer) {
    xadd = x * tile_size + xborder + (x >= size) * gap;
    yadd = y * tile_size + yborder;
  }

  if (mover != NULL && target == mover->surface) {
    cairo_pattern_t *pattern;
    xadd = 0;
    yadd = 0;
    pattern = cairo_pattern_create_for_surface (mover->surface);
    gdk_window_set_background_pattern (mover->window, pattern);
    cairo_pattern_destroy (pattern);
    state = GTK_STATE_PRELIGHT;
  }

  if (prelight)
    state = GTK_STATE_PRELIGHT;

  if (which == USED) {
    if (game_state == PAUSED)
      gui_draw_tile (buffer, GTK_STATE_NORMAL, xadd, yadd, 0, 0, 0, 0, FALSE);
    else
      gui_draw_tile (target, state, xadd, yadd, tiles[y][x].n, tiles[y][x].s,
                     tiles[y][x].e, tiles[y][x].w, state);
  }
  else
    gui_draw_socket (target, state, xadd, yadd);

  gtk_widget_queue_draw_area (space, xadd, yadd, tile_size, tile_size);
}

static void
redraw_all (void)
{
  gint x, y;
  cairo_t *context;

  if (!gtk_widget_get_window (space))
    return;

  x = gdk_window_get_width (gtk_widget_get_window (space));
  y = gdk_window_get_height (gtk_widget_get_window (space));

  context = cairo_create (buffer);
  cairo_rectangle (context, 0, 0, x, y);
  gdk_cairo_set_source_pixbuf (context, background_pixbuf, 0, 0);
  cairo_pattern_set_extend (cairo_get_source (context), CAIRO_EXTEND_REPEAT);
  cairo_fill (context);
  cairo_destroy (context);

  for (y = 0; y < size; y++)
    for (x = 0; x < size * 2; x++)
      gui_draw_surface (buffer, x, y, FALSE, NULL);

  gui_draw_arrow(buffer);

  gtk_widget_queue_draw (space);
}

static void
redraw_left (void)
{
  gint x, y;

  for (y = 0; y < size; y++)
    for (x = 0; x < size; x++)
      gui_draw_surface (buffer, x, y, FALSE, NULL);

  gtk_widget_queue_draw_area (space, xborder, yborder, tile_size * size, tile_size * size);
}

/* Enable or disable the game menu items that are only relevant
 * during a game. */
static void
set_game_menu_items_sensitive (gboolean state)
{
  gtk_action_set_sensitive (pause_action, state);
  gtk_action_set_sensitive (hint_action, state);
  gtk_action_set_sensitive (solve_action, state);
}

/* Show only valid options in the move menu. */
static void
update_move_menu_sensitivity (void)
{
  int x, y;
  gboolean clear;
  gboolean n, w, e, s;

  n = w = e = s = TRUE;

  clear = TRUE;
  for (x = 0; x < size; x++) {
    if (tiles[0][x].status == USED)
      n = FALSE;
    if (tiles[x][0].status == USED)
      w = FALSE;
    if (tiles[x][size - 1].status == USED)
      e = FALSE;
    if (tiles[size - 1][x].status == USED)
      s = FALSE;
    for (y = 0; y < size; y++)
      if (tiles[x][y].status == USED)
        clear = FALSE;
  }

  if (clear || (game_state == PAUSED))
    n = w = e = s = FALSE;

  gtk_action_set_sensitive (move_up_action, n);
  gtk_action_set_sensitive (move_left_action, w);
  gtk_action_set_sensitive (move_right_action, e);
  gtk_action_set_sensitive (move_down_action, s);
}

static void
clear_mover(Mover* mover)
{
  if(mover->window != NULL)
    gdk_window_destroy(mover->window);
  mover->window = NULL;
  if (mover->surface)
    cairo_surface_destroy (mover->surface);
  mover->surface = NULL;
}

static void
new_board (gint size)
{
  static gint myrand = 498;
  gint x, y, x1, y1, i, j;
  Tile tmp;

  have_been_hinted = 0;
  solve_me = 0;

  if (timer_timeout) {
    g_source_remove (timer_timeout);
    gtk_widget_set_sensitive (GTK_WIDGET (space), TRUE);
  }

  if (button_down || moving) {
    clear_mover(&mousemover);
    clear_mover(&automover);
    button_down = 0;
    moving = 0;
  }

  g_random_set_seed (time (NULL) + myrand);

  myrand += 17;

  for (y = 0; y < size; y++)
    for (x = 0; x < size; x++)
      tiles[y][x].status = UNUSED;

  for (y = 0; y < size; y++)
    for (x = size; x < size * 2; x++) {
      tiles[y][x].status = USED;
      tiles[y][x].n = g_random_int () % 10;
      tiles[y][x].s = g_random_int () % 10;
      tiles[y][x].w = g_random_int () % 10;
      tiles[y][x].e = g_random_int () % 10;
    }

  /* Sort */
  for (y = 0; y < size; y++)
    for (x = size; x < size * 2 - 1; x++)
      tiles[y][x].e = tiles[y][x + 1].w;
  for (y = 0; y < size - 1; y++)
    for (x = size; x < size * 2; x++)
      tiles[y][x].s = tiles[y + 1][x].n;

  /* Copy tiles to orig_tiles */
  for (y = 0; y < size; y++)
    for (x = 0; x < size; x++)
      orig_tiles[y][x] = tiles[y][x + size];

  /* Unsort */
  j = 0;
  do {
    for (i = 0; i < size * size * size; i++) {
      x = g_random_int () % size + size;
      y = g_random_int () % size;
      x1 = g_random_int () % size + size;
      y1 = g_random_int () % size;
      tmp = tiles[y1][x1];
      tiles[y1][x1] = tiles[y][x];
      tiles[y][x] = tmp;
    }
  } while (tiles[0][size].e == tiles[0][size + 1].w && j++ < 8);
}

static void
get_pixeltilexy (gint x, gint y, gint * xx, gint * yy)
{
  gint sumx = xborder, sumy = yborder;

  if (x >= size)
    sumx += gap;

  sumx += x * tile_size;
  sumy += y * tile_size;
  *xx = sumx;
  *yy = sumy;
}

static void
get_tilexy (gint x, gint y, gint * xx, gint * yy)
{
  /* We return -1, -1 if the location doesn't correspond to a tile. */

  x = x - xborder;
  y = y - yborder;

  if ((x < 0) || (y < 0) ||
      ((x >= size * tile_size) && (x < size * tile_size + gap))) {
    *xx = -1;
    *yy = -1;
    return;
  }

  if (x / tile_size < size)
    *xx = x / tile_size;
  else
    *xx = size + (x - (gap + tile_size * size)) / tile_size;
  *yy = (y / tile_size);

  if ((*xx >= 2 * size) || (*yy >= size)) {
    *xx = -1;
    *yy = -1;
  }
}

static void
get_offsetxy (gint x, gint y, gint * xoff, gint * yoff)
{
  x = x - xborder;
  y = y - yborder;
  if (x / tile_size < size)
    *xoff = x % tile_size;
  else
    *xoff = (x - (gap + tile_size * size)) % tile_size;
  *yoff = y % tile_size;
}

static gboolean
setup_mover (gint x, gint y, Mover *mover)
{
  gint xx, yy;

  get_tilexy (x, y, &xx, &yy);
  if (xx == -1)
    return FALSE; /* No move */
  if (tiles[yy][xx].status == UNUSED)
    return FALSE; /* No move */
  get_offsetxy (x, y, &mover->xoff, &mover->yoff);

  mover->heldtile = tiles[yy][xx];
  mover->xstart = xx;
  mover->ystart = yy;

  clear_mover(mover);

  /* We assume elsewhere that this has the same depth as the parent. */
  windowattrib.width = tile_size;
  windowattrib.height = tile_size;
  mover->window = gdk_window_new (gtk_widget_get_window (space), &windowattrib, 0);
  mover->surface = gdk_window_create_similar_surface (mover->window, CAIRO_CONTENT_COLOR_ALPHA, tile_size, tile_size);
  gdk_window_move (mover->window, x - mover->xoff, y - mover->yoff);
  gui_draw_surface (mover->surface, xx, yy, FALSE, mover);
  gdk_window_show(mover->window);
  /* Show held tile on top if swapping */
  if(mover == &automover && mousemover.window != NULL)
    gdk_window_show(mousemover.window);

  tiles[yy][xx].status = UNUSED;
  gui_draw_surface (buffer, xx, yy, FALSE, NULL);
  return TRUE;
}

static void
swap_without_validation (gint x1, gint y1, gint x2, gint y2)
{
  Tile swp = tiles[y1][x1];
  tiles[y1][x1] = tiles[y2][x2];
  tiles[y2][x2] = swp;
  tiles[y1][x1].status = USED;
  tiles[y2][x2].status = USED;
}

static void
move_tile (gint xx, gint yy, gint x, gint y)
{
  tiles[yy][xx] = tiles[y][x];
  tiles[y][x].status = UNUSED;
}

static void
move_column (unsigned char dir)
{
  gint x, y;
  switch (dir) {
  case 'n':
    for (x = 0; x < size; x++)
      if (tiles[0][x].status == USED)
        return;
    for (y = 1; y < size; y++)
      for (x = 0; x < size; x++)
        move_tile (x, y - 1, x, y);
    redraw_left ();
    break;
  case 's':
    for (x = 0; x < size; x++)
      if (tiles[size - 1][x].status == USED)
        return;
    for (y = size - 2; y >= 0; y--)
      for (x = 0; x < size; x++)
        move_tile (x, y + 1, x, y);
    redraw_left ();
    break;
  case 'w':
    for (y = 0; y < size; y++)
      if (tiles[y][0].status == USED)
        return;
    for (y = 0; y < size; y++)
      for (x = 1; x < size; x++)
        move_tile (x - 1, y, x, y);
    redraw_left ();
    break;
  case 'e':
    for (y = 0; y < size; y++)
      if (tiles[y][size - 1].status == USED)
        return;
    for (y = 0; y < size; y++)
      for (x = size - 2; x >= 0; x--)
        move_tile (x + 1, y, x, y);
    redraw_left ();
    break;
  default:
    break;
  }
  update_move_menu_sensitivity ();
}

static gboolean
game_over (void)
{
  gint x, y;
  for (y = 0; y < size; y++)
    for (x = 0; x < size; x++)
      if (tiles[y][x].status == UNUSED)
        return FALSE;

  return TRUE;
}

static void
message (gchar * message)
{
  guint context_id;

  context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar), "mesasge");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), context_id);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_id, message);
}

static void
pause_game (void)
{
  if (game_state != PAUSED) {
    game_state = PAUSED;
    message (_("Game paused"));
    redraw_all ();
    update_move_menu_sensitivity ();
    gtk_action_set_sensitive (hint_action, FALSE);
    gtk_action_set_sensitive (solve_action, FALSE);
    games_clock_stop (GAMES_CLOCK (timer));
  }
}

static void
resume_game (void)
{
  if (game_state == PAUSED) {
    game_state = PLAYING;
    message ("");
    redraw_all ();
    update_move_menu_sensitivity ();
    gtk_action_set_sensitive (hint_action, TRUE);
    gtk_action_set_sensitive (solve_action, TRUE);
    games_clock_start (GAMES_CLOCK (timer));
  }
}

static void
make_buffer (GtkWidget * widget)
{
  GtkAllocation allocation;

  if (buffer)
    cairo_surface_destroy (buffer);

  gtk_widget_get_allocation (widget, &allocation);
  buffer = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                           CAIRO_CONTENT_COLOR_ALPHA,
                           allocation.width,
                           allocation.height);
}

static void
timer_start (void)
{
  games_clock_stop (GAMES_CLOCK (timer));
  games_clock_reset (GAMES_CLOCK (timer));
  games_clock_start (GAMES_CLOCK (timer));
}

static void
new_game (void)
{
  gchar *str;

  /* Reset pause menu */
  gtk_action_set_sensitive(pause_action, TRUE);

  game_state = GAME_OVER;

  new_board (size);
  make_buffer (space);
  redraw_all ();
  timer_start ();
  set_game_menu_items_sensitive (TRUE);
  update_move_menu_sensitivity ();
  str = g_strdup_printf (_("Playing %d\303\227%d board"), size, size);
  message (str);
  g_free (str);

  game_state = PLAYING;
}

static void
new_game_cb (GtkAction * action, gpointer data)
{
  new_game ();
}

static void
quit_game_cb (void)
{
  gtk_main_quit ();
}

static void
create_window (void)
{
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (window), _(APPNAME_LONG));

  gtk_window_set_default_size (GTK_WINDOW (window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
  games_conf_add_window (GTK_WINDOW (window), NULL);
  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);

  gtk_widget_realize (window);
  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (quit_game_cb), NULL);
}

static gboolean
draw_space (GtkWidget * widget, cairo_t *context)
{
  cairo_set_source_surface (context, buffer, 0, 0);
  cairo_paint (context);

  return FALSE;
}

/* We use this slightly less strict version when dropping tiles. */
static void
get_tilexy_lazy (gint x, gint y, gint * xx, gint * yy)
{
  x = x - xborder;
  y = y - yborder;
  if (x / tile_size < size)
    *xx = x / tile_size;
  else
    *xx = size + (x - (gap + tile_size * size)) / tile_size;
  *yy = (y / tile_size);

  /* Bounds checking */
  if (*xx < 0)
    *xx = 0;
  else if (*xx >= size * 2)
    *xx = size * 2 - 1;
  if (y < 0)
    *yy = 0;
  else if (*yy >= size)
    *yy = size - 1;
}

static void
tile_tilexy (gint x, gint y, gint *xx, gint *yy)
{
  get_tilexy_lazy (x - mousemover.xoff + tile_size / 2,
                   y - mousemover.yoff + tile_size / 2, xx, yy);
}

static gboolean
valid_drop (gint sx, gint sy, gint ex, gint ey)
{
  if (ex >= size)
    return TRUE;

  /* West */
  if (ex != 0 && tiles[ey][ex - 1].status == USED
      && tiles[ey][ex - 1].e != tiles[sy][sx].w)
    return FALSE;
  /* East */
  if (ex != size - 1 && tiles[ey][ex + 1].status == USED
      && tiles[ey][ex + 1].w != tiles[sy][sx].e)
    return FALSE;
  /* North */
  if (ey != 0 && tiles[ey - 1][ex].status == USED
      && tiles[ey - 1][ex].s != tiles[sy][sx].n)
    return FALSE;
  /* South */
  if (ey != size - 1 && tiles[ey + 1][ex].status == USED
      && tiles[ey + 1][ex].n != tiles[sy][sx].s)
    return FALSE;

  return TRUE;
}

static gint
show_score_dialog (gint pos, gboolean endofgame)
{
  static GtkWidget *scoresdialog = NULL;
  gchar *message;
  gint result;

  if (!scoresdialog) {
    scoresdialog = games_scores_dialog_new (GTK_WINDOW (window), highscores, _("Tetravex Scores"));
    games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
                                                  (scoresdialog), _("Size:"));
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

  if (endofgame) {
    games_scores_dialog_set_buttons (GAMES_SCORES_DIALOG (scoresdialog),
                                     GAMES_SCORES_QUIT_BUTTON |
                                     GAMES_SCORES_NEW_GAME_BUTTON);
  } else {
    games_scores_dialog_set_buttons (GAMES_SCORES_DIALOG (scoresdialog), 0);
  }

  result = gtk_dialog_run (GTK_DIALOG (scoresdialog));
  gtk_widget_hide (scoresdialog);

  return result;
}

static void
game_score (void)
{
  gint pos = 0;
  time_t seconds;

  if (!have_been_hinted) {
    seconds = games_clock_get_seconds (GAMES_CLOCK (timer));
    pos = games_scores_add_time_score (highscores, (gfloat) (seconds / 60) + (gfloat) (seconds % 60) / 100);
  }

  if (show_score_dialog (pos, TRUE) == GTK_RESPONSE_REJECT) {
    gtk_main_quit ();
  } else {
    new_game ();
  }
}

static void
place_tile (gint x, gint y)
{
  tiles[automover.yend][automover.xend] = automover.heldtile;
  gui_draw_surface (buffer, automover.xend, automover.yend, FALSE, NULL);

  clear_mover(&automover);
  if (game_over () && game_state != GAME_OVER) {
    game_state = GAME_OVER;
    games_clock_stop (GAMES_CLOCK (timer));
    set_game_menu_items_sensitive (FALSE);
    if (!have_been_hinted) {
      message (_("Puzzle solved! Well done!"));
    } else {
      message (_("Puzzle solved!"));
    }
    game_score ();
  }
  update_move_menu_sensitivity ();
}

static void move_cb (void);

static void
move_held_animate (gint x, gint y, gint tx, gint ty)
{
  /* Need to take over movement from mouse mover to auto mover */
  gint xx, yy;
  move_src_x = x - mousemover.xoff;
  move_src_y = y - mousemover.yoff;
  get_tilexy (move_src_x, move_src_y, &xx, &yy);
  get_pixeltilexy (tx, ty, &move_dest_x, &move_dest_y);

  clear_mover(&automover);
  automover = mousemover;
  mousemover.window = NULL;
  mousemover.surface = NULL;

  automover.xend = tx;
  automover.yend = ty;
  moving = 1;
  if(xx == tx && yy == ty)
    animcount = SHORT_COUNT;
  else
    animcount = LONG_COUNT;
  swapanim = FALSE;
  gtk_widget_set_sensitive (GTK_WIDGET (space), FALSE);
  timer_timeout = g_timeout_add (DELAY, (GSourceFunc) (move_cb), NULL);
}

static void
move_cb (void)
{
  float dx, dy;
  static gint count = 0;
  dx = (float) (move_src_x - move_dest_x) / animcount;
  dy = (float) (move_src_y - move_dest_y) / animcount;
  if (count <= animcount) {
    gdk_window_move (automover.window, move_src_x - (gint) (count * dx),
                 (gint) move_src_y - (gint) (count * dy));
    count++;
  }
  if (count > animcount) {
    count = 0;
    place_tile (move_dest_x + 1, move_dest_y + 1);
    moving = 0;
    g_source_remove (timer_timeout);
    gtk_widget_set_sensitive (GTK_WIDGET (space), TRUE);

    if (swapanim) {
      move_held_animate (mousemover.x, mousemover.y, automover.xstart, automover.ystart);
      return;
    }

    if (game_state != PLAYING)
      return;
    if (solve_me)
      gtk_action_activate (hint_action);
  }
}

static void
move_tile_animate (gint x1, gint y1, gint x2, gint y2, gboolean sa)
{
  get_pixeltilexy (x1, y1, &move_src_x, &move_src_y);
  get_pixeltilexy (x2, y2, &move_dest_x, &move_dest_y);

  setup_mover (move_src_x, move_src_y, &automover);
  automover.xend = x2;
  automover.yend = y2;
  moving = 1;
  animcount = LONG_COUNT;
  swapanim = sa;
  gtk_widget_set_sensitive (GTK_WIDGET (space), FALSE);
  timer_timeout = g_timeout_add (DELAY, (GSourceFunc) (move_cb), NULL);
}

static void
release_tile(gint x, gint y)
{
  gint xx, yy;

  tile_tilexy (x, y, &xx, &yy);
  if (xx >= 0 && xx < size * 2 && yy >= 0 && yy < size) {
    if (tiles[yy][xx].status == UNUSED && valid_drop (mousemover.xstart, mousemover.ystart, xx, yy)) {
      move_held_animate (x, y, xx, yy);
      return;
    } else if (tiles[yy][xx].status == USED) {
      swap_without_validation(xx, yy, mousemover.xstart, mousemover.ystart);
      if(valid_drop (xx, yy, xx, yy) && valid_drop (mousemover.xstart, mousemover.ystart, mousemover.xstart, mousemover.ystart)) {
        swap_without_validation (xx, yy, mousemover.xstart, mousemover.ystart);
        move_tile_animate (xx, yy, mousemover.xstart, mousemover.ystart, TRUE);
        return;
      } else
        swap_without_validation (xx, yy, mousemover.xstart, mousemover.ystart);
    }
  }

  /* Tile needs to go back to its original position */
  move_held_animate (x, y, mousemover.xstart, mousemover.ystart);
}

static gint
button_press_space (GtkWidget * widget, GdkEventButton * event)
{
  if (game_state == PAUSED)
    gtk_action_activate (pause_action);

  if (game_state != PLAYING)
    return FALSE;

  if (event->button != 1)
    return FALSE;

  if (click_to_move)
  {
    if (button_down)
    {
      release_tile (event->x,event->y); /* Seen it happened */
      button_down = 0;
      return FALSE;
    }
    else
    {
      if (setup_mover (event->x, event->y, &mousemover))
        button_down = 1;
    }
  }
  else
  {
    if (button_down == 1)
    {
      release_tile (event->x,event->y); /* Seen it happened */
      button_down = 0;
      return FALSE;
    }
    if (setup_mover (event->x, event->y, &mousemover))
      button_down = 1;
  }

  return FALSE;
}

static gint
button_release_space (GtkWidget * widget, GdkEventButton * event)
{
  /* Ignore when using click to move mode */
  if (click_to_move)
    return FALSE;

  if (event->button == 1) {
    if (button_down == 1) {
      release_tile (event->x, event->y);
    }
    button_down = 0;
  }
  return FALSE;
}

static gint
button_motion_space (GtkWidget * widget, GdkEventButton * event)
{
  static int oldx = -1, oldy = -1;
  gint x, y;

  if (game_state == PAUSED)
    return FALSE;

  if (button_down == 1) {
    mousemover.x = event->x;
    mousemover.y = event->y;
    x = event->x - mousemover.xoff;
    y = event->y - mousemover.yoff;
    gdk_window_move (mousemover.window, x, y);
  }

  /* This code hilights pieces as the mouse moves over them
   * in general imitation of "prelight" in GTK. Need to highlight
   * differently depending on if we are holding a tile or not */
  if(mousemover.window == NULL)
    get_tilexy (event->x, event->y, &x, &y);
  else
    tile_tilexy (event->x, event->y, &x, &y);

  if ((x != oldx) || (y != oldy)) {
    if ((oldx != -1) && (tiles[oldy][oldx].status == USED)) {
      gui_draw_surface (buffer, oldx, oldy, FALSE, NULL);
    }
    if ((x != -1) && (tiles[y][x].status == USED)) {
      gui_draw_surface (buffer, x, y, TRUE, NULL);
    }
    oldx = x;
    oldy = y;
  }

  return FALSE;
}

static void
score_cb (GtkAction * action, gpointer data)
{
  show_score_dialog (0, FALSE);
}

static void
update_tile_size (gint screen_width, gint screen_height)
{
  gint xt_size, yt_size;

  /* We aim for the gap and the corners to be 1/2 a tile wide. */
  xt_size = (2 * screen_width) / (4 * size + 3);
  yt_size = screen_height / (size + 1);
  tile_size = MIN (xt_size, yt_size);
  gap = (screen_width - 2 * size * tile_size) / 3;
  xborder = gap;
  yborder = (screen_height - size * tile_size) / 2;

  /* Set tile edge to a percentage of the tile size */
  tile_border_size = 0.05 * tile_size;
  if (tile_border_size < 1.0)
    tile_border_size = 1.0;
  else if (tile_border_size > 5.0)
    tile_border_size = 5.0;

  /* Make arrow less sunken */
  arrow_border_size = 0.5 * tile_border_size;
  if (arrow_border_size < 1.0)
    arrow_border_size = 1.0;

  /* Rebuild the tile/socket vertices when required */
  rebuild_vertices = TRUE;
}

static gboolean
configure_space (GtkWidget * widget, GdkEventConfigure * event)
{
  update_tile_size (event->width, event->height);
  make_buffer (widget);
  redraw_all ();

  return FALSE;
}

static GtkWidget *
create_statusbar (void)
{
  GtkWidget *status_bar, *time_label, *time_box;

  time_box = gtk_hbox_new (FALSE, 0);
  time_label = gtk_label_new (_("Time:"));
  gtk_box_pack_start (GTK_BOX (time_box), time_label, FALSE, FALSE, 0);
  time_label = gtk_label_new (" ");
  gtk_box_pack_start (GTK_BOX (time_box), time_label, FALSE, FALSE, 0);
  timer = games_clock_new ();
  gtk_box_pack_start (GTK_BOX (time_box), timer, FALSE, FALSE, 0);

  status_bar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (status_bar), time_box, FALSE, FALSE, 0);

  return status_bar;
}

static void
init_window_attrib (void)
{
  /* The depth of mover.window must match the depth of gtk_widget_get_window (space). */
  windowattrib.wclass = GDK_INPUT_OUTPUT;
  windowattrib.window_type = GDK_WINDOW_CHILD;
  windowattrib.event_mask = 0;
  windowattrib.width = tile_size;
  windowattrib.height = tile_size;
  windowattrib.visual = gdk_window_get_visual (gtk_widget_get_window (space));
}

static void
pause_cb (void)
{
  if (game_state == GAME_OVER)
    return;

  if (game_state != PAUSED) {
    pause_game ();
  } else {
    resume_game ();
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
  quit_game_cb ();

  return FALSE;
}

#endif /* WITH_SMCLIENT */

static void
find_first_tile (gint status, gint * xx, gint * yy)
{
  gint x, y;
  for (y = 0; y < size; y++)
    for (x = size; x < size * 2; x++)
      if (tiles[y][x].status == status) {
        *xx = x;
        *yy = y;
        return;
      }
}

static gboolean
tiles_match (Tile * t1, Tile * t2)
{
  if (t1->e == t2->e && t1->w == t2->w && t1->s == t2->s && t1->n == t2->n)
    return FALSE;
  return TRUE;
}

static void
hint_move (gint x1, gint y1, gint x2, gint y2)
{
  have_been_hinted = 1;
  move_tile_animate (x1, y1, x2, y2, FALSE);
}

static void
hint_cb (GtkAction * action, gpointer data)
{
  gint x1, y1, x2 = 0, y2 = 0, x = 0, y = 0;
  Tile hint_tile;

  if ((game_state != PLAYING) || button_down || moving)
    return;

  find_first_tile (USED, &x, &y);
  x1 = x;
  y1 = y;
  hint_tile = tiles[y][x];

  /* Find position in original map */
  for (y = 0; y < size; y++)
    for (x = 0; x < size; x++)
      if (!tiles_match (&hint_tile, &orig_tiles[y][x])) {
        if (tiles[y][x].status == USED
            && !tiles_match (&hint_tile, &tiles[y][x])) {
        /* Do Nothing */
        } else {
          x2 = x;
          y2 = y;
          x = size;
          y = size;
        }
      }

  /* Tile I want to hint about is busy. Move the busy tile away! */
  if (tiles[y2][x2].status == USED) {
    find_first_tile (UNUSED, &x1, &y1);
    hint_move (x2, y2, x1, y1);
    return;
  }

  /* West */
  if (x2 != 0 && tiles[y2][x2 - 1].status == USED
      && tiles[y2][x2 - 1].e != hint_tile.w) {
    find_first_tile (UNUSED, &x1, &y1);
    hint_move (x2 - 1, y2, x1, y1);
    return;
  }

  /* East */
  if (x2 != size - 1 && tiles[y2][x2 + 1].status == USED
      && tiles[y2][x2 + 1].w != hint_tile.e) {
    find_first_tile (UNUSED, &x1, &y1);
    hint_move (x2 + 1, y2, x1, y1);
    return;
  }

  /* North */
  if (y2 != 0 && tiles[y2 - 1][x2].status == USED
      && tiles[y2 - 1][x2].s != hint_tile.n) {
    find_first_tile (UNUSED, &x1, &y1);
    hint_move (x2, y2 - 1, x1, y1);
    return;
  }

  /* South */
  if (y2 != size - 1 && tiles[y2 + 1][x2].status == USED
      && tiles[y2 + 1][x2].n != hint_tile.s) {
    find_first_tile (UNUSED, &x1, &y1);
    hint_move (x2, y2 + 1, x1, y1);
    return;
  }

  hint_move (x1, y1, x2, y2);
}

static void
solve_cb (GtkAction * action, gpointer data)
{
  solve_me = 1;
  gtk_action_activate (hint_action);
}

static void
help_cb (GtkAction * action, gpointer data)
{
  games_help_display (window, "gnotravex", NULL);
}

static void
about_cb (GtkAction * action, gpointer data)
{
  const gchar *authors[] = { "Lars Rydlinge", NULL };

  const gchar *documenters[] = { "Rob Bradford", NULL };

  gchar *license = games_get_license (_(APPNAME_LONG));

  gtk_show_about_dialog (GTK_WINDOW (window),
                         "program-name", _(APPNAME_LONG),
                         "version", VERSION,
                         "comments",
                         _("GNOME Tetravex is a simple puzzle where "
                           "pieces must be positioned so that the "
                           "same numbers are touching each other.\n\n"
                           "Tetravex is a part of GNOME Games."),
                         "copyright",
                         "Copyright \xc2\xa9 1999-2008 Lars Rydlinge",
                         "license", license,
                         "wrap-license", TRUE,
                         "authors", authors,
                         "documenters", documenters,
                         "translator-credits", _("translator-credits"),
                         "logo-icon-name", "gnome-tetravex",
                         "website", "http://www.gnome.org/projects/gnome-games",
                         "website-label", _("GNOME Games web site"),
                         NULL);
  g_free (license);
}

static void
size_cb (GtkAction * action, gpointer data)
{
  gint newsize;
  gint width, height;

  newsize = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  width = gdk_window_get_width (gtk_widget_get_window (space));
  height = gdk_window_get_height (gtk_widget_get_window (space));

  if (game_state == PAUSED)
    gtk_action_activate (pause_action);

  if (size == newsize)
    return;
  size = newsize;
  update_tile_size (width, height);
  games_scores_set_category (highscores, scorecats[size - 2].key);
  games_conf_set_integer (NULL, KEY_GRID_SIZE, size);
  gtk_action_activate (new_game_action);
}

static void
clickmove_toggle_cb(GtkToggleAction * togglebutton, gpointer data)
{
  click_to_move = gtk_toggle_action_get_active (togglebutton);
  games_conf_set_boolean (NULL, KEY_CLICK_MOVE, click_to_move);
}

static void
move_up_cb (GtkAction * action, gpointer data)
{
  move_column ('n');
}

static void
move_left_cb (GtkAction * action, gpointer data)
{
  move_column ('w');
}

static void
move_right_cb (GtkAction * action, gpointer data)
{
  move_column ('e');
}

static void
move_down_cb (GtkAction * action, gpointer data)
{
  move_column ('s');
}

static void
load_background (void)
{
  char *path;
  const char * dname;
  const char * filename = "baize.png";
  GError *error = NULL;

  dname = games_runtime_get_directory (GAMES_RUNTIME_PIXMAP_DIRECTORY);
  path = g_build_filename (dname, filename, NULL);
  background_pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (background_pixbuf == NULL) {
    g_warning ("Error loading file '%s': %s\n", path, error->message);
    g_error_free (error);

    background_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                        FALSE,
                                        8,1,1);
    gdk_pixbuf_fill (background_pixbuf, 0xffffffff);
  }

  g_free (path);

}

static GtkWidget *
create_menu (GtkUIManager * ui_manager)
{
  gint i;
  GtkActionGroup *action_group;
  GtkAction *action;
  const GtkActionEntry action_entry[] = {
    {"GameMenu", NULL, N_("_Game")},
    {"MoveMenu", NULL, N_("_Move")},
    {"SettingsMenu", NULL, N_("_Settings")},
    {"SizeMenu", NULL, N_("_Size")},
    {"HelpMenu", NULL, N_("_Help")},
    {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL,
     G_CALLBACK (new_game_cb)},
    {"Hint", GAMES_STOCK_HINT, NULL, NULL, NULL, G_CALLBACK (hint_cb)},
    {"Solve", GTK_STOCK_REFRESH, N_("Sol_ve"), NULL, N_("Solve the game"),
     G_CALLBACK (solve_cb)},
    {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL, G_CALLBACK (score_cb)},
    {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_game_cb)},
    {"MoveUp", GTK_STOCK_GO_UP, N_("_Up"), "<control>Up",
     N_("Move the pieces up"), G_CALLBACK (move_up_cb)},
    {"MoveLeft", GTK_STOCK_GO_BACK, N_("_Left"), "<control>Left",
     N_("Move the pieces left"), G_CALLBACK (move_left_cb)},
    {"MoveRight", GTK_STOCK_GO_FORWARD, N_("_Right"), "<control>Right",
     N_("Move the pieces right"), G_CALLBACK (move_right_cb)},
    {"MoveDown", GTK_STOCK_GO_DOWN, N_("_Down"), "<control>Down",
     N_("Move the pieces down"), G_CALLBACK (move_down_cb)},
    {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (help_cb)},
    {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_cb)}
  };
  const GtkToggleActionEntry toggles[] = {
    {"ClickToMove", NULL, N_("_Click to Move"), NULL, "Pick up and drop tiles by clicking",
     G_CALLBACK (clickmove_toggle_cb)}
  };

  action_group = gtk_action_group_new ("actions");

  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, action_entry,
                                G_N_ELEMENTS (action_entry), window);
  gtk_action_group_add_radio_actions (action_group, size_action_entry,
                                      G_N_ELEMENTS (size_action_entry), -1,
                                      G_CALLBACK (size_cb), NULL);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);

  new_game_action = gtk_action_group_get_action (action_group, "NewGame");
  hint_action = gtk_action_group_get_action (action_group, "Hint");
  solve_action = gtk_action_group_get_action (action_group, "Solve");
  scores_action = gtk_action_group_get_action (action_group, "Scores");
  move_up_action = gtk_action_group_get_action (action_group, "MoveUp");
  move_left_action = gtk_action_group_get_action (action_group, "MoveLeft");
  move_right_action = gtk_action_group_get_action (action_group, "MoveRight");
  move_down_action = gtk_action_group_get_action (action_group, "MoveDown");
  pause_action = GTK_ACTION (games_pause_action_new ("PauseGame"));
  g_signal_connect (G_OBJECT (pause_action), "state-changed", G_CALLBACK (pause_cb), NULL);
  gtk_action_group_add_action_with_accel (action_group, pause_action, NULL);
  fullscreen_action = GTK_ACTION (games_fullscreen_action_new ("Fullscreen", GTK_WINDOW(window)));
  gtk_action_group_add_action_with_accel (action_group, fullscreen_action, NULL);

  gtk_action_group_add_toggle_actions (action_group, toggles,
                                       G_N_ELEMENTS (toggles), NULL);
  action = gtk_action_group_get_action (action_group, "ClickToMove");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), click_to_move);

  for (i = 0; i < G_N_ELEMENTS (size_action_entry); i++)
    size_action[i] =
      gtk_action_group_get_action (action_group, size_action_entry[i].name);

  return gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
}

int
main (int argc, char **argv)
{
  GOptionContext *context;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  gboolean retval;
  GError *error = NULL;
#ifdef WITH_SMCLIENT
  EggSMClient *sm_client;
#endif /* WITH_SMCLIENT */

  if (!games_runtime_init ("gnotravex"))
    return 1;

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

  g_set_application_name (_(APPNAME_LONG));

  games_conf_initialise (APPNAME);

  highscores = games_scores_new ("gnotravex",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 NULL, NULL,
                                 1 /* default category */,
                                 GAMES_SCORES_STYLE_TIME_ASCENDING);

  games_stock_init ();

  gtk_window_set_default_icon_name ("gnome-tetravex");

#ifdef WITH_SMCLIENT
  sm_client = egg_sm_client_get ();
  g_signal_connect (sm_client, "save-state",
                    G_CALLBACK (save_state_cb), NULL);
  g_signal_connect (sm_client, "quit",
                    G_CALLBACK (quit_cb), NULL);
#endif /* WITH_SMCLIENT */

  if (size == -1)
    size = games_conf_get_integer (NULL, KEY_GRID_SIZE, NULL);
  if (size < 2 || size > 6)
    size = 3;
  games_scores_set_category (highscores, scorecats[size - 2].key);

  click_to_move = games_conf_get_boolean (NULL, KEY_CLICK_MOVE, NULL);

  load_background ();
  create_window ();

  space = gtk_drawing_area_new ();
  gtk_widget_set_events (space,
                         GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
                         | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

  statusbar = create_statusbar ();

  ui_manager = gtk_ui_manager_new ();
  games_stock_prepare_for_statusbar_tooltips (ui_manager, statusbar);

  menubar = create_menu (ui_manager);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), space, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  gtk_widget_realize (space);

  g_signal_connect (G_OBJECT (space), "draw",
                    G_CALLBACK (draw_space), NULL);
  g_signal_connect (G_OBJECT (space), "configure_event",
                    G_CALLBACK (configure_space), NULL);
  g_signal_connect (G_OBJECT (space), "button_press_event",
                    G_CALLBACK (button_press_space), NULL);
  g_signal_connect (G_OBJECT (space), "button_release_event",
                    G_CALLBACK (button_release_space), NULL);
  g_signal_connect (G_OBJECT (space), "motion_notify_event",
                    G_CALLBACK (button_motion_space), NULL);
  /* We do our own double-buffering. */
  gtk_widget_set_double_buffered (space, FALSE);

  gtk_widget_show (space);

  if (session_xpos >= 0 && session_ypos >= 0)
    gtk_window_move (GTK_WINDOW (window), session_xpos, session_ypos);

  gtk_widget_show_all (window);
  init_window_attrib ();

  gtk_action_activate (new_game_action);

  gtk_action_activate (size_action[size - 2]);

  gtk_main ();

  games_conf_shutdown ();

  games_runtime_shutdown ();

  return 0;
}
