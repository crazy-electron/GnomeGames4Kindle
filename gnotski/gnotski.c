/* -*- mode:C; indent-tabs-mode: nil; tab-width: 8; c-basic-offset: 2; -*- */

/* 
 *   Gnome Klotski
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
#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libgames-support/games-conf.h>
#include <libgames-support/games-gridframe.h>
#include <libgames-support/games-help.h>
#include <libgames-support/games-preimage.h>
#include <libgames-support/games-runtime.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-scores-dialog.h>
#include <libgames-support/games-stock.h>
#include <libgames-support/games-fullscreen-action.h>

#ifdef WITH_SMCLIENT
#include <libgames-support/eggsmclient.h>
#endif /* WITH_SMCLIENT */

#include "pieces.h"

#define APPNAME "gnotski"
#define APPNAME_LONG N_("Klotski")

#define MINWIDTH 250
#define MINHEIGHT 250
#define THEME_TILE_CENTER 14
#define THEME_TILE_SIZE 34
#define THEME_TILE_SEGMENTS 27
#define THEME_OVERLAY_SIZE 8
#define SPACE_PADDING 5
#define SPACE_OFFSET 4

#define KEY_LEVEL "level"
#define KEY_LEVEL_INFO_GROUP "level_info"

GtkWidget *window;
GtkWidget *statusbar;
GtkWidget *space;
GtkWidget *gameframe;
GtkWidget *messagewidget;
GtkWidget *moveswidget;

cairo_surface_t *buffer = NULL;
GdkPixbuf *tiles_pixbuf = NULL;
GamesPreimage *tiles_preimage;

GtkActionGroup *action_group;
GtkAction *fullscreen_action;

gboolean clear_buffer = TRUE;
gboolean clear_game = TRUE;

gchar *map = NULL;
gchar *tmpmap = NULL;
gchar *move_map = NULL;
gchar *orig_map = NULL;
gchar *lastmove_map = NULL;
gchar *undomove_map = NULL;

gint space_width = 0;
gint space_height = 0;
gint tile_size = 0;
gint prior_tile_size = 0;
gint height = -1;
gint width = -1;
gint moves = 0;
gint session_xpos = 0;
gint session_ypos = 0;
gint current_level = -1;

guint redraw_all_idle_id = 0;
guint configure_idle_id = 0;

static const GamesScoresCategory scorecats[] = {
{"1", N_("Only 18 steps")},
{"2", N_("Daisy")},
{"3", N_("Violet")},
{"4", N_("Poppy")},
{"5", N_("Pansy")},
{"6", N_("Snowdrop")},
{"7", N_("Red Donkey")},
{"8", N_("Trail")},
{"9", N_("Ambush")},
{"10", N_("Agatka")},
{"11", N_("Success")},
{"12", N_("Bone")},
{"13", N_("Fortune")},
{"14", N_("Fool")},
{"15", N_("Solomon")},
{"16", N_("Cleopatra")},
{"17", N_("Shark")},
{"18", N_("Rome")},
{"19", N_("Pennant Puzzle")},
{"20", N_("Ithaca")},
{"21", N_("Pelopones")},
{"22", N_("Transeuropa")},
{"23", N_("Lodzianka")},
{"24", N_("Polonaise")},
{"25", N_("Baltic Sea")},
{"26", N_("American Pie")},
{"27", N_("Traffic Jam")},
{"28", N_("Sunshine")}
};

GamesScores *highscores;

void create_space (void);
GtkWidget *create_menubar (void);
void create_statusbar (void);

void load_image (void);
gchar *get_level_key (gint);
void load_solved_state (void);
void gui_draw_space (void);
void gui_draw_pixmap (char *, gint, gint);
gint get_piece_nr (char *, gint, gint);
gint get_piece_id (char *, gint, gint);
void set_piece_id (char *, gint, gint, gint);
gint move_piece (gint, gint, gint, gint, gint);
void copymap (char *, char *);
gint mapcmp (char *, char *);
#ifdef WITH_SMCLIENT
static int save_state_cb (EggSMClient *client, GKeyFile *keyfile, gpointer client_data);
static int quit_cb (EggSMClient *client, gpointer client_data);
#endif /* WITH_SMCLIENT */     
void new_move (void);
void game_score (void);
gint game_over (void);

gboolean window_resize_cb (GtkWidget *, GdkEventConfigure *, gpointer);

void new_game (gint requested_level);

void next_level_cb (GtkAction *);
void prev_level_cb (GtkAction *);
void level_cb (GtkAction *, GtkRadioAction *);
void quit_game_cb (GtkAction *);
void restart_level_cb (GtkAction *);
void help_cb (GtkAction *);
void about_cb (GtkAction *);
void score_cb (GtkAction *);

/* Puzzle Info */

typedef struct _levelinfo {
  gchar *name;
  gint group;
  gint width;
  gint height;
  gint minimum_moves;
  gchar *data;
} levelinfo;

