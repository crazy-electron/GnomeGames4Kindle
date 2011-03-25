/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include <libgames-support/games-frame.h>
#include <libgames-support/games-controls.h>
#include <libgames-support/games-pause-action.h>

#ifdef GGZ_CLIENT
#include "ggz-network.h"
#endif

#include "preferences.h"
#include "main.h"

#define KB_TEXT_WIDTH 60
#define KB_TEXT_HEIGHT 32
#define KB_TEXT_NCHARS 8

extern GtkWidget *drawing_area;
static GtkWidget *pref_dialog = NULL;
static gint unpause = 0;
extern GtkWidget *window;
extern GnibblesProperties *properties;

GtkWidget *start_level_label, *start_level_spin_button;
GtkWidget *num_human, *num_ai;


static void
network_set_preferences (void)
{
#ifdef GGZ_CLIENT
  if (ggz_network_mode) {
    network_req_settings (properties->gamespeed,
                          properties->fakes, properties->startlevel);
  }
#endif
}

static void
destroy_cb (GtkWidget * widget, gpointer data)
{
  if (unpause) {
    gtk_action_activate (pause_action);
    unpause = 0;
  }
  network_set_preferences ();
  pref_dialog = NULL;
}

static void
apply_cb (GtkWidget * widget, gint action, gpointer data)
{
  gtk_widget_destroy (widget);
}

static void
game_speed_cb (GtkWidget * widget, gpointer data)
{
  if (!pref_dialog)
    return;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
    gnibbles_properties_set_speed (GPOINTER_TO_INT (data));
  }
}

static void
start_level_cb (GtkWidget * widget, gpointer data)
{
  gint level;

  if (!pref_dialog)
    return;

  end_game (FALSE);

  level = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (data));
  gnibbles_properties_set_start_level (level);
}

static void
random_order_cb (GtkWidget * widget, gpointer data)
{
  gboolean value;

  if (!pref_dialog)
    return;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  gtk_widget_set_sensitive (start_level_label, !value);
  gtk_widget_set_sensitive (start_level_spin_button, !value);

  gnibbles_properties_set_random (value);
}

static void
fake_bonus_cb (GtkWidget * widget, gpointer data)
{
  gboolean value;

  if (!pref_dialog)
    return;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  gnibbles_properties_set_fakes (value);
}

static void
sound_cb (GtkWidget * widget, gpointer data)
{
  gboolean value;

  if (!pref_dialog)
    return;

  value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  gnibbles_properties_set_sound (value);
}

static void
num_worms_cb (GtkWidget * widget, gpointer data)
{
  gint human, ai;

  if (!pref_dialog)
    return;

  human = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (num_human));
  ai = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (num_ai));

  if (!ai && !human) {
    human = 1;
  } else if (data == num_human && ai + human >= NUMWORMS) {
    ai = NUMWORMS - human;
  } else if (data == num_ai && ai + human >= NUMWORMS) {
    human = NUMWORMS - ai;
  }
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (num_human), human);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (num_ai), ai);
  gnibbles_properties_set_worms_number (human);
  gnibbles_properties_set_ai_number (ai);
}

static void
set_worm_color_cb (GtkWidget * widget, gpointer data)
{
  gint color = gtk_combo_box_get_active (GTK_COMBO_BOX (widget)) + WORMRED;
  gint worm = GPOINTER_TO_INT (data);

  gnibbles_properties_set_worm_color (worm, color);
}

static void
set_worm_controls_sensitivity (gint i, gboolean value)
{
  /* FIXME */

  /* This is meant to make the up and down controls
   * unavailable if we are in relative mode. However
   * The new key selection API doesn't support this
   * yet. */
}

