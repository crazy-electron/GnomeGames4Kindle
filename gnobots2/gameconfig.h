#ifndef GAMECONFIG_H
#define GAMECONFIG_H

#include <glib.h>


/**********************************************************************/
/* Defines                                                            */
/**********************************************************************/
#define GAME_CONFIG_DESCLEN 128
/**********************************************************************/


/**********************************************************************/
/* Game Config Structure                                              */
/**********************************************************************/
typedef struct _GameConfig GameConfig;

struct _GameConfig {
  GString *description;
  gint initial_type1;
  gint initial_type2;
  gint increment_type1;
  gint increment_type2;
  gint maximum_type1;
  gint maximum_type2;
  gint score_type1;
  gint score_type2;
  gint score_type1_waiting;
  gint score_type2_waiting;
  gint score_type1_splatted;
  gint score_type2_splatted;
  gint num_robots_per_safe;
  gint safe_score_boundary;
  gint initial_safe_teleports;
  gint free_safe_teleports;
  gint max_safe_teleports;
  gboolean moveable_heaps;
};
/**********************************************************************/


/**********************************************************************/
/* Exported functions                                                 */
/**********************************************************************/
gboolean load_game_configs (void);
gboolean free_game_configs (void);
gint num_game_configs (void);
gchar *game_config_name (gint);
gchar *game_config_filename (gint);
GameConfig *game_config (void);
GameConfig *game_config_settings (gint);
gint current_game_config (void);
gboolean set_game_config (gint);
/**********************************************************************/


#endif /* GAMECONFIG_H */
