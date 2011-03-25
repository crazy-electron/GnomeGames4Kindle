/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/*
 * Color lines for GNOME
 * Copyright © 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
 *          Szabolcs Ban <shooby@gnome.hu>
 *
 * Copyright © 2007 Christian Persch
 *
 * This game is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include <config.h>

#include <stdlib.h>    
#include <math.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>

#include <libgames-support/games-conf.h>
#include <libgames-support/games-files.h>
#include <libgames-support/games-frame.h>
#include <libgames-support/games-gridframe.h>
#include <libgames-support/games-gtk-compat.h>
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

#include "glines.h"

#define KEY_PREFERENCES_GROUP "preferences"
#define KEY_BACKGROUND_COLOR  "background_color"
#define KEY_BALL_THEME        "ball_theme"
#define KEY_MOVE_TIMEOUT      "move_timeout"
#define KEY_SIZE              "size"

#define KEY_SAVED_GROUP   "saved"
#define KEY_SAVED_SCORE   "score"
#define KEY_SAVED_FIELD   "field"
#define KEY_SAVED_PREVIEW "preview"

#define MAXNPIECES 10
#define MAXFIELDSIZE 30
#define DEFAULT_GAME_SIZE MEDIUM
#define DEFAULT_BALL_THEME "balls.svg"

enum {
  UNSET = 0,
  SMALL = 1,
  MEDIUM,
  LARGE,
  MAX_SIZE,
};

/* Keep these in sync with the enum above. */
static const gint field_sizes[MAX_SIZE][4] = {
  {-1, -1, -1, -1}, /* This is a dummy entry. */
  {7, 7, 5, 3},     /* SMALL */
  {9, 9, 7, 3},     /* MEDIUM */
  {20, 15, 7, 7}    /* LARGE */
};

static const GamesScoresCategory scorecats[] = {
  { "Small",  NC_("board size", "Small")  },
  { "Medium", NC_("board size", "Medium") },
  { "Large",  NC_("board size", "Large")  }
};

static GamesScores *highscores;

static gint hfieldsize;
static gint vfieldsize;
static gint ncolors;
static gint npieces;
static gint game_size = UNSET;
static gboolean pref_dialog_done = FALSE;

static GRand *rgen;

static GtkWidget *draw_area;
static GtkWidget *app, *statusbar, *pref_dialog, *gridframe;
static GtkWidget *preview_widgets[MAXNPIECES];
static GtkWidget *menubar;
static GtkWidget *scoreitem;
static GtkAction *fullscreen_action;

/* These keep track of what we put in the main table so we
 * can reshuffle them when we change the field size. */
static GtkWidget *table;
static GtkWidget *top_pane;
static GtkWidget *bottom_pane;

static field_props field[MAXFIELDSIZE * MAXFIELDSIZE];

/* Pre-rendering image data prepared from file. */
static GamesPreimage *ball_preimage = NULL;
/* The tile images with balls rendered on them. */
static cairo_surface_t *ball_surface = NULL;
/* The balls rendered to a size appropriate for the preview. */
static cairo_surface_t *preview_surfaces[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/* A cairo_surface_t of a blank tile. */
static cairo_surface_t *blank_surface = NULL;
static cairo_surface_t *blank_preview_surface = NULL;

static GtkWidget *fast_moves_toggle_button = NULL;

static GamesFileList *theme_file_list = NULL;

static int active = -1;
static int target = -1;
static int inmove = 0;
static guint score = 0;
static int cursor_x = MAXFIELDSIZE / 2;
static int cursor_y = MAXFIELDSIZE / 2;
static gboolean show_cursor = FALSE;

static int boxsize;

/* The width and height of the main window. */
#define DEFAULT_WIDTH  320
#define DEFAULT_HEIGHT 400

static int preview_height = 0;
static int preview_width = 0;

static int move_timeout = 100;
static int animate_id = 0;
static int preview[MAXNPIECES];
static char *ball_filename;
static GtkWidget *scorelabel;
static scoretable sctab[] =
  { {5, 10}, {6, 12}, {7, 18}, {8, 28}, {9, 42}, {10, 82}, {11, 108}, {12,
                                                                       138},
  {13, 172}, {14, 210}, {0, 0} };

static struct {
  GdkColor color;
  gchar *name;
  gint set;
} backgnd = { { 0, 0, 0, 0}, NULL, 0 };

static gchar *warning_message = NULL;

static void
set_statusbar_message (gchar * message)
{
  guint context_id;
  context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar),
                                             "message");
  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), context_id);
  gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_id, message);
}

static void
show_image_warning (gchar * message)
{
  GtkWidget *dialog;
  GtkWidget *button;

  dialog = gtk_message_dialog_new (GTK_WINDOW (app),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_CLOSE,
                                   "%s",_("Could not load theme"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%s", message);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("Preferences"), GTK_RESPONSE_ACCEPT);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  g_signal_connect (button, "clicked", G_CALLBACK (game_props_callback),
                    NULL);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static GamesPreimage *
load_image (gchar * fname)
{
  GamesPreimage *preimage;
  gchar *path;
  const char *dirname;
  GError *error = NULL;

  dirname = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  path = g_build_filename (dirname, fname, NULL);
  if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
    warning_message = g_strdup_printf (_("Unable to locate file:\n%s\n\n"
                                         "The default theme will be loaded instead."),
                                       fname);

    path = g_build_filename (dirname, "balls.svg", NULL);
    if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
      g_free (warning_message);
      warning_message = g_strdup_printf (_("Unable to locate file:\n%s\n\n"
                                           "Please check that Five or More is installed correctly."),
                                         fname);
    }
  }

  preimage = games_preimage_new_from_file (path, &error);
  g_free (path);

  if (error) {
    warning_message = g_strdup (error->message);
    g_error_free (error);
  }

  return preimage;
}

/* The main table has to be layed out differently depending on the
   size of the playing area we choose. Otherwise the preview window
   gets too large. */
static void
relay_table (void)
{
  gint height;
  GValue value = { 0 };

  height = 1 + vfieldsize;
  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, height);

  gtk_container_child_set_property (GTK_CONTAINER (table),
                                    bottom_pane, "bottom_attach", &value);

  gtk_table_resize (GTK_TABLE (table), vfieldsize + 1, 1);
}