static void
worm_relative_movement_cb (GtkWidget * widget, gpointer data)
{
  gint i;

  if (pref_dialog == NULL)
    return;

  i = GPOINTER_TO_INT (data);

  set_worm_controls_sensitivity
    (i, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

  gnibbles_properties_set_worm_relative_movement
    (i, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
}

void
gnibbles_preferences_cb (GtkWidget * widget, gpointer data)
{
  GtkWidget *notebook;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *levelspinner;
  GtkWidget *vbox, *vbox2;
  GtkAdjustment *adjustment;
  GtkWidget *label2;
  GtkWidget *table, *table2;
  GtkWidget *omenu;
  GtkWidget *controls;
  gchar *buffer;
  gint i;
  gint running = 0;

  if (pref_dialog) {
    gtk_window_present (GTK_WINDOW (pref_dialog));
    return;
  }

  if (!games_pause_action_get_is_paused (GAMES_PAUSE_ACTION (pause_action))) {
    unpause = 1;
    gtk_action_activate (pause_action);
  }

  if (game_running ())
    running = 1;

  pref_dialog = gtk_dialog_new_with_buttons (_("Nibbles Preferences"),
                                             GTK_WINDOW (window), 0,
                                             GTK_STOCK_CLOSE,
                                             GTK_RESPONSE_CLOSE, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (pref_dialog), 5);
  gtk_box_set_spacing
    (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))), 2);

  notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 5);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (pref_dialog))),
                     notebook);

  label = gtk_label_new (_("Game"));
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 18);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, label);

  frame = games_frame_new (_("Speed"));
  if (running)
    gtk_widget_set_sensitive (frame, FALSE);

  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1, 0,
                    GTK_FILL | GTK_EXPAND, 0, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  button = gtk_radio_button_new_with_label (NULL, _("Nibbles newbie"));

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  if (properties->gamespeed == 4)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (game_speed_cb), (gpointer) 4);

  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
                                            (GTK_RADIO_BUTTON (button)),
                                            _("My second day"));

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  if (properties->gamespeed == 3)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (game_speed_cb), (gpointer) 3);

  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
                                            (GTK_RADIO_BUTTON (button)),
                                            _("Not too shabby"));

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  if (properties->gamespeed == 2)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (game_speed_cb), (gpointer) 2);

  button = gtk_radio_button_new_with_label (gtk_radio_button_get_group
                                            (GTK_RADIO_BUTTON (button)),
                                            _("Finger-twitching good"));

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  if (properties->gamespeed == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (game_speed_cb), (gpointer) 1);


  /* Options */
  frame = games_frame_new (_("Options"));
  gtk_table_attach_defaults (GTK_TABLE (table), frame, 1, 2, 0, 1);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  button =
    gtk_check_button_new_with_mnemonic (_("_Play levels in random order"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  if (running || ggz_network_mode)
    gtk_widget_set_sensitive (button, FALSE);
  if (properties->random)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (random_order_cb), NULL);

  button = gtk_check_button_new_with_mnemonic (_("_Enable fake bonuses"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  if (running)
    gtk_widget_set_sensitive (button, FALSE);
  if (properties->fakes)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (fake_bonus_cb), NULL);

  button = gtk_check_button_new_with_mnemonic (_("E_nable sounds"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  if (properties->sound)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (GTK_WIDGET (button), "toggled", G_CALLBACK
                    (sound_cb), NULL);

  table2 = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table2, FALSE, FALSE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 12);
  gtk_container_set_border_width (GTK_CONTAINER (table2), 0);

  label2 = gtk_label_new_with_mnemonic (_("_Starting level:"));
  start_level_label = label2;
  gtk_widget_set_name (label2, "StartLevelLabel");
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  if (properties->random)
    gtk_widget_set_sensitive (GTK_WIDGET (label2), FALSE);
  if (running)
    gtk_widget_set_sensitive (GTK_WIDGET (label2), FALSE);
  gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

  adjustment = gtk_adjustment_new ((gfloat) properties->startlevel, 1.0,
                                   MAXLEVEL, 1.0, 5.0, 0.0);

  levelspinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
  start_level_spin_button = levelspinner;
  gtk_widget_set_name (levelspinner, "StartLevelSpinButton");
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (levelspinner), FALSE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), levelspinner);

  if (properties->random)
    gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
  if (running)
    gtk_widget_set_sensitive (GTK_WIDGET (levelspinner), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table2), levelspinner, 1, 2, 0, 1);
  g_signal_connect (GTK_ADJUSTMENT (adjustment), "value_changed",
                    G_CALLBACK (start_level_cb), levelspinner);

  label2 = gtk_label_new_with_mnemonic (_("Number of _human players:"));
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 1, 2, GTK_FILL, 0, 0,  0);
  if (running || ggz_network_mode)
    gtk_widget_set_sensitive (label2, FALSE);

  adjustment = gtk_adjustment_new ((gfloat) properties->human, 0.0,
                                   NUMWORMS, 1.0, 1.0, 0.0);

  num_human = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (num_human), FALSE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), num_human);

  gtk_table_attach_defaults (GTK_TABLE (table2), num_human, 1, 2, 1, 2);
  if (running || ggz_network_mode)
    gtk_widget_set_sensitive (num_human, FALSE);
  g_signal_connect (GTK_ADJUSTMENT (adjustment), "value_changed",
                    G_CALLBACK (num_worms_cb), num_human);

  label2 = gtk_label_new_with_mnemonic (_("Number of _AI players:"));
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);
  if (running || ggz_network_mode)
    gtk_widget_set_sensitive (label2, FALSE);

  adjustment = gtk_adjustment_new ((gfloat) properties->ai, 0.0,
                                   NUMWORMS, 1.0, 1.0, 0.0);

  num_ai = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (num_ai), FALSE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label2), num_ai);

  gtk_table_attach_defaults (GTK_TABLE (table2), num_ai, 1, 2, 2, 3);
  if (running || ggz_network_mode)
    gtk_widget_set_sensitive (num_ai, FALSE);
  g_signal_connect (GTK_ADJUSTMENT (adjustment), "value_changed",
                    G_CALLBACK (num_worms_cb), num_ai);

  for (i = 0; i < NUMWORMS; i++) {
    char up_key[64];
    char down_key[64];
    char left_key[64];
    char right_key[64];

    buffer = g_strdup_printf ("%s %d", _("Worm"), i + 1);
    label = gtk_label_new (buffer);
    g_free (buffer);

    vbox = gtk_vbox_new (FALSE, 18);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

    frame = games_frame_new (_("Keyboard Controls"));

    controls = games_controls_list_new (KEY_PREFERENCES_GROUP);

    g_snprintf (left_key, sizeof (left_key), KEY_WORM_LEFT, i);
    g_snprintf (right_key, sizeof (right_key), KEY_WORM_RIGHT, i);
    g_snprintf (up_key, sizeof (up_key), KEY_WORM_UP, i);
    g_snprintf (down_key, sizeof (down_key), KEY_WORM_DOWN, i);

    games_controls_list_add_controls (GAMES_CONTROLS_LIST (controls),
                                      left_key, _("Move left"), GDK_KEY_Left,
                                      right_key, _("Move right"), GDK_KEY_Right,
                                      up_key, _("Move up"), GDK_KEY_Up,
                                      down_key, _("Move down"), GDK_KEY_Down,
                                      NULL);
    gtk_container_add (GTK_CONTAINER (frame), controls);

    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

    frame = games_frame_new (_("Options"));
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

    vbox2 = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (frame), vbox2);

    button = gtk_check_button_new_with_mnemonic (_("_Use relative movement"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);

    table2 = gtk_table_new (1, 2, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (table2), 12);
    gtk_box_pack_start (GTK_BOX (vbox2), table2, FALSE, FALSE, 0);

    label2 = gtk_label_new_with_mnemonic (_("_Worm color:"));
    gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
    gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 0, 1, 0, 0, 0, 0);

    omenu = gtk_combo_box_text_new ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label2), omenu);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Red"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Green"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Blue"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Yellow"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Cyan"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Purple"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (omenu), _("Gray"));
    g_signal_connect (GTK_WIDGET (omenu), "changed",
                      G_CALLBACK (set_worm_color_cb),
                      GINT_TO_POINTER (i));
    gtk_combo_box_set_active (GTK_COMBO_BOX (omenu),
                              properties->wormprops[i]->color - WORMRED);
    gtk_table_attach_defaults (GTK_TABLE (table2), omenu, 1, 2, 0, 1);

    set_worm_controls_sensitivity (i, properties->wormprops[i]->relmove);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                  properties->wormprops[i]->relmove);
    g_signal_connect (G_OBJECT (button), "toggled",
                      G_CALLBACK (worm_relative_movement_cb),
                      GINT_TO_POINTER (i));
  }

  g_signal_connect (G_OBJECT (pref_dialog), "response",
                    G_CALLBACK (apply_cb), NULL);
  g_signal_connect (G_OBJECT (pref_dialog), "destroy",
                    G_CALLBACK (destroy_cb), NULL);
  g_signal_connect (G_OBJECT (pref_dialog), "close",
                    G_CALLBACK (destroy_cb), NULL);

  gtk_widget_show_all (pref_dialog);

}
