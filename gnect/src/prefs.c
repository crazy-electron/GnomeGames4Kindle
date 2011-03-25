/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/* prefs.c
 *
 * Four-in-a-row for GNOME
 * (C) 2000 - 2004
 * Authors: Timothy Musson <trmusson@ihug.co.nz>
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgames-support/games-conf.h>
#include <libgames-support/games-frame.h>
#include <libgames-support/games-gtk-compat.h>
#include <libgames-support/games-controls.h>
#include <libgames-support/games-sound.h>

#include "main.h"
#include "theme.h"
#include "prefs.h"
#include "gfx.h"

#define DEFAULT_LEVEL_PLAYER1  LEVEL_HUMAN
#define DEFAULT_LEVEL_PLAYER2  LEVEL_WEAK
#define DEFAULT_THEME_ID       0
#define DEFAULT_KEY_LEFT       GDK_KEY_Left
#define DEFAULT_KEY_RIGHT      GDK_KEY_Right
#define DEFAULT_KEY_DROP       GDK_KEY_Down
#define DEFAULT_DO_SOUND       TRUE
#define DEFAULT_DO_ANIMATE     TRUE

Prefs p;

extern GtkWidget *app;
extern Theme theme[];
extern gint n_themes;

static GtkWidget *prefsbox = NULL;
static GtkWidget *frame_player1;
static GtkWidget *frame_player2;
static GtkWidget *radio1[4];
static GtkWidget *radio2[4];
static GtkWidget *combobox_theme;
static GtkWidget *checkbutton_animate;
static GtkWidget *checkbutton_sound;

static gint
gnect_conf_get_int (gchar * key, gint default_int)
{
  return games_conf_get_integer_with_default (NULL, key, default_int);
}

static gboolean
gnect_conf_get_boolean (gchar * key, gboolean default_bool)
{
  gboolean value;
  GError *error = NULL;

  value = games_conf_get_boolean (NULL, key, &error);
  if (error) {
    g_error_free (error);
    value = default_bool;
  }

  return value;
}

static gint
sane_theme_id (gint val)
{
  if (val < 0 || val >= n_themes)
    return DEFAULT_THEME_ID;
  return val;
}

static gint
sane_player_level (gint val)
{
  if (val < LEVEL_HUMAN)
    return LEVEL_HUMAN;
  if (val > LEVEL_STRONG)
    return LEVEL_STRONG;
  return val;
}

static void
prefsbox_update_player_labels (void)
{
  /* Make player selection labels match the current theme */

  gchar *str;

  if (prefsbox == NULL)
    return;

  str = g_strdup_printf (_("Player One:\n%s"), _(theme_get_player (PLAYER1)));
  games_frame_set_label (GAMES_FRAME (frame_player1), str);
  g_free (str);

  str = g_strdup_printf (_("Player Two:\n%s"), _(theme_get_player (PLAYER2)));
  games_frame_set_label (GAMES_FRAME (frame_player2), str);
  g_free (str);
}

static void
conf_value_changed_cb (GamesConf *conf,
                       const char *group,
                       const char *key,
                       gpointer user_data)
{
  if (group != NULL)
    return;

  if (strcmp (key, KEY_DO_ANIMATE) == 0) {
    p.do_animate = games_conf_get_boolean (NULL, KEY_DO_ANIMATE, NULL);
    if (prefsbox == NULL)
      return;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_animate),
                                  p.do_animate);
  } else if (strcmp (key, KEY_DO_SOUND) == 0) {
    p.do_sound = games_conf_get_boolean (NULL, KEY_DO_SOUND, NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_sound),
                                  p.do_sound);
    games_sound_enable (p.do_sound);
  } else if (strcmp (key, KEY_MOVE_LEFT) == 0) {
    p.keypress[MOVE_LEFT] = games_conf_get_keyval_with_default (NULL, KEY_MOVE_LEFT, DEFAULT_KEY_LEFT);
  } else if (strcmp (key, KEY_MOVE_RIGHT) == 0) {
    p.keypress[MOVE_RIGHT] = games_conf_get_keyval_with_default (NULL, KEY_MOVE_RIGHT, DEFAULT_KEY_RIGHT);
  } else if (strcmp (key, KEY_MOVE_DROP) == 0) {
    p.keypress[MOVE_DROP] = games_conf_get_keyval_with_default (NULL, KEY_MOVE_DROP, DEFAULT_KEY_DROP);
  } else if (strcmp (key, KEY_THEME_ID) == 0) {
    gint val;

    val = sane_theme_id (games_conf_get_integer (NULL, KEY_THEME_ID, NULL));
    if (val != p.theme_id) {
      p.theme_id = val;
      if (!gfx_change_theme ())
        return;
      if (prefsbox == NULL)
        return;
      gtk_combo_box_set_active (GTK_COMBO_BOX (combobox_theme), p.theme_id);
      prefsbox_update_player_labels ();
    }
  }
}