/* The "puzzle name" remarks provide context for translation. */
const levelinfo level[] = {
  /* puzzle name */
  {N_("Only 18 Steps"), 0,
   6, 9, 18,
   "######"
   "#a**b#" "#m**n#" "#cdef#" "#ghij#" "#k  l#" "##--##" "    .." "    .."},

  /* puzzle name */
  {N_("Daisy"), 0,
   6, 9, 28,
   "######"
   "#a**b#" "#a**b#" "#cdef#" "#zghi#" "#j  k#" "##--##" "    .." "    .."},

  /* puzzle name */
  {N_("Violet"), 0,
   6, 9, 27,
   "######"
   "#a**b#" "#a**b#" "#cdef#" "#cghi#" "#j  k#" "##--##" "    .." "    .."},

  /* puzzle name */
  {N_("Poppy"), 0,
   6, 9, 40,
   "######"
   "#a**b#" "#a**b#" "#cdde#" "#fghi#" "#j  k#" "##--##" "    .." "    .."},

  /* puzzle name */
  {N_("Pansy"), 0,
   6, 9, 28,
   "######"
   "#a**b#" "#a**b#" "#cdef#" "#cghf#" "#i  j#" "##--##" "    .." "    .."},

  /* puzzle name */
  {N_("Snowdrop"), 0,
   6, 9, 46,
   "######"
   "#a**b#" "#a**b#" "#cdde#" "#cfgh#" "#i  j#" "##--##" "    .." "    .."},

  /* puzzle name - sometimes called "Le'Ane Rouge" */
  {N_("Red Donkey"), 0,
   6, 9, 81,
   "######"
   "#a**b#" "#a**b#" "#cdde#" "#cfge#" "#h  i#" "##--##" "    .." "    .."},

  /* puzzle name */
  {N_("Trail"), 0,
   6, 9, 102,
   "######"
   "#a**c#" "#a**c#" "#eddg#" "#hffj#" "# ii #" "##--##" "    .." "    .."},

  /* puzzle name */
  {N_("Ambush"), 0,
   6, 9, 120,
   "######"
   "#a**c#" "#d**e#" "#dffe#" "#ghhi#" "# jj #" "##--##" "    .." "    .."},

  /* puzzle name */
  {N_("Agatka"), 1,
   7, 7, 30,
   "..     " ".      " "#####--" "#**aab-" "#*ccde#" "#fgh  #" "#######"},

  /* puzzle name */
  {N_("Success"), 1,
   9, 6, 25,
   "#######  " "#**bbc#  " "#defgh#  " "#ijkgh-  " "#llk  #  " "#######.."},

  /* puzzle name */
  {N_("Bone"), 1,
   6, 9, 14,
   "######"
   "#abc*#" "# dd*#" "# ee*#" "# fgh#" "##-###" "     ." "     ." "     ."},

  /* puzzle name */
  {N_("Fortune"), 1,
   7, 10, 25,
   "     .."
   "     . "
   "####-. "
   "#ab  - " "#ccd # " "#ccd # " "#**ee# " "#*fgh# " "#*iih# " "###### "},

  /* puzzle name */
  {N_("Fool"), 1,
   10, 6, 29,
   "  ########"
   "  -aabc  #" "  #aabdef#" "  #ijggef#" "  #klhh**#" "..########"},

  /* puzzle name */
  {N_("Solomon"), 1,
   7, 9, 29,
   " .     "
   "..     "
   "#--####" "#  aab#" "# cdfb#" "#hcefg#" "#hijk*#" "#hll**#" "#######"},

  /* puzzle name */
  {N_("Cleopatra"), 1,
   6, 8, 32,
   "######" "#abcd#" "#**ee#" "#f*g #" "#fh i-" "####--" "    .." "     ."},

  /* puzzle name */
  {N_("Shark"), 1,
   11, 8, 0,			/* SOLVEME */
   "########   "
   "#nrr s #   "
   "#n*op q#   "
   "#***jml#   " "#hhijkl#   " "#ffcddg-   " "#abcdde- . " "########..."},

  /* puzzle name */
  {N_("Rome"), 1,
   8, 8, 38,
   "########"
   "#abcc**#"
   "#ddeef*#" "#ddghfi#" "#   jki#" "#--#####" " ..     " "  .     "},

  /* puzzle name */
  {N_("Pennant Puzzle"), 1,
   6, 9, 59,
   "######"
   "#**aa#" "#**bb#" "#de  #" "#fghh#" "#fgii#" "#--###" "    .." "    .."},

  /* puzzle name */
  {N_("Ithaca"), 2,
   19, 19, 0,			/* SOLVEME */
   ".aaaaaaaaaaaaaaaaab"
   "..  cddeffffffffffb"
   " .. cddeffffffffffb"
   "  . cddeffffffffffb"
   "ggg-############hhb"
   "ggg-  ABCDEFFGH#hhb"
   "ggg-       FFIJ#hhb"
   "ggg#       KLMJ#hhb"
   "ggg#NNNNOOOPQMJ#hhb"
   "ggg#NNNNOOOP*RS#hhb"
   "ggg#TTTTTUVW**X#hhb"
   "ggg#YZ12222W3**#hhb"
   "ggg#YZ12222W34*#iib"
   "jjj#YZ155555367#klb"
   "jjj#############mmb"
   "jjjnooooooooooppppb"
   "jjjqooooooooooppppb" "       rrrssssppppb" "ttttttuvvvvvvvwwwwx"},

  /* puzzle name */
  {N_("Pelopones"), 2,
   9, 8, 0,			/* SOLVEME */
   "#########"
   "#abbb***#"
   "#abbb*c*#" "#adeefgg#" "#  eefhh#" "#... ihh#" "#. . ihh#" "#########"},

  /* puzzle name */
  {N_("Transeuropa"), 2,
   15, 8, 0,			/* SOLVEME */
   "    ###########"
   "    -AAAAABBCC#"
   "    -   DEFGHI#"
   "    #   DEFGJI#"
   "    #   KEFGLI#" "    #   KEFG*I#" "  . #   MM****#" "....###########"},

  /* puzzle name */
  {N_("Lodzianka"), 2,
   9, 7, 0,			/* SOLVEME */
   "#########"
   "#**abbcc#" "#**abbdd#" "#eefgh  #" "#iiijk..#" "#iiijk..#" "#########"},

  /* puzzle name */
  {N_("Polonaise"), 2,
   7, 7, 0,			/* SOLVEME */
   "#######" "#aab**#" "#aabc*#" "#defgg#" "#..fhh#" "# .ihh#" "#######"},

  /* puzzle name */
  {N_("Baltic Sea"), 2,
   6, 8, 42,
   "######" "#.abc#" "#.dec#" "#fggc#" "#fhhi#" "#fjk*#" "#flk*#" "######"},

  /* puzzle name */
  {N_("American Pie"), 2,
   10, 12, 0,			/* SOLVEME */
   "##########"
   "#a*bcdefg#"
   "#**bhhhhg#"
   "#*iijjkkg#"
   "#liimnoop#"
   "#qiirrr  #"
   "#qstuvv  #"
   "#qwwxvv  #" "######--##" "         ." "        .." "        . "},

  /* puzzle name */
  {N_("Traffic Jam"), 2,
   10, 7, 132,
   "########  "
   "#** ffi#  "
   "#** fgh#  " "#aacehh#  " "#bbdjlm-  " "#bddklm-.." "########.."},

  /* puzzle name */
  {N_("Sunshine"), 2,
   17, 22, 345,
   "       ...       "
   "      .. ..      "
   "      .   .      "
   "      .. ..      "
   "       ...       "
   "######-----######"
   "#hh0iilltmmpp;qq#"
   "#hh,iill mmpp:qq#"
   "#2y{45v s w89x/z#"
   "#jj6kkaa nnoo<rr#"
   "#jj7kkaaunnoo>rr#"
   "#33333TTJWW11111#"
   "#33333TTJWW11111#"
   "#33333GG HH11111#"
   "#33333YYIgg11111#"
   "#33333YYIgg11111#"
   "#ddFeeA***BffOZZ#"
   "#ddFee** **ffOZZ#"
   "#MMKQQ*   *PPS^^#"
   "#VVLXX** **bbRcc#" "#VVLXXD***EbbRcc#" "#################"},
};

