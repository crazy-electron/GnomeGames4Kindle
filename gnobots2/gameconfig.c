/*
 * Gnome Robots II
 * written by Mark Rae <m.rae@inpharmatica.co.uk>
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

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>

#include <gtk/gtk.h>

#include <libgames-support/games-runtime.h>

#include "gameconfig.h"
#include "gbdefs.h"


/**********************************************************************/
/* File Static Variables                                              */
/**********************************************************************/
static gint num_configs = -1;
static GameConfig **game_configs = NULL;
static gint current_config = -1;
/**********************************************************************/


/**********************************************************************/
/* Function Prototypes                                                */
/**********************************************************************/
static GameConfig *load_config (gchar *);
/**********************************************************************/


/**********************************************************************/
/* Function Definitions                                               */
/**********************************************************************/

/**
 * load_config
 * @fname: config file name
 *
 * Description:
 * loads a configuraion from a file
 *
 * Returns:
 * a pointer to a GameConfig structure or NULL on failure
 **/
static GameConfig *
load_config (gchar * fname)
{
  GameConfig *gcfg;
  gint pflag = 0;
  FILE *fp;
  gchar buffer[256];
  gchar *bptr;
  gchar *bpstart, *bpstart2;
  gchar *vptr;
  gint val;

  fp = fopen (fname, "r");
  if (fp == NULL)
    return NULL;

  gcfg = g_new (GameConfig, 1);

  bpstart = g_strdup (fname);
  bptr = g_strrstr (bpstart, ".cfg");
  if (bptr != NULL)
    *bptr = 0;

  bpstart2 = g_path_get_basename (bpstart);
  g_free (bpstart);
  gcfg->description = g_string_new (bpstart2);
  g_free (bpstart2);

  while (fgets (buffer, sizeof(buffer), fp) != NULL) {
    if (strlen (buffer) < 3)
      continue;

    bptr = buffer;
    vptr = NULL;
    while (*bptr) {
      if (*bptr == '=') {
	*bptr = 0;
	vptr = ++bptr;
	break;
      }
      bptr++;
    }

    if (!vptr)
      continue;
    if (sscanf (vptr, "%d", &val) != 1)
      continue;

    if (!strcmp (buffer, "initial_type1")) {
      gcfg->initial_type1 = val;
      pflag |= 0x00000001;
    }
    if (!strcmp (buffer, "initial_type2")) {
      gcfg->initial_type2 = val;
      pflag |= 0x00000002;
    }
    if (!strcmp (buffer, "increment_type1")) {
      gcfg->increment_type1 = val;
      pflag |= 0x00000004;
    }
    if (!strcmp (buffer, "increment_type2")) {
      gcfg->increment_type2 = val;
      pflag |= 0x00000008;
    }
    if (!strcmp (buffer, "maximum_type1")) {
      gcfg->maximum_type1 = val;
      pflag |= 0x00000010;
    }
    if (!strcmp (buffer, "maximum_type2")) {
      gcfg->maximum_type2 = val;
      pflag |= 0x00000020;
    }
    if (!strcmp (buffer, "score_type1")) {
      gcfg->score_type1 = val;
      pflag |= 0x00000040;
    }
    if (!strcmp (buffer, "score_type2")) {
      gcfg->score_type2 = val;
      pflag |= 0x00000080;
    }
    if (!strcmp (buffer, "score_type1_waiting")) {
      gcfg->score_type1_waiting = val;
      pflag |= 0x00000100;
    }
    if (!strcmp (buffer, "score_type2_waiting")) {
      gcfg->score_type2_waiting = val;
      pflag |= 0x00000200;
    }
    if (!strcmp (buffer, "score_type1_splatted")) {
      gcfg->score_type1_splatted = val;
      pflag |= 0x00000400;
    }
    if (!strcmp (buffer, "score_type2_splatted")) {
      gcfg->score_type2_splatted = val;
      pflag |= 0x00000800;
    }
    if (!strcmp (buffer, "num_robots_per_safe")) {
      gcfg->num_robots_per_safe = val;
      pflag |= 0x00001000;
    }
    if (!strcmp (buffer, "safe_score_boundary")) {
      gcfg->safe_score_boundary = val;
      pflag |= 0x00002000;
    }
    if (!strcmp (buffer, "max_safe_teleports")) {
      gcfg->max_safe_teleports = val;
      pflag |= 0x00004000;
    }
    if (!strcmp (buffer, "initial_safe_teleports")) {
      gcfg->initial_safe_teleports = val;
      pflag |= 0x00008000;
    }
    if (!strcmp (buffer, "free_safe_teleports")) {
      gcfg->free_safe_teleports = val;
      pflag |= 0x00010000;
    }
    if (!strcmp (buffer, "moveable_heaps")) {
      gcfg->moveable_heaps = val;
      pflag |= 0x00020000;
    }

  }

  fclose (fp);

  /* Check we have got all types */
  if (pflag != 0x0003ffff) {
    g_free (gcfg);
    gcfg = NULL;
  }

  return gcfg;
}


