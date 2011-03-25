/*
 * Gnome-Mahjonggg-solubility header file
 * (C) 1998-2002 the Free Software Foundation
 *
 *
 * Author: Michael Meeks.
 *
 * http://www.gnome.org/~michael
 * michael@ximian.com
 */

#ifndef SOLUBILITY_H
#define SOLUBILITY_H

extern int tile_free (int);
extern void generate_game (guint seed);
extern void generate_dependencies (void);

int shuffle (void);

#endif /* SOLUBILITY_H */