static void
refresh_pixmaps (void)
{
  cairo_t *cr, *cr_blank;
  GdkPixbuf *ball_pixbuf = NULL;

  /* Since we get called both by configure and after loading an image.
   * it is possible the pixmaps aren't initialised. If they aren't
   * we don't do anything. */
  if (!ball_surface)
    return;

  if (ball_preimage) {
    ball_pixbuf = games_preimage_render (ball_preimage, 4 * boxsize,
                                         7 * boxsize);

    /* Handle rendering problems. */
    if (!ball_pixbuf) {
      g_object_unref (ball_preimage);
      ball_preimage = NULL;

      if (!warning_message) {
        warning_message = g_strdup ("The selected theme failed to render.\n\n"
                                    "Please check that Five or More is installed correctly.");
      }
    }
  }

  if (!ball_pixbuf) {
    ball_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                  4 * boxsize, 7 * boxsize);
    gdk_pixbuf_fill (ball_pixbuf, 0x00000000);
  }

  if (warning_message)
    show_image_warning (warning_message);
  g_free (warning_message);
  warning_message = NULL;

  cr = cairo_create (ball_surface);
  gdk_cairo_set_source_color (cr, &backgnd.color);

  cairo_rectangle (cr, 0, 0, boxsize * 4, boxsize * 7);
  cairo_fill (cr);

  gdk_cairo_set_source_pixbuf (cr, ball_pixbuf, 0, 0);
  cairo_mask (cr, cairo_get_source (cr));

  cr_blank = cairo_create (blank_surface);
  gdk_cairo_set_source_color (cr_blank, &backgnd.color);

  cairo_rectangle (cr_blank, 0, 0, boxsize, boxsize);
  cairo_fill (cr_blank);

  g_object_unref (ball_pixbuf);
  cairo_destroy (cr);
  cairo_destroy (cr_blank);
}

static void
refresh_preview_surfaces (void)
{
  guint i;
  GdkPixbuf *scaled = NULL;
  GtkWidget *widget = preview_widgets[0];
  cairo_t *cr;
  GdkRectangle preview_rect;

  /* Like the refresh_pixmaps() function, we may be called before
   * the window is ready. */
  if (preview_height == 0)
    return;

  preview_rect.x = 0;
  preview_rect.y = 0;
  preview_rect.width = preview_width;
  preview_rect.height = preview_height;

  /* We create pixmaps for each of the ball colours and then
   * set them as the background for each widget in the preview array.
   * This code assumes that each preview window is identical. */

  for (i = 0; i < 7; i++)
    if (preview_surfaces[i])
      cairo_surface_destroy (preview_surfaces[i]);

  if (ball_preimage)
    scaled = games_preimage_render (ball_preimage, 4 * preview_width,
                                    7 * preview_height);

  if (!scaled) {
    scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                             4 * preview_width, 7 * preview_height);
    gdk_pixbuf_fill (scaled, 0x00000000);
  }

  for (i = 0; i < 7; i++) {
    preview_surfaces[i] = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                                             CAIRO_CONTENT_COLOR_ALPHA,
                                                             preview_width, preview_height);
    cr = cairo_create (preview_surfaces[i]);
    gdk_cairo_set_source_color (cr, &gtk_widget_get_style (widget)->bg[GTK_STATE_NORMAL]);
    gdk_cairo_rectangle (cr, &preview_rect);
    cairo_fill (cr);

    gdk_cairo_set_source_pixbuf (cr, scaled, 0, -1.0 * preview_height * i);
    cairo_mask (cr, cairo_get_source (cr));

    cairo_destroy (cr);
  }

  if (blank_preview_surface)
    cairo_surface_destroy (blank_preview_surface);

  blank_preview_surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                                             CAIRO_CONTENT_COLOR_ALPHA,
                                                             preview_width, preview_height);
  cr = cairo_create (blank_preview_surface);
  gdk_cairo_set_source_color (cr, &gtk_widget_get_style (widget)->bg[GTK_STATE_NORMAL]);
  gdk_cairo_rectangle (cr, &preview_rect);
  cairo_fill (cr);

  cairo_destroy (cr);
  g_object_unref (scaled);
}

static void
draw_all_balls (GtkWidget * widget)
{
  gdk_window_invalidate_rect (gtk_widget_get_window (widget), NULL, FALSE);
}

static void
start_animation (void)
{
  int ms;

  ms = (inmove ? move_timeout : 100);
  if (animate_id == 0)
    animate_id = g_timeout_add (ms, animate, draw_area);
}

void
set_inmove (int i)
{
  if (inmove != i) {
    inmove = i;
    if (animate_id)
      g_source_remove (animate_id);
    animate_id = 0;
    start_animation ();
  }
}

static void
reset_game (void)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    field[i].color = 0;
    field[i].phase = 0;
    field[i].active = 0;
    field[i].pathsearch = -1;
  }
  score = 0;
  init_preview ();
  init_new_balls (npieces, -1);
}

static void
refresh_screen (void)
{
  draw_all_balls (draw_area);
  draw_preview ();
}

static void
start_game (void)
{
  char string[20];

  set_statusbar_message (_
                         ("Match five objects of the same type in a row to score!"));
  refresh_screen ();
  active = -1;
  target = -1;
  inmove = -1;
  g_snprintf (string, 19, "%d", score);
  gtk_label_set_text (GTK_LABEL (scorelabel), string);
  set_inmove (0);
}

static void
reset_pathsearch (void)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++)
    field[i].pathsearch = -1;
}

void
init_preview (void)
{
  int i;

  for (i = 0; i < npieces; i++) {
    preview[i] = g_rand_int_range (rgen, 1, ncolors + 1);
  }
}

void
draw_preview (void)
{
  guint i;
  cairo_pattern_t *pattern;

  for (i = 0; i < MAXNPIECES; i++) {

    if (i < npieces)
      pattern = cairo_pattern_create_for_surface (preview_surfaces[preview[i] - 1]);
    else
      pattern = cairo_pattern_create_for_surface (blank_preview_surface);

    gdk_window_set_background_pattern (gtk_widget_get_window (preview_widgets[i]),
                                       pattern);
    cairo_pattern_destroy (pattern);
    gtk_widget_queue_draw (preview_widgets[i]);
  }
}

static void
game_new_callback (void)
{
  reset_game ();
  start_game ();
}

