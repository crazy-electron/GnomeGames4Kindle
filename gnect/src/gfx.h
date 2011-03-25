/* gfx.h */



gboolean gfx_load_pixmaps (void);
gboolean gfx_change_theme (void);
void gfx_free (void);
void gfx_resize (GtkWidget * w);
void gfx_expose (cairo_t *cr);
void gfx_draw_tile (gint r, gint c);
void gfx_draw_all (void);
gint gfx_get_column (gint xpos);
void gfx_refresh_pixmaps (void);