static void
on_select_theme (GtkComboBox * combo, gpointer data)
{
  gint id;

  id = gtk_combo_box_get_active (combo);
  games_conf_set_integer (NULL, KEY_THEME_ID, id);
}



static void
on_toggle_animate (GtkToggleButton * t, gpointer data)
{
  p.do_animate = gtk_toggle_button_get_active (t);
  games_conf_set_boolean (NULL, KEY_DO_ANIMATE, gtk_toggle_button_get_active (t));
}

static void
on_toggle_sound (GtkToggleButton * t, gpointer data)
{
  p.do_sound = gtk_toggle_button_get_active (t);
  games_conf_set_boolean (NULL, KEY_DO_SOUND, gtk_toggle_button_get_active (t));
}

static void
on_select_player1 (GtkWidget * w, gpointer data)
{
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    return;
  p.level[PLAYER1] = GPOINTER_TO_INT (data);
  games_conf_set_integer (NULL, KEY_LEVEL_PLAYER1, GPOINTER_TO_INT (data));
  scorebox_reset ();
  who_starts = PLAYER2;		/* This gets reversed in game_reset. */
  game_reset ();
}

static void
on_select_player2 (GtkWidget * w, gpointer data)
{
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    return;
  p.level[PLAYER2] = GPOINTER_TO_INT (data);
  games_conf_set_integer (NULL, KEY_LEVEL_PLAYER2, GPOINTER_TO_INT (data));
  scorebox_reset ();
  who_starts = PLAYER2;		/* This gets reversed in game_reset. */
  game_reset ();
}

void
prefs_init (void)
{
  p.do_sound = gnect_conf_get_boolean (KEY_DO_SOUND, DEFAULT_DO_SOUND);
  p.do_animate = gnect_conf_get_boolean (KEY_DO_ANIMATE, DEFAULT_DO_ANIMATE);
  p.level[PLAYER1] =
    gnect_conf_get_int (KEY_LEVEL_PLAYER1, DEFAULT_LEVEL_PLAYER1);
  p.level[PLAYER2] =
    gnect_conf_get_int (KEY_LEVEL_PLAYER2, DEFAULT_LEVEL_PLAYER2);
  p.keypress[MOVE_LEFT] =
    games_conf_get_keyval_with_default (NULL, KEY_MOVE_LEFT, DEFAULT_KEY_LEFT);
  p.keypress[MOVE_RIGHT] =
    games_conf_get_keyval_with_default (NULL, KEY_MOVE_RIGHT, DEFAULT_KEY_RIGHT);
  p.keypress[MOVE_DROP] =
    games_conf_get_keyval_with_default (NULL, KEY_MOVE_DROP, DEFAULT_KEY_DROP);
  p.theme_id = gnect_conf_get_int (KEY_THEME_ID, DEFAULT_THEME_ID);

  g_signal_connect (games_conf_get_default (), "value-changed",
                    G_CALLBACK (conf_value_changed_cb), NULL);

  p.level[PLAYER1] = sane_player_level (p.level[PLAYER1]);
  p.level[PLAYER2] = sane_player_level (p.level[PLAYER2]);
  p.theme_id = sane_theme_id (p.theme_id);
  games_sound_enable (p.do_sound);
}



static const gchar *
get_player_radio (LevelID id)
{
  switch (id) {
  case LEVEL_HUMAN:
    return _("Human");
  case LEVEL_WEAK:
    return _("Level one");
  case LEVEL_MEDIUM:
    return _("Level two");
  case LEVEL_STRONG:
    return _("Level three");
  }
  return "";
}