static void
show_scores (gint pos, gboolean new_game)
{
  static GtkWidget *dialog;

  if (dialog == NULL) {
    dialog = games_scores_dialog_new (GTK_WINDOW (app), highscores, _("GNOME Five or More"));
    games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
                                                  (dialog), _("_Board size:"));
  }

  if (pos > 0) {
    games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (dialog), pos);
  }

  gtk_window_present (GTK_WINDOW (dialog));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);
}

static void
game_over (void)
{
  int pos;

  set_statusbar_message (_("Game Over!"));
  pos = games_scores_add_plain_score (highscores, score);
  show_scores (pos, TRUE);
  return;
}

static int
spaces_left (void)
{
  int i, j;

  j = 0;

  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    if (field[i].color == 0)
      j++;
  }

  return j;
}

static int
check_gameover (void)
{
  if (spaces_left () > 0)
    return 1;

  game_over ();

  return -1;
}

int
init_new_balls (int num, int prev)
{
  int i, j = -1;
  gfloat num_boxes = hfieldsize * vfieldsize;
  for (i = 0; i < num;) {
    j = g_rand_int_range (rgen, 0, num_boxes);
    if (field[j].color == 0) {
      field[j].color = (prev == -1) ?
        g_rand_int_range (rgen, 1, ncolors + 1) : preview[prev];
      i++;
    }
  }
  return j;
}

void
draw_box (GtkWidget * widget, int x, int y)
{
  gtk_widget_queue_draw_area (widget, x * boxsize, y * boxsize,
                              boxsize, boxsize);
}

static int
route (int num)
{
  int i;
  int flag = 0;

  if (field[target].pathsearch == num)
    return 1;
  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    if (field[i].pathsearch == num) {
      flag = 1;
      if ((i / hfieldsize > 0) && (field[i - hfieldsize].pathsearch == -1)
          && (field[i - hfieldsize].color == 0))
        field[i - hfieldsize].pathsearch = num + 1;
      if ((i / hfieldsize < vfieldsize - 1)
          && (field[i + hfieldsize].pathsearch == -1)
          && (field[i + hfieldsize].color == 0))
        field[i + hfieldsize].pathsearch = num + 1;
      if ((i % hfieldsize > 0) && (field[i - 1].pathsearch == -1)
          && (field[i - 1].color == 0))
        field[i - 1].pathsearch = num + 1;
      if ((i % hfieldsize < hfieldsize - 1) && (field[i + 1].pathsearch == -1)
          && (field[i + 1].color == 0)) {
        field[i + 1].pathsearch = num + 1;
      }
    }
  }
  if (flag == 0)
    return 0;
  return 2;
}

static void
fix_route (int num, int pos)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++)
    if ((i != pos) && (field[i].pathsearch == num))
      field[i].pathsearch = -1;
  if (num < 2)
    return;
  if ((pos / hfieldsize > 0)
      && (field[pos - hfieldsize].pathsearch == num - 1))
    fix_route (num - 1, pos - hfieldsize);
  if ((pos % hfieldsize > 0) && (field[pos - 1].pathsearch == num - 1))
    fix_route (num - 1, pos - 1);
  if ((pos / hfieldsize < vfieldsize - 1)
      && (field[pos + hfieldsize].pathsearch == num - 1))
    fix_route (num - 1, pos + hfieldsize);
  if ((pos % hfieldsize < hfieldsize - 1)
      && (field[pos + 1].pathsearch == num - 1))
    fix_route (num - 1, pos + 1);
}

int
find_route (void)
{
  int i = 0;
  int j = 0;

  field[active].pathsearch = i;
  while ((j = route (i))) {
    if (j == 1) {
      fix_route (i, target);
      return 1;
    }
    i++;
  }
  return 0;
}

static void
deactivate (GtkWidget * widget, int x, int y)
{
  field[active].active = 0;
  field[active].phase = 0;
  draw_box (widget, x, y);
  active = -1;
}

static void
cell_clicked (GtkWidget * widget, int fx, int fy)
{
  int x, y;

  set_statusbar_message ("");
  if (field[fx + fy * hfieldsize].color == 0) {
    /* Clicked on an empty field */

    if ((active != -1) && (inmove != 1)) {
      /* We have an active ball, and it's not
       * already in move. Therefore we should begin
       * the moving sequence */

      target = fx + fy * hfieldsize;
      if (find_route ()) {
        /* We found a route to the new position */

        set_inmove (1);
      } else {
        /* Can't move there! */
        set_statusbar_message (_("You can't move there!"));
        reset_pathsearch ();
        target = -1;
      }
    }
  } else {
    /* Clicked on a ball */

    if ((fx + fy * hfieldsize) == active) {
      /* It's active, so let's deactivate it! */

      deactivate (widget, fx, fy);
    } else {
      /* It's not active, so let's activate it! */

      if (active != -1) {
        /* There is an other active ball, we should deactivate it first! */

        x = active % hfieldsize;
        y = active / hfieldsize;
        deactivate (widget, x, y);
      }

      active = fx + fy * hfieldsize;
      field[active].active = 1;
      start_animation();
    }
  }

}

static gint
button_press_event (GtkWidget * widget, GdkEvent * event)
{
  int x, y;
  int fx, fy;

  if (inmove)
    return TRUE;

  /* Ignore the 2BUTTON and 3BUTTON events. */
  if (event->type != GDK_BUTTON_PRESS)
    return TRUE;

  if (show_cursor) {
    show_cursor = FALSE;
    draw_box (draw_area, cursor_x, cursor_y);
  }

  /* FIXMEchpe: why not use event->[xy] here? */
  gtk_widget_get_pointer (widget, &x, &y);
  fx = x / boxsize;
  fy = y / boxsize;

  /* If we click on the outer border pixels, the previous calculation
   * will be wrong and we must correct. */
  fx = CLAMP (fx, 0, hfieldsize - 1);
  fy = CLAMP (fy, 0, vfieldsize - 1);

  cursor_x = fx;
  cursor_y = fy;

  cell_clicked (widget, fx, fy);

  return TRUE;
}

static void
move_cursor (int dx, int dy)
{
  int old_x = cursor_x;
  int old_y = cursor_y;

  if (!show_cursor) {
    show_cursor = TRUE;
    draw_box (draw_area, cursor_x, cursor_y);
  }

  cursor_x = cursor_x + dx;
  if (cursor_x < 0)
    cursor_x = 0;
  if (cursor_x >= hfieldsize)
    cursor_x = hfieldsize - 1;
  cursor_y = cursor_y + dy;
  if (cursor_y < 0)
    cursor_y = 0;
  if (cursor_y >= vfieldsize)
    cursor_y = vfieldsize - 1;

  if (cursor_x == old_x && cursor_y == old_y)
    return;

  draw_box (draw_area, old_x, old_y);
  draw_box (draw_area, cursor_x, cursor_y);
}

