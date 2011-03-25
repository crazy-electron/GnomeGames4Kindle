#ifndef MENU_H
#define MENU_H


/**********************************************************************/
/* Exported functions                                                 */
/**********************************************************************/
void create_game_menus (GtkUIManager *);
void quit_cb (GtkAction *, gpointer);
void set_move_menu_sensitivity (gboolean state);
void connect_toolbar_toggle (GtkWidget *);
gboolean window_state_cb (GtkWidget * widget, GdkEventWindowState * event);
/**********************************************************************/


#endif /* MENU_H */