/**
 * load_game_configs
 *
 * Description:
 * loads all of the available configurations
 *
 * Returns:
 * TRUE on success, FALSE otherwise
 **/
gboolean
load_game_configs (void)
{
  gint i;
  GameConfig *gcfg;
  const gchar *dent;
  GDir *dir;
  gchar *buffer;
  const char *dname;

  dname = games_runtime_get_directory (GAMES_RUNTIME_GAME_GAMES_DIRECTORY);

  if (game_configs != NULL) {
    free_game_configs ();
  }

  dir = g_dir_open (dname, 0, NULL);
  if (dir == NULL)
    return FALSE;

  num_configs = 0;
  while ((dent = g_dir_read_name (dir)) != NULL) {
    if (!g_strrstr (dent, ".cfg")) {
      continue;
    }
    num_configs++;
  }

  game_configs = g_new (GameConfig *, num_configs);
  for (i = 0; i < num_configs; ++i) {
    game_configs[i] = NULL;
  }

  g_dir_rewind (dir);

  num_configs = 0;
  while ((dent = g_dir_read_name (dir)) != NULL) {
    if (!g_strrstr (dent, ".cfg")) {
      continue;
    }

    buffer = g_build_filename (dname, dent, NULL);

    gcfg = load_config (buffer);
    g_free (buffer);
    if (gcfg != NULL) {
      game_configs[num_configs] = gcfg;
      num_configs++;
    }
  }

  g_dir_close (dir);

  if (num_configs >= 0) {
    current_config = 0;
  } else {
    return FALSE;
  }

  return TRUE;
}


/**
 * free_game_configs
 *
 * Description:
 * frees the memory used by the loaded configurations
 *
 * Returns:
 * TRUE on success, FALSE otherwise
 **/
gboolean
free_game_configs (void)
{
  gint i;

  if (game_configs == NULL) {
    return FALSE;
  }

  for (i = 0; i < num_configs; ++i) {
    g_free (game_configs[i]);
  }
  g_free (game_configs);

  game_configs = NULL;
  current_config = -1;
  num_configs = -1;

  return TRUE;
}


/**
 * num_game_configs
 *
 * Description:
 * returns the number of available game configurations
 *
 * Returns:
 * number of configurations
 **/
gint
num_game_configs (void)
{
  if (game_configs == NULL)
    return -1;

  return num_configs;
}


/**
 * set_game_config
 * @n: configuraion number
 *
 * Description:
 * sets the game configuration to use
 *
 * Returns:
 * TRUE on success, FALSE otherwise
 **/
gboolean
set_game_config (gint n)
{
  if (game_configs == NULL)
    return FALSE;

  if ((n < 0) || (n >= num_configs))
    return FALSE;

  current_config = n;

  return TRUE;
}


/**
 * game_config
 *
 * Description:
 * returns a pointer to the currently selected game configuration
 *
 * Returns:
 * pointer to a GameConfig structure or NULL
 **/
GameConfig *
game_config (void)
{
  if (game_configs == NULL)
    return NULL;

  return game_configs[current_config];
}


/**
 * game_config_settings
 * @n: config number
 *
 * Description:
 * returns a pointer to a specified game configuration
 *
 * Returns:
 * pointer to a GameConfig structure or NULL
 **/
GameConfig *
game_config_settings (gint n)
{
  if (game_configs == NULL)
    return NULL;

  if ((n < 0) || (n >= num_configs))
    return NULL;

  return game_configs[n];
}


/**
 * current_game_config
 *
 * Description:
 * returns the number of the currently selected configuration
 *
 * Returns:
 * the selected configuration or -1
 **/
gint
current_game_config (void)
{
  if (game_configs == NULL)
    return -1;

  return current_config;
}


/**
 * game_config_name
 * @n: configuration number
 *
 * Description:
 * returns a string containing the name of the specified configuration
 *
 * Returns:
 * pointer to a string containing the name that the caller must free.
 **/
gchar *
game_config_name (gint n)
{
  gchar *buffer, *ptr;

  if (game_configs == NULL)
    return NULL;

  if ((n < 0) || (n >= num_configs))
    return NULL;

  buffer = g_strdup (game_configs[n]->description->str);
  ptr = buffer;

  while (*ptr) {
    if (*ptr == '_') {
      *ptr = ' ';
    }
    ++ptr;
  }

  return buffer;
}


/**
 * game_config_filename
 * @n: configuration number
 *
 * Description:
 * returns a string containing the filename for the specified configuration
 *
 * Returns:
 * pointer to a string containing the filename
 **/
gchar *
game_config_filename (gint n)
{
  if (game_configs == NULL)
    return NULL;

  if ((n < 0) || (n >= num_configs))
    return NULL;

  return game_configs[n]->description->str;
}

/**********************************************************************/