static gboolean
key_press_event (GtkWidget * widget, GdkEventKey * event, void *d)
{
  guint key;

  key = event->keyval;

  switch (key) {
  case GDK_KEY_Left:
  case GDK_KEY_KP_Left:
    move_cursor (-1, 0);
    break;
  case GDK_KEY_Right:
  case GDK_KEY_KP_Right:
    move_cursor (1, 0);
    break;
  case GDK_KEY_Up:
  case GDK_KEY_KP_Up:
    move_cursor (0, -1);
    break;
  case GDK_KEY_Down:
  case GDK_KEY_KP_Down:
    move_cursor (0, 1);
    break;
  case GDK_KEY_Home:
  case GDK_KEY_KP_Home:
    move_cursor (-999, 0);
    break;
  case GDK_KEY_End:
  case GDK_KEY_KP_End:
    move_cursor (999, 0);
    break;
  case GDK_KEY_Page_Up:
  case GDK_KEY_KP_Page_Up:
    move_cursor (0, -999);
    break;
  case GDK_KEY_Page_Down:
  case GDK_KEY_KP_Page_Down:
    move_cursor (0, 999);
    break;

  case GDK_KEY_space:
  case GDK_KEY_Return:
  case GDK_KEY_KP_Enter:
    if (show_cursor)
      cell_clicked (widget, cursor_x, cursor_y);
    break;
  }

  return TRUE;
}

static void
draw_grid (cairo_t *cr)
{
  GdkColor color;
  guint w, h;
  guint i;

  w = gtk_widget_get_allocated_width(draw_area);
  h = gtk_widget_get_allocated_height(draw_area);

  gdk_color_parse ("#525F6C", &color);
  gdk_cairo_set_source_color (cr, &color);
  cairo_set_line_width (cr, 1.0);

  for (i = boxsize; i < w; i = i + boxsize)
  {
    cairo_move_to (cr, i + 0.5, 0 + 0.5);
    cairo_line_to (cr, i + 0.5, h + 0.5);
  }

  for (i = boxsize; i < h; i = i + boxsize)
  {
    cairo_move_to (cr, 0 + 0.5, i + 0.5);
    cairo_line_to (cr, w + 0.5, i + 0.5);
  }

  cairo_rectangle (cr, 0.5, 0.5, w - 0.5, h - 0.5);
  cairo_stroke (cr);
}

static gboolean
field_draw_callback (GtkWidget * widget, cairo_t *cr)
{
  guint i, j, idx;
  GdkColor cursorColor;

  for (i = 0; i < vfieldsize; i++) {
    for (j = 0; j < hfieldsize; j++) {
      int phase, color;

      idx = j + i * hfieldsize;

      cairo_rectangle (cr, j * boxsize, i * boxsize, boxsize, boxsize);

      if (field[idx].color != 0) {
        phase = field[idx].phase;
        color = field[idx].color - 1;

        phase = ABS (ABS (3 - phase) - 3);

        cairo_set_source_surface (cr, ball_surface,
                                 (1.0 * j - phase) * boxsize,
                                 (1.0 * i - color) * boxsize);
      } else {
        cairo_set_source_surface (cr, blank_surface, 1.0 * j * boxsize, 1.0 * i * boxsize);
      }

      cairo_fill (cr);
    }
  }

  /* Cursor */
  if (((backgnd.color.red + backgnd.color.green + backgnd.color.blue) / 3) >
      (G_MAXUINT16 / 2))
    gdk_color_parse ("#000000", &cursorColor);
  else
    gdk_color_parse ("#FFFFFF", &cursorColor);

  gdk_cairo_set_source_color (cr, &cursorColor);
  cairo_set_line_width (cr, 1.0);
  cairo_rectangle (cr,
                   cursor_x * boxsize + 1.5, cursor_y * boxsize + 1.5,
                   boxsize - 2.5, boxsize - 2.5);
  cairo_stroke (cr);

  draw_grid (cr);

  return FALSE;
}

static int
addscore (int num)
{
  gchar string[20];
  int i = 0;
  int retval;

  while ((sctab[i].num != num) && (sctab[i].num != 0))
    i++;
  if (sctab[i].num == 0) {
    num-=5;
    retval = sctab[num%5].score + 72 * (num/5) + (num-5)*24 ;
  } else
    retval = sctab[i].score;

  score += retval;
  g_snprintf (string, 19, "%d", score);
  gtk_label_set_text (GTK_LABEL (scorelabel), string);

  return retval;
}


static void
tag_list (int *list, int len)
{
  int i;

  for (i = 0; i < len; i++)
    field[list[i]].tag = 1;
}

static int
find_lines (int num)
{
  int count = 1;
  int subcount = 0;
  int x = num % hfieldsize;
  int y = num / hfieldsize;
  int list[hfieldsize];

  /* Horizontal */

  x++;
  while ((x <= hfieldsize - 1)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    x++;
  }
  x = num % hfieldsize - 1;
  while ((x >= 0) && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    x--;
  }
  if (subcount >= 4) {
    field[num].tag = 1;
    tag_list (list, subcount);
    count += subcount;
  }
  subcount = 0;

  /* Vertical */

  x = num % hfieldsize;
  y++;
  while ((y <= vfieldsize - 1)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y++;
  }
  y = num / hfieldsize - 1;
  while ((y >= 0) && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y--;
  }
  if (subcount >= 4) {
    field[num].tag = 1;
    tag_list (list, subcount);
    count += subcount;
  }
  subcount = 0;

  /* Diagonal ++ */

  x = num % hfieldsize + 1;
  y = num / hfieldsize + 1;
  while ((y <= vfieldsize - 1) && (x <= hfieldsize - 1)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y++;
    x++;
  }
  x = num % hfieldsize - 1;
  y = num / hfieldsize - 1;
  while ((y >= 0) && (x >= 0)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y--;
    x--;
  }
  if (subcount >= 4) {
    field[num].tag = 1;
    tag_list (list, subcount);
    count += subcount;
  }
  subcount = 0;

  /* Diagonal +- */

  x = num % hfieldsize + 1;
  y = num / hfieldsize - 1;
  while ((y >= 0) && (x <= hfieldsize - 1)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y--;
    x++;
  }
  x = num % hfieldsize - 1;
  y = num / hfieldsize + 1;
  while ((y <= vfieldsize - 1) && (x >= 0)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y++;
    x--;
  }
  if (subcount >= 4) {
    field[num].tag = 1;
    tag_list (list, subcount);
    count += subcount;
  }

  return count;
}

