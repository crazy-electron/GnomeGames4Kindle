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

#include <config.h>

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libgames-support/games-sound.h>
#include <libgames-support/games-scores.h>
#include <libgames-support/games-conf.h>

#include "properties.h"
#include "main.h"

#define MAX_SPEED 4

typedef struct _ColorLookup ColorLookup;

struct _ColorLookup {
  gint colorval;
  gchar *name;
};

ColorLookup color_lookup[NUM_COLORS] = {
  {WORMRED, "red"},
  {WORMGREEN, "green"},
  {WORMBLUE, "blue"},
  {WORMYELLOW, "orange"},
  {WORMCYAN, "cyan"},
  {WORMPURPLE, "purple"},
  {WORMGRAY, "gray"}
};

static gint
colorval_from_name (gchar * name)
{
  gint i;

  for (i = 0; i < NUM_COLORS; i++)
    if (!strcmp (name, color_lookup[i].name))
      return color_lookup[i].colorval;

  return 0;
}

gchar *
colorval_name (gint colorval)
{
  gint i;

  for (i = 0; i < NUM_COLORS; i++)
    if (colorval == color_lookup[i].colorval)
      return color_lookup[i].name;

  return "unknown";
}

void
gnibbles_properties_update (GnibblesProperties * tmp)
{
  gint i;
  gchar buffer[256];
  gchar *category;
  gchar *color_name;

  tmp->human = games_conf_get_integer (KEY_PREFERENCES_GROUP,
                                       KEY_NUM_WORMS, NULL);
  if (tmp->human < 0)
    tmp->human = 0;
  else if (tmp->human > NUMWORMS)
    tmp->human = NUMWORMS;

  tmp->ai = games_conf_get_integer (KEY_PREFERENCES_GROUP, KEY_NUM_AI, NULL);
  if (tmp->ai < 0)
    tmp->ai = 0;
  else if (tmp->ai > NUMWORMS)
    tmp->ai = NUMWORMS;

  tmp->numworms = tmp->human + tmp->ai;

  tmp->gamespeed = games_conf_get_integer (KEY_PREFERENCES_GROUP,
                                           KEY_SPEED, NULL);
  if (tmp->gamespeed < 1)
    tmp->gamespeed = 2;
  else if (tmp->gamespeed > MAX_SPEED)
    tmp->gamespeed = MAX_SPEED;

  tmp->fakes = games_conf_get_boolean (KEY_PREFERENCES_GROUP,
                                       KEY_FAKES, NULL);

  tmp->random = games_conf_get_boolean (KEY_PREFERENCES_GROUP,
                                        KEY_RANDOM, NULL);

  tmp->startlevel = games_conf_get_integer (KEY_PREFERENCES_GROUP,
                                            KEY_START_LEVEL, NULL);
  if (tmp->startlevel < 1)
    tmp->startlevel = 1;
  if (tmp->startlevel > MAXLEVEL)
    tmp->startlevel = MAXLEVEL;

  tmp->sound = games_conf_get_boolean (KEY_PREFERENCES_GROUP, KEY_SOUND, NULL);
  games_sound_enable (tmp->sound);

  tmp->tilesize = games_conf_get_integer (KEY_PREFERENCES_GROUP,
                                          KEY_TILE_SIZE, NULL);
  if (tmp->tilesize < 1)
    tmp->tilesize = 5;
  if (tmp->tilesize > 30)
    tmp->tilesize = 30;

  for (i = 0; i < NUMWORMS; i++) {
    tmp->wormprops[i] = g_slice_new0 (GnibblesWormProps);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_COLOR, i);
    color_name = games_conf_get_string_with_default (KEY_PREFERENCES_GROUP,
                                                     buffer, "red");
    tmp->wormprops[i]->color = colorval_from_name (color_name);
    g_free (color_name);

    if (tmp->wormprops[i]->color < 1)
      tmp->wormprops[i]->color = (i % NUM_COLORS) + WORMRED;
    if (tmp->wormprops[i]->color > WORMRED + NUM_COLORS)
      tmp->wormprops[i]->color = (i % NUM_COLORS) + WORMRED;

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_REL_MOVE, i);
    tmp->wormprops[i]->relmove =
                 games_conf_get_boolean (KEY_PREFERENCES_GROUP,
                                         buffer, NULL);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_UP, i);
    tmp->wormprops[i]->up =
                 games_conf_get_keyval_with_default (KEY_PREFERENCES_GROUP,
                                                     buffer, GDK_KEY_Up);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_DOWN, i);
    tmp->wormprops[i]->down =
                 games_conf_get_keyval_with_default (KEY_PREFERENCES_GROUP,
                                                     buffer, GDK_KEY_Down);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_LEFT, i);
    tmp->wormprops[i]->left =
                 games_conf_get_keyval_with_default (KEY_PREFERENCES_GROUP,
                                                     buffer, GDK_KEY_Left);

    g_snprintf (buffer, sizeof (buffer), KEY_WORM_RIGHT, i);
    tmp->wormprops[i]->right =
                 games_conf_get_keyval_with_default (KEY_PREFERENCES_GROUP,
                                                     buffer, GDK_KEY_Right);
  }

  category = g_strdup_printf ("%d.%d", tmp->gamespeed, tmp->fakes);
  games_scores_set_category (highscores, category);
  g_free (category);
}