const gint max_level = G_N_ELEMENTS (level) - 1;
GtkToggleAction *level_action[G_N_ELEMENTS (level)];
GtkWidget *level_image[G_N_ELEMENTS (level)];

/* Menu Info */

const char *pack_uipath[] = {
  "/ui/MainMenu/GameMenu/HuaRongTrail",
  "/ui/MainMenu/GameMenu/ChallengePack",
  "/ui/MainMenu/GameMenu/SkillPack",
};

const GtkActionEntry entries[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"ViewMenu", NULL, N_("_View")},
  {"HelpMenu", NULL, N_("_Help")},
  /* set of puzzles */
  {"HuaRongTrail", NULL, N_("HuaRong Trail")},
  /* set of puzzles */
  {"ChallengePack", NULL, N_("Challenge Pack")},
  /* set of puzzles */
  {"SkillPack", NULL, N_("Skill Pack")},
  {"RestartPuzzle", GTK_STOCK_REFRESH, N_("_Restart Puzzle"), "<control>R",
   NULL, G_CALLBACK (restart_level_cb)},
  {"NextPuzzle", GTK_STOCK_GO_FORWARD, N_("Next Puzzle"), "Page_Down", NULL,
   G_CALLBACK (next_level_cb)},
  {"PrevPuzzle", GTK_STOCK_GO_BACK, N_("Previous Puzzle"), "Page_Up", NULL,
   G_CALLBACK (prev_level_cb)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (quit_game_cb)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (help_cb)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (about_cb)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL, G_CALLBACK (score_cb)}
};

const char ui_description[] =
  "<ui>"
  "  <menubar name='MainMenu'>"
  "    <menu action='GameMenu'>"
  "      <menuitem action='RestartPuzzle'/>"
  "      <menuitem action='NextPuzzle'/>"
  "      <menuitem action='PrevPuzzle'/>"
  "      <separator/>"
  "      <menu action='HuaRongTrail'/>"
  "      <menu action='ChallengePack'/>"
  "      <menu action='SkillPack'/>"
  "      <separator/>"
  "      <menuitem action='Scores'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
  "    </menu>"
  "    <menu action='ViewMenu'>"
  "      <menuitem action='Fullscreen'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>" "    </menu>" "  </menubar>" "</ui>";

