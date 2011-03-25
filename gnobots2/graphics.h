#ifndef GRAPHICS_H
#define GRAPHICS_H

/* The size of an individual tile. */
extern gint tile_width;
extern gint tile_height;

/**********************************************************************/
/* Exported functions                                                 */
/**********************************************************************/
gboolean resize_cb (GtkWidget * w, GdkEventConfigure * e, gpointer data);
gboolean draw_cb (GtkWidget * w, cairo_t * cr, gpointer data);
gboolean load_game_graphics (void);
gboolean free_game_graphics (void);
gint num_game_graphics (void);
void set_game_graphics (gchar *);
void set_background_color (GdkColor color);
void set_background_color_from_name (gchar * name);

void add_yahoo_bubble (gint, gint);
void add_aieee_bubble (gint, gint);
void add_splat_bubble (gint, gint);
void remove_bubble (void);
void remove_splat_bubble (void);

void redraw (void);
void clear_game_area (void);
void animate_game_graphics (void);
void reset_player_animation (void);
void player_animation_dead (void);
/**********************************************************************/


#endif /* GRAPHICS_H */