static void
clear_tags (void)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++)
    field[i].tag = 0;
}

static int
kill_tags (GtkWidget * widget)
{
  int i, j;

  j = 0;
  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    if (field[i].tag == 1) {
      j++;
      field[i].color = 0;
      field[i].phase = 0;
      field[i].active = 0;
      draw_box (widget, i % hfieldsize, i / hfieldsize);
    }
  }

  return j;
}

static gboolean
check_goal (GtkWidget * w, int target)
{
  gboolean lines_cleared;
  int count;

  clear_tags ();

  count = find_lines (target);
  lines_cleared = count >= 5;

  if (lines_cleared) {
    kill_tags (w);
    addscore (count);
  }
  reset_pathsearch ();

  return lines_cleared;
}

gint
animate (gpointer gp)
{
  GtkWidget *widget = GTK_WIDGET (gp);
  int x, y;
  int newactive = 0;
    
  x = active % hfieldsize;
  y = active / hfieldsize;

  if (active == -1) {
    animate_id = 0;
    return FALSE;
  }

  if (inmove != 0) {
    if ((x > 0)
        && (field[active - 1].pathsearch == field[active].pathsearch + 1))
      newactive = active - 1;
    else if ((x < hfieldsize - 1)
             && (field[active + 1].pathsearch ==
                 field[active].pathsearch + 1))
      newactive = active + 1;
    else if ((y > 0)
             && (field[active - hfieldsize].pathsearch ==
                 field[active].pathsearch + 1))
      newactive = active - hfieldsize;
    else if ((y < vfieldsize - 1)
             && (field[active + hfieldsize].pathsearch ==
                 field[active].pathsearch + 1))
      newactive = active + hfieldsize;
    else {
      set_inmove (0);
    }
    draw_box (widget, x, y);
    x = newactive % hfieldsize;
    y = newactive / hfieldsize;
    field[newactive].phase = field[active].phase;
    field[newactive].color = field[active].color;
    field[active].phase = 0;
    field[active].color = 0;
    field[active].active = 0;
    if (newactive == target) {
      target = -1;
      set_inmove (0);
      active = -1;
      field[newactive].phase = 0;
      field[newactive].active = 0;
      draw_box (widget, x, y);
      reset_pathsearch ();
      if (!check_goal (widget, newactive)) {
        gboolean fullline;
        int balls[npieces];
        int spaces;

        clear_tags ();
        fullline = FALSE;
        spaces = spaces_left ();
        if (spaces > npieces)
          spaces = npieces;
        for (x = 0; x < spaces; x++) {
          int tmp = init_new_balls (1, x);
          draw_box (widget, tmp % hfieldsize, tmp / hfieldsize);
          balls[x] = tmp;
          fullline = (find_lines (balls[x]) >= 5) || fullline;
        }
        if (fullline)
          addscore (kill_tags (widget));
        if (check_gameover () == -1)
          return FALSE;
        init_preview ();
        draw_preview ();
      }
      return TRUE;
    }
    field[newactive].active = 1;
    active = newactive;
  }

  field[active].phase++;
  if (field[active].phase >= 4)
    field[active].phase = 0;
  draw_box (widget, x, y);

  return TRUE;
}

static void
game_top_ten_callback (GtkAction * action, gpointer data)
{
  show_scores (0, FALSE);
}

static void
game_about_callback (GtkAction * action, gpointer * data)
{
  const gchar *authors[] = { "Robert Szokovacs <szo@appaloosacorp.hu>",
    "Szabolcs B\xc3\xa1n <shooby@gnome.hu>",
    NULL
  };

  const gchar *documenters[] = { "Emese Kovács",
    NULL
  };
  gchar *license = games_get_license (_("Five or More"));

  gtk_show_about_dialog (GTK_WINDOW (app),
                         "program-name", _("Five or More"),
                         "version", VERSION,
                         "comments", _("GNOME port of the once-popular Color Lines game.\n\nFive or More is a part of GNOME Games."),
                         "copyright",
                         "Copyright \xc2\xa9 1997-2008 Free Software Foundation, Inc.",
                         "license", license,
                         "authors", authors,
                         "documenters", documenters,
                         "translator-credits", _("translator-credits"),
                         "logo-icon-name", "gnome-glines",
                         "website", "http://www.gnome.org/projects/gnome-games/",
                         "website-label", _("GNOME Games web site"),
                         "wrap-license", TRUE, NULL);
  g_free (license);
}

static void
set_backgnd_color (const gchar * str)
{
  if (!str)
    str = g_strdup ("#000000");

  if (str != backgnd.name) {
    g_free (backgnd.name);
    backgnd.name = g_strdup (str);
  }

  if (!gdk_color_parse (backgnd.name, &backgnd.color)) {
    gdk_color_parse ("#000000", &backgnd.color);
  }
}

static void
set_sizes (gint size)
{
  hfieldsize = field_sizes[size][0];
  vfieldsize = field_sizes[size][1];
  ncolors = field_sizes[size][2];
  npieces = field_sizes[size][3];
  game_size = size;
  games_scores_set_category (highscores, scorecats[size - 1].key);

  games_conf_set_integer (KEY_PREFERENCES_GROUP, KEY_SIZE, size);

  if (gridframe)
    games_grid_frame_set (GAMES_GRID_FRAME (gridframe),
                          hfieldsize, vfieldsize);
}

static void
load_theme (void)
{
  if (ball_preimage)
    g_object_unref (ball_preimage);
  ball_preimage = load_image (ball_filename);

  refresh_pixmaps ();
  refresh_preview_surfaces ();
}