/* Session Options */

static const GOptionEntry options[] = {
  {"x", 'x', 0, G_OPTION_ARG_INT, &session_xpos, N_("X location of window"),
   N_("X")},
  {"y", 'y', 0, G_OPTION_ARG_INT, &session_ypos, N_("Y location of window"),
   N_("Y")},
  {NULL}
};

/* ------------------------------------------------------- */

int
main (int argc, char **argv)
{
  GOptionContext *context;
  GtkWidget *vbox;
  GtkWidget *menubar;
  gint startup_level;
  gboolean retval;
  GError *error = NULL;
#ifdef WITH_SMCLIENT
  EggSMClient *sm_client;
#endif /* WITH_SMCLIENT */

  if (!games_runtime_init ("gnotski"))
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

  games_stock_init ();

  gtk_window_set_default_icon_name ("gnome-klotski");
  
#ifdef WITH_SMCLIENT
  sm_client = egg_sm_client_get ();
  g_signal_connect (sm_client, "save-state",
		    G_CALLBACK (save_state_cb), NULL);
  g_signal_connect (sm_client, "quit",
                    G_CALLBACK (quit_cb), NULL);
#endif /* WITH_SMCLIENT */

  highscores = games_scores_new ("gnotski",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 NULL, NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_ASCENDING);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), _(APPNAME_LONG));

  gtk_window_set_default_size (GTK_WINDOW (window), MINWIDTH, MINHEIGHT);
  games_conf_add_window (GTK_WINDOW (window), NULL);

  startup_level = games_conf_get_integer (NULL, KEY_LEVEL, NULL);

  g_signal_connect (window, "delete_event",
		    G_CALLBACK (quit_game_cb), NULL);


  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  load_image ();
  create_space ();
  menubar = create_menubar ();
  create_statusbar ();
  load_solved_state ();

  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), gameframe, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  if (session_xpos >= 0 && session_ypos >= 0)
    gtk_window_move (GTK_WINDOW (window), session_xpos, session_ypos);

  gtk_widget_show_all (window);

  new_game (startup_level);

  gtk_main ();

  games_conf_shutdown ();

  games_runtime_shutdown ();

  return 0;
}

