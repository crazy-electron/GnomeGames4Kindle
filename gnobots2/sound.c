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

#include <gdk/gdk.h>

#include <libgames-support/games-sound.h>

#include "gbdefs.h"
#include "sound.h"
#include "properties.h"


/**********************************************************************/
/* Function Definitions                                               */
/**********************************************************************/

/**
 * init_sound
 *
 * Description:
 * Initialises game sound
 *
 * Returns:
 * TRUE on success FALSE otherwise
 **/
gboolean
init_sound (void)
{
  return TRUE;
}


/**
 * play_sound
 * @sno: sound number
 *
 * Description:
 * Plays a game sound
 *
 * Returns:
 * TRUE on success FALSE otherwise
 **/
gboolean
play_sound (gint sno)
{

  if ((sno < 0) || (sno >= NUM_SOUNDS)) {
    return FALSE;
  }

  if (properties_sound ()) {

    switch (sno) {
    case SOUND_VICTORY:
      games_sound_play ("victory");
      break;
    case SOUND_DIE:
      games_sound_play ("die");
      break;
    case SOUND_TELEPORT:
      games_sound_play ("teleport");
      break;
    case SOUND_SPLAT:
      games_sound_play ("splat");
      break;
    case SOUND_BAD:
      gdk_beep ();
      break;
    case SOUND_YAHOO:
      games_sound_play ("yahoo");
      break;
    }

  }

  return TRUE;
}

/**********************************************************************/