static void
conf_value_changed_cb (GamesConf *conf,
                       const char *group,
                       const char *key,
                       gpointer user_data)
{
  if (!group || strcmp (group, KEY_PREFERENCES_GROUP) != 0)
    return;

  if (strcmp (key, KEY_BACKGROUND_COLOR) == 0) {
    gchar *color;

    color = games_conf_get_string (KEY_PREFERENCES_GROUP, KEY_BACKGROUND_COLOR, NULL);
    if (color != NULL) {
      set_backgnd_color (color);
      g_free (color);
    }
  } else if (strcmp (key, KEY_BALL_THEME) == 0) {
    gchar *theme_tmp = NULL;

    theme_tmp = games_conf_get_string (KEY_PREFERENCES_GROUP, KEY_BALL_THEME, NULL);
    if (theme_tmp) {
      if (strcmp (theme_tmp, ball_filename) != 0) {
        g_free (ball_filename);
        ball_filename = theme_tmp;
        load_theme ();
        refresh_screen ();
      } else
        g_free (theme_tmp);
    }
    /* FIXME apply in the prefs dialog GUI */
  } else if (strcmp (key, KEY_MOVE_TIMEOUT) == 0) {
    gint timeout_tmp;

    timeout_tmp = games_conf_get_integer (KEY_PREFERENCES_GROUP, KEY_MOVE_TIMEOUT, NULL);
    timeout_tmp = CLAMP (timeout_tmp, 1, 1000);
    if (timeout_tmp != move_timeout)
      move_timeout = timeout_tmp;

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fast_moves_toggle_button),
                                  (move_timeout == 10));

  } else if (strcmp (key, KEY_SIZE) == 0) {
    gint size_tmp;
    size_tmp = games_conf_get_integer (KEY_PREFERENCES_GROUP, KEY_SIZE, NULL);

    if (size_tmp != game_size) {
      set_sizes (size_tmp);
      relay_table ();
      reset_game ();
      start_game ();
    }
  }
}

static void
bg_color_callback (GtkWidget * widget, gpointer data)
{
  GdkColor c;
  char str[64];

  gtk_color_button_get_color (GTK_COLOR_BUTTON (widget), &c);

  g_snprintf (str, sizeof (str), "#%04x%04x%04x", c.red, c.green, c.blue);

  games_conf_set_string (KEY_PREFERENCES_GROUP, KEY_BACKGROUND_COLOR, str);
}

static void
size_callback (GtkWidget * widget, gpointer data)
{
  if (pref_dialog_done)
    games_conf_set_integer (KEY_PREFERENCES_GROUP, KEY_SIZE, GPOINTER_TO_INT (data));
}

static void
set_selection (GtkWidget * widget, char *data)
{
  const gchar *entry;

  entry = games_file_list_get_nth (theme_file_list,
                                   gtk_combo_box_get_active (GTK_COMBO_BOX
                                                             (widget)));
  games_conf_set_string (KEY_PREFERENCES_GROUP, KEY_BALL_THEME, entry);
}

static GtkWidget *
fill_menu (void)
{
  const char *pixmap_dir;

  if (theme_file_list)
    g_object_unref (theme_file_list);

  pixmap_dir = games_runtime_get_directory (GAMES_RUNTIME_GAME_PIXMAP_DIRECTORY);
  theme_file_list = games_file_list_new_images (pixmap_dir, NULL);
  games_file_list_transform_basename (theme_file_list);

  return games_file_list_create_widget (theme_file_list, ball_filename,
                                        GAMES_FILE_LIST_REMOVE_EXTENSION |
                                        GAMES_FILE_LIST_REPLACE_UNDERSCORES);
}

static void
set_fast_moves_callback (GtkWidget * widget, gpointer * data)
{
  gboolean is_on = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  gint timeout = is_on ? 10 : 100;
  games_conf_set_integer (KEY_PREFERENCES_GROUP, KEY_MOVE_TIMEOUT, timeout);
}

static void
pref_dialog_response (GtkDialog * dialog, gint response, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (dialog));
}

void
game_props_callback (void)
{
  GtkWidget *w, *omenu, *l, *fv;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *vbox;
  GtkWidget *button;
  int i;
  GSList *group;

  if (!pref_dialog) {
    pref_dialog = gtk_dialog_new_with_buttons (_("Five or More Preferences"),
                                               GTK_WINDOW (app),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_STOCK_CLOSE,
                                               GTK_RESPONSE_CLOSE, NULL);
    g_signal_connect (pref_dialog, "response",
                      G_CALLBACK (pref_dialog_response), NULL);
    g_signal_connect (pref_dialog, "delete-event",
                      G_CALLBACK (gtk_widget_hide), NULL);

    gtk_window_set_resizable (GTK_WINDOW (pref_dialog), FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (pref_dialog), 5);
    gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))), 2);

    vbox = gtk_vbox_new (FALSE, 18);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))),
                        vbox, FALSE, FALSE, 0);

    frame = games_frame_new (_("Appearance"));
    table = gtk_table_new (2, 2, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (table), 0);
    gtk_table_set_row_spacings (GTK_TABLE (table), 6);
    gtk_table_set_col_spacings (GTK_TABLE (table), 12);
    gtk_container_add (GTK_CONTAINER (frame), table);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

    l = gtk_label_new_with_mnemonic (_("_Image:"));
    gtk_misc_set_alignment (GTK_MISC (l), 0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), l, 0, 1, 0, 1);

    omenu = fill_menu ();
    g_signal_connect (omenu, "changed",
                      G_CALLBACK (set_selection), NULL);
    gtk_table_attach_defaults (GTK_TABLE (table), omenu, 1, 2, 0, 1);
    gtk_label_set_mnemonic_widget (GTK_LABEL (l), omenu);


    l = gtk_label_new_with_mnemonic (_("B_ackground color:"));
    gtk_misc_set_alignment (GTK_MISC (l), 0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), l, 0, 1, 1, 2);

    {
      w = gtk_color_button_new ();
      gtk_color_button_set_color (GTK_COLOR_BUTTON (w), &backgnd.color);
      g_signal_connect (w, "color-set",
                        G_CALLBACK (bg_color_callback), NULL);
    }

    gtk_table_attach_defaults (GTK_TABLE (table), w, 1, 2, 1, 2);
    gtk_label_set_mnemonic_widget (GTK_LABEL (l), w);


    frame = games_frame_new (_("Board Size"));
    fv = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (frame), fv);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

    group = NULL;
    for (i = 0; i < G_N_ELEMENTS (scorecats); ++i) {
      button = gtk_radio_button_new_with_mnemonic (group, g_dpgettext2 (NULL, "board size", scorecats[i].name));
      if (game_size == i)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (size_callback), GINT_TO_POINTER (i + 1));

      gtk_box_pack_start (GTK_BOX (fv), button, FALSE, FALSE, 0);

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
    }

    frame = games_frame_new (C_("preferences", "General"));
    fv = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (frame), fv);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

    fast_moves_toggle_button =
      gtk_check_button_new_with_mnemonic (_("_Use fast moves"));
    if (move_timeout == 10) {
      gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON (fast_moves_toggle_button), TRUE);
    }
    g_signal_connect (fast_moves_toggle_button, "clicked",
                      G_CALLBACK (set_fast_moves_callback), NULL);

    gtk_container_add (GTK_CONTAINER (fv), fast_moves_toggle_button);

    gtk_widget_show_all (pref_dialog);

    pref_dialog_done = TRUE;
  }

  gtk_window_present (GTK_WINDOW (pref_dialog));
}