static gboolean
draw_space (GtkWidget * widget, cairo_t *cr)
{
  if (clear_game)
    return FALSE;

  cairo_set_source_surface (cr, buffer, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

static gboolean
redraw_all (void)
{
  gint x, y;

  if (clear_buffer)
    gui_draw_space ();

  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      gui_draw_pixmap (map, x, y);

  return FALSE;
}

static gboolean
movable (gint id)
{
  if (id == '#' || id == '.' || id == ' ' || id == '-')
    return FALSE;
  return TRUE;
}

gint button_down = 0;
gint piece_id = -1;
gint piece_x = 0;
gint piece_y = 0;

static gint
button_press_space (GtkWidget * widget, GdkEventButton * event)
{
  if (event->button == 1) {
    if (game_over ())
      return FALSE;
    button_down = 1;
    piece_x = (gint) event->x / tile_size;
    piece_y = (gint) event->y / tile_size;
    piece_id = get_piece_id (map, piece_x, piece_y);
    copymap (move_map, map);
  }
  return FALSE;
}

static gint
button_release_space (GtkWidget * widget, GdkEventButton * event)
{
  if (event->button == 1) {
    if (button_down == 1) {
      if (movable (piece_id))
	if (mapcmp (move_map, map)) {
	  new_move ();
	  if (game_over ()) {
	    gtk_label_set_text (GTK_LABEL (messagewidget),
				_("Level completed."));
	    game_score ();
	  }
	}
      button_down = 0;
    }
  }
  return FALSE;
}

static gint
button_motion_space (GtkWidget * widget, GdkEventButton * event)
{
  gint new_piece_x, new_piece_y;
  if (button_down == 1) {
    new_piece_x = (gint) event->x / tile_size;
    new_piece_y = (gint) event->y / tile_size;
    if (new_piece_x >= width || event->x < 0
	|| new_piece_y >= height || event->y < 0)
      return FALSE;
    if (movable (piece_id))
      if (move_piece (piece_id, piece_x, piece_y, new_piece_x, new_piece_y) ==
	  1) {
	piece_x = new_piece_x;
	piece_y = new_piece_y;
      }
    return TRUE;
  }
  return FALSE;
}

void
gui_draw_space (void)
{
  cairo_t *cr;
  GtkStyle *style;

  style = gtk_widget_get_style (space);

  if (buffer)
    cairo_surface_destroy (buffer);

  buffer = gdk_window_create_similar_surface (gtk_widget_get_window (space),
			   CAIRO_CONTENT_COLOR_ALPHA,
			   width * tile_size + SPACE_PADDING,
			   height * tile_size + SPACE_PADDING);

  cr = cairo_create (buffer);

  gdk_cairo_set_source_color (cr, &style->bg[GTK_STATE_NORMAL]);
  cairo_paint (cr);

  gdk_cairo_set_source_color (cr, &style->fg[GTK_STATE_NORMAL]);
  cairo_set_line_width (cr, 1.0);
  cairo_rectangle (cr, 1.5, 1.5, width * tile_size + SPACE_PADDING - 2.0,
		      height * tile_size + SPACE_PADDING - 2.0);
  cairo_stroke (cr);

  cairo_destroy (cr);

  clear_buffer = clear_game = FALSE;

  gtk_widget_queue_draw (space);
}

void
gui_draw_pixmap (char *target, gint x, gint y)
{
  gint value;
  gint overlay_size;
  gint overlay_offset;
  cairo_t *cr;
  GtkStyle *style;
  GdkRectangle rect;

  rect.x = x * tile_size + SPACE_OFFSET;
  rect.y = y * tile_size + SPACE_OFFSET;
  rect.width = tile_size;
  rect.height = tile_size;

  style = gtk_widget_get_style (space);

  cr = cairo_create (buffer);
  gdk_cairo_rectangle (cr, &rect);
  gdk_cairo_set_source_color (cr, &style->bg[GTK_STATE_NORMAL]);

  cairo_fill (cr);

  if (get_piece_id (target, x, y) != ' ') {
    gdk_cairo_rectangle (cr, &rect);
    gdk_cairo_set_source_pixbuf (cr, tiles_pixbuf,
                                 rect.x - get_piece_nr (target, x, y) * tile_size,
                                 rect.y - tile_size / 2);
    cairo_fill (cr);
  }

  if (get_piece_id (target, x, y) == '*') {
    if (get_piece_id (orig_map, x, y) == '.')
      value = 20;
    else
      value = 22;

    overlay_size = THEME_OVERLAY_SIZE * tile_size / THEME_TILE_SIZE;
    overlay_offset =
      THEME_TILE_CENTER * tile_size / THEME_TILE_SIZE - overlay_size / 2;

    cairo_rectangle (cr,
                     rect.x + overlay_offset, rect.y + overlay_offset,
                     overlay_size, overlay_size);

    gdk_cairo_set_source_pixbuf (cr, tiles_pixbuf,
                                 rect.x - value * tile_size,
                                 rect.y - tile_size / 2);
    cairo_fill (cr);
  }

  gdk_window_invalidate_rect (gtk_widget_get_window (space), &rect, TRUE);

  cairo_destroy (cr);
}

static gint
show_score_dialog (gint pos, gboolean endofgame)
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
      sorrydialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_INFO,
							GTK_BUTTONS_NONE,
							"<b>%s</b>\n%s",
							_
							("The Puzzle Has Been Solved!"),
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
      scoresdialog = games_scores_dialog_new (GTK_WINDOW (window), 
					highscores, _("Klotski Scores"));
      games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
						    (scoresdialog),
						    _("Puzzle:"));
    }

    if (pos > 0) {
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


void
score_cb (GtkAction * action)
{
  show_score_dialog (0, FALSE);
}

void
game_score (void)
{
  gint pos;
  gchar *key;
    
  /* Level is complete */
  key = get_level_key (current_level);
  games_conf_set_boolean (KEY_LEVEL_INFO_GROUP, key, TRUE);
  g_free (key);
  gtk_image_set_from_stock (GTK_IMAGE(level_image[current_level]), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);

  pos = games_scores_add_plain_score (highscores, (guint32) moves);
  if (show_score_dialog (pos, TRUE) == GTK_RESPONSE_REJECT)
    gtk_main_quit ();
  else
    new_game (current_level);
}

static gboolean
configure_pixmaps_idle (void)
{
  if (tile_size != prior_tile_size) {

    if (tiles_pixbuf != NULL)
      g_object_unref (tiles_pixbuf);
    tiles_pixbuf = NULL;

    if (tiles_preimage) {
      tiles_pixbuf = games_preimage_render (tiles_preimage,
					    tile_size * THEME_TILE_SEGMENTS,
					    tile_size * 2);
    }

    if (tiles_pixbuf == NULL) {
      GtkWidget *dialog;
      dialog = gtk_message_dialog_new (GTK_WINDOW (window),
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_ERROR,
				       GTK_BUTTONS_OK,
				       _
				       ("The theme for this game failed to render.\n\nPlease check that Klotski is installed correctly."));
      gtk_dialog_run (GTK_DIALOG (dialog));
      exit (1);
    }

    prior_tile_size = tile_size;
  }

  if (redraw_all_idle_id)
    g_source_remove (redraw_all_idle_id);

  redraw_all_idle_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE + 1,
					(GSourceFunc) redraw_all, NULL, NULL);

  configure_idle_id = 0;
  return FALSE;
}

static void
configure_pixmaps (void)
{
  tile_size = MIN (((space_width - SPACE_PADDING) / width),
		   ((space_height - SPACE_PADDING) / height));

  /* SVG theme renders best when tile size is multiple of 2 */
  if (tile_size < 1)
    return;

  if (tile_size % 2)
    tile_size--;

  if (clear_buffer || clear_game || (tile_size != prior_tile_size)) {
    if (configure_idle_id)
      g_source_remove (configure_idle_id);

    configure_idle_id =
      g_idle_add ((GSourceFunc) configure_pixmaps_idle, NULL);

    clear_buffer = TRUE;
  }

  return;
}

