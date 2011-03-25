/* drawing.h : Drawing routines (what else would it be)
 *
 * Copyright (C) 2003 by Callum McKenzie
 *
 * Created: <2003-09-07 10:40:24 callum>
 * Time-stamp: <2003-10-03 08:47:27 callum>
 *
 */

#ifndef DRAWING_H
#define DRAWING_H

#include <gtk/gtk.h>

GtkWidget *create_mahjongg_board (void);
void load_images (gchar * file);
void set_background (gchar * colour);
void draw_tile (gint tileno);
void draw_all_tiles (void);
void calculate_view_geometry (void);
void configure_pixmaps (void);

extern GdkColor bgcolour;

#endif

/* EOF */