static int
game_quit_callback (GtkAction * action, gpointer data)
{
  if (animate_id)
    g_source_remove (animate_id);
  gtk_main_quit ();
  return FALSE;
}

static void
game_help_callback (GtkAction * action, gpointer data)
{
  games_help_display (app, "glines", NULL);
}

static int
preview_configure_cb (GtkWidget * widget, GdkEventConfigure * event)
{
  preview_width = event->width;
  preview_height = event->height;

  refresh_preview_surfaces ();

  draw_preview ();

  return TRUE;
}

static int
configure_event_callback (GtkWidget * widget, GdkEventConfigure * event)
{
  if (ball_surface)
    cairo_surface_destroy(ball_surface);

  if (blank_surface)
    cairo_surface_destroy (blank_surface);

  boxsize = (event->width - 1) / hfieldsize;
  ball_surface = gdk_window_create_similar_surface (gtk_widget_get_window (draw_area),
                                                    CAIRO_CONTENT_COLOR_ALPHA,
                                                    boxsize * 4, boxsize * 7);
  blank_surface = gdk_window_create_similar_surface (gtk_widget_get_window (draw_area),
                                                     CAIRO_CONTENT_COLOR_ALPHA,
                                                     boxsize, boxsize);
  refresh_pixmaps ();

  refresh_screen ();

  return TRUE;
}

#ifdef WITH_SMCLIENT
static int
save_state_cb (EggSMClient *client,
            GKeyFile* keyfile,
            gpointer client_data)
{
  gchar *buf;
  int argc = 0;
  char *argv[1];
  int i;

  games_conf_set_integer (KEY_SAVED_GROUP, KEY_SAVED_SCORE, score);

  buf = g_malloc (hfieldsize * vfieldsize * 4 + 1);
  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    buf[i * 4] = field[i].color + 'h';
    buf[i * 4 + 1] = field[i].pathsearch + 'h';
    buf[i * 4 + 2] = field[i].phase + 'h';
    buf[i * 4 + 3] = field[i].active + 'h';
  }
  buf[hfieldsize * vfieldsize * 4] = '\0';
  games_conf_set_string (KEY_SAVED_GROUP, KEY_SAVED_FIELD, buf);
  for (i = 0; i < npieces; i++)
    buf[i] = preview[i] + 'h';
  buf[npieces] = '\0';
  games_conf_set_string (KEY_SAVED_GROUP, KEY_SAVED_PREVIEW, buf);
  g_free (buf);

  argv[argc++] = g_get_prgname ();

  egg_sm_client_set_restart_command (client, argc, (const char **) argv);

  return TRUE;
}

static gint
quit_cb (EggSMClient *client,
         gpointer client_data)
{
  gtk_main_quit ();

  return FALSE;
}

static void
restart (void)
{
  gchar *buf;
  int i;

  /* This isn't really a good idea, but if we're going to
   * restore the game the score has to be stored somewhere
   * and without some sort of restricted-access storage the
   * user will always be able to change it. */
  score = games_conf_get_integer (KEY_SAVED_GROUP, KEY_SAVED_SCORE, NULL);

  buf = games_conf_get_string (KEY_SAVED_GROUP, KEY_SAVED_FIELD, NULL);
  if (buf) {
    for (i = 0; i < hfieldsize * vfieldsize; i++) {
      field[i].color = CLAMP (buf[i * 4] - 'h', 1, ncolors);
      field[i].pathsearch =
        CLAMP (buf[i * 4 + 1] - 'h', -1, hfieldsize * vfieldsize);
      field[i].phase = CLAMP (buf[i * 4 + 2] - 'h', 0, 3);
      field[i].active = CLAMP (buf[i * 4 + 3] - 'h', -1, 1);
    }
    g_free (buf);
  }
  buf = games_conf_get_string (KEY_SAVED_GROUP, KEY_SAVED_PREVIEW, NULL);
  if (buf) {
    for (i = 0; i < npieces; i++)
      preview[i] = CLAMP (buf[i] - 'h', 1, ncolors);
    g_free (buf);
  }
}

#endif /* WITH_SMCLIENT */

static void
load_properties (void)
{
  gchar *buf;

  ball_filename = games_conf_get_string_with_default (KEY_PREFERENCES_GROUP, KEY_BALL_THEME, DEFAULT_BALL_THEME);

  move_timeout = games_conf_get_integer (KEY_PREFERENCES_GROUP, KEY_MOVE_TIMEOUT, NULL);
  if (move_timeout <= 0)
    move_timeout = 100;

  buf = games_conf_get_string_with_default (KEY_PREFERENCES_GROUP, KEY_BACKGROUND_COLOR, "#000000"); /* FIXMEchpe? */
  set_backgnd_color (buf);
  g_free (buf);

  load_theme ();
}

static const GtkActionEntry actions[] = {
  {"GameMenu", NULL, N_("_Game")},
  {"SettingsMenu", NULL, N_("_Settings")},
  {"HelpMenu", NULL, N_("_Help")},
  {"NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL,
   G_CALLBACK (game_new_callback)},
  {"Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL,
   G_CALLBACK (game_top_ten_callback)},
  {"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (game_quit_callback)},
  {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL,
   G_CALLBACK (game_props_callback)},
  {"Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL,
   G_CALLBACK (game_help_callback)},
  {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL,
   G_CALLBACK (game_about_callback)},
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
  "    <menu action='SettingsMenu'>"
  "      <menuitem action='Fullscreen'/>"
  "      <menuitem action='Preferences'/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='Contents'/>"
  "      <menuitem action='About'/>" "    </menu>" "  </menubar>" "</ui>";

static void
create_menus (GtkUIManager * ui_manager)
{
  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, actions,
                                G_N_ELEMENTS (actions), NULL);

  fullscreen_action = GTK_ACTION (games_fullscreen_action_new ("Fullscreen", GTK_WINDOW (app)));
  gtk_action_group_add_action_with_accel (action_group, fullscreen_action, NULL);

  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (app), accel_group);

  scoreitem = gtk_ui_manager_get_widget (ui_manager,
                                         "/MainMenu/GameMenu/Scores");
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
}