static gboolean
configure_space (GtkWidget * widget, GdkEventConfigure * event)
{
  space_width = event->width;
  space_height = event->height;
  configure_pixmaps ();

  return TRUE;
}

void
create_space (void)
{
  gameframe = games_grid_frame_new (9, 7);
  games_grid_frame_set_padding (GAMES_GRID_FRAME (gameframe),
				SPACE_PADDING, SPACE_PADDING);
  gtk_widget_set_size_request (GTK_WIDGET (gameframe), MINWIDTH, MINHEIGHT);

  space = gtk_drawing_area_new ();

  gtk_container_add (GTK_CONTAINER (gameframe), space);
  gtk_widget_set_events (space, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
			 | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
  /* We do our own double-buffering. */
  gtk_widget_set_double_buffered (space, FALSE);
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

}

/* Add puzzles to the game menu. */
static void
add_puzzle_menu (GtkUIManager * ui_manager)
{
  gint i;
  GSList *group = NULL;
  GtkRadioAction *top_action;
  GtkSizeGroup *groups[G_N_ELEMENTS (pack_uipath)];
    
  g_return_if_fail (GTK_IS_ACTION_GROUP (action_group));
    
  memset (groups, 0, sizeof(groups));
  
  for (i = max_level; i >= 0; i--) {
    GtkRadioAction *action;
    const gchar *label;
    GtkWidget *item, *box, *labelw, *image;

    label = gtk_action_group_translate_string (action_group, level[i].name);

    action = top_action = gtk_radio_action_new (level[i].name, "",
						NULL, NULL, i);

    gtk_radio_action_set_group (action, group);
    group = gtk_radio_action_get_group (action);

    gtk_action_group_add_action (action_group, GTK_ACTION (action));

    gtk_ui_manager_add_ui (ui_manager,
			   gtk_ui_manager_new_merge_id (ui_manager),
			   pack_uipath[level[i].group],
			   level[i].name, level[i].name,
			   GTK_UI_MANAGER_MENUITEM, TRUE);

    /* Unfortunately GtkUIManager only supports labels for items, so remove the label it creates and
     * replace it with our own widget */
    item = gtk_ui_manager_get_widget (ui_manager, g_strjoin("/", pack_uipath[level[i].group], level[i].name, NULL));

    /* Create a label and image for the menu item */
    box = gtk_hbox_new (FALSE, 6);
    labelw = gtk_label_new(label);
    gtk_misc_set_alignment (GTK_MISC (labelw), 0.0, 0.5);
    image = gtk_image_new ();
    gtk_box_pack_start (GTK_BOX (box), labelw, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (box), image, FALSE, TRUE, 0);
      
    /* Keep all elements the same size */
    if (groups[level[i].group] == NULL)
       groups[level[i].group] = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
    gtk_size_group_add_widget (GTK_SIZE_GROUP (groups[level[i].group]), box);

    /* Replace the label with the new one */
    gtk_container_remove (GTK_CONTAINER (item), gtk_bin_get_child (GTK_BIN (item)));
    gtk_container_add (GTK_CONTAINER (item), box);	  
    gtk_widget_show_all (box);
      
    level_image[i] = image;
    level_action[i] = GTK_TOGGLE_ACTION (action);
  }

  g_signal_connect_data (top_action, "changed",
			 G_CALLBACK (level_cb), window, NULL, 0);
}

GtkWidget *
create_menubar (void)
{
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, entries,
				G_N_ELEMENTS (entries), window);

  fullscreen_action = GTK_ACTION (games_fullscreen_action_new ("Fullscreen", GTK_WINDOW (window)));
  gtk_action_group_add_action_with_accel (action_group, fullscreen_action, NULL);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);
  add_puzzle_menu (ui_manager);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  return gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
}

void
create_statusbar (void)
{
  statusbar = gtk_hbox_new (TRUE, 0);

  messagewidget = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (statusbar), messagewidget, FALSE, FALSE, 0);


  moveswidget = gtk_label_new ("");
  gtk_box_pack_end (GTK_BOX (statusbar), moveswidget, FALSE, FALSE, 0);
}

gchar *
get_level_key (gint level_number)
{
    gchar *c;
    unsigned char octet;
    unsigned int result;
    gint i;

    /* Calculate the CRC of the level data */
    result = 0xFFFFFFFF;
    for (c = level[level_number].data; *c != '\0'; c++) {
	octet = *c;
	for (i = 0; i < 8; i++) {
	    if ((octet >> 7) ^ (result >> 31))
		result = (result << 1) ^ 0x04c11db7;
	    else
		result = (result << 1);
	    result &= 0xFFFFFFFF;
	    octet <<= 1;
	}
    }

    return g_strdup_printf ("%08X/solved", ~result);
}