void
prefsbox_open (void)
{
  GtkWidget *notebook;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox1, *vbox2;
  GtkWidget *controls_list;
  GtkWidget *label;
  GSList *group;
  gint i;

  if (prefsbox != NULL) {
    gtk_window_present (GTK_WINDOW (prefsbox));
    return;
  }

  prefsbox = gtk_dialog_new_with_buttons (_("Four-in-a-Row Preferences"),
					  GTK_WINDOW (app),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_STOCK_CLOSE,
					  GTK_RESPONSE_ACCEPT, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (prefsbox), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (prefsbox))),
		       2);

  g_signal_connect (G_OBJECT (prefsbox), "destroy",
		    G_CALLBACK (gtk_widget_destroyed), &prefsbox);

  notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 5);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (prefsbox))), 
		      notebook, TRUE, TRUE, 0);


  /* game tab */

  vbox1 = gtk_vbox_new (FALSE, 18);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 12);
  label = gtk_label_new (_("Game"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox1, label);

  hbox = gtk_hbox_new (FALSE, 18);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 0);

  frame_player1 = games_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame_player1, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame_player1), vbox2);

  group = NULL;
  for (i = 0; i < 4; i++) {
    radio1[i] = gtk_radio_button_new_with_label (group, get_player_radio (i));
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio1[i]));
    gtk_box_pack_start (GTK_BOX (vbox2), radio1[i], FALSE, FALSE, 0);
  }

  frame_player2 = games_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame_player2, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame_player2), vbox2);

  group = NULL;
  for (i = 0; i < 4; i++) {
    radio2[i] = gtk_radio_button_new_with_label (group, get_player_radio (i));
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio2[i]));
    gtk_box_pack_start (GTK_BOX (vbox2), radio2[i], FALSE, FALSE, 0);
  }

  frame = games_frame_new (_("Appearance"));
  gtk_box_pack_start (GTK_BOX (vbox1), frame, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 7);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);

  label = gtk_label_new_with_mnemonic (_("_Theme:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 7.45058e-09, 0.5);

  combobox_theme = gtk_combo_box_text_new ();
  for (i = 0; i < n_themes; i++) {
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox_theme),
		 	            _(theme_get_title (i)));
  }

  gtk_box_pack_start (GTK_BOX (hbox), combobox_theme, TRUE, TRUE, 0);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combobox_theme);

  checkbutton_animate =
    gtk_check_button_new_with_mnemonic (_("Enable _animation"));
  gtk_box_pack_start (GTK_BOX (vbox2), checkbutton_animate, FALSE, FALSE, 0);

  checkbutton_sound =
    gtk_check_button_new_with_mnemonic (_("E_nable sounds"));
  gtk_box_pack_start (GTK_BOX (vbox2), checkbutton_sound, FALSE, FALSE, 0);

  /* keyboard tab */

  label = gtk_label_new_with_mnemonic (_("Keyboard Controls"));

  controls_list = games_controls_list_new (NULL);
  games_controls_list_add_controls (GAMES_CONTROLS_LIST (controls_list),
				    KEY_MOVE_LEFT, _("Move left"), DEFAULT_KEY_LEFT,
                                    KEY_MOVE_RIGHT, _("Move right"), DEFAULT_KEY_RIGHT,
				    KEY_MOVE_DROP, _("Drop marble"), DEFAULT_KEY_DROP,
                                    NULL);
  gtk_container_set_border_width (GTK_CONTAINER (controls_list), 12);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), controls_list, label);

  /* fill in initial values */

  prefsbox_update_player_labels ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio1[p.level[PLAYER1]]),
				TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio2[p.level[PLAYER2]]),
				TRUE);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combobox_theme), p.theme_id);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_animate),
				p.do_animate);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton_sound),
				p.do_sound);

  /* connect signals */

  g_signal_connect (prefsbox, "response", G_CALLBACK (on_dialog_close),
		    &prefsbox);

  for (i = 0; i < 4; i++) {
    g_signal_connect (G_OBJECT (radio1[i]), "toggled",
		      G_CALLBACK (on_select_player1), GINT_TO_POINTER (i));
    g_signal_connect (G_OBJECT (radio2[i]), "toggled",
		      G_CALLBACK (on_select_player2), GINT_TO_POINTER (i));
  }

  g_signal_connect (G_OBJECT (combobox_theme), "changed",
		    G_CALLBACK (on_select_theme), NULL);

  g_signal_connect (G_OBJECT (checkbutton_animate), "toggled",
		    G_CALLBACK (on_toggle_animate), NULL);

  g_signal_connect (G_OBJECT (checkbutton_sound), "toggled",
		    G_CALLBACK (on_toggle_sound), NULL);

  gtk_widget_show_all (prefsbox);
}