static void
init_config (void)
{
  g_signal_connect (games_conf_get_default (), "value-changed",
                    G_CALLBACK (conf_value_changed_cb), NULL);

  game_size = games_conf_get_integer (KEY_PREFERENCES_GROUP, KEY_SIZE, NULL);
  if (game_size == UNSET)
    game_size = DEFAULT_GAME_SIZE;

  game_size = CLAMP (game_size, SMALL, MAX_SIZE - 1);
  set_sizes (game_size);
}

int
main (int argc, char *argv[])
{
  GOptionContext *context;
  char *label_text;
  GtkWidget *label;
  GtkWidget *vbox, *hbox;
  GtkWidget *preview_hbox;
  GtkUIManager *ui_manager;
  guint i;
  gboolean retval;
  GError *error = NULL;
#ifdef WITH_SMCLIENT
  EggSMClient *sm_client;
#endif /* WITH_SMCLIENT */

  if (!games_runtime_init ("glines"))
    return 1;

#ifdef ENABLE_SETGID
  setgid_io_init ();
#endif

  rgen = g_rand_new ();

  context = g_option_context_new (NULL);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
#ifdef WITH_SMCLIENT
  g_option_context_add_group (context, egg_sm_client_get_option_group ());
#endif /* WITH_SMCLIENT */

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }

  g_set_application_name (_("Five or More"));

  games_conf_initialise ("GLines");

  highscores = games_scores_new ("glines",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 "board size", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

  init_config ();

  games_stock_init ();

  gtk_window_set_default_icon_name ("gnome-glines");

#ifdef WITH_SMCLIENT
  sm_client = egg_sm_client_get ();
  g_signal_connect (sm_client, "save-state",
                    G_CALLBACK (save_state_cb), NULL);
  g_signal_connect (sm_client, "quit",
                    G_CALLBACK (quit_cb), NULL);
  if (egg_sm_client_is_resumed(sm_client))
    restart ();
  else
    reset_game ();
#else
  reset_game ();
#endif /* WITH_SMCLIENT */

  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (app), _("Five or More"));

  gtk_window_set_default_size (GTK_WINDOW (app), DEFAULT_WIDTH, DEFAULT_HEIGHT);
  g_signal_connect (app, "delete-event",
                    G_CALLBACK (game_quit_callback), NULL);
  games_conf_add_window (GTK_WINDOW (app), NULL);

  statusbar = gtk_statusbar_new ();
  ui_manager = gtk_ui_manager_new ();

  games_stock_prepare_for_statusbar_tooltips (ui_manager, statusbar);
  gtk_window_set_has_resize_grip (GTK_WINDOW (app), TRUE);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (app), vbox);

  create_menus (ui_manager);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

  table = gtk_table_new (2, 1, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);

  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 0, 1, 0, 1);
  top_pane = hbox;
  label_text =
    g_strdup_printf ("<span weight=\"bold\">%s</span>", _("Next:"));
  label = gtk_label_new (label_text);
  g_free (label_text);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

  gridframe = games_grid_frame_new (MAXNPIECES, 1);
  games_grid_frame_set_alignment (GAMES_GRID_FRAME (gridframe), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), gridframe, TRUE, TRUE, 0);

  preview_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (gridframe), preview_hbox);

  for (i = 0; i < MAXNPIECES; i++) {
    preview_widgets[i] = gtk_drawing_area_new ();
    gtk_box_pack_start (GTK_BOX (preview_hbox),
                        preview_widgets[i], TRUE, TRUE, 0);
    /* So we have all the windows at configure time since
     * we only hook into one of the configure events. */
    /* Yes, this is an evil hack. */
    gtk_widget_realize (preview_widgets[i]);
  }
  g_signal_connect (preview_widgets[0], "configure-event",
                    G_CALLBACK (preview_configure_cb), NULL);

  scorelabel = gtk_label_new (NULL);

  gtk_box_pack_end (GTK_BOX (hbox), scorelabel, FALSE, FALSE, 5);

  label_text =
    g_strdup_printf ("<span weight=\"bold\">%s</span>", _("Score:"));
  label = gtk_label_new (label_text);
  g_free (label_text);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 5);

  draw_area = gtk_drawing_area_new ();
  g_signal_connect (draw_area, "button-press-event",
                    G_CALLBACK (button_press_event), NULL);
  g_signal_connect (draw_area, "key-press-event",
                    G_CALLBACK (key_press_event), NULL);
  g_signal_connect (draw_area, "configure-event",
                    G_CALLBACK (configure_event_callback), NULL);
  g_signal_connect (draw_area, "draw",
                    G_CALLBACK (field_draw_callback), NULL);
  gridframe = games_grid_frame_new (hfieldsize, vfieldsize);
  games_grid_frame_set_padding (GAMES_GRID_FRAME (gridframe), 1, 1);
  gtk_container_add (GTK_CONTAINER (gridframe), draw_area);
  gtk_table_attach_defaults (GTK_TABLE (table), gridframe, 0, 1, 1, 2);
  bottom_pane = gridframe;

  relay_table ();

  gtk_widget_set_events (draw_area,
                         gtk_widget_get_events (draw_area) |
                         GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK);

  gtk_widget_set_can_focus (draw_area, TRUE);
  gtk_widget_grab_focus (draw_area);

  load_properties ();

  gtk_widget_show_all (app);

  start_game ();

  /* Enter the event loop */
  gtk_main ();

  if (ball_preimage)
    g_object_unref (ball_preimage);

  games_conf_shutdown ();

#ifdef WITH_SMCLIENT
  g_signal_handlers_disconnect_matched (sm_client, G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, NULL);
#endif /* WITH_SMCLIENT */

  games_runtime_shutdown ();

  return 0;
}