void
load_solved_state (void)
{
    gint i;
    gchar *key;
    
    for (i = 0; i < max_level; i++) {
	key = get_level_key (i);
	if (games_conf_get_boolean (KEY_LEVEL_INFO_GROUP, key, NULL))
	    gtk_image_set_from_stock (GTK_IMAGE(level_image[i]), GTK_STOCK_YES, GTK_ICON_SIZE_MENU);
	g_free (key);
    }
}

void
load_image (void)
{
  const char *dname;
  char *path;
  GError *error = NULL;

  dname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  path = g_build_filename (dname, "gnotski.svg", NULL);
  tiles_preimage = games_preimage_new_from_file (path, &error);
  g_free (path);

  if (!tiles_preimage) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL,
				     GTK_DIALOG_MODAL,
				     GTK_MESSAGE_ERROR,
				     GTK_BUTTONS_OK,
				     _
				     ("Could not find the image:\n%s\n\nPlease check that Klotski is installed correctly."),
				     error->message);
    g_error_free (error);
    gtk_dialog_run (GTK_DIALOG (dialog));
    exit (1);
  }
}

static void
set_move (gint x)
{
  moves = x - 1;
  new_move ();
}

void
new_move (void)
{
  static gint last_piece_id = -2;
  gchar *str = NULL;

  if (moves < 1)
    last_piece_id = -2;

  if (last_piece_id != piece_id) {
    copymap (undomove_map, lastmove_map);
    if (moves < 999)
      moves++;
  }

  if ((moves > 0) && !mapcmp (undomove_map, map)) {
    moves--;
    last_piece_id = -2;
  } else {
    last_piece_id = piece_id;
  }

  copymap (lastmove_map, map);

  str = g_strdup_printf (_("Moves: %d"), moves);
  gtk_label_set_text (GTK_LABEL (moveswidget), str);
  g_free (str);
}

int
game_over (void)
{
  gint x, y, over = 1;
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      if (get_piece_id (map, x, y) == '*'
	  && get_piece_id (orig_map, x, y) != '.')
	over = 0;
  return over;
}

static gint
do_move_piece (gint id, gint dx, gint dy)
{
  gint x, y;
  copymap (tmpmap, map);

  /* Move pieces */
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      if (get_piece_id (tmpmap, x, y) == id)
	set_piece_id (tmpmap, x, y, ' ');

  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      if (get_piece_id (map, x, y) == id)
	set_piece_id (tmpmap, (x + dx), (y + dy), id);

  /* Preserve some from original map */
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++) {
      if (get_piece_id (tmpmap, x, y) == ' '
	  && get_piece_id (orig_map, x, y) == '.')
	set_piece_id (tmpmap, x, y, '.');
      if (get_piece_id (tmpmap, x, y) == ' '
	  && get_piece_id (orig_map, x, y) == '-')
	set_piece_id (tmpmap, x, y, '-');
    }
  /* Paint changes */
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      if (get_piece_id (map, x, y) != get_piece_id (tmpmap, x, y)
	  || get_piece_id (tmpmap, x, y) == id)
	gui_draw_pixmap (tmpmap, x, y);

  copymap (map, tmpmap);
  return 1;
}

static gint
check_valid_move (gint id, gint dx, gint dy)
{
  gint x, y, valid = 1;

  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      if (get_piece_id (map, x, y) == id)
	if (!(get_piece_id (map, x + dx, y + dy) == ' '
	      || get_piece_id (map, x + dx, y + dy) == '.'
	      || get_piece_id (map, x + dx, y + dy) == id
	      || (id == '*' && get_piece_id (map, x + dx, y + dy) == '-'))) {
	  valid = 0;
	}

  return valid;
}

gint
move_piece (gint id, gint x1, gint y1, gint x2, gint y2)
{
  gint return_value = 0;

  if (get_piece_id (map, x2, y2) == id)
    return_value = 1;

  if (!((abs (y1 - y2) == 0 && abs (x1 - x2) == 1)
	|| (abs (x1 - x2) == 0 && abs (y1 - y2) == 1)))
    return 0;

  if (abs (y1 - y2) == 1) {
    if (y1 - y2 < 0)
      if (check_valid_move (id, 0, 1))
	return do_move_piece (id, 0, 1);
    if (y1 - y2 > 0)
      if (check_valid_move (id, 0, -1))
	return do_move_piece (id, 0, -1);
  }

  if (abs (x1 - x2) == 1) {
    if (x1 - x2 < 0)
      if (check_valid_move (id, 1, 0))
	return do_move_piece (id, 1, 0);
    if (x1 - x2 > 0)
      if (check_valid_move (id, -1, 0))
	return do_move_piece (id, -1, 0);
  }

  return return_value;
}

gint
get_piece_id (char *src, gint x, gint y)
{
  return src[x + 1 + (y + 1) * (width + 2)];
}

void
set_piece_id (char *src, gint x, gint y, gint id)
{
  src[x + 1 + (y + 1) * (width + 2)] = id;
}

