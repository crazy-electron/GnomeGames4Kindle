#ifndef PROPERTIES_H
#define PROPERTIES_H

/**********************************************************************/
/* Exported functions                                                 */
/**********************************************************************/
gboolean load_properties (void);
gboolean save_properties (void);
void show_properties_dialog (void);
gboolean properties_super_safe_moves (void);
gboolean properties_safe_moves (void);
gboolean properties_sound (void);
gboolean properties_splats (void);
gboolean properties_show_toolbar (void);
gboolean properties_set_config (gint);
gchar *properties_theme_name (void);

void conf_set_theme (gchar * value);
void conf_set_configuration (gchar * value);
void conf_set_control_key (gint i, guint keyval);
void conf_set_enable_sound (gboolean value);
void conf_set_enable_splats (gboolean value);
void conf_set_use_safe_moves (gboolean value);
void conf_set_use_super_safe_moves (gboolean value);
void conf_set_show_toolbar (gboolean value);

/**********************************************************************/


#endif /* PROPERTIES_H */