static void
conf_value_changed_cb (GamesConf *conf,
                       const char *group,
                       const char *key,
                       gpointer data)
{
  GnibblesProperties *props = (GnibblesProperties *) data;

  if (!group || strcmp (group, KEY_PREFERENCES_GROUP) != 0)
    return;

  gnibbles_properties_update (props);
}

GnibblesProperties *
gnibbles_properties_new (void)
{
  GnibblesProperties *props;

  props = g_slice_new0 (GnibblesProperties);

  props->conf_notify_id = g_signal_connect (games_conf_get_default (),
                                            "value-changed",
                                            G_CALLBACK (conf_value_changed_cb),
                                            props);

  gnibbles_properties_update (props);

  return props;
}

void
gnibbles_properties_destroy (GnibblesProperties * props)
{
  int i;

  for (i = 0; i < NUMWORMS; i++)
    g_slice_free (GnibblesWormProps, props->wormprops[i]);

  g_signal_handler_disconnect (games_conf_get_default (),
                               props->conf_notify_id);

  g_slice_free (GnibblesProperties, props);
}

/* FIXME: I suppose these set functions should be combined somehow */

void
gnibbles_properties_set_worms_number (gint value)
{
  games_conf_set_integer (KEY_PREFERENCES_GROUP, KEY_NUM_WORMS, value);
}

void
gnibbles_properties_set_ai_number (gint value)
{
  games_conf_set_integer (KEY_PREFERENCES_GROUP, KEY_NUM_AI, value);
}

void
gnibbles_properties_set_speed (gint value)
{
  games_conf_set_integer (KEY_PREFERENCES_GROUP, KEY_SPEED, value);
}

void
gnibbles_properties_set_fakes (gboolean value)
{
  games_conf_set_boolean (KEY_PREFERENCES_GROUP, KEY_FAKES, value);
}

void
gnibbles_properties_set_random (gboolean value)
{
  games_conf_set_boolean (KEY_PREFERENCES_GROUP, KEY_RANDOM, value);
}

void
gnibbles_properties_set_start_level (gint value)
{
  games_conf_set_integer (KEY_PREFERENCES_GROUP, KEY_START_LEVEL, value);
}

void
gnibbles_properties_set_sound (gboolean value)
{
  games_conf_set_boolean (KEY_PREFERENCES_GROUP, KEY_SOUND, value);
}

void
gnibbles_properties_set_tile_size (gint value)
{
  games_conf_set_integer (KEY_PREFERENCES_GROUP, KEY_TILE_SIZE, value);
}

void
gnibbles_properties_set_worm_relative_movement (gint i, gboolean value)
{
  char key[64];
  g_snprintf (key, sizeof (key), KEY_WORM_REL_MOVE, i);
  games_conf_set_boolean (KEY_PREFERENCES_GROUP, key, value);
}

void
gnibbles_properties_set_worm_color (gint i, gint value)
{
  char key[64];
  char *color_name;

  g_snprintf (key, sizeof (key), KEY_WORM_COLOR, i);

  color_name = colorval_name (value);
  games_conf_set_string (KEY_PREFERENCES_GROUP, key, color_name);
}

void
gnibbles_properties_save (GnibblesProperties * props)
{
}