gint
get_piece_nr (char *src, gint x, gint y)
{
  char c;
  gint i = 0, nr = 0;
  x++;
  y++;
  c = src[x + y * (width + 2)];
  if (c == '-')
    return 23;
  if (c == ' ')
    return 21;
  if (c == '.')
    return 20;

  nr += 1 * (src[(x - 1) + (y - 1) * (width + 2)] == c);
  nr += 2 * (src[(x - 0) + (y - 1) * (width + 2)] == c);
  nr += 4 * (src[(x + 1) + (y - 1) * (width + 2)] == c);
  nr += 8 * (src[(x - 1) + (y - 0) * (width + 2)] == c);
  nr += 16 * (src[(x + 1) + (y - 0) * (width + 2)] == c);
  nr += 32 * (src[(x - 1) + (y + 1) * (width + 2)] == c);
  nr += 64 * (src[(x - 0) + (y + 1) * (width + 2)] == c);
  nr += 128 * (src[(x + 1) + (y + 1) * (width + 2)] == c);
  while (nr != image_map[i] && image_map[i] != -1)
    i += 2;
  return image_map[i + 1];
}

gint
mapcmp (char *m1, char *m2)
{
  gint x, y;
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      if (get_piece_id (m1, x, y) != get_piece_id (m2, x, y))
	return 1;
  return 0;
}

void
copymap (char *dest, char *src)
{
  memcpy (dest, src, (width + 2) * (height + 1));
}

static void
prepare_map (void)
{
  gint x, y = 0;
  gchar *leveldata;

  leveldata = level[current_level].data;
  width = level[current_level].width;
  height = level[current_level].height;
  gtk_label_set_text (GTK_LABEL (messagewidget),
		      _(level[current_level].name));

  if (map) {
    free (map);
    free (tmpmap);
    free (move_map);
    free (orig_map);
    free (lastmove_map);
    free (undomove_map);
  }

  piece_id = -1;
  button_down = piece_x = piece_y = 0;

  map = calloc (1, (width + 2) * (height + 2));
  tmpmap = calloc (1, (width + 2) * (height + 2));
  orig_map = calloc (1, (width + 2) * (height + 2));
  move_map = calloc (1, (width + 2) * (height + 2));
  lastmove_map = calloc (1, (width + 2) * (height + 2));
  undomove_map = calloc (1, (width + 2) * (height + 2));
  if (leveldata)
    for (y = 0; y < height; y++)
      for (x = 0; x < width; x++)
	set_piece_id (map, x, y, *leveldata++);
  copymap (orig_map, map);
  copymap (lastmove_map, map);
}

static void
update_menu_state (void)
{
  GtkAction *action;
  gboolean action_is_sensitive;

  /* Puzzle Radio Action */
  gtk_toggle_action_set_active (level_action[current_level], TRUE);

  /* Next Puzzle Sensitivity */
  action_is_sensitive = current_level < max_level;
  action = gtk_action_group_get_action (action_group, "NextPuzzle");
  gtk_action_set_sensitive (action, action_is_sensitive);

  /* Previous Puzzle Sensitivity */
  action_is_sensitive = current_level > 0;
  action = gtk_action_group_get_action (action_group, "PrevPuzzle");
  gtk_action_set_sensitive (action, action_is_sensitive);
}

void
new_game (gint requested_level)
{
  clear_game = TRUE;

  set_move (0);
  current_level = CLAMP (requested_level, 0, max_level);

  games_scores_set_category (highscores, scorecats[current_level].key);

  games_conf_set_integer (NULL, KEY_LEVEL, current_level);

  prepare_map ();
  games_grid_frame_set (GAMES_GRID_FRAME (gameframe), width, height);
  configure_pixmaps ();
  update_menu_state ();
}

void
quit_game_cb (GtkAction * action)
{
  gtk_main_quit ();
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

void
level_cb (GtkAction * action, GtkRadioAction * current)
{
  gint requested_level = gtk_radio_action_get_current_value (current);
  if (requested_level != current_level)
    new_game (requested_level);
}

void
restart_level_cb (GtkAction * action)
{
  new_game (current_level);
}

void
next_level_cb (GtkAction * action)
{
  new_game (current_level + 1);
}

void
prev_level_cb (GtkAction * action)
{
  new_game (current_level - 1);
}

void
help_cb (GtkAction * action)
{
  games_help_display (window, "gnotski", NULL);
}

void
about_cb (GtkAction * action)
{
  const gchar *authors[] = { "Lars Rydlinge", NULL };
  const gchar *documenters[] = { "Andrew Sobala", NULL };
  gchar *license = games_get_license (_(APPNAME_LONG));

  gtk_show_about_dialog (GTK_WINDOW (window),
                         "program-name", _(APPNAME_LONG),
			 "version", VERSION,
			 "comments", _("Sliding Block Puzzles\n\n"
			 "Klotski is a part of GNOME Games."),
			 "copyright",
			 "Copyright \xc2\xa9 1999-2008 Lars Rydlinge",
			 "license", license,
                         "wrap-license", TRUE,
                         "authors", authors,
			 "documenters", documenters,
                         "translator-credits", _("translator-credits"),
                         "logo-icon-name", "gnome-klotski",
                         "website", "http://www.gnome.org/projects/gnome-games",
                         "website-label", _("GNOME Games web site"),
                         NULL);
  g_free (license);
}
