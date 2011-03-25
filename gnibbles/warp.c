/*
 *   Gnome Nibbles: Gnome Worm Game
 *   Written by Sean MacIsaac <sjm@acm.org>, Ian Peters <itp@gnu.org>,
 *              Guillaume Beland <guillaume.beland@gmail.com>
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

#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

#include "gnibbles.h"
#include "warp.h"
#include "properties.h"
#include "board.h"

extern GnibblesProperties *properties;
extern GdkPixbuf *boni_pixmaps[];
extern GnibblesBoard *board;
extern ClutterActor *stage;

static void animate_warp1 (ClutterAnimation *animation, ClutterActor *actor);
static void animate_warp2 (ClutterAnimation *animation, ClutterActor *actor);

static void
animate_warp1 (ClutterAnimation *animation, ClutterActor *actor)
{
  g_signal_connect_after (
    clutter_actor_animate (actor, CLUTTER_LINEAR, 1100,
                           "opacity", 0x96,
                           NULL),
     "completed", G_CALLBACK (animate_warp2), actor);

}

static void
animate_warp2 (ClutterAnimation *animation, ClutterActor *actor)
{
  g_signal_connect_after (
    clutter_actor_animate (actor, CLUTTER_LINEAR, 1100,
                           "opacity", 0xff,
                           NULL),
    "completed", G_CALLBACK (animate_warp1), actor);

}


GnibblesWarp *
gnibbles_warp_new (gint t_x, gint t_y, gint t_wx, gint t_wy)
{
  GnibblesWarp *tmp;

  tmp = g_new (GnibblesWarp, 1);

  tmp->x = t_x;
  tmp->y = t_y;
  tmp->wx = t_wx;
  tmp->wy = t_wy;
  tmp->actor = gtk_clutter_texture_new ();

  return (tmp);
}

void
gnibbles_warp_draw (GnibblesWarp *warp)
{
  GError *err = NULL;

  gtk_clutter_texture_set_from_pixbuf (GTK_CLUTTER_TEXTURE (warp->actor),
                                      boni_pixmaps[WARP],
                                      &err);
  if (err)
    gnibbles_error (err->message);

  clutter_actor_set_position (CLUTTER_ACTOR (warp->actor),
                              properties->tilesize * warp->x,
                              properties->tilesize * warp->y);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), warp->actor);
  clutter_actor_set_opacity (warp->actor, 0);
  clutter_actor_set_scale (warp->actor, 2.0, 2.0);
  //g_signal_connect_after (
    clutter_actor_animate (warp->actor, CLUTTER_EASE_OUT_CIRC, 410,
                          "scale-x", 1.0, "scale-y", 1.0,
                          "fixed::scale-gravity", CLUTTER_GRAVITY_CENTER,
                          "opacity", 0xff,
                          NULL);
   //"completed", G_CALLBACK (animate_warp1), warp->actor);
}
