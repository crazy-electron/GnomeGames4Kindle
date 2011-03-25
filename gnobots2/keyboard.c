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

#include <ctype.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "keyboard.h"
#include "game.h"

/**********************************************************************/
/* File Static Variables                                              */
/**********************************************************************/
#define N_KEYS 12

static guint control_keys[N_KEYS];

/**********************************************************************/


/**********************************************************************/
/* Function Definitions                                               */
/**********************************************************************/

/**
 * keyboard_set
 * @keys: array of keysyms
 *
 * Description:
 * sets the keybaord mapping
 **/
void
keyboard_set (guint * keys)
{
  gint i;

  if (keys == NULL)
    return;

  for (i = 0; i < 12; ++i) {
    control_keys[i] = keys[i];
  }
}


/**
 * keyboard_cb
 * @widget: widget
 * @event: event
 * @data: callback data
 *
 * Description:
 * handles keyboard events
 *
 * Returns:
 * TRUE if the event is handled
 **/
gint
keyboard_cb (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  gint i, keyval;

  /* This is a bit of a kludge to let through accelerator keys, otherwise
   * if N is used as a key, then Ctrl-N is never picked up. The cleaner
   * option, making the signal a connect_after signal skims the arrow keys
   * before we can get to them which is a bigger problem. */
  if (event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))
    return FALSE;

  keyval = toupper(event->keyval);

  for (i = 0; i < 12; ++i) {
    if (keyval == toupper(control_keys[i])) {
      game_keypress (i);
      return TRUE;
    }
  }

  return FALSE;
}

/**********************************************************************/
